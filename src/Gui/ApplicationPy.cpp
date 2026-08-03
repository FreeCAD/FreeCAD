// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2005 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <QApplication>
#include <QDir>
#include <QPrinter>
#include <QFileInfo>
#include <map>
#include <QIcon>
#if defined(Q_OS_WIN)
# include <windows.h>
#elif defined(Q_WS_X11)
# include <QX11EmbedWidget>
#endif
#include <Inventor/SoInput.h>
#include <Inventor/SoPath.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/nodekits/SoNodeKit.h>
#include <Inventor/nodes/SoSeparator.h>
#include <xercesc/util/TranscodingException.hpp>
#include <xercesc/util/XMLString.hpp>

#include <boost/regex.hpp>
#include <thread>

#include <App/Application.h>
#include <App/DocumentObjectPy.h>
#include <App/DocumentPy.h>
#include <App/PropertyFile.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <CXX/Objects.hxx>

#include <Gui/PreferencePages/DlgSettingsPDF.h>

#include "Application.h"
#include "ApplicationPy.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Dialogs/DlgPreferencesImp.h"
#include "Document.h"
#include "DocumentObserverPython.h"
#include "DownloadManager.h"
#include "EditorView.h"
#include "FileHandler.h"
#include "Macro.h"
#include "MainWindow.h"
#include "MainWindowPy.h"
#include "PythonEditor.h"
#include "PythonWrapper.h"
#include "SoFCDB.h"
#include "SplitView3DInventor.h"
#include "StartupProcess.h"
#include "View3DInventor.h"
#include "ViewProvider.h"
#include "WaitCursor.h"
#include "WidgetFactory.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "WorkbenchManipulatorPython.h"
#include "Inventor/MarkerBitmaps.h"
#include "Language/Translator.h"
#include "Selection/SoFCUnifiedSelection.h"


using namespace Gui;

namespace
{
void requirePythonMainThread(const char* api)
{
    try {
        Gui::requireMainThread(api);
    }
    catch (const Base::Exception& exception) {
        throw Py::RuntimeError(exception.what());
    }
}

struct CoinActionTarget
{
    SoNode* node {nullptr};
    SoPath* path {nullptr};
};

std::string pythonStringToStdString(PyObject* value)
{
    if (PyUnicode_Check(value)) {
        const char* utf8 = PyUnicode_AsUTF8(value);
        if (!utf8) {
            throw Py::Exception();
        }
        return utf8;
    }

    if (PyBytes_Check(value)) {
        char* data = nullptr;
        Py_ssize_t size = 0;
        if (PyBytes_AsStringAndSize(value, &data, &size) != 0) {
            throw Py::Exception();
        }
        return std::string(data, static_cast<std::size_t>(size));
    }

    throw Py::TypeError("color override keys must be strings");
}

Base::Color pythonToColor(PyObject* value)
{
    Base::Color color;
    if (PyTuple_Check(value) && (PyTuple_Size(value) == 3 || PyTuple_Size(value) == 4)) {
        PyObject* item = PyTuple_GetItem(value, 0);
        if (PyFloat_Check(item)) {
            color.r = static_cast<float>(PyFloat_AsDouble(item));
            item = PyTuple_GetItem(value, 1);
            if (!PyFloat_Check(item)) {
                throw Py::TypeError("color tuples must use consistent float components");
            }
            color.g = static_cast<float>(PyFloat_AsDouble(item));
            item = PyTuple_GetItem(value, 2);
            if (!PyFloat_Check(item)) {
                throw Py::TypeError("color tuples must use consistent float components");
            }
            color.b = static_cast<float>(PyFloat_AsDouble(item));
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value, 3);
                if (!PyFloat_Check(item)) {
                    throw Py::TypeError("color tuples must use consistent float components");
                }
                color.a = static_cast<float>(PyFloat_AsDouble(item));
            }
            return color;
        }

        if (PyLong_Check(item)) {
            color.r = static_cast<float>(PyLong_AsLong(item)) / 255.0F;
            item = PyTuple_GetItem(value, 1);
            if (!PyLong_Check(item)) {
                throw Py::TypeError("color tuples must use consistent integer components");
            }
            color.g = static_cast<float>(PyLong_AsLong(item)) / 255.0F;
            item = PyTuple_GetItem(value, 2);
            if (!PyLong_Check(item)) {
                throw Py::TypeError("color tuples must use consistent integer components");
            }
            color.b = static_cast<float>(PyLong_AsLong(item)) / 255.0F;
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value, 3);
                if (!PyLong_Check(item)) {
                    throw Py::TypeError("color tuples must use consistent integer components");
                }
                color.a = static_cast<float>(PyLong_AsLong(item)) / 255.0F;
            }
            return color;
        }

        throw Py::TypeError("color tuples must contain either floats or integers");
    }

    if (PyLong_Check(value)) {
        color.setPackedValue(PyLong_AsUnsignedLong(value));
        return color;
    }

    throw Py::TypeError("colors must be packed integers or RGB/RGBA tuples");
}

std::map<std::string, Base::Color> pythonToColorOverrideMap(PyObject* value)
{
    if (!PyDict_Check(value)) {
        throw Py::TypeError("colors must be a dict mapping element names to colors");
    }

    std::map<std::string, Base::Color> colors;
    PyObject* key = nullptr;
    PyObject* item = nullptr;
    Py_ssize_t pos = 0;
    while (PyDict_Next(value, &pos, &key, &item)) {
        colors.emplace(pythonStringToStdString(key), pythonToColor(item));
    }
    return colors;
}

CoinActionTarget pythonToCoinActionTarget(PyObject* proxy)
{
    CoinActionTarget target;
    void* ptr = nullptr;

    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoPath *", proxy, &ptr, 0);
    }
    catch (const Base::Exception&) {
        ptr = nullptr;
    }
    if (ptr) {
        target.path = static_cast<SoPath*>(ptr);
        return target;
    }

    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "SoNode *", proxy, &ptr, 0);
    }
    catch (const Base::Exception&) {
        ptr = nullptr;
    }
    if (ptr) {
        target.node = static_cast<SoNode*>(ptr);
        return target;
    }

    throw Py::TypeError("target must be of type coin.SoNode or coin.SoPath");
}

