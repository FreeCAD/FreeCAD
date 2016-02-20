/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
# include <QTextStream>
#endif

// Uncomment this block to remove PySide support and switch back to PyQt
// #undef HAVE_SHIBOKEN
// #undef HAVE_PYSIDE

#ifdef FC_OS_WIN32
#undef max
#undef min
#ifdef _MSC_VER
#pragma warning( disable : 4099 )
#pragma warning( disable : 4522 )
#endif
#endif

#ifdef HAVE_SHIBOKEN
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
# include <basewrapper.h>
# include <conversions.h>
# include <sbkmodule.h>
# include <typeresolver.h>
# include <shiboken.h>
# ifdef HAVE_PYSIDE
# include <pyside_qtcore_python.h>
# include <pyside_qtgui_python.h>
PyTypeObject** SbkPySide_QtCoreTypes=NULL;
PyTypeObject** SbkPySide_QtGuiTypes=NULL;
# endif
#endif

#include <CXX/Objects.hxx>
#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Base/QuantityPy.h>


#include "WidgetFactory.h"
#include "PrefWidgets.h"
#include "PropertyPage.h"


using namespace Gui;

#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
namespace Shiboken {
template<> struct Converter<Base::Quantity>
{
    static inline bool checkType(PyObject* pyObj) {
        return PyObject_TypeCheck(pyObj, &(Base::QuantityPy::Type));
    }
    static inline bool isConvertible(PyObject* pyObj) {
        return PyObject_TypeCheck(pyObj, &(Base::QuantityPy::Type));
    }
    static inline PyObject* toPython(void* cppobj) {
        return toPython(*reinterpret_cast<Base::Quantity*>(cppobj));
    }
    static inline PyObject* toPython(Base::Quantity cpx) {
        return new Base::QuantityPy(new Base::Quantity(cpx));
    }
    static inline Base::Quantity toCpp(PyObject* pyobj) {
        Base::Quantity q = *static_cast<Base::QuantityPy*>(pyobj)->getQuantityPtr();
        return q;
    }
};
}

PyObject* toPythonFuncQuantity(const void* cpp)
{
    return Shiboken::Converter<Base::Quantity>::toPython(const_cast<void*>(cpp));
}

void toCppPointerConvFuncQuantity(PyObject*,void*)
{
}

PythonToCppFunc toCppPointerCheckFuncQuantity(PyObject* obj)
{
    if (Shiboken::Converter<Base::Quantity>::isConvertible(obj))
        return toCppPointerConvFuncQuantity;
    else
        return 0;
}

void registerTypes()
{
    SbkConverter* convert = Shiboken::Conversions::createConverter(&Base::QuantityPy::Type, toPythonFuncQuantity);
    Shiboken::Conversions::setPythonToCppPointerFunctions(convert, toCppPointerConvFuncQuantity, toCppPointerCheckFuncQuantity);
    Shiboken::Conversions::registerConverterName(convert, "Base::Quantity");
}
#endif

// --------------------------------------------------------

PythonWrapper::PythonWrapper()
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
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
        str = PyString_AsString(unicode);
        Py_DECREF(unicode);
        return true;
    }
    else if (PyString_Check(pyobject.ptr())) {
        str = PyString_AsString(pyobject.ptr());
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
    PyTypeObject * type = Shiboken::SbkType<QObject>();
    if (type) {
        if (Shiboken::Object::checkType(pyobject.ptr())) {
            SbkObject* sbkobject = reinterpret_cast<SbkObject *>(pyobject.ptr());
            void* cppobject = Shiboken::Object::cppPointer(sbkobject, type);
            return reinterpret_cast<QObject*>(cppobject);
        }
    }
#else
    Py::Module mainmod(PyImport_AddModule((char*)"sip"));
    Py::Callable func = mainmod.getDict().getItem("unwrapinstance");
    Py::Tuple arguments(1);
    arguments[0] = pyobject; //PyQt pointer
    Py::Object result = func.apply(arguments);
    void* ptr = PyLong_AsVoidPtr(result.ptr());
    return reinterpret_cast<QObject*>(ptr);
#endif

    return 0;
}

Py::Object PythonWrapper::fromQIcon(const QIcon* icon)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyObject* pyobj = Shiboken::createWrapper<QIcon>(icon, true);
    if (pyobj)
        return Py::asObject(pyobj);
