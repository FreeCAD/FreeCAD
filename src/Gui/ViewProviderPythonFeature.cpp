/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QEvent>
# include <QFileInfo>
# include <QMenu>
# include <QPixmap>
# include <boost/bind.hpp>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/draggers/SoDragger.h>
# include <Inventor/manips/SoCenterballManip.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/sensors/SoNodeSensor.h> 
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoRayPickAction.h> 
# include <Inventor/details/SoDetail.h> 
#endif

#include "ViewProviderPythonFeature.h"
#include "Tree.h"
#include "Window.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "WidgetFactory.h"
#include "View3DInventorViewer.h"
#include <App/DocumentObjectPy.h>
#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Interpreter.h>

FC_LOG_LEVEL_INIT("ViewProviderPythonFeature",true,true);


using namespace Gui;

// #0003564: Python objects: updateData calls to proxy instance that should have been deleted
// See https://forum.freecadweb.org/viewtopic.php?f=22&t=30429&p=252429#p252429
#if 0
namespace Gui {

struct ProxyInfo {
    Py::Object viewObject;
    Py::Object proxy;

    ~ProxyInfo() {
        Base::PyGILStateLocker lock;
        viewObject = Py::Object();
        proxy = Py::Object();
    }
};

class PropertyEvent : public QEvent
{
public:
    PropertyEvent(const Gui::ViewProviderDocumentObject* vp, const ProxyInfo &info)
        : QEvent(QEvent::Type(QEvent::User)), view(vp), info(info)
    {
    }

    const Gui::ViewProviderDocumentObject* view;
    ProxyInfo info;
};

class ViewProviderPythonFeatureObserver : public QObject
{
public:
    /// The one and only instance.
    static ViewProviderPythonFeatureObserver* instance();
    /// Destructs the sole instance.
    static void destruct ();
    void slotAppendObject(const Gui::ViewProvider&);
    void slotDeleteObject(const Gui::ViewProvider&);
    void slotDeleteDocument(const Gui::Document&);

private:
    void customEvent(QEvent* e)
    {
        PropertyEvent* pe = static_cast<PropertyEvent*>(e);
        std::set<const Gui::ViewProvider*>::iterator it = viewMap.find(pe->view);
        // Make sure that the object hasn't been deleted in the meantime (#0001522)
        if (it != viewMap.end()) {
            viewMap.erase(it);

            // We check below the python object of the view provider to make
            // sure that the view provider is actually the owner of the proxy
            // object we cached before. This step is necessary to prevent a
            // very obscure bug described here.
            //
            // The proxy caching is only effective when an object is deleted in
            // an event of undo, and we restore the proxy in the event of redo.
            // The object is not really freed while inside undo/redo stack. It
            // gets really deleted from memory when either the user clears the
            // undo/redo stack manually, or the redo stack gets automatically
            // cleared when new transaction is created. FC has no explicit
            // signaling of when the object is really deleted from the memory.
            // This ViewProviderPythonFeatureObserver uses a heuristic to
            // decide when to flush the cache in slotAppendObject(), that is,
            // it sees any cache miss event as the signaling of an redo clear.
            // The bug happens in the very rare event, when the redo stack is
            // cleared when new transaction is added, and the freed object's
            // memory gets immediately reused by c++ allocator for the new
            // object created in the new transaction.  This creates a cache
            // false hit event, where the old deleted view provider's proxy
            // gets mistakenly assigned to the newly created object, which
            // happens to have the exact same memory location. This situation
            // is very rare and really depends on the system's allocator
            // implementation.  However, tests show that it happens regularly
            // in Linux debug build. To prevent this, we use the trick of
            // checking the python object pointer of the view provider to make
            // sure the view provider are in fact the same. We hold the python
            // object reference count, so it never gets freed and reused like
            // its owner view provider.
            //
            // Side note: the original implementation uses property copy and
            // paste to store the proxy object, which is fine, but has the
            // trouble of having to manually freed the copied property. And the
            // original implementation didn't do that in every case, causing
            // memory leak. We now simply stores the python object with
            // reference counting, so no need to worry about deleting
            
            Py::Object viewObject(const_cast<ViewProviderDocumentObject*>(pe->view)->getPyObject(),true);
            if(viewObject.ptr() != pe->info.viewObject.ptr()) {
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                    FC_WARN("invalid proxy cache " << viewObject.ptr() << ", " <<
                            pe->info.viewObject.ptr() << ", " << pe->info.proxy.ptr());
            }else{
                App::Property* prop = pe->view->getPropertyByName("Proxy");
                if (prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
                    prop->setPyObject(pe->info.proxy.ptr());
                }
            }
        }
    }
    static ViewProviderPythonFeatureObserver* _singleton;

