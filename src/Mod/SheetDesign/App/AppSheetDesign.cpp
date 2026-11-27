// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <Base/Console.h>
#include <Standard_Failure.hxx>

#include <App/DocumentObjectPy.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>

#include "Features.h"


namespace SheetDesign
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("SheetDesign")
    {
        add_varargs_method(
            "edgeInfo",
            &Module::edgeInfo,
            "edgeInfo(feature, element) -> dict or None\n"
            "Test an edge or a side face of a sheet design feature. If a flange is possible on\n"
            "it, the result has the keys Panel (the name of the panel), Top (the edge is on the\n"
            "top side), and Length. If not, the result is None."
        );
        add_varargs_method(
            "bendCount",
            &Module::bendCount,
            "bendCount(feature) -> int\n"
            "The number of bends of a sheet design feature."
        );
        add_varargs_method(
            "foldedShape",
            &Module::foldedShape,
            "foldedShape(feature, fractions, fuse=False) -> (shape, overlap)\n"
            "The solid of a sheet design feature in a state between flat and folded. The list\n"
            "has one value for each bend, in the sequence of the bend lines of the flat pattern:\n"
            "0 = flat, 1 = folded. With fuse, the result is one solid and overlap is the volume\n"
            "of material in the same space. The document does not change."
        );
        initialize("This module is the SheetDesign module.");  // register with Python
    }

private:
    Py::Object bendCount(const Py::Tuple& args)
    {
        PyObject* object = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O!", &(App::DocumentObjectPy::Type), &object)) {
            throw Py::Exception();
        }
        auto* feature = freecad_cast<SheetDesign::Feature*>(
            static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr()
        );
        if (!feature) {
            throw Py::TypeError("The object is not a sheet design feature");
        }
        try {
            return Py::Long(static_cast<long>(feature->getModel()->bends.size()));
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
        catch (const Standard_Failure& e) {
            throw Py::RuntimeError(e.GetMessageString());
        }
    }

    Py::Object foldedShape(const Py::Tuple& args)
    {
        PyObject* object = nullptr;
        PyObject* list = nullptr;
        PyObject* fuse = Py_False;
        if (!PyArg_ParseTuple(
                args.ptr(),
                "O!O|O!",
                &(App::DocumentObjectPy::Type),
                &object,
                &list,
                &PyBool_Type,
                &fuse
            )) {
            throw Py::Exception();
        }
        auto* feature = freecad_cast<SheetDesign::Feature*>(
            static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr()
        );
        if (!feature) {
            throw Py::TypeError("The object is not a sheet design feature");
        }
        std::vector<double> fractions;
        for (const Py::Object& item : Py::Sequence(list)) {
            fractions.push_back(static_cast<double>(Py::Float(item)));
        }
        try {
            double overlap = 0.0;
            const bool one = PyObject_IsTrue(fuse) == 1;
            const TopoDS_Shape shape
                = feature->getModel()->makeFoldedAt(fractions, one, one ? &overlap : nullptr);
            Py::Tuple result(2);
            result.setItem(0, Py::asObject(new Part::TopoShapePy(new Part::TopoShape(shape))));
            result.setItem(1, Py::Float(overlap));
            return result;
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
        catch (const Standard_Failure& e) {
            throw Py::RuntimeError(e.GetMessageString());
        }
    }

    Py::Object edgeInfo(const Py::Tuple& args)
    {
        PyObject* object = nullptr;
        const char* element = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O!s", &(App::DocumentObjectPy::Type), &object, &element)) {
            throw Py::Exception();
        }
        auto* feature = freecad_cast<SheetDesign::Feature*>(
            static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr()
        );
        if (!feature) {
            return Py::None();
        }
        try {
            SheetDesign::EdgeRef ref;
            if (!feature->findEdge(element, ref)) {
                return Py::None();
            }
            Py::Dict info;
            info.setItem("Panel", Py::String(feature->getModel()->panels[ref.panel].tag));
            info.setItem("Top", Py::Boolean(ref.top));
            info.setItem("Length", Py::Float(ref.a.Distance(ref.b)));
            return info;
        }
        catch (const Base::Exception&) {
            return Py::None();
        }
        catch (const Standard_Failure&) {
            return Py::None();
        }
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace SheetDesign

/* Python entry */
PyMOD_INIT_FUNC(SheetDesign)
{
    try {
        Base::Interpreter().runString("import Part");
        // The features are PartDesign features, thus a PartDesign Body accepts them.
        Base::Interpreter().runString("import _PartDesign");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = SheetDesign::initModule();
    Base::Console().log("Loading SheetDesign module… done\n");

    // clang-format off
    SheetDesign::Feature     ::init();
    SheetDesign::BasePanel   ::init();
    SheetDesign::Refold    ::init();
    SheetDesign::Unfold   ::init();
    SheetDesign::Flange      ::init();
    SheetDesign::Cut         ::init();
    SheetDesign::Extend      ::init();
    SheetDesign::Fold        ::init();
    SheetDesign::Hem         ::init();
    SheetDesign::CornerRelief::init();
    SheetDesign::BaseShape   ::init();
    SheetDesign::Convert     ::init();
    SheetDesign::Form        ::init();
    SheetDesign::Perforation ::init();
    SheetDesign::FlatPattern ::init();
    // clang-format on

    PyMOD_Return(mod);
}