#endif
    throw Py::RuntimeError("Failed to wrap icon");
}

Py::Object PythonWrapper::fromQWidget(QWidget* widget, const char* className)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject * type = Shiboken::SbkType<QWidget>();
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
    Py::Module sipmod(PyImport_AddModule((char*)"sip"));
    Py::Callable func = sipmod.getDict().getItem("wrapinstance");
    Py::Tuple arguments(2);
    arguments[0] = Py::asObject(PyLong_FromVoidPtr(widget));
    Py::Module qtmod(PyImport_ImportModule((char*)"PyQt4.Qt"));
    arguments[1] = qtmod.getDict().getItem("QWidget");
    return func.apply(arguments);
#endif
}

bool PythonWrapper::loadCoreModule()
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
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
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
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

void PythonWrapper::createChildrenNameAttributes(PyObject* root, QObject* object)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    Q_FOREACH (QObject* child, object->children()) {
        const QByteArray name = child->objectName().toLocal8Bit();

        if (!name.isEmpty() && !name.startsWith("_") && !name.startsWith("qt_")) {
            bool hasAttr = PyObject_HasAttrString(root, name.constData());
            if (!hasAttr) {
                Shiboken::AutoDecRef pyChild(Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkPySide_QtCoreTypes[SBK_QOBJECT_IDX], child));
                PyObject_SetAttrString(root, name.constData(), pyChild);
            }
            createChildrenNameAttributes(root, child);
        }
        createChildrenNameAttributes(root, child);
    }
#endif
}

void PythonWrapper::setParent(PyObject* pyWdg, QObject* parent)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    if (parent) {
        Shiboken::AutoDecRef pyParent(Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkPySide_QtGuiTypes[SBK_QWIDGET_IDX], parent));
        Shiboken::Object::setParent(pyParent, pyWdg);
    }
#endif
}

// ----------------------------------------------------

Gui::WidgetFactoryInst* Gui::WidgetFactoryInst::_pcSingleton = NULL;

WidgetFactoryInst& WidgetFactoryInst::instance()
{
    if (_pcSingleton == 0L)
        _pcSingleton = new WidgetFactoryInst;
    return *_pcSingleton;
}

void WidgetFactoryInst::destruct ()
{
    if (_pcSingleton != 0)
        delete _pcSingleton;
    _pcSingleton = 0;
}

/**
 * Creates a widget with the name \a sName which is a child of \a parent.
 * To create an instance of this widget once it must has been registered. 
 * If there is no appropriate widget registered 0 is returned.
 */
QWidget* WidgetFactoryInst::createWidget (const char* sName, QWidget* parent) const
{
    QWidget* w = (QWidget*)Produce(sName);

    // this widget class is not registered
    if (!w) {
#ifdef FC_DEBUG
        Base::Console().Warning("\"%s\" is not registered\n", sName);
#else
        Base::Console().Log("\"%s\" is not registered\n", sName);
#endif
        return 0;
    }

    try {
#ifdef FC_DEBUG
        const char* cName = dynamic_cast<QWidget*>(w)->metaObject()->className();
        Base::Console().Log("Widget of type '%s' created.\n", cName);
#endif
    }
    catch (...) {
#ifdef FC_DEBUG
        Base::Console().Error("%s does not inherit from \"QWidget\"\n", sName);
#else
        Base::Console().Log("%s does not inherit from \"QWidget\"\n", sName);
#endif
        delete w;
        return 0;
    }

    // set the parent to the widget
    if (parent)
        w->setParent(parent);

    return w;
}

/**
 * Creates a widget with the name \a sName which is a child of \a parent.
 * To create an instance of this widget once it must has been registered. 
 * If there is no appropriate widget registered 0 is returned.
 */
Gui::Dialog::PreferencePage* WidgetFactoryInst::createPreferencePage (const char* sName, QWidget* parent) const
{
    Gui::Dialog::PreferencePage* w = (Gui::Dialog::PreferencePage*)Produce(sName);

    // this widget class is not registered
    if (!w) {
#ifdef FC_DEBUG
        Base::Console().Warning("\"%s\" is not registered\n", sName);
#else
        Base::Console().Log("\"%s\" is not registered\n", sName);
#endif
        return 0;
    }

    if (qobject_cast<Gui::Dialog::PreferencePage*>(w)) {
#ifdef FC_DEBUG
        Base::Console().Log("Preference page of type '%s' created.\n", w->metaObject()->className());
#endif
    }
    else {
#ifdef FC_DEBUG
        Base::Console().Error("%s does not inherit from 'Gui::Dialog::PreferencePage'\n", sName);
#endif
        delete w;
        return 0;
    }

    // set the parent to the widget
    if (parent)
        w->setParent(parent);

    return w;
}

