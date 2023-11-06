/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/MeshPy.h>

#include <CXX/Extensions.hxx>

#include "TestJtReader.h"


using std::vector;
using namespace MeshCore;


namespace JtReaderNS
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("JtReader")
    {
        add_varargs_method("read",
                           &Module::read,
                           "Read the mesh from a JT file and return a mesh object.");
        add_varargs_method("open",
                           &Module::open,
                           "open(string)\n"
                           "Create a new document and load the JT file into\n"
                           "the document.");
        add_varargs_method("insert",
                           &Module::importer,
                           "insert(string|mesh,[string])\n"
                           "Load or insert a JT file into the given or active document.");
        initialize("This module is the JtReader module.");
    }

private:
    Py::Object read(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);
        return Py::None();
    }
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);


        // Base::Console().Log("Open in Mesh with %s",Name);
        Base::FileInfo file(EncodedName.c_str());
        if (file.hasExtension("jt")) {
            TestJtReader reader;
            reader.setFile(EncodedName.c_str());
            reader.read();
            return Py::None();
        }

        throw Py::RuntimeError("unknown file ending");
    }
    Py::Object importer(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName;
        if (!PyArg_ParseTuple(args.ptr(), "ets", "utf-8", &Name, &DocName)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        Base::FileInfo file(EncodedName.c_str());

        if (file.hasExtension("jt")) {
            // add Import feature
            App::Document* pcDoc = App::GetApplication().getDocument(DocName);
            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument(DocName);
            }

            return Py::None();
        }

        throw Py::RuntimeError("unknown file ending");
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace JtReaderNS
