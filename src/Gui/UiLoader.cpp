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
# include <QFile>
# include <QTextStream>
#endif

#include "UiLoader.h"
#include "WidgetFactory.h"
#include <Base/Interpreter.h>

using namespace Gui;


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
    str << "import pyside2uic\n"
        << "from PySide2 import QtCore, QtGui, QtWidgets\n"
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
    str << "from PySide2 import QtCore, QtGui, QtWidgets\n"
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

PyObject *UiLoaderPy::PyMake(struct _typeobject * /*type*/, PyObject * args, PyObject * /*kwds*/)
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
    Gui::PythonWrapper wrap;

    // 1st argument
    Py::String str(args[0]);
    std::string className;
    className = str.as_std_string("utf-8");
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
        objectName = str.as_std_string("utf-8");
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
    wrap.loadWidgetsModule();

    const char* typeName = wrap.getWrapperName(widget);
    return wrap.fromQWidget(widget, typeName);
}
