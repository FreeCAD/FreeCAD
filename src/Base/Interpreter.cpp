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
#   include <sstream>
#   include <boost/regex.hpp>
#endif

#include "Interpreter.h"
#include "Console.h"
#include "ExceptionFactory.h"
#include "FileInfo.h"
#include "PyObjectBase.h"
#include "PyTools.h"
#include "Stream.h"


char format2[1024];  //Warning! Can't go over 512 characters!!!
unsigned int format2_len = 1024;

using namespace Base;

PyException::PyException(const Py::Object &obj) {
    _sErrMsg = obj.as_string();
    // WARNING: we are assuming that python type object will never be
    // destroyed, so we don't keep reference here to save book-keeping in
    // our copy constructor and destructor
    // NOLINTBEGIN
    _exceptionType = reinterpret_cast<PyObject*>(obj.ptr()->ob_type);
    _errorType = obj.ptr()->ob_type->tp_name;
    // NOLINTEND
}

PyException::PyException()
{
    PP_Fetch_Error_Text();    /* fetch (and clear) exception */

    setPyObject(PP_PyDict_Object);

    std::string prefix = PP_last_error_type; /* exception name text */
    std::string error = PP_last_error_info;            /* exception data text */

    _sErrMsg = error;
    _errorType = prefix;

    // NOLINTNEXTLINE
    _exceptionType = PP_last_exception_type;

    if (PP_last_exception_type) {
        // WARNING: we are assuming that python type object will never be
        // destroyed, so we don't keep reference here to save book-keeping in
        // our copy constructor and destructor
        Py_DECREF(PP_last_exception_type);
        PP_last_exception_type = nullptr;

    }

    _stackTrace = PP_last_error_trace;     /* exception traceback text */

    // This should be done in the constructor because when doing
    // in the destructor it's not always clear when it is called
    // and thus may clear a Python exception when it should not.
    PyGILStateLocker locker;
    PyErr_Clear(); // must be called to keep Python interpreter in a valid state (Werner)
}

PyException::~PyException() noexcept = default;

void PyException::ThrowException()
{
    PyException myexcp;
    myexcp.ReportException();
    myexcp.raiseException();
}

void PyException::raiseException() {
    PyGILStateLocker locker;
    if (PP_PyDict_Object) {
        // delete the Python dict upon destruction of edict
        Py::Dict edict(PP_PyDict_Object, true);
        PP_PyDict_Object = nullptr;

        std::string exceptionname;
        if (_exceptionType == Base::PyExc_FC_FreeCADAbort)
            edict.setItem("sclassname",
                    Py::String(typeid(Base::AbortException).name()));
        if (_isReported)
            edict.setItem("breported", Py::True());
        Base::ExceptionFactory::Instance().raiseException(edict.ptr());
    }

    if (_exceptionType == Base::PyExc_FC_FreeCADAbort) {
        Base::AbortException e(_sErrMsg.c_str());
        e.setReported(_isReported);
        throw e;
    }

    throw *this;
}

void PyException::ReportException () const
{
    if (!_isReported) {
        _isReported = true;
        Base::Console().DeveloperError("pyException","%s%s: %s\n",
            _stackTrace.c_str(), _errorType.c_str(), what());
    }
}

void PyException::setPyException() const
{
    std::stringstream str;
    str << getStackTrace()
        << getErrorType()
        << ": " << what();
    PyErr_SetString(getPyExceptionType(), str.str().c_str());
}

// ---------------------------------------------------------

SystemExitException::SystemExitException()
{
    // Set exception message and code based upon the python sys.exit() code and/or message
    // based upon the following sys.exit() call semantics.
    //
    // Invocation       |  _exitCode  |  _sErrMsg
    // ---------------- +  ---------  +  --------
    // sys.exit(int#)   |   int#      |   "System Exit"
    // sys.exit(string) |   1         |   string
    // sys.exit()       |   1         |   "System Exit"

    long int errCode = 1;
    std::string errMsg  = "System exit";
    PyObject  *type{}, *value{}, *traceback{}, *code{};

    PyGILStateLocker locker;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);

    if (value) {
        code = PyObject_GetAttrString(value, "code");
        if (code && value != Py_None) {
           Py_DECREF(value);
           value = code;
        }

        if (PyLong_Check(value)) {
            errCode = PyLong_AsLong(value);
        }
        else {
            const char *str = PyUnicode_AsUTF8(value);
            if (str)
                errMsg = errMsg + ": " + str;
        }
    }

    _sErrMsg  = errMsg;
    _exitCode = errCode;
}

