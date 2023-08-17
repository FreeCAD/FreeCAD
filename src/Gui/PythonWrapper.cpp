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
# include <unordered_map>
# include <list>
# include <QApplication>
# include <QDir>
# include <QIcon>
# include <QPrinter>
# include <QWidget>
#endif

#include <QMetaType>

// Uncomment this block to remove PySide C++ support and switch to its Python interface
//#undef HAVE_SHIBOKEN2
//#undef HAVE_PYSIDE2
//#undef HAVE_SHIBOKEN6
//#undef HAVE_PYSIDE6

#ifdef FC_OS_WIN32
#undef max
#undef min
#ifdef _MSC_VER
#pragma warning( disable : 4099 )
#pragma warning( disable : 4522 )
#endif
#endif

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

//-----------------------------------------------------------------------------
//
// shiboken2 and PySide2 specific defines and includes
//

// class and struct used for SbkObject
//
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
PyTypeObject** SbkPySide2_QtCoreTypes = nullptr;
PyTypeObject** SbkPySide2_QtGuiTypes = nullptr;
PyTypeObject** SbkPySide2_QtWidgetsTypes = nullptr;
PyTypeObject** SbkPySide2_QtPrintSupportTypes = nullptr;
PyTypeObject** SbkPySide2_QtUiToolsTypes = nullptr;
# endif // HAVE_PYSIDE2
#endif // HAVE_SHIBOKEN2


//-----------------------------------------------------------------------------
//
// shiboken6 and PySide6 specific defines and includes
//

// class and struct used for SbkObject
//
#ifdef HAVE_SHIBOKEN6
# define HAVE_SHIBOKEN
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
# include <basewrapper.h>
# include <sbkconverter.h>
# include <sbkmodule.h>
# include <shiboken.h>
# ifdef HAVE_PYSIDE6
# define HAVE_PYSIDE
# define HAVE_SHIBOKEN_TYPE_FOR_TYPENAME
# include <signalmanager.h>
PyTypeObject** SbkPySide6_QtCoreTypes = nullptr;
PyTypeObject** SbkPySide6_QtGuiTypes = nullptr;
PyTypeObject** SbkPySide6_QtWidgetsTypes = nullptr;
PyTypeObject** SbkPySide6_QtPrintSupportTypes = nullptr;
PyTypeObject** SbkPySide6_QtUiToolsTypes = nullptr;
# endif // HAVE_PYSIDE6
#endif // HAVE_SHIBOKEN6


//-----------------------------------------------------------------------------

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
#include <Base/Interpreter.h>
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
    return toPythonFuncQuantityTyped(*static_cast<const Base::Quantity*>(cpp));
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
    auto pyIn = static_cast<PyObject*>(w);
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
PyTypeObject *getPyTypeObjectForTypeName();

/*!
 * \brief The WrapperManager class
 * This is a helper class that records the Python wrappers of a QObject and invalidates
 * them when the QObject is about to be destroyed.
 * This is to make sure that if the Python wrapper doesn't own the QObject it won't be notified
 * if the QObject is destroyed.
 * \code
 * ui = Gui.UiLoader()
 * lineedit = ui.createWidget("QLineEdit")
 * lineedit.deleteLater()
 * # Make sure this won't crash
 * lineedit.show()
 * \endcode
 */
class WrapperManager : public QObject
{
    std::unordered_map<QObject*, std::list<Py::Object>> wrappers;

public:
    static WrapperManager& instance()
    {
        static WrapperManager singleton;
        return singleton;
    }
    /*!
     * \brief addQObject
     * \param obj
     * \param pyobj
     * Add the QObject and its Python wrapper to the list.
     */
    void addQObject(QObject* obj, PyObject* pyobj)
    {
        if (wrappers.find(obj) == wrappers.end()) {
            QObject::connect(obj, &QObject::destroyed, this, &WrapperManager::destroyed);
        }

        auto& pylist = wrappers[obj];
        if (std::find_if(pylist.cbegin(), pylist.cend(),
                [pyobj](const Py::Object& py) {
                    return py.ptr() == pyobj;
                }) == pylist.end()) {

            pylist.emplace_back(pyobj);
        }
    }

private:
    /*!
     * \brief destroyed
     * \param obj
     * The listed QObject is about to be destroyed. Invalidate its Python wrappers now.
     */
    void destroyed(QObject* obj = nullptr)
    {
        if (obj) {
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
            auto key = wrappers.find(obj);
            if (key != wrappers.end()) {
                Base::PyGILStateLocker lock;
                for (const auto& it : key->second) {
                    auto value = it.ptr();
                    Shiboken::Object::setValidCpp(reinterpret_cast<SbkObject*>(value), false);
                }

                wrappers.erase(key);
            }
#endif
        }
    }
    void clear()
    {
        Base::PyGILStateLocker lock;
        wrappers.clear();
    }
    void wrapQApplication()
    {
        // We have to explicitly hold a reference to the wrapper of the QApplication
        // as otherwise it can happen that when running the gc the program crashes
        // The code snippet below caused a crash on older versions:
        // mw = Gui.getMainWindow()
        // mw.style()
        // import gc
        // gc.collect()
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
        PyTypeObject * type = getPyTypeObjectForTypeName<QApplication>();
        if (type) {
#if defined (HAVE_SHIBOKEN2)
            auto sbk_type = reinterpret_cast<SbkObjectType*>(type);
#else
            auto sbk_type = type;
#endif
            std::string typeName = "QApplication";
            PyObject* pyobj = Shiboken::Object::newObject(sbk_type, qApp, false, false, typeName.c_str());
            addQObject(qApp, pyobj);
        }
#endif
    }

