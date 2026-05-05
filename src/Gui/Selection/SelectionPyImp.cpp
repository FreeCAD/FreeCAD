// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Juergen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
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


#include <array>
#include <set>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Base/Exception.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include "Selection.h"
#include "SelectionPy.h"
#include "SelectionObject.h"
#include "SelectionFilter.h"
#include "SelectionFilterPy.h"
#include "SelectionObserverPython.h"


using namespace Gui;

//**************************************************************************
// Python stuff

namespace
{
using SelectionStyle = SelectionSingleton::SelectionStyle;

class PythonSelectionObject: public SelectionObject
{
public:
    PythonSelectionObject(
        const char* docName,
        const char* objName,
        const char* subName,
        const Base::Vector3d* pickedPoint
    )
    {
        DocName = docName ? docName : "";
        FeatName = objName ? objName : "";
        if (subName && subName[0] != '\0') {
            SubNames.emplace_back(subName);
            if (pickedPoint) {
                SelPoses.emplace_back(*pickedPoint);
            }
        }
    }
};

PyObject* sAddSelection(PyObject* self, PyObject* args);
PyObject* sUpdateSelection(PyObject* self, PyObject* args);
PyObject* sRemoveSelection(PyObject* self, PyObject* args);
PyObject* sClearSelection(PyObject* self, PyObject* args);
PyObject* sIsSelected(PyObject* self, PyObject* args);
PyObject* sCountObjectsOfType(PyObject* self, PyObject* args);
PyObject* sGetSelection(PyObject* self, PyObject* args);
PyObject* sSetPreselection(PyObject* self, PyObject* args, PyObject* kwd);
PyObject* sGetPreselection(PyObject* self, PyObject* args);
PyObject* sRemPreselection(PyObject* self, PyObject* args);
PyObject* sGetCompleteSelection(PyObject* self, PyObject* args);
PyObject* sGetSelectionEx(PyObject* self, PyObject* args);
PyObject* sGetSelectionObject(PyObject* self, PyObject* args);
PyObject* sSetSelectionStyle(PyObject* self, PyObject* args);
PyObject* sAddSelObserver(PyObject* self, PyObject* args);
PyObject* sRemSelObserver(PyObject* self, PyObject* args);
PyObject* sAddSelectionGate(PyObject* self, PyObject* args);
PyObject* sRemoveSelectionGate(PyObject* self, PyObject* args);
PyObject* sGetPickedList(PyObject* self, PyObject* args);
PyObject* sEnablePickedList(PyObject* self, PyObject* args);
PyObject* sSetVisible(PyObject* self, PyObject* args);
PyObject* sPushSelStack(PyObject* self, PyObject* args);
PyObject* sHasSelection(PyObject* self, PyObject* args);
PyObject* sHasSubSelection(PyObject* self, PyObject* args);
PyObject* sGetSelectionFromStack(PyObject* self, PyObject* args);
ResolveMode toEnum(int value);

// Selection module methods
PyMethodDef selectionMethods[] = {
    {"addSelection",
     (PyCFunction)sAddSelection,
     METH_VARARGS,
     "addSelection(docName, objName, subName, x=0, y=0, z=0, clear=True) -> None\n"
     "addSelection(obj, subName, x=0, y=0, z=0, clear=True) -> None\n"
     "addSelection(obj, subNames, clear=True) -> None\n"
     "\n"
     "Add an object to the selection.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "objName : str\n    Name of the `App.DocumentObject` to add.\n"
     "obj : App.DocumentObject\n    Object to add.\n"
     "subName : str\n    Subelement name.\n"
     "x : float\n    Coordinate `x` of the point to pick.\n"
     "y : float\n    Coordinate `y` of the point to pick.\n"
     "z : float\n    Coordinate `z` of the point to pick.\n"
     "subNames : list of str\n    List of subelement names.\n"
     "clear : bool\n    Clear preselection."},
    {"updateSelection",
     (PyCFunction)sUpdateSelection,
     METH_VARARGS,
     "updateSelection(show, obj, subName) -> None\n"
     "\n"
     "Update an object in the selection.\n"
     "\n"
     "show : bool\n    Show or hide the selection.\n"
     "obj : App.DocumentObject\n    Object to update.\n"
     "subName : str\n    Name of the subelement to update."},
    {"removeSelection",
     (PyCFunction)sRemoveSelection,
     METH_VARARGS,
     "removeSelection(obj, subName) -> None\n"
     "removeSelection(docName, objName, subName) -> None\n"
     "\n"
     "Remove an object from the selection.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "objName : str\n    Name of the `App.DocumentObject` to remove.\n"
     "obj : App.DocumentObject\n    Object to remove.\n"
     "subName : str\n    Name of the subelement to remove."},
    {"clearSelection",
     (PyCFunction)sClearSelection,
     METH_VARARGS,
     "clearSelection(docName, clearPreSelect=True) -> None\n"
     "clearSelection(clearPreSelect=True) -> None\n"
     "\n"
     "Clear the selection in the given document. If no document is\n"
     "given the complete selection is cleared.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "clearPreSelect : bool\n    Clear preselection."},
    {"isSelected",
     (PyCFunction)sIsSelected,
     METH_VARARGS,
     "isSelected(obj, subName, resolve=ResolveMode.OldStyleElement) -> bool\n"
     "\n"
     "Check if a given object is selected.\n"
     "\n"
     "obj : App.DocumentObject\n    Object to check.\n"
     "subName : str\n    Name of the subelement.\n"
     "resolve : int\n    Resolve subelement reference."},
    {"setPreselection",
     reinterpret_cast<PyCFunction>(reinterpret_cast<void (*)()>(sSetPreselection)),
     METH_VARARGS | METH_KEYWORDS,
     "setPreselection(obj, subName, x=0, y=0, z=0, type=1) -> None\n"
     "\n"
     "Set preselected object.\n"
     "\n"
     "obj : App.DocumentObject\n    Object to preselect.\n"
     "subName : str\n    Subelement name.\n"
     "x : float\n    Coordinate `x` of the point to preselect.\n"
     "y : float\n    Coordinate `y` of the point to preselect.\n"
     "z : float\n    Coordinate `z` of the point to preselect.\n"
     "type : int"},
    {"getPreselection",
     (PyCFunction)sGetPreselection,
     METH_VARARGS,
     "getPreselection() -> Gui.SelectionObject\n"
     "\n"
     "Get preselected object."},
    {"clearPreselection",
     (PyCFunction)sRemPreselection,
     METH_VARARGS,
     "clearPreselection() -> None\n"
     "\n"
     "Clear the preselection."},
    {"countObjectsOfType",
     (PyCFunction)sCountObjectsOfType,
     METH_VARARGS,
     "countObjectsOfType(type, docName, resolve=ResolveMode.OldStyleElement) -> int\n"
     "\n"
     "Get the number of selected objects. If no document name is given the\n"
     "active document is used and '*' means all documents.\n"
     "\n"
     "type : str\n    Object type id name.\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int"},
    {"getSelection",
     (PyCFunction)sGetSelection,
     METH_VARARGS,
     "getSelection(docName, resolve=ResolveMode.OldStyleElement, single=False) -> list\n"
     "\n"
     "Return a list of selected objects. If no document name is given\n"
     "the active document is used and '*' means all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int\n    Resolve the subname references.\n"
     "    0: do not resolve, 1: resolve, 2: resolve with element map.\n"
     "single : bool\n    Only return if there is only one selection."},
    {"getPickedList",
     (PyCFunction)sGetPickedList,
     1,
     "getPickedList(docName) -> list of Gui.SelectionObject\n"
     "\n"
     "Return a list of SelectionObjects generated by the last mouse click.\n"
     "If no document name is given the active document is used and '*'\n"
     "means all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`."},
    {"enablePickedList",
     (PyCFunction)sEnablePickedList,
     METH_VARARGS,
     "enablePickedList(enable=True) -> None\n"
     "\n"
     "Enable/disable pick list.\n"
     "\n"
     "enable : bool"},
    {"getCompleteSelection",
     (PyCFunction)sGetCompleteSelection,
     METH_VARARGS,
     "getCompleteSelection(resolve=ResolveMode.OldStyleElement) -> list\n"
     "\n"
     "Return a list of selected objects across all documents.\n"
     "\n"
     "resolve : int"},
    {"getSelectionEx",
     (PyCFunction)sGetSelectionEx,
     METH_VARARGS,
     "getSelectionEx(docName, resolve=ResolveMode.OldStyleElement, single=False) -> list of "
     "Gui.SelectionObject\n"
     "\n"
     "Return a list of SelectionObjects. If no document name is given the\n"
     "active document is used and '*' means all documents.\n"
     "The SelectionObjects contain a variety of information about the selection,\n"
     "e.g. subelement names.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int\n    Resolve the subname references.\n"
     "    0: do not resolve, 1: resolve, 2: resolve with element map.\n"
     "single : bool\n    Only return if there is only one selection."},
    {"getSelectionObject",
     (PyCFunction)sGetSelectionObject,
     METH_VARARGS,
     "getSelectionObject(docName, objName, subName, point) -> Gui.SelectionObject\n"
     "\n"
     "Return a SelectionObject.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "objName : str\n    Name of the `App.DocumentObject` to select.\n"
     "subName : str\n    Subelement name.\n"
     "point : tuple\n    Coordinates of the point to pick."},
    {"setSelectionStyle",
     (PyCFunction)sSetSelectionStyle,
     METH_VARARGS,
     "setSelectionStyle(selectionStyle) -> None\n"
     "\n"
     "Change the selection style. 0 for normal selection, 1 for greedy selection\n"
     "\n"
     "selectionStyle : int"},
    {"addObserver",
     (PyCFunction)sAddSelObserver,
     METH_VARARGS,
     "addObserver(object, resolve=ResolveMode.OldStyleElement) -> None\n"
     "\n"
     "Install an observer.\n"
     "\n"
     "object : object\n    Python object instance.\n"
     "resolve : int"},
    {"removeObserver",
     (PyCFunction)sRemSelObserver,
     METH_VARARGS,
     "removeObserver(object) -> None\n"
     "\n"
     "Uninstall an observer.\n"
     "\n"
     "object : object\n    Python object instance."},
    {"addSelectionGate",
     (PyCFunction)sAddSelectionGate,
     METH_VARARGS,
     "addSelectionGate(filter, resolve=ResolveMode.OldStyleElement) -> None\n"
     "\n"
     "Activate the selection gate.\n"
     "The selection gate will prohibit all selections that do not match\n"
     "the given selection criteria.\n"
     "\n"
     "filter : str, SelectionFilter, object\n"
     "resolve : int\n"
     "\n"
     "Examples strings are:\n"
     "Gui.Selection.addSelectionGate('SELECT Part::Feature SUBELEMENT Edge')\n"
     "Gui.Selection.addSelectionGate('SELECT Robot::RobotObject')\n"
     "\n"
     "An instance of SelectionFilter can also be set:\n"
     "filter = Gui.Selection.Filter('SELECT Part::Feature SUBELEMENT Edge')\n"
     "Gui.Selection.addSelectionGate(filter)\n"
     "\n"
     "The most flexible approach is to write a selection gate class that\n"
     "implements the method 'allow':\n"
     "class Gate:\n"
     "    def allow(self,doc,obj,sub):\n"
     "        return (sub[0:4] == 'Face')\n"
     "Gui.Selection.addSelectionGate(Gate())"},
    {"removeSelectionGate",
     (PyCFunction)sRemoveSelectionGate,
     METH_VARARGS,
     "removeSelectionGate() -> None\n"
     "\n"
     "Remove the active selection gate."},
    {"setVisible",
     (PyCFunction)sSetVisible,
     METH_VARARGS,
     "setVisible(visible=None) -> None\n"
     "\n"
     "Set visibility of all selection items.\n"
     "\n"
     "visible : bool, None\n    If None, then toggle visibility."},
    {"pushSelStack",
     (PyCFunction)sPushSelStack,
     METH_VARARGS,
     "pushSelStack(clearForward=True, overwrite=False) -> None\n"
     "\n"
     "Push current selection to stack.\n"
     "\n"
     "clearForward : bool\n    Clear the forward selection stack.\n"
     "overwrite : bool\n    Overwrite the top back selection stack with current selection."},
    {"hasSelection",
     (PyCFunction)sHasSelection,
     METH_VARARGS,
     "hasSelection(docName, resolve=ResolveMode.NoResolve) -> bool\n"
     "\n"
     "Check if there is any selection. If no document name is given,\n"
     "checks selections in all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int"},
    {"hasSubSelection",
     (PyCFunction)sHasSubSelection,
     METH_VARARGS,
     "hasSubSelection(docName, subElement=False) -> bool\n"
     "\n"
     "Check if there is any selection with subname. If no document name\n"
     "is given the active document is used and '*' means all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "subElement : bool"},
    {"getSelectionFromStack",
     (PyCFunction)sGetSelectionFromStack,
     METH_VARARGS,
     "getSelectionFromStack(docName, resolve=ResolveMode.OldStyleElement, index=0) -> list of "
     "Gui.SelectionObject\n"
     "\n"
     "Return SelectionObjects from selection stack. If no document name is given\n"
     "the active document is used and '*' means all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int\n    Resolve the subname references.\n"
     "    0: do not resolve, 1: resolve, 2: resolve with element map.\n"
     "index : int\n    Select stack index.\n"
     "    0: last pushed selection, > 0: trace back, < 0: trace forward."},
    {nullptr, nullptr, 0, nullptr} /* Sentinel */
};

PyObject* sAddSelection(PyObject* /*self*/, PyObject* args)
{
    SelectionLogDisabler disabler(true);
    PyObject* clearPreselect = Py_True;
    char* objname;
    char* docname;
    char* subname = nullptr;
    float x = 0, y = 0, z = 0;
    if (
        PyArg_ParseTuple(args, "ss|sfffO!", &docname, &objname, &subname, &x, &y, &z, &PyBool_Type, &clearPreselect)
    ) {
        Selection()
            .addSelection(docname, objname, subname, x, y, z, nullptr, Base::asBoolean(clearPreselect));
        Py_Return;
    }

    PyErr_Clear();
    PyObject* object;
    subname = nullptr;
    x = 0, y = 0, z = 0;
    if (PyArg_ParseTuple(
            args,
            "O!|sfffO!",
            &(App::DocumentObjectPy::Type),
            &object,
            &subname,
            &x,
            &y,
            &z,
            &PyBool_Type,
            &clearPreselect
        )) {
        auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
        App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
        if (!docObj || !docObj->isAttachedToDocument()) {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
            return nullptr;
        }

        Selection().addSelection(
            docObj->getDocument()->getName(),
            docObj->getNameInDocument(),
            subname,
            x,
            y,
            z,
            nullptr,
            Base::asBoolean(clearPreselect)
        );
        Py_Return;
    }

    PyErr_Clear();
    PyObject* sequence;
    if (PyArg_ParseTuple(
            args,
            "O!O|O!",
            &(App::DocumentObjectPy::Type),
            &object,
            &sequence,
            &PyBool_Type,
            &clearPreselect
        )) {
        auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
        App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
        if (!docObj || !docObj->isAttachedToDocument()) {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
            return nullptr;
        }

        try {
            if (PyTuple_Check(sequence) || PyList_Check(sequence)) {
                Py::Sequence list(sequence);
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    std::string subname = static_cast<std::string>(Py::String(*it));
                    Selection().addSelection(
                        docObj->getDocument()->getName(),
                        docObj->getNameInDocument(),
                        subname.c_str(),
                        0,
                        0,
                        0,
                        nullptr,
                        Base::asBoolean(clearPreselect)
                    );
                }
                Py_Return;
            }
        }
        catch (const Py::Exception&) {
            // do nothing here
        }
    }

    PyErr_SetString(PyExc_ValueError, "type must be 'DocumentObject[,subname[,x,y,z]]' or 'DocumentObject, list or tuple of subnames'");

    return nullptr;
}

PyObject* sUpdateSelection(PyObject* /*self*/, PyObject* args)
{
    PyObject* show;
    PyObject* object;
    char* subname = nullptr;
    if (
        !PyArg_ParseTuple(args, "O!O!|s", &PyBool_Type, &show, &(App::DocumentObjectPy::Type), &object, &subname)
    ) {
        return nullptr;
    }

    auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
    App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
    if (!docObj || !docObj->isAttachedToDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
        return nullptr;
    }

    Selection().updateSelection(
        Base::asBoolean(show),
        docObj->getDocument()->getName(),
        docObj->getNameInDocument(),
        subname
    );

    Py_Return;
}


PyObject* sRemoveSelection(PyObject* /*self*/, PyObject* args)
{
    SelectionLogDisabler disabler(true);
    char *docname, *objname;
    char* subname = nullptr;
    if (PyArg_ParseTuple(args, "ss|s", &docname, &objname, &subname)) {
        Selection().rmvSelection(docname, objname, subname);
        Py_Return;
    }

    PyErr_Clear();
    PyObject* object;
    subname = nullptr;
    if (!PyArg_ParseTuple(args, "O!|s", &(App::DocumentObjectPy::Type), &object, &subname)) {
        return nullptr;
    }

    auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
    App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
    if (!docObj || !docObj->isAttachedToDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
        return nullptr;
    }

    Selection().rmvSelection(docObj->getDocument()->getName(), docObj->getNameInDocument(), subname);

    Py_Return;
}

PyObject* sClearSelection(PyObject* /*self*/, PyObject* args)
{
    SelectionLogDisabler disabler(true);
    PyObject* clearPreSelect = Py_True;
    char* documentName = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &clearPreSelect)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "|sO!", &documentName, &PyBool_Type, &clearPreSelect)) {
            return nullptr;
        }
    }
    Selection().clearSelection(documentName, Base::asBoolean(clearPreSelect));

    Py_Return;
}

