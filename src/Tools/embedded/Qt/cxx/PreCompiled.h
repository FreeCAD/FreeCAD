// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#ifdef FC_OS_WIN32
// cmake generates this define
# if defined(FreeCADPlugin_EXPORTS)
#  define FC_PLUGIN_EXPORT __declspec(dllexport)
# else
#  define FC_PLUGIN_EXPORT __declspec(dllimport)
# endif
# define MeshExport __declspec(dllimport)
#else  // for Linux
# define FC_PLUGIN_EXPORT
# define MeshExport
#endif