// ---------------------------------------------------------

// Fixes #0000831: python print causes File descriptor error on windows
// NOLINTNEXTLINE
class PythonStdOutput : public Py::PythonExtension<PythonStdOutput>
{
public:
    static void init_type()
    {
        behaviors().name("PythonStdOutput");
        behaviors().doc("Python standard output");
        add_varargs_method("write",&PythonStdOutput::write,"write()");
        add_varargs_method("flush",&PythonStdOutput::flush,"flush()");
    }

    PythonStdOutput() = default;
    ~PythonStdOutput() override = default;

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
    this->_global = nullptr;
}

InterpreterSingleton::~InterpreterSingleton() = default;


std::string InterpreterSingleton::runString(const char *sCmd)
{
    PyObject *module{}, *dict{}, *presult{};          /* "exec code in d, d" */

    PyGILStateLocker locker;
    module = PP_Load_Module("__main__");         /* get module, init python */
    if (!module)
        throw PyException();                         /* not incref'd */
    dict = PyModule_GetDict(module);            /* get dict namespace */
    if (!dict)
        throw PyException();                           /* not incref'd */


    presult = PyRun_String(sCmd, Py_file_input, dict, dict); /* eval direct */
    if (!presult) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit))
            throw SystemExitException();
        else {
            PyException::ThrowException();
            return {}; // just to quieten code analyzers
        }
    }

    PyObject* repr = PyObject_Repr(presult);
    Py_DECREF(presult);
    if (repr) {
        std::string ret(PyUnicode_AsUTF8(repr));
        Py_DECREF(repr);
        return ret;
    }
    else {
        PyErr_Clear();
        return {};
    }
}

/** runStringWithKey(psCmd, key, key_initial_value)
 * psCmd is python script to run
 * key is the name of a python string variable the script will have read/write
 * access to during script execution.  It will be our return value.
 * key_initial_value is the initial value c++ will set before calling the script
 * If the script runs successfully it will be able to change the value of key as
 * the return value, but if there is a runtime error key will not be changed even
 * if the error occurs after changing it inside the script.
 */

std::string InterpreterSingleton::runStringWithKey(const char *psCmd, const char *key, const char *key_initial_value)
{
    PyGILStateLocker locker;
    Py::Module module("__main__");
    Py::Dict globalDictionary = module.getDict();
    Py::Dict localDictionary;
    Py::String initial_value(key_initial_value);
    localDictionary.setItem(key, initial_value);

    PyObject* presult = PyRun_String(psCmd, Py_file_input, globalDictionary.ptr(), localDictionary.ptr());
    if (!presult) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }
        else {
            PyException::ThrowException();
            return {}; // just to quieten code analyzers
        }
    }
    Py_DECREF(presult);

    Py::Object key_return_value = localDictionary.getItem(key);
    if (!key_return_value.isString())
        key_return_value = key_return_value.str();

    Py::Bytes str = Py::String(key_return_value).encode("utf-8", "~E~");
    std::string result = static_cast<std::string>(str);
    return result;
}

Py::Object InterpreterSingleton::runStringObject(const char *sCmd)
{
    PyObject *module{}, *dict{}, *presult{};          /* "exec code in d, d" */

    PyGILStateLocker locker;
    module = PP_Load_Module("__main__");         /* get module, init python */
    if (!module)
        throw PyException();                         /* not incref'd */
    dict = PyModule_GetDict(module);            /* get dict namespace */
    if (!dict)
        throw PyException();                           /* not incref'd */


    presult = PyRun_String(sCmd, Py_eval_input, dict, dict); /* eval direct */
    if (!presult) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit))
            throw SystemExitException();
        else
            throw PyException();
    }

    return Py::asObject(presult);
}

