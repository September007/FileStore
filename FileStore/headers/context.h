#pragma once
#include <object.h>
#include <string>
using std::string;

/**
 * @class Context
 * @brief contain setting for FileStore and its parent class.
 * @note the context::load() does not set everything up yet, some setting member
 * can be customize as wished, seek benchmark for sample
 */
class Context {
public:
	/** filestore path,  where journal block will be move to when flush some wope and rope will seek
	 * data from.  */
	string fsPath;
	/** journal path,    where journal block will be write. */
	string journalPath;
	/** kv path,         where to mount RocksKV.*/
	string kvPath;
	/** rb path,         where to store referedblock data in blockstore, this is \deprecated. */
	string rbPath;
	/** not used yet */
	int default_block_size;
	/** the count of threads of JournalingObjectStore::callback_worker */
	int journal_callback_worker_count;
	/** not used yet */
	int journal_write_worker_count;

	/**
	 * this is used by  GetOpeId() to generate opeid then the journalObjectStore can retrieve
	 * unfinished ope through this head in omap
	 */
	string wope_log_head;
	/**
	 * rope_log_head will be used to generated opeid but not used yet, because don't see any benefit
	 * of read operation records yet.
	 */
	string rope_log_head;
	Context();
	/**
	 * @brief according to config file specified by $name, to set up setting.
	 * @param[in]	name: specified config name, will be call like GetConfig(name, "config",
	 * "windows.context", true), see GetConfig for detail
	 * @return if successfully load coonfig return true ,otherwise like when encounter
	 * json::parse_error will return false,
	 * @log context
	 */
	bool load(string name);
	/** method of writefile */
	std::function<bool(const string&, const string&, const bool)> m_WriteFile;
	/** method of readfile */
	std::function<string(const string&)> m_ReadFile;
	/** method of get path od referd block, differs in dirs deep which influence dirs cache reuse */
	std::function<string(const ReferedBlock& rb, string root_pat)> m_GetReferedBlockStoragePath;
	/**
	 *  when journalingObjectStore doing RePlay(), actually we have lost
	 * original callbacks,but this is inevitable the belows replay_default_callback_* is born to be
	 * call when replay reload unfinished wope log
	 */
	std::function<void(const WOPE& wope)> replay_default_callback_when_log_done;
	/** see [replay_default_callback_when_log_done](#replay_default_callback_when_log_done) */
	std::function<void(const WOPE& wope)> replay_default_callback_when_journal_done;
	/** see [replay_default_callback_when_log_done](#replay_default_callback_when_log_done) */
	std::function<void(const WOPE& wope)> replay_default_callback_when_flush_done;

	/**
	 * whether using epoll or iocp ,for this filestore, which give up these feature,
	 * this maybe useful when add osd socket listening.
	 * \todo clear
	 */
	bool using_io_acc = false;
};