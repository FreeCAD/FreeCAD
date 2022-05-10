/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <limits>
# include <QDir>
# include <QIcon>
# include <QWidget>
#endif

#include <QMetaType>

// Uncomment this block to remove PySide C++ support and switch to its Python interface
//#undef HAVE_SHIBOKEN
//#undef HAVE_PYSIDE
//#undef HAVE_SHIBOKEN2
//#undef HAVE_PYSIDE2

#ifdef FC_OS_WIN32
#undef max
#undef min
#ifdef _MSC_VER
#pragma warning( disable : 4099 )
#pragma warning( disable : 4522 )
#endif
#endif

// class and struct used for SbkObject
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmismatched-tags"
# pragma clang diagnostic ignored "-Wunused-parameter"
# if __clang_major__ > 3
# pragma clang diagnostic ignored "-Wkeyword-macro"
# endif
#elif defined (__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef HAVE_SHIBOKEN
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
# include <basewrapper.h>
# include <sbkconverter.h>
# include <sbkmodule.h>
# include <typeresolver.h>
# include <shiboken.h>
# ifdef HAVE_PYSIDE
# include <pyside_qtcore_python.h>
# include <pyside_qtgui_python.h>
PyTypeObject** SbkPySide_QtCoreTypes=nullptr;
PyTypeObject** SbkPySide_QtGuiTypes=nullptr;
# endif
#endif

#ifdef HAVE_SHIBOKEN2
# define HAVE_SHIBOKEN
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
# include <basewrapper.h>
# include <sbkconverter.h>
# include <sbkmodule.h>
# include <shiboken.h>
# ifdef HAVE_PYSIDE2
# define HAVE_PYSIDE

// Since version 5.12 shiboken offers a method to get wrapper by class name (typeForTypeName)
// This helps to avoid to include the PySide2 headers since MSVC has a compiler bug when
// compiling together with std::bitset (https://bugreports.qt.io/browse/QTBUG-72073)

// Do not use SHIBOKEN_MICRO_VERSION; it might contain a dot
# define SHIBOKEN_FULL_VERSION QT_VERSION_CHECK(SHIBOKEN_MAJOR_VERSION, SHIBOKEN_MINOR_VERSION, 0)
# if (SHIBOKEN_FULL_VERSION >= QT_VERSION_CHECK(5, 12, 0))
# define HAVE_SHIBOKEN_TYPE_FOR_TYPENAME
# endif

# ifndef HAVE_SHIBOKEN_TYPE_FOR_TYPENAME
# include <pyside2_qtcore_python.h>
# include <pyside2_qtgui_python.h>
# include <pyside2_qtwidgets_python.h>
# endif
# include <signalmanager.h>
PyTypeObject** SbkPySide2_QtCoreTypes=nullptr;
PyTypeObject** SbkPySide2_QtGuiTypes=nullptr;
PyTypeObject** SbkPySide2_QtWidgetsTypes=nullptr;
# endif // HAVE_PYSIDE2
#endif // HAVE_SHIBOKEN2

#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined (__GNUC__)
# pragma GCC diagnostic pop
#endif

// Must be imported after PySide headers
#ifndef _PreComp_
# include <QGraphicsItem>
# include <QGraphicsObject>
#endif

#include <App/Application.h>
#include <Base/Quantity.h>
#include <Base/QuantityPy.h>

#include "PythonWrapper.h"
#include "UiLoader.h"
#include "MetaTypes.h"


using namespace Gui;

#if defined (HAVE_SHIBOKEN)

/**
  Example:
  \code
    ui = FreeCADGui.UiLoader()
    w = ui.createWidget("Gui::InputField")
    w.show()
    w.property("quantity")
  \endcode
  */

PyObject* toPythonFuncQuantityTyped(Base::Quantity cpx) {
    return new Base::QuantityPy(new Base::Quantity(cpx));
}

PyObject* toPythonFuncQuantity(const void* cpp)
{
    return toPythonFuncQuantityTyped(*reinterpret_cast<const Base::Quantity*>(cpp));
}

void toCppPointerConvFuncQuantity(PyObject* pyobj,void* cpp)
{
   *((Base::Quantity*)cpp) = *static_cast<Base::QuantityPy*>(pyobj)->getQuantityPtr();
}

PythonToCppFunc toCppPointerCheckFuncQuantity(PyObject* obj)
{
    if (PyObject_TypeCheck(obj, &(Base::QuantityPy::Type)))
        return toCppPointerConvFuncQuantity;
    else
        return nullptr;
}

