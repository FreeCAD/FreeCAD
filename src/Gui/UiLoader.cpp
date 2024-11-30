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
# include <QAction>
# include <QActionGroup>
# include <QCoreApplication>
# include <QDir>
# include <QFile>
# include <QLayout>
# include <QTextStream>
#endif

#include <functional>
#include <Base/Interpreter.h>

#include "UiLoader.h"
#include "PythonWrapper.h"
#include "WidgetFactory.h"


using namespace Gui;

namespace {

QWidget* createFromWidgetFactory(const QString & className, QWidget * parent, const QString& name)
{
    QWidget* widget = nullptr;
    if (WidgetFactory().CanProduce((const char*)className.toLatin1()))
        widget = WidgetFactory().createWidget((const char*)className.toLatin1(), parent);
    if (widget)
        widget->setObjectName(name);
    return widget;
}

Py::Object wrapFromWidgetFactory(const Py::Tuple& args, const std::function<QWidget*(const QString&, QWidget *, const QString&)> & callableFunc)
{
    Gui::PythonWrapper wrap;

    // 1st argument
    Py::String str(args[0]);
    std::string className;
    className = str.as_std_string("utf-8");
    // 2nd argument
    QWidget* parent = nullptr;
    if (wrap.loadCoreModule() && args.size() > 1) {
        QObject* object = wrap.toQObject(args[1]);
        if (object)
            parent = qobject_cast<QWidget*>(object);
    }

    // 3rd argument
    std::string objectName;
    if (args.size() > 2) {
        Py::String str(args[2]);
        objectName = str.as_std_string("utf-8");
    }

    QWidget* widget = callableFunc(QString::fromLatin1(className.c_str()), parent,
                                   QString::fromLatin1(objectName.c_str()));
    if (!widget) {
        return Py::None();
    //    std::string err = "No such widget class '";
    //    err += className;
    //    err += "'";
    //    throw Py::RuntimeError(err);
    }
    wrap.loadGuiModule();
    wrap.loadWidgetsModule();

    const char* typeName = wrap.getWrapperName(widget);
    return wrap.fromQWidget(widget, typeName);
}

}

