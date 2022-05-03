/*****************************************************************/
/**
 * \file   FileStore.h
 */
/*****************************************************************/

#pragma once
#include <JounralingObjectStore.h>
#include <iostream>
#include <port.h>
/**
 * \brief	filesystem implement of StoreInterface .
 * \author	September007
 * \date	2022-5-2
 */

class DLL_INTERFACE_API FileStore : public JournalingObjectStore {

public:
	/** actually nothing to do but recurisvely call JournalingObjectStore::Mount(). */
	bool Mount(Context* ctx);
	/** actually nothing to do but recurisvely call JournalingObjectStore::UnMount(). */
	void UnMount();

	/**  only call UnMount().*/
	~FileStore() { UnMount(); }

	/** simply write file. */
	void   RecordData(const string& key, const string& data) override;
	/** simply read file.  */
	string ReadData(const string& key) override;
	/** simply remove file or dir */
	void   RemoveData(const string& key) override;
	/** simply copy   file or dir */
	void   CopyData(const string& keyFrom, const string& keyTo) override;
};