void BaseQuantity_PythonToCpp_QVariant(PyObject* pyIn, void* cppOut)
{
    Base::Quantity* q = static_cast<Base::QuantityPy*>(pyIn)->getQuantityPtr();
    *((QVariant*)cppOut) = QVariant::fromValue<Base::Quantity>(*q);
}

PythonToCppFunc isBaseQuantity_PythonToCpp_QVariantConvertible(PyObject* obj)
{
    if (PyObject_TypeCheck(obj, &(Base::QuantityPy::Type)))
        return BaseQuantity_PythonToCpp_QVariant;
    return nullptr;
}

#if defined (HAVE_PYSIDE)
Base::Quantity convertWrapperToQuantity(const PySide::PyObjectWrapper &w)
{
    PyObject* pyIn = static_cast<PyObject*>(w);
    if (PyObject_TypeCheck(pyIn, &(Base::QuantityPy::Type))) {
        return *static_cast<Base::QuantityPy*>(pyIn)->getQuantityPtr();
    }

    return Base::Quantity(std::numeric_limits<double>::quiet_NaN());
}
#endif

void registerTypes()
{
    SbkConverter* convert = Shiboken::Conversions::createConverter(&Base::QuantityPy::Type,
                                                                   toPythonFuncQuantity);
    Shiboken::Conversions::setPythonToCppPointerFunctions(convert,
                                                          toCppPointerConvFuncQuantity,
                                                          toCppPointerCheckFuncQuantity);
    Shiboken::Conversions::registerConverterName(convert, "Base::Quantity");

    SbkConverter* qvariant_conv = Shiboken::Conversions::getConverter("QVariant");
    if (qvariant_conv) {
        // The type QVariant already has a converter from PyBaseObject_Type which will
        // come before our own converter.
        Shiboken::Conversions::addPythonToCppValueConversion(qvariant_conv,
                                                             BaseQuantity_PythonToCpp_QVariant,
                                                             isBaseQuantity_PythonToCpp_QVariantConvertible);
    }

#if defined (HAVE_PYSIDE)
    QMetaType::registerConverter<PySide::PyObjectWrapper, Base::Quantity>(&convertWrapperToQuantity);
#endif
}
#endif

// --------------------------------------------------------

namespace Gui {
template<typename qttype>
Py::Object qt_wrapInstance(qttype object, const char* className,
                           const char* shiboken, const char* pyside,
                           const char* wrap)
{
    PyObject* module = PyImport_ImportModule(shiboken);
    if (!module) {
        std::string error = "Cannot load ";
        error += shiboken;
        error += " module";
        throw Py::Exception(PyExc_ImportError, error);
    }

    Py::Module mainmod(module, true);
    Py::Callable func = mainmod.getDict().getItem(wrap);

    Py::Tuple arguments(2);
    arguments[0] = Py::asObject(PyLong_FromVoidPtr((void*)object));

    module = PyImport_ImportModule(pyside);
    if (!module) {
        std::string error = "Cannot load ";
        error += pyside;
        error += " module";
        throw Py::Exception(PyExc_ImportError, error);
    }

    Py::Module qtmod(module);
    arguments[1] = qtmod.getDict().getItem(className);
    return func.apply(arguments);
}

const char* qt_identifyType(QObject* ptr, const char* pyside)
{
    PyObject* module = PyImport_ImportModule(pyside);
    if (!module) {
        std::string error = "Cannot load ";
        error += pyside;
        error += " module";
        throw Py::Exception(PyExc_ImportError, error);
    }

    Py::Module qtmod(module);
    const QMetaObject* metaObject = ptr->metaObject();
    while (metaObject) {
        const char* className = metaObject->className();
        if (qtmod.getDict().hasKey(className))
            return className;
        metaObject = metaObject->superClass();
    }

    return nullptr;
}

void* qt_getCppPointer(const Py::Object& pyobject, const char* shiboken, const char* unwrap)
{
    // https://github.com/PySide/Shiboken/blob/master/shibokenmodule/typesystem_shiboken.xml
    PyObject* module = PyImport_ImportModule(shiboken);
    if (!module) {
        std::string error = "Cannot load ";
        error += shiboken;
        error += " module";
        throw Py::Exception(PyExc_ImportError, error);
    }

    Py::Module mainmod(module, true);
    Py::Callable func = mainmod.getDict().getItem(unwrap);

    Py::Tuple arguments(1);
    arguments[0] = pyobject; //PySide pointer
    Py::Tuple result(func.apply(arguments));
    void* ptr = PyLong_AsVoidPtr(result[0].ptr());
    return ptr;
}


template<typename qttype>
PyTypeObject *getPyTypeObjectForTypeName()
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
#if defined (HAVE_SHIBOKEN_TYPE_FOR_TYPENAME)
    SbkObjectType* sbkType = Shiboken::ObjectType::typeForTypeName(typeid(qttype).name());
    if (sbkType)
        return &(sbkType->type);
#else
    return Shiboken::SbkType<qttype>();
#endif
#endif
    return nullptr;
}
}