ResolveMode toEnum(int value)
{
    switch (value) {
        case 0:
            return ResolveMode::NoResolve;
        case 1:
            return ResolveMode::OldStyleElement;
        case 2:
            return ResolveMode::NewStyleElement;
        case 3:
            return ResolveMode::FollowLink;
        default:
            throw Base::ValueError("Wrong enum value");
    }
}

PyObject* sIsSelected(PyObject* /*self*/, PyObject* args)
{
    PyObject* object;
    char* subname = nullptr;
    int resolve = 1;
    if (!PyArg_ParseTuple(args, "O!|si", &(App::DocumentObjectPy::Type), &object, &subname, &resolve)) {
        return nullptr;
    }

    try {
        auto docObj = static_cast<App::DocumentObjectPy*>(object);
        bool ok = Selection().isSelected(docObj->getDocumentObjectPtr(), subname, toEnum(resolve));

        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}

PyObject* sCountObjectsOfType(PyObject* /*self*/, PyObject* args)
{
    char* objecttype;
    char* document = nullptr;
    int resolve = 1;
    if (!PyArg_ParseTuple(args, "s|si", &objecttype, &document, &resolve)) {
        return nullptr;
    }

    try {
        unsigned int count = Selection().countObjectsOfType(objecttype, document, toEnum(resolve));
        return PyLong_FromLong(count);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}

PyObject* sGetSelection(PyObject* /*self*/, PyObject* args)
{
    char* documentName = nullptr;
    int resolve = 1;
    PyObject* single = Py_False;
    if (!PyArg_ParseTuple(args, "|siO!", &documentName, &resolve, &PyBool_Type, &single)) {
        return nullptr;
    }

    try {
        std::vector<SelectionSingleton::SelObj> sel;
        sel = Selection().getSelection(documentName, toEnum(resolve), Base::asBoolean(single));

        std::set<App::DocumentObject*> noduplicates;
        std::vector<App::DocumentObject*> selectedObjects;  // keep the order of selection
        Py::List list;
        for (const auto& it : sel) {
            if (noduplicates.insert(it.pObject).second) {
                selectedObjects.push_back(it.pObject);
            }
        }
        for (const auto& selectedObject : selectedObjects) {
            list.append(Py::asObject(selectedObject->getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    catch (Py::Exception&) {
        return nullptr;
    }
}

PyObject* sEnablePickedList(PyObject* /*self*/, PyObject* args)
{
    PyObject* enable = Py_True;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &enable)) {
        return nullptr;
    }

    Selection().enablePickedList(Base::asBoolean(enable));

    Py_Return;
}

PyObject* sSetPreselection(PyObject* /*self*/, PyObject* args, PyObject* kwd)
{
    PyObject* object;
    const char* subname = nullptr;
    float x = 0, y = 0, z = 0;
    int type = 1;
    static const std::array<const char*, 7> kwlist {"obj", "subname", "x", "y", "z", "tp", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(
            args,
            kwd,
            "O!|sfffi",
            kwlist,
            &(App::DocumentObjectPy::Type),
            &object,
            &subname,
            &x,
            &y,
            &z,
            &type
        )) {
        auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
        App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
        if (!docObj || !docObj->isAttachedToDocument()) {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
            return nullptr;
        }

        Selection().setPreselect(
            docObj->getDocument()->getName(),
            docObj->getNameInDocument(),
            subname,
            x,
            y,
            z,
            static_cast<SelectionChanges::MsgSource>(type)
        );
        Py_Return;
    }

    PyErr_SetString(PyExc_ValueError, "type must be 'DocumentObject[,subname[,x,y,z]]'");

    return nullptr;
}

PyObject* sGetPreselection(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    const SelectionChanges& sel = Selection().getPreselection();
    SelectionObject obj(sel);

    return obj.getPyObject();
}

PyObject* sRemPreselection(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Selection().rmvPreselect();

    Py_Return;
}

PyObject* sGetCompleteSelection(PyObject* /*self*/, PyObject* args)
{
    int resolve = 1;
    if (!PyArg_ParseTuple(args, "|i", &resolve)) {
        return nullptr;
    }

    try {
        std::vector<SelectionSingleton::SelObj> sel;
        sel = Selection().getCompleteSelection(toEnum(resolve));

        Py::List list;
        for (const auto& it : sel) {
            SelectionObject obj(SelectionChanges(
                SelectionChanges::AddSelection,
                it.DocName,
                it.FeatName,
                it.SubName,
                it.TypeName,
                it.x,
                it.y,
                it.z
            ));
            list.append(Py::asObject(obj.getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    catch (Py::Exception&) {
        return nullptr;
    }
}

PyObject* sGetSelectionEx(PyObject* /*self*/, PyObject* args)
{
    char* documentName = nullptr;
    int resolve = 1;
    PyObject* single = Py_False;
    if (!PyArg_ParseTuple(args, "|siO!", &documentName, &resolve, &PyBool_Type, &single)) {
        return nullptr;
    }

    try {
        std::vector<SelectionObject> sel;
        sel = Selection().getSelectionEx(
            documentName,
            App::DocumentObject::getClassTypeId(),
            toEnum(resolve),
            Base::asBoolean(single)
        );

        Py::List list;
        for (auto& it : sel) {
            list.append(Py::asObject(it.getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    catch (Py::Exception&) {
        return nullptr;
    }
}

PyObject* sGetPickedList(PyObject* /*self*/, PyObject* args)
{
    char* documentName = nullptr;
    if (!PyArg_ParseTuple(args, "|s", &documentName)) {
        return nullptr;
    }

    std::vector<SelectionObject> sel;
    sel = Selection().getPickedListEx(documentName);

    try {
        Py::List list;
        for (auto& it : sel) {
            list.append(Py::asObject(it.getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (Py::Exception&) {
        return nullptr;
    }
}

PyObject* sGetSelectionObject(PyObject* /*self*/, PyObject* args)
{
    char *docName, *objName, *subName;
    PyObject* tuple = nullptr;
    if (!PyArg_ParseTuple(args, "sss|O!", &docName, &objName, &subName, &PyTuple_Type, &tuple)) {
        return nullptr;
    }

    try {
        Base::Vector3d pickedPoint;
        auto* pickedPointPtr = static_cast<const Base::Vector3d*>(nullptr);
        if (subName[0] != '\0' && tuple) {
            Py::Tuple t(tuple);
            double x = (double)Py::Float(t.getItem(0));
            double y = (double)Py::Float(t.getItem(1));
            double z = (double)Py::Float(t.getItem(2));
            pickedPoint = Base::Vector3d(x, y, z);
            pickedPointPtr = &pickedPoint;
        }

        PythonSelectionObject selObj(docName, objName, subName, pickedPointPtr);
        return selObj.getPyObject();
    }
    catch (const Py::Exception&) {
        return nullptr;
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}

PyObject* sSetSelectionStyle(PyObject* /*self*/, PyObject* args)
{
    int selStyle = 0;
    if (!PyArg_ParseTuple(args, "i", &selStyle)) {
        return nullptr;
    }

    PY_TRY
    {
        Selection().setSelectionStyle(
            selStyle == 0 ? SelectionStyle::NormalSelection : SelectionStyle::GreedySelection
        );
        Py_Return;
    }
    PY_CATCH;
}

PyObject* sAddSelObserver(PyObject* /*self*/, PyObject* args)
{
    PyObject* o;
    int resolve = 1;
    if (!PyArg_ParseTuple(args, "O|i", &o, &resolve)) {
        return nullptr;
    }

    PY_TRY
    {
        SelectionObserverPython::addObserver(Py::Object(o), toEnum(resolve));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* sRemSelObserver(PyObject* /*self*/, PyObject* args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O", &o)) {
        return nullptr;
    }

    PY_TRY
    {
        SelectionObserverPython::removeObserver(Py::Object(o));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* sAddSelectionGate(PyObject* /*self*/, PyObject* args)
{
    char* filter;
    int resolve = 1;
    if (PyArg_ParseTuple(args, "s|i", &filter, &resolve)) {
        PY_TRY
        {
            Selection().addSelectionGate(new SelectionFilterGate(filter), toEnum(resolve));
            Py_Return;
        }
        PY_CATCH;
    }

    PyErr_Clear();
    PyObject* filterPy;
    if (PyArg_ParseTuple(args, "O!|i", SelectionFilterPy::type_object(), &filterPy, resolve)) {
        PY_TRY
        {
            Selection().addSelectionGate(
                new SelectionFilterGatePython(SelectionFilterPy::cast(filterPy)),
                toEnum(resolve)
            );
            Py_Return;
        }
        PY_CATCH;
    }

    PyErr_Clear();
    PyObject* gate;
    if (PyArg_ParseTuple(args, "O|i", &gate, &resolve)) {
        PY_TRY
        {
            Selection().addSelectionGate(
                new SelectionGatePython(Py::Object(gate, false)),
                toEnum(resolve)
            );
            Py_Return;
        }
        PY_CATCH;
    }

    PyErr_SetString(PyExc_ValueError, "Argument is neither string nor SelectionFiler nor SelectionGate");

    return nullptr;
}

PyObject* sRemoveSelectionGate(PyObject* /*self*/, PyObject* args)
{
    const char* pDocName = "";
    if (!PyArg_ParseTuple(args, "|s", &pDocName)) {
        return nullptr;
    }

    PY_TRY
    {
        Selection().rmvSelectionGate(pDocName);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* sSetVisible(PyObject* /*self*/, PyObject* args)
{
    PyObject* visible = Py_None;
    if (!PyArg_ParseTuple(args, "|O", &visible)) {
        return nullptr;
    }

    PY_TRY
    {
        auto vis = SelectionSingleton::VisToggle;
        Base::PyTypeCheck(&visible, &PyBool_Type);
        if (visible) {
            vis = PyObject_IsTrue(visible) ? SelectionSingleton::VisShow
                                           : SelectionSingleton::VisHide;
        }

        Selection().setVisible(vis);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* sPushSelStack(PyObject* /*self*/, PyObject* args)
{
    PyObject* clear = Py_True;
    PyObject* overwrite = Py_False;
    if (!PyArg_ParseTuple(args, "|O!O!", &PyBool_Type, &clear, &PyBool_Type, &overwrite)) {
        return nullptr;
    }

    Selection().selStackPush(Base::asBoolean(clear), Base::asBoolean(overwrite));

    Py_Return;
}

PyObject* sHasSelection(PyObject* /*self*/, PyObject* args)
{
    const char* doc = nullptr;
    int resolve = 0;
    if (!PyArg_ParseTuple(args, "|si", &doc, &resolve)) {
        return nullptr;
    }

    PY_TRY
    {
        bool ret;
        if (doc || resolve > 0) {
            ret = Selection().hasSelection(doc, toEnum(resolve));
        }
        else {
            ret = Selection().hasSelection();
        }

        return Py::new_reference_to(Py::Boolean(ret));
    }
    PY_CATCH;
}

PyObject* sHasSubSelection(PyObject* /*self*/, PyObject* args)
{
    const char* doc = nullptr;
    PyObject* subElement = Py_False;
    if (!PyArg_ParseTuple(args, "|sO!", &doc, &PyBool_Type, &subElement)) {
        return nullptr;
    }

    PY_TRY
    {
        return Py::new_reference_to(
            Py::Boolean(Selection().hasSubSelection(doc, Base::asBoolean(subElement)))
        );
    }
    PY_CATCH;
}

PyObject* sGetSelectionFromStack(PyObject* /*self*/, PyObject* args)
{
    char* documentName = nullptr;
    int resolve = 1;
    int index = 0;
    if (!PyArg_ParseTuple(args, "|sii", &documentName, &resolve, &index)) {
        return nullptr;
    }

    PY_TRY
    {
        Py::List list;
        for (auto& sel : Selection().selStackGet(documentName, toEnum(resolve), index)) {
            list.append(Py::asObject(sel.getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    PY_CATCH;
}

}  // namespace

PyMethodDef* Gui::SelectionPy::methods()
{
    return selectionMethods;
}
