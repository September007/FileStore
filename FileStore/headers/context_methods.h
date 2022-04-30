#pragma once
#include <assistant_utility.h>
//#include<object.h>
class ReferedBlock;
using std::string;
/**
 * io methods
 */
string stdio_ReadFile(const string& path);
bool   stdio_WriteFile(
	  const string& path, const string& content, const bool create_parent_dir_if_missing = true);

string keep_ReadFile(const string& path, int sz);
bool   keep_WriteFile(
	  const string& path, const string& content, const bool create_parent_dir_if_missing = true);

/**
 * referdblock placement methods
 */

string GetReferedBlockStoragePath_deep4(const ReferedBlock& rb, string root_path);
string GetReferedBlockStoragePath_deep2(const ReferedBlock& rb, string root_path);
