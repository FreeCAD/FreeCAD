/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include "ImagePlane.h"


/* registration table  */
static struct PyMethodDef Image_methods[] = {
    {NULL, NULL}                   /* end of table marker */
};

/* Python entry */
extern "C" {
void ImageExport initImage() {
    (void) Py_InitModule("Image", Image_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading Image module... done\n");

    Image::ImagePlane::init();

    return;
}

} // extern "C"
