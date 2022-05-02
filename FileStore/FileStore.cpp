// FileStore.cpp : Defines the entry point for the application.
//

#include "FileStore.h"

using namespace std;

bool FileStore::Mount(Context* ctx) { return JournalingObjectStore::Mount(ctx); }

void FileStore::UnMount() { JournalingObjectStore::UnMount(); }
void FileStore::RecordData(const string& key, const string& data)
{
	ctx->m_WriteFile(key, data, true);
}
string FileStore::ReadData(const string& key) { return ctx->m_ReadFile(key); }
/** no matter file or dir,this will work,depending on filesystem::remove_all*/
void FileStore::RemoveData(const string& key) { filesystem::remove_all(key); }
void FileStore::CopyData(const string& keyFrom, const string& keyTo)
{
	auto parentDir = filesystem::path(keyTo).parent_path();
	if (!filesystem::is_directory(parentDir))
		filesystem::create_directories(parentDir);
	filesystem::copy(keyFrom, parentDir, filesystem::copy_options::overwrite_existing);
}