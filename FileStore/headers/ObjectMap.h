/*****************************************************************************************************/
/**
 * \file   ObjectMap.h
 * \brief  define objectMap which support object attr storage , or in another word, table storage
 *//**************************************************************************************************/
#pragma once
#include <assistant_utility.h>
#include <context.h>
#include <port.h>
#include <rocksdb/db.h>
#include <simulate_table.h>
#include <typeindex>
#include <typeinfo>
using namespace rocksdb;
using std::is_same_v;

/**  ObjectMap doesn't allow simple built-in type as key type */
template <typename T>
concept allowed_table_type
	= !std::is_integral_v<T> && !std::is_floating_point_v<T> && std::is_enum_v<T>;

/**
 * object map.
 * @param with_type_head decide if will add type_head when to key presentation
 */
template <bool with_type_head = true> class ObjectMap {
protected:
	string		 path;
	rocksdb::DB* db;

	mutex _m;

public:
	ObjectMap()
		: path()
		, db(0)
	{
	}
	~ObjectMap() { UnMount(); }
	bool								 Mount(Context* context);
	bool								 UnMount();
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
	template <typename oriType, typename type_head_type = oriType>
	void Erase_Meta(const oriType& key);

	template <typename KeyType, typename ValType>
	void Write_Meta(const KeyType& key, const ValType& val)
	{
		Write_Meta(_Create_type_headed_key<KeyType, KeyType>(key), as_string(val));
	}
	template <typename KeyType, typename ValType> ValType Read_Meta(const KeyType key)
	{
		// performance
		buffer buf;
		Read_Meta(_Create_type_headed_key<KeyType, KeyType>(key), buf.data);
		// if empty,create a new one
		if (buf.data.empty())
			buf = as_buffer(ValType());
		return from_buffer<ValType>(buf);
	}

	virtual rocksdb::DB* GetDB() { return nullptr; }

public:
	bool beginWith(const string& pre, const string& str);
	template <typename type_head_type> const string& Get_Type_Prefix()
	{
		static string ret = as_string(to_string(typeid(remove_cv_t<type_head_type>).hash_code()));
		return ret;
	}
	/**
	 * construct presentation of key with its type prefix.
	 * @param oritype is TableKeyType<T> or KeyType from two template param func
	 * Write_Meta<KeyType,ValTYpe>
	 */
	template <typename oritype, typename type_head_type>
	string _Create_type_headed_key(const oritype& t)
	{
		buffer buf;
		if constexpr (with_type_head) {
			buf.data = Get_Type_Prefix<type_head_type>();
			Write(buf, &t);
		} else {
			Write(buf, &t);
		}
		return move(buf.data);
	}
	/**  construct key from presentation with its type prefix*/
	template <typename oritype, typename type_head_type> oritype _Retrieve_type_headed_key()
	{
		buffer buf;
		if constexpr (with_type_head) {
			// decltype(typeid(type_head_type).hash_code()) hsh;
			// Read(buf, &hsh);
			/** \sa Get_Type_Prefix and _Create_type_headed_key, prefix just a string */
			Read<string>(buf);
			return Read<oritype>(buf);
		} else {
			return Read<oritype>(buf);
		}
	}
};

template <bool with_type_head>
template <TableType T>
inline void ObjectMap<with_type_head>::Write_Meta(T& t)
{
	auto orikey			 = GetKey<T>(t);
	auto type_headed_key = _Create_type_headed_key<TableKeyType<T>, T>(orikey);
	auto val			 = as_string<TableAttrType<T>>(GetAttr(t));
	Write_Meta(type_headed_key, val);
	// auto key = as_string(GetKey<T>(t));
	// auto val = as_string(GetAttr<T>(t));
	// Write_Meta(key, val);
}
/**
 * allow use tabletype or tabletype::keytype to access data
 */