/**
 * Creates a preference widget with the name \a sName and the preference name \a sPref 
 * which is a child of \a parent.
 * To create an instance of this widget once it must has been registered. 
 * If there is no appropriate widget registered 0 is returned.
 * After creation of this widget its recent preferences are restored automatically.
 */
QWidget* WidgetFactoryInst::createPrefWidget(const char* sName, QWidget* parent, const char* sPref)
{
    QWidget* w = createWidget(sName);
    // this widget class is not registered
    if (!w)
        return 0; // no valid QWidget object

    // set the parent to the widget
    w->setParent(parent);

    try {
        dynamic_cast<PrefWidget*>(w)->setEntryName(sPref);
        dynamic_cast<PrefWidget*>(w)->restorePreferences();
    }
    catch (...) {
#ifdef FC_DEBUG
        Base::Console().Error("%s does not inherit from \"PrefWidget\"\n", w->metaObject()->className());
#endif
        delete w;
        return 0;
    }

    return w;
}

// ----------------------------------------------------

PySideUicModule::PySideUicModule()
  : Py::ExtensionModule<PySideUicModule>("PySideUic")
{
    add_varargs_method("loadUiType",&PySideUicModule::loadUiType,
        "PySide lacks the \"loadUiType\" command, so we have to convert the ui file to py code in-memory first\n"
        "and then execute it in a special frame to retrieve the form_class.");
    add_varargs_method("loadUi",&PySideUicModule::loadUi,
        "Addition of \"loadUi\" to PySide.");
    initialize("PySideUic helper module"); // register with Python
}

Py::Object PySideUicModule::loadUiType(const Py::Tuple& args)
{
    Base::PyGILStateLocker lock;
    PyObject* main = PyImport_AddModule("__main__");
    PyObject* dict = PyModule_GetDict(main);
    Py::Dict d(PyDict_Copy(dict), true);
    Py::String uiFile(args.getItem(0));
    std::string file = uiFile.as_string();
    std::replace(file.begin(), file.end(), '\\', '/');

    QString cmd;
    QTextStream str(&cmd);
    // https://github.com/albop/dolo/blob/master/bin/load_ui.py
    str << "import pysideuic\n"
        << "from PySide import QtCore, QtGui\n"
        << "import xml.etree.ElementTree as xml\n"
        << "from cStringIO import StringIO\n"
        << "\n"
        << "uiFile = \"" << file.c_str() << "\"\n"
        << "parsed = xml.parse(uiFile)\n"
        << "widget_class = parsed.find('widget').get('class')\n"
        << "form_class = parsed.find('class').text\n"
        << "with open(uiFile, 'r') as f:\n"
        << "    o = StringIO()\n"
        << "    frame = {}\n"
        << "    pysideuic.compileUi(f, o, indent=0)\n"
        << "    pyc = compile(o.getvalue(), '<string>', 'exec')\n"
        << "    exec pyc in frame\n"
        << "    #Fetch the base_class and form class based on their type in the xml from designer\n"
        << "    form_class = frame['Ui_%s'%form_class]\n"
        << "    base_class = eval('QtGui.%s'%widget_class)\n";

    PyObject* result = PyRun_String((const char*)cmd.toLatin1(), Py_file_input, d.ptr(), d.ptr());
    if (result) {
        Py_DECREF(result);
        if (d.hasKey("form_class") && d.hasKey("base_class")) {
            Py::Tuple t(2);
            t.setItem(0, d.getItem("form_class"));
            t.setItem(1, d.getItem("base_class"));
            return t;
        }
    }
    else {
        throw Py::Exception();
    }

    return Py::None();
}

