/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *
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
#   include <Python.h>
#   include <sstream>
#   include <boost/regex.hpp>
#endif

#include "Console.h"
#include "Interpreter.h"
#include "FileInfo.h"
#include "Stream.h"
#include "PyTools.h"
#include "Exception.h"
#include "PyObjectBase.h"
#include <CXX/Extensions.hxx>


char format2[1024];  //Warning! Can't go over 512 characters!!!
unsigned int format2_len = 1024;

using namespace Base;

#if PY_VERSION_HEX <= 0x02050000
#error "Use Python2.5.x or higher"
#endif


PyException::PyException(void)
{
    PP_Fetch_Error_Text();    /* fetch (and clear) exception */
    std::string prefix = PP_last_error_type; /* exception name text */
//  prefix += ": ";
    std::string error = PP_last_error_info;            /* exception data text */
#if 0
    // The Python exceptions might be thrown from nested functions, so take
    // into account not to add the same prefix several times
    std::string::size_type pos = error.find(prefix);
    if (pos == std::string::npos)
        _sErrMsg = prefix + error;
    else
        _sErrMsg = error;
#endif
    _sErrMsg = error;
    _errorType = prefix;


    _stackTrace = PP_last_error_trace;     /* exception traceback text */

    // This should be done in the constructor because when doing
    // in the destructor it's not always clear when it is called
    // and thus may clear a Python exception when it should not.
    PyGILStateLocker locker;
    PyErr_Clear(); // must be called to keep Python interpreter in a valid state (Werner)
}

PyException::~PyException() throw()
{
}

void PyException::ReportException (void) const
{
    Base::Console().Error("%s%s: %s\n",
        _stackTrace.c_str(), _errorType.c_str(), what());
}

// ---------------------------------------------------------

SystemExitException::SystemExitException()
{
    _sErrMsg = "System exit";
}

SystemExitException::SystemExitException(const SystemExitException &inst)
        : Exception(inst)
{
}

// ---------------------------------------------------------

// Fixes #0000831: python print causes File descriptor error on windows
class PythonStdOutput : public Py::PythonExtension<PythonStdOutput>
{
public:
    static void init_type(void)
    {
        behaviors().name("PythonStdOutput");
        behaviors().doc("Python standard output");
        add_varargs_method("write",&PythonStdOutput::write,"write()");
        add_varargs_method("flush",&PythonStdOutput::flush,"flush()");
    }

    PythonStdOutput()
    {
    }
    ~PythonStdOutput()
    {
    }

    Py::Object write(const Py::Tuple&)
    {
        return Py::None();
    }
    Py::Object flush(const Py::Tuple&)
    {
        return Py::None();
    }
};

// ---------------------------------------------------------

InterpreterSingleton::InterpreterSingleton()
{
    //Py_Initialize();
}

InterpreterSingleton::~InterpreterSingleton()
{

}


std::string InterpreterSingleton::runString(const char *sCmd)
{
    PyObject *module, *dict, *presult;          /* "exec code in d, d" */

    PyGILStateLocker locker;
    module = PP_Load_Module("__main__");         /* get module, init python */
    if (module == NULL)
        throw PyException();                         /* not incref'd */
    dict = PyModule_GetDict(module);            /* get dict namespace */
    if (dict == NULL)
        throw PyException();                           /* not incref'd */


    presult = PyRun_String(sCmd, Py_file_input, dict, dict); /* eval direct */
    if (!presult) {
        throw PyException();
    }

    PyObject* repr = PyObject_Repr(presult);
    Py_DECREF(presult);
    if (repr) {
        std::string ret(PyString_AsString(repr));
        Py_DECREF(repr);
        return ret;
    }
    else {
        PyErr_Clear();
        return std::string();
    }
}

