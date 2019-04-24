/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <qfileinfo.h>
# include <qdir.h>
# include <QPrinter>
# include <QFileInfo>
# include <Inventor/SoInput.h>
# include <Inventor/actions/SoGetPrimitiveCountAction.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/TranscodingException.hpp>
#include <boost/regex.hpp>

#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Document.h"
#include "MainWindow.h"
#include "Macro.h"
#include "EditorView.h"
#include "PythonEditor.h"
#include "SoFCDB.h"
#include "View3DInventor.h"
#include "SplitView3DInventor.h"
#include "ViewProvider.h"
#include "WaitCursor.h"
#include "WidgetFactory.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "Language/Translator.h"
#include "DownloadManager.h"
#include "DlgPreferencesImp.h"
#include "DocumentObserverPython.h"
#include <App/DocumentObjectPy.h>
#include <App/DocumentPy.h>
#include <App/PropertyFile.h>
#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <CXX/Objects.hxx>
#include <Inventor/MarkerBitmaps.h>

using namespace Gui;

// FCApplication Methods						// Methods structure
PyMethodDef Application::Methods[] = {
  {"activateWorkbench",(PyCFunction) Application::sActivateWorkbenchHandler, METH_VARARGS,
   "activateWorkbench(string) -> None\n\n"
   "Activate the workbench by name"},
  {"addWorkbench",     (PyCFunction) Application::sAddWorkbenchHandler, METH_VARARGS,
   "addWorkbench(string, object) -> None\n\n"
   "Add a workbench under a defined name."},
  {"removeWorkbench",  (PyCFunction) Application::sRemoveWorkbenchHandler, METH_VARARGS,
   "removeWorkbench(string) -> None\n\n"
   "Remove the workbench with name"},
  {"getWorkbench",     (PyCFunction) Application::sGetWorkbenchHandler, METH_VARARGS,
   "getWorkbench(string) -> object\n\n"
   "Get the workbench by its name"},
  {"listWorkbenches",   (PyCFunction) Application::sListWorkbenchHandlers, METH_VARARGS,
   "listWorkbenches() -> list\n\n"
   "Show a list of all workbenches"},
  {"activeWorkbench", (PyCFunction) Application::sActiveWorkbenchHandler, METH_VARARGS,
   "activeWorkbench() -> object\n\n"
   "Return the active workbench object"},
  {"addResourcePath",             (PyCFunction) Application::sAddResPath, METH_VARARGS,
   "addResourcePath(string) -> None\n\n"
   "Add a new path to the system where to find resource files\n"
   "like icons or localization files"},
  {"addLanguagePath",             (PyCFunction) Application::sAddLangPath, METH_VARARGS,
   "addLanguagePath(string) -> None\n\n"
   "Add a new path to the system where to find language files"},
  {"addIconPath",             (PyCFunction) Application::sAddIconPath, METH_VARARGS,
   "addIconPath(string) -> None\n\n"
   "Add a new path to the system where to find icon files"},
  {"addIcon",                 (PyCFunction) Application::sAddIcon, METH_VARARGS,
   "addIcon(string, string or list) -> None\n\n"
   "Add an icon as file name or in XPM format to the system"},
  {"getIcon",                 (PyCFunction) Application::sGetIcon, METH_VARARGS,
   "getIcon(string -> QIcon\n\n"
   "Get an icon in the system"},
  {"getMainWindow",           (PyCFunction) Application::sGetMainWindow, METH_VARARGS,
   "getMainWindow() -> QMainWindow\n\n"
   "Return the main window instance"},
  {"updateGui",               (PyCFunction) Application::sUpdateGui, METH_VARARGS,
   "updateGui() -> None\n\n"
   "Update the main window and all its windows"},
  {"updateLocale",            (PyCFunction) Application::sUpdateLocale, METH_VARARGS,
   "updateLocale() -> None\n\n"
   "Update the localization"},
  {"getLocale",            (PyCFunction) Application::sGetLocale, METH_VARARGS,
   "getLocale() -> string\n\n"
   "Returns the locale currently used by FreeCAD"},
  {"setLocale",            (PyCFunction) Application::sSetLocale, METH_VARARGS,
   "getLocale(string) -> None\n\n"
   "Sets the locale used by FreeCAD. You can set it by\n"
   "top-level domain (e.g. \"de\") or the language name (e.g. \"German\")"},
  {"supportedLocales", (PyCFunction) Application::sSupportedLocales, METH_VARARGS,
   "supportedLocales() -> dict\n\n"
   "Returns a dict of all supported languages/top-level domains"},
  {"createDialog",            (PyCFunction) Application::sCreateDialog, METH_VARARGS,
   "createDialog(string) -- Open a UI file"},
  {"addPreferencePage",       (PyCFunction) Application::sAddPreferencePage, METH_VARARGS,
   "addPreferencePage(string,string) -- Add a UI form to the\n"
   "preferences dialog. The first argument specifies the file name"
   "and the second specifies the group name"},
  {"addCommand",              (PyCFunction) Application::sAddCommand, METH_VARARGS,
   "addCommand(string, object) -> None\n\n"
   "Add a command object"},
  {"runCommand",              (PyCFunction) Application::sRunCommand, METH_VARARGS,
   "runCommand(string) -> None\n\n"
   "Run command with name"},
  {"isCommandActive",         (PyCFunction) Application::sIsCommandActive, METH_VARARGS,
   "isCommandActive(string) -> Bool\n\n"
   "Test if a command is active"},
  {"listCommands",               (PyCFunction) Application::sListCommands, METH_VARARGS,
   "listCommands() -> list of strings\n\n"
   "Returns a list of all commands known to FreeCAD."},
  {"SendMsgToActiveView",     (PyCFunction) Application::sSendActiveView, METH_VARARGS,
   "deprecated -- use class View"},
  {"sendMsgToFocusView",     (PyCFunction) Application::sSendFocusView, METH_VARARGS,
   "send message to the focused view"},
  {"hide",                    (PyCFunction) Application::sHide, METH_VARARGS,
   "deprecated"},
  {"show",                    (PyCFunction) Application::sShow, METH_VARARGS,
   "deprecated"},
  {"hideObject",              (PyCFunction) Application::sHideObject, METH_VARARGS,
   "hideObject(object) -> None\n\n"
   "Hide the view provider to the given object"},
  {"showObject",              (PyCFunction) Application::sShowObject, METH_VARARGS,
   "showObject(object) -> None\n\n"
   "Show the view provider to the given object"},
  {"open",                    (PyCFunction) Application::sOpen, METH_VARARGS,
   "Open a macro, Inventor or VRML file"},
  {"insert",                  (PyCFunction) Application::sInsert, METH_VARARGS,
   "Open a macro, Inventor or VRML file"},
  {"export",                  (PyCFunction) Application::sExport, METH_VARARGS,
   "save scene to Inventor or VRML file"},
  {"activeDocument",          (PyCFunction) Application::sActiveDocument, METH_VARARGS,
   "activeDocument() -> object or None\n\n"
   "Return the active document or None if no one exists"},
  {"setActiveDocument",       (PyCFunction) Application::sSetActiveDocument, METH_VARARGS,
   "setActiveDocument(string or App.Document) -> None\n\n"
   "Activate the specified document"},
  {"activeView", (PyCFunction)Application::sActiveView, METH_VARARGS,
   "activeView() -> object or None\n\n"
   "Return the active view of the active document or None if no one exists" },
  {"activateView", (PyCFunction)Application::sActivateView, METH_VARARGS,
   "activateView(type)\n\n"
   "Activate a view of the given type of the active document"},
  {"editDocument", (PyCFunction)Application::sEditDocument, METH_VARARGS,
   "editDocument() -> object or None\n\n"
   "Return the current editing document or None if no one exists" },
  {"getDocument",             (PyCFunction) Application::sGetDocument, METH_VARARGS,
   "getDocument(string) -> object\n\n"
   "Get a document by its name"},
  {"doCommand",               (PyCFunction) Application::sDoCommand, METH_VARARGS,
   "doCommand(string) -> None\n\n"
   "Prints the given string in the python console and runs it"},
  {"doCommandGui",               (PyCFunction) Application::sDoCommandGui, METH_VARARGS,
   "doCommandGui(string) -> None\n\n"
   "Prints the given string in the python console and runs it but doesn't record it in macros"},
  {"addModule",               (PyCFunction) Application::sAddModule, METH_VARARGS,
   "addModule(string) -> None\n\n"
   "Prints the given module import only once in the macro recording"},
  {"showDownloads",               (PyCFunction) Application::sShowDownloads, METH_VARARGS,
   "showDownloads() -> None\n\n"
   "Shows the downloads manager window"},
  {"showPreferences",               (PyCFunction) Application::sShowPreferences, METH_VARARGS,
   "showPreferences([string,int]) -> None\n\n"
   "Shows the preferences window. If string and int are provided, the given page index in the given group is shown."},
  {"createViewer",               (PyCFunction) Application::sCreateViewer, METH_VARARGS,
   "createViewer([int]) -> View3DInventor/SplitView3DInventor\n\n"
   "shows and returns a viewer. If the integer argument is given and > 1: -> splitViewer"},

  {"getMarkerIndex", (PyCFunction) Application::sGetMarkerIndex, METH_VARARGS,
   "Get marker index according to marker size setting"},
   
    {"addDocumentObserver",  (PyCFunction) Application::sAddDocObserver, METH_VARARGS,
     "addDocumentObserver() -> None\n\n"
     "Add an observer to get notified about changes on documents."},
    {"removeDocumentObserver",  (PyCFunction) Application::sRemoveDocObserver, METH_VARARGS,
     "removeDocumentObserver() -> None\n\n"
     "Remove an added document observer."},

  {"reload",                    (PyCFunction) Application::sReload, METH_VARARGS,
   "reload(name) -> doc\n\n"
   "Reload a partial opened document"},

  {NULL, NULL, 0, NULL}		/* Sentinel */
};

