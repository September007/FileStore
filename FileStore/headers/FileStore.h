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

	//imple the StoreInterface
	void RecordData(const string& key, const string& data) override;
	string ReadData(const string& key) override;
	void RemoveData(const string& key) override;
	void CopyData(const string& keyFrom, const string& keyTo) override;
};