#include <logger.h>
#include <objectMap.h>
using namespace std;

bool ObjectMap::Mount(Context* context)
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

bool ObjectMap::UnMount()
{
	if (db != nullptr)
		db->Close();
	db = nullptr;
	return true;
}

rocksdb::Status ObjectMap::Write_Meta(const string& key, const string& value)
{
	auto ret = db->Put(rocksdb::WriteOptions(), key, value);
	return ret;
}

rocksdb::Status ObjectMap::Read_Meta(const string& key, string& value)
{
	auto status = db->Get(rocksdb::ReadOptions(), key, &value);
	return status;
}

rocksdb::Status ObjectMap::Erase_Meta(const string& key)
{
	return db->Delete(rocksdb::WriteOptions(), key);
}
vector<pair<string, string>> ObjectMap::GetMatchPrefix(const string& prefix)
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

rocksdb::Status ObjectMap::EraseMatchPrefix(const string& prefix)
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
bool ObjectMap::beginWith(const string& pre, const string& str)
{
	if (pre.size() > str.size())
		return false;
	return str.substr(0, pre.size()) == pre;
}