PyObject* Gui::Application::sEditDocument(PyObject * /*self*/, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
		return NULL;                       // NULL triggers exception 

	Document *pcDoc = Instance->editDocument();
	if (pcDoc) {
		return pcDoc->getPyObject();
	}
	else {
		Py_Return;
	}
}

PyObject* Gui::Application::sActiveDocument(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Document *pcDoc = Instance->activeDocument();
    if (pcDoc) {
        return pcDoc->getPyObject();
    }
    else {
        Py_Return;
    }
}

PyObject* Gui::Application::sActiveView(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Gui::MDIView* mdiView = Instance->activeView();
    if (mdiView) {
        // already incremented in getPyObject().
        return mdiView->getPyObject();
    }

    Py_Return;
}

PyObject* Gui::Application::sActivateView(PyObject * /*self*/, PyObject *args)
{
    char* typeStr;
    PyObject *create = Py_False;
    if (!PyArg_ParseTuple(args, "sO!", &typeStr, &PyBool_Type, &create))
        return NULL;

    Base::Type type = Base::Type::fromName(typeStr);
    Instance->activateView(type, PyObject_IsTrue(create) ? true : false);
    Py_Return;
}

PyObject* Gui::Application::sSetActiveDocument(PyObject * /*self*/, PyObject *args)
{
    Document *pcDoc = 0;

    do {
        char *pstr=0;
        if (PyArg_ParseTuple(args, "s", &pstr)) {
            pcDoc = Instance->getDocument(pstr);
            if (!pcDoc) {
                PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
                return 0;
            }
            break;
        }

        PyErr_Clear();
        PyObject* doc;
        if (PyArg_ParseTuple(args, "O!", &(App::DocumentPy::Type), &doc)) {
            pcDoc = Instance->getDocument(static_cast<App::DocumentPy*>(doc)->getDocumentPtr());
            if (!pcDoc) {
                PyErr_Format(PyExc_KeyError, "Unknown document instance");
                return 0;
            }
            break;
        }
    }
    while(false);

    if (!pcDoc) {
        PyErr_SetString(PyExc_TypeError, "Either string or App.Document expected");
        return 0;
    }

    if (Instance->activeDocument() != pcDoc) {
        Gui::MDIView* view = pcDoc->getActiveView();
        getMainWindow()->setActiveWindow(view);
    }
    Py_Return;
}

