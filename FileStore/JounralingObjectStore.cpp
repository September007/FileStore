#include <JounralingObjectStore.h>

void JournalingObjectStore::RegisterCallback(CallBackIndex idx,
                                             CallBackType call) {
    unique_lock lg(access_to_callbacks);
    callbacks[idx].push_back(move(call));
}

void JournalingObjectStore::RegisterCallback(CallBackIndex idx,
                                             vector<CallBackType> calls) {
    unique_lock lg(access_to_callbacks);
    callbacks[idx].insert(callbacks[idx].end(), calls.begin(), calls.end());
}

void JournalingObjectStore::MergeCallback(CallBackIndex to,
                                          const vector<CallBackIndex>& froms) {
    unique_lock lg(access_to_callbacks);
    auto& pto = callbacks[to];
    for (auto f : froms)
        if (f != to) {
            auto p = callbacks.find(f);
            if (p != callbacks.end()) {
                pto.insert(pto.end(), p->second.begin(), p->second.end());
                callbacks.erase(p);
            }
        }
}

void JournalingObjectStore::UnregisterCallBack(CallBackIndex idx) {
    unique_lock lg(access_to_callbacks);
    callbacks.erase(idx);
}

void JournalingObjectStore::SubmitCallbacks(CallBackIndex idx) {
    vector<CallBackType> tasks;
    {
        unique_lock lg(access_to_callbacks);
        tasks = move(callbacks[idx]);
    }
    UnregisterCallBack(idx);
    for (auto& cb : tasks)
        callback_workers.enqueue(move(cb));
}

bool JournalingObjectStore::Mount(Context* ctx) {
    journalPath = ctx->journalPath;
    callback_workers.active(ctx->journal_callback_worker_count);

    return omap.Mount(ctx) && ObjectStore::Mount(ctx);
}

void JournalingObjectStore::UnMount() { ObjectStore::UnMount(); }
// todo: object storage support
void JournalingObjectStore::Write(const GHObject_t& ghobj, const string& data) {
}

string JournalingObjectStore::Read(const GHObject_t& ghobj) { return string(); }

void JournalingObjectStore::Modify(const GHObject_t& ghobj,
                                   int modifiedArea_start, int len,
                                   const string& data) {}
void JournalingObjectStore::Submit_wope(WOPE wope, CallBackType when_log_done,
                                        CallBackType when_journal_done,
                                        CallBackType when_flush_done) {
    auto log_done = GetNewCallBackIndex();
    auto journal_done = GetNewCallBackIndex();
    auto flush_done = GetNewCallBackIndex();
    RegisterCallback(log_done, move(when_log_done));
    RegisterCallback(journal_done, move(when_journal_done));
    RegisterCallback(flush_done, move(when_flush_done));
    callback_workers.enqueue(
        [wope = move(wope), pthis = this, log_done, journal_done, flush_done] {
            pthis->do_wope(wope, log_done, journal_done, flush_done);
        });
}

void JournalingObjectStore::Submit_rope(ROPE rope, ROPE_Result& rope_reulst,
                                        CallBackType when_read_done) {
    auto read_done = GetNewCallBackIndex();
    RegisterCallback(read_done, move(when_read_done));
    callback_workers.enqueue(
        [rope = move(rope), read_done, pthis = this, presult = &rope_reulst] {
            auto result = pthis->do_rope(rope, read_done);
            swap(result, *presult);
            pthis->SubmitCallbacks(read_done);
        });
}

void JournalingObjectStore::WithDraw_wope(WOPE wope,
                                          CallBackType when_withdraw_donw) {
    auto idx_when_withdraw_donw = GetNewCallBackIndex();
    RegisterCallback(idx_when_withdraw_donw, when_withdraw_donw);
    this->callback_workers.enqueue(
        [wope = move(wope), pthis = this, idx_when_withdraw_donw] {
            pthis->do_withdraw_wope(wope, idx_when_withdraw_donw);
        });
}

