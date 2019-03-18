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

#include <Base/Matrix.h>
#include <Base/MatrixPy.h>

#include <App/Document.h>

#include "Document.h"
#include "MergeDocuments.h"
#include "ViewProviderExtern.h"

// inclusion of the generated files (generated out of DocumentPy.xml)
#include "DocumentPy.h"
#include "DocumentPy.cpp"
#include <App/DocumentObjectPy.h>
#include "Tree.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderPy.h"
#include "ViewProviderDocumentObjectPy.h"


using namespace Gui;

// returns a string which represent the object e.g. when printed in python
std::string DocumentPy::representation(void) const
{
    std::stringstream str;
    str << "<GUI Document object at " << getDocumentPtr() << ">";

    return str.str();
}


PyObject*  DocumentPy::show(PyObject *args)
{
    char *psFeatStr;
    if (!PyArg_ParseTuple(args, "s;Name of the Feature to show have to be given!",&psFeatStr))     // convert args: Python->C 
        return NULL;  // NULL triggers exception 

    PY_TRY {
        getDocumentPtr()->setShow(psFeatStr);  
        Py_Return;
    } PY_CATCH;
}

PyObject*  DocumentPy::hide(PyObject *args)
{
    char *psFeatStr;
    if (!PyArg_ParseTuple(args, "s;Name of the Feature to hide have to be given!",&psFeatStr))     // convert args: Python->C 
        return NULL;  // NULL triggers exception 

    PY_TRY {
        getDocumentPtr()->setHide(psFeatStr);  
        Py_Return;
    } PY_CATCH;
}

PyObject*  DocumentPy::setPos(PyObject *args)
{
    char *psFeatStr;
    Base::Matrix4D mat;
    PyObject *pcMatObj;
    if (!PyArg_ParseTuple(args, "sO!;Name of the Feature and the transformation matrix have to be given!",
                          &psFeatStr,
                          &(Base::MatrixPy::Type), &pcMatObj))     // convert args: Python->C 
        return NULL;  // NULL triggers exception 

    mat = static_cast<Base::MatrixPy*>(pcMatObj)->value();

    PY_TRY {
        getDocumentPtr()->setPos(psFeatStr,mat);  
        Py_Return;
    } PY_CATCH;
}

PyObject* DocumentPy::setEdit(PyObject *args)
{
    char *psFeatStr;
    int mod = 0;

    // by name
    if (PyArg_ParseTuple(args, "s|i;Name of the object to edit has to be given!", &psFeatStr,&mod)) {
        App::DocumentObject * obj = getDocumentPtr()->getDocument()->getObject(psFeatStr);
        if (!obj) {
            PyErr_Format(Base::BaseExceptionFreeCADError, "No such object found in document: '%s'", psFeatStr);
            return 0;
        }

        bool ok = getDocumentPtr()->setEdit(getDocumentPtr()->getViewProvider(obj),mod);
        return PyBool_FromLong(ok ? 1 : 0);
    }

    // by document object
    PyErr_Clear();
    PyObject *docObj;
    if (PyArg_ParseTuple(args, "O!|i", &(App::DocumentObjectPy::Type), &docObj,&mod)) {
        App::DocumentObject * obj = static_cast<App::DocumentObjectPy*>(docObj)->getDocumentObjectPtr();
        bool ok = getDocumentPtr()->setEdit(getDocumentPtr()->getViewProvider(obj),mod);
        return PyBool_FromLong(ok ? 1 : 0);
    }

    // by view provider
    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!|i", &(Gui::ViewProviderPy::Type), &docObj,&mod)) {
        Gui::ViewProvider * view = static_cast<Gui::ViewProviderPy*>(docObj)->getViewProviderPtr();
        bool ok = getDocumentPtr()->setEdit(view,mod);
        return PyBool_FromLong(ok ? 1 : 0);
    }

    PyErr_SetString(PyExc_TypeError, "Either string, document object or view provider expected.");
    return 0;
}

PyObject* DocumentPy::getInEdit(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    ViewProvider* vp = getDocumentPtr()->getInEdit();
    if (vp) {
        return vp->getPyObject();
    }

    Py_Return;
}