PyObject* Application::sGetDocument(PyObject * /*self*/, PyObject *args)
{
    char *pstr=0;
    if (PyArg_ParseTuple(args, "s", &pstr)) {
        Document *pcDoc = Instance->getDocument(pstr);
        if (!pcDoc) {
            PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
            return 0;
        }
        return pcDoc->getPyObject();
    }

    PyErr_Clear();
    PyObject* doc;
    if (PyArg_ParseTuple(args, "O!", &(App::DocumentPy::Type), &doc)) {
        Document *pcDoc = Instance->getDocument(static_cast<App::DocumentPy*>(doc)->getDocumentPtr());
        if (!pcDoc) {
            PyErr_Format(PyExc_KeyError, "Unknown document instance");
            return 0;
        }
        return pcDoc->getPyObject();
    }

    PyErr_SetString(PyExc_TypeError, "Either string or App.Document exprected");
    return 0;
}

PyObject* Application::sHide(PyObject * /*self*/, PyObject *args)
{
    char *psFeatStr;
    if (!PyArg_ParseTuple(args, "s;Name of the object to hide has to be given!",&psFeatStr))
        return NULL;

    Document *pcDoc = Instance->activeDocument();

    if (pcDoc)
        pcDoc->setHide(psFeatStr);

    Py_Return;
}

PyObject* Application::sShow(PyObject * /*self*/, PyObject *args)
{
    char *psFeatStr;
    if (!PyArg_ParseTuple(args, "s;Name of the object to show has to be given!",&psFeatStr))
        return NULL;

    Document *pcDoc = Instance->activeDocument();

    if (pcDoc)
        pcDoc->setShow(psFeatStr);

    Py_Return;
}

PyObject* Application::sHideObject(PyObject * /*self*/, PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!",&(App::DocumentObjectPy::Type),&object))
        return 0;

    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    Instance->hideViewProvider(obj);

    Py_Return;
}

PyObject* Application::sShowObject(PyObject * /*self*/, PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!",&(App::DocumentObjectPy::Type),&object))
        return 0;

    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    Instance->showViewProvider(obj);

    Py_Return;
}