void JournalingObjectStore::do_wope(WOPE wope, CallBackIndex when_log_done,
                                    CallBackIndex when_journal_done,
                                    CallBackIndex when_flush_done) {
    //@wope phase.1 log
    omap.Write_Meta<opeIdType, WOPE>(GetOpeId(wope), wope);
    SubmitCallbacks(when_log_done);
    //@wope phase.2 write journal
    /**
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
        auto block_num = wope.block_nums[i];
        auto& block_data = wope.block_datas[i];
        auto opetype = wope.types[i];
        auto p = orb.serials_list.begin();
        for (int k = 0; k < block_num; ++k)
            p++;
        // phase2.2
        switch (opetype) {
        case WOPE::opetype::Delete: {
            orb.serials_list.erase(p);
        } break;
        case WOPE::opetype::Insert: {
            //@dataflow kv rb create
            auto newRb =
                BlockStore::addNewReferedBlock(block_data, this->journalPath);
            orb.serials_list.insert(p, newRb.serial);
            omap.Write_Meta<ReferedBlock>(newRb);
            //{
            //	auto rb_attr = kv->GetAttr(newRb);
            //	LOG_INFO("rb_log", fmt::format("create new rb[{} ref: {}]",
            //		rb_attr.first.serial, rb_attr.first.refer_count));
            //}
        } break;
        case WOPE::opetype::OverWrite: {
            auto newRb =
                BlockStore::addNewReferedBlock(block_data, this->journalPath);
            *p = newRb.serial;
            omap.Write_Meta<ReferedBlock>(newRb);
            //{
            //	auto rb_attr = kv->GetAttr(newRb);
            //	LOG_INFO("rb_log", fmt::format("create new rb[{} ref: {}]",
            //		rb_attr.first.serial, rb_attr.first.refer_count));
            //}
        } break;
        }
    }
    // phase2.3
    //@dataflow kv rb update refer_count
    for (auto& rbs : orb.serials_list) {
        auto rrb = ReferedBlock(rbs);
        auto rb = omap.Read_Meta<ReferedBlock>(rrb);
        rb.refer_count++;
        omap.Write_Meta<ReferedBlock>(rb);
        //{
        //	auto rb_attr_new = kv->GetAttr(rrb);
        //	LOG_INFO("rb_log",
        //		fmt::format("journal set rb[{} ref_count={}]
        // ref_count++,now rb.ref_count={}",
        // rrb.serial,rrb.refer_count, rb_attr_new.first.refer_count));
        //}
    }
    //@wope phase2.4 register new gh
    //@dataflow kv gh create gh:orb
    omap.Write_Meta<GHObject_t, ObjectWithRB>(wope.new_ghobj, orb);
    SubmitCallbacks(when_journal_done);
    //@wope phase.3 log flush
    /**
     * 1. copy block data to fs path
     */
    for (auto rbs : orb.serials_list) {
        auto rb = omap.Read_Meta<ReferedBlock>(ReferedBlock(rbs));
        if (rb.refer_count == 1) {
            // mean new block
            auto to_path = GetReferedBlockStoragePath(rb, this->fspath);
            auto from_path = GetReferedBlockStoragePath(rb, this->journalPath);
            auto to_path_parent_dir = GetParentDir(to_path);
            try {
                if (!filesystem::is_regular_file(to_path)) {
                    if (!filesystem::is_directory(to_path_parent_dir))
                        filesystem::create_directories(to_path_parent_dir);
                    filesystem::copy(
                        from_path, to_path_parent_dir,
                        filesystem::copy_options::overwrite_existing);
                    LOG_INFO("wope", format("copy block from {} to {}",
                                            from_path, to_path));
                }
            } catch (std::exception& e) {
                cout << e.what() << endl;
            }
        }
    }
    SubmitCallbacks(when_flush_done);
}

ROPE_Result JournalingObjectStore::do_rope(ROPE rope,
                                           CallBackIndex when_read_done) {
    auto gh_attr = omap.Read_Meta<GHObject_t, ObjectWithRB>(rope.ghobj);
    vector<string> block_datas(rope.blocks.size());
    // load as vector for simpfiying indexing
    vector<decay_t<decltype(gh_attr.serials_list.front())>> serials;
    for (auto s : gh_attr.serials_list)
        serials.push_back(s);
    for (int i = 0; i < rope.blocks.size(); ++i)
        //@todo: try read hot data from journal cache instead of fs
        block_datas[i] =
            BlockStore::ReadReferedBlock(serials[rope.blocks[i]], this->fspath);
    return ROPE_Result(GetOpeId(rope), rope.ghobj, rope.blocks,
                       move(block_datas));
}

void JournalingObjectStore::do_withdraw_wope(WOPE wope,
                                             CallBackIndex when_withdraw_done) {
    /**
    *  1. get original gh and new_gh
    *  2. query new_gh info from omap
    *  3. for each refer block set reference--
    *  4. remove object record in omap
    */
    auto &gh = wope.ghobj, &new_gh = wope.new_ghobj;
    auto new_gh_attr = omap.Read_Meta<GHObject_t, ObjectWithRB>(new_gh);
    
    for (auto& rb_serial : new_gh_attr.serials_list) {
        auto rb = omap.Read_Meta<ReferedBlock>(rb_serial);
        rb.refer_count--;
        omap.Write_Meta<ReferedBlock>(rb);
    }
    omap.Erase_Meta<GHObject_t>(new_gh);
    SubmitCallbacks(when_withdraw_done);
}