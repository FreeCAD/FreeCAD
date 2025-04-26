// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <FCConfig.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/Sequencer.h>

#include "Application.h"
#include "ApplicationPy.h"
#include "DocumentPy.h"
#include "DocumentObserverPython.h"
#include "DocumentObjectPy.h"


// using Base::GetConsole;
using namespace Base;
using namespace App;


//**************************************************************************
// Python stuff

// Application methods structure
PyMethodDef ApplicationPy::Methods[] = {
    {"ParamGet", (PyCFunction)ApplicationPy::sGetParam, METH_VARARGS, "Get parameters by path"},
    {"saveParameter",
     (PyCFunction)ApplicationPy::sSaveParameter,
     METH_VARARGS,
     "saveParameter(config='User parameter') -> None\n"
     "Save parameter set to file. The default set is 'User parameter'"},
    {"Version",
     (PyCFunction)ApplicationPy::sGetVersion,
     METH_VARARGS,
     "Print the version to the output."},
    {"ConfigGet",
     (PyCFunction)ApplicationPy::sGetConfig,
     METH_VARARGS,
     "ConfigGet(string) -- Get the value for the given key."},
    {"ConfigSet",
     (PyCFunction)ApplicationPy::sSetConfig,
     METH_VARARGS,
     "ConfigSet(string, string) -- Set the given key to the given value."},
    {"ConfigDump",
     (PyCFunction)ApplicationPy::sDumpConfig,
     METH_VARARGS,
     "Dump the configuration to the output."},
    {"addImportType",
     (PyCFunction)ApplicationPy::sAddImportType,
     METH_VARARGS,
     "Register filetype for import"},
    {"changeImportModule",
     (PyCFunction)ApplicationPy::sChangeImportModule,
     METH_VARARGS,
     "Change the import module name of a registered filetype"},
    {"getImportType",
     (PyCFunction)ApplicationPy::sGetImportType,
     METH_VARARGS,
     "Get the name of the module that can import the filetype"},
    {"addExportType",
     (PyCFunction)ApplicationPy::sAddExportType,
     METH_VARARGS,
     "Register filetype for export"},
    {"changeExportModule",
     (PyCFunction)ApplicationPy::sChangeExportModule,
     METH_VARARGS,
     "Change the export module name of a registered filetype"},
    {"getExportType",
     (PyCFunction)ApplicationPy::sGetExportType,
     METH_VARARGS,
     "Get the name of the module that can export the filetype"},
    {"getResourceDir",
     (PyCFunction)ApplicationPy::sGetResourcePath,
     METH_VARARGS,
     "Get the root directory of all resources"},
    {"getLibraryDir",
     (PyCFunction)ApplicationPy::sGetLibraryPath,
     METH_VARARGS,
     "Get the directory of all extension modules"},
    {"getTempPath",
     (PyCFunction)ApplicationPy::sGetTempPath,
     METH_VARARGS,
     "Get the root directory of cached files"},
    {"getUserCachePath",
     (PyCFunction)ApplicationPy::sGetUserCachePath,
     METH_VARARGS,
     "Get the root path of cached files"},
    {"getUserConfigDir",
     (PyCFunction)ApplicationPy::sGetUserConfigPath,
     METH_VARARGS,
     "Get the root path of user config files"},
    {"getUserAppDataDir",
     (PyCFunction)ApplicationPy::sGetUserAppDataPath,
     METH_VARARGS,
     "Get the root directory of application data"},
    {"getUserMacroDir",
     (PyCFunction)ApplicationPy::sGetUserMacroPath,
     METH_VARARGS,
     "getUserMacroDir(bool=False) -> str\n\n"
     "Get the directory of the user's macro directory\n"
     "If parameter is False (the default) it returns the standard path in the"
     "user's home directory, otherwise it returns the user-defined path."},
    {"getHelpDir",
     (PyCFunction)ApplicationPy::sGetHelpPath,
     METH_VARARGS,
     "Get the directory of the documentation"},
    {"getHomePath",
     (PyCFunction)ApplicationPy::sGetHomePath,
     METH_VARARGS,
     "Get the home path, i.e. the parent directory of the executable"},

    {"loadFile",
     (PyCFunction)ApplicationPy::sLoadFile,
     METH_VARARGS,
     "loadFile(string=filename,[string=module]) -> None\n\n"
     "Loads an arbitrary file by delegating to the given Python module:\n"
     "* If no module is given it will be determined by the file extension.\n"
     "* If more than one module can load a file the first one will be taken.\n"
     "* If no module exists to load the file an exception will be raised."},
    {"open",
     reinterpret_cast<PyCFunction>(reinterpret_cast<void (*)()>(ApplicationPy::sOpenDocument)),
     METH_VARARGS | METH_KEYWORDS,
     "See openDocument(string)"},
    {"openDocument",
     reinterpret_cast<PyCFunction>(reinterpret_cast<void (*)()>(ApplicationPy::sOpenDocument)),
     METH_VARARGS | METH_KEYWORDS,
     "openDocument(filepath,hidden=False,temporary=False) -> object\n"
     "Create a document and load the project file into the document.\n\n"
     "filepath: file path to an existing file. If the file doesn't exist\n"
     "          or the file cannot be loaded an I/O exception is thrown.\n"
     "          In this case the document is kept alive.\n"
     "hidden: whether to hide document 3D view.\n"
     "temporary: whether to hide document in the tree view."},
    //  {"saveDocument",   (PyCFunction) ApplicationPy::sSaveDocument, METH_VARARGS,
    //   "saveDocument(string) -- Save the document to a file."},
    //  {"saveDocumentAs", (PyCFunction) ApplicationPy::sSaveDocumentAs, METH_VARARGS},
    {"newDocument",
     reinterpret_cast<PyCFunction>(reinterpret_cast<void (*)()>(ApplicationPy::sNewDocument)),
     METH_VARARGS | METH_KEYWORDS,
     "newDocument(name, label=None, hidden=False, temp=False) -> object\n"
     "Create a new document with a given name.\n\n"
     "name: unique document name which is checked automatically.\n"
     "label: optional user changeable label for the document.\n"
     "hidden: whether to hide document 3D view.\n"
     "temp: mark the document as temporary so that it will not be saved"},
    {"closeDocument",
     (PyCFunction)ApplicationPy::sCloseDocument,
     METH_VARARGS,
     "closeDocument(string) -> None\n\n"
     "Close the document with a given name."},
    {"activeDocument",
     (PyCFunction)ApplicationPy::sActiveDocument,
     METH_VARARGS,
     "activeDocument() -> object or None\n\n"
     "Return the active document or None if there is no one."},
    {"setActiveDocument",
     (PyCFunction)ApplicationPy::sSetActiveDocument,
     METH_VARARGS,
     "setActiveDocement(string) -> None\n\n"
     "Set the active document by its name."},
    {"getDocument",
     (PyCFunction)ApplicationPy::sGetDocument,
     METH_VARARGS,
     "getDocument(string) -> object\n\n"
     "Get a document by its name or raise an exception\n"
     "if there is no document with the given name."},
    {"listDocuments",
     (PyCFunction)ApplicationPy::sListDocuments,
     METH_VARARGS,
     "listDocuments(sort=False) -> list\n\n"
     "Return a list of names of all documents, optionally sort in dependency order."},
    {"addDocumentObserver",
     (PyCFunction)ApplicationPy::sAddDocObserver,
     METH_VARARGS,
     "addDocumentObserver() -> None\n\n"
     "Add an observer to get notified about changes on documents."},
    {"removeDocumentObserver",
     (PyCFunction)ApplicationPy::sRemoveDocObserver,
     METH_VARARGS,
     "removeDocumentObserver() -> None\n\n"
     "Remove an added document observer."},
    {"setLogLevel",
     (PyCFunction)ApplicationPy::sSetLogLevel,
     METH_VARARGS,
     "setLogLevel(tag, level) -- Set the log level for a string tag.\n"
     "'level' can either be string 'Log', 'Msg', 'Wrn', 'Error', or an integer value"},
    {"getLogLevel",
     (PyCFunction)ApplicationPy::sGetLogLevel,
     METH_VARARGS,
     "getLogLevel(tag) -- Get the log level of a string tag"},
    {"checkLinkDepth",
     (PyCFunction)ApplicationPy::sCheckLinkDepth,
     METH_VARARGS,
     "checkLinkDepth(depth) -- check link recursion depth"},
    {"getLinksTo",
     (PyCFunction)ApplicationPy::sGetLinksTo,
     METH_VARARGS,
     "getLinksTo(obj,options=0,maxCount=0) -- return the objects linked to 'obj'\n\n"
     "options: 1: recursive, 2: check link array. Options can combine.\n"
     "maxCount: to limit the number of links returned\n"},
    {"getDependentObjects",
     (PyCFunction)ApplicationPy::sGetDependentObjects,
     METH_VARARGS,
     "getDependentObjects(obj|[obj,...], options=0)\n"
     "Return a list of dependent objects including the given objects.\n\n"
     "options: can have the following bit flags,\n"
     "         1: to sort the list in topological order.\n"
     "         2: to exclude dependency of Link type object."},
    {"setActiveTransaction",
     (PyCFunction)ApplicationPy::sSetActiveTransaction,
     METH_VARARGS,
     "setActiveTransaction(name, persist=False) -- setup active transaction with the given name\n\n"
     "name: the transaction name\n"
     "persist(False): by default, if the calling code is inside any invocation of a command, it\n"
     "                will be auto closed once all commands within the current stack exists. To\n"
     "                disable auto closing, set persist=True\n"
     "Returns the transaction ID for the active transaction. An application-wide\n"
     "active transaction causes any document changes to open a transaction with\n"
     "the given name and ID."},
    {"getActiveTransaction",
     (PyCFunction)ApplicationPy::sGetActiveTransaction,
     METH_VARARGS,
     "getActiveTransaction() -> (name,id)\n\n"
     "return the current active transaction name and ID"},
    {"closeActiveTransaction",
     (PyCFunction)ApplicationPy::sCloseActiveTransaction,
     METH_VARARGS,
     "closeActiveTransaction(abort=False) -- commit or abort current active transaction"},
    {"isRestoring",
     (PyCFunction)ApplicationPy::sIsRestoring,
     METH_VARARGS,
     "isRestoring() -> bool\n\n"
     "Test if the application is opening some document"},
    {"checkAbort",
     (PyCFunction)ApplicationPy::sCheckAbort,
     METH_VARARGS,
     "checkAbort() -- check for user abort in length operation.\n\n"
     "This only works if there is an active sequencer (or ProgressIndicator in Python).\n"
     "There is an active sequencer during document restore and recomputation. User may\n"
     "abort the operation by pressing the ESC key. Once detected, this function will\n"
     "trigger a Base.FreeCADAbort exception."},
    {nullptr, nullptr, 0, nullptr} /* Sentinel */
};


