// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 wandererfan <wandererfan@gmail.com>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_

#endif
#include <Mod/Measure/MeasureGlobal.h>

#include <algorithm>  // clears "include what you use" lint message, but creates "included header not used"
#include <string>

#include <Python.h>

#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include <Mod/Part/App/TopoShapePy.h>
#include "Mod/Part/App/OCCError.h"

#include "ShapeFinder.h"


namespace Measure
{
// module level static C++ functions go here
}

namespace Measure
{
/** Copies a Python dictionary of Python strings to a C++ container.
 *
 * After the function call, the key-value pairs of the Python
 * dictionary are copied into the target buffer as C++ pairs
 * (pair<string, string>).
 *
 * @param sourceRange is a Python dictionary (Py::Dict). Both, the
 * keys and the values must be Python strings.
 *
 * @param targetIt refers to where the data should be inserted. Must
 * be of concept output iterator.
 */
template<typename OutputIt>
void copy(Py::Dict sourceRange, OutputIt targetIt)
{
    std::string key;
    std::string value;

    for (const auto& keyPy : sourceRange.keys()) {
        key = Py::String(keyPy);
        value = Py::String(sourceRange[keyPy]);
        *targetIt = {key, value};
        ++targetIt;
    }
}


class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Measure")
    {
        add_varargs_method(
            "getLocatedTopoShape",
            &Module::getLocatedTopoShape,
            "Part.TopoShape = Measure.getLocatedTopoShape(DocumentObject, longSubElement) Resolves "
            "the net placement of DocumentObject and returns the object's shape/subshape with the "
            "net placement applied.  Link scaling operations along the path are also applied.");
        initialize("This is a module for measuring");  // register with Python
    }
    ~Module() override
    {}

private:
    Py::Object invoke_method_varargs(void* method_def, const Py::Tuple& args) override
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_varargs(method_def, args);
        }
        catch (const Standard_Failure& e) {
            std::string str;
            Standard_CString msg = e.GetMessageString();
            str += typeid(e).name();
            str += " ";
            if (msg) {
                str += msg;
            }
            else {
                str += "No OCCT Exception Message";
            }
            Base::Console().error("%s\n", str.c_str());
            throw Py::Exception(Part::PartExceptionOCCError, str);
        }
        catch (const Base::Exception& e) {
            std::string str;
            str += "FreeCAD exception thrown (";
            str += e.what();
            str += ")";
            e.reportException();
            throw Py::RuntimeError(str);
        }
        catch (const std::exception& e) {
            std::string str;
            str += "C++ exception thrown (";
            str += e.what();
            str += ")";
            Base::Console().error("%s\n", str.c_str());
            throw Py::RuntimeError(str);
        }
    }

    Py::Object getLocatedTopoShape(const Py::Tuple& args)
    {
        PyObject* pyRootObject {nullptr};
        PyObject* pyLeafSubName {nullptr};
        App::DocumentObject* rootObject {nullptr};
        std::string leafSub;
        if (!PyArg_ParseTuple(args.ptr(), "OO", &pyRootObject, &pyLeafSubName)) {
            throw Py::TypeError("expected (rootObject, subname");
        }

        if (PyObject_TypeCheck(pyRootObject, &(App::DocumentObjectPy::Type))) {
            rootObject = static_cast<App::DocumentObjectPy*>(pyRootObject)->getDocumentObjectPtr();
        }

        if (PyUnicode_Check(pyLeafSubName)) {
            leafSub = PyUnicode_AsUTF8(pyLeafSubName);
        }

        if (!rootObject) {
            return Py::None();
        }

        // this is on the stack
        auto temp = ShapeFinder::getLocatedShape(*rootObject, leafSub);
        // need new in here to make the twin object on the heap
        auto topoShapePy = new Part::TopoShapePy(new Part::TopoShape(temp));
        return Py::asObject(topoShapePy);
    }
};

}  // namespace Measure
