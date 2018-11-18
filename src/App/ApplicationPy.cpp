/***************************************************************************
 *   (c) Juergen Riegel (juergen.riegel@web.de) 2002                       *
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
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/



#include "PreCompiled.h"

#ifndef _PreComp_
# include <stdexcept>
#endif


#include "Application.h"
#include "Document.h"
#include "DocumentPy.h"
#include "DocumentObserverPython.h"

// FreeCAD Base header
#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <Base/Factory.h>
#include <Base/FileInfo.h>
#include <Base/UnitsApi.h>

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
    {"getImportType",  (PyCFunction) Application::sGetImportType, METH_VARARGS,
     "Get the name of the module that can import the filetype"},
    {"EndingAdd",      (PyCFunction) Application::sAddImportType, METH_VARARGS, // deprecated
     "deprecated -- use addImportType"},
    {"EndingGet",      (PyCFunction) Application::sGetImportType, METH_VARARGS, // deprecated
     "deprecated -- use getImportType"},
    {"addExportType",  (PyCFunction) Application::sAddExportType, METH_VARARGS,
     "Register filetype for export"},
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
     "* If more than one module can load a file the first one one will be taken.\n"
     "* If no module exists to load the file an exception will be raised."},
    {"open",   (PyCFunction) Application::sOpenDocument, METH_VARARGS,
     "See openDocument(string)"},
    {"openDocument",   (PyCFunction) Application::sOpenDocument, METH_VARARGS,
     "openDocument(string) -> object\n\n"
     "Create a document and load the project file into the document.\n"
     "The string argument must point to an existing file. If the file doesn't exist\n"
     "or the file cannot be loaded an I/O exception is thrown. In this case the\n"
     "document is kept alive."},
//  {"saveDocument",   (PyCFunction) Application::sSaveDocument, METH_VARARGS,
//   "saveDocument(string) -- Save the document to a file."},
//  {"saveDocumentAs", (PyCFunction) Application::sSaveDocumentAs, METH_VARARGS},
    {"newDocument",    (PyCFunction) Application::sNewDocument, METH_VARARGS,
     "newDocument([string]) -> object\n\n"
     "Create a new document with a given name.\n"
     "The document name must be unique which\n"
     "is checked automatically."},
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
     "listDocuments() -> list\n\n"
     "Return a list of names of all documents."},
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
        PyErr_SetString(PyExc_IOError, e.what());
        return 0;
    }
    catch (const std::exception& e) {
        // might be subclass from zipios
        PyErr_Format(PyExc_IOError, "Invalid project file %s: %s", path, e.what());
        return 0;
    }
}

PyObject* Application::sOpenDocument(PyObject * /*self*/, PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    try {
        // return new document
        return (GetApplication().openDocument(EncodedName.c_str())->getPyObject());
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

PyObject* Application::sNewDocument(PyObject * /*self*/, PyObject *args)
{
    char *docName = 0;
    char *usrName = 0;
    if (!PyArg_ParseTuple(args, "|etet", "utf-8", &docName, "utf-8", &usrName))
        return NULL;

    PY_TRY {
        App::Document* doc = GetApplication().newDocument(docName, usrName);
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
#if PY_MAJOR_VERSION >= 3
        return PyUnicode_FromString("");
#else
        return PyString_FromString("");
#endif
    }
}

PyObject* Application::sDumpConfig(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "") )    // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PyObject *dict = PyDict_New();
    for (std::map<std::string,std::string>::iterator It= GetApplication()._mConfig.begin();
         It!=GetApplication()._mConfig.end();++It) {
#if PY_MAJOR_VERSION >= 3
        PyDict_SetItemString(dict,It->first.c_str(), PyUnicode_FromString(It->second.c_str()));
#else
        PyDict_SetItemString(dict,It->first.c_str(), PyString_FromString(It->second.c_str()));
#endif
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
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception
    PY_TRY {
        PyObject *pDict = PyDict_New();
        PyObject *pKey;
        Base::PyObjectBase* pValue;

        for (std::map<std::string,Document*>::const_iterator It = GetApplication().DocMap.begin();
             It != GetApplication().DocMap.end();++It) {
#if PY_MAJOR_VERSION >= 3
            pKey   = PyUnicode_FromString(It->first.c_str());
#else
            pKey   = PyString_FromString(It->first.c_str());
#endif
            // GetPyObject() increments
            pValue = static_cast<Base::PyObjectBase*>(It->second->getPyObject());
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
#if PY_MAJOR_VERSION < 3
        if (PyString_Check(pcObj)) {
            const char *pstr = PyString_AsString(pcObj);
#else
        if (PyUnicode_Check(pcObj)) {
            const char *pstr = PyUnicode_AsUTF8(pcObj);
#endif
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


