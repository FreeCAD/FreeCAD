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

#pragma once

#if defined(_POSIX_C_SOURCE)
# undef _POSIX_C_SOURCE
#endif  // (re-)defined in pyconfig.h
#if defined(_XOPEN_SOURCE)
# undef _XOPEN_SOURCE
#endif  // (re-)defined in pyconfig.h

#include <FCConfig.h>

#ifdef FC_OS_MACOSX
# undef toupper
# undef tolower
# undef isupper
# undef islower
# undef isspace
# undef isalpha
# undef isalnum
#endif

#include <CXX/Extensions.hxx>
#include <list>
#include <string>
#include "Exception.h"


// NOLINTBEGIN
/** Helper macro to obtain callable from an object
 *
 * @param _pyobj: PyObject pointer
 * @param _name: the callable string name
 * @param _var: the callable variable to be assigned
 *
 *  See FeaturePythonImp::init() for example usage
 */
#define FC_PY_GetCallable(_pyobj, _name, _var) \
    do { \
        _var = Py::Object(); \
        if (PyObject_HasAttrString(_pyobj, _name)) { \
            Py::Object _obj(PyObject_GetAttrString(_pyobj, _name), true); \
            if (_obj.isCallable()) \
                _var = _obj; \
        } \
    } while (0)

/** Helper macro to obtain attribute from an object
 *
 * @param _pyobj: PyObject pointer
 * @param _name: the attribute string name
 * @param _var: the attribute variable to be assigned
 *
 *  See FeaturePythonImp::init() for example usage
 */
#define FC_PY_GetObject(_pyobj, _name, _var) \
    do { \
        _var = Py::Object(); \
        if (PyObject_HasAttrString(_pyobj, _name)) \
            _var = Py::asObject(PyObject_GetAttrString(_pyobj, _name)); \
    } while (0)
// NOLINTEND


namespace Base
{


class BaseExport PyException: public Exception
{
public:
    /// constructor does the whole job
    PyException();
    explicit PyException(const Py::Object& obj);
    PyException(const PyException&) = default;
    PyException(PyException&&) = default;
    ~PyException() noexcept override;

    PyException& operator=(const PyException&) = default;
    PyException& operator=(PyException&&) = default;
    void raiseException();

    /// this method determines if the original exception
    /// can be reconstructed or not, if yes throws the reconstructed version
    /// if not, throws a generic PyException.
    static void throwException();

    ///  this function returns the stack trace
    const std::string& getStackTrace() const
    {
        return _stackTrace;
    }
    const std::string& getErrorType() const
    {
        return _errorType;
    }
    PyObject* getPyExceptionType() const override
    {
        return _exceptionType;
    }
    void reportException() const override;
    /// Sets the Python error indicator and an error message
    void setPyException() const override;

private:
    std::string _stackTrace;
    std::string _errorType;
    PyObject* _exceptionType;
};

inline Py::Object pyCall(PyObject* callable, PyObject* args = nullptr)
{
    PyObject* result = PyObject_CallObject(callable, args);
    if (!result) {
        throw Py::Exception();
    }
    return Py::asObject(result);
}

inline Py::Object pyCallWithKeywords(PyObject* callable, PyObject* args, PyObject* kwds = nullptr)
{
    PyObject* result = PyObject_Call(callable, args, kwds);
    if (!result) {
        throw Py::Exception();
    }
    return Py::asObject(result);
}

/**
 * The SystemExitException is thrown if the Python-internal PyExc_SystemExit exception
 * was thrown.
 * @author Werner Mayer
 */
class BaseExport SystemExitException: public Exception
{
public:
    SystemExitException();
    SystemExitException(const SystemExitException&) = default;
    SystemExitException(SystemExitException&&) = default;
    ~SystemExitException() noexcept override = default;
    SystemExitException& operator=(const SystemExitException&) = default;
    SystemExitException& operator=(SystemExitException&&) = default;
    long getExitCode() const
    {
        return _exitCode;
    }

private:
    long _exitCode;
};

/** If the application starts we release immediately the global interpreter lock
 * (GIL) once the Python interpreter is initialized, i.e. no thread -- including
 * the main thread doesn't hold the GIL. Thus, every thread must instantiate an
 * object of PyGILStateLocker if it needs to access protected areas in Python or
 * areas where the lock is needed. It's best to create the instance on the stack,
 * not on the heap.
 */
class BaseExport PyGILStateLocker
{
public:
    PyGILStateLocker()
    {
        gstate = PyGILState_Ensure();  // NOLINT
    }
    ~PyGILStateLocker()
    {
        PyGILState_Release(gstate);
    }

    PyGILStateLocker(const PyGILStateLocker&) = delete;
    PyGILStateLocker(PyGILStateLocker&&) = delete;
    PyGILStateLocker& operator=(const PyGILStateLocker&) = delete;
    PyGILStateLocker& operator=(PyGILStateLocker&&) = delete;

private:
    PyGILState_STATE gstate;
};

/**
 * If a thread holds the global interpreter lock (GIL) but runs a long operation
 * in C where it doesn't need to hold the GIL it can release it temporarily. Or
 * if the thread has to run code in the main thread where Python code may be
 * executed it must release the GIL to avoid a deadlock. In either case the thread
 * must hold the GIL when instantiating an object of PyGILStateRelease.
 * As PyGILStateLocker it's best to create an instance of PyGILStateRelease on the
 * stack.
 */
class BaseExport PyGILStateRelease
{
public:
    PyGILStateRelease()
    {
        // release the global interpreter lock
        state = PyEval_SaveThread();  // NOLINT
    }
    ~PyGILStateRelease()
    {
        // grab the global interpreter lock again
        PyEval_RestoreThread(state);
    }

