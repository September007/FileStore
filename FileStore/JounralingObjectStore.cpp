#include "JounralingObjectStore.h"
#include <JounralingObjectStore.h>

void JournalingObjectStore::RegisterCallback(CallBackIndex idx, CallBackType call)
{
	unique_lock lg(access_to_callbacks);
	callbacks[idx].push_back(move(call));
}

void JournalingObjectStore::RegisterCallback(CallBackIndex idx, vector<CallBackType> calls)
{
	unique_lock lg(access_to_callbacks);
	callbacks[idx].insert(callbacks[idx].end(), calls.begin(), calls.end());
}

void JournalingObjectStore::MergeCallback(CallBackIndex to, const vector<CallBackIndex>& froms)
{
	unique_lock lg(access_to_callbacks);
	auto&		pto = callbacks[to];
	for (auto f : froms)
		if (f != to) {
			auto p = callbacks.find(f);
			if (p != callbacks.end()) {
				pto.insert(pto.end(), p->second.begin(), p->second.end());
				callbacks.erase(p);
			}
		}
}

void JournalingObjectStore::UnregisterCallBack(CallBackIndex idx)
{
	unique_lock lg(access_to_callbacks);
	callbacks.erase(idx);
}

void JournalingObjectStore::SubmitCallbacks(CallBackIndex idx)
{
	vector<CallBackType> tasks;
	{
		unique_lock lg(access_to_callbacks);
		tasks = move(callbacks[idx]);
	}
	UnregisterCallBack(idx);
	for (auto& cb : tasks)
		callback_workers.enqueue(move(cb));
}

ReferedBlock JournalingObjectStore::GetNewReferedBlock() { return ++atomic_serial; }

bool JournalingObjectStore::Mount(Context* ctx)
{
	journalpath = ctx->journalpath;
	callback_workers.active(ctx->journal_callback_worker_count);

	omap.Mount(ctx);
	ObjectStore::Mount(ctx);
	/** load atomic_serail */
	auto rbprefix	= omap.Get_Type_Prefix<ReferedBlock>();
	auto previousRB = omap.GetMatchPrefix(rbprefix);
	using rbsT		= decltype(ReferedBlock::serial);
	using rbrT		= decltype(ReferedBlock::refer_count);
	set<ReferedBlock> sr;
	for (auto& prb : previousRB) {
		buffer buf(prb.first), sbuf(prb.second);
		auto   prefix = ::Read<string>(buf);
		CTX_LOG_EXPECT_EQ(ctx->name, prefix, rbprefix, "");
		auto rb_s	  = ::Read<rbsT>(buf);
		auto rb_refer = ::Read<rbrT>(sbuf);
		sr.insert(ReferedBlock(rb_s, rb_refer));
	}
	this->atomic_serial = (sr.rbegin() != sr.rend() ? sr.rbegin()->serial : 1);
	return true;
}

void JournalingObjectStore::UnMount()
{
	this->callback_workers.shutdown();
	ObjectStore::UnMount();
}

void JournalingObjectStore::Write(const GHObject_t& ghobj, const string& data) {}

string JournalingObjectStore::Read(const GHObject_t& ghobj) { return string(); }

void JournalingObjectStore::Modify(
	const GHObject_t& ghobj, int modifiedArea_start, int len, const string& data)
{
}
void JournalingObjectStore::Submit_wope(WOPE wope, CallBackType when_log_done,
	CallBackType when_journal_done, CallBackType when_flush_done)
{
	CTX_LOG_INFO(ctx->name,
		fmt::format("wope:[ gh:[{}:{}] ,new_gh:[{}:{}] in form of [name:generation]]",
			wope.ghobj.hobj.oid.name, wope.ghobj.generation, wope.new_ghobj.hobj.oid.name,
			wope.new_ghobj.generation));
	auto log_done	  = GetNewCallBackIndex();
	auto journal_done = GetNewCallBackIndex();
	auto flush_done	  = GetNewCallBackIndex();
	RegisterCallback(log_done, move(when_log_done));
	RegisterCallback(journal_done, move(when_journal_done));
	RegisterCallback(flush_done, move(when_flush_done));

	//@wope phase.1 create log
	omap.Write_Meta<opeIdType, WOPE>(GetOpeId(wope), wope);
	SubmitCallbacks(log_done);

	callback_workers.enqueue([wope = move(wope), pthis = this, log_done, journal_done, flush_done] {
		pthis->do_wope(wope, log_done, journal_done, flush_done);
	});
}

