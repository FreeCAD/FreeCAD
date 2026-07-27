// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <FCGlobal.h>

#ifndef MbDFEMExport
# ifdef MbDFEM_EXPORTS
#  define MbDFEMExport FREECAD_DECL_EXPORT
# else
#  define MbDFEMExport FREECAD_DECL_IMPORT
# endif
#endif

#ifndef MbDFEMGuiExport
# ifdef MbDFEMGui_EXPORTS
#  define MbDFEMGuiExport FREECAD_DECL_EXPORT
# else
#  define MbDFEMGuiExport FREECAD_DECL_IMPORT
# endif
#endif
