/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <FCConfig.h>

#if HAVE_CONFIG_H
#   include <config.h>
#endif // HAVE_CONFIG_H

#ifdef _MSC_VER
#   pragma warning(disable : 4005)
#endif

#include <QApplication>
#include <QIcon>
#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_WS_X11)
#include <QX11EmbedWidget>
#endif
#include <thread>

// FreeCAD Base header
#include <App/Application.h>
#include <Base/Exception.h>
#include <Base/Factory.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Gui/SoFCDB.h>
#include <Gui/Quarter/Quarter.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/nodekits/SoNodeKit.h>


static bool _isSetupWithoutGui = false;

static
QWidget* setupMainWindow();

class QtApplication : public QApplication {
public:
    QtApplication(int &argc, char **argv)
        : QApplication(argc, argv) {
    }
    bool notify (QObject * receiver, QEvent * event) override {
        try {
            return QApplication::notify(receiver, event);
        }
        catch (const Base::SystemExitException& e) {
            exit(e.getExitCode());
            return true;
        }
    }
};

#if defined(Q_OS_WIN)
HHOOK hhook;

LRESULT CALLBACK
FilterProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (qApp)
        qApp->sendPostedEvents(0, -1); // special DeferredDelete
    return CallNextHookEx(hhook, nCode, wParam, lParam);
}
#endif