void InterpreterSingleton::systemExit()
{
    /* This code is taken from the original Python code */
    PyObject *exception{}, *value{}, *tb{};
    int exitcode = 0;

    PyErr_Fetch(&exception, &value, &tb);
    fflush(stdout);
    if (!value || value == Py_None)
        goto done; // NOLINT
    if (PyExceptionInstance_Check(value)) {
        /* The error code should be in the `code' attribute. */
        PyObject *code = PyObject_GetAttrString(value, "code");
        if (code) {
            Py_DECREF(value);
            value = code;
            if (value == Py_None)
                goto done; // NOLINT
        }
        /* If we failed to dig out the 'code' attribute,
           just let the else clause below print the error. */
    }
    if (PyLong_Check(value)) {
        exitcode = (int)PyLong_AsLong(value);
    }
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
    PyObject *module{}, *dict{}, *presult{};          /* "exec code in d, d" */

    PyGILStateLocker locker;
    module = PP_Load_Module("__main__");         /* get module, init python */
    if (!module)
        throw PyException();                         /* not incref'd */
    dict = PyModule_GetDict(module);            /* get dict namespace */
    if (!dict)
        throw PyException();                           /* not incref'd */

    presult = PyRun_String(sCmd, Py_single_input, dict, dict); /* eval direct */
    if (!presult) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            throw SystemExitException();
        }
        /* get latest python exception information */
        /* and print the error to the error output */
        PyObject *errobj{}, *errdata{}, *errtraceback{};
        PyErr_Fetch(&errobj, &errdata, &errtraceback);

        RuntimeError exc(""); // do not use PyException since this clears the error indicator
        if (errdata) {
            if (PyUnicode_Check(errdata))
                exc.setMessage(PyUnicode_AsUTF8(errdata));
        }
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
        PyObject *module{}, *dict{};
        module = PyImport_AddModule("__main__");
        dict = PyModule_GetDict(module);
        if (local) {
            dict = PyDict_Copy(dict);
        }
        else {
            Py_INCREF(dict); // avoid to further distinguish between local and global dict
        }

        if (!PyDict_GetItemString(dict, "__file__")) {
            PyObject *pyObj = PyUnicode_FromString(pxFileName);
            if (!pyObj) {
                fclose(fp);
                Py_DECREF(dict);
                return;
            }
            if (PyDict_SetItemString(dict, "__file__", pyObj) < 0) {
                Py_DECREF(pyObj);
                fclose(fp);
                Py_DECREF(dict);
                return;
            }
            Py_DECREF(pyObj);
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
        throw FileException("Unknown file", pxFileName);
    }
}