void JournalingObjectStore::Submit_rope(
	ROPE rope, ROPE_Result& rope_reulst, CallBackType when_read_done)
{
	CTX_LOG_INFO(ctx->name,
		fmt::format("rope:[ gh:[{}:{}] ] in form of [name:generation]]", rope.ghobj.hobj.oid.name,
			rope.ghobj.generation));
	auto read_done = GetNewCallBackIndex();
	RegisterCallback(read_done, move(when_read_done));
	callback_workers.enqueue([rope = move(rope), read_done, pthis = this, presult = &rope_reulst] {
		auto result = pthis->do_rope(rope, read_done);
		swap(result, *presult);
		pthis->SubmitCallbacks(read_done);
	});
}

void JournalingObjectStore::WithDraw_wope(WOPE wope, CallBackType when_withdraw_done)
{
	auto idx_when_withdraw_done = GetNewCallBackIndex();
	RegisterCallback(idx_when_withdraw_done, when_withdraw_done);
	this->callback_workers.enqueue([wope = move(wope), pthis = this, idx_when_withdraw_done] {
		pthis->do_withdraw_wope(wope, idx_when_withdraw_done);
	});
}

void JournalingObjectStore::do_wope(WOPE wope, CallBackIndex when_log_done,
	CallBackIndex when_journal_done, CallBackIndex when_flush_done)
{
	/**  wope phase.1 is create log, which is done inside   Submit_wope() */

	/** wope phase.2 write journal
	 * 1. read referedblock data from omap
	 * 2. do each sub write ope in the list
	 *		2.1 insert: create new block and insert
	 *		2.2 overwrite : create new and replace
	 *		2.3 delete : just erase block from list
	 * 3. update refered block's refer_count , these data is stored in omap
	 * 4. register new ghobj in the omap
	 */
	// phase2.1
	auto orb = omap.Read_Meta<GHObject_t, ObjectWithRB>(wope.ghobj);
	for (int i = 0; i < wope.block_nums.size(); ++i) {
		auto  block_num	 = wope.block_nums[i];
		auto& block_data = wope.block_datas[i];
		auto  opetype	 = wope.types[i];
		auto  p			 = orb.serials_list.begin();
		for (int k = 0; k < block_num; ++k)
			p++;
		// phase2.2
		switch (opetype) {
		case WOPE::opetype::Delete: {
			orb.serials_list.erase(p);
		} break;
		case WOPE::opetype::Insert: {
			//@dataflow kv rb create
			auto newRb = BlockStore::addNewReferedBlock(block_data, this->journalpath);
			orb.serials_list.insert(p, newRb.serial);
			omap.Write_Meta<ReferedBlock>(newRb);
		} break;
		case WOPE::opetype::OverWrite: {
			auto newRb = BlockStore::addNewReferedBlock(block_data, this->journalpath);
			*p		   = newRb.serial;
			omap.Write_Meta<ReferedBlock>(newRb);
		} break;
		}
	}
	// phase2.3
	//@dataflow kv rb update refer_count
	for (auto& rbs : orb.serials_list) {
		auto rrb = ReferedBlock(rbs);
		auto rb	 = omap.Read_Meta<ReferedBlock>(rrb);
		rb.refer_count++;
		omap.Write_Meta<ReferedBlock>(rb);
	}
	//@wope phase2.4 register new gh
	//@dataflow kv gh create gh:orb
	omap.Write_Meta<GHObject_t, ObjectWithRB>(wope.new_ghobj, orb);
	SubmitCallbacks(when_journal_done);
	/** wope phase.3 log flush
	 *   copy block data to fs path
	 */
	// record this for future phase.4
	list<ReferedBlock> rb_refer1;
	for (auto rbs : orb.serials_list) {
		auto rb = omap.Read_Meta<ReferedBlock>(ReferedBlock(rbs));
		if (rb.refer_count == 1) {
			// mean new block
			rb_refer1.push_back(rb);
			auto to_path   = GetReferedBlockStoragePath(rb, this->fspath);
			auto from_path = GetReferedBlockStoragePath(rb, this->journalpath);
			try {
				CopyData(from_path, to_path);
				CTX_LOG_INFO(ctx->name, format("copy block from {} to {}", from_path, to_path));
			} catch (std::exception& e) {
				cout << e.what() << endl;
			}
		}
	}
	SubmitCallbacks(when_flush_done);
	{
		/** wope phase.4 delete relative log in the omap and log file in journal
		 *  1. get opeid
		 *  2. remove tail time_stamp
		 *  3. delete relative omap log by prefix, which contain ${wope_log_head}+gh+new_gh,check
		 * detail in GetOpeId()
		 */
		auto opeid					 = GetOpeId(wope);
		auto p_time_stamp			 = opeid.find_last_of(":");
		auto opeid_with_no_timestamp = opeid.substr(0, p_time_stamp);
		omap.EraseMatchPrefix(opeid_with_no_timestamp);
		auto id		 = Get_Thread_Id();
		namespace fs = filesystem;
		auto perm	 = [](fs::perms p) -> string {
			   return string() + ((p & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
				   + ((p & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
				   + ((p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-")
				   + ((p & fs::perms::group_read) != fs::perms::none ? "r" : "-")
				   + ((p & fs::perms::group_write) != fs::perms::none ? "w" : "-")
				   + ((p & fs::perms::group_exec) != fs::perms::none ? "x" : "-")
				   + ((p & fs::perms::others_read) != fs::perms::none ? "r" : "-")
				   + ((p & fs::perms::others_write) != fs::perms::none ? "w" : "-")
				   + ((p & fs::perms::others_exec) != fs::perms::none ? "x" : "-");
		};
		for (auto rb_serial : rb_refer1) {
			auto path	   = GetReferedBlockStoragePath(rb_serial, this->journalpath);
			auto path_perm = filesystem::status(path).permissions();
			auto p		   = path_perm & filesystem::perms::owner_write;
			auto rb		   = omap.Read_Meta<ReferedBlock>(rb_serial);
			if (!filesystem::is_regular_file(path) || p == filesystem::perms::none
				|| rb.refer_count == 0) {
				int	 i	  = 0;
				auto pstr = perm(path_perm);
				i++;
			}
			// CTX_LOG_INFO(ctx->name, format("{} will tring to remove path {}", id, path));
			RemoveData(path);
		}
	}
}

ROPE_Result JournalingObjectStore::do_rope(ROPE rope, CallBackIndex when_read_done)
{
	auto		   gh_attr = omap.Read_Meta<GHObject_t, ObjectWithRB>(rope.ghobj);
	vector<string> block_datas(rope.blocks.size());
	// load as vector for simpfiying indexing
	vector<decay_t<decltype(gh_attr.serials_list.front())>> serials;
	for (auto s : gh_attr.serials_list)
		serials.push_back(s);
	for (int i = 0; i < rope.blocks.size(); ++i)
		//@todo: try read hot data from journal cache instead of fs
		block_datas[i] = BlockStore::ReadReferedBlock(serials[rope.blocks[i]], this->fspath);
	return ROPE_Result(GetOpeId(rope), rope.ghobj, rope.blocks, move(block_datas));
}

void JournalingObjectStore::do_withdraw_wope(WOPE wope, CallBackIndex when_withdraw_done)
{
	/**
	 *  1. get original gh and new_gh
	 *  2. query new_gh info from omap
	 *  3. for each refer block set reference--
	 *  4. remove object record (new_gh) in omap
	 */
	auto &gh = wope.ghobj, &new_gh = wope.new_ghobj;
	auto  new_gh_attr = omap.Read_Meta<GHObject_t, ObjectWithRB>(new_gh);

	for (auto& rb_serial : new_gh_attr.serials_list) {
		auto rb = omap.Read_Meta<ReferedBlock>(rb_serial);
		rb.refer_count--;
		omap.Write_Meta<ReferedBlock>(rb);
	}
	omap.Erase_Meta<GHObject_t>(new_gh);
	SubmitCallbacks(when_withdraw_done);
}

/**
 * @brief create unique key for wope
 * the last part of opeid should be the time_stamp of chrono::system_clock::rep
 * which is used by replay to distinguish something ,see comcrete implemntation to RePlay()
 */
opeIdType JournalingObjectStore::GetOpeId(const WOPE& wope)
{
	// add time_stamp
	auto time_stamp = chrono::system_clock::now().time_since_epoch().count();
	return fmt::format("{}{}:{}:{}", ctx->wope_log_head, GetObjUniqueStrDesc(wope.ghobj),
		GetObjUniqueStrDesc(wope.new_ghobj), time_stamp);
}
/**
 * @brief create unique key for rope
 * this is not requested to binding time_stamp appending in the end like GetOpeId(const WOPE&)
 */
opeIdType JournalingObjectStore::GetOpeId(const ROPE& rope)
{
	// add time_stamp
	auto time_stamp = chrono::system_clock::now().time_since_epoch().count();
	return fmt::format("{}{}:{}", ctx->wope_log_head, GetObjUniqueStrDesc(rope.ghobj), time_stamp);
}

void JournalingObjectStore::RePlay()
{

	/**
	 *  1. read logs from omap
	 *  2. filter unfinished logs by recorded time_point
	 *  3. reload each ope
	 */
	// 1
	auto undone_wopes
		= omap.GetMatchPrefix(omap._Create_type_headed_key<decltype(GetOpeId(declval<WOPE>())),
							  decltype(GetOpeId(declval<WOPE>()))>(ctx->wope_log_head));
	// 2
	vector<WOPE> wopes;
	wopes.reserve(undone_wopes.size());
	for (auto& log : undone_wopes) {
		auto key	= log.first;
		auto p_date = key.find_last_of(":");
		if (p_date == string::npos)
			continue;
		chrono::system_clock::rep log_date = stoll(key.substr(p_date + 1));
		// check date
		if (log_date < this->boot_time) {
			auto wope = from_string<WOPE>(log.second);
			wopes.push_back(move(wope));
		}
	}
	// 3
	// \todo : how do we deal with the callbacks?
	for (auto& wope : wopes) {
		this->callback_workers.enqueue([wope = move(wope), pthis = this] {
			auto when_log_done_idx	   = pthis->GetNewCallBackIndex();
			auto when_journal_done_idx = pthis->GetNewCallBackIndex();
			auto when_flush_done_idx   = pthis->GetNewCallBackIndex();
			/**
			when reload wope ,we already lost original callback, so we are gonna use default
			callback specified by Context::replay_default_callback_when_log_done .etc but after
			do_wope() exit after submit callbacks,then fall off this lambda,the stack variable like
			wope would deconstruct,but the callbacks which need wope is not promised to be
			completed,so need make_shared to extend life time of wope
			code like this `auto shared_wope =
			make_shared<WOPE>(move(wope));pthis->ctx->replay_default_callback_when_log_done(shared_wope);
			});`
			*/
			auto shared_wope = make_shared<WOPE>(move(wope));
			pthis->RegisterCallback(when_log_done_idx, [pthis = pthis, shared_wope] {
				pthis->ctx->replay_default_callback_when_log_done(shared_wope);
			});
			pthis->RegisterCallback(when_journal_done_idx, [pthis = pthis, shared_wope] {
				pthis->ctx->replay_default_callback_when_journal_done(shared_wope);
			});
			pthis->RegisterCallback(when_flush_done_idx, [pthis = pthis, shared_wope] {
				pthis->ctx->replay_default_callback_when_flush_done(shared_wope);
			});
			pthis->do_wope(
				*shared_wope.get(), when_log_done_idx, when_journal_done_idx, when_flush_done_idx);
		});
	}
}