// --------------------------------------------------------

PythonWrapper::PythonWrapper()
{
#if defined (HAVE_SHIBOKEN)
    static bool init = false;
    if (!init) {
        init = true;
        registerTypes();
    }
#endif
}

bool PythonWrapper::toCString(const Py::Object& pyobject, std::string& str)
{
    if (PyUnicode_Check(pyobject.ptr())) {
        PyObject* unicode = PyUnicode_AsUTF8String(pyobject.ptr());
        str = PyBytes_AsString(unicode);
        Py_DECREF(unicode);
        return true;
    }
    else if (PyBytes_Check(pyobject.ptr())) {
        str = PyBytes_AsString(pyobject.ptr());
        return true;
    }
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    if (Shiboken::String::check(pyobject.ptr())) {
        const char* s = Shiboken::String::toCString(pyobject.ptr());
        if (s) str = s;
        return true;
    }
#endif
    return false;
}

QObject* PythonWrapper::toQObject(const Py::Object& pyobject)
{
    // http://pastebin.com/JByDAF5Z
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject * type = getPyTypeObjectForTypeName<QObject>();
    if (type) {
        if (Shiboken::Object::checkType(pyobject.ptr())) {
            SbkObject* sbkobject = reinterpret_cast<SbkObject *>(pyobject.ptr());
            void* cppobject = Shiboken::Object::cppPointer(sbkobject, type);
            return reinterpret_cast<QObject*>(cppobject);
        }
    }
#else
    // Access shiboken2/PySide2 via Python
    //
    void* ptr = qt_getCppPointer(pyobject, "shiboken2", "getCppPointer");
    return reinterpret_cast<QObject*>(ptr);
#endif

#if 0 // Unwrapping using sip/PyQt
    void* ptr = qt_getCppPointer(pyobject, "sip", "unwrapinstance");
    return reinterpret_cast<QObject*>(ptr);
#endif

    return nullptr;
}

QGraphicsItem* PythonWrapper::toQGraphicsItem(PyObject* pyPtr)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject* type = getPyTypeObjectForTypeName<QGraphicsItem>();
    if (type) {
        if (Shiboken::Object::checkType(pyPtr)) {
            SbkObject* sbkobject = reinterpret_cast<SbkObject*>(pyPtr);
            void* cppobject = Shiboken::Object::cppPointer(sbkobject, type);
            return reinterpret_cast<QGraphicsItem*>(cppobject);
        }
    }
#else
    // Access shiboken2/PySide2 via Python
    //
    void* ptr = qt_getCppPointer(Py::asObject(pyPtr), "shiboken2", "getCppPointer");
    return reinterpret_cast<QGraphicsItem*>(ptr);
#endif
    return nullptr;
}

QGraphicsObject* PythonWrapper::toQGraphicsObject(PyObject* pyPtr)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject* type = getPyTypeObjectForTypeName<QGraphicsObject>();
    if (type) {
        if (Shiboken::Object::checkType(pyPtr)) {
            SbkObject* sbkobject = reinterpret_cast<SbkObject*>(pyPtr);
            void* cppobject = Shiboken::Object::cppPointer(sbkobject, type);
            return reinterpret_cast<QGraphicsObject*>(cppobject);
        }
    }
#else
    // Access shiboken2/PySide2 via Python
    //
    void* ptr = qt_getCppPointer(Py::asObject(pyPtr), "shiboken2", "getCppPointer");
    return reinterpret_cast<QGraphicsObject*>(ptr);
#endif
    return nullptr;
}