PyObject* DocumentPy::resetEdit(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ";No arguments allowed"))     // convert args: Python->C 
        return NULL;  // NULL triggers exception 
    getDocumentPtr()->resetEdit();

    Py_Return;
}

PyObject*  DocumentPy::addAnnotation(PyObject *args)
{
    char *psAnnoName,*psFileName,*psModName=0;
    if (!PyArg_ParseTuple(args, "ss|s;Name of the Annotation and a file name have to be given!",&psAnnoName,&psFileName,&psModName))     // convert args: Python->C 
        return NULL;  // NULL triggers exception 

    PY_TRY {
        ViewProviderExtern *pcExt = new ViewProviderExtern();

        pcExt->setModeByFile(psModName?psModName:"Main",psFileName);
        pcExt->adjustDocumentName(getDocumentPtr()->getDocument()->getName());

        getDocumentPtr()->setAnnotationViewProvider(psAnnoName,pcExt);

        Py_Return;

    } PY_CATCH;
}

PyObject*  DocumentPy::update(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                       // NULL triggers exception 

    PY_TRY {
        getDocumentPtr()->onUpdate();
        Py_Return;
    } PY_CATCH;
}

PyObject*  DocumentPy::getObject(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s",&sName))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        ViewProvider *pcView = getDocumentPtr()->getViewProviderByName(sName);
        if (pcView)
            return pcView->getPyObject();
        else {
            Py_Return;
        }
    } PY_CATCH;
}

PyObject*  DocumentPy::activeObject(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                       // NULL triggers exception 

    PY_TRY {
        App::DocumentObject *pcFtr = getDocumentPtr()->getDocument()->getActiveObject();
        if (pcFtr) {
            ViewProvider *pcView = getDocumentPtr()->getViewProvider(pcFtr);
            return pcView->getPyObject();
        } else {
            Py_Return;
        }
    } PY_CATCH;
}

PyObject*  DocumentPy::activeView(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        Gui::MDIView  *pcView = getDocumentPtr()->getActiveView();
        if (pcView){
            // already incremented in getPyObject().
            return pcView->getPyObject();
        } else {
            Py_Return;
        }
    } PY_CATCH;
}

PyObject*  DocumentPy::mdiViewsOfType(PyObject *args)
{
    char* sType;
    if (!PyArg_ParseTuple(args, "s", &sType))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    Base::Type type = Base::Type::fromName(sType);
    if (type == Base::Type::badType()) {
        PyErr_Format(Base::BaseExceptionFreeCADError, "'%s' is not a valid type", sType);
        return NULL;
    }

    PY_TRY {
        std::list<Gui::MDIView*> views = getDocumentPtr()->getMDIViewsOfType(type);
        Py::List list;
        for (std::list<Gui::MDIView*>::iterator it = views.begin(); it != views.end(); ++it)
            list.append(Py::asObject((*it)->getPyObject()));
        return Py::new_reference_to(list);
    } PY_CATCH;
}

PyObject*  DocumentPy::sendMsgToViews(PyObject *args)
{
    char* msg;
    if (!PyArg_ParseTuple(args, "s", &msg))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        getDocumentPtr()->sendMsgToViews(msg);
        Py_Return;
    } PY_CATCH;
}

PyObject* DocumentPy::mergeProject(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        Base::FileInfo fi(filename);
        Base::ifstream str(fi, std::ios::in | std::ios::binary);
        App::Document* doc = getDocumentPtr()->getDocument();
        MergeDocuments md(doc);
        md.importObjects(str);
        Py_Return;
    } PY_CATCH;
}

PyObject* DocumentPy::toggleTreeItem(PyObject *args)
{
    PyObject *object=0;
    int mod = 0;
    if (PyArg_ParseTuple(args,"O!|i",&(App::DocumentObjectPy::Type), &object,&mod)) {
        App::DocumentObject* Object = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
        // Should be set!
        assert(Object);    

        // get the gui document of the Assembly Item 
        //ActiveAppDoc = Item->getDocument();
        //ActiveGuiDoc = Gui::Application::Instance->getDocument(getDocumentPtr());
        Gui::ViewProviderDocumentObject* ActiveVp = dynamic_cast<Gui::ViewProviderDocumentObject*> (getDocumentPtr()->getViewProvider(Object));
        assert(ActiveVp);
        switch(mod) {
            case 0: getDocumentPtr()->signalExpandObject(*ActiveVp,Gui::ToggleItem); break;
            case 1: getDocumentPtr()->signalExpandObject(*ActiveVp,Gui::CollapseItem); break;
            case 2: getDocumentPtr()->signalExpandObject(*ActiveVp,Gui::ExpandItem); break;
            case 3: getDocumentPtr()->signalExpandObject(*ActiveVp,Gui::ExpandPath); break;
            default: break;
        }
    }

    Py_Return;
}

