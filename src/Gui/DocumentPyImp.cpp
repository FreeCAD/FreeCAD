/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
#endif

#include <App/Document.h>
#include <Base/Matrix.h>
#include <Base/MatrixPy.h>
#include <Base/Stream.h>

#include "Application.h"
#include "MergeDocuments.h"
#include "MDIView.h"
#include "ViewProviderExtern.h"

// inclusion of the generated files (generated out of DocumentPy.xml)
#include "DocumentPy.h"
#include "DocumentPy.cpp"
#include <App/DocumentObjectPy.h>
#include "Tree.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderDocumentObjectPy.h"
#include "ViewProviderPy.h"
#include "ViewProviderDocumentObjectPy.h"


using namespace Gui;

// returns a string which represent the object e.g. when printed in python
std::string DocumentPy::representation() const
{
    std::stringstream str;
    str << "<GUI Document object at " << getDocumentPtr() << ">";

    return str.str();
}


PyObject* DocumentPy::show(PyObject *args)
{
    char *psFeatStr;
    if (!PyArg_ParseTuple(args, "s;Name of the Feature to show have to be given!", &psFeatStr))
        return nullptr;

    PY_TRY {
        getDocumentPtr()->setShow(psFeatStr);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* DocumentPy::hide(PyObject *args)
{
    char *psFeatStr;
    if (!PyArg_ParseTuple(args, "s;Name of the Feature to hide have to be given!", &psFeatStr))
        return nullptr;

    PY_TRY {
        getDocumentPtr()->setHide(psFeatStr);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* DocumentPy::setPos(PyObject *args)
{
    char *psFeatStr;
    Base::Matrix4D mat;
    PyObject *pcMatObj;
    if (!PyArg_ParseTuple(args, "sO!;Name of the Feature and the transformation matrix have to be given!",
                          &psFeatStr, &(Base::MatrixPy::Type), &pcMatObj))
        return nullptr;

    mat = static_cast<Base::MatrixPy*>(pcMatObj)->value();

    PY_TRY {
        getDocumentPtr()->setPos(psFeatStr,mat);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* DocumentPy::setEdit(PyObject *args)
{
    char *psFeatStr;
    int mod = 0;
    char *subname = nullptr;
    ViewProvider *vp = nullptr;
    App::DocumentObject *obj = nullptr;

    // by name
    if (PyArg_ParseTuple(args, "s|is", &psFeatStr,&mod,&subname)) {
        obj = getDocumentPtr()->getDocument()->getObject(psFeatStr);
        if (!obj) {
            PyErr_Format(Base::PyExc_FC_GeneralError, "No such object found in document: '%s'", psFeatStr);
            return nullptr;
        }
    }
    else {
        PyErr_Clear();
        PyObject *pyObj;
        if (!PyArg_ParseTuple(args, "O|is", &pyObj,&mod,&subname))
            return nullptr;

        if (PyObject_TypeCheck(pyObj,&App::DocumentObjectPy::Type)) {
            obj = static_cast<App::DocumentObjectPy*>(pyObj)->getDocumentObjectPtr();
        }
        else if (PyObject_TypeCheck(pyObj,&ViewProviderPy::Type)) {
            vp = static_cast<ViewProviderPy*>(pyObj)->getViewProviderPtr();
        }
        else {
            PyErr_SetString(PyExc_TypeError,"Expect the first argument to be string, DocumentObject or ViewObject");
            return nullptr;
        }
    }

    if (!vp) {
        if (!obj || !obj->getNameInDocument() || !(vp=Application::Instance->getViewProvider(obj))) {
            PyErr_SetString(PyExc_ValueError,"Invalid document object");
            return nullptr;
        }
    }

    bool ok = getDocumentPtr()->setEdit(vp,mod,subname);

    return PyBool_FromLong(ok ? 1 : 0);
}

PyObject* DocumentPy::getInEdit(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    ViewProvider* vp = getDocumentPtr()->getInEdit();
    if (vp)
        return vp->getPyObject();

    Py_Return;
}

PyObject* DocumentPy::resetEdit(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getDocumentPtr()->resetEdit();

    Py_Return;
}

PyObject* DocumentPy::addAnnotation(PyObject *args)
{
    char *psAnnoName,*psFileName,*psModName = nullptr;
    if (!PyArg_ParseTuple(args, "ss|s;Name of the Annotation and a file name have to be given!",
                          &psAnnoName,&psFileName,&psModName))
        return nullptr;

    PY_TRY {
        auto pcExt = new ViewProviderExtern();

        pcExt->setModeByFile(psModName ? psModName : "Main", psFileName);
        pcExt->adjustDocumentName(getDocumentPtr()->getDocument()->getName());
        getDocumentPtr()->setAnnotationViewProvider(psAnnoName,pcExt);

        Py_Return;
    }
    PY_CATCH;
}

PyObject* DocumentPy::update(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getDocumentPtr()->onUpdate();
        Py_Return;
    }
    PY_CATCH;
}

PyObject* DocumentPy::getObject(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s", &sName))
        return nullptr;

    PY_TRY {
        ViewProvider *pcView = getDocumentPtr()->getViewProviderByName(sName);
        if (pcView)
            return pcView->getPyObject();
        else
            Py_Return;
    }
    PY_CATCH;
}

PyObject* DocumentPy::activeObject(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        App::DocumentObject *pcFtr = getDocumentPtr()->getDocument()->getActiveObject();
        if (pcFtr) {
            ViewProvider *pcView = getDocumentPtr()->getViewProvider(pcFtr);
            return pcView->getPyObject();
        }
        else {
            Py_Return;
        }
    }
    PY_CATCH;
}

PyObject* DocumentPy::activeView(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        Gui::MDIView  *pcView = getDocumentPtr()->getActiveView();
        if (pcView){
            // already incremented in getPyObject().
            return pcView->getPyObject();
        }
        else {
            Py_Return;
        }
    }
    PY_CATCH;
}

PyObject* DocumentPy::mdiViewsOfType(PyObject *args)
{
    char* sType;
    if (!PyArg_ParseTuple(args, "s", &sType))
        return nullptr;

    Base::Type type = Base::Type::fromName(sType);
    if (type.isBad()) {
        PyErr_Format(PyExc_TypeError, "'%s' is not a valid type", sType);
        return nullptr;
    }

    PY_TRY {
        std::list<Gui::MDIView*> views = getDocumentPtr()->getMDIViewsOfType(type);
        Py::List list;
        for (auto it : views)
            list.append(Py::asObject(it->getPyObject()));
        return Py::new_reference_to(list);
    }
    PY_CATCH;
}

PyObject* DocumentPy::save(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        bool ok = getDocumentPtr()->save();
        return Py::new_reference_to(Py::Boolean(ok));
    }
    PY_CATCH;
}

PyObject* DocumentPy::saveAs(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        bool ok = getDocumentPtr()->saveAs();
        return Py::new_reference_to(Py::Boolean(ok));
    }
    PY_CATCH;
}

PyObject* DocumentPy::sendMsgToViews(PyObject *args)
{
    char* msg;
    if (!PyArg_ParseTuple(args, "s", &msg))
        return nullptr;

    PY_TRY {
        getDocumentPtr()->sendMsgToViews(msg);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* DocumentPy::mergeProject(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return nullptr;

    PY_TRY {
        Base::FileInfo fi(filename);
        Base::ifstream str(fi, std::ios::in | std::ios::binary);
        App::Document* doc = getDocumentPtr()->getDocument();
        MergeDocuments md(doc);
        md.importObjects(str);

        Py_Return;
    }
    PY_CATCH;
}

PyObject* DocumentPy::toggleTreeItem(PyObject *args)
{
    PyObject *object;
    const char *subname = nullptr;
    int mod = 0;
    if (!PyArg_ParseTuple(args,"O!|is",&(App::DocumentObjectPy::Type), &object,&mod,&subname))
        return nullptr;

    App::DocumentObject* Object = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    App::DocumentObject* parent = nullptr;
    if (subname) {
        App::DocumentObject* sobj = Object->getSubObject(subname);
        if (!sobj) {
            PyErr_SetString(PyExc_ValueError, "Subobject not found");
            return nullptr;
        }

        parent = Object;
        Object = sobj;
    }

    // get the gui document of the Assembly Item
    //ActiveAppDoc = Item->getDocument();
    //ActiveGuiDoc = Gui::Application::Instance->getDocument(getDocumentPtr());
    auto ActiveVp = dynamic_cast<Gui::ViewProviderDocumentObject*>(getDocumentPtr()->getViewProvider(Object));
    switch (mod) {
        case 0:
            getDocumentPtr()->signalExpandObject(*ActiveVp, TreeItemMode::ToggleItem, parent, subname);
            break;
        case 1:
            getDocumentPtr()->signalExpandObject(*ActiveVp, TreeItemMode::CollapseItem, parent, subname);
            break;
        case 2:
            getDocumentPtr()->signalExpandObject(*ActiveVp, TreeItemMode::ExpandItem, parent, subname);
            break;
        case 3:
            getDocumentPtr()->signalExpandObject(*ActiveVp, TreeItemMode::ExpandPath, parent, subname);
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "Item mode out of range");
            return nullptr;
    }

    Py_Return;
}

PyObject* DocumentPy::scrollToTreeItem(PyObject *args)
{
    PyObject *view;
    if (!PyArg_ParseTuple(args,"O!",&(Gui::ViewProviderDocumentObjectPy::Type), &view))
        return nullptr;

    Gui::ViewProviderDocumentObject* vp = static_cast<ViewProviderDocumentObjectPy*>
            (view)->getViewProviderDocumentObjectPtr();
    getDocumentPtr()->signalScrollToObject(*vp);

    Py_Return;
}

PyObject* DocumentPy::toggleInSceneGraph(PyObject *args) {
    PyObject *view;
    if (!PyArg_ParseTuple(args,"O!",&(Gui::ViewProviderPy::Type), &view))
        return nullptr;

    Gui::ViewProvider* vp = static_cast<ViewProviderPy*>(view)->getViewProviderPtr();
    getDocumentPtr()->toggleInSceneGraph(vp);

    Py_Return;
}

Py::Object DocumentPy::getActiveObject() const
{
    App::DocumentObject *object = getDocumentPtr()->getDocument()->getActiveObject();
    if (object) {
        ViewProvider *viewObj = getDocumentPtr()->getViewProvider(object);
        return Py::Object(viewObj->getPyObject(), true);
    }
    else {
        return Py::None();
    }
}

void DocumentPy::setActiveObject(Py::Object /*arg*/)
{
    throw Py::AttributeError("'Document' object attribute 'ActiveObject' is read-only");
}

Py::Object DocumentPy::getActiveView() const
{
    Gui::MDIView *view = getDocumentPtr()->getActiveView();
    if (view) {
        // already incremented in getPyObject().
        return Py::Object(view->getPyObject(), true);
    }
    else {
        return Py::None();
    }
}

void DocumentPy::setActiveView(Py::Object /*arg*/)
{
    throw Py::AttributeError("'Document' object attribute 'ActiveView' is read-only");
}

Py::Object DocumentPy::getDocument() const
{
    App::Document *doc = getDocumentPtr()->getDocument();
    if (doc) {
        // already incremented in getPyObject().
        return Py::Object(doc->getPyObject(), true);
    }
    else {
        return Py::None();
    }
}

Py::Object DocumentPy::getEditingTransform() const
{
    return Py::asObject(new Base::MatrixPy(new Base::Matrix4D(
                    getDocumentPtr()->getEditingTransform())));
}

void DocumentPy::setEditingTransform(Py::Object arg)
{
    if (!PyObject_TypeCheck(arg.ptr(),&Base::MatrixPy::Type))
        throw Py::TypeError("Expecting type of matrix");

    getDocumentPtr()->setEditingTransform(
            *static_cast<Base::MatrixPy*>(arg.ptr())->getMatrixPtr());
}

Py::Object DocumentPy::getInEditInfo() const {
    ViewProviderDocumentObject *vp = nullptr;
    std::string subname,subelement;
    int mode = 0;
    getDocumentPtr()->getInEdit(&vp,&subname,&mode,&subelement);
    if (!vp || !vp->getObject() || !vp->getObject()->getNameInDocument())
        return Py::None();

    return Py::TupleN(Py::Object(vp->getObject()->getPyObject(),true),
            Py::String(subname),Py::String(subelement),Py::Int(mode));
}

void DocumentPy::setInEditInfo(Py::Object arg)
{
    PyObject *pyobj;
    const char *subname;
    if (!PyArg_ParseTuple(arg.ptr(), "O!s", &Gui::ViewProviderDocumentObjectPy::Type,
                          &pyobj, &subname))
        throw Py::Exception();

    getDocumentPtr()->setInEdit(static_cast<ViewProviderDocumentObjectPy*>(
                pyobj)->getViewProviderDocumentObjectPtr(),subname);
}

Py::Int DocumentPy::getEditMode() const
{
    int mode = -1;
    getDocumentPtr()->getInEdit(nullptr,nullptr,&mode);

    return Py::Int(mode);
}

Py::Boolean DocumentPy::getTransacting() const
{
    return {getDocumentPtr()->isPerformingTransaction()};
}

Py::Boolean DocumentPy::getModified() const
{
    return {getDocumentPtr()->isModified()};
}

PyObject *DocumentPy::getCustomAttributes(const char* attr) const
{
    // Note: Here we want to return only a document object if its
    // name matches 'attr'. However, it is possible to have an object
    // with the same name as an attribute. If so, we return 0 as other-
    // wise it wouldn't be possible to address this attribute any more.
    // The object must then be addressed by the getObject() method directly.
    if (!this->ob_type->tp_dict) {
        if (PyType_Ready(this->ob_type) < 0)
            return nullptr;
    }

    PyObject* item = PyDict_GetItemString(this->ob_type->tp_dict, attr);
    if (item)
        return nullptr;

    // search for an object with this name
    ViewProvider* obj = getDocumentPtr()->getViewProviderByName(attr);

    return (obj ? obj->getPyObject() : nullptr);
}

int DocumentPy::setCustomAttributes(const char* attr, PyObject *)
{
    // Note: Here we want to return only a document object if its
    // name matches 'attr'. However, it is possible to have an object
    // with the same name as an attribute. If so, we return 0 as other-
    // wise it wouldn't be possible to address this attribute any more.
    // The object must then be addressed by the getObject() method directly.
    if (!this->ob_type->tp_dict) {
        if (PyType_Ready(this->ob_type) < 0)
            return 0;
    }

    PyObject* item = PyDict_GetItemString(this->ob_type->tp_dict, attr);
    if (item)
        return 0;

    ViewProvider* obj = getDocumentPtr()->getViewProviderByName(attr);
    if (obj) {
        std::stringstream str;
        str << "'Document' object attribute '" << attr
            << "' must not be set this way" << std::ends;
        throw Py::AttributeError(str.str());
    }

    return 0;
}
