/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Base/VectorPy.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/Application.h>

#include "CommandPy.h"
#include "PathPy.h"
#include "Path.h"
#include "FeaturePath.h"
#include "FeaturePathCompound.h"

using namespace Path;


static PyObject * write (PyObject *self, PyObject *args)
{
    char* Name;
    PyObject* pObj;
    if (!PyArg_ParseTuple(args, "Oet",&pObj,"utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    Base::FileInfo file(EncodedName.c_str());
    
    if (PyObject_TypeCheck(pObj, &(App::DocumentObjectPy::Type))) {
        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(pObj)->getDocumentObjectPtr();
        if (obj->getTypeId().isDerivedFrom(Base::Type::fromName("Path::Feature"))) {
            const Toolpath& path = static_cast<Path::Feature*>(obj)->Path.getValue();
            std::string gcode = path.toGCode();    
            std::ofstream ofile(EncodedName.c_str());
            ofile << gcode;
            ofile.close();
        } else
            Py_Error(Base::BaseExceptionFreeCADError, "The given file is not a path");
    }
    Py_Return;
}


static PyObject * read (PyObject *self, PyObject *args)
{
    char* Name;
    const char* DocName=0;
    if (!PyArg_ParseTuple(args, "et|s","utf-8",&Name,&DocName))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    Base::FileInfo file(EncodedName.c_str());
    if (!file.exists())
        Py_Error(Base::BaseExceptionFreeCADError, "File doesn't exist");
    App::Document *pcDoc;
    if (DocName)
        pcDoc = App::GetApplication().getDocument(DocName);
    else
        pcDoc = App::GetApplication().getActiveDocument();
    if (!pcDoc) 
        pcDoc = App::GetApplication().newDocument(DocName);

    PY_TRY {
        // read the gcode file
        std::ifstream filestr(file.filePath().c_str());
        std::stringstream buffer;
        buffer << filestr.rdbuf();
        std::string gcode = buffer.str();
        Toolpath path;
        path.setFromGCode(gcode);
        Path::Feature *object = static_cast<Path::Feature *>(pcDoc->addObject("Path::Feature",file.fileNamePure().c_str()));
        object->Path.setValue(path);
        pcDoc->recompute();
    } PY_CATCH;
    Py_Return;
}


static PyObject * show (PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(PathPy::Type), &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        App::Document *pcDoc = App::GetApplication().getActiveDocument(); 	 
        if (!pcDoc)
            pcDoc = App::GetApplication().newDocument();
        PathPy* pPath = static_cast<PathPy*>(pcObj);
        Path::Feature *pcFeature = (Path::Feature *)pcDoc->addObject("Path::Feature", "Path");
        Path::Toolpath* pa = pPath->getToolpathPtr();
        if (!pa) {
            PyErr_SetString(PyExc_ReferenceError,
                "object doesn't reference a valid path");
            return 0;
        }
        // copy the data
        pcFeature->Path.setValue(*pa);
    } PY_CATCH;

    Py_Return;
}


/* registration table  */
struct PyMethodDef Path_methods[] = {
    {"write"      ,write     ,METH_VARARGS,
     "write(object,filename): Exports a given path object to a GCode file"},
    {"read"       ,read      ,METH_VARARGS,
     "read(filename,[document]): Imports a GCode file into the given document"},
    {"show"       ,show      ,METH_VARARGS,
     "show(path): Add the path to the active document or create one if no document exists."},
    {NULL, NULL}        /* end of table marker */
};
