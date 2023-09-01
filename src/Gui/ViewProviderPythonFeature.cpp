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
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoDetail.h>
#endif

#include <App/DocumentObjectPy.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>

#include "ViewProviderPythonFeature.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "PythonWrapper.h"
#include "View3DInventorViewer.h"
#include "ViewProviderDocumentObjectPy.h"


FC_LOG_LEVEL_INIT("ViewProviderPythonFeature", true, true)


using namespace Gui;
namespace sp = std::placeholders;


// ----------------------------------------------------------------------------

ViewProviderPythonFeatureImp::ViewProviderPythonFeatureImp(
        ViewProviderDocumentObject* vp, App::PropertyPythonObject &proxy)
  : object(vp)
  , Proxy(proxy)
{
}

ViewProviderPythonFeatureImp::~ViewProviderPythonFeatureImp()
{
    Base::PyGILStateLocker lock;
#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) py_##_name = Py::None();

    try {
        FC_PY_VIEW_OBJECT
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

void ViewProviderPythonFeatureImp::init(PyObject *pyobj) {
    Base::PyGILStateLocker lock;
    has__object__ = !!PyObject_HasAttrString(pyobj, "__object__");

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) FC_PY_ELEMENT_INIT(_name)

    FC_PY_VIEW_OBJECT
}

#define FC_PY_CALL_CHECK(_name) _FC_PY_CALL_CHECK(_name,return(NotImplemented))

QIcon ViewProviderPythonFeatureImp::getIcon() const
{
    _FC_PY_CALL_CHECK(getIcon,return(QIcon()));

    // Run the getIcon method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Object ret(Base::pyCall(py_getIcon.ptr()));
        if(ret.isNone())
            return {};

        if(ret.isString()) {
            std::string content = Py::String(ret).as_std_string("utf-8");
            QPixmap icon;
            if (BitmapFactory().findPixmapInCache(content.c_str(), icon))
                return icon;

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
                for (const auto & line : lines) {
                    QByteArray trim = line.trimmed();
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
        } else {
            PythonWrapper wrap;
            wrap.loadGuiModule();
            wrap.loadWidgetsModule();
            QIcon *picon = wrap.toQIcon(ret.ptr());
            if(picon)
                return *picon;
        }
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError))
            PyErr_Clear();
        else {
            Base::PyException e; // extract the Python error text
            e.ReportException();
        }
    }

    return {};
}

