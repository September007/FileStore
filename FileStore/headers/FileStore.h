// FileStore.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include<JounralingObjectStore.h>

class FileStore :public JournalingObjectStore {

public:
	bool Mount(Context* ctx);
	void UnMount();

	~FileStore() {
		UnMount();
	}
	
};