void InterpreterSingleton::systemExit(void)
{
    /* This code is taken from the original Python code */
    PyObject *exception, *value, *tb;
    int exitcode = 0;

    PyErr_Fetch(&exception, &value, &tb);
    if (Py_FlushLine())
        PyErr_Clear();
    fflush(stdout);
    if (value == NULL || value == Py_None)
        goto done;
    if (PyInstance_Check(value)) {
        /* The error code should be in the `code' attribute. */
        PyObject *code = PyObject_GetAttrString(value, "code");
        if (code) {
            Py_DECREF(value);
            value = code;
            if (value == Py_None)
                goto done;
        }
        /* If we failed to dig out the 'code' attribute,
           just let the else clause below print the error. */
    }
    if (PyInt_Check(value))
        exitcode = (int)PyInt_AsLong(value);
    else {
        PyObject_Print(value, stderr, Py_PRINT_RAW);
        PySys_WriteStderr("\n");
        exitcode = 1;
    }
done:
    /* Restore and clear the exception info, in order to properly decref
     * the exception, value, and traceback.  If we just exit instead,
     * these leak, which confuses PYTHONDUMPREFS output, and may prevent
     * some finalizers from running.
     */
    PyErr_Restore(exception, value, tb);
    PyErr_Clear();
    Py_Exit(exitcode);
    /* NOTREACHED */
}

void InterpreterSingleton::runInteractiveString(const char *sCmd)
{
    PyObject *module, *dict, *presult;          /* "exec code in d, d" */

    PyGILStateLocker locker;
    module = PP_Load_Module("__main__");         /* get module, init python */
    if (module == NULL)
        throw PyException();                         /* not incref'd */
    dict = PyModule_GetDict(module);            /* get dict namespace */
    if (dict == NULL)
        throw PyException();                           /* not incref'd */

    presult = PyRun_String(sCmd, Py_single_input, dict, dict); /* eval direct */
    if (!presult) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }
        /* get latest python exception information */
        /* and print the error to the error output */
        PyObject *errobj, *errdata, *errtraceback;
        PyErr_Fetch(&errobj, &errdata, &errtraceback);

        Exception exc; // do not use PyException since this clears the error indicator
        if (PyString_Check(errdata))
            exc.setMessage(PyString_AsString(errdata));
        PyErr_Restore(errobj, errdata, errtraceback);
        if (PyErr_Occurred())
            PyErr_Print();
        throw exc;
    }
    else
        Py_DECREF(presult);
}

void InterpreterSingleton::runFile(const char*pxFileName, bool local)
{
#ifdef FC_OS_WIN32
    FileInfo fi(pxFileName);
    FILE *fp = _wfopen(fi.toStdWString().c_str(),L"r");
#else
    FILE *fp = fopen(pxFileName,"r");
#endif
    if (fp) {
        PyGILStateLocker locker;
        //std::string encoding = PyUnicode_GetDefaultEncoding();
        //PyUnicode_SetDefaultEncoding("utf-8");
        //PyUnicode_SetDefaultEncoding(encoding.c_str());
        if (local) {
            PyObject *module, *dict;
            module = PyImport_AddModule("__main__");
            dict = PyModule_GetDict(module);
            dict = PyDict_Copy(dict);
            if (PyDict_GetItemString(dict, "__file__") == NULL) {
                PyObject *f = PyString_FromString(pxFileName);
                if (f == NULL) {
                    fclose(fp);
                    return;
                }
                if (PyDict_SetItemString(dict, "__file__", f) < 0) {
                    Py_DECREF(f);
                    fclose(fp);
                    return;
                }
                Py_DECREF(f);
            }

            PyObject *result = PyRun_File(fp, pxFileName, Py_file_input, dict, dict);
            fclose(fp);
            Py_DECREF(dict);
            if (!result) {
                if (PyErr_ExceptionMatches(PyExc_SystemExit))
                    throw SystemExitException();
                else
                    throw PyException();
            }
            Py_DECREF(result);
        }
        else {
            int ret = PyRun_SimpleFile(fp, pxFileName);
            fclose(fp);
            if (ret != 0)
                throw PyException();
        }

    }
    else {
        std::string err = "Unknown file: ";
        err += pxFileName;
        err += "\n";
        throw Exception(err);
    }
}