Py::Object PySideUicModule::loadUi(const Py::Tuple& args)
{
    Base::PyGILStateLocker lock;
    PyObject* main = PyImport_AddModule("__main__");
    PyObject* dict = PyModule_GetDict(main);
    Py::Dict d(PyDict_Copy(dict), true);
    d.setItem("uiFile_", args[0]);
    if (args.size() > 1)
        d.setItem("base_", args[1]);
    else
        d.setItem("base_", Py::None());

    QString cmd;
    QTextStream str(&cmd);
#if 0
    // https://github.com/lunaryorn/snippets/blob/master/qt4/designer/pyside_dynamic.py
    str << "from PySide import QtCore, QtGui, QtUiTools\n"
        << "import FreeCADGui"
        << "\n"
        << "class UiLoader(QtUiTools.QUiLoader):\n"
        << "    def __init__(self, baseinstance):\n"
        << "        QtUiTools.QUiLoader.__init__(self, baseinstance)\n"
        << "        self.baseinstance = baseinstance\n"
        << "        self.ui = FreeCADGui.UiLoader()\n"
        << "\n"
        << "    def createWidget(self, class_name, parent=None, name=''):\n"
        << "        if parent is None and self.baseinstance:\n"
        << "            return self.baseinstance\n"
        << "        else:\n"
        << "            widget = self.ui.createWidget(class_name, parent, name)\n"
        << "            if not widget:\n"
        << "                widget = QtUiTools.QUiLoader.createWidget(self, class_name, parent, name)\n"
        << "            if self.baseinstance:\n"
        << "                setattr(self.baseinstance, name, widget)\n"
        << "            return widget\n"
        << "\n"
        << "loader = UiLoader(globals()[\"base_\"])\n"
        << "widget = loader.load(globals()[\"uiFile_\"])\n"
        << "\n";
#else
    str << "from PySide import QtCore, QtGui\n"
        << "import FreeCADGui"
        << "\n"
        << "loader = FreeCADGui.UiLoader()\n"
        << "widget = loader.load(globals()[\"uiFile_\"])\n"
        << "\n";
#endif

    PyObject* result = PyRun_String((const char*)cmd.toLatin1(), Py_file_input, d.ptr(), d.ptr());
    if (result) {
        Py_DECREF(result);
        if (d.hasKey("widget")) {
            return d.getItem("widget");
        }
    }
    else {
        throw Py::Exception();
    }

    return Py::None();
}

// ----------------------------------------------------

UiLoader::UiLoader(QObject* parent)
  : QUiLoader(parent)
{
    // do not use the plugins for additional widgets as we don't need them and
    // the application may crash under Linux (tested on Ubuntu 7.04 & 7.10).
    clearPluginPaths();
    this->cw = availableWidgets();
}

UiLoader::~UiLoader()
{
}

QWidget* UiLoader::createWidget(const QString & className, QWidget * parent,
                                const QString& name)
{
    if (this->cw.contains(className))
        return QUiLoader::createWidget(className, parent, name);
    QWidget* w = 0;
    if (WidgetFactory().CanProduce((const char*)className.toLatin1()))
        w = WidgetFactory().createWidget((const char*)className.toLatin1(), parent);
    if (w) w->setObjectName(name);
    return w;
}

// ----------------------------------------------------

PyObject *UiLoaderPy::PyMake(struct _typeobject *type, PyObject * args, PyObject * kwds)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    return new UiLoaderPy();
}

void UiLoaderPy::init_type()
{
    behaviors().name("UiLoader");
    behaviors().doc("UiLoader to create widgets");
    behaviors().set_tp_new(PyMake);
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    add_varargs_method("load",&UiLoaderPy::load,"load(string, QWidget parent=None) -> QWidget\n"
                                                "load(QIODevice, QWidget parent=None) -> QWidget");
    add_varargs_method("createWidget",&UiLoaderPy::createWidget,"createWidget()");
}

UiLoaderPy::UiLoaderPy()
{
}

UiLoaderPy::~UiLoaderPy()
{
}

Py::Object UiLoaderPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "Ui loader";
    return Py::String(s_out.str());
}

