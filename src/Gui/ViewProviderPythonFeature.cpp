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
#include <App/DocumentObjectPy.h>
#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Interpreter.h>


using namespace Gui;

// #0003564: Python objects: updateData calls to proxy instance that should have been deleted
// See https://forum.freecadweb.org/viewtopic.php?f=22&t=30429&p=252429#p252429
#if 0
namespace Gui {

class PropertyEvent : public QEvent
{
public:
    PropertyEvent(const Gui::ViewProvider* vp, App::Property* p)
        : QEvent(QEvent::Type(QEvent::User)), view(vp), prop(p)
    {
    }

    const Gui::ViewProvider* view;
    App::Property* prop;
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
            App::Property* prop = pe->view->getPropertyByName("Proxy");
            if (prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
                prop->Paste(*pe->prop);
            }
        }
        delete pe->prop;
    }
    static ViewProviderPythonFeatureObserver* _singleton;

    ViewProviderPythonFeatureObserver();
    ~ViewProviderPythonFeatureObserver();
    typedef std::map<
                const App::DocumentObject*,
                App::Property*
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
            proxyMap[doc][docobj] = prop->Copy();
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
  : object(vp)
{
#if 0
    (void)ViewProviderPythonFeatureObserver::instance();
#endif
}

ViewProviderPythonFeatureImp::~ViewProviderPythonFeatureImp()
{
}

QIcon ViewProviderPythonFeatureImp::getIcon() const
{
    // default icon
    //static QPixmap px = BitmapFactory().pixmap("Tree_Python");

    // Run the getIcon method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("getIcon"))) {
                Py::Callable method(vp.getAttr(std::string("getIcon")));
                Py::Tuple args;
                Py::String str(method.apply(args));
                std::string content = str.as_std_string("utf-8");
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
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return QIcon();
}

std::vector<App::DocumentObject*> ViewProviderPythonFeatureImp::claimChildren(const std::vector<App::DocumentObject*>& base) const 
{
    std::vector<App::DocumentObject*> children;
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("claimChildren"))) {
                Py::Callable method(vp.getAttr(std::string("claimChildren")));
                Py::Tuple args;
                Py::Sequence list(method.apply(args));
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    PyObject* item = (*it).ptr();
                    if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                        children.push_back(obj);
                    }
                }
            }
            else {
                children = base;
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
    // Run the useNewSelectionModel method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("useNewSelectionModel"))) {
                Py::Callable method(vp.getAttr(std::string("useNewSelectionModel")));
                Py::Tuple args;
                Py::Boolean ok(method.apply(args));
                return (bool)ok;
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

std::string ViewProviderPythonFeatureImp::getElement(const SoDetail *det) const
{
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("getElement"))) {
                PyObject* pivy = 0;
                // Note: As there is no ref'counting mechanism for the SoDetail class we must
                // pass '0' as the last parameter so that the Python object does not 'own'
                // the detail object.
                pivy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoDetail *", (void*)det, 0);
                Py::Callable method(vp.getAttr(std::string("getElement")));
                Py::Tuple args(1);
                args.setItem(0, Py::Object(pivy, true));
                Py::String name(method.apply(args));
                return (std::string)name;
            }
        }
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return "";
}

SoDetail* ViewProviderPythonFeatureImp::getDetail(const char* name) const
{
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("getDetail"))) {
                Py::Callable method(vp.getAttr(std::string("getDetail")));
                Py::Tuple args(1);
                args.setItem(0, Py::String(name));
                Py::Object det(method.apply(args));
                void* ptr = 0;
                Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoDetail *", det.ptr(), &ptr, 0);
                SoDetail* detail = reinterpret_cast<SoDetail*>(ptr);
                return detail ? detail->copy() : 0;
            }
        }
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

