// FileStore.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <JounralingObjectStore.h>
#include <iostream>

class FileStore : public JournalingObjectStore {

public:
	/** actually nothing to do but recurisvely call JournalingObjectStore::Mount(). */
	bool Mount(Context* ctx);
	/** actually nothing to do but recurisvely call JournalingObjectStore::UnMount(). */
	void UnMount();

	/**  only call UnMount().*/
	~FileStore() { UnMount(); }

	/** simply write file. */
	void RecordData(const string& key, const string& data) override;
	/** simply read file.  */
	string ReadData(const string& key) override;
	/** simply remove file or dir */
	void RemoveData(const string& key) override;
	/** simply copy   file or dir */
	void CopyData(const string& keyFrom, const string& keyTo) override;
};