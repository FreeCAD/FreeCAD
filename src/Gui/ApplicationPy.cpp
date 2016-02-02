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
#include "ViewProvider.h"
#include "WidgetFactory.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "Language/Translator.h"
#include "DownloadManager.h"
#include "DlgPreferencesImp.h"
#include <App/DocumentObjectPy.h>
#include <App/DocumentPy.h>
#include <App/PropertyFile.h>
#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <CXX/Objects.hxx>

using namespace Gui;

// FCApplication Methods						// Methods structure
PyMethodDef Application::Methods[] = {
  {"activateWorkbench",(PyCFunction) Application::sActivateWorkbenchHandler,1,
   "activateWorkbench(string) -> None\n\n"
   "Activate the workbench by name"},
  {"addWorkbench",     (PyCFunction) Application::sAddWorkbenchHandler,     1,
   "addWorkbench(string, object) -> None\n\n"
   "Add a workbench under a defined name."},
  {"removeWorkbench",  (PyCFunction) Application::sRemoveWorkbenchHandler,  1,
   "removeWorkbench(string) -> None\n\n"
   "Remove the workbench with name"},
  {"getWorkbench",     (PyCFunction) Application::sGetWorkbenchHandler,     1,
   "getWorkbench(string) -> object\n\n"
   "Get the workbench by its name"},
  {"listWorkbenches",   (PyCFunction) Application::sListWorkbenchHandlers,    1,
   "listWorkbenches() -> list\n\n"
   "Show a list of all workbenches"},
  {"activeWorkbench", (PyCFunction) Application::sActiveWorkbenchHandler,   1,
   "activeWorkbench() -> object\n\n"
   "Return the active workbench object"},
  {"addResourcePath",             (PyCFunction) Application::sAddResPath,      1,
   "addResourcePath(string) -> None\n\n"
   "Add a new path to the system where to find resource files\n"
   "like icons or localization files"},
  {"addLanguagePath",             (PyCFunction) Application::sAddLangPath,      1,
   "addLanguagePath(string) -> None\n\n"
   "Add a new path to the system where to find language files"},
  {"addIconPath",             (PyCFunction) Application::sAddIconPath,      1,
   "addIconPath(string) -> None\n\n"
   "Add a new path to the system where to find icon files"},
  {"addIcon",                 (PyCFunction) Application::sAddIcon,          1,
   "addIcon(string, string or list) -> None\n\n"
   "Add an icon as file name or in XPM format to the system"},
  {"getMainWindow",           (PyCFunction) Application::sGetMainWindow,    1,
   "getMainWindow() -> QMainWindow\n\n"
   "Return the main window instance"},
  {"updateGui",               (PyCFunction) Application::sUpdateGui,        1,
   "updateGui() -> None\n\n"
   "Update the main window and all its windows"},
  {"updateLocale",            (PyCFunction) Application::sUpdateLocale,     1,
   "updateLocale() -> None\n\n"
   "Update the localization"},
  {"getLocale",            (PyCFunction) Application::sGetLocale,     1,
   "getLocale() -> string\n\n"
   "Returns the locale currently used by FreeCAD"},
  {"createDialog",            (PyCFunction) Application::sCreateDialog,     1,
   "createDialog(string) -- Open a UI file"},
  {"addPreferencePage",       (PyCFunction) Application::sAddPreferencePage,1,
   "addPreferencePage(string,string) -- Add a UI form to the\n"
   "preferences dialog. The first argument specifies the file name"
   "and the second specifies the group name"},
  {"addCommand",              (PyCFunction) Application::sAddCommand,       1,
   "addCommand(string, object) -> None\n\n"
   "Add a command object"},
  {"runCommand",              (PyCFunction) Application::sRunCommand,       1,
   "runCommand(string) -> None\n\n"
   "Run command with name"},
  {"SendMsgToActiveView",     (PyCFunction) Application::sSendActiveView,   1,
   "deprecated -- use class View"},
  {"hide",                    (PyCFunction) Application::sHide,             1,
   "deprecated"},
  {"show",                    (PyCFunction) Application::sShow,             1,
   "deprecated"},
  {"hideObject",              (PyCFunction) Application::sHideObject,       1,
   "hideObject(object) -> None\n\n"
   "Hide the view provider to the given object"},
  {"showObject",              (PyCFunction) Application::sShowObject,       1,
   "showObject(object) -> None\n\n"
   "Show the view provider to the given object"},
  {"open",                    (PyCFunction) Application::sOpen,             1,
   "Open a macro, Inventor or VRML file"},
  {"insert",                  (PyCFunction) Application::sInsert,           1,
   "Open a macro, Inventor or VRML file"},
  {"export",                  (PyCFunction) Application::sExport,           1,
   "save scene to Inventor or VRML file"},
  {"activeDocument",          (PyCFunction) Application::sActiveDocument,   1,
   "activeDocument() -> object or None\n\n"
   "Return the active document or None if no one exists"},
  {"setActiveDocument",       (PyCFunction) Application::sSetActiveDocument,1,
   "setActiveDocument(string or App.Document) -> None\n\n"
   "Activate the specified document"},
  {"getDocument",             (PyCFunction) Application::sGetDocument,      1,
   "getDocument(string) -> object\n\n"
   "Get a document by its name"},
  {"doCommand",               (PyCFunction) Application::sDoCommand,        1,
   "doCommand(string) -> None\n\n"
   "Prints the given string in the python console and runs it"},
  {"doCommandGui",               (PyCFunction) Application::sDoCommandGui,  1,
   "doCommandGui(string) -> None\n\n"
   "Prints the given string in the python console and runs it but doesn't record it in macros"},
  {"addModule",               (PyCFunction) Application::sAddModule,        1,
   "addModule(string) -> None\n\n"
   "Prints the given module import only once in the macro recording"},
  {"showDownloads",               (PyCFunction) Application::sShowDownloads,1,
   "showDownloads() -> None\n\n"
   "Shows the downloads manager window"},
  {"showPreferences",               (PyCFunction) Application::sShowPreferences,1,
   "showPreferences([string,int]) -> None\n\n"
   "Shows the preferences window. If string and int are provided, the given page index in the given group is shown."},

  {NULL, NULL}		/* Sentinel */
};