std::vector<Base::Vector3d> ViewProviderPythonFeatureImp::getSelectionShape(const char* /*Element*/) const
{
    return std::vector<Base::Vector3d>();
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::setEdit(int ModNum)
{
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("setEdit"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("setEdit")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Int(ModNum));
                    Py::Object ret(method.apply(args));
                    if (ret.isNone())
                        return NotImplemented;
                    Py::Boolean ok(ret);
                    bool value = static_cast<bool>(ok);
                    return value ? Accepted : Rejected;
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("setEdit")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    args.setItem(1, Py::Int(ModNum));
                    Py::Object ret(method.apply(args));
                    if (ret.isNone())
                        return NotImplemented;
                    Py::Boolean ok(ret);
                    bool value = static_cast<bool>(ok);
                    return value ? Accepted : Rejected;
                }
            }
        }
    }
    catch (Py::Exception&) {
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
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("unsetEdit"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("unsetEdit")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Int(ModNum));
                    Py::Object ret(method.apply(args));
                    if (ret.isNone())
                        return NotImplemented;
                    Py::Boolean ok(ret);
                    bool value = static_cast<bool>(ok);
                    return value ? Accepted : Rejected;
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("unsetEdit")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    args.setItem(1, Py::Int(ModNum));
                    Py::Object ret(method.apply(args));
                    if (ret.isNone())
                        return NotImplemented;
                    Py::Boolean ok(ret);
                    bool value = static_cast<bool>(ok);
                    return value ? Accepted : Rejected;
                }
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return NotImplemented;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::doubleClicked(void)
{
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("doubleClicked"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("doubleClicked")));
                    Py::Tuple args;
                    //args.setItem(0, Py::Int(ModNum));
                    Py::Boolean ok(method.apply(args));
                    bool value = (bool)ok;
                    return value ? Accepted : Rejected;
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("doubleClicked")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    Py::Boolean ok(method.apply(args));
                    bool value = (bool)ok;
                    return value ? Accepted : Rejected;
                }
            }
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
    // Run the attach method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("setupContextMenu"))) {
                if (vp.hasAttr("__object__")) {
                    PythonWrapper wrap;
                    wrap.loadGuiModule();
                    wrap.loadWidgetsModule();
                    Py::Callable method(vp.getAttr(std::string("setupContextMenu")));
                    Py::Tuple args(1);
                    args.setItem(0, wrap.fromQWidget(menu, "QMenu"));
                    method.apply(args);
                }
                else {
                    PythonWrapper wrap;
                    wrap.loadGuiModule();
                    wrap.loadWidgetsModule();
                    Py::Callable method(vp.getAttr(std::string("setupContextMenu")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    args.setItem(1, wrap.fromQWidget(menu, "QMenu"));
                    method.apply(args);
                }
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void ViewProviderPythonFeatureImp::attach(App::DocumentObject *pcObject)
{
    // Run the attach method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("attach"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("attach")));
                    Py::Tuple args;
                    method.apply(args);
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("attach")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    method.apply(args);
                }

                // #0000415: Now simulate a property change event to call
                // claimChildren if implemented.
                pcObject->Label.touch();
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void ViewProviderPythonFeatureImp::updateData(const App::Property* prop)
{
    // Run the updateData method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("updateData"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("updateData")));
                    Py::Tuple args(1);
                    const char* prop_name = object->getObject()->getPropertyName(prop);
                    if (prop_name) {
                        args.setItem(0, Py::String(prop_name));
                        method.apply(args);
                    }
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("updateData")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getObject()->getPyObject(), true));
                    const char* prop_name = object->getObject()->getPropertyName(prop);
                    if (prop_name) {
                        args.setItem(1, Py::String(prop_name));
                        method.apply(args);
                    }
                }
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
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("onChanged"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("onChanged")));
                    Py::Tuple args(1);
                    const char* prop_name = object->getPropertyName(prop);
                    if (prop_name) {
                        args.setItem(0, Py::String(prop_name));
                        method.apply(args);
                    }
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("onChanged")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    const char* prop_name = object->getPropertyName(prop);
                    if (prop_name) {
                        args.setItem(1, Py::String(prop_name));
                        method.apply(args);
                    }
                }
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
    App::Property* proxy = object->getPropertyByName("Proxy");
    if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
        Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
        if (vp.ptr() == Py::_None()) {
            object->show();
            static_cast<App::PropertyPythonObject*>(proxy)->setValue(Py::Int(1));
        }
    }
}

bool ViewProviderPythonFeatureImp::onDelete(const std::vector<std::string> & sub)
{
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("onDelete"))) {
                Py::Tuple seq(sub.size());
                int index=0;
                for (std::vector<std::string>::const_iterator it = sub.begin(); it != sub.end(); ++it) {
                    seq.setItem(index++, Py::String(*it));
                }

                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("onDelete")));
                    Py::Tuple args(1);
                    args.setItem(0, seq);
                    Py::Boolean ok(method.apply(args));
                    return (bool)ok;
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("onDelete")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    args.setItem(1, seq);
                    Py::Boolean ok(method.apply(args));
                    return (bool)ok;
                }
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

