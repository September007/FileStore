#pragma once
#include <map>
#include <objectMap.h>
#include <objectStore.h>
#include <referedBlock.h>
#include <thread_pool.h>
using CallBackIndex = uint64_t;
class JournalingObjectStore : public ObjectStore {
  protected:
    std::string journalPath;
    GD::ThreadPool callback_workers;
    // callback manage
    // mapping callbacks for manage
    std::shared_mutex access_to_callbacks;
    std::map<CallBackIndex, std::vector<CallBackType>> callbacks;
    std::atomic<CallBackIndex> cur_callbackIndex;

    CallBackIndex GetNewCallBackIndex() { return ++cur_callbackIndex; }
    void RegisterCallback(CallBackIndex idx, CallBackType call);
    void RegisterCallback(CallBackIndex idx, vector<CallBackType> calls);
    void MergeCallback(CallBackIndex to, const vector<CallBackIndex>& froms);
    void UnregisterCallBack(CallBackIndex idx);
    void SubmitCallbacks(CallBackIndex idx);

    // log and metadata manage
    ObjectMap omap;
    /**
     * as name indicating
     */
    void do_wope(WOPE wope, CallBackIndex when_log_done,
                 CallBackIndex when_journal_done,
                 CallBackIndex when_flush_done);
    // although do_rope() take a when_read_done,but call {when_read_done} should
    // after set rope_result into reference param {rope_reulst} from
    // Submit_rope() ,so when_read_done would not call inside do_rope()
    ROPE_Result do_rope(ROPE rope, CallBackIndex when_read_done);
    void do_withdraw_wope(WOPE wope, CallBackIndex when_withdraw_done);

    void Write(const GHObject_t& ghobj, const string& data) override;
    string Read(const GHObject_t& ghobj) override;
    void Modify(const GHObject_t& ghobj, int modifiedArea_start, int len,
                const string& data) override;

    //@follow definition of WOPE
    opeIdType GetOpeId(const WOPE& wope);
    //@follow definition of ROPE
    opeIdType GetOpeId(const ROPE& rope);
    /**
    * use this to compare with omap.logs to distinguish  unfinished loged-before wope
    * with logs which create after boot-time
    */
    chrono::system_clock::rep boot_time;
    void RePlay();
  public:
    JournalingObjectStore()
        : ObjectStore(), journalPath(), callbacks(), cur_callbackIndex(0),
          callback_workers(0), boot_time(chrono::system_clock::now().time_since_epoch().count()) {}
    ~JournalingObjectStore() { UnMount(); }
    bool Mount(Context* ctx);
    void UnMount();
    void Submit_wope(WOPE wope, CallBackType when_log_done,
                     CallBackType when_journal_done,
                     CallBackType when_flush_done);

    void Submit_rope(ROPE rope, ROPE_Result& rope_reulst,
                     CallBackType when_read_done);

    // this wope only need gh & new_gh
    void WithDraw_wope(WOPE wope, CallBackType when_withdraw_donw);
};