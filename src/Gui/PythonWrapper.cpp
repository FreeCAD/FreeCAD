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
# include <QAction>
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
# ifdef HAVE_PYSIDE2
#  define HAVE_PYSIDE

// Include shiboken first to get the version
#  include <shiboken.h>

// Do not use SHIBOKEN_MICRO_VERSION; it might contain a dot
#  define SHIBOKEN_FULL_VERSION QT_VERSION_CHECK(SHIBOKEN_MAJOR_VERSION, SHIBOKEN_MINOR_VERSION, 0)

#  include <pyside2_qtcore_python.h>
#  include <pyside2_qtgui_python.h>
#  include <pyside2_qtwidgets_python.h>
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
# ifdef HAVE_PYSIDE6
#  define HAVE_PYSIDE
#  define HAVE_SHIBOKEN_TYPE_FOR_TYPENAME
# endif // HAVE_PYSIDE6
# include <sbkversion.h>
# define SHIBOKEN_FULL_VERSION QT_VERSION_CHECK(SHIBOKEN_MAJOR_VERSION, SHIBOKEN_MINOR_VERSION, 0)
# if (SHIBOKEN_FULL_VERSION >= QT_VERSION_CHECK(6, 7, 0))
#  define HAVE_SHIBOKEN_TYPEINITSTRUCT
# endif
#endif // HAVE_SHIBOKEN6

//-----------------------------------------------------------------------------

#ifdef HAVE_SHIBOKEN
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
# include <basewrapper.h>
# include <sbkconverter.h>
# include <sbkmodule.h>
# include <shiboken.h>
#endif // HAVE_SHIBOKEN

#ifdef HAVE_PYSIDE
# include <signalmanager.h>
#endif // HAVE_PYSIDE

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

// NOLINTBEGIN
#if defined(HAVE_SHIBOKEN2)
PyTypeObject** SbkPySide2_QtCoreTypes           = nullptr;
PyTypeObject** SbkPySide2_QtGuiTypes            = nullptr;
PyTypeObject** SbkPySide2_QtWidgetsTypes        = nullptr;
PyTypeObject** SbkPySide2_QtPrintSupportTypes   = nullptr;
PyTypeObject** SbkPySide2_QtUiToolsTypes        = nullptr;
constexpr auto &SbkPySide_QtCoreTypes           = SbkPySide2_QtCoreTypes;
constexpr auto &SbkPySide_QtGuiTypes            = SbkPySide2_QtGuiTypes;
constexpr auto &SbkPySide_QtWidgetsTypes        = SbkPySide2_QtWidgetsTypes;
constexpr auto &SbkPySide_QtPrintSupportTypes   = SbkPySide2_QtPrintSupportTypes;
constexpr auto &SbkPySide_QtUiToolsTypes        = SbkPySide2_QtUiToolsTypes;
#if !defined(HAVE_PYSIDE2)
constexpr const char* ModuleShiboken            = "shiboken2";
#endif
constexpr const char* ModulePySide              = "PySide2";
#elif defined(HAVE_SHIBOKEN6)
#ifdef HAVE_SHIBOKEN_TYPEINITSTRUCT
Shiboken::Module::TypeInitStruct* SbkPySide6_QtCoreTypes           = nullptr;
Shiboken::Module::TypeInitStruct* SbkPySide6_QtGuiTypes            = nullptr;
Shiboken::Module::TypeInitStruct* SbkPySide6_QtWidgetsTypes        = nullptr;
Shiboken::Module::TypeInitStruct* SbkPySide6_QtPrintSupportTypes   = nullptr;
Shiboken::Module::TypeInitStruct* SbkPySide6_QtUiToolsTypes        = nullptr;
#else
PyTypeObject** SbkPySide6_QtCoreTypes           = nullptr;
PyTypeObject** SbkPySide6_QtGuiTypes            = nullptr;
PyTypeObject** SbkPySide6_QtWidgetsTypes        = nullptr;
PyTypeObject** SbkPySide6_QtPrintSupportTypes   = nullptr;
PyTypeObject** SbkPySide6_QtUiToolsTypes        = nullptr;
#endif
constexpr auto &SbkPySide_QtCoreTypes           = SbkPySide6_QtCoreTypes;
constexpr auto &SbkPySide_QtGuiTypes            = SbkPySide6_QtGuiTypes;
constexpr auto &SbkPySide_QtWidgetsTypes        = SbkPySide6_QtWidgetsTypes;
constexpr auto &SbkPySide_QtPrintSupportTypes   = SbkPySide6_QtPrintSupportTypes;
constexpr auto &SbkPySide_QtUiToolsTypes        = SbkPySide6_QtUiToolsTypes;
#if !defined(HAVE_PYSIDE6)
constexpr const char* ModuleShiboken            = "shiboken6";
#endif
constexpr const char* ModulePySide              = "PySide6";
#else
static PyTypeObject** SbkPySide_DummyTypes;
constexpr auto &SbkPySide_QtCoreTypes           = SbkPySide_DummyTypes;
constexpr auto &SbkPySide_QtGuiTypes            = SbkPySide_DummyTypes;
constexpr auto &SbkPySide_QtWidgetsTypes        = SbkPySide_DummyTypes;
constexpr auto &SbkPySide_QtPrintSupportTypes   = SbkPySide_DummyTypes;
constexpr auto &SbkPySide_QtUiToolsTypes        = SbkPySide_DummyTypes;
# if QT_VERSION < QT_VERSION_CHECK(6,0,0)
constexpr const char* ModuleShiboken            = "shiboken2";
constexpr const char* ModulePySide              = "PySide2";
# else
constexpr const char* ModuleShiboken            = "shiboken6";
constexpr const char* ModulePySide              = "PySide6";
# endif
#endif
// NOLINTEND

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

