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

#ifndef BASE_INTERPRETER_H
#define BASE_INTERPRETER_H

#if defined (_POSIX_C_SOURCE)
#   undef  _POSIX_C_SOURCE
#endif // (re-)defined in pyconfig.h
#if defined (_XOPEN_SOURCE)
#   undef _XOPEN_SOURCE
#endif // (re-)defined in pyconfig.h


#include <Python.h>
#include <CXX/Extensions.hxx>


#ifdef FC_OS_MACOSX
#undef toupper
#undef tolower
#undef isupper
#undef islower
#undef isspace
#undef isalpha
#undef isalnum
#endif

// Std. configurations
#include <string>
#include <map>

#include "Exception.h"


namespace Base {

    using std::string;
    using std::vector;



class BaseExport PyException : public Exception
{
public:
    /// constructor does the whole job
    PyException(void);
    ~PyException() throw();

    ///  this function returns the stack trace
    const std::string &getStackTrace(void) const {return _stackTrace;}
    const std::string &getErrorType(void) const {return _errorType;}
    void ReportException (void) const;

protected:
    std::string _stackTrace;
    std::string _errorType;
};

/**
 * The SystemExitException is thrown if the Python-internal PyExc_SystemExit exception
 * was thrown.
 * @author Werner Mayer
 */
class BaseExport SystemExitException : public Exception
{
public:
    SystemExitException(void);
    SystemExitException(const SystemExitException &inst);
    virtual ~SystemExitException() throw() {}
    const long getExitCode(void) const { return _exitCode;}

protected:
    long int _exitCode;
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
        gstate = PyGILState_Ensure();
    }
    ~PyGILStateLocker()
    {
        PyGILState_Release(gstate);
    }

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
        state = PyEval_SaveThread();
    }
    ~PyGILStateRelease()
    {
        // grab the global interpreter lock again
        PyEval_RestoreThread(state);
    }

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

    /** @name execution methods
     */
    //@{
    /// Run a statement on the python interpreter and gives back a string with the representation of the result.
    std::string runString(const char *psCmd);
    /// Runs a string (expression) and returns object returned by expression.
    Py::Object runString_returnObject(const char *sCmd);
    /// Run a statement on the python interpreter and gives back a string with the representation of the result.
    void runInteractiveString(const char *psCmd);
    /// Run file (script) on the python interpreter
    void runFile(const char*pxFileName, bool local);
    /// Run a statement with arguments on the python interpreter
    void runStringArg(const char * psCom,...);
    /// runs a python object method with no return value and no arguments
    void runMethodVoid(PyObject *pobject, const char *method);
    /// runs a python object method which returns a arbitrary object
    PyObject* runMethodObject(PyObject *pobject, const char *method);
    /// runs a python method with arbitrary params
    void runMethod(PyObject *pobject, const char *method,
                   const char *resfmt=0,   void *cresult=0,   
                   const char *argfmt="()",   ...  );
    //@}

    /** @name Module handling
     */
    //@{
    /* Loads a module
     */
    bool loadModule(const char* psModName);
    /// Add an addtional pyhton path
    void addPythonPath(const char* Path);
    static void addType(PyTypeObject* Type,PyObject* Module, const char * Name);
    //@}

    /** @name Cleanup
     */
    //@{
    /** Register a cleanup function to be called by finalize(). The cleanup function will be called with no 
     * arguments and should return no value. At most 32 cleanup functions can be registered.When the registration 
     * is successful 0 is returned; on failure -1 is returned. The cleanup function registered last is called 
     * first. Each cleanup function will be called at most once. Since Python's internal finallization will have 
     * completed before the cleanup function, no Python APIs should be called by \a func. 
     */
    int cleanup(void (*func)(void));
    /** This calls the registered cleanup functions. @see cleanup() for more details. */
    void finalize();
    /// This shuts down the application.
    void systemExit();
    //@}

    /** @name startup and singletons
     */
    //@{
    /// init the interpreter and returns the module search path
    const char* init(int argc,char *argv[]);
    int  runCommandLine(const char *prompt);
    void replaceStdOutput();
    static InterpreterSingleton &Instance(void);
    static void Destruct(void);
    //@}

    /** @name external wrapper libs
     *  here we can access external dynamically loaded wrapper libs
     *  done e.g. by SWIG or SIP
     */
    //@{
    /// generate a SWIG object
    PyObject* createSWIGPointerObj(const char* Modole, const char* TypeName, void* Pointer, int own);
    bool convertSWIGPointerObj(const char* Module, const char* TypeName, PyObject* obj, void** ptr, int flags);
    void cleanupSWIG(const char* TypeName);
    //@}

    /** @name methods for debugging facility
     */
    //@{
    /// sets the file name which should be debugged
    void dbgObserveFile(const char* sFileName="");
    /// sets a break point to a special line number in the current file
    void dbgSetBreakPoint(unsigned int uiLineNumber);
    /// unsets a break point to a special line number in the current file
    void dbgUnsetBreakPoint(unsigned int uiLineNumber);
    /// One step further
    void dbgStep(void);
    //@}


    /** @name static helper functions
     */
    //@{
    /// replaces all char with escapes for usage in python console
    static const std::string strToPython(const char* Str);
    static const std::string strToPython(const std::string &Str){return strToPython(Str.c_str());}
    //@}

    PyObject *getValue(const char *key, const char *result_var);

protected:
    // singleton
    static InterpreterSingleton *_pcSingelton;

private:
    std::string _cDebugFileName;
    PyThreadState* _global;
};


/** Access to the InterpreterSingleton object
 *  This method is used to gain access to the one and only instance of 
 *  the InterpreterSingleton class.
 */  
inline InterpreterSingleton &Interpreter(void)
{
    return InterpreterSingleton::Instance();
}

} //namespace Base 

#endif // BASE_INTERPRETER_H