    WrapperManager()
    {
        connect(QApplication::instance(), &QCoreApplication::aboutToQuit,
                this, &WrapperManager::clear);
        wrapQApplication();
    }
    ~WrapperManager() override = default;
};

template<typename qttype>
Py::Object qt_wrapInstance(qttype object,
                           const std::string& className,
                           const std::string& shiboken,
                           const std::string& pyside,
                           const std::string& wrap)
{
    PyObject* module = PyImport_ImportModule(shiboken.c_str());
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

    module = PyImport_ImportModule(pyside.c_str());
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

const char* qt_identifyType(QObject* ptr, const std::string& pyside)
{
    PyObject* module = PyImport_ImportModule(pyside.c_str());
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

void* qt_getCppPointer(const Py::Object& pyobject, const std::string& shiboken, const std::string& unwrap)
{
    // https://github.com/PySide/Shiboken/blob/master/shibokenmodule/typesystem_shiboken.xml
    PyObject* module = PyImport_ImportModule(shiboken.c_str());
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
#if defined (HAVE_SHIBOKEN2)
    SbkObjectType* sbkType = Shiboken::ObjectType::typeForTypeName(typeid(qttype).name());
    if (sbkType)
        return &(sbkType->type);
#else
    return Shiboken::ObjectType::typeForTypeName(typeid(qttype).name());
#endif
#else
    return Shiboken::SbkType<qttype>();
#endif
#endif
    return nullptr;
}

template<typename qttype>
qttype* qt_getCppType(PyObject* pyobj)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject * type = getPyTypeObjectForTypeName<qttype>();
    if (type) {
        if (Shiboken::Object::checkType(pyobj)) {
            auto sbkobject = reinterpret_cast<SbkObject *>(pyobj);
            void* cppobject = Shiboken::Object::cppPointer(sbkobject, type);
            return static_cast<qttype*>(cppobject);
        }
    }
#else
    Q_UNUSED(pyobj)
#endif

    return nullptr;
}

bool loadPySideModule(const std::string& moduleName, PyTypeObject**& types)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    if (!types) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import(moduleName.c_str()));
        if (requiredModule.isNull())
            return false;
        types = Shiboken::Module::getTypes(requiredModule);
    }
#else
    Q_UNUSED(moduleName)
    Q_UNUSED(types)
#endif
    return true;
}

}

// --------------------------------------------------------

#ifdef HAVE_SHIBOKEN6
std::string PythonWrapper::shiboken{"shiboken6"};
std::string PythonWrapper::PySide{"PySide6"};
#elif HAVE_SHIBOKEN2
std::string PythonWrapper::shiboken{"shiboken2"};
std::string PythonWrapper::PySide{"PySide2"};
#else
std::string PythonWrapper::shiboken{"shiboken"};
std::string PythonWrapper::PySide{"PySide"};
#endif

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
    return qt_getCppType<QObject>(pyobject.ptr());
#else
    // Access shiboken/PySide via Python
    //
    void* ptr = qt_getCppPointer(pyobject, shiboken, "getCppPointer");
    return static_cast<QObject*>(ptr);
#endif

    return nullptr;
}

QGraphicsItem* PythonWrapper::toQGraphicsItem(PyObject* pyPtr)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    return qt_getCppType<QGraphicsItem>(pyPtr);
#else
    // Access shiboken/PySide via Python
    //
    void* ptr = qt_getCppPointer(Py::asObject(pyPtr), shiboken, "getCppPointer");
    return static_cast<QGraphicsItem*>(ptr);
#endif
    return nullptr;
}

QGraphicsItem* PythonWrapper::toQGraphicsItem(const Py::Object& pyobject)
{
    return toQGraphicsItem(pyobject.ptr());
}

