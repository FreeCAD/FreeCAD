/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/



#include "PreCompiled.h"

#ifndef _PreComp_
# include <stdexcept>
#endif


#include "Application.h"
#include "Document.h"
#include "DocumentPy.h"
#include "DocumentObserverPython.h"
#include "DocumentObjectPy.h"

// FreeCAD Base header
#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <Base/Factory.h>
#include <Base/FileInfo.h>
#include <Base/UnitsApi.h>
#include <Base/Sequencer.h>

//using Base::GetConsole;
using namespace Base;
using namespace App;



//**************************************************************************
// Python stuff

// Application Methods						// Methods structure
PyMethodDef Application::Methods[] = {
    {"ParamGet",       (PyCFunction) Application::sGetParam, METH_VARARGS,
     "Get parameters by path"},
    {"saveParameter",  (PyCFunction) Application::sSaveParameter, METH_VARARGS,
     "saveParameter(config='User parameter') -> None\n"
     "Save parameter set to file. The default set is 'User parameter'"},
    {"Version",        (PyCFunction) Application::sGetVersion, METH_VARARGS,
     "Print the version to the output."},
    {"ConfigGet",      (PyCFunction) Application::sGetConfig, METH_VARARGS,
     "ConfigGet(string) -- Get the value for the given key."},
    {"ConfigSet",      (PyCFunction) Application::sSetConfig, METH_VARARGS,
     "ConfigSet(string, string) -- Set the given key to the given value."},
    {"ConfigDump",     (PyCFunction) Application::sDumpConfig, METH_VARARGS,
     "Dump the configuration to the output."},
    {"addImportType",  (PyCFunction) Application::sAddImportType, METH_VARARGS,
     "Register filetype for import"},
    {"changeImportModule",  (PyCFunction) Application::sChangeImportModule, METH_VARARGS,
     "Change the import module name of a registered filetype"},
    {"getImportType",  (PyCFunction) Application::sGetImportType, METH_VARARGS,
     "Get the name of the module that can import the filetype"},
    {"EndingAdd",      (PyCFunction) Application::sAddImportType, METH_VARARGS, // deprecated
     "deprecated -- use addImportType"},
    {"EndingGet",      (PyCFunction) Application::sGetImportType, METH_VARARGS, // deprecated
     "deprecated -- use getImportType"},
    {"addExportType",  (PyCFunction) Application::sAddExportType, METH_VARARGS,
     "Register filetype for export"},
    {"changeExportModule",  (PyCFunction) Application::sChangeExportModule, METH_VARARGS,
     "Change the export module name of a registered filetype"},
    {"getExportType",  (PyCFunction) Application::sGetExportType, METH_VARARGS,
     "Get the name of the module that can export the filetype"},
    {"getResourceDir", (PyCFunction) Application::sGetResourceDir, METH_VARARGS,
     "Get the root directory of all resources"},
    {"getUserAppDataDir", (PyCFunction) Application::sGetUserAppDataDir, METH_VARARGS,
     "Get the root directory of user settings"},
    {"getUserMacroDir", (PyCFunction) Application::sGetUserMacroDir, METH_VARARGS,
     "getUserMacroDir(bool=False) -> string"
     "Get the directory of the user's macro directory\n"
     "If parameter is False (the default) it returns the standard path in the"
     "user's home directory, otherwise it returns the user-defined path."},
    {"getHelpDir", (PyCFunction) Application::sGetHelpDir, METH_VARARGS,
     "Get the directory of the documentation"},
    {"getHomePath",    (PyCFunction) Application::sGetHomePath, METH_VARARGS,
     "Get the home path, i.e. the parent directory of the executable"},

    {"loadFile",       (PyCFunction) Application::sLoadFile, METH_VARARGS,
     "loadFile(string=filename,[string=module]) -> None\n\n"
     "Loads an arbitrary file by delegating to the given Python module:\n"
     "* If no module is given it will be determined by the file extension.\n"
     "* If more than one module can load a file the first one will be taken.\n"
     "* If no module exists to load the file an exception will be raised."},
    {"open",   reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) (void)>( Application::sOpenDocument )), METH_VARARGS|METH_KEYWORDS,
     "See openDocument(string)"},
    {"openDocument",   reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) (void)>( Application::sOpenDocument )), METH_VARARGS|METH_KEYWORDS,
     "openDocument(filepath,hidden=False) -> object\n"
     "Create a document and load the project file into the document.\n\n"
     "filepath: file path to an existing file. If the file doesn't exist\n"
     "          or the file cannot be loaded an I/O exception is thrown.\n"
     "          In this case the document is kept alive.\n"
     "hidden: whether to hide document 3D view."},