PyObject* Gui::Application::sActiveDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                       // NULL triggers exception 

    Document *pcDoc = Instance->activeDocument();
    if (pcDoc) {
        return pcDoc->getPyObject();
    } else {
        Py_Return;
    }
}

PyObject* Gui::Application::sSetActiveDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sGetDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sHide(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *psFeatStr;
    if (!PyArg_ParseTuple(args, "s;Name of the object to hide has to be given!",&psFeatStr))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

    Document *pcDoc = Instance->activeDocument();

    if (pcDoc)
        pcDoc->setHide(psFeatStr);

    Py_Return;
}

PyObject* Application::sShow(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *psFeatStr;
    if (!PyArg_ParseTuple(args, "s;Name of the object to show has to be given!",&psFeatStr))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

    Document *pcDoc = Instance->activeDocument();

    if (pcDoc)
        pcDoc->setShow(psFeatStr);

    Py_Return;
}

PyObject* Application::sHideObject(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!",&(App::DocumentObjectPy::Type),&object))
        return 0;

    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    Instance->hideViewProvider(obj);

    Py_Return;
}

PyObject* Application::sShowObject(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!",&(App::DocumentObjectPy::Type),&object))
        return 0;

    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    Instance->showViewProvider(obj);

    Py_Return;
}

PyObject* Application::sOpen(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sInsert(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sExport(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sSendActiveView(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *psCommandStr;
    PyObject *suppress=Py_False;
    if (!PyArg_ParseTuple(args, "s|O!",&psCommandStr,&PyBool_Type,&suppress))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

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

PyObject* Application::sGetMainWindow(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PythonWrapper wrap;
    wrap.loadCoreModule();
    wrap.loadGuiModule();
    try {
        return Py::new_reference_to(wrap.fromQWidget(Gui::getMainWindow(), "QMainWindow"));
    }
    catch (const Py::Exception&) {
        return 0;
    }
}

PyObject* Application::sUpdateGui(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

    qApp->processEvents();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sUpdateLocale(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

    Translator::instance()->refresh();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sGetLocale(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

    std::string locale = Translator::instance()->activeLanguage();
    return PyString_FromString(locale.c_str());
}

PyObject* Application::sCreateDialog(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char* fn = 0;
    if (!PyArg_ParseTuple(args, "s", &fn))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

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

PyObject* Application::sAddPreferencePage(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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
    if (PyArg_ParseTuple(args, "O!s", &PyClass_Type, &dlg, &grp)) {
        // add to the preferences dialog
        new PrefPagePyProducer(Py::Object(dlg), grp);

        Py_INCREF(Py_None);
        return Py_None;
    }

    return 0;
}

PyObject* Application::sActivateWorkbenchHandler(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char*       psKey;
    if (!PyArg_ParseTuple(args, "s", &psKey))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

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

PyObject* Application::sAddWorkbenchHandler(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    PyObject*   pcObject;
    std::string item;
    if (!PyArg_ParseTuple(args, "O", &pcObject))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

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

PyObject* Application::sRemoveWorkbenchHandler(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char*       psKey;
    if (!PyArg_ParseTuple(args, "s", &psKey))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

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

PyObject* Application::sGetWorkbenchHandler(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char* psKey;
    if (!PyArg_ParseTuple(args, "s", &psKey))     // convert args: Python->C 
        return NULL;                                // NULL triggers exception 
   
    // get the python workbench object from the dictionary
    PyObject* pcWorkbench = PyDict_GetItemString(Instance->_pcWorkbenchDictionary, psKey);
    if (!pcWorkbench) {
        PyErr_Format(PyExc_KeyError, "No such workbench '%s'", psKey);
        return NULL;
    }

    Py_INCREF(pcWorkbench);
    return pcWorkbench;
}

PyObject* Application::sListWorkbenchHandlers(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    Py_INCREF(Instance->_pcWorkbenchDictionary);
    return Instance->_pcWorkbenchDictionary;
}

PyObject* Application::sActiveWorkbenchHandler(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                       // NULL triggers exception 

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

PyObject* Application::sAddResPath(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char* filePath;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath))     // convert args: Python->C
        return NULL;                    // NULL triggers exception
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

PyObject* Application::sAddLangPath(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char* filePath;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath))     // convert args: Python->C
        return NULL;                    // NULL triggers exception
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

PyObject* Application::sAddIconPath(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char* filePath;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filePath))     // convert args: Python->C
        return NULL;                    // NULL triggers exception
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

PyObject* Application::sAddIcon(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *iconName;
    char *pixmap;
    if (!PyArg_ParseTuple(args, "ss", &iconName,&pixmap))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 
    
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

PyObject* Application::sAddCommand(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char*       pName;
    char*       pSource=0;
    PyObject*   pcCmdObj;
    if (!PyArg_ParseTuple(args, "sO|s", &pName,&pcCmdObj,&pSource))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

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

PyObject* Application::sRunCommand(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char*       pName;
    if (!PyArg_ParseTuple(args, "s", &pName))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

    Command* cmd = Application::Instance->commandManager().getCommandByName(pName);
    if (cmd) {
        cmd->invoke(0);
        Py_INCREF(Py_None);
        return Py_None;
    }
    else {
        PyErr_Format(Base::BaseExceptionFreeCADError, "No such command '%s'", pName);
        return 0;
    }
}

PyObject* Application::sDoCommand(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    char *sCmd=0;
    if (!PyArg_ParseTuple(args, "s", &sCmd))
        return NULL;

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

PyObject* Application::sDoCommandGui(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    char *sCmd=0;
    if (!PyArg_ParseTuple(args, "s", &sCmd))
        return NULL;

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

PyObject* Application::sAddModule(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception
    Command::addModule(Command::Doc,pstr);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sShowDownloads(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))             // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    Gui::Dialog::DownloadManager::getInstance();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sShowPreferences(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr=0;
    int idx=0;
    if (!PyArg_ParseTuple(args, "|si", &pstr, &idx))             // convert args: Python->C 
        return NULL;                             // NULL triggers exception 
    Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
    if (pstr) 
        cDlg.activateGroupPage(QString::fromUtf8(pstr),idx);
    cDlg.exec();

    Py_INCREF(Py_None);
    return Py_None;
}