bool ViewProviderPythonFeatureImp::claimChildren(std::vector<App::DocumentObject*> &children) const
{
    _FC_PY_CALL_CHECK(claimChildren,return(false));

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
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::useNewSelectionModel() const
{
    FC_PY_CALL_CHECK(useNewSelectionModel);

    // Run the useNewSelectionModel method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Py::Callable(py_useNewSelectionModel).apply(Py::Tuple()));
        return ok?Accepted:Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Accepted;
}

bool ViewProviderPythonFeatureImp::getElement(const SoDetail *det, std::string &res) const
{
    _FC_PY_CALL_CHECK(getElement,return(false));

    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        PyObject* pivy = nullptr;
        // Note: As there is no ref'counting mechanism for the SoDetail class we must
        // pass '0' as the last parameter so that the Python object does not 'own'
        // the detail object.
        pivy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoDetail *", const_cast<void*>(static_cast<const void*>(det)), 0);
        Py::Tuple args(1);
        args.setItem(0, Py::Object(pivy, true));
        Py::String name(Base::pyCall(py_getElement.ptr(),args.ptr()));
        res = name;
        return true;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::getElementPicked(const SoPickedPoint *pp, std::string &subname) const
{
    FC_PY_CALL_CHECK(getElementPicked);

    Base::PyGILStateLocker lock;
    try {
        PyObject* pivy = nullptr;
        pivy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoPickedPoint *", const_cast<void*>(static_cast<const void*>(pp)), 0);
        Py::Tuple args(1);
        args.setItem(0, Py::Object(pivy, true));
        Py::Object ret(Base::pyCall(py_getElementPicked.ptr(),args.ptr()));
        if(!ret.isString())
            return Rejected;
        subname = ret.as_string();
        return Accepted;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

bool ViewProviderPythonFeatureImp::getDetail(const char* name, SoDetail *&det) const
{
    _FC_PY_CALL_CHECK(getDetail,return(false));

    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::String(name));
        Py::Object pydet(Base::pyCall(py_getDetail.ptr(),args.ptr()));
        void* ptr = nullptr;
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoDetail *", pydet.ptr(), &ptr, 0);
        auto detail = static_cast<SoDetail*>(ptr);
        det = detail ? detail->copy() : nullptr;
        return true;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

ViewProviderPythonFeatureImp::ValueT ViewProviderPythonFeatureImp::getDetailPath(
        const char* name, SoFullPath *path, bool append, SoDetail *&det) const
{
    FC_PY_CALL_CHECK(getDetailPath);

    Base::PyGILStateLocker lock;
    auto length = path->getLength();
    try {
        PyObject* pivy = nullptr;
        pivy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoFullPath *", static_cast<void*>(path), 1);
        path->ref();
        Py::Tuple args(3);
        args.setItem(0, Py::String(name));
        args.setItem(1, Py::Object(pivy, true));
        args.setItem(2, Py::Boolean(append));
        Py::Object pyDet(Base::pyCall(py_getDetailPath.ptr(),args.ptr()));
        if(!pyDet.isTrue())
            return Rejected;
        if(pyDet.isBoolean())
            return Accepted;
        void* ptr = nullptr;
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoDetail *", pyDet.ptr(), &ptr, 0);
        auto detail = static_cast<SoDetail*>(ptr);
        det = detail ? detail->copy() : nullptr;
        if(det)
            return Accepted;
        delete det;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    path->truncate(length);
    return Rejected;
}


std::vector<Base::Vector3d> ViewProviderPythonFeatureImp::getSelectionShape(const char* /*Element*/) const
{
    return {};
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::setEdit(int ModNum)
{
    FC_PY_CALL_CHECK(setEdit)

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
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::unsetEdit(int ModNum)
{
    FC_PY_CALL_CHECK(unsetEdit)

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
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::setEditViewer(View3DInventorViewer *viewer, int ModNum)
{
    FC_PY_CALL_CHECK(setEditViewer)

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(3);
        args.setItem(0, Py::Object(object->getPyObject(),true));
        args.setItem(1, Py::Object(viewer->getPyObject(),true));
        args.setItem(2, Py::Int(ModNum));
        Py::Object ret(Base::pyCall(py_setEditViewer.ptr(),args.ptr()));
        return ret.isTrue()?Accepted:Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::unsetEditViewer(View3DInventorViewer *viewer)
{
    FC_PY_CALL_CHECK(unsetEditViewer)

    // Run the onChanged method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::Object(object->getPyObject(),true));
        args.setItem(1, Py::Object(viewer->getPyObject(),true));
        Py::Object ret(Base::pyCall(py_unsetEditViewer.ptr(),args.ptr()));
        return ret.isTrue()?Accepted:Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::doubleClicked()
{
    FC_PY_CALL_CHECK(doubleClicked)

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
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

bool ViewProviderPythonFeatureImp::setupContextMenu(QMenu* menu)
{
    _FC_PY_CALL_CHECK(setupContextMenu,return(false));

    // Run the attach method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            PythonWrapper wrap;
            wrap.loadGuiModule();
            wrap.loadWidgetsModule();
            Py::Tuple args(1);
            args.setItem(0, wrap.fromQWidget(menu, "QMenu"));
            return Base::pyCall(py_setupContextMenu.ptr(),args.ptr()).isTrue();
        }
        else {
            PythonWrapper wrap;
            wrap.loadGuiModule();
            wrap.loadWidgetsModule();
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, wrap.fromQWidget(menu, "QMenu"));
            return Base::pyCall(py_setupContextMenu.ptr(),args.ptr()).isTrue();
        }
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return true;
}

void ViewProviderPythonFeatureImp::attach(App::DocumentObject *pcObject)
{
    _FC_PY_CALL_CHECK(attach,return);

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
        Py::Object vp = Proxy.getValue();
        if (vp.isNone()) {
            object->show();
            Proxy.setValue(Py::Int(1));
        } else {
            _FC_PY_CALL_CHECK(finishRestoring,return);
            Base::pyCall(py_finishRestoring.ptr());
        }
    }catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::onDelete(const std::vector<std::string> & sub)
{
    FC_PY_CALL_CHECK(onDelete);

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple seq(sub.size());
        int index=0;
        for (const auto & it : sub) {
            seq.setItem(index++, Py::String(it));
        }

        if (has__object__) {
            Py::Tuple args(1);
            args.setItem(0, seq);
            Py::Boolean ok(Base::pyCall(py_onDelete.ptr(),args.ptr()));
            return ok?Accepted:Rejected;
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, seq);
            Py::Boolean ok(Base::pyCall(py_onDelete.ptr(),args.ptr()));
            return ok?Accepted:Rejected;
        }
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return Rejected;
    }
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDelete(App::DocumentObject *obj) const
{
    FC_PY_CALL_CHECK(canDelete);

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0,obj?Py::Object(obj->getPyObject(),true):Py::Object());
        return Py::Boolean(Base::pyCall(py_canDelete.ptr(),args.ptr()))?Accepted:Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return Rejected;
    }
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canAddToSceneGraph() const
{
    FC_PY_CALL_CHECK(canAddToSceneGraph);

    Base::PyGILStateLocker lock;
    try {
        return Py::Boolean(Py::Callable(py_canAddToSceneGraph).apply(Py::Tuple()))?Accepted:Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return Accepted;
}

bool ViewProviderPythonFeatureImp::getDefaultDisplayMode(std::string &mode) const
{
    _FC_PY_CALL_CHECK(getDefaultDisplayMode,return(0));

    // Run the getDefaultDisplayMode method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::String str(Base::pyCall(py_getDefaultDisplayMode.ptr()));
        mode = str.as_std_string("ascii");
        return true;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return true;
}

std::vector<std::string> ViewProviderPythonFeatureImp::getDisplayModes() const
{
    std::vector<std::string> modes;
    _FC_PY_CALL_CHECK(getDisplayModes,return(modes));

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
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError))
            PyErr_Clear();
        else {
            Base::PyException e; // extract the Python error text
            e.ReportException();
        }
    }

    return modes;
}

std::string ViewProviderPythonFeatureImp::setDisplayMode(const char* ModeName)
{
    _FC_PY_CALL_CHECK(setDisplayMode,return(ModeName));

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
    FC_PY_CALL_CHECK(canDragObjects);

    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Base::pyCall(py_canDragObjects.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDragObject(App::DocumentObject* obj) const
{
    FC_PY_CALL_CHECK(canDragObject);

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(obj->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_canDragObject.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::dragObject(App::DocumentObject* obj)
{
    FC_PY_CALL_CHECK(dragObject);

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
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDropObjects() const
{
    FC_PY_CALL_CHECK(canDropObjects);

    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Base::pyCall(py_canDropObjects.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDropObject(App::DocumentObject* obj) const
{
    FC_PY_CALL_CHECK(canDropObject);

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(obj->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_canDropObject.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::dropObject(App::DocumentObject* obj)
{
    FC_PY_CALL_CHECK(dropObject);

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
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException::ThrowException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDragAndDropObject(App::DocumentObject *obj) const
{
    FC_PY_CALL_CHECK(canDragAndDropObject);

    Base::PyGILStateLocker lock;
    try {
        Py::TupleN args(Py::Object(obj->getPyObject(),true));
        Py::Boolean ok(Base::pyCall(py_canDragAndDropObject.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canDropObjectEx(App::DocumentObject* obj,
        App::DocumentObject *owner, const char *subname, const std::vector<std::string> &elements) const
{
    FC_PY_CALL_CHECK(canDropObjectEx);

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
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}

bool ViewProviderPythonFeatureImp::dropObjectEx(App::DocumentObject* obj, App::DocumentObject *owner,
        const char *subname, const std::vector<std::string> &elements,std::string &ret)
{
    _FC_PY_CALL_CHECK(dropObjectEx, return(false));

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
        return true;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException::ThrowException();
    }
    return true;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::isShow() const
{
    FC_PY_CALL_CHECK(isShow);

    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Base::pyCall(py_isShow.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return Rejected;
}


ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::canRemoveChildrenFromRoot() const {

    FC_PY_CALL_CHECK(canRemoveChildrenFromRoot);

    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok(Base::pyCall(py_canRemoveChildrenFromRoot.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return Rejected;
}

bool ViewProviderPythonFeatureImp::getDropPrefix(std::string &prefix) const {

    _FC_PY_CALL_CHECK(getDropPrefix,return(false));

    Base::PyGILStateLocker lock;
    try {
        Py::Object ret(Base::pyCall(py_getDropPrefix.ptr()));
        if(ret.isNone())
            return false;
        prefix = ret.as_string();
        return true;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return true;
}

ViewProviderPythonFeatureImp::ValueT
ViewProviderPythonFeatureImp::replaceObject(
        App::DocumentObject *oldObj, App::DocumentObject *newObj)
{
    if(!oldObj || !oldObj->getNameInDocument()
            || !newObj || !newObj->getNameInDocument())
        return NotImplemented;

    FC_PY_CALL_CHECK(replaceObject);

    Base::PyGILStateLocker lock;
    try {
        Py::TupleN args(Py::asObject(oldObj->getPyObject()),
                Py::asObject(newObj->getPyObject()));
        Py::Boolean ok(Base::pyCall(py_replaceObject.ptr(),args.ptr()));
        return ok ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return Rejected;
}

bool ViewProviderPythonFeatureImp::getLinkedViewProvider(
        ViewProviderDocumentObject *&vp, std::string *subname, bool recursive) const
{
    _FC_PY_CALL_CHECK(getLinkedViewProvider,return(false));

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0,Py::Boolean(recursive));
        Py::Object res(Base::pyCall(py_getLinkedViewProvider.ptr(),args.ptr()));
        if(res.isNone())
            return true;
        if(PyObject_TypeCheck(res.ptr(),&ViewProviderDocumentObjectPy::Type)) {
            vp = static_cast<ViewProviderDocumentObjectPy*>(
                    res.ptr())->getViewProviderDocumentObjectPtr();
            return true;
        } else if (PySequence_Check(res.ptr()) && PySequence_Length(res.ptr())==2) {
            Py::Sequence seq(res);
            Py::Object item0(seq[0].ptr());
            Py::Object item1(seq[1].ptr());
            if(PyObject_TypeCheck(item0.ptr(), &ViewProviderDocumentObjectPy::Type) && item1.isString()) {
                if(subname)
                    *subname = Py::String(item1).as_std_string("utf-8");
                vp = static_cast<ViewProviderDocumentObjectPy*>(
                        item0.ptr())->getViewProviderDocumentObjectPtr();
                return true;
            }
        }
        FC_ERR("getLinkedViewProvider(): invalid return type, expects ViewObject or (ViewObject, subname)");
        return true;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return true;
}

bool ViewProviderPythonFeatureImp::editProperty(const char *name)
{
    _FC_PY_CALL_CHECK(editProperty,return false);
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::String(name));
        Py::Object ret(Base::pyCall(py_editProperty.ptr(),args.ptr()));
        return ret.isTrue();
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }

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


