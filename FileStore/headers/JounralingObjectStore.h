/***********************************************************************/
/**
 * \file   JounralingObjectStore.h
 * \author Semptember007
 * \date   May 2022
 *//*********************************************************************/
#pragma once
#include <map>
#include <objectMap.h>
#include <objectStore.h>

#include <thread_pool.h>
/** index type of callback*/
using CallBackIndex = uint64_t;

/**
 * @brief implement basic journal logic.
 */
class DLL_INTERFACE_API JournalingObjectStore : public ObjectStore {
protected:
	/** journal storage path,using as rb root path*/
	std::string										   journalpath;
	/** thread pool, use to execute io such write,read and callbacks*/
	GD::ThreadPool									   callback_workers;
	/** access to callbacks*/
	std::shared_mutex								   access_to_callbacks;
	/** set callbacks in form of key-value,it makes easier to represent callbacks,which may needed
	 * when combine WOPE or ROPE as one,which makes more than one callback is binding to one ope
	 * \sa MergeCallback() SubmitCallbacks()
	 */
	std::map<CallBackIndex, std::vector<CallBackType>> callbacks;
	/** generation of CallBackIndex is determined by this \sa GetNewCallBackIndex */
	std::atomic<CallBackIndex>						   cur_callbackIndex;
	CallBackIndex GetNewCallBackIndex() { return ++cur_callbackIndex; }
	void		  RegisterCallback(CallBackIndex idx, CallBackType call);
	void		  RegisterCallback(CallBackIndex idx, vector<CallBackType> calls);
	void		  MergeCallback(CallBackIndex to, const vector<CallBackIndex>& froms);
	void		  UnregisterCallBack(CallBackIndex idx);
	void		  SubmitCallbacks(CallBackIndex idx);
	/** work for GetNewReferedBlock */
	atomic<decltype(ReferedBlock::serial)> atomic_serial;
	/** inherite from BlockStore */
	ReferedBlock						   GetNewReferedBlock();
	/**
	 * object map.
	 * using Rocksdb for storage
	 * @note initialization using Context::kvpath, Mount when JournalingObjectStore::Mount(),
	 * UnMount() too
	 */
	ObjectMap<true>						   omap;
	/**
	 * as name indicating
	 */
	void		do_wope(WOPE wope, CallBackIndex when_log_done, CallBackIndex when_journal_done,
			   CallBackIndex when_flush_done);
	/** do read operation.
	 *  although do_rope() take a when_read_done,but call { when_read_done } should after set
	 *  rope_result into reference param {rope_reulst} from Submit_rope() ,so when_read_done would
	 *  not call inside do_rope()
	 */
	ROPE_Result do_rope(ROPE rope, CallBackIndex when_read_done);
	void		do_withdraw_wope(WOPE wope, CallBackIndex when_withdraw_done);

	/**
	 * hide this interface.
	 */
private:
	void   Write(const GHObject_t& ghobj, const string& data) override;
	string Read(const GHObject_t& ghobj) override;
	void   Modify(
		  const GHObject_t& ghobj, int modifiedArea_start, int len, const string& data) override;

public:
	opeIdType				  GetOpeId(const WOPE& wope);
	opeIdType				  GetOpeId(const ROPE& rope);
	/**
	 * use this to compare with omap.logs to distinguish  unfinished loged-before wope
	 * with logs which create after boot-time
	 */
	chrono::system_clock::rep boot_time;
	/**
	 * reload unfinished wope .
	 * @todo reload rope
	 */
	void					  RePlay();

public:
	/**
	 * most important thing is to set boot_time
	 */
	JournalingObjectStore()
		: ObjectStore()
		, journalpath()
		, callbacks()
		, cur_callbackIndex(0)
		, callback_workers(0)
		, boot_time(chrono::system_clock::now().time_since_epoch().count())
	{
	}
	~JournalingObjectStore() /**<*/ { UnMount(); }
	bool Mount(Context* ctx); /**<*/
	void UnMount();			  /**<*/

	/**
	 * submit wope as transcation.
	 * @note if this function return, that mean this transcation is submitted successfully.
	 * @note submit any ope is forbidden before call Mount().
	 * @param wope as name indicating
	 * @param when_log_done			callback which will be submit when log done
	 * @param when_journal_done		callback which will be submit when journal done
	 * @param when_flush_done		callback which will be submit when flush done
	 */
	void Submit_wope(WOPE wope, CallBackType when_log_done, CallBackType when_journal_done,
		CallBackType when_flush_done);
	/**
	 * submit rope as transcation.
	 * @note this transcation is simple as common asynchronous exexcution
	 * @note submit any ope is forbidden before call Mount().
	 * @param rope					as name indicating
	 * @param rope_result			using to carring returning read result
	 * @param when_read_done		callback which will be submit when read done
	 */
	void Submit_rope(ROPE rope, ROPE_Result& rope_reulst, CallBackType when_read_done);

	/**
	 * withdraw wope.
	 * @note this wope only need member WOPE::gh and WOPE::new_gh of original wope,which are enough
	 * info to withdraw
	 * @param wope					as name indicating
	 * @param when_withdraw_done	callback which will be submit when withdraw done
	 * @todo delete refered block whose ReferedBlock::refer_count is 0
	 */
	void WithDraw_wope(WOPE wope, CallBackType when_withdraw_done);
};