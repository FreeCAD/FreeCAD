/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QIcon>
#include <QUrl>
#include <string>
#endif

#ifdef Q_OS_WIN32
#include <Windows.h>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/MainWindow.h>

#include "BrowserView.h"
#include "Workbench.h"


// use a different name to CreateCommand()
void CreateWebCommands();

void loadWebResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Web);
    Q_INIT_RESOURCE(Web_translation);
    Gui::Translator::instance()->refresh();
}

namespace WebGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("WebGui")
    {
        add_varargs_method("openBrowser", &Module::openBrowser);
        add_varargs_method("openBrowserHTML", &Module::openBrowserHTML);
        add_varargs_method("openBrowserWindow", &Module::openBrowserWindow);
        add_varargs_method("open",
                           &Module::openBrowser,
                           "open(htmlcode,baseurl,[title,iconpath])\n"
                           "Load a local (X)HTML file.");
        add_varargs_method("insert",
                           &Module::openBrowser,
                           "insert(string)\n"
                           "Load a local (X)HTML file.");
        initialize("This module is the WebGui module.");  // register with Python
    }

private:
    Py::Object openBrowser(const Py::Tuple& args)
    {
        const char* url;
        if (!PyArg_ParseTuple(args.ptr(), "s", &url)) {
            throw Py::Exception();
        }

        WebGui::BrowserView* pcBrowserView;

        pcBrowserView = new WebGui::BrowserView(Gui::getMainWindow());
        pcBrowserView->setWindowTitle(QObject::tr("Browser"));
        pcBrowserView->resize(400, 300);
        pcBrowserView->load(url);
        Gui::getMainWindow()->addWindow(pcBrowserView);
        if (!Gui::getMainWindow()->activeWindow()) {
            Gui::getMainWindow()->setActiveWindow(pcBrowserView);
        }

        return Py::None();
    }

    Py::Object openBrowserHTML(const Py::Tuple& args)
    {
        const char* HtmlCode;
        const char* BaseUrl;
        const char* IconPath;
        char* TabName = nullptr;
        if (!PyArg_ParseTuple(args.ptr(),
                              "ss|ets",
                              &HtmlCode,
                              &BaseUrl,
                              "utf-8",
                              &TabName,
                              &IconPath)) {
            throw Py::Exception();
        }

        std::string EncodedName = "Browser";
        if (TabName) {
            EncodedName = std::string(TabName);
            PyMem_Free(TabName);
        }

        WebGui::BrowserView* pcBrowserView = nullptr;
        pcBrowserView = new WebGui::BrowserView(Gui::getMainWindow());
        pcBrowserView->resize(400, 300);
        pcBrowserView->setHtml(QString::fromUtf8(HtmlCode), QUrl(QString::fromLatin1(BaseUrl)));
        pcBrowserView->setWindowTitle(QString::fromUtf8(EncodedName.c_str()));
        if (IconPath) {
            pcBrowserView->setWindowIcon(QIcon(QString::fromUtf8(IconPath)));
        }
        Gui::getMainWindow()->addWindow(pcBrowserView);
        if (!Gui::getMainWindow()->activeWindow()) {
            Gui::getMainWindow()->setActiveWindow(pcBrowserView);
        }

        return Py::None();
    }

    Py::Object openBrowserWindow(const Py::Tuple& args)
    {
        char* TabName = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "|et", "utf-8", &TabName)) {
            throw Py::Exception();
        }

        std::string EncodedName = "Browser";
        if (TabName) {
            EncodedName = std::string(TabName);
            PyMem_Free(TabName);
        }

        WebGui::BrowserView* pcBrowserView = nullptr;
        pcBrowserView = new WebGui::BrowserView(Gui::getMainWindow());
        pcBrowserView->resize(400, 300);
        pcBrowserView->setWindowTitle(QString::fromUtf8(EncodedName.c_str()));
        Gui::getMainWindow()->addWindow(pcBrowserView);
        if (!Gui::getMainWindow()->activeWindow()) {
            Gui::getMainWindow()->setActiveWindow(pcBrowserView);
        }

        return Py::asObject(pcBrowserView->getPyObject());
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

class NativeEventFilter: public QAbstractNativeEventFilter
{
public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override
#else
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override
#endif
    {
        Q_UNUSED(eventType)
        Q_UNUSED(result)

        // Fixes bug #9364: WM_INPUTLANGCHANGEREQUEST seems to freeze FreeCAD
#ifdef Q_OS_WIN32
        MSG* msg = (MSG*)(message);
        if (msg->message == WM_INPUTLANGCHANGEREQUEST) {
            Base::Console().Log("Ignore WM_INPUTLANGCHANGEREQUEST\n");
            return true;
        }
#else
        Q_UNUSED(message)
#endif
        return false;
    }
};

}  // namespace WebGui


/* Python entry */
PyMOD_INIT_FUNC(WebGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    PyObject* mod = WebGui::initModule();
    Base::Console().Log("Loading GUI of Web module... done\n");

    // instantiating the commands
    CreateWebCommands();
    WebGui::BrowserView::init();
    WebGui::Workbench::init();

#ifdef Q_OS_WIN32
    qApp->installNativeEventFilter(new WebGui::NativeEventFilter);
#endif

    // add resources and reloads the translators
    loadWebResource();

    PyMOD_Return(mod);
}
