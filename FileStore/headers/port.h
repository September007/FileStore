#pragma once
#if defined(_WIN32) || defined(_WIN64)
#ifdef FileStore_EXPORTS   //定义了该宏的才为导出，否则均为导入
#define DLL_INTERFACE_API _declspec(dllexport)
#else
#define DLL_INTERFACE_API _declspec(dllimport)
#endif
#else
#define DLL_INTERFACE_API
#endif