PyObject* Application::sOpen(PyObject * /*self*/, PyObject *args)
{
    // only used to open Python files
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);
    PY_TRY {
        QString fileName = QString::fromUtf8(Utf8Name.c_str());
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.suffix().toLower();
        QList<EditorView*> views = getMainWindow()->findChildren<EditorView*>();
        for (QList<EditorView*>::Iterator it = views.begin(); it != views.end(); ++it) {
            if ((*it)->fileName() == fileName) {
                (*it)->setFocus();
                Py_Return;
            }
        }

        if (ext == QLatin1String("iv")) {
            if (!Application::Instance->activeDocument())
                App::GetApplication().newDocument();
            //QString cmd = QString("Gui.activeDocument().addAnnotation(\"%1\",\"%2\")").arg(fi.baseName()).arg(fi.absoluteFilePath());
            QString cmd = QString::fromLatin1(
                "App.ActiveDocument.addObject(\"App::InventorObject\",\"%1\")."
                "FileName=\"%2\"\n"
                "App.ActiveDocument.ActiveObject.Label=\"%1\"\n"
                "App.ActiveDocument.recompute()")
                .arg(fi.baseName()).arg(fi.absoluteFilePath());
            Base::Interpreter().runString(cmd.toUtf8());
        }
        else if (ext == QLatin1String("wrl") ||
                 ext == QLatin1String("vrml") ||
                 ext == QLatin1String("wrz")) {
            if (!Application::Instance->activeDocument())
                App::GetApplication().newDocument();

            // Add this to the search path in order to read inline files (#0002029)
            QByteArray path = fi.absolutePath().toUtf8();
            SoInput::addDirectoryFirst(path.constData());

            //QString cmd = QString("Gui.activeDocument().addAnnotation(\"%1\",\"%2\")").arg(fi.baseName()).arg(fi.absoluteFilePath());
            QString cmd = QString::fromLatin1(
                "App.ActiveDocument.addObject(\"App::VRMLObject\",\"%1\")."
                "VrmlFile=\"%2\"\n"
                "App.ActiveDocument.ActiveObject.Label=\"%1\"\n"
                "App.ActiveDocument.recompute()")
                .arg(fi.baseName()).arg(fi.absoluteFilePath());
            Base::Interpreter().runString(cmd.toUtf8());
            SoInput::removeDirectory(path.constData());
        }
        else if (ext == QLatin1String("py") || ext == QLatin1String("fcmacro") ||
                 ext == QLatin1String("fcscript")) {
            PythonEditor* editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
            PythonEditorView* edit = new PythonEditorView(editor, getMainWindow());
            edit->open(fileName);
            edit->resize(400, 300);
            getMainWindow()->addWindow( edit );
        }
        else {
            Base::Console().Error("File type '%s' not supported\n", ext.toLatin1().constData());
        }
    } PY_CATCH;

    Py_Return;
}

PyObject* Application::sInsert(PyObject * /*self*/, PyObject *args)
{
    char* Name;
    char* DocName=0;
    if (!PyArg_ParseTuple(args, "et|s","utf-8",&Name,&DocName))
        return NULL;
    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);

    PY_TRY {
        QString fileName = QString::fromUtf8(Utf8Name.c_str());
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.suffix().toLower();
        if (ext == QLatin1String("iv")) {
            App::Document *doc = 0;
            if (DocName)
                doc = App::GetApplication().getDocument(DocName);
            else
                doc = App::GetApplication().getActiveDocument();
            if (!doc)
                doc = App::GetApplication().newDocument(DocName);

            App::DocumentObject* obj = doc->addObject("App::InventorObject",
                (const char*)fi.baseName().toUtf8());
            obj->Label.setValue((const char*)fi.baseName().toUtf8());
            static_cast<App::PropertyString*>(obj->getPropertyByName("FileName"))
                ->setValue((const char*)fi.absoluteFilePath().toUtf8());
            doc->recompute();
        }
        else if (ext == QLatin1String("wrl") ||
                 ext == QLatin1String("vrml") ||
                 ext == QLatin1String("wrz")) {
            App::Document *doc = 0;
            if (DocName)
                doc = App::GetApplication().getDocument(DocName);
            else
                doc = App::GetApplication().getActiveDocument();
            if (!doc)
                doc = App::GetApplication().newDocument(DocName);

            // Add this to the search path in order to read inline files (#0002029)
            QByteArray path = fi.absolutePath().toUtf8();
            SoInput::addDirectoryFirst(path.constData());

            App::DocumentObject* obj = doc->addObject("App::VRMLObject",
                (const char*)fi.baseName().toUtf8());
            obj->Label.setValue((const char*)fi.baseName().toUtf8());
            static_cast<App::PropertyFileIncluded*>(obj->getPropertyByName("VrmlFile"))
                ->setValue((const char*)fi.absoluteFilePath().toUtf8());
            doc->recompute();

            SoInput::removeDirectory(path.constData());
        }
        else if (ext == QLatin1String("py") || ext == QLatin1String("fcmacro") ||
                 ext == QLatin1String("fcscript")) {
            PythonEditor* editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
            PythonEditorView* edit = new PythonEditorView(editor, getMainWindow());
            edit->open(fileName);
            edit->resize(400, 300);
            getMainWindow()->addWindow( edit );
        }
        else {
            Base::Console().Error("File type '%s' not supported\n", ext.toLatin1().constData());
        }
    } PY_CATCH;

    Py_Return;
}