static PyObject *
FreeCADGui_showMainWindow(PyObject * /*self*/, PyObject *args)
{
    if (_isSetupWithoutGui) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot call showMainWindow() after calling setupWithoutGUI()\n");
        return nullptr;
    }

    PyObject* inThread = Py_False;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &inThread))
        return nullptr;

    static bool thr = false;
    if (!qApp) {
        if (Base::asBoolean(inThread) && !thr) {
            thr = true;
            std::thread t([]() {
                static int argc = 0;
                static char **argv = {nullptr};
                QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
                // This only works well if the QApplication is the very first created instance
                // of a QObject. Otherwise the application lives in a different thread than the
                // main thread which will cause hazardous behaviour.
                QtApplication app(argc, argv);
                if (setupMainWindow()) {
                    app.exec();
                }
            });
            t.detach();
        }
        else {
            // In order to get Jupiter notebook integration working we must create a direct instance
            // of QApplication. Not even a sub-class can be used because otherwise PySide2 wraps it
            // with a QtCore.QCoreApplication which will raise an exception in ipykernel
#if defined(Q_OS_WIN)
            static int argc = 0;
            static char **argv = {0};
            (void)new QApplication(argc, argv);
            // When QApplication is constructed
            hhook = SetWindowsHookEx(WH_GETMESSAGE,
                FilterProc, 0, GetCurrentThreadId());
#elif !defined(QT_NO_GLIB)
            static int argc = 0;
            static char **argv = {nullptr};
            (void)new QApplication(argc, argv);
#else
            PyErr_SetString(PyExc_RuntimeError, "Must construct a QApplication before a QPaintDevice\n");
            return NULL;
#endif
        }
    }
    else if (!qobject_cast<QApplication*>(qApp)) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot create widget when no GUI is being used\n");
        return nullptr;
    }

    if (!thr) {
        if (!setupMainWindow()) {
            PyErr_SetString(PyExc_RuntimeError, "Cannot create main window\n");
            return nullptr;
        }
    }

    // if successful then enable Console logger
    Base::ILogger *console = Base::Console().Get("Console");
    if (console) {
        console->bMsg = true;
        console->bWrn = true;
        console->bErr = true;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FreeCADGui_exec_loop(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    if (!qApp) {
        PyErr_SetString(PyExc_RuntimeError, "Must construct a QApplication before a QPaintDevice\n");
        return nullptr;
    }
    else if (!qobject_cast<QApplication*>(qApp)) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot create widget when no GUI is being used\n");
        return nullptr;
    }

    qApp->exec();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FreeCADGui_setupWithoutGUI(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    if (!Gui::Application::Instance) {
        static Gui::Application *app = new Gui::Application(false);
        _isSetupWithoutGui = true;
        Q_UNUSED(app);
    }
    if (!SoDB::isInitialized()) {
        // init the Inventor subsystem
        SoDB::init();
        SoNodeKit::init();
        SoInteraction::init();
    }
    if (!Gui::SoFCDB::isInitialized()) {
        Gui::SoFCDB::init();
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FreeCADGui_embedToWindow(PyObject * /*self*/, PyObject *args)
{
    char* pointer;
    if (!PyArg_ParseTuple(args, "s", &pointer))
        return nullptr;

    QWidget* widget = Gui::getMainWindow();
    if (!widget) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "No main window");
        return nullptr;
    }

    std::string pointer_str = pointer;
    std::stringstream str(pointer_str);

#if defined(Q_OS_WIN)
    void* window = 0;
    str >> window;
    HWND winid = (HWND)window;

    LONG oldLong = GetWindowLong(winid, GWL_STYLE);
    SetWindowLong(winid, GWL_STYLE,
    oldLong | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    //SetWindowLong(widget->winId(), GWL_STYLE,
    //    WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    SetParent((HWND)widget->winId(), winid);

    QEvent embeddingEvent(QEvent::EmbeddingControl);
    QApplication::sendEvent(widget, &embeddingEvent);
#elif defined(Q_WS_X11)
    WId winid;
    str >> winid;

    QX11EmbedWidget* x11 = new QX11EmbedWidget();
    widget->setParent(x11);
    x11->embedInto(winid);
    x11->show();
#else
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented for this platform");
    return nullptr;
#endif

    Py_INCREF(Py_None);
    return Py_None;
}

struct PyMethodDef FreeCADGui_methods[] = {
    {"showMainWindow",FreeCADGui_showMainWindow,METH_VARARGS,
     "showMainWindow() -- Show the main window\n"
     "If no main window does exist one gets created"},
    {"exec_loop",FreeCADGui_exec_loop,METH_VARARGS,
     "exec_loop() -- Starts the event loop\n"
     "Note: this will block the call until the event loop has terminated"},
    {"setupWithoutGUI",FreeCADGui_setupWithoutGUI,METH_VARARGS,
     "setupWithoutGUI() -- Uses this module without starting\n"
     "an event loop or showing up any GUI\n"},
    {"embedToWindow",FreeCADGui_embedToWindow,METH_VARARGS,
     "embedToWindow() -- Embeds the main window into another window\n"},
    {nullptr, nullptr, 0, nullptr}  /* sentinel */
};

static
QWidget* setupMainWindow()
{
    if (!Gui::Application::Instance) {
        static Gui::Application *app = new Gui::Application(true);
        Q_UNUSED(app);
    }

    if (!Gui::MainWindow::getInstance()) {
        static bool hasMainWindow = false;
        if (hasMainWindow) {
            // if a main window existed and has been deleted it's not supported
            // to re-create it
            return nullptr;
        }

        Base::PyGILStateLocker lock;
        // It's sufficient to create the config key
        App::Application::Config()["DontOverrideStdIn"] = "";
        Gui::MainWindow *mw = new Gui::MainWindow();
        hasMainWindow = true;

        QIcon icon = qApp->windowIcon();
        if (icon.isNull()) {
            qApp->setWindowIcon(Gui::BitmapFactory().pixmap(App::Application::Config()["AppIcon"].c_str()));
        }
        mw->setWindowIcon(qApp->windowIcon());
        QString appName = qApp->applicationName();
        if (!appName.isEmpty()) {
            mw->setWindowTitle(appName);
        }
        else {
            mw->setWindowTitle(QString::fromLatin1(App::Application::Config()["ExeName"].c_str()));
        }

        // set toolbar icon size
        ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("General");
        int size = hGrp->GetInt("ToolbarIconSize", 0);
        if (size >= 16) // must not be lower than this
            mw->setIconSize(QSize(size,size));

        if (!SoDB::isInitialized()) {
            // init the Inventor subsystem
            SoDB::init();
            SIM::Coin3D::Quarter::Quarter::init();
            Gui::SoFCDB::init();
        }

        static bool init = false;
        if (!init) {
            try {
                Base::Console().Log("Run Gui init script\n");
                Base::Interpreter().runString(Base::ScriptFactory().ProduceScript("FreeCADGuiInit"));
            }
            catch (const Base::Exception& e) {
                PyErr_Format(Base::PyExc_FC_GeneralError, "Error in FreeCADGuiInit.py: %s\n", e.what());
                return nullptr;
            }
            init = true;
        }

        qApp->setActiveWindow(mw);

        // Activate the correct workbench
        std::string start = App::Application::Config()["StartWorkbench"];
        Base::Console().Log("Init: Activating default workbench %s\n", start.c_str());
        std::string autoload = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                               GetASCII("AutoloadModule", start.c_str());
        if ("$LastModule" == autoload) {
            start = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                                   GetASCII("LastModule", start.c_str());
        } else {
            start = autoload;
        }
        // if the auto workbench is not visible then force to use the default workbech
        // and replace the wrong entry in the parameters
        QStringList wb = Gui::Application::Instance->workbenches();
        if (!wb.contains(QString::fromLatin1(start.c_str()))) {
            start = App::Application::Config()["StartWorkbench"];
            if ("$LastModule" == autoload) {
                App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                                      SetASCII("LastModule", start.c_str());
            } else {
                App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                                      SetASCII("AutoloadModule", start.c_str());
            }
        }

        Gui::Application::Instance->activateWorkbench(start.c_str());
        mw->loadWindowSettings();
    }
    else {
        Gui::getMainWindow()->show();
    }

    return Gui::getMainWindow();
}

PyMOD_INIT_FUNC(FreeCADGui)
{
    try {
        Base::Interpreter().loadModule("FreeCAD");
        App::Application::Config()["AppIcon"] = "freecad";
        App::Application::Config()["SplashScreen"] = "freecadsplash";
        App::Application::Config()["CopyrightInfo"] = "\xc2\xa9 Juergen Riegel, Werner Mayer, Yorik van Havre and others 2001-2023\n";
        App::Application::Config()["LicenseInfo"] = "FreeCAD is free and open-source software licensed under the terms of LGPL2+ license.\n";
        App::Application::Config()["CreditsInfo"] = "FreeCAD wouldn't be possible without FreeCAD community.\n";
        // it's possible that the GUI is already initialized when the Gui version of the executable
        // is started in command mode
        if (Base::Type::fromName("Gui::BaseView").isBad())
            Gui::Application::initApplication();
        static struct PyModuleDef FreeCADGuiModuleDef = {
            PyModuleDef_HEAD_INIT,
            "FreeCADGui", "FreeCAD GUI module\n", -1,
            FreeCADGui_methods,
            nullptr, nullptr, nullptr, nullptr
        };
        PyObject* module = PyModule_Create(&FreeCADGuiModuleDef);
        return module;
    }
    catch (const Base::Exception& e) {
        PyErr_Format(PyExc_ImportError, "%s\n", e.what());
    }
    catch (...) {
        PyErr_SetString(PyExc_ImportError, "Unknown runtime error occurred");
    }
    return nullptr;
}

