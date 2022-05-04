/******************************************************************/
/**
 * \file   ObjectStore.h
 * \brief  define ObjectStore and its parent interface class
 *//****************************************************************/
#pragma once
#include <config.h>
#include <context.h>
#include <functional>
#include <object.h>

#include <string>
// since ObjectStore inmport Context ctx as member ,we can use this macro
#define CTX_LOG_INFO(logname, msg)                                                                 \
	LOG_INFO(logname, fmt::format("{}:{}", Get_Thread_Id(), msg), true, "", ctx->logpath);
#define CTX_LOG_WARN(logname, msg)                                                                 \
	LOG_WARN(logname, fmt::format("{}:{}", Get_Thread_Id(), msg), true, "", ctx->logpath);
#define CTX_LOG_ERROR(logname, msg)                                                                \
	LOG_ERROR(logname, fmt::format("{}:{}", Get_Thread_Id(), msg), true, "", ctx->logpath);

/*! \fn typedef std::function<void()> CallBackType;
 *  callback type
 */
typedef std::function<void()> CallBackType;
using std::string;
/**
 * @class
 * @brief this class define basic store interface.
 * @note  like journalingObjectStore on the deriving tree didn't implement these interface ,so
 * actually that mean journlaingObjectStore,ObjecStore, and BlockStore are virtual class
 */
class DLL_INTERFACE_API StoreInterface {
public:
	virtual ~StoreInterface() {};
	/**
	 * @brief write file, in filestore.
	 *  using file path as key
	 */
	virtual void   RecordData(const string& key, const string& data) = 0;
	/**
	 * @brief read file, in filestore.
	 *  using file path as key, in filestore
	 * @retval the file content
	 */
	virtual string ReadData(const string& key) = 0;
	/** delete file, in filestore. using file path as key.*/
	virtual void   RemoveData(const string& key) = 0;
	/**
	 * no matter what means key is symbol of, the implements should support it.
	 * in filestore, key is file or directory path, which require implements have the ablity to
	 * decide what to do with what it is.
	 */
	virtual void   CopyData(const string& keyFrom, const string& keyTo) = 0;
};
/**
 * @class
 * @brief storage interface for block data.
 */
class DLL_INTERFACE_API BlockStore : public StoreInterface {
protected:
	string	 rbpath; /**< refered block storage path */
	Context* ctx;	 /**< from this class import Context member */

protected:
	BlockStore()
		: rbpath()
		, ctx(nullptr)
	{
	}
	bool Mount(Context* ctx) /**< set member #rbpath and #ctx */
	{
		rbpath	  = ctx->rbpath;
		this->ctx = ctx;
		return true;
	}
	void UnMount() {} /**< nothing to do but standard behavior*/
	/**
	 * as name indicating erase block.
	 * @param rb ReferedBlock
	 */
	void EraseBlock(ReferedBlock rb)
	{
		auto path = GetReferedBlockStoragePath(rb, rbpath);
		RemoveData(path);
	}
	/**
	 * add new ReferedBlock to physical storage device, this rely on StoreInterface::RecordData().
	 * @param data the data the referedblock will carring
	 * @param root_path the root path of rb
	 * @return just created ReferedBlock
	 * @note journal and disk(or filestore itself) have different root path, naming
	 * JournalingObjectStore::journalpath and ObjectStore::fspath
	 */
	ReferedBlock addNewReferedBlock(string data, string root_path)
	{
		auto rb		   = ReferedBlock::getNewReferedBlock();
		rb.refer_count = 0;
		auto rbPath	   = GetReferedBlockStoragePath(rb, root_path);
		// ctx->m_WriteFile(rbPath, data, true);
		RecordData(rbPath, data);
		CTX_LOG_INFO("rb_log",
			fmt::format("add new rb[{} ref:{} ,path:{}]", rb.serial, rb.refer_count,
				GetReferedBlockStoragePath(rb, root_path)));
		return rb;
	}
	/**
	 * read referedblock, rely on StoreInterface::ReadData(string).
	 * @param serial rb serial
	 * @param root_path root path of where you want to read this rb
	 * @return the ReferedBlock data
	 * @sa this is colaborate with addNewReferedBlock
	 */
	inline string ReadReferedBlock(int64_t serial, string root_path)
	{
		auto path = GetReferedBlockStoragePath(serial, root_path);
		// return ctx->m_ReadFile(path);
		return ReadData(path);
	}
	/**
	 * read a ObjectWithRB.
	 * @param orb as name indicating
	 * @param root_path the root path of all ReferedBlock of orb
	 * @return linear compound result of all ReferedBlock data
	 * @sa ReadReferedBlock
	 */
	inline string ReadObjectWithRB(ObjectWithRB orb, string root_path)
	{
		string ret;
		for (auto rb : orb.serials_list)
			ret += ReadReferedBlock(rb, root_path);
		return ret;
	}
	/**
	 * Write ReferedBlock .
	 * @note this is encapsulate interface of StoreInterface::RecordData(string)
	 * @todo check data.length is obligated to FileStore, if neccessary
	 */
	inline void WriteReferedBlock(ReferedBlock rb, string root_path, string data)
	{
		auto path = GetReferedBlockStoragePath(rb, root_path);
		// ctx->m_WriteFile(path, data, true);
		RecordData(path, data);
	}
	/**
	 * Get path of referedblock storage path.
	 * @note extract this interface is result of Context test ,which is in
	 *  rb_deep.cpp, which is designed to find best directory depth of storage path
	 */
	inline string GetReferedBlockStoragePath(const ReferedBlock& rb, string root_path)
	{
		return ctx->m_GetReferedBlockStoragePath(rb, root_path);
	}
};
/**
 * @class object interface .
 * @note seems like these inerfaces are not recommended, otherwise what's journalingObecjtStore for
 * ?
 */
class DLL_INTERFACE_API ObjectStore : public BlockStore {
protected:
	/**  cuz blockstore already have storage path(BlockStore::rbpath), and seems like we need fspath
	 * ,so set fspath as reference*/
	std::string& fspath;

public:
	/**
	 * not use yet.
	 */
	static ObjectStore* create(
		const string& type, const string& path, const string& journal_path, const string& kv_path);
	/**
	 *  @brief set fspath as reference to BlockStore::rbpath
	 */
	ObjectStore()
		: fspath(BlockStore::rbpath)
	{
	}
	/** nothing to do but recursively call BlockStore::Mount() */
	bool Mount(Context* ctx) /**< nothing to do but recursively call BlockStore::Mount() */
	{
		// fspath = ctx->fsPath;
		return BlockStore::Mount(ctx);
	}
	void UnMount() /**< nothing to do but recursively call BlockStore::UnMount() */
	{
		BlockStore::UnMount();
	}
	/**
	 * write a GHObject_t
	 */
	virtual void   Write(const GHObject_t& ghobj, const string& data) = 0;
	/**
	 * read a GHObject_T.
	 * @return data of GHObject_t, rely on BlockStore::ReadObjectWithRB()
	 */
	virtual string Read(const GHObject_t& ghobj) = 0;
	/**
	 * erase [modi_start,modistart+len) , and insert data at mmodi_start.
	 * not use yet
	 */
	virtual void   Modify(
		  const GHObject_t& ghobj, int modifiedArea_start, int len, const string& data)
		= 0;
};