PyObject* Application::sExport(PyObject * /*self*/, PyObject *args)
{
    PyObject* object;
    char* Name;
    if (!PyArg_ParseTuple(args, "Oet",&object,"utf-8",&Name))
        return NULL;
    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);

    PY_TRY {
        App::Document* doc = 0;
        Py::Sequence list(object);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                doc = obj->getDocument();
                break;
            }
        }

        QString fileName = QString::fromUtf8(Utf8Name.c_str());
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.suffix().toLower();
        if (ext == QLatin1String("iv") || ext == QLatin1String("wrl") ||
            ext == QLatin1String("vrml") || ext == QLatin1String("wrz")) {

            // build up the graph
            SoSeparator* sep = new SoSeparator();
            sep->ref();

            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();

                    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
                    if (vp) {
                        sep->addChild(vp->getRoot());
                    }
                }
            }


            SoGetPrimitiveCountAction action;
            action.setCanApproximate(true);
            action.apply(sep);

            bool binary = false;
            if (action.getTriangleCount() > 100000 ||
                action.getPointCount() > 30000 ||
                action.getLineCount() > 10000)
                binary = true;

            SoFCDB::writeToFile(sep, Utf8Name.c_str(), binary);
            sep->unref();
        }
        else if (ext == QLatin1String("pdf")) {
            // get the view that belongs to the found document
            Gui::Document* gui_doc = Application::Instance->getDocument(doc);
            if (gui_doc) {
                Gui::MDIView* view = gui_doc->getActiveView();
                if (view) {
                    View3DInventor* view3d = qobject_cast<View3DInventor*>(view);
                    if (view3d)
                        view3d->viewAll();
                    QPrinter printer(QPrinter::ScreenResolution);
                    printer.setOutputFormat(QPrinter::PdfFormat);
                    printer.setOutputFileName(fileName);
                    view->print(&printer);
                }
            }
        }
        else {
            Base::Console().Error("File type '%s' not supported\n", ext.toLatin1().constData());
        }
    } PY_CATCH;

    Py_Return;
}