bool InterpreterSingleton::loadModule(const char* psModName)
{
    // buffer acrobatics
    //PyBuf ModName(psModName);
    PyObject *module{};

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

PyObject* InterpreterSingleton::addModule(Py::ExtensionModuleBase* mod)
{
    _modules.push_back(mod);
    return mod->module().ptr();
}

void InterpreterSingleton::cleanupModules()
{
    // This is only needed to make the address sanitizer happy
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
    for (auto it : _modules) {
        delete it;
    }
    _modules.clear();
#  endif
#endif
}

void InterpreterSingleton::addType(PyTypeObject* Type,PyObject* Module, const char * Name)
{
    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
    if (PyType_Ready(Type) < 0)
        return;
    PyModule_AddObject(Module, Name, Base::getTypeAsObject(Type));
}

void InterpreterSingleton::addPythonPath(const char* Path)
{
    PyGILStateLocker locker;
    Py::List list(PySys_GetObject("path"));
    list.append(Py::String(Path));
}

#if PY_VERSION_HEX < 0x030b0000
const char* InterpreterSingleton::init(int argc,char *argv[])
{
    if (!Py_IsInitialized()) {
        Py_SetProgramName(Py_DecodeLocale(argv[0],nullptr));
        // There is a serious bug in VS from 2010 until 2013 where the file descriptor for stdin, stdout or stderr
        // returns a valid value for GUI applications (i.e. subsystem = Windows) where it shouldn't.
        // This causes Python to fail during initialization.
        // A workaround is to use freopen on stdin, stdout and stderr. See the class Redirection inside main()
        // https://bugs.python.org/issue17797#msg197474
        //
        Py_Initialize();
        const char* virtualenv = getenv("VIRTUAL_ENV");
        if (virtualenv) {
            PyRun_SimpleString(
                "# Check for virtualenv, and activate if present.\n"
                "# See https://virtualenv.pypa.io/en/latest/userguide/#using-virtualenv-without-bin-python\n"
                "import os\n"
                "import sys\n"
                "base_path = os.getenv(\"VIRTUAL_ENV\")\n"
                "if not base_path is None:\n"
                "    activate_this = os.path.join(base_path, \"bin\", \"activate_this.py\")\n"
                "    exec(open(activate_this).read(), {'__file__':activate_this})\n"
            );
        }

#if PY_VERSION_HEX < 0x03090000
        PyEval_InitThreads();
#endif

        size_t size = argc;
        static std::vector<wchar_t *> _argv(size);
        for (int i = 0; i < argc; i++) {
            _argv[i] = Py_DecodeLocale(argv[i],nullptr);
        }
        PySys_SetArgv(argc, _argv.data());
        PythonStdOutput::init_type();
        this->_global = PyEval_SaveThread();
    }

    PyGILStateLocker lock;
    return Py_EncodeLocale(Py_GetPath(),nullptr);
}
#else
namespace {
void initInterpreter(int argc,char *argv[])
{
    PyStatus status;
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);

    status = PyConfig_SetBytesArgv(&config, argc, argv);
    if (PyStatus_Exception(status)) {
        throw Base::RuntimeError("Failed to set config");
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        throw Base::RuntimeError("Failed to init from config");
    }

    PyConfig_Clear(&config);

    Py_Initialize();
    const char* virtualenv = getenv("VIRTUAL_ENV");
    if (virtualenv) {
        PyRun_SimpleString(
            "# Check for virtualenv, and activate if present.\n"
            "# See https://virtualenv.pypa.io/en/latest/userguide/#using-virtualenv-without-bin-python\n"
            "import os\n"
            "import sys\n"
            "base_path = os.getenv(\"VIRTUAL_ENV\")\n"
            "if not base_path is None:\n"
            "    activate_this = os.path.join(base_path, \"bin\", \"activate_this.py\")\n"
            "    exec(open(activate_this).read(), {'__file__':activate_this})\n"
        );
    }
}
}
const char* InterpreterSingleton::init(int argc,char *argv[])
{
    try {
        if (!Py_IsInitialized()) {
            initInterpreter(argc, argv);

            PythonStdOutput::init_type();
            this->_global = PyEval_SaveThread();
        }

        PyGILStateLocker lock;
        return Py_EncodeLocale(Py_GetPath(),nullptr);
    }
    catch (const Base::Exception& e) {
        e.ReportException();
        throw;
    }
}
#endif

void InterpreterSingleton::replaceStdOutput()
{
    PyGILStateLocker locker;
    PythonStdOutput* out = new PythonStdOutput();
    PySys_SetObject("stdout", out);
    PySys_SetObject("stderr", out);
}

int InterpreterSingleton::cleanup(void (*func)())
{
    return Py_AtExit( func );
}