PyObject* toPythonFuncQuantityTyped(Base::Quantity cpx)
{
    return new Base::QuantityPy(new Base::Quantity(cpx));
}

PyObject* toPythonFuncQuantity(const void* cpp)
{
    return toPythonFuncQuantityTyped(*static_cast<const Base::Quantity*>(cpp));
}

void toCppPointerConvFuncQuantity(PyObject* pyobj,void* cpp)
{
   *static_cast<Base::Quantity*>(cpp) = *static_cast<Base::QuantityPy*>(pyobj)->getQuantityPtr();
}

PythonToCppFunc toCppPointerCheckFuncQuantity(PyObject* obj)
{
    if (PyObject_TypeCheck(obj, &(Base::QuantityPy::Type))) {
        return toCppPointerConvFuncQuantity;
    }
    return nullptr;
}

void BaseQuantity_PythonToCpp_QVariant(PyObject* pyIn, void* cppOut)
{
    Base::Quantity* q = static_cast<Base::QuantityPy*>(pyIn)->getQuantityPtr();
    *((QVariant*)cppOut) = QVariant::fromValue<Base::Quantity>(*q);
}

PythonToCppFunc isBaseQuantity_PythonToCpp_QVariantConvertible(PyObject* obj)
{
    if (PyObject_TypeCheck(obj, &(Base::QuantityPy::Type))) {
        return BaseQuantity_PythonToCpp_QVariant;
    }
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

static std::string getPySideModuleName(const std::string& moduleName)
{
    std::string name(ModulePySide);
    name += '.';
    name += moduleName;

    return name;
}

#ifdef HAVE_SHIBOKEN_TYPEINITSTRUCT
static bool loadPySideModule(const std::string& moduleName, Shiboken::Module::TypeInitStruct*& types)
#else
static bool loadPySideModule(const std::string& moduleName, PyTypeObject**& types)
#endif
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    if (!types) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import(getPySideModuleName(moduleName).c_str()));
        if (requiredModule.isNull()) {
            return false;
        }
        types = Shiboken::Module::getTypes(requiredModule);
    }
#else
    Q_UNUSED(moduleName)
    Q_UNUSED(types)
#endif
    return true;
}

#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
template<typename qttype>
#if defined (HAVE_SHIBOKEN2)
SbkObjectType*
#else
PyTypeObject*
#endif
getPyTypeObjectForTypeName()
{
#if defined (HAVE_SHIBOKEN_TYPE_FOR_TYPENAME)
# if defined (HAVE_SHIBOKEN2)
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeid(qttype).name());
    return reinterpret_cast<SbkObjectType*>(&sbkType->type);