bool InterpreterSingleton::loadModule(const char* psModName)
{
    // buffer acrobatics
    //PyBuf ModName(psModName);
    PyObject *module;

    PyGILStateLocker locker;
    module = PP_Load_Module(psModName);

    if (!module) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit))
            throw SystemExitException();
        else
            throw PyException();
    }

    return true;
}

void InterpreterSingleton::addType(PyTypeObject* Type,PyObject* Module, const char * Name)
{
    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
    if (PyType_Ready(Type) < 0) return;
    union PyType_Object pyType = {Type};
    PyModule_AddObject(Module, Name, pyType.o);
}

void InterpreterSingleton::addPythonPath(const char* Path)
{
    PyGILStateLocker locker;
    PyObject *list = PySys_GetObject("path");
    PyObject *path = PyString_FromString(Path);
    PyList_Append(list, path);
    Py_DECREF(path);
    PySys_SetObject("path", list);
}

const char* InterpreterSingleton::init(int argc,char *argv[])
{
    if (!Py_IsInitialized()) {
        Py_SetProgramName(argv[0]);
        PyEval_InitThreads();
        Py_Initialize();
        PySys_SetArgv(argc, argv);
        PythonStdOutput::init_type();
        this->_global = PyEval_SaveThread();
    }

    return Py_GetPath();
}

void InterpreterSingleton::replaceStdOutput()
{
    PyGILStateLocker locker;
    PythonStdOutput* out = new PythonStdOutput();
    PySys_SetObject("stdout", out);
    PySys_SetObject("stderr", out);
}

int InterpreterSingleton::cleanup(void (*func)(void))
{
    return Py_AtExit( func );
}

void InterpreterSingleton::finalize()
{
    try {
        PyEval_RestoreThread(this->_global);
        Py_Finalize();
    }
    catch (...) {
    }
}

void InterpreterSingleton::runStringArg(const char * psCom,...)
{
    // va stuff
    va_list namelessVars;
    va_start(namelessVars, psCom);  // Get the "..." vars
    int len = vsnprintf(format2, format2_len, psCom, namelessVars);
    va_end(namelessVars);
    if ( len == -1 ) {
        // argument too long
        assert(false);
    }

    runString(format2);
}


// Singelton:

InterpreterSingleton * InterpreterSingleton::_pcSingelton = 0;

InterpreterSingleton & InterpreterSingleton::Instance(void)
{
    // not initialized!
    if (!_pcSingelton)
        _pcSingelton = new InterpreterSingleton();
    return *_pcSingelton;
}

void InterpreterSingleton::Destruct(void)
{
    // not initialized or double destruct!
    assert(_pcSingelton);
    delete _pcSingelton;
    _pcSingelton = 0;
}

int InterpreterSingleton::runCommandLine(const char *prompt)
{
    PyGILStateLocker locker;
    return PP_Run_Command_Line(prompt);
}

/**
 *  Runs a member method of an object with no parameter and no return value
 *  void (void). There are other methods to run with returns
 */
void InterpreterSingleton::runMethodVoid(PyObject *pobject, const char *method)
{
    PyGILStateLocker locker;
    if (PP_Run_Method(pobject ,    // object
                      method,  // run method
                      0,			   // no return type
                      0,		       // so no return object
                      "()")		   // no arguments
            != 0)
        throw PyException(/*"Error running InterpreterSingleton::RunMethodVoid()"*/);

}

PyObject* InterpreterSingleton::runMethodObject(PyObject *pobject, const char *method)
{
    PyObject *pcO;

    PyGILStateLocker locker;
    if (PP_Run_Method(pobject ,    // object
                      method,  // run method
                      "O",		   // return type
                      &pcO,		   // return object
                      "()")		   // no arguments
            != 0)
        throw PyException();

    return pcO;
}

