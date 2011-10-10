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

#define new DEBUG_CLIENTBLOCK
//using Base::GetConsole;
using namespace Base;
using namespace App;



//**************************************************************************
// Python stuff

// Application Methods						// Methods structure
PyMethodDef Application::Methods[] = {
    {"ParamGet",       (PyCFunction) Application::sGetParam,       1,
     "Get parameters by path"},
    {"Version",        (PyCFunction) Application::sGetVersion,     1,
     "Print the version to the output."},
    {"ConfigGet",      (PyCFunction) Application::sGetConfig,      1,
     "ConfigGet([string]) -- Get the value for the given key.\n"
     "If no key is given the complete configuration is dumped to\n"
     "the output."},
    {"ConfigSet",      (PyCFunction) Application::sSetConfig,      1,
     "ConfigSet(string, string) -- Set the given key to the given value."},
    {"ConfigDump",     (PyCFunction) Application::sDumpConfig,     1,
     "Dump the configuration to the output."},
    {"addImportType",  (PyCFunction) Application::sAddImportType,  1,
     "Register filetype for import"},
    {"getImportType",  (PyCFunction) Application::sGetImportType,  1,
     "Get the name of the module that can import the filetype"},
    {"EndingAdd",      (PyCFunction) Application::sAddImportType  ,1, // deprecated
     "deprecated -- use addImportType"},
    {"EndingGet",      (PyCFunction) Application::sGetImportType  ,1, // deprecated
     "deprecated -- use getImportType"},
    {"addExportType",  (PyCFunction) Application::sAddExportType  ,1,
     "Register filetype for export"},
    {"getExportType",  (PyCFunction) Application::sGetExportType  ,1,
     "Get the name of the module that can export the filetype"},
    {"getResourceDir", (PyCFunction) Application::sGetResourceDir  ,1,
     "Get the root directory of all resources"},
    {"getHomePath",    (PyCFunction) Application::sGetHomePath  ,1,
     "Get the home path, i.e. the parent directory of the executable"},

    {"loadFile",       (PyCFunction) Application::sLoadFile,   1,
     "loadFile(string=filename,[string=module]) -> None\n\n"
     "Loads an arbitrary file by delegating to the given Python module:\n"
     "* If no module is given it will be determined by the file extension.\n"
     "* If more than one module can load a file the first one one will be taken.\n"
     "* If no module exists to load the file an exception will be raised."},
    {"open",   (PyCFunction) Application::sOpenDocument,   1,
     "See openDocument(string)"},
    {"openDocument",   (PyCFunction) Application::sOpenDocument,   1,
     "openDocument(string) -> object\n\n"
     "Create a document and load the project file into the document.\n"
     "The string argument must point to an existing file. If the file doesn't exist\n"
     "or the file cannot be loaded an I/O exception is thrown. In this case the\n"
     "document is kept alive."},
//  {"saveDocument",   (PyCFunction) Application::sSaveDocument,   1,
//   "saveDocument(string) -- Save the document to a file."},
//  {"saveDocumentAs", (PyCFunction) Application::sSaveDocumentAs, 1},
    {"newDocument",    (PyCFunction) Application::sNewDocument,    1,
     "newDocument([string]) -> object\n\n"
     "Create a new document with a given name.\n"
     "The document name must be unique which\n"
     "is checked automatically."},
    {"closeDocument",  (PyCFunction) Application::sCloseDocument,  1,
     "closeDocument(string) -> None\n\n"
     "Close the document with a given name."},
    {"activeDocument", (PyCFunction) Application::sActiveDocument, 1,
     "activeDocument() -> object or None\n\n"
     "Return the active document or None if there is no one."},
    {"setActiveDocument",(PyCFunction) Application::sSetActiveDocument, 1,
     "setActiveDocement(string) -> None\n\n"
     "Set the active document by its name."},
    {"getDocument",    (PyCFunction) Application::sGetDocument,    1,
     "getDocument(string) -> object\n\n"
     "Get a document by its name or raise an exception\n"
     "if there is no document with the given name."},
    {"listDocuments",  (PyCFunction) Application::sListDocuments  ,1,
     "listDocuments() -> list\n\n"
     "Return a list of names of all documents."},
    {"addDocumentObserver",  (PyCFunction) Application::sAddDocObserver  ,1,
     "addDocumentObserver() -> None\n\n"
     "Add an observer to get notified about changes on documents."},
    {"removeDocumentObserver",  (PyCFunction) Application::sRemoveDocObserver  ,1,
     "removeDocumentObserver() -> None\n\n"
     "Remove an added document observer."},

    {NULL, NULL, 0, NULL}		/* Sentinel */
};


PyObject* Application::sLoadFile(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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
            std::string ext = fi.extension(false);
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

PyObject* Application::sOpenDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception
    try {
        // return new document
        return (GetApplication().openDocument(pstr)->getPyObject());
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return 0L;
    }
    catch (const std::exception& e) {
        // might be subclass from zipios
        PyErr_Format(PyExc_IOError, "Invalid project file %s: %s\n", pstr, e.what());
        return 0L;
    }
}