template <bool with_type_head>
template <TableType T>
T ObjectMap<with_type_head>::Read_Meta(const TableKeyType<T>& key)
{
	auto   type_headed_key = _Create_type_headed_key<TableKeyType<T>, T>(key);
	buffer keyBuf		   = as_buffer(key);
	buffer attrBuf;
	Read_Meta(type_headed_key, attrBuf.data);
	return GetAssemble<std::remove_cv_t<T>>(keyBuf, attrBuf);
	// using RT = std::remove_const_t<T>;
	// buffer attrbuf;
	// buffer keyBuf = as_buffer(key);
	// Read_Meta(keyBuf.data, attrbuf.data);
	// return GetAssemble<RT>(keyBuf, attrbuf);
}
template <bool with_type_head>
template <TableType T>
T ObjectMap<with_type_head>::Read_Meta(const T& key)
{
	auto keyk = GetKey(key);
	return Read_Meta<T>(keyk);
}
// template <typename T>
// inline void ObjectMap::Erase_Meta(const TableKeyType<T>& key) {
//     Erase_Meta(as_string(key));
// }
template <bool with_type_head>
template <typename oriType, typename type_head_type>
inline void ObjectMap<with_type_head>::Erase_Meta(const oriType& key)
{
	Erase_Meta(_Create_type_headed_key<oriType, type_head_type>(key));
}
template <bool with_type_head> bool ObjectMap<with_type_head>::Mount(Context* context)
{
	if (path == context->kvpath && db != nullptr)
		return true;
	if (db)
		UnMount();
	path = context->kvpath;
	if (!filesystem::is_directory(path))
		filesystem::create_directories(path);
	try {
		rocksdb::Options options;
		options.create_if_missing = true;
		rocksdb::Status status	  = rocksdb::DB::Open(options, path, &this->db);
		if (!this->db) {
			LOG_ERROR("RocksKV", format("create RocksKV[{}] failed.", path));
			return false;
		}
		return true;
	} catch (std::exception& e) {
		LOG_ERROR("RocksKV", format("catch error[{}]", e.what()));
		return false;
	}
}
/**
 * be sure mutex _m is not busy, so this method should call lock_guard.
 */
template <bool with_type_head> bool ObjectMap<with_type_head>::UnMount()
{
	lock_guard lg(_m);
	if (db != nullptr)
		db->Close();
	db = nullptr;
	return true;
}

template <bool with_type_head>
rocksdb::Status ObjectMap<with_type_head>::Write_Meta(const string& key, const string& value)
{
	lock_guard lg(_m);
	auto	   ret = db->Put(rocksdb::WriteOptions(), key, value);
	return ret;
}

template <bool with_type_head>
rocksdb::Status ObjectMap<with_type_head>::Read_Meta(const string& key, string& value)
{
	lock_guard lg(_m);
	auto	   status = db->Get(rocksdb::ReadOptions(), key, &value);
	return status;
}

template <bool with_type_head>
rocksdb::Status ObjectMap<with_type_head>::Erase_Meta(const string& key)
{
	lock_guard lg(_m);
	return db->Delete(rocksdb::WriteOptions(), key);
}
template <bool with_type_head>
vector<pair<string, string>> ObjectMap<with_type_head>::GetMatchPrefix(const string& prefix)
{
	auto						 it = db->NewIterator(rocksdb::ReadOptions());
	vector<pair<string, string>> ret;
	for (it->SeekForPrev(prefix); it->Valid(); it->Next()) {
		if (beginWith(prefix, it->key().ToString()))
			ret.push_back({ string(it->key().ToString()), string(it->value().ToString()) });
	}
	// dont forget to delete it
	delete it;
	return ret;
}

template <bool with_type_head>
rocksdb::Status ObjectMap<with_type_head>::EraseMatchPrefix(const string& prefix)
{
	rocksdb::Slice start, end;
	// set start and end
	auto		   it = db->NewIterator(ReadOptions());

	for (it->SeekForPrev(prefix); beginWith(prefix, it->key().ToString()); it->Next()) {
		db->Delete(WriteOptions(), it->key());
	}
	auto ret = it->status();

	delete it;
	return ret;
}
template <bool with_type_head>
bool ObjectMap<with_type_head>::beginWith(const string& pre, const string& str)
{
	if (pre.size() > str.size())
		return false;
	return str.substr(0, pre.size()) == pre;
}