# else
    return Shiboken::ObjectType::typeForTypeName(typeid(qttype).name());
# endif
#else
# if defined (HAVE_SHIBOKEN2)
    return reinterpret_cast<SbkObjectType*>(Shiboken::SbkType<qttype>());
# else
    return Shiboken::SbkType<qttype>();
# endif
#endif
}

template<typename qttype>
qttype* qt_getCppType(PyObject* pyobj)
{
    auto type = getPyTypeObjectForTypeName<qttype>();
    if (type) {
        if (Shiboken::Object::checkType(pyobj)) {
            auto skbobj = reinterpret_cast<SbkObject *>(pyobj);
            auto pytypeobj = reinterpret_cast<PyTypeObject *>(type);
            return static_cast<qttype*>(Shiboken::Object::cppPointer(skbobj, pytypeobj));
        }
    }
    return nullptr;
}

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
     * Connects destruction event of a QObject with invalidation of its PythonWrapper via a helper QObject.
     */
    void addQObject(QObject* obj, PyObject* pyobj)
    {
        // static array to contain created connections so they can be safely disconnected later
        static std::map<QObject*, QMetaObject::Connection> connections = {};

        const auto PyW_uniqueName = QString::number(reinterpret_cast<quintptr>(pyobj));
        auto PyW_invalidator = findChild<QObject*>(PyW_uniqueName, Qt::FindDirectChildrenOnly);

        if (PyW_invalidator == nullptr) {
            PyW_invalidator = new QObject(this);
            PyW_invalidator->setObjectName(PyW_uniqueName);

            Py_INCREF (pyobj);
        }
        else if (connections.contains(PyW_invalidator)) {
            disconnect(connections[PyW_invalidator]);
            connections.erase(PyW_invalidator);
        }

        auto destroyedFun = [pyobj]() {
            Base::PyGILStateLocker lock;

            if (auto sbkPtr = reinterpret_cast<SbkObject*>(pyobj); sbkPtr != nullptr) {
                Shiboken::Object::setValidCpp(sbkPtr, false);
            }
            else {
                Base::Console().developerError("WrapperManager", "A QObject has just been destroyed after its Pythonic wrapper.\n");
            }

            Py_DECREF (pyobj);
        };

        connections[PyW_invalidator] = connect(PyW_invalidator, &QObject::destroyed, this, destroyedFun);
        connect(obj, &QObject::destroyed, PyW_invalidator, &QObject::deleteLater);
    }

private:
    void wrapQApplication()
    {
        // We have to explicitly hold a reference to the wrapper of the QApplication
        // as otherwise it can happen that when running the gc the program crashes
        // The code snippet below caused a crash on older versions:
        // mw = Gui.getMainWindow()
        // mw.style()
        // import gc
        // gc.collect()
        auto type = getPyTypeObjectForTypeName<QApplication>();
        if (type) {
            PyObject* pyobj = Shiboken::Object::newObject(type, qApp, false, false, "QApplication");
            addQObject(qApp, pyobj);
        }
    }

    WrapperManager()
    {
        wrapQApplication();
    }
    ~WrapperManager() override = default;
};

#else

static std::string formatModuleError(const std::string& name)
{
    std::string error = "Cannot load " + name + " module";
    return error;
}

static PyObject* importShiboken()
{
    PyObject* obj = PyImport_ImportModule(ModuleShiboken);
    if (obj) {
        return obj;
    }

    throw Py::Exception(PyExc_ImportError, formatModuleError(ModuleShiboken));
}

static PyObject* importPySide(const std::string& moduleName)
{
    std::string name = getPySideModuleName(moduleName);
    PyObject* obj = PyImport_ImportModule(name.c_str());
    if (obj) {
        return obj;
    }

    throw Py::Exception(PyExc_ImportError, formatModuleError(name));
}

template<typename qttype>
qttype* qt_getCppType(PyObject* pyobj)
{
    // https://github.com/PySide/Shiboken/blob/master/shibokenmodule/typesystem_shiboken.xml
    Py::Module mainmod(importShiboken(), true);
    Py::Callable func = mainmod.getDict().getItem("getCppPointer");
    if (func.isNull()) {
        throw Py::RuntimeError("Failed to get C++ pointer");
    }

    Py::Tuple arguments(1);
    arguments[0] = Py::Object(pyobj); // PySide pointer
    Py::Tuple result(func.apply(arguments));
    return reinterpret_cast<qttype*>(PyLong_AsVoidPtr(result[0].ptr()));
}