//  {"saveDocument",   (PyCFunction) Application::sSaveDocument, METH_VARARGS,
//   "saveDocument(string) -- Save the document to a file."},
//  {"saveDocumentAs", (PyCFunction) Application::sSaveDocumentAs, METH_VARARGS},
    {"newDocument",    reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) (void)>( Application::sNewDocument )), METH_VARARGS|METH_KEYWORDS,
     "newDocument(name, label=None, hidden=False, temp=False) -> object\n"
     "Create a new document with a given name.\n\n"
     "name: unique document name which is checked automatically.\n"
     "label: optional user changeable label for the document.\n"
     "hidden: whether to hide document 3D view.\n"
     "temp: mark the document as temporary so that it will not be saved"},
    {"closeDocument",  (PyCFunction) Application::sCloseDocument, METH_VARARGS,
     "closeDocument(string) -> None\n\n"
     "Close the document with a given name."},
    {"activeDocument", (PyCFunction) Application::sActiveDocument, METH_VARARGS,
     "activeDocument() -> object or None\n\n"
     "Return the active document or None if there is no one."},
    {"setActiveDocument",(PyCFunction) Application::sSetActiveDocument, METH_VARARGS,
     "setActiveDocement(string) -> None\n\n"
     "Set the active document by its name."},
    {"getDocument",    (PyCFunction) Application::sGetDocument, METH_VARARGS,
     "getDocument(string) -> object\n\n"
     "Get a document by its name or raise an exception\n"
     "if there is no document with the given name."},
    {"listDocuments",  (PyCFunction) Application::sListDocuments, METH_VARARGS,
     "listDocuments(sort=False) -> list\n\n"
     "Return a list of names of all documents, optionally sort in dependency order."},
    {"addDocumentObserver",  (PyCFunction) Application::sAddDocObserver, METH_VARARGS,
     "addDocumentObserver() -> None\n\n"
     "Add an observer to get notified about changes on documents."},
    {"removeDocumentObserver",  (PyCFunction) Application::sRemoveDocObserver, METH_VARARGS,
     "removeDocumentObserver() -> None\n\n"
     "Remove an added document observer."},
    {"setLogLevel",          (PyCFunction) Application::sSetLogLevel, METH_VARARGS,
     "setLogLevel(tag, level) -- Set the log level for a string tag.\n"
     "'level' can either be string 'Log', 'Msg', 'Wrn', 'Error', or an integer value"},
    {"getLogLevel",          (PyCFunction) Application::sGetLogLevel, METH_VARARGS,
     "getLogLevel(tag) -- Get the log level of a string tag"},
    {"checkLinkDepth",       (PyCFunction) Application::sCheckLinkDepth, METH_VARARGS,
     "checkLinkDepth(depth) -- check link recursion depth"},
    {"getLinksTo",       (PyCFunction) Application::sGetLinksTo, METH_VARARGS,
     "getLinksTo(obj,options=0,maxCount=0) -- return the objects linked to 'obj'\n\n"
     "options: 1: recursive, 2: check link array. Options can combine.\n"
     "maxCount: to limit the number of links returned\n"},
    {"getDependentObjects", (PyCFunction) Application::sGetDependentObjects, METH_VARARGS,
     "getDependentObjects(obj|[obj,...], options=0)\n"
     "Return a list of dependent objects including the given objects.\n\n"
     "options: can have the following bit flags,\n"
     "         1: to sort the list in topological order.\n"
     "         2: to exclude dependency of Link type object."},
    {"setActiveTransaction", (PyCFunction) Application::sSetActiveTransaction, METH_VARARGS,
     "setActiveTransaction(name, persist=False) -- setup active transaction with the given name\n\n"
     "name: the transaction name\n"
     "persist(False): by default, if the calling code is inside any invocation of a command, it\n"
     "                will be auto closed once all commands within the current stack exists. To\n"
     "                disable auto closing, set persist=True\n"
     "Returns the transaction ID for the active transaction. An application-wide\n"
     "active transaction causes any document changes to open a transaction with\n"
     "the given name and ID."},
    {"getActiveTransaction", (PyCFunction) Application::sGetActiveTransaction, METH_VARARGS,
     "getActiveTransaction() -> (name,id) return the current active transaction name and ID"},
    {"closeActiveTransaction", (PyCFunction) Application::sCloseActiveTransaction, METH_VARARGS,
     "closeActiveTransaction(abort=False) -- commit or abort current active transaction"},
    {"isRestoring", (PyCFunction) Application::sIsRestoring, METH_VARARGS,
     "isRestoring() -> Bool -- Test if the application is opening some document"},
    {"checkAbort", (PyCFunction) Application::sCheckAbort, METH_VARARGS,
     "checkAbort() -- check for user abort in length operation.\n\n"
     "This only works if there is an active sequencer (or ProgressIndicator in Python).\n"
     "There is an active sequencer during document restore and recomputation. User may\n"
     "abort the operation by pressing the ESC key. Once detected, this function will\n"
     "trigger a BaseExceptionFreeCADAbort exception."},
    {NULL, NULL, 0, NULL}		/* Sentinel */
};


