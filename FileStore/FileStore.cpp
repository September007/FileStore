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