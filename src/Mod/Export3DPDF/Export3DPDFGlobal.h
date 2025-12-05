#ifndef EXPORT3DPDF_GLOBAL_H
#define EXPORT3DPDF_GLOBAL_H

#include <Base/BaseClass.h>

#ifndef Export3DPDFExport
#ifdef Export3DPDF_EXPORTS
#  define Export3DPDFExport    FREECAD_DECL_EXPORT
#else
#  define Export3DPDFExport    FREECAD_DECL_IMPORT
#endif
#endif

#ifndef Export3DPDFGuiExport
#ifdef Export3DPDFGui_EXPORTS
#  define Export3DPDFGuiExport    FREECAD_DECL_EXPORT
#else
#  define Export3DPDFGuiExport    FREECAD_DECL_IMPORT
#endif
#endif

#endif // EXPORT3DPDF_GLOBAL_H 