Py::Object PythonWrapper::fromQIcon(const QIcon* icon)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    const char* typeName = typeid(*const_cast<QIcon*>(icon)).name();
    PyObject* pyobj = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QIcon>()),
                              const_cast<QIcon*>(icon), true, false, typeName);
    if (pyobj)
        return Py::asObject(pyobj);
#else
    // Access shiboken2/PySide2 via Python
    //
    return qt_wrapInstance<const QIcon*>(icon, "QIcon", "shiboken2", "PySide2.QtGui", "wrapInstance");
#endif
    throw Py::RuntimeError("Failed to wrap icon");
}

QIcon *PythonWrapper::toQIcon(PyObject *pyobj)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject * type = getPyTypeObjectForTypeName<QIcon>();
    if(type) {
        if (Shiboken::Object::checkType(pyobj)) {
            SbkObject* sbkobject = reinterpret_cast<SbkObject *>(pyobj);
            void* cppobject = Shiboken::Object::cppPointer(sbkobject, type);
            return reinterpret_cast<QIcon*>(cppobject);
        }
    }
#else
    Q_UNUSED(pyobj);
#endif
    return nullptr;
}

Py::Object PythonWrapper::fromQDir(const QDir& dir)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    const char* typeName = typeid(dir).name();
    PyObject* pyobj = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QDir>()),
        const_cast<QDir*>(&dir), false, false, typeName);
    if (pyobj)
        return Py::asObject(pyobj);
#else
    Q_UNUSED(dir)
#endif
    throw Py::RuntimeError("Failed to wrap icon");
}

QDir* PythonWrapper::toQDir(PyObject* pyobj)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject* type = getPyTypeObjectForTypeName<QDir>();
    if (type) {
        if (Shiboken::Object::checkType(pyobj)) {
            SbkObject* sbkobject = reinterpret_cast<SbkObject*>(pyobj);
            void* cppobject = Shiboken::Object::cppPointer(sbkobject, type);
            return reinterpret_cast<QDir*>(cppobject);
        }
    }
#else
    Q_UNUSED(pyobj);
#endif
    return nullptr;
}

Py::Object PythonWrapper::fromQObject(QObject* object, const char* className)
{
    if (!object)
        return Py::None();
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // Access shiboken/PySide via C++
    //
    PyTypeObject * type = getPyTypeObjectForTypeName<QObject>();
    if (type) {
        SbkObjectType* sbk_type = reinterpret_cast<SbkObjectType*>(type);
        std::string typeName;
        if (className)
            typeName = className;
        else
            typeName = object->metaObject()->className();
        PyObject* pyobj = Shiboken::Object::newObject(sbk_type, object, false, false, typeName.c_str());
        return Py::asObject(pyobj);
    }
    throw Py::RuntimeError("Failed to wrap object");
#else
    // Access shiboken2/PySide2 via Python
    //
    return qt_wrapInstance<QObject*>(object, className, "shiboken2", "PySide2.QtCore", "wrapInstance");
#endif
#if 0 // Unwrapping using sip/PyQt
    Q_UNUSED(className);
    return qt_wrapInstance<QObject*>(object, "QObject", "sip", "PyQt5.QtCore", "wrapinstance");
#endif
}

Py::Object PythonWrapper::fromQWidget(QWidget* widget, const char* className)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // Access shiboken/PySide via C++
    //
    PyTypeObject * type = getPyTypeObjectForTypeName<QWidget>();
    if (type) {
        SbkObjectType* sbk_type = reinterpret_cast<SbkObjectType*>(type);
        std::string typeName;
        if (className)
            typeName = className;
        else
            typeName = widget->metaObject()->className();
        PyObject* pyobj = Shiboken::Object::newObject(sbk_type, widget, false, false, typeName.c_str());
        return Py::asObject(pyobj);
    }
    throw Py::RuntimeError("Failed to wrap widget");

#else
    // Access shiboken2/PySide2 via Python
    //
    return qt_wrapInstance<QWidget*>(widget, className, "shiboken2", "PySide2.QtWidgets", "wrapInstance");
#endif

#if 0 // Unwrapping using sip/PyQt
    Q_UNUSED(className);
    return qt_wrapInstance<QWidget*>(widget, "QWidget", "sip", "PyQt5.QtWidgets", "wrapinstance");
#endif
}

const char* PythonWrapper::getWrapperName(QObject* obj) const
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    const QMetaObject* meta = obj->metaObject();
    while (meta) {
        const char* typeName = meta->className();
        PyTypeObject* exactType = Shiboken::Conversions::getPythonTypeObject(typeName);
        if (exactType)
            return typeName;
        meta = meta->superClass();
    }