PyObject* Application::sLoadFile(PyObject * /*self*/, PyObject *args)
{
    char *path, *doc="",*mod="";
    if (!PyArg_ParseTuple(args, "s|ss", &path, &doc, &mod))     // convert args: Python->C
        return 0;                             // NULL triggers exception
    try {
        Base::FileInfo fi(path);
        if (!fi.isFile() || !fi.exists()) {
            PyErr_Format(PyExc_IOError, "File %s doesn't exist.", path);
            return 0;
        }

        std::string module = mod;
        if (module.empty()) {
            std::string ext = fi.extension();
            std::vector<std::string> modules = GetApplication().getImportModules(ext.c_str());
            if (modules.empty()) {
                PyErr_Format(PyExc_IOError, "Filetype %s is not supported.", ext.c_str());
                return 0;
            }
            else {
                module = modules.front();
            }
        }

        std::stringstream str;
        str << "import " << module << std::endl;
        if (fi.hasExtension("FCStd"))
            str << module << ".openDocument('" << path << "')" << std::endl;
        else
            str << module << ".insert('" << path << "','" << doc << "')" << std::endl;
        Base::Interpreter().runString(str.str().c_str());
        Py_Return;
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    catch (const std::exception& e) {
        // might be subclass from zipios
        PyErr_Format(PyExc_IOError, "Invalid project file %s: %s", path, e.what());
        return 0;
    }
}

PyObject* Application::sIsRestoring(PyObject * /*self*/, PyObject *args) {
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    return Py::new_reference_to(Py::Boolean(GetApplication().isRestoring()));
}

PyObject* Application::sOpenDocument(PyObject * /*self*/, PyObject *args, PyObject *kwd)
{
    char* Name;
    PyObject *hidden = Py_False;
    static char *kwlist[] = {"name","hidden",0};
    if (!PyArg_ParseTupleAndKeywords(args, kwd, "et|O", kwlist,
                "utf-8", &Name, &hidden))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    try {
        // return new document
        return (GetApplication().openDocument(EncodedName.c_str(),!PyObject_IsTrue(hidden))->getPyObject());
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return 0L;
    }
    catch (const std::exception& e) {
        // might be subclass from zipios
        PyErr_Format(PyExc_IOError, "Invalid project file %s: %s\n", EncodedName.c_str(), e.what());
        return 0L;
    }
}

PyObject* Application::sNewDocument(PyObject * /*self*/, PyObject *args, PyObject *kwd)
{
    char *docName = 0;
    char *usrName = 0;
    PyObject *hidden = Py_False;
    PyObject *temp = Py_False;
    static char *kwlist[] = {"name","label","hidden","temp",0};
    if (!PyArg_ParseTupleAndKeywords(args, kwd, "|etetOO", kwlist,
                "utf-8", &docName, "utf-8", &usrName, &hidden, &temp))
        return NULL;

    PY_TRY {
        App::Document* doc = GetApplication().newDocument(docName, usrName,
                                                          !PyObject_IsTrue(hidden),
                                                          PyObject_IsTrue(temp));
        PyMem_Free(docName);
        PyMem_Free(usrName);
        return doc->getPyObject();
    }PY_CATCH;
}

PyObject* Application::sSetActiveDocument(PyObject * /*self*/, PyObject *args)
{
    char *pstr = 0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    try {
        GetApplication().setActiveDocument(pstr);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return NULL;
    }

    Py_Return;
}

PyObject* Application::sCloseDocument(PyObject * /*self*/, PyObject *args)
{
    char *pstr = 0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Document* doc = GetApplication().getDocument(pstr);
    if (!doc) {
        PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
        return NULL;
    }
    if (!doc->isClosable()) {
        PyErr_Format(PyExc_RuntimeError, "The document '%s' is not closable for the moment", pstr);
        return NULL;
    }

    if (GetApplication().closeDocument(pstr) == false) {
        PyErr_Format(PyExc_RuntimeError, "Closing the document '%s' failed", pstr);
        return NULL;
    }

    Py_Return;
}

PyObject* Application::sSaveDocument(PyObject * /*self*/, PyObject *args)
{
    char *pDoc;
    if (!PyArg_ParseTuple(args, "s", &pDoc))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Document* doc = GetApplication().getDocument(pDoc);
    if ( doc ) {
        if ( doc->save() == false ) {
            PyErr_Format(Base::BaseExceptionFreeCADError, "Cannot save document '%s'", pDoc);
            return 0L;
        }
    }
    else {
        PyErr_Format(PyExc_NameError, "Unknown document '%s'", pDoc);
        return NULL;
    }

    Py_Return;
}
#if 0
PyObject* Application::sSaveDocumentAs(PyObject * /*self*/, PyObject *args)
{
    char *pDoc, *pFileName;
    if (!PyArg_ParseTuple(args, "ss", &pDoc, &pFileName))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Document* doc = GetApplication().getDocument(pDoc);
    if (doc) {
        doc->saveAs( pFileName );
    }
    else {
        PyErr_Format(PyExc_NameError, "Unknown document '%s'", pDoc);
        return NULL;
    }

    Py_Return;
}
#endif
PyObject* Application::sActiveDocument(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception

    Document* doc = GetApplication().getActiveDocument();
    if (doc) {
        return doc->getPyObject();
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* Application::sGetDocument(PyObject * /*self*/, PyObject *args)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Document* doc = GetApplication().getDocument(pstr);
    if ( !doc ) {
        PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
        return 0L;
    }

    return doc->getPyObject();
}

PyObject* Application::sGetParam(PyObject * /*self*/, PyObject *args)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        return GetPyObject(GetApplication().GetParameterGroupByPath(pstr));
    }PY_CATCH;
}