    ViewProviderPythonFeatureObserver();
    ~ViewProviderPythonFeatureObserver();
    typedef std::map<
                const App::DocumentObject*,
                ProxyInfo
            > ObjectProxy;

    std::map<const App::Document*, ObjectProxy> proxyMap;
    std::set<const Gui::ViewProvider*> viewMap;
};

}

ViewProviderPythonFeatureObserver* ViewProviderPythonFeatureObserver::_singleton = 0;

ViewProviderPythonFeatureObserver* ViewProviderPythonFeatureObserver::instance()
{
    if (!_singleton)
        _singleton = new ViewProviderPythonFeatureObserver;
    return _singleton;
}

void ViewProviderPythonFeatureObserver::destruct ()
{
    delete _singleton;
    _singleton = 0;
}

void ViewProviderPythonFeatureObserver::slotDeleteDocument(const Gui::Document& d)
{
    App::Document* doc = d.getDocument();
    std::map<const App::Document*, ObjectProxy>::iterator it = proxyMap.find(doc);
    if (it != proxyMap.end()) {
        Base::PyGILStateLocker lock;
        proxyMap.erase(it);
    }
}

void ViewProviderPythonFeatureObserver::slotAppendObject(const Gui::ViewProvider& obj)
{
    if (!obj.isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId()))
        return;
    const Gui::ViewProviderDocumentObject& vp = static_cast<const Gui::ViewProviderDocumentObject&>(obj);
    const App::DocumentObject* docobj = vp.getObject();
    App::Document* doc = docobj->getDocument();
    std::map<const App::Document*, ObjectProxy>::iterator it = proxyMap.find(doc);
    if (it != proxyMap.end()) {
        ObjectProxy::iterator jt = it->second.find(docobj);
        if (jt != it->second.end()) {
            Base::PyGILStateLocker lock;
            try {
                App::Property* prop = vp.getPropertyByName("Proxy");
                if (prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
                    // make this delayed so that the corresponding item in the tree view is accessible
                    QApplication::postEvent(this, new PropertyEvent(&vp, jt->second));
                    // needed in customEvent()
                    viewMap.insert(&vp);
                    it->second.erase(jt);
                }
            }
            catch (Py::Exception& e) {
                e.clear();
            }
        }
        // all cached objects of the documents are already destroyed
        else {
            it->second.clear();
        }
    }
}