template<typename qttype>
Py::Object qt_wrapInstance(qttype object,
                           const std::string& className,
                           const std::string& moduleName)
{
    Py::Module mainmod(importShiboken(), true);
    Py::Callable func = mainmod.getDict().getItem("wrapInstance");
    if (func.isNull()) {
        // Failure will be handled in the calling instance
        return func;
    }

    Py::Module qtmod(importPySide(moduleName));
    Py::Object item = qtmod.getDict().getItem(className);
    if (item.isNull()) {
        // Failure will be handled in the calling instance
        return item;
    }

    Py::Tuple arguments(2);
    arguments[0] = Py::asObject(PyLong_FromVoidPtr((void*)object));
    arguments[1] = item;

    return func.apply(arguments);
}

const char* qt_identifyType(QObject* ptr, const std::string& moduleName)
{
    Py::Module qtmod(importPySide(moduleName));
    const QMetaObject* metaObject = ptr->metaObject();
    while (metaObject) {
        const char* className = metaObject->className();
        if (qtmod.getDict().hasKey(className)) {
            return className;
        }
        metaObject = metaObject->superClass();
    }

    return nullptr;
}

#endif

}

// --------------------------------------------------------

PythonWrapper::PythonWrapper()
{
#if defined (HAVE_SHIBOKEN)
    static bool init;
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
    return qt_getCppType<QObject>(pyobject.ptr());
}

qsizetype PythonWrapper::tryEnum(PyObject* pyPtr)
{
    if (PyObject* number = PyNumber_Long(pyPtr)) {
        Py::Long longObj(number, true);
        return longObj.as_long();
    }

    // if PyNumber_Long failed then an exception is set
    PyErr_Clear();

    Py::Object object(pyPtr);
    if (object.hasAttr(std::string("value"))) {
        Py::Long longObj(object.getAttr(std::string("value")));
        return longObj.as_long();
    }

    return 0;
}

qsizetype PythonWrapper::toEnum(PyObject* pyPtr)
{
    try {
        return tryEnum(pyPtr);
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.reportException();
        return 0;
    }
}

qsizetype PythonWrapper::toEnum(const Py::Object& pyobject)
{
    return toEnum(pyobject.ptr());
}

Py::Object PythonWrapper::tryToStandardButton(qsizetype value)
{
    std::stringstream cmd;
    cmd << "from PySide import QtWidgets\n";
    cmd << "btn = QtWidgets.QDialogButtonBox.StandardButton(" << value << ")";
    return Py::asObject(Base::Interpreter().getValue(cmd.str().c_str(), "btn"));
}

Py::Object PythonWrapper::toStandardButton(qsizetype value)
{
    try {
        return tryToStandardButton(value);
    }
    catch (Py::Exception& e) {
        e.clear();
        return Py::Long(value);
    }
}

QGraphicsItem* PythonWrapper::toQGraphicsItem(PyObject* pyPtr)
{
    return qt_getCppType<QGraphicsItem>(pyPtr);
}

QGraphicsItem* PythonWrapper::toQGraphicsItem(const Py::Object& pyobject)
{
    return toQGraphicsItem(pyobject.ptr());
}

QGraphicsObject* PythonWrapper::toQGraphicsObject(PyObject* pyPtr)
{
    return qt_getCppType<QGraphicsObject>(pyPtr);
}

QGraphicsObject* PythonWrapper::toQGraphicsObject(const Py::Object& pyobject)
{
    return toQGraphicsObject(pyobject.ptr());
}

Py::Object PythonWrapper::fromQImage(const QImage& img)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    auto type = getPyTypeObjectForTypeName<QImage>();
    if (type) {
        PyObject* pyobj = Shiboken::Conversions::copyToPython(type, const_cast<QImage*>(&img));
        return Py::asObject(pyobj);
    }
#else
    // Access shiboken/PySide via Python
    Py::Object obj = qt_wrapInstance<const QImage*>(&img, "QImage", "QtGui");
    if (!obj.isNull()) {
        return obj;
    }
