// FileStore.cpp : Defines the entry point for the application.
//

#include "FileStore.h"

using namespace std;

bool FileStore::Mount(Context* ctx) {
	return JournalingObjectStore::Mount(ctx);
}

void FileStore::UnMount() {
	JournalingObjectStore::UnMount();
}
void FileStore::RecordData(const string& key, const string& data) {
	ctx->m_WriteFile(key, data, true);
}
string FileStore::ReadData(const string& key) {
	return ctx->m_ReadFile(key);
}
void FileStore::RemoveData(const string& key) {
	if (filesystem::is_regular_file(key))
		filesystem::remove(key);
}
void FileStore::CopyData(const string& keyFrom, const string& keyTo) {
	auto noncvPath = keyTo;
	auto parentDir = GetParentDir(noncvPath);
	if (!filesystem::is_directory(parentDir))
		filesystem::create_directories(parentDir);
	filesystem::copy(keyFrom, parentDir, filesystem::copy_options::overwrite_existing);
}