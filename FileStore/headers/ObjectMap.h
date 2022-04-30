#pragma once
#include <assistant_utility.h>
#include <context.h>
#include <rocksdb/db.h>
#include <simulate_table.h>
using namespace rocksdb;
using std::is_same_v;
class ObjectMap {
protected:
	string		 path;
	rocksdb::DB* db;

public:
	ObjectMap()
		: path()
		, db(0)
	{
	}
	~ObjectMap() { UnMount(); }
	bool Mount(Context* context);
	bool UnMount();
	// meta data interface
	virtual rocksdb::Status				 Write_Meta(const string& key, const string& value);
	virtual rocksdb::Status				 Read_Meta(const string& key, string& value);
	virtual rocksdb::Status				 Erase_Meta(const string& key);
	virtual vector<pair<string, string>> GetMatchPrefix(const string& prefix);
	virtual rocksdb::Status				 EraseMatchPrefix(const string& prefix);
	template <TableType T> void			 Write_Meta(T& t);
	template <TableType T> T			 Read_Meta(const TableKeyType<T>& key);
	template <TableType T> T			 Read_Meta(const T& key);
	// template <typename T>
	// void Erase_Meta(const TableKeyType<T>& key);
	template <typename T> void Erase_Meta(const T& key);

	template <typename KeyType, typename ValType>
	void Write_Meta(const KeyType key, const ValType& val)
	{
		Write_Meta(as_string(key), as_string(val));
	}
	template <typename KeyType, typename ValType> ValType Read_Meta(const KeyType key)
	{
		// performance
		buffer buf;
		Read_Meta(as_string(key), buf.data);
		// if empty,create a new one
		if (buf.data.empty())
			buf = as_buffer(ValType());
		return from_buffer<ValType>(buf);
	}

	virtual rocksdb::DB* GetDB() { return nullptr; }

private:
	bool beginWith(const string& pre, const string& str);
};

template <TableType T> inline void ObjectMap::Write_Meta(T& t)
{
	auto key = as_string(GetKey<T>(t));
	auto val = as_string(GetAttr<T>(t));
	Write_Meta(key, val);
}
/**
 * allow use tabletype or tabletype::keytype to access data
 */
template <TableType T> T ObjectMap::Read_Meta(const TableKeyType<T>& key)
{
	using RT = std::remove_const_t<T>;
	buffer attrbuf;
	buffer keyBuf = as_buffer(key);
	Read_Meta(keyBuf.data, attrbuf.data);
	return GetAssemble<RT>(keyBuf, attrbuf);
}
template <TableType T> T ObjectMap::Read_Meta(const T& key)
{
	auto keyk = GetKey(key);
	return Read_Meta<T>(keyk);
}
// template <typename T>
// inline void ObjectMap::Erase_Meta(const TableKeyType<T>& key) {
//     Erase_Meta(as_string(key));
// }
template <typename T> inline void ObjectMap::Erase_Meta(const T& key)
{
	Erase_Meta(as_string(key));
}