#pragma once
#include <config.h>
#include <context.h>
#include <functional>
#include <object.h>
#include <referedBlock.h>
#include <string>

using CallBackType = std::function<void()>;
using std::string;
/**
 * .
 */
class StoreInterface {
public:
	virtual ~StoreInterface() {};
	// using string as unified key of stored data,other key like GHObject should
	// serialize
	virtual void   RecordData(const string& key, const string& data)	= 0;
	virtual string ReadData(const string& key)							= 0;
	virtual void   RemoveData(const string& key)						= 0;
	virtual void   CopyData(const string& keyFrom, const string& keyTo) = 0;
};
/**
 * storage for block data
 */
class BlockStore : public StoreInterface {
protected:
	// deprecated
	string	 rbPath;
	Context* ctx;

protected:
	BlockStore()
		: rbPath()
		, ctx(nullptr)
	{
	}
	bool Mount(Context* ctx)
	{
		rbPath	  = ctx->rbPath;
		this->ctx = ctx;
		return true;
	}
	void UnMount() {}

	void EraseBlock(ReferedBlock rb)
	{
		auto path = GetReferedBlockStoragePath(rb, rbPath);
		if (filesystem::is_regular_file(path))
			filesystem::remove(path);
	}
	ReferedBlock addNewReferedBlock(string data, string root_path)
	{
		auto rb		   = ReferedBlock::getNewReferedBlock();
		rb.refer_count = 0;
		auto rbPath	   = GetReferedBlockStoragePath(rb, root_path);
		// ctx->m_WriteFile(rbPath, data, true);
		StoreInterface::RecordData(rbPath, data);
		LOG_INFO("rb_log",
			fmt::format("add new rb[{} ref:{} ,path:{}]", rb.serial, rb.refer_count,
				GetReferedBlockStoragePath(rb, root_path)));
		return rb;
	}
	inline string ReadReferedBlock(int64_t serial, string root_path)
	{
		auto path = GetReferedBlockStoragePath(serial, root_path);
		// return ctx->m_ReadFile(path);
		return StoreInterface::ReadData(path);
	}
	inline string ReadObjectWithRB(ObjectWithRB orb, string root_path)
	{
		string ret;
		for (auto rb : orb.serials_list)
			ret += ReadReferedBlock(rb, root_path);
		return ret;
	}
	// check data.length is obligated with fs
	inline void WriteReferedBlock(ReferedBlock rb, string root_path, string data)
	{
		auto path = GetReferedBlockStoragePath(rb, root_path);
		// ctx->m_WriteFile(path, data, true);
		StoreInterface::RecordData(path, data);
	}
	inline string GetReferedBlockStoragePath(const ReferedBlock& rb, string root_path)
	{
		return ctx->m_GetReferedBlockStoragePath(rb, root_path);
	}
};

class ObjectStore : public BlockStore {
protected:
	// cuz blockstore already set path up(rbpath),and seems like we need fspath
	// also
	std::string& fspath;

public:
	static ObjectStore* create(
		const string& type, const string& path, const string& journal_path, const string& kv_path);

	ObjectStore()
		: fspath(BlockStore::rbPath)
	{
	}

	bool Mount(Context* ctx)
	{
		// fspath = ctx->fsPath;
		return BlockStore::Mount(ctx);
	}
	void UnMount() { BlockStore::UnMount(); }
	/**
	 * only these interface for object store
	 */
	virtual void   Write(const GHObject_t& ghobj, const string& data) = 0;
	virtual string Read(const GHObject_t& ghobj)					  = 0;
	/**
	 * erase [modi_start,modistart+len) , and insert data at mmodi_start
	 */
	virtual void Modify(
		const GHObject_t& ghobj, int modifiedArea_start, int len, const string& data)
		= 0;
};