PyObject* Application::sNewDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *docName = 0;
    char *usrName = 0;
    if (!PyArg_ParseTuple(args, "|ss", &docName, &usrName))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        return GetApplication().newDocument(docName, usrName)->getPyObject();
    }PY_CATCH;
}

PyObject* Application::sSetActiveDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr = 0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    try {
        GetApplication().setActiveDocument(pstr);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }

    Py_Return;
}

PyObject* Application::sCloseDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sSaveDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pDoc;
    if (!PyArg_ParseTuple(args, "s", &pDoc))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Document* doc = GetApplication().getDocument(pDoc);
    if ( doc ) {
        if ( doc->save() == false ) {
            PyErr_Format(PyExc_Exception, "Cannot save document '%s'", pDoc);
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
PyObject* Application::sSaveDocumentAs(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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
PyObject* Application::sActiveDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sGetDocument(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sGetParam(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr=0;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        return GetPyObject(GetApplication().GetParameterGroupByPath(pstr));
    }PY_CATCH;
}


PyObject* Application::sGetConfig(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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
        return PyString_FromString("");
    }
}

PyObject* Application::sDumpConfig(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, "") )    // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PyObject *dict = PyDict_New();
    for (std::map<std::string,std::string>::iterator It= GetApplication()._mConfig.begin();
         It!=GetApplication()._mConfig.end();It++) {
        PyDict_SetItemString(dict,It->first.c_str(), PyString_FromString(It->second.c_str()));
    }
    return dict;
}

PyObject* Application::sSetConfig(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *pstr,*pstr2;

    if (!PyArg_ParseTuple(args, "ss", &pstr,&pstr2))  // convert args: Python->C
        return NULL; // NULL triggers exception

    GetApplication()._mConfig[pstr] = pstr2;

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Application::sGetVersion(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL; // NULL triggers exception

    PyObject* pList = PyList_New(5);
    PyObject *pItem;
    pItem = PyString_FromString(Application::Config()["BuildVersionMajor"].c_str());
    PyList_SetItem(pList, 0, pItem);
    pItem = PyString_FromString(Application::Config()["BuildVersionMinor"].c_str());
    PyList_SetItem(pList, 1, pItem);
    pItem = PyString_FromString(Application::Config()["BuildRevision"].c_str());
    PyList_SetItem(pList, 2, pItem);
    pItem = PyString_FromString(Application::Config()["BuildRepositoryURL"].c_str());
    PyList_SetItem(pList, 4, pItem);
    pItem = PyString_FromString(Application::Config()["BuildCurrentDate"].c_str());
    PyList_SetItem(pList, 6, pItem);

    return pList;
}


PyObject* Application::sAddImportType(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *psKey,*psMod;

    if (!PyArg_ParseTuple(args, "ss", &psKey,&psMod))
        return NULL;

    GetApplication().addImportType(psKey,psMod);

    Py_Return;
}

PyObject* Application::sGetImportType(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sAddExportType(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    char *psKey,*psMod;

    if (!PyArg_ParseTuple(args, "ss", &psKey,&psMod))
        return NULL;

    GetApplication().addExportType(psKey,psMod);

    Py_Return;
}

PyObject* Application::sGetExportType(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
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

PyObject* Application::sGetResourceDir(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception

    Py::String datadir(Application::getResourceDir());
    return Py::new_reference_to(datadir);
}

PyObject* Application::sGetHomePath(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception

    Py::String homedir(GetApplication().GetHomePath());
    return Py::new_reference_to(homedir);
}

PyObject* Application::sListDocuments(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                       // NULL triggers exception
    PY_TRY {
        PyObject *pDict = PyDict_New();
        PyObject *pKey;
        Base::PyObjectBase* pValue;

        for (std::map<std::string,Document*>::const_iterator It = GetApplication().DocMap.begin();
             It != GetApplication().DocMap.end();++It) {
            pKey   = PyString_FromString(It->first.c_str());
            // GetPyObject() increments
            pValue = static_cast<Base::PyObjectBase*>(It->second->getPyObject());
            PyDict_SetItem(pDict, pKey, pValue);
            // now we can decrement again as PyDict_SetItem also has incremented
            pValue->DecRef();
        }

        return pDict;
    } PY_CATCH;
}

PyObject* Application::sAddDocObserver(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O",&o))
        return NULL;
    PY_TRY {
        DocumentObserverPython::addObserver(Py::Object(o));
        Py_Return;
    } PY_CATCH;
}

PyObject* Application::sRemoveDocObserver(PyObject * /*self*/, PyObject *args,PyObject * /*kwd*/)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O",&o))
        return NULL;
    PY_TRY {
        DocumentObserverPython::removeObserver(Py::Object(o));
        Py_Return;
    } PY_CATCH;
}
