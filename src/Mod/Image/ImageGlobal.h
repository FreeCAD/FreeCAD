/***************************************************************************
 *   Copyright (c) Imetric 4D Imaging Sarl                                 *
 *                                                                         *
 *   Author: Werner Mayer                                                  *
 *                                                                         *
 ***************************************************************************/

#include <FCGlobal.h>

#ifndef IMAGE_GLOBAL_H
#define IMAGE_GLOBAL_H


// Image
#ifndef ImageExport
#ifdef Image_EXPORTS
#  define ImageExport   FREECAD_DECL_EXPORT
#else
#  define ImageExport   FREECAD_DECL_IMPORT
#endif
#endif

// ImageGui
#ifndef ImageGuiExport
#ifdef ImageGui_EXPORTS
#  define ImageGuiExport   FREECAD_DECL_EXPORT
#else
#  define ImageGuiExport   FREECAD_DECL_IMPORT
#endif
#endif

#endif //IMAGE_GLOBAL_H
