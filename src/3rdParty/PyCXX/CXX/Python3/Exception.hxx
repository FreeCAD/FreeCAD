//-----------------------------------------------------------------------------
//
// Copyright (c) 1998 - 2007, The Regents of the University of California
// Produced at the Lawrence Livermore National Laboratory
// All rights reserved.
//
// This file is part of PyCXX. For details,see http://cxx.sourceforge.net/. The
// full copyright notice is contained in the file COPYRIGHT located at the root
// of the PyCXX distribution.
//
// Redistribution  and  use  in  source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
//  - Redistributions of  source code must  retain the above  copyright notice,
//    this list of conditions and the disclaimer below.
//  - Redistributions in binary form must reproduce the above copyright notice,
//    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
//    documentation and/or materials provided with the distribution.
//  - Neither the name of the UC/LLNL nor  the names of its contributors may be
//    used to  endorse or  promote products derived from  this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
// ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
// CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
// ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
// SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
// CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
// LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
// OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
//-----------------------------------------------------------------------------

#ifndef __CXX_Exception_h
#define __CXX_Exception_h

#include "CXX/WrapPython.h"
#include "CXX/Version.hxx"
#include "CXX/Python3/Config.hxx"
#include "CXX/Python3/CxxDebug.hxx"
#include "CXX/Python3/IndirectPythonInterface.hxx"

#include <string>
#include <iostream>

// This mimics the Python structure, in order to minimize confusion
namespace Py
{
    class ExtensionExceptionType;

    class Object;

    class PYCXX_EXPORT BaseException
    {
    public:
        BaseException( ExtensionExceptionType &exception, const std::string &reason );
        BaseException( ExtensionExceptionType &exception, Object &reason );
        BaseException( PyObject *exception, Object &reason );
        BaseException( PyObject *exception, const std::string &reason );
        explicit BaseException();

        void clear(); // clear the error

        // is the exception this specific exception 'exc'
        bool matches( ExtensionExceptionType &exc );

        Object errorType(); // get the error type of the active exception
        Object errorValue();// get the error value of the active exception
    };

    // for user defined exceptions to be made know to pycxx
    typedef void (*throw_exception_func_t)( void );
    void addPythonException( ExtensionExceptionType &py_exc_type, throw_exception_func_t throw_func );

#if defined( PYCXX_6_2_COMPATIBILITY )
    class PYCXX_EXPORT Exception : public BaseException
    {
    public:
        Exception( ExtensionExceptionType &exception, const std::string &reason )
        : BaseException( exception, reason )
        {}

        Exception( ExtensionExceptionType &exception, Object &reason )
        : BaseException( exception, reason )
        {}

        Exception( PyObject *exception, Object &reason  )
        : BaseException( exception, reason  )
        {}

        Exception( PyObject *exception, const std::string &reason )
        : BaseException( exception, reason )
        {}

        explicit Exception()
        : BaseException()
        {}
    };
#endif

#define PYCXX_STANDARD_EXCEPTION( eclass, bclass ) \
    class PYCXX_EXPORT eclass : public bclass \
    { \
    public: \
        eclass() {} \
        eclass( const char *reason ) { PyErr_SetString( _Exc_##eclass(), reason ); } \
        eclass( const std::string &reason ) { PyErr_SetString( _Exc_##eclass(), reason.c_str() ); } \
        ~eclass() {} \
        \
        static void throwFunc() { throw eclass(); } \
        static PyObject *exceptionType() { return _Exc_##eclass(); } \
    }; \

#include <CXX/Python3/cxx_standard_exceptions.hxx>

#undef PYCXX_STANDARD_EXCEPTION

#define PYCXX_USER_EXCEPTION_STR_ARG( uclass ) \
class PYCXX_EXPORT uclass : public Py::BaseException \
{ \
public: \
    uclass( const std::string &reason ) \
    : Py::BaseException( m_error, reason ) \
    { } \
    ~uclass() {} \
    static void init( Py::ExtensionModuleBase &module ) \
    { \
        m_error.init( module, #uclass ); \
        Py::addPythonException( m_error, throwFunc ); \
        Py::Dict d( module.moduleDictionary() ); \
        d[#uclass] = m_error; \
    } \
private: \
    uclass() : Py::BaseException() {} \
    static void throwFunc() \
    { \
        throw uclass(); \
    } \
    static Py::ExtensionExceptionType m_error; \
}; \
Py::ExtensionExceptionType uclass::m_error;

#define PYCXX_USER_EXCEPTION_NO_ARG( uclass ) \
class PYCXX_EXPORT uclass : public Py::BaseException \
{ \
public: \
    uclass() \
    : Py::BaseException() \
    { } \
    ~uclass() {} \
    static void init( Py::ExtensionModuleBase &module ) \
    { \
        m_error.init( module, #uclass ); \
        Py::addPythonException( m_error, throwFunc ); \
        Py::Dict d( module.moduleDictionary() ); \
        d[#uclass] = m_error; \
    } \
private: \
    static void throwFunc() \
    { \
        throw uclass(); \
    } \
    static Py::ExtensionExceptionType m_error; \
}; \
Py::ExtensionExceptionType uclass::m_error;

}// Py

#endif
