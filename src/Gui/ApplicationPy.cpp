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
# include <qfileinfo.h>
# include <qdir.h>
# include <QPrinter>
#endif


#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Document.h"
#include "MainWindow.h"
#include "EditorView.h"
#include "PythonEditor.h"
#include "View3DInventor.h"
#include "WidgetFactory.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "Language/Translator.h"
#include <App/DocumentObjectPy.h>
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
  {"getDocument",             (PyCFunction) Application::sGetDocument,      1,
   "getDocument(string) -> object\n\n"
   "Get a document by its name"},
  {"doCommand",               (PyCFunction) Application::sDoCommand,        1,
   "doCommand(string) -> None\n\n"
   "Prints the given string in the python console and runs it"},
  {"addModule",               (PyCFunction) Application::sAddModule,        1,
   "addModule(string) -> None\n\n"
   "Prints the given module import only once in the macro recording"},

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

PyObject* Application::sGetDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception

    Document *pcDoc = Instance->getDocument(pstr);
    if (!pcDoc) {
        PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
        return 0;
    }

    return pcDoc->getPyObject();
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
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL;
    PY_TRY {
        QString fileName = QString::fromUtf8(Name);
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.completeSuffix().toLower();
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
            //QString cmd = QString("Gui.activeDocument().addAnnotation(\"%1\",\"%2\")").arg(fi.baseName()).arg(fi.absoluteFilePath());
            QString cmd = QString::fromLatin1(
                "App.ActiveDocument.addObject(\"App::VRMLObject\",\"%1\")."
                "VrmlFile=\"%2\"\n"
                "App.ActiveDocument.ActiveObject.Label=\"%1\"\n"
                "App.ActiveDocument.recompute()")
                .arg(fi.baseName()).arg(fi.absoluteFilePath());
            Base::Interpreter().runString(cmd.toUtf8());
        }
        else if (ext == QLatin1String("py") || ext == QLatin1String("fcmacro") ||
                 ext == QLatin1String("fcscript")) {
            PythonEditor* editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().pixmap("python_small"));
            PythonEditorView* edit = new PythonEditorView(editor, getMainWindow());
            edit->open(fileName);
            edit->resize(400, 300);
            getMainWindow()->addWindow( edit );
        }
    } PY_CATCH;

    Py_Return;
}

PyObject* Application::sInsert(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    const char* Name;
    const char* DocName=0;
    if (!PyArg_ParseTuple(args, "s|s",&Name,&DocName))
        return NULL;

    PY_TRY {
        QString fileName = QString::fromUtf8(Name);
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.completeSuffix().toLower();
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

            App::DocumentObject* obj = doc->addObject("App::VRMLObject",
                (const char*)fi.baseName().toUtf8());
            obj->Label.setValue((const char*)fi.baseName().toUtf8());
            static_cast<App::PropertyFileIncluded*>(obj->getPropertyByName("VrmlFile"))
                ->setValue((const char*)fi.absoluteFilePath().toUtf8());
            doc->recompute();
        }
        else if (ext == QLatin1String("py") || ext == QLatin1String("fcmacro") ||
                 ext == QLatin1String("fcscript")) {
            PythonEditor* editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().pixmap("python_small"));
            PythonEditorView* edit = new PythonEditorView(editor, getMainWindow());
            edit->open(fileName);
            edit->resize(400, 300);
            getMainWindow()->addWindow( edit );
        }
    } PY_CATCH;

    Py_Return;
}