PyObject* DocumentPy::scrollToTreeItem(PyObject *args)
{
    PyObject *view;
    if (!PyArg_ParseTuple(args,"O!",&(Gui::ViewProviderDocumentObjectPy::Type), &view))
        return 0;

    Gui::ViewProviderDocumentObject* vp = static_cast<ViewProviderDocumentObjectPy*>
            (view)->getViewProviderDocumentObjectPtr();
    getDocumentPtr()->signalScrollToObject(*vp);
    Py_Return;
}

Py::Object DocumentPy::getActiveObject(void) const
{
    App::DocumentObject *object = getDocumentPtr()->getDocument()->getActiveObject();
    if (object) {
        ViewProvider *viewObj = getDocumentPtr()->getViewProvider(object);
        return Py::Object(viewObj->getPyObject(), true);
    } else {
        return Py::None();
    }
}

void  DocumentPy::setActiveObject(Py::Object /*arg*/)
{
    throw Py::AttributeError("'Document' object attribute 'ActiveObject' is read-only");
}

Py::Object DocumentPy::getActiveView(void) const
{
    Gui::MDIView *view = getDocumentPtr()->getActiveView();
    if (view) {
        // already incremented in getPyObject().
        return Py::Object(view->getPyObject(), true);
    } else {
        return Py::None();
    }
}

void  DocumentPy::setActiveView(Py::Object /*arg*/)
{
    throw Py::AttributeError("'Document' object attribute 'ActiveView' is read-only");
}

Py::Object DocumentPy::getDocument(void) const
{
    App::Document *doc = getDocumentPtr()->getDocument();
    if (doc) {
        // already incremented in getPyObject().
        return Py::Object(doc->getPyObject(), true);
    } else {
        return Py::None();
    }
}

Py::Boolean DocumentPy::getModified(void) const
{
    return Py::Boolean(getDocumentPtr()->isModified());
}

PyObject *DocumentPy::getCustomAttributes(const char* attr) const
{
    // Note: Here we want to return only a document object if its
    // name matches 'attr'. However, it is possible to have an object
    // with the same name as an attribute. If so, we return 0 as other-
    // wise it wouldn't be possible to address this attribute any more.
    // The object must then be addressed by the getObject() method directly.
    if (this->ob_type->tp_dict == NULL) {
        if (PyType_Ready(this->ob_type) < 0)
            return 0;
    }
    PyObject* item = PyDict_GetItemString(this->ob_type->tp_dict, attr);
    if (item) return 0;
    // search for an object with this name
    ViewProvider* obj = getDocumentPtr()->getViewProviderByName(attr);
    return (obj ? obj->getPyObject() : 0);
}

int DocumentPy::setCustomAttributes(const char* attr, PyObject *)
{
    // Note: Here we want to return only a document object if its
    // name matches 'attr'. However, it is possible to have an object
    // with the same name as an attribute. If so, we return 0 as other-
    // wise it wouldn't be possible to address this attribute any more.
    // The object must then be addressed by the getObject() method directly.
    if (this->ob_type->tp_dict == NULL) {
        if (PyType_Ready(this->ob_type) < 0)
            return 0;
    }
    PyObject* item = PyDict_GetItemString(this->ob_type->tp_dict, attr);
    if (item) return 0;
    ViewProvider* obj = getDocumentPtr()->getViewProviderByName(attr);
    if (obj) {
        std::stringstream str;
        str << "'Document' object attribute '" << attr 
            << "' must not be set this way" << std::ends;
        throw Py::AttributeError(str.str());
    }
    
    return 0;
}


