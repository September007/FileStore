#pragma once
#if defined(_WIN32) || defined(_WIN64)
#ifdef FileStore_EXPORTS   //�����˸ú�Ĳ�Ϊ�����������Ϊ����
#define DLL_INTERFACE_API _declspec(dllexport)
#else
#define DLL_INTERFACE_API _declspec(dllimport)
#endif
#else
#define DLL_INTERFACE_API
#endif