PyObject* Application::sSaveParameter(PyObject * /*self*/, PyObject *args)
{
    char *pstr = "User parameter";
    if (!PyArg_ParseTuple(args, "|s", &pstr))
        return NULL;

    PY_TRY {
        ParameterManager* param = App::GetApplication().GetParameterSet(pstr);
        if (!param) {
            std::stringstream str;
            str << "No parameter set found with name: " << pstr;
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return NULL;
        }
        else if (!param->HasSerializer()) {
            std::stringstream str;
            str << "Parameter set cannot be serialized: " << pstr;
            PyErr_SetString(PyExc_RuntimeError, str.str().c_str());
            return NULL;
        }

        param->SaveDocument();
        Py_INCREF(Py_None);
        return Py_None;
    }PY_CATCH;
}


PyObject* Application::sGetConfig(PyObject * /*self*/, PyObject *args)
{
    char *pstr;

    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception
    const std::map<std::string, std::string>& Map = GetApplication().Config();

    std::map<std::string, std::string>::const_iterator it = Map.find(pstr);
    if (it != Map.end()) {
        return Py_BuildValue("s",it->second.c_str());
    }
    else {
        // do not set an error because this may break existing python code
        return PyUnicode_FromString("");
    }
}

PyObject* Application::sDumpConfig(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "") )    // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PyObject *dict = PyDict_New();
    for (std::map<std::string,std::string>::iterator It= GetApplication()._mConfig.begin();
         It!=GetApplication()._mConfig.end();++It) {
        PyDict_SetItemString(dict,It->first.c_str(), PyUnicode_FromString(It->second.c_str()));
    }
    return dict;
}