#else
    QUiLoader ui;
    QStringList names = ui.availableWidgets();
    const QMetaObject* meta = obj->metaObject();
    while (meta) {
        const char* typeName = meta->className();
        if (names.indexOf(QLatin1String(typeName)) >= 0)
            return typeName;
        meta = meta->superClass();
    }
#endif
    return "QObject";
}

bool PythonWrapper::loadCoreModule()
{
#if defined (HAVE_SHIBOKEN2) && (HAVE_PYSIDE2)
    // QtCore
    if (!SbkPySide2_QtCoreTypes) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide2.QtCore"));
        if (requiredModule.isNull())
            return false;
        SbkPySide2_QtCoreTypes = Shiboken::Module::getTypes(requiredModule);
    }
#elif defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // QtCore
    if (!SbkPySide_QtCoreTypes) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide.QtCore"));
        if (requiredModule.isNull())
            return false;
        SbkPySide_QtCoreTypes = Shiboken::Module::getTypes(requiredModule);
    }
#endif
    return true;
}

bool PythonWrapper::loadGuiModule()
{
#if defined (HAVE_SHIBOKEN2) && defined(HAVE_PYSIDE2)
    // QtGui
    if (!SbkPySide2_QtGuiTypes) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide2.QtGui"));
        if (requiredModule.isNull())
            return false;
        SbkPySide2_QtGuiTypes = Shiboken::Module::getTypes(requiredModule);
    }
#elif defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // QtGui
    if (!SbkPySide_QtGuiTypes) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide.QtGui"));
        if (requiredModule.isNull())
            return false;
        SbkPySide_QtGuiTypes = Shiboken::Module::getTypes(requiredModule);
    }
#endif
    return true;
}

bool PythonWrapper::loadWidgetsModule()
{
#if defined (HAVE_SHIBOKEN2) && defined(HAVE_PYSIDE2)
    // QtWidgets
    if (!SbkPySide2_QtWidgetsTypes) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide2.QtWidgets"));
        if (requiredModule.isNull())
            return false;
        SbkPySide2_QtWidgetsTypes = Shiboken::Module::getTypes(requiredModule);
    }
#endif
    return true;
}

bool PythonWrapper::loadUiToolsModule()
{
#if defined (HAVE_SHIBOKEN2) && defined(HAVE_PYSIDE2)
    // QtUiTools
    static PyTypeObject** SbkPySide2_QtUiToolsTypes = nullptr;
    if (!SbkPySide2_QtUiToolsTypes) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide2.QtUiTools"));
        if (requiredModule.isNull())
            return false;
        SbkPySide2_QtUiToolsTypes = Shiboken::Module::getTypes(requiredModule);
    }
#endif
    return true;
}

void PythonWrapper::createChildrenNameAttributes(PyObject* root, QObject* object)
{
    Q_FOREACH (QObject* child, object->children()) {
        const QByteArray name = child->objectName().toLocal8Bit();

        if (!name.isEmpty() && !name.startsWith("_") && !name.startsWith("qt_")) {
            bool hasAttr = PyObject_HasAttrString(root, name.constData());
            if (!hasAttr) {
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
                Shiboken::AutoDecRef pyChild(Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QObject>()), child));
                PyObject_SetAttrString(root, name.constData(), pyChild);
#else
                const char* className = qt_identifyType(child, "PySide2.QtWidgets");
                if (!className) {
                    if (qobject_cast<QWidget*>(child))
                        className = "QWidget";
                    else
                        className = "QObject";
                }

                Py::Object pyChild(qt_wrapInstance<QObject*>(child, className, "shiboken2", "PySide2.QtWidgets", "wrapInstance"));
                PyObject_SetAttrString(root, name.constData(), pyChild.ptr());
#endif
            }
            createChildrenNameAttributes(root, child);
        }
        createChildrenNameAttributes(root, child);
    }
}

void PythonWrapper::setParent(PyObject* pyWdg, QObject* parent)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    if (parent) {
        Shiboken::AutoDecRef pyParent(Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QWidget>()), parent));
        Shiboken::Object::setParent(pyParent, pyWdg);
    }
#else
    Q_UNUSED(pyWdg);
    Q_UNUSED(parent);
#endif
}
