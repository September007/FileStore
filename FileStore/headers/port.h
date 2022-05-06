#pragma once
/*********************************************************************************/
/**
 * \file   port.h
 * \brief  define import and export macro for api on windows
 * \note   non-staitc func and class is automatically export and import on linux
 *
 * \author 28688
 * \date   2022-5-3
 *//*******************************************************************************/

// windows will define a precessor macro 'FileStore_EXPORTS' for target FileStore
#if defined(_WIN32) || defined(_WIN64)
#ifdef FileStore_EXPORTS
#define DLL_INTERFACE_API _declspec(dllexport)
#else
#define DLL_INTERFACE_API _declspec(dllimport)
#endif
#else
#define DLL_INTERFACE_API
#endif
/** shut down some *port warning */
#pragma warning(disable : 4251)

#define OMAP_WRITE_CHECKArea(l, ...) l, ##__VA_ARGS__

#define MAC_Area(mac, l, ...) mac##Area(l, ##__VA_ARGS__)
