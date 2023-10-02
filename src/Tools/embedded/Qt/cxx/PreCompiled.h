#ifndef __PRECOMPILED__
#define __PRECOMPILED__

#ifdef _MSC_VER
#pragma warning(disable : 4251)
#endif

#ifdef FC_OS_WIN32
// cmake generates this define
#if defined(FreeCADPlugin_EXPORTS)
#define FC_PLUGIN_EXPORT __declspec(dllexport)
#else
#define FC_PLUGIN_EXPORT __declspec(dllimport)
#endif
#define MeshExport __declspec(dllimport)
#else  // for Linux
#define FC_PLUGIN_EXPORT
#define MeshExport
#endif


#endif
