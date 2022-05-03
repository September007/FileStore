/*****************************************************************/
/**
 * \file   context_methods.h
 * \brief  define some methods for context, these methods will be use in benchmark for efficiency
 * comparison
 */
/*********************************************************************/
#pragma once
#include <assistant_utility.h>
#include <port.h>
//#include<object.h>
class ReferedBlock;
using std::string;
/** io read using std::ifstream */
DLL_INTERFACE_API string stdio_ReadFile(const string& path);
/** io read using std::ofstream */
DLL_INTERFACE_API bool	 stdio_WriteFile(
	  const string& path, const string& content, const bool create_parent_dir_if_missing = true);
/** io read using fread */
DLL_INTERFACE_API string keep_ReadFile(const string& path, int sz);
/** io read using fwrite */
DLL_INTERFACE_API bool	 keep_WriteFile(
	  const string& path, const string& content, const bool create_parent_dir_if_missing = true);

/**  referdblock placement methods in deep 4  */
DLL_INTERFACE_API string GetReferedBlockStoragePath_deep4(const ReferedBlock& rb, string root_path);
/**  referdblock placement methods in deep 2  */
DLL_INTERFACE_API string GetReferedBlockStoragePath_deep2(const ReferedBlock& rb, string root_path);