void applyElementColorOverrideAction(
    const CoinActionTarget& target,
    std::map<std::string, Base::Color> colors
)
{
    SoSelectionElementAction action(SoSelectionElementAction::Color, true);
    action.swapColors(colors);
    if (target.path) {
        action.apply(target.path);
    }
    else if (target.node) {
        action.apply(target.node);
    }
    else {
        throw Py::TypeError("target must be of type coin.SoNode or coin.SoPath");
    }
}
}  // namespace

static bool _isSetupWithoutGui = false;

static QWidget* setupMainWindow();

class QtApplication: public QApplication
{
public:
    QtApplication(int& argc, char** argv)
        : QApplication(argc, argv)
    {}
    bool notify(QObject* receiver, QEvent* event) override
    {
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

LRESULT CALLBACK FilterProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (qApp) {
        qApp->sendPostedEvents(0, -1);  // special DeferredDelete
    }
    return CallNextHookEx(hhook, nCode, wParam, lParam);
}
#endif

PyObject* Gui::ApplicationPy::sShowMainWindow(PyObject* /*self*/, PyObject* args)
{
    if (_isSetupWithoutGui) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Cannot call showMainWindow() after calling setupWithoutGUI()\n"
        );
        return nullptr;
    }

    PyObject* inThread = Py_False;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &inThread)) {
        return nullptr;
    }

    static bool thr = false;
    if (!qApp) {
        if (Base::asBoolean(inThread) && !thr) {
            thr = true;
            std::thread t([]() {
                static int argc = 0;
                static char** argv = {nullptr};
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
            static char** argv = {0};
            (void)new QApplication(argc, argv);
            // When QApplication is constructed
            hhook = SetWindowsHookEx(WH_GETMESSAGE, FilterProc, 0, GetCurrentThreadId());
#elif !defined(QT_NO_GLIB)
            static int argc = 0;
            static char** argv = {nullptr};
            (void)new QApplication(argc, argv);
#else
            PyErr_SetString(PyExc_RuntimeError, "Must construct a QApplication before a QPaintDevice\n");
            return nullptr;
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
    Base::ILogger* console = Base::Console().get("Console");
    if (console) {
        console->bMsg = true;
        console->bWrn = true;
        console->bErr = true;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Gui::ApplicationPy::sExec_loop(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

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

PyObject* Gui::ApplicationPy::sSetupWithoutGUI(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    if (!Gui::Application::Instance) {
        static Gui::Application* app = new Gui::Application(false);
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

PyObject* Gui::ApplicationPy::sEmbedToWindow(PyObject* /*self*/, PyObject* args)
{
    char* pointer;
    if (!PyArg_ParseTuple(args, "s", &pointer)) {
        return nullptr;
    }

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
    SetWindowLong(winid, GWL_STYLE, oldLong | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    // SetWindowLong(widget->winId(), GWL_STYLE,
    //     WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
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

static QWidget* setupMainWindow()
{
    if (!Gui::Application::Instance) {
        static Gui::Application* app = new Gui::Application(true);
        Q_UNUSED(app);
    }

    if (!Gui::MainWindow::getInstance()) {
        static bool hasMainWindow = false;
        if (hasMainWindow) {
            // if a main window existed and has been deleted it's not supported
            // to re-create it
            return nullptr;
        }

        Gui::StartupProcess process;
        process.execute();

        Base::PyGILStateLocker lock;
        // It's sufficient to create the config key
        App::Application::Config()["DontOverrideStdIn"] = "";
        Gui::MainWindow* mw = new Gui::MainWindow();
        hasMainWindow = true;

        QIcon icon = qApp->windowIcon();
        if (icon.isNull()) {
            qApp->setWindowIcon(
                Gui::BitmapFactory().pixmap(App::Application::Config()["AppIcon"].c_str())
            );
        }
        mw->setWindowIcon(qApp->windowIcon());

        try {
            Gui::StartupPostProcess postProcess(mw, *Gui::Application::Instance, qApp);
            postProcess.setLoadFromPythonModule(true);
            postProcess.execute();
        }
        catch (const Base::Exception&) {
            return nullptr;
        }
    }
    else {
        Gui::getMainWindow()->show();
    }

    return Gui::getMainWindow();
}

PyObject* Gui::ApplicationPy::sEditDocument(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.editDocument");

    Document* pcDoc = Application::Instance->editDocument();
    if (pcDoc) {
        return pcDoc->getPyObject();
    }

    Py_Return;
}

PyObject* Gui::ApplicationPy::sActiveDocument(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.activeDocument");

    Document* pcDoc = Application::Instance->activeDocument();
    if (pcDoc) {
        return pcDoc->getPyObject();
    }

    Py_Return;
}

PyObject* Gui::ApplicationPy::sActiveView(PyObject* /*self*/, PyObject* args)
{
    const char* typeName = nullptr;
    if (!PyArg_ParseTuple(args, "|s", &typeName)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.activeView");

    PY_TRY
    {
        Base::Type type;
        if (typeName) {
            type = Base::Type::fromName(typeName);
            if (type.isBad()) {
                PyErr_Format(PyExc_TypeError, "Invalid type '%s'", typeName);
                return nullptr;
            }
        }

        Gui::MDIView* mdiView = Application::Instance->activeView();
        if (mdiView && (type.isBad() || mdiView->isDerivedFrom(type))) {
            auto res = Py::asObject(mdiView->getPyObject());
            if (!res.isNone() || !type.isBad()) {
                return Py::new_reference_to(res);
            }
        }

        if (type.isBad()) {
            type = Gui::View3DInventor::getClassTypeId();
        }
        Application::Instance->activateView(type, true);
        mdiView = Application::Instance->activeView();
        if (mdiView) {
            return mdiView->getPyObject();
        }

        Py_Return;
    }
    PY_CATCH
}

PyObject* Gui::ApplicationPy::sActivateView(PyObject* /*self*/, PyObject* args)
{
    char* typeStr = nullptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    PyObject* create = Py_False;
    if (!PyArg_ParseTuple(args, "sO!", &typeStr, &PyBool_Type, &create)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.activateView");

    Base::Type type = Base::Type::fromName(typeStr);
    Application::Instance->activateView(type, Base::asBoolean(create));

    Py_Return;
}

PyObject* Gui::ApplicationPy::sSetActiveDocument(PyObject* /*self*/, PyObject* args)
{
    Document* pcDoc = nullptr;

    do {
        char* pstr = nullptr;
        if (PyArg_ParseTuple(args, "s", &pstr)) {
            pcDoc = Application::Instance->getDocument(pstr);
            if (!pcDoc) {
                PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
                return nullptr;
            }
            break;
        }

        PyErr_Clear();
        PyObject* doc = nullptr;
        if (PyArg_ParseTuple(args, "O!", &(App::DocumentPy::Type), &doc)) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            pcDoc = Application::Instance->getDocument(
                static_cast<App::DocumentPy*>(doc)->getDocumentPtr()
            );
            if (!pcDoc) {
                PyErr_Format(PyExc_KeyError, "Unknown document instance");
                return nullptr;
            }
            break;
        }
    } while (false);

    if (!pcDoc) {
        PyErr_SetString(PyExc_TypeError, "Either string or App.Document expected");
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.setActiveDocument");

    if (Application::Instance->activeDocument() != pcDoc) {
        Gui::MDIView* view = pcDoc->getActiveView();
        getMainWindow()->setActiveWindow(view);
    }

    Py_Return;
}

PyObject* ApplicationPy::sGetDocument(PyObject* /*self*/, PyObject* args)
{
    requirePythonMainThread("FreeCADGui.getDocument");

    if (!Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "FreeCADGui is not initialized");
        return nullptr;
    }

    char* pstr = nullptr;
    if (PyArg_ParseTuple(args, "s", &pstr)) {
        Document* pcDoc = Application::Instance->getDocument(pstr);
        if (!pcDoc) {
            PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
            return nullptr;
        }
        return pcDoc->getPyObject();
    }

    PyErr_Clear();
    PyObject* doc = nullptr;
    if (PyArg_ParseTuple(args, "O!", &(App::DocumentPy::Type), &doc)) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        Document* pcDoc = Application::Instance->getDocument(
            static_cast<App::DocumentPy*>(doc)->getDocumentPtr()
        );
        if (!pcDoc) {
            PyErr_Format(PyExc_KeyError, "Unknown document instance");
            return nullptr;
        }
        return pcDoc->getPyObject();
    }

    PyErr_SetString(PyExc_TypeError, "Either string or App.Document expected");
    return nullptr;
}

PyObject* ApplicationPy::sHide(PyObject* /*self*/, PyObject* args)
{
    char* psFeatStr = nullptr;
    if (!PyArg_ParseTuple(args, "s;Name of the object to hide has to be given!", &psFeatStr)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.hide");

    Document* pcDoc = Application::Instance->activeDocument();

    if (pcDoc) {
        pcDoc->setHide(psFeatStr);
    }

    Py_Return;
}

PyObject* ApplicationPy::sShow(PyObject* /*self*/, PyObject* args)
{
    char* psFeatStr = nullptr;
    if (!PyArg_ParseTuple(args, "s;Name of the object to show has to be given!", &psFeatStr)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.show");

    Document* pcDoc = Application::Instance->activeDocument();

    if (pcDoc) {
        pcDoc->setShow(psFeatStr);
    }

    Py_Return;
}

PyObject* ApplicationPy::sHideObject(PyObject* /*self*/, PyObject* args)
{
    PyObject* object = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &object)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.hideObject");

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    Application::Instance->hideViewProvider(obj);

    Py_Return;
}

PyObject* ApplicationPy::sShowObject(PyObject* /*self*/, PyObject* args)
{
    PyObject* object = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &object)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.showObject");

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    Application::Instance->showViewProvider(obj);

    Py_Return;
}

PyObject* ApplicationPy::sOpen(PyObject* /*self*/, PyObject* args)
{
    // only used to open Python files
    char* Name = nullptr;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &Name)) {
        return nullptr;
    }

    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);
    PY_TRY
    {
        QString fileName = QString::fromUtf8(Utf8Name.c_str());
        FileHandler handler(fileName);
        if (!handler.openFile()) {
            QString ext = handler.extension();
            Base::Console().error("File type '%s' not supported\n", ext.toLatin1().constData());
        }
    }
    PY_CATCH;

    Py_Return;
}

PyObject* ApplicationPy::sInsert(PyObject* /*self*/, PyObject* args)
{
    char* Name = nullptr;
    char* DocName = nullptr;
    if (!PyArg_ParseTuple(args, "et|s", "utf-8", &Name, &DocName)) {
        return nullptr;
    }

    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);

    PY_TRY
    {
        QString fileName = QString::fromUtf8(Utf8Name.c_str());
        FileHandler handler(fileName);
        if (!handler.importFile(std::string(DocName ? DocName : ""))) {
            QString ext = handler.extension();
            Base::Console().error("File type '%s' not supported\n", ext.toLatin1().constData());
        }
    }
    PY_CATCH;

    Py_Return;
}

PyObject* ApplicationPy::sExport(PyObject* /*self*/, PyObject* args)
{
    PyObject* object = nullptr;
    char* Name = nullptr;
    if (!PyArg_ParseTuple(args, "Oet", &object, "utf-8", &Name)) {
        return nullptr;
    }

    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);

    PY_TRY
    {
        App::Document* doc = nullptr;
        Py::Sequence list(object);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                App::DocumentObject* obj
                    = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                doc = obj->getDocument();
                break;
            }
        }

        QString fileName = QString::fromUtf8(Utf8Name.c_str());
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.suffix().toLower();
        if (ext == QLatin1String("iv") || ext == QLatin1String("wrl") || ext == QLatin1String("vrml")
            || ext == QLatin1String("wrz") || ext == QLatin1String("x3d")
            || ext == QLatin1String("x3dz") || ext == QLatin1String("xhtml")) {

            // build up the graph
            auto sep = new SoSeparator();
            sep->ref();

            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                    App::DocumentObject* obj
                        = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();

                    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
                    if (vp) {
                        sep->addChild(vp->getRoot());
                    }
                }
            }


            SoGetPrimitiveCountAction action;
            action.setCanApproximate(true);
            action.apply(sep);

            constexpr const int triangleLimit = 100000;
            constexpr const int pointLimit = 30000;
            constexpr const int lineLimit = 10000;

            bool binary = false;
            if (action.getTriangleCount() > triangleLimit || action.getPointCount() > pointLimit
                || action.getLineCount() > lineLimit) {
                binary = true;
            }

            SoFCDB::writeToFile(sep, Utf8Name.c_str(), binary);
            sep->unref();
        }
        else if (ext == QLatin1String("pdf")) {
            // get the view that belongs to the found document
            Gui::Document* gui_doc = Application::Instance->getDocument(doc);
            if (gui_doc) {
                Gui::MDIView* view = gui_doc->getActiveView();
                if (view) {
                    auto view3d = qobject_cast<View3DInventor*>(view);
                    if (view3d) {
                        view3d->viewAll();
                    }
                    QPrinter printer(QPrinter::ScreenResolution);
                    // setPdfVersion sets the printed PDF Version to what is chosen in
                    // Preferences/Import-Export/PDF more details under:
                    // https://www.kdab.com/creating-pdfa-documents-qt/
                    printer.setPdfVersion(Gui::Dialog::DlgSettingsPDF::evaluatePDFVersion());
                    printer.setOutputFormat(QPrinter::PdfFormat);
                    printer.setOutputFileName(fileName);
                    printer.setCreator(QString::fromStdString(App::Application::getNameWithVersion()));
                    view->print(&printer);
                }
            }
        }
        else {
            Base::Console().error("File type '%s' not supported\n", ext.toLatin1().constData());
        }
    }
    PY_CATCH;

    Py_Return;
}

PyObject* ApplicationPy::sSendMsgToActiveView(PyObject* /*self*/, PyObject* args)
{
    char* psCommandStr = nullptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    PyObject* suppress = Py_False;
    if (!PyArg_ParseTuple(args, "s|O!", &psCommandStr, &PyBool_Type, &suppress)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.SendMsgToActiveView");

    if (!Application::Instance->sendMsgToActiveView(psCommandStr)) {
        if (!Base::asBoolean(suppress)) {
            Base::Console().warning("Unknown view command: %s\n", psCommandStr);
        }
    }

    Py_Return;
}

PyObject* ApplicationPy::sSendMsgToFocusView(PyObject* /*self*/, PyObject* args)
{
    char* psCommandStr = nullptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    PyObject* suppress = Py_False;
    if (!PyArg_ParseTuple(args, "s|O!", &psCommandStr, &PyBool_Type, &suppress)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.SendMsgToFocusView");

    if (!Application::Instance->sendMsgToFocusView(psCommandStr)) {
        if (!Base::asBoolean(suppress)) {
            Base::Console().warning("Unknown view command: %s\n", psCommandStr);
        }
    }

    Py_Return;
}

PyObject* ApplicationPy::sGetMainWindow(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.getMainWindow");

    try {
        return Py::new_reference_to(MainWindowPy::createWrapper(Gui::getMainWindow()));
    }
    catch (const Py::Exception&) {
        return nullptr;
    }
}

PyObject* ApplicationPy::sUpdateGui(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.updateGui");

    qApp->processEvents();

    Py_Return;
}

PyObject* ApplicationPy::sUpdateLocale(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Translator::instance()->refresh();

    Py_Return;
}

PyObject* ApplicationPy::sGetLocale(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    std::string locale = Translator::instance()->activeLanguage();
    return PyUnicode_FromString(locale.c_str());
}

PyObject* ApplicationPy::sSetLocale(PyObject* /*self*/, PyObject* args)
{
    char* name = nullptr;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    std::string cname(name);
    TStringMap map = Translator::instance()->supportedLocales();
    map["English"] = "en";
    for (const auto& it : map) {
        if (it.first == cname || it.second == cname) {
            Translator::instance()->activateLanguage(it.first.c_str());
            break;
        }
    }

    Py_Return;
}

PyObject* ApplicationPy::sSupportedLocales(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    TStringMap map = Translator::instance()->supportedLocales();
    Py::Dict dict;
    dict.setItem(Py::String("English"), Py::String("en"));
    for (const auto& it : map) {
        Py::String key(it.first);
        Py::String val(it.second);
        dict.setItem(key, val);
    }
    return Py::new_reference_to(dict);
}

PyObject* ApplicationPy::sCreateDialog(PyObject* /*self*/, PyObject* args)
{
    char* fn = nullptr;
    if (!PyArg_ParseTuple(args, "s", &fn)) {
        return nullptr;
    }

    PyObject* pPyResource = nullptr;
    try {
        pPyResource = new PyResource();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        static_cast<PyResource*>(pPyResource)->load(fn);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_AssertionError, e.what());
        return nullptr;
    }

    return pPyResource;
}

PyObject* ApplicationPy::sAddPreferencePage(PyObject* /*self*/, PyObject* args)
{
    char* fn = nullptr;
    char* grp = nullptr;
    if (PyArg_ParseTuple(args, "ss", &fn, &grp)) {
        QFileInfo fi(QString::fromUtf8(fn));
        if (!fi.exists()) {
            PyErr_SetString(PyExc_RuntimeError, "UI file does not exist");
            return nullptr;
        }

        // add to the preferences dialog
        new PrefPageUiProducer(fn, grp);
        Py_Return;
    }

    PyErr_Clear();
    PyObject* dlg = nullptr;
    // new style classes
    if (PyArg_ParseTuple(args, "O!s", &PyType_Type, &dlg, &grp)) {
        // add to the preferences dialog
        new PrefPagePyProducer(Py::Object(dlg), grp);
        Py_Return;
    }

    return nullptr;
}

PyObject* ApplicationPy::sActivateWorkbench(PyObject* /*self*/, PyObject* args)
{
    char* psKey = nullptr;
    if (!PyArg_ParseTuple(args, "s", &psKey)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.activateWorkbench");

    // search for workbench handler from the dictionary
    PyObject* pcWorkbench = PyDict_GetItemString(Application::Instance->_pcWorkbenchDictionary, psKey);
    if (!pcWorkbench) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", psKey);
        return nullptr;
    }

    try {
        bool ok = Application::Instance->activateWorkbench(psKey);
        return Py::new_reference_to(Py::Boolean(ok));
    }
    catch (const Base::Exception& e) {
        std::stringstream err;
        err << psKey << ": " << e.what();
        PyErr_SetString(e.getPyExceptionType(), err.str().c_str());
        return nullptr;
    }
    catch (const XERCES_CPP_NAMESPACE::TranscodingException& e) {
        std::stringstream err;
        char* pMsg = XERCES_CPP_NAMESPACE::XMLString::transcode(e.getMessage());
        err << "Transcoding exception in Xerces-c:\n\n"
            << "Transcoding exception raised in activateWorkbench.\n"
            << "Check if your user configuration file is valid.\n"
            << "  Exception message:" << pMsg;
        XERCES_CPP_NAMESPACE::XMLString::release(&pMsg);
        PyErr_SetString(PyExc_RuntimeError, err.str().c_str());
        return nullptr;
    }
    catch (...) {
        std::stringstream err;
        err << "Unknown C++ exception raised in activateWorkbench('" << psKey << "')";
        PyErr_SetString(Base::PyExc_FC_GeneralError, err.str().c_str());
        return nullptr;
    }
}

PyObject* ApplicationPy::sAddWorkbench(PyObject* /*self*/, PyObject* args)
{
    PyObject* pcObject = nullptr;
    if (!PyArg_ParseTuple(args, "O", &pcObject)) {
        return nullptr;
    }

    try {
        // get the class object 'Workbench' from the main module that is expected
        // to be base class for all workbench classes
        Py::Module module("__main__");
        Py::Object baseclass(module.getAttr(std::string("Workbench")));

        // check whether it is an instance or class object
        Py::Object object(pcObject);
        Py::String name;

        if (PyObject_IsSubclass(object.ptr(), baseclass.ptr()) == 1) {
            // create an instance of this class
            name = object.getAttr(std::string("__name__"));
            Py::Tuple arg;
            Py::Callable creation(object);
            object = creation.apply(arg);
        }
        else if (PyObject_IsInstance(object.ptr(), baseclass.ptr()) == 1) {
            // extract the class name of the instance
            PyErr_Clear();  // PyObject_IsSubclass set an exception
            Py::Object classobj = object.getAttr(std::string("__class__"));
            name = classobj.getAttr(std::string("__name__"));
        }
        else {
            PyErr_SetString(
                PyExc_TypeError,
                "arg must be a subclass or an instance of "
                "a subclass of 'Workbench'"
            );
            return nullptr;
        }

        // NOLINTBEGIN(bugprone-unused-raii)
        // Search for some methods and members without invoking them
        Py::Callable(object.getAttr(std::string("Initialize")));
        Py::Callable(object.getAttr(std::string("GetClassName")));
        // NOLINTEND(bugprone-unused-raii)

        std::string item = name.as_std_string("ascii");
        PyObject* wb
            = PyDict_GetItemString(Application::Instance->_pcWorkbenchDictionary, item.c_str());
        if (wb) {
            PyErr_Format(PyExc_KeyError, "'%s' already exists.", item.c_str());
            return nullptr;
        }

        PyDict_SetItemString(Application::Instance->_pcWorkbenchDictionary, item.c_str(), object.ptr());
        Application::Instance->signalRefreshWorkbenches();
    }
    catch (const Py::Exception&) {
        return nullptr;
    }

    Py_Return;
}

PyObject* ApplicationPy::sRemoveWorkbench(PyObject* /*self*/, PyObject* args)
{
    char* psKey = nullptr;
    if (!PyArg_ParseTuple(args, "s", &psKey)) {
        return nullptr;
    }

    PyObject* wb = PyDict_GetItemString(Application::Instance->_pcWorkbenchDictionary, psKey);
    if (!wb) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", psKey);
        return nullptr;
    }

    WorkbenchManager::instance()->removeWorkbench(psKey);
    PyDict_DelItemString(Application::Instance->_pcWorkbenchDictionary, psKey);
    Application::Instance->signalRefreshWorkbenches();

    Py_Return;
}

PyObject* ApplicationPy::sGetWorkbench(PyObject* /*self*/, PyObject* args)
{
    char* psKey = nullptr;
    if (!PyArg_ParseTuple(args, "s", &psKey)) {
        return nullptr;
    }

    // get the python workbench object from the dictionary
    PyObject* pcWorkbench = PyDict_GetItemString(Application::Instance->_pcWorkbenchDictionary, psKey);
    if (!pcWorkbench) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", psKey);
        return nullptr;
    }

    Py_INCREF(pcWorkbench);
    return pcWorkbench;
}