PyObject* Application::sSetConfig(PyObject * /*self*/, PyObject *args)
{
    char *pstr,*pstr2;

    if (!PyArg_ParseTuple(args, "ss", &pstr,&pstr2))  // convert args: Python->C
        return NULL; // NULL triggers exception

    GetApplication()._mConfig[pstr] = pstr2;

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sGetVersion(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL; // NULL triggers exception

    Py::List list;
    const std::map<std::string, std::string>& cfg = Application::Config();
    std::map<std::string, std::string>::const_iterator it;

    it = cfg.find("BuildVersionMajor");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildVersionMinor");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildRevision");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildRepositoryURL");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildRevisionDate");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildRevisionBranch");
    if (it != cfg.end())
        list.append(Py::String(it->second));

    it = cfg.find("BuildRevisionHash");
    if (it != cfg.end())
        list.append(Py::String(it->second));

    return Py::new_reference_to(list);
}

PyObject* Application::sAddImportType(PyObject * /*self*/, PyObject *args)
{
    char *psKey,*psMod;

    if (!PyArg_ParseTuple(args, "ss", &psKey,&psMod))
        return NULL;

    GetApplication().addImportType(psKey,psMod);

    Py_Return;
}

PyObject* Application::sChangeImportModule(PyObject * /*self*/, PyObject *args)
{
    char *key,*oldMod,*newMod;

    if (!PyArg_ParseTuple(args, "sss", &key,&oldMod,&newMod))
        return nullptr;

    GetApplication().changeImportModule(key,oldMod,newMod);

    Py_Return;
}

PyObject* Application::sGetImportType(PyObject * /*self*/, PyObject *args)
{
    char*       psKey=0;

    if (!PyArg_ParseTuple(args, "|s", &psKey))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    if (psKey) {
        Py::List list;
        std::vector<std::string> modules = GetApplication().getImportModules(psKey);
        for (std::vector<std::string>::iterator it = modules.begin(); it != modules.end(); ++it) {
            list.append(Py::String(*it));
        }

        return Py::new_reference_to(list);
    }
    else {
        Py::Dict dict;
        std::vector<std::string> types = GetApplication().getImportTypes();
        for (std::vector<std::string>::iterator it = types.begin(); it != types.end(); ++it) {
            std::vector<std::string> modules = GetApplication().getImportModules(it->c_str());
            if (modules.empty()) {
                dict.setItem(it->c_str(), Py::None());
            }
            else if (modules.size() == 1) {
                dict.setItem(it->c_str(), Py::String(modules.front()));
            }
            else {
                Py::List list;
                for (std::vector<std::string>::iterator jt = modules.begin(); jt != modules.end(); ++jt) {
                    list.append(Py::String(*jt));
                }
                dict.setItem(it->c_str(), list);
            }
        }

        return Py::new_reference_to(dict);
    }
}