PyObject* ApplicationPy::sLoadFile(PyObject* /*self*/, PyObject* args)
{
    const char* path;
    const char* doc = "";
    const char* mod = "";
    if (!PyArg_ParseTuple(args, "s|ss", &path, &doc, &mod)) {
        return nullptr;
    }
    try {
        Base::FileInfo fi(path);
        if (!fi.isFile() || !fi.exists()) {
            PyErr_Format(PyExc_IOError, "File %s doesn't exist.", path);
            return nullptr;
        }

        std::string module = mod;
        if (module.empty()) {
            std::string ext = fi.extension();
            std::vector<std::string> modules = GetApplication().getImportModules(ext.c_str());
            if (modules.empty()) {
                PyErr_Format(PyExc_IOError, "Filetype %s is not supported.", ext.c_str());
                return nullptr;
            }
            else {
                module = modules.front();
            }
        }

        // path could contain characters that need escaping, such as quote signs
        // therefore use its representation in the Python code string
        PyObject* pathObj = PyUnicode_FromString(path);
        PyObject* pathReprObj = PyObject_Repr(pathObj);
        const char* pathRepr = PyUnicode_AsUTF8(pathReprObj);

        std::stringstream str;
        str << "import " << module << std::endl;
        if (fi.hasExtension("FCStd")) {
            str << module << ".openDocument(" << pathRepr << ")" << std::endl;
        }
        else {
            str << module << ".insert(" << pathRepr << ",'" << doc << "')" << std::endl;
        }

        Py_DECREF(pathObj);
        Py_DECREF(pathReprObj);

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
        return nullptr;
    }
}