void InterpreterSingleton::finalize()
{
    try {
        PyEval_RestoreThread(this->_global);
        cleanupModules();
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


// Singleton:

InterpreterSingleton * InterpreterSingleton::_pcSingleton = nullptr;

InterpreterSingleton & InterpreterSingleton::Instance()
{
    // not initialized!
    if (!_pcSingleton)
        _pcSingleton = new InterpreterSingleton();
    return *_pcSingleton;
}

void InterpreterSingleton::Destruct()
{
    // not initialized or double destruct!
    assert(_pcSingleton);
    delete _pcSingleton;
    _pcSingleton = nullptr;
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
    if (PP_Run_Method(pobject ,     // object
                      method,       // run method
                      nullptr,      // no return type
                      nullptr,      // so no return object
                      "()")         // no arguments
            != 0)
        throw PyException(/*"Error running InterpreterSingleton::RunMethodVoid()"*/);

}

PyObject* InterpreterSingleton::runMethodObject(PyObject *pobject, const char *method)
{
    PyObject *pcO{};

    PyGILStateLocker locker;
    if (PP_Run_Method(pobject ,     // object
                      method,       // run method
                      "O",          // return type
                      &pcO,         // return object
                      "()")         // no arguments
            != 0)
        throw PyException();

    return pcO;
}

void InterpreterSingleton::runMethod(PyObject *pobject, const char *method,
                                     const char *resfmt,   void *cresult,        /* convert to c/c++ */
                                     const char *argfmt,   ...  )                /* convert to python */
{
    PyObject *pmeth{}, *pargs{}, *presult{};
    va_list argslist;                              /* "pobject.method(args)" */
    va_start(argslist, argfmt);

    PyGILStateLocker locker;
    pmeth = PyObject_GetAttrString(pobject, method);
    if (!pmeth) {                            /* get callable object */
        va_end(argslist);
        throw AttributeError("Error running InterpreterSingleton::RunMethod() method not defined");                                 /* bound method? has self */
    }

    pargs = Py_VaBuildValue(argfmt, argslist);     /* args: c->python */
    va_end(argslist);

    if (!pargs) {
        Py_DECREF(pmeth);
        throw TypeError("InterpreterSingleton::RunMethod() wrong arguments");
    }

#if PY_VERSION_HEX < 0x03090000
    presult = PyEval_CallObject(pmeth, pargs);   /* run interpreter */
#else
    presult = PyObject_CallObject(pmeth, pargs);   /* run interpreter */
#endif

    Py_DECREF(pmeth);
    Py_DECREF(pargs);
    if (PP_Convert_Result(presult, resfmt, cresult)!= 0) {
        if ( PyErr_Occurred() )
            PyErr_Print();
        throw RuntimeError("Error running InterpreterSingleton::RunMethod() exception in called method");
    }
}

PyObject * InterpreterSingleton::getValue(const char * key, const char * result_var)
{
    PyObject *module{}, *dict{}, *presult{};          /* "exec code in d, d" */

    PyGILStateLocker locker;
    module = PP_Load_Module("__main__");         /* get module, init python */
    if (!module)
        throw PyException();                         /* not incref'd */
    dict = PyModule_GetDict(module);            /* get dict namespace */
    if (!dict)
        throw PyException();                           /* not incref'd */


    presult = PyRun_String(key, Py_file_input, dict, dict); /* eval direct */
    if (!presult) {
        throw PyException();
    }
    Py_DECREF(presult);

    return PyObject_GetAttrString(module, result_var);
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

void InterpreterSingleton::dbgStep()
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
            filename = filename.substr(0, filename.rfind('.'));
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
namespace Swig_python {
extern int createSWIGPointerObj_T(const char* TypeName, void* obj, PyObject** ptr, int own);
extern int convertSWIGPointerObj_T(const char* TypeName, PyObject* obj, void** ptr, int flags);
extern void cleanupSWIG_T(const char* TypeName);
extern int getSWIGPointerTypeObj_T(const char* TypeName, PyTypeObject** ptr);
}
#endif

PyObject* InterpreterSingleton::createSWIGPointerObj(const char* Module, const char* TypeName, void* Pointer, int own)
{
    int result = 0;
    PyObject* proxy=nullptr;
    PyGILStateLocker locker;
    (void)Module;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    result = Swig_python::createSWIGPointerObj_T(TypeName, Pointer, &proxy, own);
#else
    (void)TypeName;
    (void)Pointer;
    (void)own;
    result = -1; // indicates error
#endif

    if (result == 0)
        return proxy;

    // none of the SWIG's succeeded
    throw Base::RuntimeError("No SWIG wrapped library loaded");
}

bool InterpreterSingleton::convertSWIGPointerObj(const char* Module, const char* TypeName, PyObject* obj, void** ptr, int flags)
{
    int result = 0;
    PyGILStateLocker locker;
    (void)Module;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    result = Swig_python::convertSWIGPointerObj_T(TypeName, obj, ptr, flags);
#else
    (void)TypeName;
    (void)obj;
    (void)ptr;
    (void)flags;
    result = -1; // indicates error
#endif

    if (result == 0)
        return true;

    // none of the SWIG's succeeded
    throw Base::RuntimeError("No SWIG wrapped library loaded");
}

void InterpreterSingleton::cleanupSWIG(const char* TypeName)
{
    PyGILStateLocker locker;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    Swig_python::cleanupSWIG_T(TypeName);
#else
    (void)TypeName;
#endif
}

PyTypeObject* InterpreterSingleton::getSWIGPointerTypeObj(const char* Module, const char* TypeName)
{
    int result = 0;
    PyTypeObject* proxy = nullptr;
    PyGILStateLocker locker;
    (void)Module;
#if (defined(HAVE_SWIG) && (HAVE_SWIG == 1))
    result = Swig_python::getSWIGPointerTypeObj_T(TypeName, &proxy);
#else
    (void)TypeName;
    result = -1; // indicates error
#endif

    if (result == 0)
        return proxy;

    // none of the SWIG's succeeded
    throw Base::RuntimeError("No SWIG wrapped library loaded");
}