void InterpreterSingleton::runMethod(PyObject *pobject, const char *method,
                                     const char *resfmt,   void *cresult,        /* convert to c/c++ */
                                     const char *argfmt,   ...  )                /* convert to python */
{
    PyObject *pmeth, *pargs, *presult;
    va_list argslist;                              /* "pobject.method(args)" */
    va_start(argslist, argfmt);

    PyGILStateLocker locker;
    pmeth = PyObject_GetAttrString(pobject, method);
    if (pmeth == NULL)                             /* get callable object */
        throw Exception("Error running InterpreterSingleton::RunMethod() method not defined");                                 /* bound method? has self */

    pargs = Py_VaBuildValue(argfmt, argslist);     /* args: c->python */

    if (pargs == NULL) {
        Py_DECREF(pmeth);
        throw Exception("InterpreterSingleton::RunMethod() wrong arguments");
    }

    presult = PyEval_CallObject(pmeth, pargs);   /* run interpreter */

    Py_DECREF(pmeth);
    Py_DECREF(pargs);
    if (PP_Convert_Result(presult, resfmt, cresult)!= 0) {
        if ( PyErr_Occurred() )
            PyErr_Print();
        throw Exception("Error running InterpreterSingleton::RunMethod() exception in called method");
    }
}

void InterpreterSingleton::dbgObserveFile(const char* sFileName)
{
    if (sFileName)
        _cDebugFileName = sFileName;
    else
        _cDebugFileName = "";
}

void InterpreterSingleton::dbgSetBreakPoint(unsigned int /*uiLineNumber*/)
{

}

void InterpreterSingleton::dbgUnsetBreakPoint(unsigned int /*uiLineNumber*/)
{

}

void InterpreterSingleton::dbgStep(void)
{

}

const std::string InterpreterSingleton::strToPython(const char* Str)
{
    std::string result;
    const char *It=Str;

    while (*It != '\0') {
        switch (*It) {
        case '\\':
            result += "\\\\";
            break;
        case '\"':
            result += "\\\"";
            break;
        case '\'':
            result += "\\\'";
            break;
        default:
            result += *It;
        }
        It++;
    }

    return result;
}

// --------------------------------------------------------------------

int getSWIGVersionFromModule(const std::string& module)
{
    static std::map<std::string, int> moduleMap;
    std::map<std::string, int>::iterator it = moduleMap.find(module);
    if (it != moduleMap.end()) {
        return it->second;
    }
    else {
        try {
            // Get the module and check its __file__ attribute
            Py::Dict dict(PyImport_GetModuleDict());
            if (!dict.hasKey(module))
                return 0;
            Py::Module mod(module);
            Py::String file(mod.getAttr("__file__"));
            std::string filename = (std::string)file;
            // file can have the extension .py or .pyc
            filename = filename.substr(0, filename.rfind("."));
            filename += ".py";
            boost::regex rx("^# Version ([1-9])\\.([0-9])\\.([0-9]+)");
            boost::cmatch what;

            std::string line;
            Base::FileInfo fi(filename);

            Base::ifstream str(fi, std::ios::in);
            while (str && std::getline(str, line)) {
                if (boost::regex_match(line.c_str(), what, rx)) {
                    int major = std::atoi(what[1].first);
                    int minor = std::atoi(what[2].first);
                    int micro = std::atoi(what[3].first);
                    int version = (major<<16)+(minor<<8)+micro;
                    moduleMap[module] = version;
                    return version;
                }
            }
        }
        catch (Py::Exception& e) {
            e.clear();
        }
    }

#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    moduleMap[module] = 0;
#endif
    return 0;
}

#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
namespace Swig_python { extern int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own); }
#endif
namespace Swig_1_3_25 { extern int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own); }
namespace Swig_1_3_33 { extern int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own); }
namespace Swig_1_3_36 { extern int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own); }
namespace Swig_1_3_38 { extern int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own); }
namespace Swig_1_3_40 { extern int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own); }