PyObject* Application::sSendActiveView(PyObject * /*self*/, PyObject *args)
{
    char *psCommandStr;
    PyObject *suppress=Py_False;
    if (!PyArg_ParseTuple(args, "s|O!",&psCommandStr,&PyBool_Type,&suppress))
        return NULL;

    const char* ppReturn=0;
    if (!Instance->sendMsgToActiveView(psCommandStr,&ppReturn)) {
        if (!PyObject_IsTrue(suppress))
            Base::Console().Warning("Unknown view command: %s\n",psCommandStr);
    }

    // Print the return value to the output
    if (ppReturn) {
        return Py_BuildValue("s",ppReturn);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sSendFocusView(PyObject * /*self*/, PyObject *args)
{
    char *psCommandStr;
    PyObject *suppress=Py_False;
    if (!PyArg_ParseTuple(args, "s|O!",&psCommandStr,&PyBool_Type,&suppress))
        return NULL;

    const char* ppReturn=0;
    if (!Instance->sendMsgToFocusView(psCommandStr,&ppReturn)) {
        if (!PyObject_IsTrue(suppress))
            Base::Console().Warning("Unknown view command: %s\n",psCommandStr);
    }

    // Print the return value to the output
    if (ppReturn) {
        return Py_BuildValue("s",ppReturn);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sGetMainWindow(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PythonWrapper wrap;
    if (!wrap.loadCoreModule() ||
        !wrap.loadGuiModule() ||
        !wrap.loadWidgetsModule()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load Python wrapper for Qt");
        return 0;
    }
    try {
        return Py::new_reference_to(wrap.fromQWidget(Gui::getMainWindow(), "QMainWindow"));
    }
    catch (const Py::Exception&) {
        return 0;
    }
}

PyObject* Application::sUpdateGui(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    qApp->processEvents();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sUpdateLocale(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Translator::instance()->refresh();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sGetLocale(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    std::string locale = Translator::instance()->activeLanguage();
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromString(locale.c_str());
#else
    return PyString_FromString(locale.c_str());
#endif
}

PyObject* Application::sSetLocale(PyObject * /*self*/, PyObject *args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    std::string cname(name);
    TStringMap map = Translator::instance()->supportedLocales();
    map["English"] = "en";
    for (const auto& it : map) {
        if (it.first == cname || it.second == cname) {
            Translator::instance()->activateLanguage(it.first.c_str());
            break;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sSupportedLocales(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

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

PyObject* Application::sCreateDialog(PyObject * /*self*/, PyObject *args)
{
    char* fn = 0;
    if (!PyArg_ParseTuple(args, "s", &fn))
        return NULL;

    PyObject* pPyResource=0L;
    try{
        pPyResource = new PyResource();
        ((PyResource*)pPyResource)->load(fn);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_AssertionError, e.what());
        return NULL;
    }

    return pPyResource;
}

PyObject* Application::sAddPreferencePage(PyObject * /*self*/, PyObject *args)
{
    char *fn, *grp;
    if (PyArg_ParseTuple(args, "ss", &fn,&grp)) {
        QFileInfo fi(QString::fromUtf8(fn));
        if (!fi.exists()) {
            PyErr_SetString(PyExc_RuntimeError, "UI file does not exist");
            return 0;
        }

        // add to the preferences dialog
        new PrefPageUiProducer(fn, grp);

        Py_INCREF(Py_None);
        return Py_None;
    }
    PyErr_Clear();

    PyObject* dlg;
    // old style classes
#if PY_MAJOR_VERSION >= 3
    if (PyArg_ParseTuple(args, "O!s", &PyType_Type, &dlg, &grp)) {
#else
    if (PyArg_ParseTuple(args, "O!s", &PyClass_Type, &dlg, &grp)) {
#endif
        // add to the preferences dialog
        new PrefPagePyProducer(Py::Object(dlg), grp);

        Py_INCREF(Py_None);
        return Py_None;
    }
    PyErr_Clear();

    // new style classes
    if (PyArg_ParseTuple(args, "O!s", &PyType_Type, &dlg, &grp)) {
        // add to the preferences dialog
        new PrefPagePyProducer(Py::Object(dlg), grp);

        Py_INCREF(Py_None);
        return Py_None;
    }

    return 0;
}

PyObject* Application::sActivateWorkbenchHandler(PyObject * /*self*/, PyObject *args)
{
    char*       psKey;
    if (!PyArg_ParseTuple(args, "s", &psKey))
        return NULL;

    // search for workbench handler from the dictionary
    PyObject* pcWorkbench = PyDict_GetItemString(Instance->_pcWorkbenchDictionary, psKey);
    if (!pcWorkbench) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", psKey);
        return NULL;
    }

    try {
        Instance->activateWorkbench(psKey);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    catch (const XERCES_CPP_NAMESPACE_QUALIFIER TranscodingException& e) {
        std::stringstream err;
        char *pMsg = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::transcode(e.getMessage());
        err << "Transcoding exception in Xerces-c:\n\n"
            << "Transcoding exception raised in activateWorkbench.\n"
            << "Check if your user configuration file is valid.\n"
            << "  Exception message:"
            << pMsg;
        XERCES_CPP_NAMESPACE_QUALIFIER XMLString::release(&pMsg);
        PyErr_SetString(PyExc_RuntimeError, err.str().c_str());
        return 0;
    }
    catch (...) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, "Unknown C++ exception raised in activateWorkbench");
        return 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sAddWorkbenchHandler(PyObject * /*self*/, PyObject *args)
{
    PyObject*   pcObject;
    std::string item;
    if (!PyArg_ParseTuple(args, "O", &pcObject))
        return NULL;

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
            Py::Tuple args;
            Py::Callable creation(object);
            object = creation.apply(args);
        }
        else if (PyObject_IsInstance(object.ptr(), baseclass.ptr()) == 1) {
            // extract the class name of the instance
            PyErr_Clear(); // PyObject_IsSubclass set an exception
            Py::Object classobj = object.getAttr(std::string("__class__"));
            name = classobj.getAttr(std::string("__name__"));
        }
        else {
            PyErr_SetString(PyExc_TypeError, "arg must be a subclass or an instance of "
                                             "a subclass of 'Workbench'");
            return NULL;
        }

        // Search for some methods and members without invoking them
        Py::Callable(object.getAttr(std::string("Initialize")));
        Py::Callable(object.getAttr(std::string("GetClassName")));
        item = name.as_std_string("ascii");

        PyObject* wb = PyDict_GetItemString(Instance->_pcWorkbenchDictionary,item.c_str()); 
        if (wb) {
            PyErr_Format(PyExc_KeyError, "'%s' already exists.", item.c_str());
            return NULL;
        }

        PyDict_SetItemString(Instance->_pcWorkbenchDictionary,item.c_str(),object.ptr());
        Instance->signalAddWorkbench(item.c_str());
    }
    catch (const Py::Exception&) {
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sRemoveWorkbenchHandler(PyObject * /*self*/, PyObject *args)
{
    char*       psKey;
    if (!PyArg_ParseTuple(args, "s", &psKey))
        return NULL;

    PyObject* wb = PyDict_GetItemString(Instance->_pcWorkbenchDictionary,psKey); 
    if (!wb) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", psKey);
        return NULL;
    }

    Instance->signalRemoveWorkbench(psKey);
    WorkbenchManager::instance()->removeWorkbench(psKey);
    PyDict_DelItemString(Instance->_pcWorkbenchDictionary,psKey);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sGetWorkbenchHandler(PyObject * /*self*/, PyObject *args)
{
    char* psKey;
    if (!PyArg_ParseTuple(args, "s", &psKey))
        return NULL;
   
    // get the python workbench object from the dictionary
    PyObject* pcWorkbench = PyDict_GetItemString(Instance->_pcWorkbenchDictionary, psKey);
    if (!pcWorkbench) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", psKey);
        return NULL;
    }

    Py_INCREF(pcWorkbench);
    return pcWorkbench;
}

PyObject* Application::sListWorkbenchHandlers(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Py_INCREF(Instance->_pcWorkbenchDictionary);
    return Instance->_pcWorkbenchDictionary;
}

PyObject* Application::sActiveWorkbenchHandler(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Workbench* actWb = WorkbenchManager::instance()->active();
    if (!actWb) {
        PyErr_SetString(PyExc_AssertionError, "No active workbench\n");
        return NULL;
    }

    // get the python workbench object from the dictionary
    std::string key = actWb->name();
    PyObject* pcWorkbench = PyDict_GetItemString(Instance->_pcWorkbenchDictionary, key.c_str());
    if (!pcWorkbench) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", key.c_str());
        return NULL;
    }

    // object get incremented
    Py_INCREF(pcWorkbench);
    return pcWorkbench;
}

PyObject* Application::sAddResPath(PyObject * /*self*/, PyObject *args)
{
    char* filePath;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath))
        return NULL;
    QString path = QString::fromUtf8(filePath);
    PyMem_Free(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromUtf8(App::GetApplication().getHomePath());
        path = home + path;
    }

    BitmapFactory().addPath(path);
    Translator::instance()->addPath(path);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sAddLangPath(PyObject * /*self*/, PyObject *args)
{
    char* filePath;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath))
        return NULL;
    QString path = QString::fromUtf8(filePath);
    PyMem_Free(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromUtf8(App::GetApplication().getHomePath());
        path = home + path;
    }

    Translator::instance()->addPath(path);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sAddIconPath(PyObject * /*self*/, PyObject *args)
{
    char* filePath;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath))
        return NULL;
    QString path = QString::fromUtf8(filePath);
    PyMem_Free(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromUtf8(App::GetApplication().getHomePath());
        path = home + path;
    }

    BitmapFactory().addPath(path);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sAddIcon(PyObject * /*self*/, PyObject *args)
{
    char *iconName;
    char *pixmap;
    if (!PyArg_ParseTuple(args, "ss", &iconName,&pixmap))
        return NULL;
    
    QPixmap icon;
    if (BitmapFactory().findPixmapInCache(iconName, icon)) {
        PyErr_SetString(PyExc_AssertionError, "Icon with this name already registered");
        return NULL;
    }

    QByteArray ary;
    std::string content = pixmap;
    int strlen = (int)content.size();
    ary.resize(strlen);
    for (int j=0; j<strlen; j++)
        ary[j]=content[j];
    icon.loadFromData(ary, "XPM");

    if (icon.isNull()){
        QString file = QString::fromUtf8(pixmap);
        icon.load(file);
    }

    if (icon.isNull()) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, "Invalid icon added to application");
        return NULL;
    }

    BitmapFactory().addPixmapToCache(iconName, icon);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sGetIcon(PyObject * /*self*/, PyObject *args)
{
    char *iconName;
    if (!PyArg_ParseTuple(args, "s", &iconName))
        return NULL;
    
    PythonWrapper wrap;
    wrap.loadGuiModule();
    wrap.loadWidgetsModule();
    auto pixmap = BitmapFactory().pixmap(iconName);
    if(!pixmap.isNull())
        return Py::new_reference_to(wrap.fromQIcon(new QIcon(pixmap)));
    Py_Return;
}

PyObject* Application::sAddCommand(PyObject * /*self*/, PyObject *args)
{
    char*       pName;
    char*       pSource=0;
    PyObject*   pcCmdObj;
    if (!PyArg_ParseTuple(args, "sO|s", &pName,&pcCmdObj,&pSource))
        return NULL;

    // get the call stack to find the Python module name
    //
    std::string module, group;
    try {
        Base::PyGILStateLocker lock;
        Py::Module mod(PyImport_ImportModule("inspect"), true);
        Py::Callable inspect(mod.getAttr("stack"));
        Py::Tuple args;
        Py::List list(inspect.apply(args));
        args = list.getItem(0);

        // usually this is the file name of the calling script
        std::string file = args.getItem(1).as_string();
        Base::FileInfo fi(file);
        // convert backslashes to slashes
        file = fi.filePath();
        module = fi.fileNamePure();

        // for the group name get the directory name after 'Mod'
        boost::regex rx("/Mod/(\\w+)/");
        boost::smatch what;
        if (boost::regex_search(file, what, rx)) {
            group = what[1];
        }
        else {
            boost::regex rx("/Ext/freecad/(\\w+)/");
            if (boost::regex_search(file, what, rx))
                group = what[1];
            else
                group = module;
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
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    catch (...) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, "Unknown C++ exception raised in Application::sAddCommand()");
        return 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sRunCommand(PyObject * /*self*/, PyObject *args)
{
    char* pName;
    int item = 0;
    if (!PyArg_ParseTuple(args, "s|i", &pName, &item))
        return NULL;

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    Command* cmd = Application::Instance->commandManager().getCommandByName(pName);
    if (cmd) {
        cmd->invoke(item);
        Py_INCREF(Py_None);
        return Py_None;
    }
    else {
        PyErr_Format(Base::BaseExceptionFreeCADError, "No such command '%s'", pName);
        return 0;
    }
}

PyObject* Application::sIsCommandActive(PyObject * /*self*/, PyObject *args)
{
    char* pName;
    if (!PyArg_ParseTuple(args, "s", &pName))
        return NULL;

    Command* cmd = Application::Instance->commandManager().getCommandByName(pName);
    if (!cmd) {
        PyErr_Format(Base::BaseExceptionFreeCADError, "No such command '%s'", pName);
        return 0;
    }
    PY_TRY {
        return Py::new_reference_to(Py::Boolean(cmd->isActive()));
    }PY_CATCH;
}


PyObject* Application::sListCommands(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    std::vector <Command*> cmds = Application::Instance->commandManager().getAllCommands();
    PyObject* pyList = PyList_New(cmds.size());
    int i=0;
    for ( std::vector<Command*>::iterator it = cmds.begin(); it != cmds.end(); ++it ) {
#if PY_MAJOR_VERSION >= 3
        PyObject* str = PyUnicode_FromString((*it)->getName());
#else
        PyObject* str = PyString_FromString((*it)->getName());
#endif
        PyList_SetItem(pyList, i++, str);
    }
    return pyList;
}

PyObject* Application::sDoCommand(PyObject * /*self*/, PyObject *args)
{
    char *sCmd=0;
    if (!PyArg_ParseTuple(args, "s", &sCmd))
        return NULL;

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    Gui::Command::printPyCaller();
    Gui::Application::Instance->macroManager()->addLine(MacroManager::App, sCmd);

    PyObject *module, *dict;

    Base::PyGILStateLocker locker;
    module = PyImport_AddModule("__main__");
    if (module == NULL)
        return 0;
    dict = PyModule_GetDict(module);
    if (dict == NULL)
        return 0;

    return PyRun_String(sCmd, Py_file_input, dict, dict);
}

PyObject* Application::sDoCommandGui(PyObject * /*self*/, PyObject *args)
{
    char *sCmd=0;
    if (!PyArg_ParseTuple(args, "s", &sCmd))
        return NULL;

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    Gui::Command::printPyCaller();
    Gui::Application::Instance->macroManager()->addLine(MacroManager::Gui, sCmd);

    PyObject *module, *dict;

    Base::PyGILStateLocker locker;
    module = PyImport_AddModule("__main__");
    if (module == NULL)
        return 0;
    dict = PyModule_GetDict(module);
    if (dict == NULL)
        return 0;

    return PyRun_String(sCmd, Py_file_input, dict, dict);
}

PyObject* Application::sAddModule(PyObject * /*self*/, PyObject *args)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args, "s", &pstr))
        return NULL;

    try {
        Command::addModule(Command::Doc,pstr);

        Py_INCREF(Py_None);
        return Py_None;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return 0;
    }
}

PyObject* Application::sShowDownloads(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Gui::Dialog::DownloadManager::getInstance();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sShowPreferences(PyObject * /*self*/, PyObject *args)
{
    char *pstr=0;
    int idx=0;
    if (!PyArg_ParseTuple(args, "|si", &pstr, &idx))
        return NULL;
    Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
    if (pstr) 
        cDlg.activateGroupPage(QString::fromUtf8(pstr),idx);
    WaitCursor wc;
    wc.restoreCursor();
    cDlg.exec();
    wc.setWaitCursor();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sCreateViewer(PyObject * /*self*/, PyObject *args)
{
    int num_of_views = 1;
    char* title = nullptr;
    // if one argument (int) is given
    if (PyArg_ParseTuple(args, "|is", &num_of_views, &title))
    {
        if (num_of_views < 0)
            return NULL;
        else if (num_of_views==1)
        {
            View3DInventor* viewer = new View3DInventor(0, 0);
            if (title)
                viewer->setWindowTitle(QString::fromUtf8(title));
            Gui::getMainWindow()->addWindow(viewer);
            return viewer->getPyObject();
        }
        else
        {
            SplitView3DInventor* viewer = new SplitView3DInventor(num_of_views, 0, 0);
            if (title)
                viewer->setWindowTitle(QString::fromUtf8(title));
            Gui::getMainWindow()->addWindow(viewer);
            return viewer->getPyObject();
        }
    }
    return Py_None;
}

PyObject* Application::sGetMarkerIndex(PyObject * /*self*/, PyObject *args)
{
    char *pstr   = 0;
    int  defSize = 9;
    if (!PyArg_ParseTuple(args, "|si", &pstr, &defSize))
        return NULL;

    PY_TRY {
        ParameterGrp::handle const hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

        if (strcmp(pstr, "square") == 0)
            return Py_BuildValue("i", Gui::Inventor::MarkerBitmaps::getMarkerIndex("DIAMOND_FILLED", hGrp->GetInt("MarkerSize", defSize)));
        else if (strcmp(pstr, "cross") == 0)
            return Py_BuildValue("i", Gui::Inventor::MarkerBitmaps::getMarkerIndex("CROSS", hGrp->GetInt("MarkerSize", defSize)));
        else if (strcmp(pstr, "plus") == 0)
            return Py_BuildValue("i", Gui::Inventor::MarkerBitmaps::getMarkerIndex("PLUS", hGrp->GetInt("MarkerSize", defSize)));
        else if (strcmp(pstr, "empty") == 0)
            return Py_BuildValue("i", Gui::Inventor::MarkerBitmaps::getMarkerIndex("SQUARE_LINE", hGrp->GetInt("MarkerSize", defSize)));
        else if (strcmp(pstr, "quad") == 0)
            return Py_BuildValue("i", Gui::Inventor::MarkerBitmaps::getMarkerIndex("SQUARE_FILLED", hGrp->GetInt("MarkerSize", defSize)));
        else if (strcmp(pstr, "circle") == 0)
            return Py_BuildValue("i", Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", hGrp->GetInt("MarkerSize", defSize)));
        else
            return Py_BuildValue("i", Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", hGrp->GetInt("MarkerSize", defSize)));
    }PY_CATCH;
}

PyObject* Application::sReload(PyObject * /*self*/, PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    PY_TRY {
        auto doc = Application::Instance->reopen(App::GetApplication().getDocument(name));
        if(doc)
            return doc->getPyObject();
    }PY_CATCH;
    Py_Return;
}

PyObject* Application::sAddDocObserver(PyObject * /*self*/, PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O",&o))
        return NULL;
    PY_TRY {
        DocumentObserverPython::addObserver(Py::Object(o));
        Py_Return;
    } PY_CATCH;
}

PyObject* Application::sRemoveDocObserver(PyObject * /*self*/, PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O",&o))
        return NULL;
    PY_TRY {
        DocumentObserverPython::removeObserver(Py::Object(o));
        Py_Return;
    } PY_CATCH;
}