const char* ViewProviderPythonFeatureImp::getDefaultDisplayMode() const
{
    // Run the getDefaultDisplayMode method of the proxy object.
    Base::PyGILStateLocker lock;
    static std::string mode;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("getDefaultDisplayMode"))) {
                Py::Callable method(vp.getAttr(std::string("getDefaultDisplayMode")));
                Py::Tuple args;
                Py::String str(method.apply(args));
                //if (str.isUnicode())
                //    str = str.encode("ascii"); // json converts strings into unicode
                mode = str.as_std_string("ascii");
                return mode.c_str();
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return 0;
}

std::vector<std::string> ViewProviderPythonFeatureImp::getDisplayModes(void) const
{
    // Run the getDisplayModes method of the proxy object.
    Base::PyGILStateLocker lock;
    std::vector<std::string> modes;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("getDisplayModes"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("getDisplayModes")));
                    Py::Tuple args;
                    Py::Sequence list(method.apply(args));
                    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                        Py::String str(*it);
                        modes.push_back(str.as_std_string("ascii"));
                    }
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("getDisplayModes")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    Py::Sequence list(method.apply(args));
                    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                        Py::String str(*it);
                        modes.push_back(str.as_std_string("ascii"));
                    }
                }
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
    // Run the setDisplayMode method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("setDisplayMode"))) {
                Py::Callable method(vp.getAttr(std::string("setDisplayMode")));
                Py::Tuple args(1);
                args.setItem(0, Py::String(ModeName));
                Py::String str(method.apply(args));
                return str.as_std_string("ascii");
            }
        }
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
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("canDragObjects"))) {
                Py::Callable method(vp.getAttr(std::string("canDragObjects")));
                Py::Tuple args;
                Py::Boolean ok(method.apply(args));
                return static_cast<bool>(ok) ? Accepted : Rejected;
            }
            else {
                return NotImplemented;
            }
        }
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
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("canDragObject"))) {
                Py::Callable method(vp.getAttr(std::string("canDragObject")));
                Py::Tuple args(1);
                args.setItem(0, Py::Object(obj->getPyObject(), true));
                Py::Boolean ok(method.apply(args));
                return static_cast<bool>(ok) ? Accepted : Rejected;
            }
            else {
                return NotImplemented;
            }
        }
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
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("dragObject"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("dragObject")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Object(obj->getPyObject(), true));
                    method.apply(args);
                    return Accepted;
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("dragObject")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    args.setItem(1, Py::Object(obj->getPyObject(), true));
                    method.apply(args);
                    return Accepted;
                }
            }
            else {
                return NotImplemented;
            }
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
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("canDropObjects"))) {
                Py::Callable method(vp.getAttr(std::string("canDropObjects")));
                Py::Tuple args;
                Py::Boolean ok(method.apply(args));
                return static_cast<bool>(ok) ? Accepted : Rejected;
            }
            else {
                return NotImplemented;
            }
        }
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
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("canDropObject"))) {
                Py::Callable method(vp.getAttr(std::string("canDropObject")));
                Py::Tuple args(1);
                args.setItem(0, Py::Object(obj->getPyObject(), true));
                Py::Boolean ok(method.apply(args));
                return static_cast<bool>(ok) ? Accepted : Rejected;
            }
            else {
                return NotImplemented;
            }
        }
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
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("dropObject"))) {
                if (vp.hasAttr("__object__")) {
                    Py::Callable method(vp.getAttr(std::string("dropObject")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Object(obj->getPyObject(), true));
                    method.apply(args);
                    return Accepted;
                }
                else {
                    Py::Callable method(vp.getAttr(std::string("dropObject")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    args.setItem(1, Py::Object(obj->getPyObject(), true));
                    method.apply(args);
                    return Accepted;
                }
            }
            else {
                return NotImplemented;
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

bool ViewProviderPythonFeatureImp::isShow() const
{
    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("isShow"))) {
                Py::Callable method(vp.getAttr(std::string("isShow")));
                Py::Tuple args;
                Py::Boolean ok(method.apply(args));
                return static_cast<bool>(ok) ? true : false;
            }
        }
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