PyObject* InterpreterSingleton::createSWIGPointerObj(const char* Module, const char* TypeName, void* Pointer, int own)
{
    int result = 0;
    PyObject* proxy=0;
    PyGILStateLocker locker;
    int version = getSWIGVersionFromModule(Module);
    switch (version)
    {
    case 66329:
        result = Swig_1_3_25::createSWIGPointerObj_T(TypeName, Pointer, &proxy, own);
        break;
    case 66337:
        result = Swig_1_3_33::createSWIGPointerObj_T(TypeName, Pointer, &proxy, own);
        break;
    case 66340:
        result = Swig_1_3_36::createSWIGPointerObj_T(TypeName, Pointer, &proxy, own);
        break;
    case 66342:
        result = Swig_1_3_38::createSWIGPointerObj_T(TypeName, Pointer, &proxy, own);
        break;
    case 66344:
        result = Swig_1_3_40::createSWIGPointerObj_T(TypeName, Pointer, &proxy, own);
        break;
    default:
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    result = Swig_python::createSWIGPointerObj_T(TypeName, Pointer, &proxy, own);
#else
    result = -1; // indicates error
#endif
    }

    if (result == 0)
        return proxy;

    // none of the SWIG's succeeded
    throw Base::Exception("No SWIG wrapped library loaded");
}

#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
namespace Swig_python { extern int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags); }
#endif
namespace Swig_1_3_25 { extern int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags); }
namespace Swig_1_3_33 { extern int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags); }
namespace Swig_1_3_36 { extern int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags); }
namespace Swig_1_3_38 { extern int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags); }
namespace Swig_1_3_40 { extern int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags); }

bool InterpreterSingleton::convertSWIGPointerObj(const char* Module, const char* TypeName, PyObject* obj, void** ptr, int flags)
{
    int result = 0;
    PyGILStateLocker locker;
    int version = getSWIGVersionFromModule(Module);
    switch (version)
    {
    case 66329:
        result = Swig_1_3_25::convertSWIGPointerObj_T(TypeName, obj, ptr, flags);
        break;
    case 66337:
        result = Swig_1_3_33::convertSWIGPointerObj_T(TypeName, obj, ptr, flags);
        break;
    case 66340:
        result = Swig_1_3_36::convertSWIGPointerObj_T(TypeName, obj, ptr, flags);
        break;
    case 66342:
        result = Swig_1_3_38::convertSWIGPointerObj_T(TypeName, obj, ptr, flags);
        break;
    case 66344:
        result = Swig_1_3_40::convertSWIGPointerObj_T(TypeName, obj, ptr, flags);
        break;
    default:
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
        result = Swig_python::convertSWIGPointerObj_T(TypeName, obj, ptr, flags);
#else
        result = -1; // indicates error
#endif
    }

    if (result == 0)
        return true;

    // none of the SWIG's succeeded
    throw Base::Exception("No SWIG wrapped library loaded");
}

#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
namespace Swig_python { extern void cleanupSWIG_T(const char* TypeName); }
#endif
namespace Swig_1_3_25 { extern void cleanupSWIG_T(const char* TypeName); }
namespace Swig_1_3_33 { extern void cleanupSWIG_T(const char* TypeName); }
namespace Swig_1_3_36 { extern void cleanupSWIG_T(const char* TypeName); }
namespace Swig_1_3_38 { extern void cleanupSWIG_T(const char* TypeName); }
namespace Swig_1_3_40 { extern void cleanupSWIG_T(const char* TypeName); }

void InterpreterSingleton::cleanupSWIG(const char* TypeName)
{
    PyGILStateLocker locker;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    Swig_python::cleanupSWIG_T(TypeName);
#endif
    Swig_1_3_25::cleanupSWIG_T(TypeName);
    Swig_1_3_33::cleanupSWIG_T(TypeName);
    Swig_1_3_36::cleanupSWIG_T(TypeName);
    Swig_1_3_38::cleanupSWIG_T(TypeName);
    Swig_1_3_40::cleanupSWIG_T(TypeName);
}