#endif
    throw Py::RuntimeError("Failed to wrap image");
}

QImage *PythonWrapper::toQImage(PyObject *pyobj)
{
    return qt_getCppType<QImage>(pyobj);
}

Py::Object PythonWrapper::fromQIcon(const QIcon* icon)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    auto type = getPyTypeObjectForTypeName<QIcon>();
    if (type) {
        const char* typeName = typeid(*const_cast<QIcon*>(icon)).name();
        PyObject* pyobj = Shiboken::Object::newObject(type, const_cast<QIcon*>(icon), true, false, typeName);
        return Py::asObject(pyobj);
    }
#else
    // Access shiboken/PySide via Python
    Py::Object obj = qt_wrapInstance<const QIcon*>(icon, "QIcon", "QtGui");
    if (!obj.isNull()) {
        return obj;
    }
#endif
    throw Py::RuntimeError("Failed to wrap icon");
}

QIcon *PythonWrapper::toQIcon(PyObject *pyobj)
{
    return qt_getCppType<QIcon>(pyobj);
}

Py::Object PythonWrapper::fromQDir(const QDir& dir)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    auto type = getPyTypeObjectForTypeName<QDir>();
    if (type) {
        const char* typeName = typeid(dir).name();
        PyObject* pyobj = Shiboken::Object::newObject(type, const_cast<QDir*>(&dir), false, false, typeName);
        return Py::asObject(pyobj);
    }
#else
    // Access shiboken/PySide via Python
    Py::Object obj = qt_wrapInstance<const QDir*>(&dir, "QDir", "QtGui");
    if (!obj.isNull()) {
        return obj;
    }
#endif
    throw Py::RuntimeError("Failed to wrap directory");
}

QDir* PythonWrapper::toQDir(PyObject* pyobj)
{
    return qt_getCppType<QDir>(pyobj);
}

Py::Object PythonWrapper::fromQAction(QAction* action)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // Access shiboken/PySide via C++
    auto type = getPyTypeObjectForTypeName<QAction>();
    if (type) {
        PyObject* pyobj = Shiboken::Object::newObject(type, action, false, false, "QAction");
        WrapperManager::instance().addQObject(action, pyobj);
        return Py::asObject(pyobj);
    }
#else
    // Access shiboken/PySide via Python
# if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    constexpr const char* qtModWithQAction = "QtWidgets";
# else
    constexpr const char* qtModWithQAction = "QtGui";
# endif
    Py::Object obj = qt_wrapInstance<QAction*>(action, "QAction", qtModWithQAction);
    if (!obj.isNull()) {
        return obj;
    }
#endif
    throw Py::RuntimeError("Failed to wrap action");
}

Py::Object PythonWrapper::fromQPrinter(QPrinter* printer)
{
    if (!printer) {
        return Py::None();
    }
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // Access shiboken/PySide via C++
    auto type = getPyTypeObjectForTypeName<QPrinter>();
    if (!type) {
        // XXX: Why is QPrinter special?
#if defined (HAVE_SHIBOKEN2)
        type = reinterpret_cast<SbkObjectType*>(Shiboken::Conversions::getPythonTypeObject("QPrinter"));
#else
        type = Shiboken::Conversions::getPythonTypeObject("QPrinter");
#endif
    }
    if (type) {
        PyObject* pyobj = Shiboken::Object::newObject(type, printer, false, false, "QPrinter");
        return Py::asObject(pyobj);
    }
#else
    // Access shiboken/PySide via Python
    Py::Object obj = qt_wrapInstance<QPrinter*>(printer, "QPrinter", "QtCore");
    if (!obj.isNull()) {
        return obj;
    }
#endif
    throw Py::RuntimeError("Failed to wrap printer");
}