QGraphicsObject* PythonWrapper::toQGraphicsObject(PyObject* pyPtr)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    return qt_getCppType<QGraphicsObject>(pyPtr);
#else
    // Access shiboken/PySide via Python
    //
    void* ptr = qt_getCppPointer(Py::asObject(pyPtr), shiboken, "getCppPointer");
    return reinterpret_cast<QGraphicsObject*>(ptr);
#endif
    return nullptr;
}

QGraphicsObject* PythonWrapper::toQGraphicsObject(const Py::Object& pyobject)
{
    return toQGraphicsObject(pyobject.ptr());
}

Py::Object PythonWrapper::fromQImage(const QImage& img)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
#if defined (HAVE_SHIBOKEN2)
    PyObject* pyobj = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QImage>()),
                              const_cast<QImage*>(&img));
#else
    PyObject* pyobj = Shiboken::Conversions::copyToPython(getPyTypeObjectForTypeName<QImage>(),
                              const_cast<QImage*>(&img));
#endif
    if (pyobj) {
        return Py::asObject(pyobj);
    }
#else
    // Access shiboken/PySide via Python
    //
    return qt_wrapInstance<const QImage*>(&img, "QImage", shiboken, PySide + ".QtGui", "wrapInstance");
#endif
    throw Py::RuntimeError("Failed to wrap icon");
}

QImage *PythonWrapper::toQImage(PyObject *pyobj)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    return qt_getCppType<QImage>(pyobj);
#else
    Q_UNUSED(pyobj);
#endif
    return nullptr;
}

Py::Object PythonWrapper::fromQIcon(const QIcon* icon)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    const char* typeName = typeid(*const_cast<QIcon*>(icon)).name();
#if defined (HAVE_SHIBOKEN2)
    PyObject* pyobj = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QIcon>()),
                              const_cast<QIcon*>(icon), true, false, typeName);
#else
    PyObject* pyobj = Shiboken::Object::newObject(getPyTypeObjectForTypeName<QIcon>(),
                              const_cast<QIcon*>(icon), true, false, typeName);
#endif
    if (pyobj)
        return Py::asObject(pyobj);
#else
    // Access shiboken/PySide via Python
    //
    return qt_wrapInstance<const QIcon*>(icon, "QIcon", shiboken, PySide + ".QtGui", "wrapInstance");
#endif
    throw Py::RuntimeError("Failed to wrap icon");
}

QIcon *PythonWrapper::toQIcon(PyObject *pyobj)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    return qt_getCppType<QIcon>(pyobj);
#else
    Q_UNUSED(pyobj);
#endif
    return nullptr;
}

Py::Object PythonWrapper::fromQDir(const QDir& dir)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    const char* typeName = typeid(dir).name();
#if defined (HAVE_SHIBOKEN2)
    PyObject* pyobj = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QDir>()),
        const_cast<QDir*>(&dir), false, false, typeName);
#else
    PyObject* pyobj = Shiboken::Object::newObject(getPyTypeObjectForTypeName<QDir>(),
        const_cast<QDir*>(&dir), false, false, typeName);
#endif
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
    return qt_getCppType<QDir>(pyobj);
#else
    Q_UNUSED(pyobj);
#endif
    return nullptr;
}

Py::Object PythonWrapper::fromQPrinter(QPrinter* printer)
{
    if (!printer)
        return Py::None();
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // Access shiboken/PySide via C++
    //
    PyTypeObject * type = getPyTypeObjectForTypeName<QPrinter>();
    if (!type) {
        type = Shiboken::Conversions::getPythonTypeObject("QPrinter");
    }
    if (type) {
#if defined (HAVE_SHIBOKEN2)
        auto sbk_type = reinterpret_cast<SbkObjectType*>(type);
#else
        auto sbk_type = type;
#endif
        PyObject* pyobj = Shiboken::Object::newObject(sbk_type, printer, false, false, "QPrinter");
        return Py::asObject(pyobj);
    }

    throw Py::RuntimeError("Failed to wrap object");
#else
    // Access shiboken/PySide via Python
    //
    return qt_wrapInstance<QPrinter*>(printer, "QPrinter", shiboken, PySide + ".QtCore", "wrapInstance");
#endif
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
#if defined (HAVE_SHIBOKEN2)
        auto sbk_type = reinterpret_cast<SbkObjectType*>(type);
#else
        auto sbk_type = type;
#endif
        std::string typeName;
        if (className)
            typeName = className;
        else
            typeName = object->metaObject()->className();
        PyObject* pyobj = Shiboken::Object::newObject(sbk_type, object, false, false, typeName.c_str());
        WrapperManager::instance().addQObject(object, pyobj);
        return Py::asObject(pyobj);
    }
    throw Py::RuntimeError("Failed to wrap object");
