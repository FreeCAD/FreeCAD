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
#include "FoldSolver.h"


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
            "foldPath",
            &Module::foldPath,
            "foldPath(feature, scores, order, steps=12) -> dict\n"
            "The motion of a part from flat to folded, score by score. A part with coupled folds\n"
            "has loops of panels: the solver moves one score and finds the angles of the other\n"
            "bends that keep each loop closed.\n"
            "scores: for each score, the indexes of its bends. order: the number of each score\n"
            "in the fold sequence. The result has States (for each step, the angle of each bend\n"
            "in radians), Drivers, Mismatch (mm), EndError (radians), Complete, Notes, Loops."
        );
        add_varargs_method(
            "modelInfo",
            &Module::modelInfo,
            "modelInfo(feature) -> dict\n"
            "The structure of the sheet model of a feature: Thickness, Panels (the count), Root\n"
            "(the index of the root panel), and Bends. Each bend is a dict with Parent, Child,\n"
            "Start, End, Normal (in flat space), Angle (radians), Up, Radius, Allowance, Closing."
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
    Py::Object foldPath(const Py::Tuple& args)
    {
        PyObject* object = nullptr;
        PyObject* scoreList = nullptr;
        PyObject* orderList = nullptr;
        int steps = 12;
        if (!PyArg_ParseTuple(
                args.ptr(),
                "O!OO|i",
                &(App::DocumentObjectPy::Type),
                &object,
                &scoreList,
                &orderList,
                &steps
            )) {
            throw Py::Exception();
        }
        auto* feature = freecad_cast<SheetDesign::Feature*>(
            static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr()
        );
        if (!feature) {
            throw Py::TypeError("The object is not a sheet design feature");
        }
        std::vector<std::vector<int>> scores;
        for (const Py::Object& group : Py::Sequence(scoreList)) {
            std::vector<int> bends;
            for (const Py::Object& item : Py::Sequence(group)) {
                bends.push_back(static_cast<int>(static_cast<long>(Py::Long(item))));
            }
            scores.push_back(bends);
        }
        std::vector<int> order;
        for (const Py::Object& item : Py::Sequence(orderList)) {
            order.push_back(static_cast<int>(static_cast<long>(Py::Long(item))));
        }
        try {
            SheetDesign::FoldSolver solver(*feature->getModel(), scores, order);
            const SheetDesign::FoldPath path = solver.solve(steps);
            Py::Dict result;
            Py::List states;
            for (const std::vector<double>& state : path.states) {
                Py::List angles;
                for (double angle : state) {
                    angles.append(Py::Float(angle));
                }
                states.append(angles);
            }
            Py::List drivers;
            for (int driver : path.drivers) {
                drivers.append(Py::Long(static_cast<long>(driver)));
            }
            Py::List notes;
            for (const std::string& note : path.notes) {
                notes.append(Py::String(note));
            }
            result.setItem("States", states);
            result.setItem("Drivers", drivers);
            result.setItem("Notes", notes);
            result.setItem("Mismatch", Py::Float(path.mismatch));
            result.setItem("EndError", Py::Float(path.endError));
            result.setItem("Complete", Py::Boolean(path.complete));
            result.setItem("Loops", Py::Boolean(solver.hasLoops()));
            return result;
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
        catch (const Standard_Failure& e) {
            throw Py::RuntimeError(e.GetMessageString());
        }
    }

    Py::Object modelInfo(const Py::Tuple& args)
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
            std::shared_ptr<const SheetDesign::SheetModel> sheet = feature->getModel();
            auto point = [](double x, double y) {
                Py::Tuple value(2);
                value.setItem(0, Py::Float(x));
                value.setItem(1, Py::Float(y));
                return value;
            };
            Py::Dict info;
            info.setItem("Thickness", Py::Float(sheet->params.thickness));
            info.setItem("Panels", Py::Long(static_cast<long>(sheet->panels.size())));
            long root = 0;
            for (std::size_t i = 0; i < sheet->panels.size(); ++i) {
                if (sheet->panels[i].parentBend < 0) {
                    root = static_cast<long>(i);
                    break;
                }
            }
            info.setItem("Root", Py::Long(root));
            Py::List bends;
            for (const SheetDesign::Bend& bend : sheet->bends) {
                Py::Dict item;
                item.setItem("Parent", Py::Long(static_cast<long>(bend.parentPanel)));
                item.setItem("Child", Py::Long(static_cast<long>(bend.childPanel)));
                item.setItem("Start", point(bend.start.X(), bend.start.Y()));
                item.setItem("End", point(bend.end.X(), bend.end.Y()));
                item.setItem("Normal", point(bend.normal.X(), bend.normal.Y()));
                item.setItem("Angle", Py::Float(bend.angle));
                item.setItem("Up", Py::Boolean(bend.up));
                item.setItem("Radius", Py::Float(bend.radius));
                item.setItem("Allowance", Py::Float(bend.allowance));
                item.setItem("Closing", Py::Boolean(bend.closing));
                bends.append(item);
            }
            info.setItem("Bends", bends);
            return info;
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
        catch (const Standard_Failure& e) {
            throw Py::RuntimeError(e.GetMessageString());
        }
    }

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