PyObject* ApplicationPy::sListWorkbenches(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py_INCREF(Application::Instance->_pcWorkbenchDictionary);
    return Application::Instance->_pcWorkbenchDictionary;
}

PyObject* ApplicationPy::sActiveWorkbench(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Workbench* actWb = WorkbenchManager::instance()->active();
    if (!actWb) {
        PyErr_SetString(PyExc_AssertionError, "No active workbench\n");
        return nullptr;
    }

    // get the python workbench object from the dictionary
    std::string key = actWb->name();
    PyObject* pcWorkbench
        = PyDict_GetItemString(Application::Instance->_pcWorkbenchDictionary, key.c_str());
    if (!pcWorkbench) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", key.c_str());
        return nullptr;
    }

    // object get incremented
    Py_INCREF(pcWorkbench);
    return pcWorkbench;
}

PyObject* ApplicationPy::sAddResourcePath(PyObject* /*self*/, PyObject* args)
{
    char* filePath = nullptr;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath)) {
        return nullptr;
    }

    QString path = QString::fromUtf8(filePath);
    PyMem_Free(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromStdString(App::Application::getHomePath());
        path = home + path;
    }

    BitmapFactory().addPath(path);
    Translator::instance()->addPath(path);

    Py_Return;
}

PyObject* ApplicationPy::sAddLanguagePath(PyObject* /*self*/, PyObject* args)
{
    char* filePath = nullptr;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath)) {
        return nullptr;
    }

    QString path = QString::fromUtf8(filePath);
    PyMem_Free(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromStdString(App::Application::getHomePath());
        path = home + path;
    }

    Translator::instance()->addPath(path);

    Py_Return;
}