PyObject* ApplicationPy::sIsRestoring(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    return Py::new_reference_to(Py::Boolean(GetApplication().isRestoring()));
}

PyObject* ApplicationPy::sOpenDocument(PyObject* /*self*/, PyObject* args, PyObject* kwd)
{
    char* Name;
    PyObject* hidden = Py_False;
    PyObject* temporary = Py_False;
    static const std::array<const char*, 4> kwlist {"name", "hidden", "temporary", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args,
                                             kwd,
                                             "et|O!O!",
                                             kwlist,
                                             "utf-8",
                                             &Name,
                                             &PyBool_Type,
                                             &hidden,
                                             &PyBool_Type,
                                             &temporary)) {
        return nullptr;
    }
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    try {
        DocumentInitFlags initFlags {
            .createView = !Base::asBoolean(hidden),
            .temporary = Base::asBoolean(temporary)
        };

        // return new document
        return (GetApplication()
                    .openDocument(EncodedName.c_str(), initFlags)
                    ->getPyObject());
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_IOError, e.what());
        return nullptr;
    }
    catch (const std::exception& e) {
        // might be subclass from zipios
        PyErr_Format(PyExc_IOError, "Invalid project file %s: %s\n", EncodedName.c_str(), e.what());
        return nullptr;
    }
}

