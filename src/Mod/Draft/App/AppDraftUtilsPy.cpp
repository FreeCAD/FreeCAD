/***************************************************************************
 *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/VectorPy.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/Application.h>

#include "DraftDxf.h"

namespace DraftUtils {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("DraftUtils")
    {
        add_varargs_method("readDXF",&Module::readDXF,
            "readDXF(filename,[document,ignore_errors]): Imports a DXF file into the given document. ignore_errors is True by default."
        );
        initialize("The DraftUtils module contains utility functions for the Draft module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object readDXF(const Py::Tuple& args)
    {
        Base::Console().Warning("DraftUtils.readDXF is deprecated. Use Import.readDxf instead.\n");
        char* Name;
        const char* DocName=nullptr;
        bool IgnoreErrors=true;
        if (!PyArg_ParseTuple(args.ptr(), "et|sb","utf-8",&Name,&DocName,&IgnoreErrors))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        Base::FileInfo file(EncodedName.c_str());
        if (!file.exists())
            throw Py::RuntimeError("File doesn't exist");

        App::Document *pcDoc;
        if (DocName)
            pcDoc = App::GetApplication().getDocument(DocName);
        else
            pcDoc = App::GetApplication().getActiveDocument();
        if (!pcDoc) 
            pcDoc = App::GetApplication().newDocument(DocName);

        try {
            // read the DXF file
            DraftDxfRead dxf_file(EncodedName,pcDoc);
            dxf_file.DoRead(IgnoreErrors);
            pcDoc->recompute();
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace DraftUtils