#else
    // Access shiboken/PySide via Python
    //
    return qt_wrapInstance<QObject*>(object, className, shiboken, PySide + ".QtCore", "wrapInstance");
#endif
}

Py::Object PythonWrapper::fromQWidget(QWidget* widget, const char* className)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // Access shiboken/PySide via C++
    //
    PyTypeObject * type = getPyTypeObjectForTypeName<QWidget>();
    if (type) {
#if defined (HAVE_SHIBOKEN2)
        auto sbk_type = reinterpret_cast<SbkObjectType*>(type);
#else
        auto sbk_type = type;
#endif
        std::string typeName;
        if (className)
            typeName = className;
        else
            typeName = widget->metaObject()->className();
        PyObject* pyobj = Shiboken::Object::newObject(sbk_type, widget, false, false, typeName.c_str());
        WrapperManager::instance().addQObject(widget, pyobj);
        return Py::asObject(pyobj);
    }
    throw Py::RuntimeError("Failed to wrap widget");

#else
    // Access shiboken/PySide via Python
    //
    return qt_wrapInstance<QWidget*>(widget, className, shiboken, PySide + ".QtWidgets", "wrapInstance");
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
    return loadPySideModule(PySide + ".QtCore", SbkPySide2_QtCoreTypes);
#elif defined (HAVE_SHIBOKEN6) && (HAVE_PYSIDE6)
    return loadPySideModule(PySide + ".QtCore", SbkPySide6_QtCoreTypes);
#endif
    return true;
}

bool PythonWrapper::loadGuiModule()
{
#if defined (HAVE_SHIBOKEN2) && defined(HAVE_PYSIDE2)
    return loadPySideModule(PySide + ".QtGui", SbkPySide2_QtGuiTypes);
#elif defined (HAVE_SHIBOKEN6) && defined(HAVE_PYSIDE6)
    return loadPySideModule(PySide + ".QtGui", SbkPySide6_QtGuiTypes);
#endif
    return true;
}

bool PythonWrapper::loadWidgetsModule()
{
#if defined (HAVE_SHIBOKEN2) && defined(HAVE_PYSIDE2)
    return loadPySideModule(PySide + ".QtWidgets", SbkPySide2_QtWidgetsTypes);
#elif defined (HAVE_SHIBOKEN6) && defined(HAVE_PYSIDE6)
    return loadPySideModule(PySide + ".QtWidgets", SbkPySide6_QtWidgetsTypes);
#endif
    return true;
}

bool PythonWrapper::loadPrintSupportModule()
{
#if defined (HAVE_SHIBOKEN2) && defined(HAVE_PYSIDE2)
    return loadPySideModule(PySide + ".QtPrintSupport", SbkPySide2_QtPrintSupportTypes);
#elif defined (HAVE_SHIBOKEN6) && defined(HAVE_PYSIDE6)
    return loadPySideModule(PySide + ".QtPrintSupport", SbkPySide6_QtPrintSupportTypes);
#endif
    return true;
}

bool PythonWrapper::loadUiToolsModule()
{
#if defined (HAVE_SHIBOKEN2) && defined(HAVE_PYSIDE2)
    return loadPySideModule(PySide + ".QtUiTools", SbkPySide2_QtUiToolsTypes);
#elif defined (HAVE_SHIBOKEN6) && defined(HAVE_PYSIDE6)
    return loadPySideModule(PySide + ".QtUiTools", SbkPySide6_QtUiToolsTypes);
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
#if defined (HAVE_SHIBOKEN2)
                Shiboken::AutoDecRef pyChild(Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QObject>()), child));
#else
                Shiboken::AutoDecRef pyChild(Shiboken::Conversions::pointerToPython(getPyTypeObjectForTypeName<QObject>(), child));
#endif
                PyObject_SetAttrString(root, name.constData(), pyChild);
#else
                const char* className = qt_identifyType(child, PySide + ".QtWidgets");
                if (!className) {
                    if (qobject_cast<QWidget*>(child))
                        className = "QWidget";
                    else
                        className = "QObject";
                }

                Py::Object pyChild(qt_wrapInstance<QObject*>(child, className, shiboken, PySide + ".QtWidgets", "wrapInstance"));
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
#if defined (HAVE_SHIBOKEN2)
        Shiboken::AutoDecRef pyParent(Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType*>(getPyTypeObjectForTypeName<QWidget>()), parent));
#else
        Shiboken::AutoDecRef pyParent(Shiboken::Conversions::pointerToPython(getPyTypeObjectForTypeName<QWidget>(), parent));
#endif
        Shiboken::Object::setParent(pyParent, pyWdg);
    }
#else
    Q_UNUSED(pyWdg);
    Q_UNUSED(parent);
#endif
}