void ViewProviderPythonFeatureObserver::slotDeleteObject(const Gui::ViewProvider& obj)
{
    // check this in customEvent() if the object is still there
    std::set<const Gui::ViewProvider*>::iterator it = viewMap.find(&obj);
    if (it != viewMap.end())
        viewMap.erase(it);
    if (!obj.isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId()))
        return;
    const Gui::ViewProviderDocumentObject& vp = static_cast<const Gui::ViewProviderDocumentObject&>(obj);
    const App::DocumentObject* docobj = vp.getObject();
    App::Document* doc = docobj->getDocument();
    if (!doc->getUndoMode())
        return; // object will be deleted immediately, thus we don't need to store anything
    Base::PyGILStateLocker lock;
    try {
        App::Property* prop = vp.getPropertyByName("Proxy");
        if (prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
            auto &info = proxyMap[doc][docobj];
            info.viewObject = Py::asObject(const_cast<ViewProviderDocumentObject&>(vp).getPyObject());
            info.proxy = Py::asObject(prop->getPyObject());
            FC_LOG("proxy cache " << info.viewObject.ptr() << ", " << info.proxy.ptr());
        }
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

ViewProviderPythonFeatureObserver::ViewProviderPythonFeatureObserver()
{
    Gui::Application::Instance->signalDeletedObject.connect(boost::bind
        (&ViewProviderPythonFeatureObserver::slotDeleteObject, this, _1));
    Gui::Application::Instance->signalNewObject.connect(boost::bind
        (&ViewProviderPythonFeatureObserver::slotAppendObject, this, _1));
    Gui::Application::Instance->signalDeleteDocument.connect(boost::bind
        (&ViewProviderPythonFeatureObserver::slotDeleteDocument, this, _1));
}

ViewProviderPythonFeatureObserver::~ViewProviderPythonFeatureObserver()
{
}
#endif

// ----------------------------------------------------------------------------

ViewProviderPythonFeatureImp::ViewProviderPythonFeatureImp(ViewProviderDocumentObject* vp)
  : object(vp), has__object__(false)
{
#if 0
    (void)ViewProviderPythonFeatureObserver::instance();
#endif
}

ViewProviderPythonFeatureImp::~ViewProviderPythonFeatureImp()
{
}

void ViewProviderPythonFeatureImp::init(PyObject *pyobj) {
    Base::PyGILStateLocker lock;
    has__object__ = !!PyObject_HasAttrString(pyobj, "__object__");
#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) FC_PY_GetCallable(pyobj,#_name,py_##_name);
    FC_PY_VIEW_OBJECT
}

QIcon ViewProviderPythonFeatureImp::getIcon() const
{
    if(py_getIcon.isNone())
        return QIcon();

    // default icon
    //static QPixmap px = BitmapFactory().pixmap("Tree_Python");

    // Run the getIcon method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Object ret(Base::pyCall(py_getIcon.ptr()));
        if(ret.isNone())
            return QIcon();

        PythonWrapper wrap;
        wrap.loadGuiModule();
        wrap.loadWidgetsModule();
        QIcon *picon = wrap.toQIcon(ret.ptr());
        if(picon) 
            return *picon;

        std::string content = Py::String(ret).as_std_string("utf-8");
        QPixmap icon;
        // Check if the passed string is a filename, otherwise treat as xpm data
        QFileInfo fi(QString::fromUtf8(content.c_str()));
        if (fi.isFile() && fi.exists()) {
            icon.load(fi.absoluteFilePath());
        } else {
            QByteArray ary;
            int strlen = (int)content.size();
            ary.resize(strlen);
            for (int j=0; j<strlen; j++)
                ary[j]=content[j];
            // Make sure to remove crap around the XPM data
            QList<QByteArray> lines = ary.split('\n');
            QByteArray buffer;
            buffer.reserve(ary.size()+lines.size());
            for (QList<QByteArray>::iterator it = lines.begin(); it != lines.end(); ++it) {
                QByteArray trim = it->trimmed();
                if (!trim.isEmpty()) {
                    buffer.append(trim);
                    buffer.append('\n');
                }
            }
            icon.loadFromData(buffer, "XPM");
        }
        if (!icon.isNull()) {
            return icon;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return QIcon();
}

std::vector<App::DocumentObject*> 
ViewProviderPythonFeatureImp::claimChildren(std::vector<App::DocumentObject*>&& base) const 
{
    if(py_claimChildren.isNone())
        return std::move(base);

    std::vector<App::DocumentObject*> children;
    Base::PyGILStateLocker lock;
    try {
        Py::Sequence list(Base::pyCall(py_claimChildren.ptr()));
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                children.push_back(obj);
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return children;
}

bool ViewProviderPythonFeatureImp::useNewSelectionModel() const
{
    if(py_useNewSelectionModel.isNone())
        return true;
    // Run the useNewSelectionModel method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Py::Callable(py_useNewSelectionModel).apply(Py::Tuple()));
        return (bool)ok;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

std::string ViewProviderPythonFeatureImp::getElement(const SoDetail *det) const
{
    if(py_getElement.isNone())
        return std::string();

    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        PyObject* pivy = 0;
        // Note: As there is no ref'counting mechanism for the SoDetail class we must
        // pass '0' as the last parameter so that the Python object does not 'own'
        // the detail object.
        pivy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoDetail *", (void*)det, 0);
        Py::Tuple args(1);
        args.setItem(0, Py::Object(pivy, true));
        Py::String name(Base::pyCall(py_getElement.ptr(),args.ptr()));
        return (std::string)name;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return std::string();
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::getElementPicked(const SoPickedPoint *pp, std::string &subname) const
{
    if(py_getElementPicked.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        PyObject* pivy = 0;
        pivy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoPickedPoint *", (void*)pp, 0);
        Py::Tuple args(1);
        args.setItem(0, Py::Object(pivy, true));
        Py::Object ret(Base::pyCall(py_getElementPicked.ptr(),args.ptr()));
        if(!ret.isString()) return Rejected;
        subname = ret.as_string();
        return Accepted;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return NotImplemented;
}
SoDetail* ViewProviderPythonFeatureImp::getDetail(const char* name) const
{
    if(py_getDetail.isNone())
        return 0;
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::String(name));
        Py::Object det(Base::pyCall(py_getDetail.ptr(),args.ptr()));
        void* ptr = 0;
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoDetail *", det.ptr(), &ptr, 0);
        SoDetail* detail = reinterpret_cast<SoDetail*>(ptr);
        return detail ? detail->copy() : 0;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return 0;
}

ViewProviderPythonFeatureImp::ValueT ViewProviderPythonFeatureImp::getDetailPath(
        const char* name, SoFullPath *path, bool append, SoDetail *&det) const
{
    if(py_getDetailPath.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    auto length = path->getLength();
    try {
        PyObject* pivy = 0;
        pivy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoFullPath *", (void*)path, 1);
        path->ref();
        Py::Tuple args(3);
        args.setItem(0, Py::String(name));
        args.setItem(1, Py::Object(pivy, true));
        args.setItem(2, Py::Boolean(append));
        Py::Object pyDet(Base::pyCall(py_getDetailPath.ptr(),args.ptr()));
        void* ptr = 0;
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoDetail *", pyDet.ptr(), &ptr, 0);
        SoDetail* detail = reinterpret_cast<SoDetail*>(ptr);
        det = detail ? detail->copy() : 0;
        if(det || pyDet.isTrue())
            return Accepted;
        delete det;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    path->truncate(length);
    return Rejected;
}


std::vector<Base::Vector3d> ViewProviderPythonFeatureImp::getSelectionShape(const char* /*Element*/) const
{
    return std::vector<Base::Vector3d>();
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::setEdit(int ModNum)
{
    if(py_setEdit.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Tuple args(1);
            args.setItem(0, Py::Int(ModNum));
            Py::Object ret(Base::pyCall(py_setEdit.ptr(),args.ptr()));
            if (ret.isNone())
                return NotImplemented;
            Py::Boolean ok(ret);
            bool value = static_cast<bool>(ok);
            return value ? Accepted : Rejected;
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, Py::Int(ModNum));
            Py::Object ret(Base::pyCall(py_setEdit.ptr(),args.ptr()));
            if (ret.isNone())
                return NotImplemented;
            Py::Boolean ok(ret);
            bool value = static_cast<bool>(ok);
            return value ? Accepted : Rejected;
        }
    }
    catch (Py::Exception&) {
        // If an explicit NotImplementedError is raised then
        // handle it differently to other RuntimeError
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        // If a runtime error occurred when calling setEdit
        // then handle it like returning false
        if (PyErr_ExceptionMatches(PyExc_RuntimeError)) {
            PyErr_Clear();
            return Rejected;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return NotImplemented;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::unsetEdit(int ModNum)
{
    if(py_unsetEdit.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Tuple args(1);
            args.setItem(0, Py::Int(ModNum));
            Py::Object ret(Base::pyCall(py_unsetEdit.ptr(),args.ptr()));
            if (ret.isNone())
                return NotImplemented;
            Py::Boolean ok(ret);
            bool value = static_cast<bool>(ok);
            return value ? Accepted : Rejected;
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, Py::Int(ModNum));
            Py::Object ret(Base::pyCall(py_unsetEdit.ptr(),args.ptr()));
            if (ret.isNone())
                return NotImplemented;
            Py::Boolean ok(ret);
            bool value = static_cast<bool>(ok);
            return value ? Accepted : Rejected;
        }
    }
    catch (Py::Exception&) {
        // If an explicit NotImplementedError is raised then
        // handle it differently to other RuntimeError
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        // If a runtime error occurred when calling setEdit
        // then handle it like returning false
        if (PyErr_ExceptionMatches(PyExc_RuntimeError)) {
            PyErr_Clear();
            return Rejected;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return NotImplemented;
}

bool ViewProviderPythonFeatureImp::setEditViewer(View3DInventorViewer *viewer, int ModNum)
{
    if(py_setEditViewer.isNone())
        return false;

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(3);
        args.setItem(0, Py::Object(object->getPyObject(),true));
        args.setItem(1, Py::Object(viewer->getPyObject(),true));
        args.setItem(2, Py::Int(ModNum));
        Py::Object ret(Base::pyCall(py_setEditViewer.ptr(),args.ptr()));
        return ret.isTrue();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return false;
}

bool ViewProviderPythonFeatureImp::unsetEditViewer(View3DInventorViewer *viewer)
{
    if(py_unsetEditViewer.isNone())
        return false;

    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::Object(object->getPyObject(),true));
        args.setItem(1, Py::Object(viewer->getPyObject(),true));
        Py::Object ret(Base::pyCall(py_unsetEditViewer.ptr(),args.ptr()));
        return ret.isTrue();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return false;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::doubleClicked(void)
{
    if(py_doubleClicked.isNone())
        return NotImplemented;

    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Boolean ok(Base::pyCall(py_doubleClicked.ptr()));
            bool value = (bool)ok;
            return value ? Accepted : Rejected;
        }
        else {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            Py::Boolean ok(Base::pyCall(py_doubleClicked.ptr(),args.ptr()));
            bool value = (bool)ok;
            return value ? Accepted : Rejected;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return NotImplemented;
}

void ViewProviderPythonFeatureImp::setupContextMenu(QMenu* menu)
{
    if(py_setupContextMenu.isNone())
        return;
    // Run the attach method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            PythonWrapper wrap;
            wrap.loadGuiModule();
            wrap.loadWidgetsModule();
            Py::Tuple args(1);
            args.setItem(0, wrap.fromQWidget(menu, "QMenu"));
            Base::pyCall(py_setupContextMenu.ptr(),args.ptr());
        }
        else {
            PythonWrapper wrap;
            wrap.loadGuiModule();
            wrap.loadWidgetsModule();
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, wrap.fromQWidget(menu, "QMenu"));
            Base::pyCall(py_setupContextMenu.ptr(),args.ptr());
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void ViewProviderPythonFeatureImp::attach(App::DocumentObject *pcObject)
{
    if(py_attach.isNone())
        return;
    // Run the attach method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Base::pyCall(py_attach.ptr());
        }
        else {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            Base::pyCall(py_attach.ptr(),args.ptr());
        }

        // #0000415: Now simulate a property change event to call
        // claimChildren if implemented.
        pcObject->Label.touch();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void ViewProviderPythonFeatureImp::updateData(const App::Property* prop)
{
    if(py_updateData.isNone())
        return;
    // Run the updateData method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Tuple args(1);
            const char* prop_name = object->getObject()->getPropertyName(prop);
            if (prop_name) {
                args.setItem(0, Py::String(prop_name));
                Base::pyCall(py_updateData.ptr(),args.ptr());
            }
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getObject()->getPyObject(), true));
            const char* prop_name = object->getObject()->getPropertyName(prop);
            if (prop_name) {
                args.setItem(1, Py::String(prop_name));
                Base::pyCall(py_updateData.ptr(),args.ptr());
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void ViewProviderPythonFeatureImp::onChanged(const App::Property* prop)
{
    if(py_onChanged.isNone())
        return;
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Tuple args(1);
            const char* prop_name = object->getPropertyName(prop);
            if (prop_name) {
                args.setItem(0, Py::String(prop_name));
                Base::pyCall(py_onChanged.ptr(),args.ptr());
            }
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            const char* prop_name = object->getPropertyName(prop);
            if (prop_name) {
                args.setItem(1, Py::String(prop_name));
                Base::pyCall(py_onChanged.ptr(),args.ptr());
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void ViewProviderPythonFeatureImp::startRestoring()
{
}

void ViewProviderPythonFeatureImp::finishRestoring()
{
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.isNone()) {
                object->show();
                static_cast<App::PropertyPythonObject*>(proxy)->setValue(Py::Int(1));
            }else if(!py_finishRestoring.isNone())
                Base::pyCall(py_finishRestoring.ptr());
        }
    }catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

bool ViewProviderPythonFeatureImp::onDelete(const std::vector<std::string> & sub)
{
    if(py_onDelete.isNone())
        return true;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple seq(sub.size());
        int index=0;
        for (std::vector<std::string>::const_iterator it = sub.begin(); it != sub.end(); ++it) {
            seq.setItem(index++, Py::String(*it));
        }

        if (has__object__) {
            Py::Tuple args(1);
            args.setItem(0, seq);
            Py::Boolean ok(Base::pyCall(py_onDelete.ptr(),args.ptr()));
            return (bool)ok;
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, seq);
            Py::Boolean ok(Base::pyCall(py_onDelete.ptr(),args.ptr()));
            return (bool)ok;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDelete(App::DocumentObject *obj) const
{
    if(py_canDelete.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0,obj?Py::Object(obj->getPyObject(),true):Py::Object());
        return Py::Boolean(Base::pyCall(py_canDelete.ptr(),args.ptr()))?Accepted:Rejected;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return Rejected;
    }
}

bool ViewProviderPythonFeatureImp::canAddToSceneGraph() const
{
    if(py_canAddToSceneGraph.isNone())
        return true;

    Base::PyGILStateLocker lock;
    try {
        return Py::Boolean(Py::Callable(py_canAddToSceneGraph).apply(Py::Tuple()));
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return true;
}

const char* ViewProviderPythonFeatureImp::getDefaultDisplayMode() const
{
    if(py_getDefaultDisplayMode.isNone())
        return 0;
    // Run the getDefaultDisplayMode method of the proxy object.
    Base::PyGILStateLocker lock;
    static std::string mode;
    try {
        Py::String str(Base::pyCall(py_getDefaultDisplayMode.ptr()));
        //if (str.isUnicode())
        //    str = str.encode("ascii"); // json converts strings into unicode
        mode = str.as_std_string("ascii");
        return mode.c_str();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return 0;
}

std::vector<std::string> ViewProviderPythonFeatureImp::getDisplayModes(void) const
{
    std::vector<std::string> modes;
    if(py_getDefaultDisplayMode.isNone())
        return modes;

    // Run the getDisplayModes method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Sequence list(Base::pyCall(py_getDisplayModes.ptr()));
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Py::String str(*it);
                modes.push_back(str.as_std_string("ascii"));
            }
        }
        else {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            Py::Sequence list(Base::pyCall(py_getDisplayModes.ptr(),args.ptr()));
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Py::String str(*it);
                modes.push_back(str.as_std_string("ascii"));
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return modes;
}

std::string ViewProviderPythonFeatureImp::setDisplayMode(const char* ModeName)
{
    if(py_setDisplayMode.isNone())
        return ModeName;
    // Run the setDisplayMode method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::String(ModeName));
        Py::String str(Base::pyCall(py_setDisplayMode.ptr(),args.ptr()));
        return str.as_std_string("ascii");
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return ModeName;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDragObjects() const
{
    if(py_canDragObjects.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Base::pyCall(py_canDragObjects.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDragObject(App::DocumentObject* obj) const
{
    if(py_canDragObject.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(obj->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_canDragObject.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::dragObject(App::DocumentObject* obj)
{
    if(py_dragObject.isNone())
        return NotImplemented;

    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(obj->getPyObject(), true));
            Base::pyCall(py_dragObject.ptr(),args.ptr());
            return Accepted;
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, Py::Object(obj->getPyObject(), true));
            Base::pyCall(py_dragObject.ptr(),args.ptr());
            return Accepted;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDropObjects() const
{
    if(py_canDropObjects.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Base::pyCall(py_canDropObjects.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDropObject(App::DocumentObject* obj) const
{
    if(py_canDropObject.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(obj->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_canDropObject.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::dropObject(App::DocumentObject* obj)
{
    if(py_dropObject.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(obj->getPyObject(), true));
            Base::pyCall(py_dropObject.ptr(),args.ptr());
            return Accepted;
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, Py::Object(obj->getPyObject(), true));
            Base::pyCall(py_dropObject.ptr(),args.ptr());
            return Accepted;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        throw e;
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDragAndDropObject(App::DocumentObject *obj) const
{
    if(py_canDragAndDropObject.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::TupleN args(Py::Object(obj->getPyObject(),true));
        Py::Boolean ok(Base::pyCall(py_canDragAndDropObject.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDropObjectEx(App::DocumentObject* obj,
        App::DocumentObject *owner, const char *subname, const std::vector<std::string> &elements) const
{
    if(py_canDropObjectEx.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(4);
        args.setItem(0, Py::Object(obj->getPyObject(), true));
        args.setItem(1, owner?Py::Object(owner->getPyObject(), true):Py::None());
        args.setItem(2, Py::String(subname?subname:""));
        Py::Tuple tuple(elements.size());
        int i=0;
        for(auto &element : elements)
            tuple.setItem(i++,Py::String(element));
        args.setItem(3, tuple);
        Py::Boolean ok(Base::pyCall(py_canDropObjectEx.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::dropObjectEx(App::DocumentObject* obj, App::DocumentObject *owner, 
        const char *subname, const std::vector<std::string> &elements,std::string &ret)
{
    if(py_dropObjectEx.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple tuple(elements.size());
        int i=0;
        for(auto &element : elements)
            tuple.setItem(i++,Py::String(element));
        Py::Object res;
        Py::TupleN args(
                Py::Object(object->getPyObject(),true),
                Py::Object(obj->getPyObject(),true),
                owner?Py::Object(owner->getPyObject(),true):Py::Object(),
                Py::String(subname?subname:""),tuple);
        res = Base::pyCall(py_dropObjectEx.ptr(),args.ptr());
        if(!res.isNone())
            ret = res.as_string();
        return Accepted;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        throw e;
    }
}

bool ViewProviderPythonFeatureImp::isShow() const
{
    if(py_isShow.isNone())
        return false;

    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Base::pyCall(py_isShow.ptr()));
        return static_cast<bool>(ok) ? true : false;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return false;
}


ViewProviderPythonFeatureImp::ValueT 
ViewProviderPythonFeatureImp::canRemoveChildrenFromRoot() const {
    if(py_canRemoveChildrenFromRoot.isNone())
        return NotImplemented;

    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Base::pyCall(py_canRemoveChildrenFromRoot.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return Rejected;
}

bool ViewProviderPythonFeatureImp::getDropPrefix(std::string &prefix) const {
    if(py_getDropPrefix.isNone())
        return false;

    Base::PyGILStateLocker lock;
    try {
        Py::Object ret(Base::pyCall(py_getDropPrefix.ptr()));
        if(ret.isNone())
            return false;
        prefix = ret.as_string();
        return true;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return false;
}

// ---------------------------------------------------------

namespace Gui {
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPythonFeature, Gui::ViewProviderDocumentObject)
// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderDocumentObject>;
}

// ---------------------------------------------------------

namespace Gui {
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPythonGeometry, Gui::ViewProviderGeometryObject)
// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderGeometryObject>;
}


