#pragma once
#include <object.h>
#include <string>
using std::string;


class Context {
  public:
    /**
     * as name indicating
     */
    string fsPath;
    string journalPath;
    string kvPath;
    string rbPath;

    int default_block_size;
    int journal_callback_worker_count;
    int journal_write_worker_count;

    /**
    * this is used by  GetOpeId() to generate opeid
    * then the journalObjectStore will retrieve undone ope through these head in omap
    */
    string wope_log_head;
    string rope_log_head;
    Context();
    bool load(string name);
    // them often set by load
    std::function<bool(const string&, const string&, const bool)> m_WriteFile;
    std::function<string(const string&)> m_ReadFile;
    std::function<string(const ReferedBlock& rb, string root_pat)>
        m_GetReferedBlockStoragePath;
    /**
    *  when journalingObjectStore doing RePlay(), actually we have lost original
    *  callbacks,but this is inevitable the below three is born to be call when 
    *  replay reload unfinished wope log
    */
    std::function<void(const WOPE& wope)> replay_default_callback_when_log_done;
    std::function<void(const WOPE& wope)> replay_default_callback_when_journal_done;
    std::function<void(const WOPE& wope)> replay_default_callback_when_flush_done;

    // io accelerate
    // bool using_iocp = false;
    // bool using_epoll = false;
    bool using_io_acc = false;
};