PyObject* Application::sExport(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    PyObject* object;
    const char* filename;
    if (!PyArg_ParseTuple(args, "Os",&object,&filename))
        return NULL;

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

        // get the view that belongs to the found document
        if (doc) {
            QString fileName = QString::fromUtf8(filename);
            QFileInfo fi;
            fi.setFile(fileName);
            QString ext = fi.completeSuffix().toLower();
            if (ext == QLatin1String("iv") || ext == QLatin1String("wrl") ||
                ext == QLatin1String("vrml") || ext == QLatin1String("wrz") ||
                ext == QLatin1String("svg") || ext == QLatin1String("idtf")) {
                Gui::Document* gui_doc = Application::Instance->getDocument(doc);
                std::list<MDIView*> view3d = gui_doc->getMDIViewsOfType(View3DInventor::getClassTypeId());
                if (view3d.empty()) {
                    PyErr_SetString(PyExc_Exception, "Cannot export to SVG because document doesn't have a 3d view");
                    return 0;
                }
                else {
                    QString cmd = QString::fromLatin1(
                        "Gui.getDocument(\"%1\").mdiViewsOfType('Gui::View3DInventor')[0].dump(\"%2\")"
                        ).arg(QLatin1String(doc->getName())).arg(fi.absoluteFilePath());
                    Base::Interpreter().runString(cmd.toUtf8());
                }
            }
            else if (ext == QLatin1String("pdf")) {
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
        }
    } PY_CATCH;

    Py_Return;
}

PyObject* Application::sSendActiveView(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *psCommandStr;
    if (!PyArg_ParseTuple(args, "s",&psCommandStr))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

    const char* ppReturn=0;
    if (!Instance->sendMsgToActiveView(psCommandStr,&ppReturn))
        Base::Console().Warning("Unknown view command: %s\n",psCommandStr);

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
    if (!PyArg_ParseTuple(args, "ss", &fn,&grp))     // convert args: Python->C 
        return NULL;                                      // NULL triggers exception 

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
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
    catch (...) {
        PyErr_SetString(PyExc_Exception, "Unknown C++ exception raised in activateWorkbench");
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
        item = name.as_std_string();

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
    if (!PyArg_ParseTuple(args, "s", &filePath))     // convert args: Python->C
        return NULL;                    // NULL triggers exception
    QString path = QString::fromUtf8(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromUtf8(App::GetApplication().GetHomePath());
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
    if (!PyArg_ParseTuple(args, "s", &filePath))     // convert args: Python->C
        return NULL;                    // NULL triggers exception
    QString path = QString::fromUtf8(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromUtf8(App::GetApplication().GetHomePath());
        path = home + path;
    }

    Translator::instance()->addPath(path);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sAddIconPath(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char* filePath;
    if (!PyArg_ParseTuple(args, "s", &filePath))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 
    QString path = QString::fromUtf8(filePath);
    if (QDir::isRelativePath(path)) {
        // Home path ends with '/'
        QString home = QString::fromUtf8(App::GetApplication().GetHomePath());
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
        PyErr_SetString(PyExc_Exception, "Invalid icon added to application");
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
#if 0
    std::string source = (pSource ? pSource : "");

    if (source.empty()) {
        try {
            Py::Module module(PyImport_ImportModule("inspect"),true);
            Py::Dict dict = module.getDict();
            Py::Callable call(dict.getItem("getsourcelines"));
            Py::Tuple arg(1);
            arg.setItem(0, Py::Object(pcCmdObj).getAttr("Activated"));
            Py::Tuple tuple(call.apply(arg));
            Py::List lines(tuple[0]);

            int pos=0;
            std::string code = (std::string)(Py::String(lines[1]));
            while (code[pos] == ' ' || code[pos] == '\t')
                pos++;
            for (Py::List::iterator it = lines.begin()+1; it != lines.end(); ++it) {
                Py::String str(*it);
                source += ((std::string)str).substr(pos);
            }
        }
        catch (Py::Exception& e) {
            e.clear();
        }
    }

    Application::Instance->commandManager().addCommand(new PythonCommand(pName,pcCmdObj,source.c_str()));
#else
    Application::Instance->commandManager().addCommand(new PythonCommand(pName,pcCmdObj,pSource));
#endif
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
        PyErr_Format(PyExc_Exception, "No such command '%s'", pName);
        return 0;
    }
} 

PyObject* Application::sDoCommand(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception
    Command::doCommand(Command::Doc,pstr);
    return Py_None;
}

PyObject* Application::sAddModule(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception
    Command::addModule(Command::Doc,pstr);
    return Py_None;
}
