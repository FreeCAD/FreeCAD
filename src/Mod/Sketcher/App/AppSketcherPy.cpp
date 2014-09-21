/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRepPrimAPI_MakeBox.hxx>
# include <TopoDS_Face.hxx>
# include <Geom_Plane.hxx>
# include <Handle_Geom_Plane.hxx>
#endif

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <App/Application.h>
#include <App/Document.h>

// Things from the part module
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>

#include "SketchObjectSF.h"

using Base::Console;
using namespace Part;
using namespace std;


/* module functions */
static PyObject * open(PyObject *self, PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    PY_TRY {
    } PY_CATCH;
        //Base::Console().Log("Open in Part with %s",Name);
        Base::FileInfo file(EncodedName.c_str());

        // extract ending
        if (file.extension() == "")
            Py_Error(Base::BaseExceptionFreeCADError,"no file ending");

        //if (file.hasExtension("igs") || file.hasExtension("iges")) {
        //    // create new document and add Import feature
        //    App::Document *pcDoc = App::GetApplication().newDocument(file.fileNamePure().c_str());
        //    Part::ImportIges *pcFeature = (Part::ImportIges*) pcDoc->addObject("Part::ImportIges",file.fileNamePure().c_str());
        //    pcFeature->FileName.setValue(Name);
        //    pcDoc->recompute();
        //}
        // else {
            Py_Error(Base::BaseExceptionFreeCADError,"unknown file ending");
        //}

    Py_Return;
}

/* module functions */
static PyObject * insert(PyObject *self, PyObject *args)
{
    char* Name;
    const char* DocName;
    if (!PyArg_ParseTuple(args, "ets","utf-8",&Name,&DocName))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    PY_TRY {
        //Base::Console().Log("Insert in Part with %s",Name);
        Base::FileInfo file(EncodedName.c_str());

        // extract ending
        if (file.extension() == "")
            Py_Error(Base::BaseExceptionFreeCADError,"no file ending");
        App::Document *pcDoc = App::GetApplication().getDocument(DocName);
        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName);
        }

        if (file.hasExtension("skf")) {

            Sketcher::SketchObjectSF *pcFeature = (Sketcher::SketchObjectSF *)pcDoc->addObject("Sketcher::SketchObjectSF",file.fileNamePure().c_str());
            pcFeature->SketchFlatFile.setValue(EncodedName.c_str());

            pcDoc->recompute();
        }
        else {
            Py_Error(Base::BaseExceptionFreeCADError,"unknown file ending");
        }

    } PY_CATCH;

    Py_Return;
}

/* module functions */
//static PyObject * read(PyObject *self, PyObject *args)
//{
//    const char* Name;
//    if (!PyArg_ParseTuple(args, "s",&Name))
//        return NULL;
//    PY_TRY {
//    } PY_CATCH;
//  
//    Py_Return;
//}

/* registration table  */
struct PyMethodDef Sketcher_methods[] = {
    {"open"   , open,    1},
    {"insert" , insert,  1},
//    {"read"   , read,  1},
    {NULL, NULL}        /* end of table marker */
};
