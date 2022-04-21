#include<objectMap.h>
#include<logger.h>
using namespace std;

bool ObjectMap::Mount(Context* context) {
	if (path == context->kvPath && db != nullptr)
		return true;
	if (db)
		UnMount();
	path = context->kvPath;
	if (!filesystem::is_directory(path))
		filesystem::create_directories(path);
	try {
		rocksdb::Options options;
		options.create_if_missing = true;
		rocksdb::Status status = rocksdb::DB::Open(options, path, &this->db);
		if (!this->db) {
			LOG_ERROR("RocksKV", format("create RocksKV[{}] failed.", path));
			return false;
		}
	}
	catch (std::exception& e) {
		LOG_ERROR("RocksKV", format("catch error[{}]", e.what()));
		return false;
	}
}

bool ObjectMap::UnMount() {
	if(db!=nullptr) 
		db->Close();
	db = nullptr;
	return true;
}

rocksdb::Status ObjectMap::Write_Meta(const string& key, const string& value) {
	auto ret = db->Put(rocksdb::WriteOptions(), key, value);
	return ret;
}

rocksdb::Status ObjectMap::Read_Meta(const string& key, string& value) {
	auto status = db->Get(rocksdb::ReadOptions(), key, &value);
	return status;
}

rocksdb::Status ObjectMap::Erase_Meta(const string& key) {
	return db->Delete(rocksdb::WriteOptions(), key);
}