PyObject* ApplicationPy::sNewDocument(PyObject* /*self*/, PyObject* args, PyObject* kwd)
{
    char* docName = nullptr;
    char* usrName = nullptr;
    PyObject* hidden = Py_False;
    PyObject* temp = Py_False;
    static const std::array<const char*, 5> kwlist {"name", "label", "hidden", "temp", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args,
                                             kwd,
                                             "|etetO!O!",
                                             kwlist,
                                             "utf-8",
                                             &docName,
                                             "utf-8",
                                             &usrName,
                                             &PyBool_Type,
                                             &hidden,
                                             &PyBool_Type,
                                             &temp)) {
        return nullptr;
    }

    PY_TRY
    {
        DocumentInitFlags initFlags {
            .createView = !Base::asBoolean(hidden),
            .temporary = Base::asBoolean(temp)
        };
        App::Document* doc = GetApplication().newDocument(docName,
                                                          usrName,
                                                          initFlags);
        PyMem_Free(docName);
        PyMem_Free(usrName);
        return doc->getPyObject();
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sSetActiveDocument(PyObject* /*self*/, PyObject* args)
{
    char* pstr = nullptr;
    if (!PyArg_ParseTuple(args, "s", &pstr)) {
        return nullptr;
    }

    try {
        GetApplication().setActiveDocument(pstr);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }

    Py_Return;
}

PyObject* ApplicationPy::sCloseDocument(PyObject* /*self*/, PyObject* args)
{
    char* pstr = nullptr;
    if (PyArg_ParseTuple(args, "s", &pstr)) {
        Document* doc = GetApplication().getDocument(pstr);
        if (!doc) {
            PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
            return nullptr;
        }
        if (!doc->isClosable()) {
            PyErr_Format(PyExc_RuntimeError, "The document '%s' is not closable for the moment", pstr);
            return nullptr;
        }

        if (!GetApplication().closeDocument(pstr)) {
            PyErr_Format(PyExc_RuntimeError, "Closing the document '%s' failed", pstr);
            return nullptr;
        }

        Py_Return;
    }

    PyErr_Clear();
    PyObject* docpy {};
    if (PyArg_ParseTuple(args, "O!", &App::DocumentPy::Type, &docpy)) {
        Document* doc = static_cast<App::DocumentPy*>(docpy)->getDocumentPtr();
        if (!doc) {
            PyErr_Format(PyExc_RuntimeError, "Invalid document");
            return nullptr;
        }

        if (!doc->isClosable()) {
            PyErr_Format(PyExc_RuntimeError, "The document '%s' is not closable for the moment", doc->getName());
            return nullptr;
        }

        if (!GetApplication().closeDocument(doc)) {
            PyErr_Format(PyExc_RuntimeError, "Closing the document '%s' failed", doc->getName());
            return nullptr;
        }

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Expect str or Document");
    return nullptr;
}

PyObject* ApplicationPy::sSaveDocument(PyObject* /*self*/, PyObject* args)
{
    char* pDoc;
    if (!PyArg_ParseTuple(args, "s", &pDoc)) {
        return nullptr;
    }

    Document* doc = GetApplication().getDocument(pDoc);
    if (doc) {
        if (!doc->save()) {
            PyErr_Format(Base::PyExc_FC_GeneralError, "Cannot save document '%s'", pDoc);
            return nullptr;
        }
    }
    else {
        PyErr_Format(PyExc_NameError, "Unknown document '%s'", pDoc);
        return nullptr;
    }

    Py_Return;
}

PyObject* ApplicationPy::sActiveDocument(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Document* doc = GetApplication().getActiveDocument();
    if (doc) {
        return doc->getPyObject();
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* ApplicationPy::sGetDocument(PyObject* /*self*/, PyObject* args)
{
    char* pstr = nullptr;
    if (!PyArg_ParseTuple(args, "s", &pstr)) {
        return nullptr;
    }

    Document* doc = GetApplication().getDocument(pstr);
    if (!doc) {
        PyErr_Format(PyExc_NameError, "Unknown document '%s'", pstr);
        return nullptr;
    }

    return doc->getPyObject();
}

PyObject* ApplicationPy::sGetParam(PyObject* /*self*/, PyObject* args)
{
    char* pstr = nullptr;
    if (!PyArg_ParseTuple(args, "s", &pstr)) {
        return nullptr;
    }

    PY_TRY
    {
        return GetPyObject(GetApplication().GetParameterGroupByPath(pstr));
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sSaveParameter(PyObject* /*self*/, PyObject* args)
{
    const char* pstr = "User parameter";
    if (!PyArg_ParseTuple(args, "|s", &pstr)) {
        return nullptr;
    }

    PY_TRY
    {
        ParameterManager* param = App::GetApplication().GetParameterSet(pstr);
        if (!param) {
            std::stringstream str;
            str << "No parameter set found with name: " << pstr;
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return nullptr;
        }
        else if (!param->HasSerializer()) {
            std::stringstream str;
            str << "Parameter set cannot be serialized: " << pstr;
            PyErr_SetString(PyExc_RuntimeError, str.str().c_str());
            return nullptr;
        }

        param->SaveDocument();
        Py_INCREF(Py_None);
        return Py_None;
    }
    PY_CATCH;
}


PyObject* ApplicationPy::sGetConfig(PyObject* /*self*/, PyObject* args)
{
    char* pstr;

    if (!PyArg_ParseTuple(args, "s", &pstr)) {
        return nullptr;
    }
    const std::map<std::string, std::string>& Map = GetApplication().Config();

    std::map<std::string, std::string>::const_iterator it = Map.find(pstr);
    if (it != Map.end()) {
        return Py_BuildValue("s", it->second.c_str());
    }
    else {
        // do not set an error because this may break existing python code
        return PyUnicode_FromString("");
    }
}

PyObject* ApplicationPy::sDumpConfig(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PyObject* dict = PyDict_New();
    for (const auto& It : GetApplication().Config()) {
        PyDict_SetItemString(dict, It.first.c_str(), PyUnicode_FromString(It.second.c_str()));
    }
    return dict;
}

PyObject* ApplicationPy::sSetConfig(PyObject* /*self*/, PyObject* args)
{
    char *pstr, *pstr2;

    if (!PyArg_ParseTuple(args, "ss", &pstr, &pstr2)) {
        return nullptr;
    }

    GetApplication().Config()[pstr] = pstr2;

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* ApplicationPy::sGetVersion(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::List list;
    const std::map<std::string, std::string>& cfg = Application::Config();
    std::map<std::string, std::string>::const_iterator it;

    it = cfg.find("BuildVersionMajor");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildVersionMinor");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildVersionPoint");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildRevision");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildRepositoryURL");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildRevisionDate");
    list.append(Py::String(it != cfg.end() ? it->second : ""));

    it = cfg.find("BuildRevisionBranch");
    if (it != cfg.end()) {
        list.append(Py::String(it->second));
    }

    it = cfg.find("BuildRevisionHash");
    if (it != cfg.end()) {
        list.append(Py::String(it->second));
    }

    return Py::new_reference_to(list);
}

PyObject* ApplicationPy::sAddImportType(PyObject* /*self*/, PyObject* args)
{
    char *psKey, *psMod;

    if (!PyArg_ParseTuple(args, "ss", &psKey, &psMod)) {
        return nullptr;
    }

    GetApplication().addImportType(psKey, psMod);

    Py_Return;
}

PyObject* ApplicationPy::sChangeImportModule(PyObject* /*self*/, PyObject* args)
{
    char *key, *oldMod, *newMod;

    if (!PyArg_ParseTuple(args, "sss", &key, &oldMod, &newMod)) {
        return nullptr;
    }

    GetApplication().changeImportModule(key, oldMod, newMod);

    Py_Return;
}

PyObject* ApplicationPy::sGetImportType(PyObject* /*self*/, PyObject* args)
{
    char* psKey = nullptr;

    if (!PyArg_ParseTuple(args, "|s", &psKey)) {
        return nullptr;
    }

    if (psKey) {
        Py::List list;
        std::vector<std::string> modules = GetApplication().getImportModules(psKey);
        for (const auto& it : modules) {
            list.append(Py::String(it));
        }

        return Py::new_reference_to(list);
    }
    else {
        Py::Dict dict;
        std::vector<std::string> types = GetApplication().getImportTypes();
        for (const auto& it : types) {
            std::vector<std::string> modules = GetApplication().getImportModules(it.c_str());
            if (modules.empty()) {
                dict.setItem(it.c_str(), Py::None());
            }
            else if (modules.size() == 1) {
                dict.setItem(it.c_str(), Py::String(modules.front()));
            }
            else {
                Py::List list;
                for (const auto& jt : modules) {
                    list.append(Py::String(jt));
                }
                dict.setItem(it.c_str(), list);
            }
        }

        return Py::new_reference_to(dict);
    }
}

PyObject* ApplicationPy::sAddExportType(PyObject* /*self*/, PyObject* args)
{
    char *psKey, *psMod;

    if (!PyArg_ParseTuple(args, "ss", &psKey, &psMod)) {
        return nullptr;
    }

    GetApplication().addExportType(psKey, psMod);

    Py_Return;
}

PyObject* ApplicationPy::sChangeExportModule(PyObject* /*self*/, PyObject* args)
{
    char *key, *oldMod, *newMod;

    if (!PyArg_ParseTuple(args, "sss", &key, &oldMod, &newMod)) {
        return nullptr;
    }

    GetApplication().changeExportModule(key, oldMod, newMod);

    Py_Return;
}

PyObject* ApplicationPy::sGetExportType(PyObject* /*self*/, PyObject* args)
{
    char* psKey = nullptr;

    if (!PyArg_ParseTuple(args, "|s", &psKey)) {
        return nullptr;
    }

    if (psKey) {
        Py::List list;
        std::vector<std::string> modules = GetApplication().getExportModules(psKey);
        for (const auto& it : modules) {
            list.append(Py::String(it));
        }

        return Py::new_reference_to(list);
    }
    else {
        Py::Dict dict;
        std::vector<std::string> types = GetApplication().getExportTypes();
        for (const auto& it : types) {
            std::vector<std::string> modules = GetApplication().getExportModules(it);
            if (modules.empty()) {
                dict.setItem(it.c_str(), Py::None());
            }
            else if (modules.size() == 1) {
                dict.setItem(it.c_str(), Py::String(modules.front()));
            }
            else {
                Py::List list;
                for (const auto& jt : modules) {
                    list.append(Py::String(jt));
                }
                dict.setItem(it.c_str(), list);
            }
        }

        return Py::new_reference_to(dict);
    }
}

PyObject* ApplicationPy::sGetResourcePath(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String datadir(Application::getResourceDir(), "utf-8");
    return Py::new_reference_to(datadir);
}

PyObject* ApplicationPy::sGetLibraryPath(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String datadir(Application::getLibraryDir(), "utf-8");
    return Py::new_reference_to(datadir);
}

PyObject* ApplicationPy::sGetTempPath(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String datadir(Application::getTempPath(), "utf-8");
    return Py::new_reference_to(datadir);
}

PyObject* ApplicationPy::sGetUserCachePath(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String datadir(Application::getUserCachePath(), "utf-8");
    return Py::new_reference_to(datadir);
}

PyObject* ApplicationPy::sGetUserConfigPath(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String datadir(Application::getUserConfigPath(), "utf-8");
    return Py::new_reference_to(datadir);
}

PyObject* ApplicationPy::sGetUserAppDataPath(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String user_data_dir(Application::getUserAppDataDir(), "utf-8");
    return Py::new_reference_to(user_data_dir);
}

PyObject* ApplicationPy::sGetUserMacroPath(PyObject* /*self*/, PyObject* args)
{
    PyObject* actual = Py_False;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &actual)) {
        return nullptr;
    }

    std::string macroDir = Application::getUserMacroDir();
    if (Base::asBoolean(actual)) {
        macroDir = App::GetApplication()
                       .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
                       ->GetASCII("MacroPath", macroDir.c_str());
        std::replace(macroDir.begin(), macroDir.end(), '/', PATHSEP);
    }

    Py::String user_macro_dir(macroDir, "utf-8");
    return Py::new_reference_to(user_macro_dir);
}

PyObject* ApplicationPy::sGetHelpPath(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String user_macro_dir(Application::getHelpDir(), "utf-8");
    return Py::new_reference_to(user_macro_dir);
}

PyObject* ApplicationPy::sGetHomePath(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String homedir(Application::getHomePath(), "utf-8");
    return Py::new_reference_to(homedir);
}

PyObject* ApplicationPy::sListDocuments(PyObject* /*self*/, PyObject* args)
{
    PyObject* sort = Py_False;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &sort)) {
        return nullptr;
    }
    PY_TRY
    {
        PyObject* pDict = PyDict_New();
        PyObject* pKey;
        Base::PyObjectBase* pValue;

        std::vector<Document*> docs = GetApplication().getDocuments();
        ;
        if (Base::asBoolean(sort)) {
            docs = Document::getDependentDocuments(docs, true);
        }

        for (auto doc : docs) {
            pKey = PyUnicode_FromString(doc->getName());
            // GetPyObject() increments
            pValue = static_cast<Base::PyObjectBase*>(doc->getPyObject());
            PyDict_SetItem(pDict, pKey, pValue);
            // now we can decrement again as PyDict_SetItem also has incremented
            pValue->DecRef();
        }

        return pDict;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sAddDocObserver(PyObject* /*self*/, PyObject* args)
{
    PyObject* o;
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

PyObject* ApplicationPy::sRemoveDocObserver(PyObject* /*self*/, PyObject* args)
{
    PyObject* o;
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

PyObject* ApplicationPy::sSetLogLevel(PyObject* /*self*/, PyObject* args)
{
    char* tag;
    PyObject* pcObj;
    if (!PyArg_ParseTuple(args, "sO", &tag, &pcObj)) {
        return nullptr;
    }
    PY_TRY
    {
        int l;
        if (PyUnicode_Check(pcObj)) {
            const char* pstr = PyUnicode_AsUTF8(pcObj);
            if (strcmp(pstr, "Log") == 0) {
                l = FC_LOGLEVEL_LOG;
            }
            else if (strcmp(pstr, "Warning") == 0) {
                l = FC_LOGLEVEL_WARN;
            }
            else if (strcmp(pstr, "Message") == 0) {
                l = FC_LOGLEVEL_MSG;
            }
            else if (strcmp(pstr, "Error") == 0) {
                l = FC_LOGLEVEL_ERR;
            }
            else if (strcmp(pstr, "Trace") == 0) {
                l = FC_LOGLEVEL_TRACE;
            }
            else if (strcmp(pstr, "Default") == 0) {
                l = FC_LOGLEVEL_DEFAULT;
            }
            else {
                Py_Error(PyExc_ValueError,
                         "Unknown Log Level (use 'Default', 'Error', 'Warning', 'Message', 'Log', "
                         "'Trace' or an integer)");
                return nullptr;
            }
        }
        else {
            l = PyLong_AsLong(pcObj);
        }
        GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/LogLevels")
            ->SetInt(tag, l);
        if (strcmp(tag, "Default") == 0) {
#ifndef FC_DEBUG
            if (l >= 0) {
                Base::Console().setDefaultLogLevel(l);
            }
#endif
        }
        else if (strcmp(tag, "DebugDefault") == 0) {
#ifdef FC_DEBUG
            if (l >= 0) {
                Base::Console().setDefaultLogLevel(l);
            }
#endif
        }
        else {
            *Base::Console().getLogLevel(tag) = l;
        }
        Py_INCREF(Py_None);
        return Py_None;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sGetLogLevel(PyObject* /*self*/, PyObject* args)
{
    char* tag;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    PY_TRY
    {
        int l = -1;
        if (strcmp(tag, "Default") == 0) {
#ifdef FC_DEBUG
            l = GetApplication().GetUserParameter().GetGroup("BaseApp/LogLevels")->GetInt(tag, -1);
#endif
        }
        else if (strcmp(tag, "DebugDefault") == 0) {
#ifndef FC_DEBUG
            l = GetApplication().GetUserParameter().GetGroup("BaseApp/LogLevels")->GetInt(tag, -1);
#endif
        }
        else {
            int* pl = Base::Console().getLogLevel(tag, false);
            l = pl ? *pl : -1;
        }
        // For performance reason, we only output integer value
        return Py_BuildValue("i", Base::Console().logLevel(l));
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sCheckLinkDepth(PyObject* /*self*/, PyObject* args)
{
    short depth = 0;
    if (!PyArg_ParseTuple(args, "h", &depth)) {
        return nullptr;
    }

    PY_TRY
    {
        return Py::new_reference_to(
            Py::Long(GetApplication().checkLinkDepth(depth, MessageOption::Throw)));
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sGetLinksTo(PyObject* /*self*/, PyObject* args)
{
    PyObject* pyobj = Py_None;
    int options = 0;
    short count = 0;
    if (!PyArg_ParseTuple(args, "|Oih", &pyobj, &options, &count)) {
        return nullptr;
    }

    PY_TRY
    {
        Base::PyTypeCheck(&pyobj,
                          &DocumentObjectPy::Type,
                          "Expect the first argument of type App.DocumentObject or None");
        DocumentObject* obj = nullptr;
        if (pyobj) {
            obj = static_cast<DocumentObjectPy*>(pyobj)->getDocumentObjectPtr();
        }

        auto links = GetApplication().getLinksTo(obj, options, count);
        Py::Tuple ret(links.size());
        int i = 0;
        for (auto o : links) {
            ret.setItem(i++, Py::Object(o->getPyObject(), true));
        }

        return Py::new_reference_to(ret);
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sGetDependentObjects(PyObject* /*self*/, PyObject* args)
{
    PyObject* obj;
    int options = 0;
    if (!PyArg_ParseTuple(args, "O|i", &obj, &options)) {
        return nullptr;
    }

    std::vector<App::DocumentObject*> objs;
    if (PySequence_Check(obj)) {
        Py::Sequence seq(obj);
        for (Py_ssize_t i = 0; i < seq.size(); ++i) {
            if (!PyObject_TypeCheck(seq[i].ptr(), &DocumentObjectPy::Type)) {
                PyErr_SetString(PyExc_TypeError,
                                "Expect element in sequence to be of type document object");
                return nullptr;
            }
            objs.push_back(static_cast<DocumentObjectPy*>(seq[i].ptr())->getDocumentObjectPtr());
        }
    }
    else if (!PyObject_TypeCheck(obj, &DocumentObjectPy::Type)) {
        PyErr_SetString(
            PyExc_TypeError,
            "Expect first argument to be either a document object or sequence of document objects");
        return nullptr;
    }
    else {
        objs.push_back(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr());
    }

    PY_TRY
    {
        auto ret = App::Document::getDependencyList(objs, options);

        Py::Tuple tuple(ret.size());
        for (size_t i = 0; i < ret.size(); ++i) {
            tuple.setItem(i, Py::Object(ret[i]->getPyObject(), true));
        }
        return Py::new_reference_to(tuple);
    }
    PY_CATCH;
}


PyObject* ApplicationPy::sSetActiveTransaction(PyObject* /*self*/, PyObject* args)
{
    char* name;
    PyObject* persist = Py_False;
    if (!PyArg_ParseTuple(args, "s|O!", &name, &PyBool_Type, &persist)) {
        return nullptr;
    }

    PY_TRY
    {
        Py::Long ret(GetApplication().setActiveTransaction(name, Base::asBoolean(persist)));
        return Py::new_reference_to(ret);
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sGetActiveTransaction(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        int id = 0;
        const char* name = GetApplication().getActiveTransaction(&id);
        if (!name || id <= 0) {
            Py_Return;
        }
        Py::Tuple ret(2);
        ret.setItem(0, Py::String(name));
        ret.setItem(1, Py::Long(id));
        return Py::new_reference_to(ret);
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sCloseActiveTransaction(PyObject* /*self*/, PyObject* args)
{
    PyObject* abort = Py_False;
    int id = 0;
    if (!PyArg_ParseTuple(args, "|O!i", &PyBool_Type, &abort, &id)) {
        return nullptr;
    }

    PY_TRY
    {
        GetApplication().closeActiveTransaction(Base::asBoolean(abort), id);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* ApplicationPy::sCheckAbort(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        Base::Sequencer().checkAbort();
        Py_Return;
    }
    PY_CATCH
}