PyObject* Application::sAddExportType(PyObject * /*self*/, PyObject *args)
{
    char *psKey,*psMod;

    if (!PyArg_ParseTuple(args, "ss", &psKey,&psMod))
        return NULL;

    GetApplication().addExportType(psKey,psMod);

    Py_Return;
}

PyObject* Application::sChangeExportModule(PyObject * /*self*/, PyObject *args)
{
    char *key,*oldMod,*newMod;

    if (!PyArg_ParseTuple(args, "sss", &key,&oldMod,&newMod))
        return nullptr;

    GetApplication().changeExportModule(key,oldMod,newMod);

    Py_Return;
}

PyObject* Application::sGetExportType(PyObject * /*self*/, PyObject *args)
{
    char*       psKey=0;

    if (!PyArg_ParseTuple(args, "|s", &psKey))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    if (psKey) {
        Py::List list;
        std::vector<std::string> modules = GetApplication().getExportModules(psKey);
        for (std::vector<std::string>::iterator it = modules.begin(); it != modules.end(); ++it) {
            list.append(Py::String(*it));
        }

        return Py::new_reference_to(list);
    }
    else {
        Py::Dict dict;
        std::vector<std::string> types = GetApplication().getExportTypes();
        for (std::vector<std::string>::iterator it = types.begin(); it != types.end(); ++it) {
            std::vector<std::string> modules = GetApplication().getExportModules(it->c_str());
            if (modules.empty()) {
                dict.setItem(it->c_str(), Py::None());
            }
            else if (modules.size() == 1) {
                dict.setItem(it->c_str(), Py::String(modules.front()));
            }
            else {
                Py::List list;
                for (std::vector<std::string>::iterator jt = modules.begin(); jt != modules.end(); ++jt) {
                    list.append(Py::String(*jt));
                }
                dict.setItem(it->c_str(), list);
            }
        }

        return Py::new_reference_to(dict);
    }
}

PyObject* Application::sGetResourceDir(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception

    Py::String datadir(Application::getResourceDir(),"utf-8");
    return Py::new_reference_to(datadir);
}

PyObject* Application::sGetUserAppDataDir(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception

    Py::String user_data_dir(Application::getUserAppDataDir(),"utf-8");
    return Py::new_reference_to(user_data_dir);
}

PyObject* Application::sGetUserMacroDir(PyObject * /*self*/, PyObject *args)
{
    PyObject *actual = Py_False;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &actual))
        return NULL;

    std::string macroDir = Application::getUserMacroDir();
    if (PyObject_IsTrue(actual)) {
        macroDir = App::GetApplication().
            GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
            ->GetASCII("MacroPath",macroDir.c_str());
    }

    Py::String user_macro_dir(macroDir,"utf-8");
    return Py::new_reference_to(user_macro_dir);
}

PyObject* Application::sGetHelpDir(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception

    Py::String user_macro_dir(Application::getHelpDir(),"utf-8");
    return Py::new_reference_to(user_macro_dir);
}

PyObject* Application::sGetHomePath(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception

    Py::String homedir(GetApplication().getHomePath(),"utf-8");
    return Py::new_reference_to(homedir);
}

