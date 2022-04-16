#pragma once
#include<ObjectStore.h>
#include<referedBlock.h>
#include<map>
#include<thread_pool.h>
#include<ObjectMap.h>
using CallBackIndex =  uint64_t;
class JournalingObjectStore :public ObjectStore {
protected:
	std::string journalPath;
	GD::ThreadPool callback_workers;
	// callback manage
	//mapping callbacks for manage
	std::shared_mutex access_to_callbacks;
	std::map<CallBackIndex, std::vector<CallBackType>> callbacks;
	std::atomic<CallBackIndex> cur_callbackIndex;

	CallBackIndex GetNewCallBackIndex() { return ++cur_callbackIndex; }
	void RegisterCallback(CallBackIndex idx, CallBackType call);
	void RegisterCallback(CallBackIndex idx, vector<CallBackType> calls);
	void MergeCallback(CallBackIndex to, const vector<CallBackIndex>& froms);
	void UnregisterCallBack(CallBackIndex idx);
	void SubmitCallbacks(CallBackIndex idx);

	//log and metadata manage
	ObjectMap omap;
	/**
	* as name indicating
	*/
	void do_wope(WOPE wope,
		CallBackIndex when_log_done,
		CallBackIndex when_journal_done,
		CallBackIndex when_flush_done
	);
	ROPE_Result do_rope(ROPE rope,
		CallBackIndex when_read_done
	);
public:
	JournalingObjectStore() :ObjectStore(), journalPath()
		,callbacks(),cur_callbackIndex(0),callback_workers(0) {}
	~JournalingObjectStore() { UnMount(); }
	bool Mount(Context* ctx);
	void UnMount();

	void Write(const GHObject_t& ghobj, const string& data)override;
	string Read(const GHObject_t& ghobj)override;
	void Modify(const GHObject_t& ghobj, int modifiedArea_start, int len, const string& data)override;
	void Submit_wope(WOPE wope,
		CallBackType when_log_done,
		CallBackType when_journal_done,
		CallBackType when_flush_done);

	void Submit_rope(ROPE rope,ROPE_Result&rope_reulst,
		CallBackType when_read_done);
};