PySideUicModule::PySideUicModule()
  : Py::ExtensionModule<PySideUicModule>("PySideUic")
{
    add_varargs_method("loadUiType",&PySideUicModule::loadUiType,
        "PySide lacks the \"loadUiType\" command, so we have to convert the ui file to py code in-memory first\n"
        "and then execute it in a special frame to retrieve the form_class.");
    add_varargs_method("loadUi",&PySideUicModule::loadUi,
        "Addition of \"loadUi\" to PySide.");
    add_varargs_method("createCustomWidget",&PySideUicModule::createCustomWidget,
        "Create custom widgets.");
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
    str << "import pyside2uic\n"
        << "from PySide import QtCore, QtGui, QtWidgets\n"
        << "import xml.etree.ElementTree as xml\n"
        << "try:\n"
        << "    from cStringIO import StringIO\n"
        << "except Exception:\n"
        << "    from io import StringIO\n"
        << "\n"
        << "uiFile = \"" << file.c_str() << "\"\n"
        << "parsed = xml.parse(uiFile)\n"
        << "widget_class = parsed.find('widget').get('class')\n"
        << "form_class = parsed.find('class').text\n"
        << "with open(uiFile, 'r') as f:\n"
        << "    o = StringIO()\n"
        << "    frame = {}\n"
        << "    pyside2uic.compileUi(f, o, indent=0)\n"
        << "    pyc = compile(o.getvalue(), '<string>', 'exec')\n"
        << "    exec(pyc, frame)\n"
        << "    #Fetch the base_class and form class based on their type in the xml from designer\n"
        << "    form_class = frame['Ui_%s'%form_class]\n"
        << "    base_class = eval('QtWidgets.%s'%widget_class)\n";

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

    str << "from PySide import QtCore, QtGui, QtWidgets\n"
        << "import FreeCADGui"
        << "\n"
        << "loader = FreeCADGui.UiLoader()\n"
        << "widget = loader.load(globals()[\"uiFile_\"])\n"
        << "\n";


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

Py::Object PySideUicModule::createCustomWidget(const Py::Tuple& args)
{
    return wrapFromWidgetFactory(args, &createFromWidgetFactory);
}

// ----------------------------------------------------

#if !defined (HAVE_QT_UI_TOOLS)
QUiLoader::QUiLoader(QObject* parent)
{
    Base::PyGILStateLocker lock;
    PythonWrapper wrap;
    wrap.loadUiToolsModule();
  //PyObject* module = PyImport_ImportModule("PySide2.QtUiTools");
    PyObject* module = PyImport_ImportModule("freecad.UiTools");
    if (module) {
        Py::Tuple args(1);
        args[0] = wrap.fromQObject(parent);
        Py::Module mod(module, true);
        uiloader = mod.callMemberFunction("QUiLoader", args);
    }
}

QUiLoader::~QUiLoader()
{
    Base::PyGILStateLocker lock;
    uiloader = Py::None();
}

QStringList QUiLoader::pluginPaths() const
{
    Base::PyGILStateLocker lock;
    try {
        Py::List list(uiloader.callMemberFunction("pluginPaths"));
        QStringList paths;
        for (const auto& it : list) {
            paths << QString::fromStdString(Py::String(it).as_std_string());
        }
        return paths;
    }
    catch (Py::Exception& e) {
        e.clear();
        return QStringList();
    }
}

void QUiLoader::clearPluginPaths()
{
    Base::PyGILStateLocker lock;
    try {
        uiloader.callMemberFunction("clearPluginPaths");
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

void QUiLoader::addPluginPath(const QString& path)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args[0] = Py::String(path.toStdString());
        uiloader.callMemberFunction("addPluginPath", args);
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

QWidget* QUiLoader::load(QIODevice* device, QWidget* parentWidget)
{
    Base::PyGILStateLocker lock;
    try {
        PythonWrapper wrap;
        Py::Tuple args(2);
        args[0] = wrap.fromQObject(device);
        args[1] = wrap.fromQObject(parentWidget);
        Py::Object form(uiloader.callMemberFunction("load", args));
        return qobject_cast<QWidget*>(wrap.toQObject(form));
    }
    catch (Py::Exception& e) {
        e.clear();
        return nullptr;
    }
}

QStringList QUiLoader::availableWidgets() const
{
    Base::PyGILStateLocker lock;
    try {
        Py::List list(uiloader.callMemberFunction("availableWidgets"));
        QStringList widgets;
        for (const auto& it : list) {
            widgets << QString::fromStdString(Py::String(it).as_std_string());
        }
        return widgets;
    }
    catch (Py::Exception& e) {
        e.clear();
        return QStringList();
    }
}

QStringList QUiLoader::availableLayouts() const
{
    Base::PyGILStateLocker lock;
    try {
        Py::List list(uiloader.callMemberFunction("availableLayouts"));
        QStringList layouts;
        for (const auto& it : list) {
            layouts << QString::fromStdString(Py::String(it).as_std_string());
        }
        return layouts;
    }
    catch (Py::Exception& e) {
        e.clear();
        return QStringList();
    }
}

QWidget* QUiLoader::createWidget(const QString& className, QWidget* parent, const QString& name)
{
    Base::PyGILStateLocker lock;
    try {
        PythonWrapper wrap;
        Py::Tuple args(3);
        args[0] = Py::String(className.toStdString());
        args[1] = wrap.fromQObject(parent);
        args[2] = Py::String(name.toStdString());
        Py::Object form(uiloader.callMemberFunction("createWidget", args));
        return qobject_cast<QWidget*>(wrap.toQObject(form));
    }
    catch (Py::Exception& e) {
        e.clear();
        return nullptr;
    }
}

QLayout* QUiLoader::createLayout(const QString& className, QObject* parent, const QString& name)
{
    Base::PyGILStateLocker lock;
    try {
        PythonWrapper wrap;
        Py::Tuple args(3);
        args[0] = Py::String(className.toStdString());
        args[1] = wrap.fromQObject(parent);
        args[2] = Py::String(name.toStdString());
        Py::Object form(uiloader.callMemberFunction("createLayout", args));
        return qobject_cast<QLayout*>(wrap.toQObject(form));
    }
    catch (Py::Exception& e) {
        e.clear();
        return nullptr;
    }
}

QActionGroup* QUiLoader::createActionGroup(QObject* parent, const QString& name)
{
    Base::PyGILStateLocker lock;
    try {
        PythonWrapper wrap;
        Py::Tuple args(2);
        args[0] = wrap.fromQObject(parent);
        args[1] = Py::String(name.toStdString());
        Py::Object action(uiloader.callMemberFunction("createActionGroup", args));
        return qobject_cast<QActionGroup*>(wrap.toQObject(action));
    }
    catch (Py::Exception& e) {
        e.clear();
        return nullptr;
    }
}

QAction* QUiLoader::createAction(QObject* parent, const QString& name)
{
    Base::PyGILStateLocker lock;
    try {
        PythonWrapper wrap;
        Py::Tuple args(2);
        args[0] = wrap.fromQObject(parent);
        args[1] = Py::String(name.toStdString());
        Py::Object action(uiloader.callMemberFunction("createAction", args));
        return qobject_cast<QAction*>(wrap.toQObject(action));
    }
    catch (Py::Exception& e) {
        e.clear();
        return nullptr;
    }
}

void QUiLoader::setWorkingDirectory(const QDir& dir)
{
    Base::PyGILStateLocker lock;
    try {
        PythonWrapper wrap;
        Py::Tuple args(1);
        args[0] = wrap.fromQDir(dir);
        uiloader.callMemberFunction("setWorkingDirectory", args);
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

QDir QUiLoader::workingDirectory() const
{
    Base::PyGILStateLocker lock;
    try {
        PythonWrapper wrap;
        Py::Object dir((uiloader.callMemberFunction("workingDirectory")));
        QDir* d = wrap.toQDir(dir.ptr());
        if (d)
            return *d;
        return QDir::current();
    }
    catch (Py::Exception& e) {
        e.clear();
        return QDir::current();
    }
}

void QUiLoader::setLanguageChangeEnabled(bool enabled)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args[0] = Py::Boolean(enabled);
        uiloader.callMemberFunction("setLanguageChangeEnabled", args);
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

bool QUiLoader::isLanguageChangeEnabled() const
{
    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok((uiloader.callMemberFunction("isLanguageChangeEnabled")));
        return static_cast<bool>(ok);
    }
    catch (Py::Exception& e) {
        e.clear();
        return false;
    }
}

void QUiLoader::setTranslationEnabled(bool enabled)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args[0] = Py::Boolean(enabled);
        uiloader.callMemberFunction("setTranslationEnabled", args);
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

bool QUiLoader::isTranslationEnabled() const
{
    Base::PyGILStateLocker lock;
    try {
        Py::Boolean ok((uiloader.callMemberFunction("isTranslationEnabled")));
        return static_cast<bool>(ok);
    }
    catch (Py::Exception& e) {
        e.clear();
        return false;
    }
}

QString QUiLoader::errorString() const
{
    Base::PyGILStateLocker lock;
    try {
        Py::String error((uiloader.callMemberFunction("errorString")));
        return QString::fromStdString(error.as_std_string());
    }
    catch (Py::Exception& e) {
        e.clear();
        return QString();
    }
}
#endif

// ----------------------------------------------------

UiLoader::UiLoader(QObject* parent)
  : QUiLoader(parent)
{
    this->cw = availableWidgets();
    setLanguageChangeEnabled(true);
}

std::unique_ptr<UiLoader> UiLoader::newInstance(QObject *parent)
{
    QCoreApplication* app = QCoreApplication::instance();
    QStringList libPaths = app->libraryPaths();

    app->setLibraryPaths(QStringList{}); //< backup library paths, so QUiLoader won't load plugins by default
    std::unique_ptr<UiLoader> rv{new UiLoader{parent}};
    app->setLibraryPaths(libPaths);

    return rv;
}

UiLoader::~UiLoader() = default;

QWidget* UiLoader::createWidget(const QString & className, QWidget * parent,
                                const QString& name)
{
    if (this->cw.contains(className))
        return QUiLoader::createWidget(className, parent, name);

    return createFromWidgetFactory(className, parent, name);
}

// ----------------------------------------------------

PyObject *UiLoaderPy::PyMake(struct _typeobject * /*type*/, PyObject * args, PyObject * /*kwds*/)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
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

    add_varargs_method("availableWidgets",&UiLoaderPy::availableWidgets,"availableWidgets()");
    add_varargs_method("clearPluginPaths",&UiLoaderPy::clearPluginPaths,"clearPluginPaths()");
    add_varargs_method("pluginPaths",&UiLoaderPy::pluginPaths,"pluginPaths()");
    add_varargs_method("addPluginPath",&UiLoaderPy::addPluginPath,"addPluginPath()");
    add_varargs_method("errorString",&UiLoaderPy::errorString,"errorString()");
    add_varargs_method("isLanguageChangeEnabled",&UiLoaderPy::isLanguageChangeEnabled,
                       "isLanguageChangeEnabled()");
    add_varargs_method("setLanguageChangeEnabled",&UiLoaderPy::setLanguageChangeEnabled,
                       "setLanguageChangeEnabled()");
    add_varargs_method("setWorkingDirectory",&UiLoaderPy::setWorkingDirectory,
                       "setWorkingDirectory()");
    add_varargs_method("workingDirectory",&UiLoaderPy::workingDirectory,"workingDirectory()");
}

UiLoaderPy::UiLoaderPy()
    : loader{UiLoader::newInstance()}
{
}

UiLoaderPy::~UiLoaderPy() = default;

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
        QIODevice* device = nullptr;
        QWidget* parent = nullptr;
        if (wrap.toCString(args[0], fn)) {
            file.setFileName(QString::fromUtf8(fn.c_str()));
            if (!file.open(QFile::ReadOnly))
                throw Py::RuntimeError("Cannot open file");
            device = &file;
        }
        else if (args[0].isString()) {
            fn = static_cast<std::string>(Py::String(args[0]));
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
            QWidget* widget = loader->load(device, parent);
            if (widget) {
                wrap.loadGuiModule();
                wrap.loadWidgetsModule();

                const char* typeName = wrap.getWrapperName(widget);
                Py::Object pyWdg = wrap.fromQWidget(widget, typeName);
                wrap.createChildrenNameAttributes(*pyWdg, widget);
                wrap.setParent(*pyWdg, parent);
                return pyWdg;
            }
        }
        else {
            throw Py::TypeError("string or QIODevice expected");
        }
    }
    return Py::None();
}

Py::Object UiLoaderPy::createWidget(const Py::Tuple& args)
{
    //NOLINTBEGIN
    return wrapFromWidgetFactory(args, std::bind(&UiLoader::createWidget, loader.get(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3));
    //NOLINTEND
}

Py::Object UiLoaderPy::addPluginPath(const Py::Tuple& args)
{
    Gui::PythonWrapper wrap;
    if (wrap.loadCoreModule()) {
        std::string fn;
        if (wrap.toCString(args[0], fn)) {
            loader->addPluginPath(QString::fromStdString(fn));
        }
    }
    return Py::None();
}

Py::Object UiLoaderPy::clearPluginPaths(const Py::Tuple& /*args*/)
{
    loader->clearPluginPaths();
    return Py::None();
}

Py::Object UiLoaderPy::pluginPaths(const Py::Tuple& /*args*/)
{
    auto list = loader->pluginPaths();
    Py::List py;
    for (const auto& it : list) {
        py.append(Py::String(it.toStdString()));
    }
    return py;
}

Py::Object UiLoaderPy::availableWidgets(const Py::Tuple& /*args*/)
{
    auto list = loader->availableWidgets();
    Py::List py;
    for (const auto& it : list) {
        py.append(Py::String(it.toStdString()));
    }

    auto producer = WidgetFactory().CanProduce();
    for (const auto& it : producer) {
        py.append(Py::String(it));
    }

    return py;
}

Py::Object UiLoaderPy::errorString(const Py::Tuple& /*args*/)
{
    return Py::String(loader->errorString().toStdString());
}

Py::Object UiLoaderPy::isLanguageChangeEnabled(const Py::Tuple& /*args*/)
{
    return Py::Boolean(loader->isLanguageChangeEnabled());
}

Py::Object UiLoaderPy::setLanguageChangeEnabled(const Py::Tuple& args)
{
    loader->setLanguageChangeEnabled(Py::Boolean(args[0]));
    return Py::None();
}

Py::Object UiLoaderPy::setWorkingDirectory(const Py::Tuple& args)
{
    Gui::PythonWrapper wrap;
    if (wrap.loadCoreModule()) {
        std::string fn;
        if (wrap.toCString(args[0], fn)) {
            loader->setWorkingDirectory(QString::fromStdString(fn));
        }
    }
    return Py::None();
}

Py::Object UiLoaderPy::workingDirectory(const Py::Tuple& /*args*/)
{
    QDir dir = loader->workingDirectory();
    QString path = dir.absolutePath();
    return Py::String(path.toStdString());
}

#if !defined (HAVE_QT_UI_TOOLS)
# include "moc_UiLoader.cpp"
#endif