PyObject* Application::sListDocuments(PyObject * /*self*/, PyObject *args)
{
    PyObject *sort = Py_False;
    if (!PyArg_ParseTuple(args, "|O",&sort))     // convert args: Python->C
        return NULL;                       // NULL triggers exception
    PY_TRY {
        PyObject *pDict = PyDict_New();
        PyObject *pKey;
        Base::PyObjectBase* pValue;

        std::vector<Document*> docs = GetApplication().getDocuments();;
        if(PyObject_IsTrue(sort))
            docs = Document::getDependentDocuments(docs,true);

        for (auto doc : docs) {
            pKey   = PyUnicode_FromString(doc->getName());
            // GetPyObject() increments
            pValue = static_cast<Base::PyObjectBase*>(doc->getPyObject());
            PyDict_SetItem(pDict, pKey, pValue);
            // now we can decrement again as PyDict_SetItem also has incremented
            pValue->DecRef();
        }

        return pDict;
    } PY_CATCH;
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

PyObject *Application::sSetLogLevel(PyObject * /*self*/, PyObject *args)
{
    char *tag;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "sO", &tag, &pcObj))
        return NULL;
    PY_TRY{
        int l;
        if (PyUnicode_Check(pcObj)) {
            const char *pstr = PyUnicode_AsUTF8(pcObj);
            if(strcmp(pstr,"Log") == 0)
                l = FC_LOGLEVEL_LOG;
            else if(strcmp(pstr,"Warning") == 0)
                l = FC_LOGLEVEL_WARN;
            else if(strcmp(pstr,"Message") == 0)
                l = FC_LOGLEVEL_MSG;
            else if(strcmp(pstr,"Error") == 0)
                l = FC_LOGLEVEL_ERR;
            else if(strcmp(pstr,"Trace") == 0)
                l = FC_LOGLEVEL_TRACE;
            else if(strcmp(pstr,"Default") == 0)
                l = FC_LOGLEVEL_DEFAULT;
            else {
                Py_Error(Base::BaseExceptionFreeCADError,
                        "Unknown Log Level (use 'Default', 'Error', 'Warning', 'Message', 'Log', 'Trace' or an integer)");
                return NULL;
            }
        }else
            l = PyLong_AsLong(pcObj);
        GetApplication().GetParameterGroupByPath("User parameter:BaseApp/LogLevels")->SetInt(tag,l);
        if(strcmp(tag,"Default") == 0) {
#ifndef FC_DEBUG
            if(l>=0) Base::Console().SetDefaultLogLevel(l);
#endif
        }else if(strcmp(tag,"DebugDefault") == 0) {
#ifdef FC_DEBUG
            if(l>=0) Base::Console().SetDefaultLogLevel(l);
#endif
        }else
            *Base::Console().GetLogLevel(tag) = l;
        Py_INCREF(Py_None);
        return Py_None;
    }PY_CATCH;
}

PyObject *Application::sGetLogLevel(PyObject * /*self*/, PyObject *args)
{
    char *tag;
    if (!PyArg_ParseTuple(args, "s", &tag))
        return NULL;

    PY_TRY{
        int l = -1;
        if(strcmp(tag,"Default")==0) {
#ifdef FC_DEBUG
            l = _pcUserParamMngr->GetGroup("BaseApp/LogLevels")->GetInt(tag,-1);
#endif
        }else if(strcmp(tag,"DebugDefault")==0) {
#ifndef FC_DEBUG
            l = _pcUserParamMngr->GetGroup("BaseApp/LogLevels")->GetInt(tag,-1);
#endif
        }else{
            int *pl = Base::Console().GetLogLevel(tag,false);
            l = pl?*pl:-1;
        }
        // For performance reason, we only output integer value
        return Py_BuildValue("i",Base::Console().LogLevel(l));

        // switch(l) {
        // case FC_LOGLEVEL_LOG:
        //     return Py_BuildValue("s","Log");
        // case FC_LOGLEVEL_WARN:
        //     return Py_BuildValue("s","Warning");
        // case FC_LOGLEVEL_ERR:
        //     return Py_BuildValue("s","Error");
        // case FC_LOGLEVEL_MSG:
        //     return Py_BuildValue("s","Message");
        // case FC_LOGLEVEL_TRACE:
        //     return Py_BuildValue("s","Trace");
        // default:
        //     return Py_BuildValue("i",l);
        // }
    } PY_CATCH;
}

PyObject *Application::sCheckLinkDepth(PyObject * /*self*/, PyObject *args)
{
    short depth = 0;
    if (!PyArg_ParseTuple(args, "h", &depth))
        return NULL;

    PY_TRY {
        return Py::new_reference_to(Py::Int(GetApplication().checkLinkDepth(depth,false)));
    }PY_CATCH;
}