Py::Object PythonWrapper::fromQObject(QObject* object, const char* className)
{
    if (!object) {
        return Py::None();
    }
    const char* typeName;
    if (className) {
        typeName = className;
    }
    else {
        typeName = object->metaObject()->className();
    }
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // Access shiboken/PySide via C++
    auto type = getPyTypeObjectForTypeName<QObject>();
    if (type) {
        PyObject* pyobj = Shiboken::Object::newObject(type, object, false, false, typeName);
        WrapperManager::instance().addQObject(object, pyobj);
        return Py::asObject(pyobj);
    }
#else
    // Access shiboken/PySide via Python
    Py::Object obj = qt_wrapInstance<QObject*>(object, typeName, "QtCore");
    if (!obj.isNull()) {
        return obj;
    }
#endif
    throw Py::RuntimeError("Failed to wrap object");
}

Py::Object PythonWrapper::fromQWidget(QWidget* widget, const char* className)
{
    const char* typeName;
    if (className) {
        typeName = className;
    }
    else {
        typeName = widget->metaObject()->className();
    }
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // Access shiboken/PySide via C++
    auto type = getPyTypeObjectForTypeName<QWidget>();
    if (type) {
        PyObject* pyobj = Shiboken::Object::newObject(type, widget, false, false, typeName);
        WrapperManager::instance().addQObject(widget, pyobj);
        return Py::asObject(pyobj);
    }
#else
    // Access shiboken/PySide via Python
    Py::Object obj = qt_wrapInstance<QWidget*>(widget, typeName, "QtWidgets");
    if (!obj.isNull()) {
        return obj;
    }
#endif
    throw Py::RuntimeError("Failed to wrap widget");
}

const char* PythonWrapper::getWrapperName(QObject* obj) const
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    const QMetaObject* meta = obj->metaObject();
    while (meta) {
        const char* typeName = meta->className();
        PyTypeObject* exactType = Shiboken::Conversions::getPythonTypeObject(typeName);
        if (exactType) {
            return typeName;
        }
        meta = meta->superClass();
    }
#else
    QUiLoader ui;
    QStringList names = ui.availableWidgets();
    const QMetaObject* meta = obj->metaObject();
    while (meta) {
        const char* typeName = meta->className();
        if (names.indexOf(QLatin1String(typeName)) >= 0) {
            return typeName;
        }
        meta = meta->superClass();
    }
#endif
    return "QObject";
}

bool PythonWrapper::loadCoreModule()
{
    return loadPySideModule("QtCore", SbkPySide_QtCoreTypes);
}

bool PythonWrapper::loadGuiModule()
{
    return loadPySideModule("QtGui", SbkPySide_QtGuiTypes);
}

bool PythonWrapper::loadWidgetsModule()
{
    return loadPySideModule("QtWidgets", SbkPySide_QtWidgetsTypes);
}

bool PythonWrapper::loadPrintSupportModule()
{
    return loadPySideModule("QtPrintSupport", SbkPySide_QtPrintSupportTypes);
}

bool PythonWrapper::loadUiToolsModule()
{
    return loadPySideModule("QtUiTools", SbkPySide_QtUiToolsTypes);
}

void PythonWrapper::createChildrenNameAttributes(PyObject* root, QObject* object)
{
    Q_FOREACH (QObject* child, object->children()) {
        const QByteArray name = child->objectName().toLocal8Bit();

        if (!name.isEmpty() && !name.startsWith("_") && !name.startsWith("qt_")) {
            bool hasAttr = PyObject_HasAttrString(root, name.constData());
            if (!hasAttr) {
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
                Shiboken::AutoDecRef pyChild(Shiboken::Conversions::pointerToPython(getPyTypeObjectForTypeName<QObject>(), child));
                PyObject_SetAttrString(root, name.constData(), pyChild);
#else
                const char* className = qt_identifyType(child, "QtWidgets");
                if (!className) {
                    if (qobject_cast<QWidget*>(child)) {
                        className = "QWidget";
                    }
                    else {
                        className = "QObject";
                    }
                }

                Py::Object pyChild(qt_wrapInstance<QObject*>(child, className, "QtWidgets"));
                if (!pyChild.isNull()) {
                    PyObject_SetAttrString(root, name.constData(), pyChild.ptr());
                }
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
        Shiboken::AutoDecRef pyParent(Shiboken::Conversions::pointerToPython(getPyTypeObjectForTypeName<QWidget>(), parent));
        Shiboken::Object::setParent(pyParent, pyWdg);
    }
#else
    Q_UNUSED(pyWdg);
    Q_UNUSED(parent);
#endif
}