    PyGILStateRelease(const PyGILStateRelease&) = delete;
    PyGILStateRelease(PyGILStateRelease&&) = delete;
    PyGILStateRelease& operator=(const PyGILStateRelease&) = delete;
    PyGILStateRelease& operator=(PyGILStateRelease&&) = delete;

private:
    PyThreadState* state;
};


/** The Interpreter class
 *  This class manage the python interpreter and hold a lot
 *  helper functions for handling python stuff
 */
class BaseExport InterpreterSingleton
{
public:
    InterpreterSingleton();
    ~InterpreterSingleton();

    InterpreterSingleton(const InterpreterSingleton&) = delete;
    InterpreterSingleton(InterpreterSingleton&&) = delete;
    InterpreterSingleton& operator=(const InterpreterSingleton&) = delete;
    InterpreterSingleton& operator=(InterpreterSingleton&&) = delete;

    /** @name execution methods
     */
    //@{
    /// Run a statement on the python interpreter and gives back a string with the representation of
    /// the result.
    std::string runString(const char* psCmd);
    /// Run a statement on the python interpreter with a key for exchanging strings
    std::string runStringWithKey(const char* psCmd, const char* key, const char* key_initial_value = "");
    /// Run a statement on the python interpreter and return back the result object.
    Py::Object runStringObject(const char* sCmd);
    /// Run a statement on the python interpreter and gives back a string with the representation of
    /// the result.
    void runInteractiveString(const char* psCmd);
    /// Run file (script) on the python interpreter
    void runFile(const char* pxFileName, bool local);
    /// Run a statement with arguments on the python interpreter
    void runStringArg(const char* psCom, ...);
    /// runs a python object method with no return value and no arguments
    void runMethodVoid(PyObject* pobject, const char* method);
    /// runs a python object method which returns a arbitrary object
    PyObject* runMethodObject(PyObject* pobject, const char* method);
    /// runs a python method with arbitrary params
    void runMethod(
        PyObject* pobject,
        const char* method,
        const char* resfmt = nullptr,
        void* cresult = nullptr,
        const char* argfmt = "()",
        ...
    );
    //@}

    /** @name Module handling
     */
    //@{
    /* Loads a module
     */
    bool loadModule(const char* psModName);
    /// Add an additional python path
    void addPythonPath(const char* Path);
    /// Get the path (replaces the deprecated Py_GetPath method)
    std::string getPythonPath();
    static void addType(PyTypeObject* Type, PyObject* Module, const char* Name);
    /// Add a module and return a PyObject to it
    PyObject* addModule(Py::ExtensionModuleBase*);
    /// Clean-up registered modules
    void cleanupModules();
    //@}

    /** @name Cleanup
     */
    //@{
    /** Register a cleanup function to be called by finalize(). The cleanup function will be called
     * with no arguments and should return no value. At most 32 cleanup functions can be registered.
     * When the registration is successful 0 is returned; on failure -1 is returned. The cleanup
     * function registered last is called first. Each cleanup function will be called at most once.
     * Since Python's internal finalization will have completed before the cleanup function, no
     * Python APIs should be called by \a func.
     */
    int cleanup(void (*func)());
    /** This calls the registered cleanup functions. @see cleanup() for more details. */
    void finalize();
    /// This shuts down the application.
    void systemExit();
    //@}

    /** @name startup and singletons
     */
    //@{
    /// init the interpreter and returns the module search path
    std::string init(int argc, char* argv[]);
    int runCommandLine(const char* prompt);
    void replaceStdOutput();
    static InterpreterSingleton& Instance();
    static void Destruct();
    //@}

    /** @name external wrapper libs
     *  here we can access external dynamically loaded wrapper libs
     *  done e.g. by SWIG or SIP
     */
    //@{
    /// generate a SWIG object
    PyObject* createSWIGPointerObj(const char* Module, const char* TypeName, void* Pointer, int own);
    bool convertSWIGPointerObj(
        const char* Module,
        const char* TypeName,
        PyObject* obj,
        void** ptr,
        int flags
    );
    void cleanupSWIG(const char* TypeName);
    PyTypeObject* getSWIGPointerTypeObj(const char* Module, const char* TypeName);
    //@}

    /** @name methods for debugging facility
     */
    //@{
    /// sets the file name which should be debugged
    void dbgObserveFile(const char* sFileName = "");
    //@}


    /** @name static helper functions
     */
    //@{
    /// replaces all char with escapes for usage in python console
    static std::string strToPython(const char* Str);
    static std::string strToPython(const std::string& Str)
    {
        return strToPython(Str.c_str());
    }
    //@}

    PyObject* getValue(const char* key, const char* result_var);

protected:
    // singleton
    static InterpreterSingleton* _pcSingleton;

private:
    std::string _cDebugFileName;
    PyThreadState* _global;
    std::list<Py::ExtensionModuleBase*> _modules;
};


/** Access to the InterpreterSingleton object
 *  This method is used to gain access to the one and only instance of
 *  the InterpreterSingleton class.
 */
inline InterpreterSingleton& Interpreter()
{
    return InterpreterSingleton::Instance();
}

}  // namespace Base