PyObject *Application::sGetLinksTo(PyObject * /*self*/, PyObject *args)
{
    PyObject *pyobj = Py_None;
    int options = 0;
    short count = 0;
    if (!PyArg_ParseTuple(args, "|Oih",&pyobj,&options, &count))
        return NULL;

    PY_TRY {
        DocumentObject *obj = 0;
        if(pyobj!=Py_None) {
            if(!PyObject_TypeCheck(pyobj,&DocumentObjectPy::Type)) {
                PyErr_SetString(PyExc_TypeError, "Expect the first argument of type document object");
                return 0;
            }
            obj = static_cast<DocumentObjectPy*>(pyobj)->getDocumentObjectPtr();
        }
        auto links = GetApplication().getLinksTo(obj,options,count);
        Py::Tuple ret(links.size());
        int i=0;
        for(auto o : links)
            ret.setItem(i++,Py::Object(o->getPyObject(),true));
        return Py::new_reference_to(ret);
    }PY_CATCH;
}

PyObject *Application::sGetDependentObjects(PyObject * /*self*/, PyObject *args)
{
    PyObject *obj;
    int options = 0;
    if (!PyArg_ParseTuple(args, "O|i", &obj,&options))
        return 0;

    std::vector<App::DocumentObject*> objs;
    if(PySequence_Check(obj)) {
        Py::Sequence seq(obj);
        for(Py_ssize_t i=0;i<seq.size();++i) {
            if(!PyObject_TypeCheck(seq[i].ptr(),&DocumentObjectPy::Type)) {
                PyErr_SetString(PyExc_TypeError, "Expect element in sequence to be of type document object");
                return 0;
            }
            objs.push_back(static_cast<DocumentObjectPy*>(seq[i].ptr())->getDocumentObjectPtr());
        }
    }else if(!PyObject_TypeCheck(obj,&DocumentObjectPy::Type)) {
        PyErr_SetString(PyExc_TypeError,
            "Expect first argument to be either a document object or sequence of document objects");
        return 0;
    }else
        objs.push_back(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr());

    PY_TRY {
        auto ret = App::Document::getDependencyList(objs,options);

        Py::Tuple tuple(ret.size());
        for(size_t i=0;i<ret.size();++i)
            tuple.setItem(i,Py::Object(ret[i]->getPyObject(),true));
        return Py::new_reference_to(tuple);
    } PY_CATCH;
}


PyObject *Application::sSetActiveTransaction(PyObject * /*self*/, PyObject *args)
{
    char *name;
    PyObject *persist = Py_False;
    if (!PyArg_ParseTuple(args, "s|O", &name,&persist))
        return 0;

    PY_TRY {
        Py::Int ret(GetApplication().setActiveTransaction(name,PyObject_IsTrue(persist)));
        return Py::new_reference_to(ret);
    }PY_CATCH;
}

PyObject *Application::sGetActiveTransaction(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    PY_TRY {
        int id = 0;
        const char *name = GetApplication().getActiveTransaction(&id);
        if(!name || id<=0)
            Py_Return;
        Py::Tuple ret(2);
        ret.setItem(0,Py::String(name));
        ret.setItem(1,Py::Int(id));
        return Py::new_reference_to(ret);
    }PY_CATCH;
}

PyObject *Application::sCloseActiveTransaction(PyObject * /*self*/, PyObject *args)
{
    PyObject *abort = Py_False;
    int id = 0;
    if (!PyArg_ParseTuple(args, "|Oi", &abort,&id))
        return 0;

    PY_TRY {
        GetApplication().closeActiveTransaction(PyObject_IsTrue(abort),id);
        Py_Return;
    } PY_CATCH;
}

PyObject *Application::sCheckAbort(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    PY_TRY {
        Base::Sequencer().checkAbort();
        Py_Return;
    }PY_CATCH
}
