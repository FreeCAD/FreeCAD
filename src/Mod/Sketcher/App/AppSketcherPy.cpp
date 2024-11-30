/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/PyObjectBase.h>

#include "SketchObjectSF.h"


namespace Sketcher
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Sketcher")
    {
        add_varargs_method("open", &Module::open);
        add_varargs_method("insert", &Module::insert);
        initialize("This module is the Sketcher module.");  // register with Python
    }

    ~Module() override
    {}

private:
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
            throw Py::Exception();
        }
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        // Base::Console().Log("Open in Part with %s",Name);
        Base::FileInfo file(EncodedName.c_str());

        // extract extension
        if (file.extension().empty()) {
            throw Py::RuntimeError("No file extension");
        }

        throw Py::RuntimeError("Unknown file extension");
        // return Py::None();
    }

    Py::Object insert(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName;
        if (!PyArg_ParseTuple(args.ptr(), "ets", "utf-8", &Name, &DocName)) {
            throw Py::Exception();
        }
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        try {
            // Base::Console().Log("Insert in Part with %s",Name);
            Base::FileInfo file(EncodedName.c_str());

            // extract extension
            if (file.extension().empty()) {
                throw Py::RuntimeError("No file extension");
            }

            App::Document* pcDoc = App::GetApplication().getDocument(DocName);
            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument(DocName);
            }

            if (file.hasExtension("skf")) {
                Sketcher::SketchObjectSF* pcFeature = static_cast<Sketcher::SketchObjectSF*>(
                    pcDoc->addObject("Sketcher::SketchObjectSF", file.fileNamePure().c_str()));
                pcFeature->SketchFlatFile.setValue(EncodedName.c_str());

                pcDoc->recompute();
            }
            else {
                throw Py::RuntimeError("Unknown file extension");
            }
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
        return Py::None();
    }
};

/// @cond DOXERR
PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}
/// @endcond

}  // namespace Sketcher