PyObject* ApplicationPy::sAddIconPath(PyObject* /*self*/, PyObject* args)
{
    char* filePath = nullptr;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath)) {
        return nullptr;
    }

    QString path = QString::fromUtf8(filePath);
    PyMem_Free(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromStdString(App::Application::getHomePath());
        path = home + path;
    }

    BitmapFactory().addPath(path);

    Py_Return;
}

PyObject* ApplicationPy::sAddIcon(PyObject* /*self*/, PyObject* args)
{
    const char* iconName = nullptr;
    Py_buffer content;
    const char* format = "XPM";
    if (!PyArg_ParseTuple(args, "ss*|s", &iconName, &content, &format)) {
        return nullptr;
    }

    QPixmap icon;
    if (BitmapFactory().findPixmapInCache(iconName, icon)) {
        PyErr_SetString(PyExc_AssertionError, "Icon with this name already registered");
        PyBuffer_Release(&content);
        return nullptr;
    }

    const char* contentStr = static_cast<const char*>(content.buf);
    QByteArray ary(contentStr, static_cast<int>(content.len));
    icon.loadFromData(ary, format);

    if (icon.isNull()) {
        QString file = QString::fromUtf8(contentStr, static_cast<int>(content.len));
        icon.load(file);
    }

    PyBuffer_Release(&content);

    if (icon.isNull()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Invalid icon added to application");
        return nullptr;
    }

    BitmapFactory().addPixmapToCache(iconName, icon);

    Py_Return;
}