Py::Object UiLoaderPy::load(const Py::Tuple& args)
{
    Gui::PythonWrapper wrap;
    if (wrap.loadCoreModule()) {
        std::string fn;
        QFile file;
        QIODevice* device = 0;
        QWidget* parent = 0;
        if (wrap.toCString(args[0], fn)) {
            file.setFileName(QString::fromUtf8(fn.c_str()));
            if (!file.open(QFile::ReadOnly))
                throw Py::RuntimeError("Cannot open file");
            device = &file;
        }
        else if (args[0].isString()) {
            fn = (std::string)Py::String(args[0]);
            file.setFileName(QString::fromUtf8(fn.c_str()));
            if (!file.open(QFile::ReadOnly))
                throw Py::RuntimeError("Cannot open file");
            device = &file;
        }
        else {
            QObject* obj = wrap.toQObject(args[0]);
            device = qobject_cast<QIODevice*>(obj);
        }

        if (args.size() > 1) {
            QObject* obj = wrap.toQObject(args[1]);
            parent = qobject_cast<QWidget*>(obj);
        }

        if (device) {
            QWidget* widget = loader.load(device, parent);
            if (widget) {
                wrap.loadGuiModule();

                Py::Object pyWdg = wrap.fromQWidget(widget);
                wrap.createChildrenNameAttributes(*pyWdg, widget);
                wrap.setParent(*pyWdg, parent);
                return pyWdg;
            }
        }
        else {
            throw Py::Exception("string or QIODevice expected");
        }
    }
    return Py::None();
}

Py::Object UiLoaderPy::createWidget(const Py::Tuple& args)
{
    Gui::PythonWrapper wrap;

    // 1st argument
    Py::String str(args[0]);
    std::string className;
    if (str.isUnicode()) {
        className = str.as_std_string("utf-8");
    }
    else {
        className = (std::string)str;
    }
    // 2nd argument
    QWidget* parent = 0;
    if (wrap.loadCoreModule() && args.size() > 1) {
        QObject* object = wrap.toQObject(args[1]);
        if (object)
            parent = qobject_cast<QWidget*>(object);
    }

    // 3rd argument
    std::string objectName;
    if (args.size() > 2) {
        Py::String str(args[2]);
        if (str.isUnicode()) {
            objectName = str.as_std_string("utf-8");
        }
        else {
            objectName = (std::string)str;
        }
    }

    QWidget* widget = loader.createWidget(QString::fromLatin1(className.c_str()), parent,
        QString::fromLatin1(objectName.c_str()));
    if (!widget) {
        std::string err = "No such widget class '";
        err += className;
        err += "'";
        throw Py::RuntimeError(err);
    }
    wrap.loadGuiModule();
    return wrap.fromQWidget(widget);
}

// ----------------------------------------------------

WidgetFactorySupplier* WidgetFactorySupplier::_pcSingleton = 0L;

WidgetFactorySupplier & WidgetFactorySupplier::instance()
{
    // not initialized?
    if (!_pcSingleton)
        _pcSingleton = new WidgetFactorySupplier;
    return *_pcSingleton;
}

void WidgetFactorySupplier::destruct()
{
    // delete the widget factory and all its producers first
    WidgetFactoryInst::destruct();
    delete _pcSingleton;
    _pcSingleton=0;
}

// ----------------------------------------------------

PrefPageUiProducer::PrefPageUiProducer (const char* filename, const char* group)
  : fn(QString::fromUtf8(filename))
{
    WidgetFactoryInst::instance().AddProducer(filename, this);
    Gui::Dialog::DlgPreferencesImp::addPage(filename, group);
}

PrefPageUiProducer::~PrefPageUiProducer()
{
}

void* PrefPageUiProducer::Produce () const
{
    QWidget* page = new Gui::Dialog::PreferenceUiForm(fn);
    return (void*)page;
}

// ----------------------------------------------------

PrefPagePyProducer::PrefPagePyProducer (const Py::Object& p, const char* group)
  : type(p)
{
    std::string str;
    Base::PyGILStateLocker lock;
    if (type.hasAttr("__name__")) {
        str = static_cast<std::string>(Py::String(type.getAttr("__name__")));
    }

    WidgetFactoryInst::instance().AddProducer(str.c_str(), this);
    Gui::Dialog::DlgPreferencesImp::addPage(str, group);
}

PrefPagePyProducer::~PrefPagePyProducer ()
{
    Base::PyGILStateLocker lock;
    type = Py::None();
}

void* PrefPagePyProducer::Produce () const
{
    Base::PyGILStateLocker lock;
    Py::Callable method(type);
    Py::Tuple args;
    Py::Object page = method.apply(args);
    return new Gui::Dialog::PreferencePagePython(page);
}