PyObject* ApplicationPy::sGetIcon(PyObject* /*self*/, PyObject* args)
{
    char* iconName = nullptr;
    if (!PyArg_ParseTuple(args, "s", &iconName)) {
        return nullptr;
    }

    PythonWrapper wrap;
    wrap.loadGuiModule();
    wrap.loadWidgetsModule();
    auto pixmap = BitmapFactory().pixmap(iconName);
    if (!pixmap.isNull()) {
        return Py::new_reference_to(wrap.fromQIcon(new QIcon(pixmap)));
    }

    Py_Return;
}

PyObject* ApplicationPy::sIsIconCached(PyObject* /*self*/, PyObject* args)
{
    char* iconName = nullptr;
    if (!PyArg_ParseTuple(args, "s", &iconName)) {
        return nullptr;
    }

    QPixmap icon;

    return Py::new_reference_to(Py::Boolean(BitmapFactory().findPixmapInCache(iconName, icon)));
}

PyObject* ApplicationPy::sAddCommand(PyObject* /*self*/, PyObject* args)
{
    char* pName = nullptr;
    char* pSource = nullptr;
    PyObject* pcCmdObj = nullptr;
    if (!PyArg_ParseTuple(args, "sO|s", &pName, &pcCmdObj, &pSource)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.addCommand");

    // get the call stack to find the Python module name
    //
    std::string module;
    std::string group;
    try {
        Base::PyGILStateLocker lock;

        // Get the filename of the running code by using the low-level sys._getframe() method.
        // We use this instead of the `inspect` module (which may actually cause imports to execute
        // and can result in a circular import if sAddCommand is being called as part of an import
        // statement itself), and the `traceback` module (which cannot access the filename of code
        // that is being run through the C interface).

        Py::Module sysMod(PyImport_ImportModule("sys"), /*owned=*/true);
        Py::Callable getFrame(sysMod.getAttr("_getframe"));

        Py::Object callerFrame;
        Py::Tuple getFrameArgs(1);
        getFrameArgs[0] = Py::Long(0);
        callerFrame = getFrame.apply(getFrameArgs);

        Py::Object codeObj(callerFrame.getAttr("f_code"));

        Py::Object filenameObj(codeObj.getAttr("co_filename"));
        std::string filename(Py::String(filenameObj).as_std_string());

        Base::FileInfo fi(filename);
        // convert backslashes to slashes
        filename = fi.filePath();
        module = fi.fileNamePure();
        // for the group name get the directory name after 'Mod'
        boost::regex rx("/Mod/(\\w+)/");
        boost::smatch what;
        if (boost::regex_search(filename, what, rx)) {
            group = what[1];
        }
        else {
            rx = "/Ext/freecad/(\\w+)/";
            if (boost::regex_search(filename, what, rx)) {
                group = what[1];
            }
            else {
                group = module;
            }
        }
    }
    catch (Py::Exception& e) {
        e.clear();
    }
    try {
        Base::PyGILStateLocker lock;

        Py::Object cmd(pcCmdObj);
        if (cmd.hasAttr("GetCommands")) {
            Command* cmd = new PythonGroupCommand(pName, pcCmdObj);
            if (!module.empty()) {
                cmd->setAppModuleName(module.c_str());
            }
            if (!group.empty()) {
                cmd->setGroupName(group.c_str());
            }
            Application::Instance->commandManager().addCommand(cmd);
        }
        else {
            Command* cmd = new PythonCommand(pName, pcCmdObj, pSource);
            if (!module.empty()) {
                cmd->setAppModuleName(module.c_str());
            }
            if (!group.empty()) {
                cmd->setGroupName(group.c_str());
            }
            Application::Instance->commandManager().addCommand(cmd);
        }
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    catch (...) {
        PyErr_SetString(
            Base::PyExc_FC_GeneralError,
            "Unknown C++ exception raised in ApplicationPy::sAddCommand()"
        );
        return nullptr;
    }

    Py_Return;
}

PyObject* ApplicationPy::sRunCommand(PyObject* /*self*/, PyObject* args)
{
    char* pName = nullptr;
    int item = 0;
    if (!PyArg_ParseTuple(args, "s|i", &pName, &item)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.runCommand");

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    Command* cmd = Application::Instance->commandManager().getCommandByName(pName);
    if (cmd) {
        cmd->invoke(item);
        Py_Return;
    }

    PyErr_Format(Base::PyExc_FC_GeneralError, "No such command '%s'", pName);
    return nullptr;
}

PyObject* ApplicationPy::sDoCommand(PyObject* /*self*/, PyObject* args)
{
    char* sCmd = nullptr;
    if (!PyArg_ParseTuple(args, "s", &sCmd)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.doCommand");

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    Gui::Command::printPyCaller();
    Gui::Application::Instance->macroManager()->addLine(MacroManager::App, sCmd);

    PyObject* module = nullptr;
    PyObject* dict = nullptr;

    Base::PyGILStateLocker locker;
    module = PyImport_AddModule("__main__");
    if (!module) {
        return nullptr;
    }

    dict = PyModule_GetDict(module);
    if (!dict) {
        return nullptr;
    }

    return PyRun_String(sCmd, Py_file_input, dict, dict);
}

PyObject* ApplicationPy::sDoCommandGui(PyObject* /*self*/, PyObject* args)
{
    char* sCmd = nullptr;
    if (!PyArg_ParseTuple(args, "s", &sCmd)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.doCommandGui");

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    Gui::Command::printPyCaller();
    Gui::Application::Instance->macroManager()->addLine(MacroManager::Gui, sCmd);

    PyObject* module = nullptr;
    PyObject* dict = nullptr;

    Base::PyGILStateLocker locker;
    module = PyImport_AddModule("__main__");
    if (!module) {
        return nullptr;
    }

    dict = PyModule_GetDict(module);
    if (!dict) {
        return nullptr;
    }

    return PyRun_String(sCmd, Py_file_input, dict, dict);
}

PyObject* ApplicationPy::sDoCommandEval(PyObject* /*self*/, PyObject* args)
{
    char* sCmd = nullptr;
    if (!PyArg_ParseTuple(args, "s", &sCmd)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.doCommandEval");

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    PyObject* module = nullptr;
    PyObject* dict = nullptr;

    Base::PyGILStateLocker locker;
    module = PyImport_AddModule("__main__");
    if (!module) {
        return nullptr;
    }

    dict = PyModule_GetDict(module);
    if (!dict) {
        return nullptr;
    }

    return PyRun_String(sCmd, Py_eval_input, dict, dict);
}

PyObject* ApplicationPy::sDoCommandSkip(PyObject* /*self*/, PyObject* args)
{
    char* sCmd = nullptr;
    if (!PyArg_ParseTuple(args, "s", &sCmd)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.doCommandSkip");

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    Gui::Command::printPyCaller();
    Gui::Application::Instance->macroManager()->addLine(MacroManager::App, sCmd);
    return Py::None().ptr();
}

PyObject* ApplicationPy::sAddModule(PyObject* /*self*/, PyObject* args)
{
    char* pstr = nullptr;
    if (!PyArg_ParseTuple(args, "s", &pstr)) {
        return nullptr;
    }

    try {
        Command::addModule(Command::Doc, pstr);
        Py_Return;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return nullptr;
    }
}

PyObject* ApplicationPy::sShowDownloads(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.showDownloads");

    Gui::Dialog::DownloadManager::getInstance();

    Py_Return;
}

PyObject* ApplicationPy::sShowPreferences(PyObject* /*self*/, PyObject* args)
{
    char* pstr = nullptr;
    int idx = 0;
    if (!PyArg_ParseTuple(args, "|si", &pstr, &idx)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.showPreferences");

    Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
    if (pstr) {
        cDlg.activateGroupPage(QString::fromUtf8(pstr), idx);
    }

    WaitCursor wc;
    wc.restoreCursor();
    cDlg.exec();
    wc.setWaitCursor();

    Py_Return;
}

PyObject* ApplicationPy::sShowPreferencesByName(PyObject* /*self*/, PyObject* args)
{
    char* pstr = nullptr;
    const char* prefType = "";
    if (!PyArg_ParseTuple(args, "s|s", &pstr, &prefType)) {
        return nullptr;
    }

    requirePythonMainThread("FreeCADGui.showPreferences");

    Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
    if (pstr && prefType) {
        cDlg.activateGroupPageByPageName(QString::fromUtf8(pstr), QString::fromUtf8(prefType));
    }

    WaitCursor wc;
    wc.restoreCursor();
    cDlg.exec();
    wc.setWaitCursor();

    Py_Return;
}

PyObject* ApplicationPy::sCreateViewer(PyObject* /*self*/, PyObject* args)
{
    int num_of_views = 1;
    char* title = nullptr;
    // if one argument (int) is given
    if (!PyArg_ParseTuple(args, "|is", &num_of_views, &title)) {
        return nullptr;
    }

    if (num_of_views <= 0) {
        PyErr_Format(PyExc_ValueError, "views must be > 0");
        return nullptr;
    }
    requirePythonMainThread("FreeCADGui.createViewer");
    if (num_of_views == 1) {
        auto viewer = new View3DInventor(nullptr, nullptr);
        if (title) {
            viewer->setWindowTitle(QString::fromUtf8(title));
        }
        Gui::getMainWindow()->addWindow(viewer);
        return viewer->getPyObject();
    }
    else {
        auto viewer = new SplitView3DInventor(num_of_views, nullptr, nullptr);
        if (title) {
            viewer->setWindowTitle(QString::fromUtf8(title));
        }
        Gui::getMainWindow()->addWindow(viewer);
        return viewer->getPyObject();
    }
}

PyObject* ApplicationPy::sGetMarkerIndex(PyObject* /*self*/, PyObject* args)
{
    constexpr const int defaultSize = 9;
    char* pstr {};
    int defSize = defaultSize;
    if (!PyArg_ParseTuple(args, "s|i", &pstr, &defSize)) {
        return nullptr;
    }

    PY_TRY
    {
        ParameterGrp::handle const hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View"
        );

        // find the appropriate marker style string token
        std::string marker_arg = pstr;

        std::list<std::pair<std::string, std::string>> markerList = {
            {"square", "DIAMOND_FILLED"},
            {"cross", "CROSS"},
            {"hourglass", "HOURGLASS_FILLED"},
            {"plus", "PLUS"},
            {"empty", "SQUARE_LINE"},
            {"quad", "SQUARE_FILLED"},
            {"circle", "CIRCLE_LINE"},
            {"default", "CIRCLE_FILLED"}
        };

        auto findIt = std::find_if(markerList.begin(), markerList.end(), [&marker_arg](const auto& it) {
            return marker_arg == it.first || marker_arg == it.second;
        });

        marker_arg = (findIt != markerList.end() ? findIt->second : "CIRCLE_FILLED");


        // get the marker size
        auto sizeList = Gui::Inventor::MarkerBitmaps::getSupportedSizes(marker_arg);

        if (std::ranges::find(sizeList, defSize) == std::end(sizeList)) {
            defSize = defaultSize;
        }

        return Py_BuildValue("i", Gui::Inventor::MarkerBitmaps::getMarkerIndex(marker_arg, defSize));
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sReload(PyObject* /*self*/, PyObject* args)
{
    const char* name = nullptr;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    PY_TRY
    {
        auto doc = Application::Instance->reopen(App::GetApplication().getDocument(name));
        if (doc) {
            return doc->getPyObject();
        }
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sLoadFile(PyObject* /*self*/, PyObject* args)
{
    const char* path = "";
    const char* mod = "";
    if (!PyArg_ParseTuple(args, "s|s", &path, &mod)) {
        return nullptr;
    }

    PY_TRY
    {
        Base::FileInfo fi(path);
        if (!fi.isFile() || !fi.exists()) {
            PyErr_Format(PyExc_IOError, "File %s doesn't exist.", path);
            return nullptr;
        }

        std::string module = mod;
        if (module.empty()) {
            std::string ext = fi.extension();
            std::vector<std::string> modules = App::GetApplication().getImportModules(ext.c_str());
            if (modules.empty()) {
                PyErr_Format(PyExc_IOError, "Filetype %s is not supported.", ext.c_str());
                return nullptr;
            }

            module = modules.front();
        }

        Application::Instance->open(path, module.c_str());

        Py_Return;
    }
    PY_CATCH
}

PyObject* ApplicationPy::sAddDocumentObserver(PyObject* /*self*/, PyObject* args)
{
    PyObject* o = nullptr;
    if (!PyArg_ParseTuple(args, "O", &o)) {
        return nullptr;
    }

    PY_TRY
    {
        DocumentObserverPython::addObserver(Py::Object(o));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sRemoveDocumentObserver(PyObject* /*self*/, PyObject* args)
{
    PyObject* o = nullptr;
    if (!PyArg_ParseTuple(args, "O", &o)) {
        return nullptr;
    }

    PY_TRY
    {
        DocumentObserverPython::removeObserver(Py::Object(o));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sAddWorkbenchManipulator(PyObject* /*self*/, PyObject* args)
{
    PyObject* o = nullptr;
    if (!PyArg_ParseTuple(args, "O", &o)) {
        return nullptr;
    }

    PY_TRY
    {
        WorkbenchManipulatorPython::installManipulator(Py::Object(o));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sRemoveWorkbenchManipulator(PyObject* /*self*/, PyObject* args)
{
    PyObject* o = nullptr;
    if (!PyArg_ParseTuple(args, "O", &o)) {
        return nullptr;
    }

    PY_TRY
    {
        WorkbenchManipulatorPython::removeManipulator(Py::Object(o));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sCoinRemoveAllChildren(PyObject* /*self*/, PyObject* args)
{
    PyObject* pynode = nullptr;
    if (!PyArg_ParseTuple(args, "O", &pynode)) {
        return nullptr;
    }

    PY_TRY
    {
        void* ptr = nullptr;
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "_p_SoGroup", pynode, &ptr, 0);
        if (!ptr) {
            PyErr_SetString(PyExc_RuntimeError, "Conversion of coin.SoGroup failed");
            return nullptr;
        }

        coinRemoveAllChildren(static_cast<SoGroup*>(ptr));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sApplyElementColorOverride(PyObject* /*self*/, PyObject* args)
{
    PyObject* targetObj = nullptr;
    PyObject* colorsObj = nullptr;
    if (!PyArg_ParseTuple(args, "OO", &targetObj, &colorsObj)) {
        return nullptr;
    }

    PY_TRY
    {
        requirePythonMainThread("FreeCADGui.applyElementColorOverride");
        auto target = pythonToCoinActionTarget(targetObj);
        auto colors = pythonToColorOverrideMap(colorsObj);
        applyElementColorOverrideAction(target, std::move(colors));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sClearElementColorOverride(PyObject* /*self*/, PyObject* args)
{
    PyObject* targetObj = nullptr;
    if (!PyArg_ParseTuple(args, "O", &targetObj)) {
        return nullptr;
    }

    PY_TRY
    {
        requirePythonMainThread("FreeCADGui.clearElementColorOverride");
        auto target = pythonToCoinActionTarget(targetObj);
        applyElementColorOverrideAction(target, {});
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sListUserEditModes(PyObject* /*self*/, PyObject* args)
{
    Py::List ret;
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    for (auto const& uem : Application::Instance->listUserEditModes()) {
        ret.append(Py::String(uem.second.first));
    }

    return Py::new_reference_to(ret);
}

PyObject* ApplicationPy::sGetUserEditMode(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    return Py::new_reference_to(Py::String(Application::Instance->getUserEditModeUIStrings().first));
}

PyObject* ApplicationPy::sSetUserEditMode(PyObject* /*self*/, PyObject* args)
{
    const char* mode = "";
    if (!PyArg_ParseTuple(args, "s", &mode)) {
        return nullptr;
    }

    bool ok = Application::Instance->setUserEditMode(std::string(mode));

    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ApplicationPy::sSuspendWaitCursor(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    WaitCursor::suspend();
    Py_RETURN_NONE;
}

PyObject* ApplicationPy::sResumeWaitCursor(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    WaitCursor::resume();
    Py_RETURN_NONE;
}