// ----------------------------------------------------

using namespace Gui::Dialog;

PreferencePagePython::PreferencePagePython(const Py::Object& p, QWidget* parent)
  : PreferencePage(parent), page(p)
{
    Base::PyGILStateLocker lock;
    if (page.hasAttr(std::string("form"))) {
        Py::Object widget(page.getAttr(std::string("form")));

        Gui::PythonWrapper wrap;
        if (wrap.loadCoreModule()) {
            QObject* object = wrap.toQObject(widget);
            if (object) {
                QWidget* form = qobject_cast<QWidget*>(object);
                if (form) {
                    this->setWindowTitle(form->windowTitle());
                    QVBoxLayout *layout = new QVBoxLayout;
                    layout->addWidget(form);
                    setLayout(layout);
                }
            }
        }
    }
}

PreferencePagePython::~PreferencePagePython()
{
    Base::PyGILStateLocker lock;
    page = Py::None();
}

void PreferencePagePython::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
}

void PreferencePagePython::loadSettings()
{
    Base::PyGILStateLocker lock;
    try {
        if (page.hasAttr(std::string("loadSettings"))) {
            Py::Callable method(page.getAttr(std::string("loadSettings")));
            Py::Tuple args;
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void PreferencePagePython::saveSettings()
{
    Base::PyGILStateLocker lock;
    try {
        if (page.hasAttr(std::string("saveSettings"))) {
            Py::Callable method(page.getAttr(std::string("saveSettings")));
            Py::Tuple args;
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

// ----------------------------------------------------

/* TRANSLATOR Gui::ContainerDialog */

/**
 *  Constructs a ContainerDialog which embeds the child \a templChild.
 *  The dialog will be modal.
 */
ContainerDialog::ContainerDialog( QWidget* templChild )
  : QDialog( QApplication::activeWindow())
{
    setModal(true);
    setWindowTitle( templChild->objectName() );
    setObjectName( templChild->objectName() );

    setSizeGripEnabled( true );
    MyDialogLayout = new QGridLayout(this);

    buttonOk = new QPushButton(this);
    buttonOk->setObjectName(QLatin1String("buttonOK"));
    buttonOk->setText( tr( "&OK" ) );
    buttonOk->setAutoDefault( true );
    buttonOk->setDefault( true );

    MyDialogLayout->addWidget( buttonOk, 1, 0 );
    QSpacerItem* spacer = new QSpacerItem( 210, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    MyDialogLayout->addItem( spacer, 1, 1 );

    buttonCancel = new QPushButton(this);
    buttonCancel->setObjectName(QLatin1String("buttonCancel"));
    buttonCancel->setText( tr( "&Cancel" ) );
    buttonCancel->setAutoDefault( true );

    MyDialogLayout->addWidget( buttonCancel, 1, 2 );

    templChild->setParent(this);

    MyDialogLayout->addWidget( templChild, 0, 0, 0, 2 );
    resize( QSize(307, 197).expandedTo(minimumSizeHint()) );

    // signals and slots connections
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

/** Destroys the object and frees any allocated resources */
ContainerDialog::~ContainerDialog()
{
}

// ----------------------------------------------------

void PyResource::init_type()
{
    behaviors().name("PyResource");
    behaviors().doc("PyResource");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    add_varargs_method("value",&PyResource::value);
    add_varargs_method("setValue",&PyResource::setValue);
    add_varargs_method("show",&PyResource::show);
    add_varargs_method("connect",&PyResource::connect);
}

PyResource::PyResource() : myDlg(0)
{
}

PyResource::~PyResource()
{
    delete myDlg;
    for (std::vector<SignalConnect*>::iterator it = mySingals.begin(); it != mySingals.end(); ++it) {
        SignalConnect* sc = *it;
        delete sc;
    }
}

/**
 * Loads an .ui file with the name \a name. If the .ui file cannot be found or the QWidgetFactory
 * cannot create an instance an exception is thrown. If the created resource does not inherit from
 * QDialog an instance of ContainerDialog is created to embed it.
 */
void PyResource::load(const char* name)
{
    QString fn = QString::fromUtf8(name);
    QFileInfo fi(fn);

    // checks whether it's a relative path
    if (fi.isRelative()) {
        QString cwd = QDir::currentPath ();
        QString home= QDir(QString::fromUtf8(App::GetApplication().getHomePath())).path();

        // search in cwd and home path for the file
        //
        // file does not reside in cwd, check home path now
        if (!fi.exists()) {
            if (cwd == home) {
                QString what = QObject::tr("Cannot find file %1").arg(fi.absoluteFilePath());
                throw Base::Exception(what.toUtf8().constData());
            }
            else {
                fi.setFile( QDir(home), fn );

                if (!fi.exists()) {
                    QString what = QObject::tr("Cannot find file %1 neither in %2 nor in %3")
                        .arg(fn).arg(cwd).arg(home);
                    throw Base::Exception(what.toUtf8().constData());
                }
                else {
                    fn = fi.absoluteFilePath(); // file resides in FreeCAD's home directory
                }
            }
        }
    }
    else {
        if (!fi.exists()) {
            QString what = QObject::tr("Cannot find file %1").arg(fn);
            throw Base::Exception(what.toUtf8().constData());
        }
    }

    QWidget* w=0;
    try {
        UiLoader loader;
#if QT_VERSION >= 0x040500
        loader.setLanguageChangeEnabled(true);
#endif
        QFile file(fn);
        if (file.open(QFile::ReadOnly))
            w = loader.load(&file, QApplication::activeWindow());
        file.close();
    }
    catch (...) {
        throw Base::Exception("Cannot create resource");
    }

    if (!w)
        throw Base::Exception("Invalid widget.");

    if (w->inherits("QDialog")) {
        myDlg = (QDialog*)w;
    }
    else {
        myDlg = new ContainerDialog(w);
    }
}

/**
 * Makes a connection between the sender widget \a sender and its signal \a signal
 * of the created resource and Python callback function \a cb.
 * If the sender widget does not exist or no resource has been loaded this method returns FALSE, 
 * otherwise it returns TRUE.
 */
bool PyResource::connect(const char* sender, const char* signal, PyObject* cb)
{
    if ( !myDlg )
        return false;

    QObject* objS=0L;
    QList<QWidget*> list = myDlg->findChildren<QWidget*>();
    QList<QWidget*>::const_iterator it = list.begin();
    QObject *obj;
    QString sigStr = QString::fromLatin1("2%1").arg(QString::fromLatin1(signal));

    while ( it != list.end() ) {
        obj = *it;
        ++it;
        if (obj->objectName() == QLatin1String(sender)) {
            objS = obj;
            break;
        }
    }

    if (objS) {
        SignalConnect* sc = new SignalConnect(this, cb);
        mySingals.push_back(sc);
        return QObject::connect(objS, sigStr.toLatin1(), sc, SLOT ( onExecute() )  );
    }
    else
        qWarning( "'%s' does not exist.\n", sender );

    return false;
}

Py::Object PyResource::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "Resource object";
    return Py::String(s_out.str());
}

/**
 * Searches for a widget and its value in the argument object \a args
 * to returns its value as Python object.
 * In the case it fails 0 is returned.
 */
Py::Object PyResource::value(const Py::Tuple& args)
{
    char *psName;
    char *psProperty;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &psName, &psProperty))
        throw Py::Exception();

    QVariant v;
    if (myDlg) {
        QList<QWidget*> list = myDlg->findChildren<QWidget*>();
        QList<QWidget*>::const_iterator it = list.begin();
        QObject *obj;

        bool fnd = false;
        while ( it != list.end() ) {
            obj = *it;
            ++it;
            if (obj->objectName() == QLatin1String(psName)) {
                fnd = true;
                v = obj->property(psProperty);
                break;
            }
        }

        if ( !fnd )
            qWarning( "'%s' not found.\n", psName );
    }

    Py::Object item = Py::None();
    switch (v.type())
    {
    case QVariant::StringList:
        {
            QStringList str = v.toStringList();
            int nSize = str.count();
            Py::List slist(nSize);
            for (int i=0; i<nSize;++i) {
                slist.setItem(i, Py::String(str[i].toLatin1()));
            }
            item = slist;
        }   break;
    case QVariant::ByteArray:
        break;
    case QVariant::String:
        item = Py::String(v.toString().toLatin1());
        break;
    case QVariant::Double:
        item = Py::Float(v.toDouble());
        break;
    case QVariant::Bool:
        item = Py::Boolean(v.toBool() ? 1 : 0);
        break;
    case QVariant::UInt:
        item = Py::Long(static_cast<unsigned long>(v.toUInt()));
        break;
    case QVariant::Int:
        item = Py::Int(v.toInt());
        break;
    default:
        item = Py::String("");
        break;
    }

    return item;
}

/**
 * Searches for a widget, its value name and the new value in the argument object \a args
 * to set even this new value.
 * In the case it fails 0 is returned.
 */
Py::Object PyResource::setValue(const Py::Tuple& args)
{
    char *psName;
    char *psProperty;
    PyObject *psValue;
    if (!PyArg_ParseTuple(args.ptr(), "ssO", &psName, &psProperty, &psValue))
        throw Py::Exception();

    QVariant v;
    if (PyString_Check(psValue)) {
        v = QString::fromLatin1(PyString_AsString(psValue));
    }
    else if (PyInt_Check(psValue)) {
        int val = PyInt_AsLong(psValue);
        v = val;
    }
    else if (PyLong_Check(psValue)) {
        unsigned int val = PyLong_AsLong(psValue);
        v = val;
    }
    else if (PyFloat_Check(psValue)) {
        v = PyFloat_AsDouble(psValue);
    }
    else if (PyList_Check(psValue)) {
        QStringList str;
        int nSize = PyList_Size(psValue);
        for (int i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(psValue, i);
            if (!PyString_Check(item))
                continue;

            char* pItem = PyString_AsString(item);
            str.append(QString::fromLatin1(pItem));
        }

        v = str;
    }
    else {
        throw Py::TypeError("Unsupported type");
    }

    if (myDlg) {
        QList<QWidget*> list = myDlg->findChildren<QWidget*>();
        QList<QWidget*>::const_iterator it = list.begin();
        QObject *obj;

        bool fnd = false;
        while ( it != list.end() ) {
            obj = *it;
            ++it;
            if (obj->objectName() == QLatin1String(psName)) {
                fnd = true;
                obj->setProperty(psProperty, v);
                break;
            }
        }

        if (!fnd)
            qWarning( "'%s' not found.\n", psName );
    }

    return Py::None();
}

/**
 * If any resouce has been loaded this methods shows it as a modal dialog.
 */
Py::Object PyResource::show(const Py::Tuple& args)
{
    if (myDlg) {
        // small trick to get focus
        myDlg->showMinimized();

#ifdef Q_WS_X11
        // On X11 this may not work. For further information see QWidget::showMaximized
        //
        // workaround for X11
        myDlg->hide();
        myDlg->show();
#endif

        myDlg->showNormal();
        myDlg->exec();
    }

    return Py::None();
}

/**
 * Searches for the sender, the signal and the callback function to connect with
 * in the argument object \a args. In the case it fails 0 is returned.
 */
Py::Object PyResource::connect(const Py::Tuple& args)
{
    char *psSender;
    char *psSignal;

    PyObject *temp;

    if (PyArg_ParseTuple(args.ptr(), "ssO", &psSender, &psSignal, &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            throw Py::Exception();
        }

        Py_XINCREF(temp);         /* Add a reference to new callback */
        std::string sSender = psSender;
        std::string sSignal = psSignal;

        if (!connect(psSender, psSignal, temp)) {
            // no signal object found => dispose the callback object
            Py_XDECREF(temp);  /* Dispose of callback */
        }

        return Py::None();
    }

    // error set by PyArg_ParseTuple
    throw Py::Exception();
}

// ----------------------------------------------------

SignalConnect::SignalConnect(PyObject* res, PyObject* cb)
  : myResource(res), myCallback(cb)
{
}

SignalConnect::~SignalConnect()
{
    Base::PyGILStateLocker lock;
    Py_XDECREF(myCallback);  /* Dispose of callback */
}

/**
 * Calls the callback function of the connected Python object.
 */
void SignalConnect::onExecute()
{
    PyObject *arglist;
    PyObject *result;

    /* Time to call the callback */
    arglist = Py_BuildValue("(O)", myResource);
    result = PyEval_CallObject(myCallback, arglist);
    Py_XDECREF(result);
    Py_DECREF(arglist);
}

#include "moc_WidgetFactory.cpp"
