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

#ifndef __CXX_Extensions__h
#define __CXX_Extensions__h


#ifdef _MSC_VER
// disable warning C4786: symbol greater than 255 character,
// okay to ignore
#pragma warning( disable: 4786 )
#endif

#include "CXX/WrapPython.h"
#include "CXX/Version.hxx"
#include "CXX/Python3/Config.hxx"
#include "CXX/Python3/CxxDebug.hxx"
#include "CXX/Python3/Objects.hxx"

extern "C" { extern PyObject py_object_initializer; }

#include <vector>
#include <map>

// ----------------------------------------------------------------------

namespace Py
{
    class ExtensionModuleBase;

    // Make an Exception Type for use in raising custom exceptions
    class PYCXX_EXPORT ExtensionExceptionType : public Object
    {
    public:
        ExtensionExceptionType();
        virtual ~ExtensionExceptionType();

        // call init to create the type
        void init( ExtensionModuleBase &module, const std::string &name, ExtensionExceptionType &parent );
        void init( ExtensionModuleBase &module, const std::string &name );
    };

    class PYCXX_EXPORT MethodTable
    {
    public:
        MethodTable();
        virtual ~MethodTable();

        void add( const char *method_name, PyCFunction f, const char *doc="", int flag=1 );
        PyMethodDef *table();

    protected:
        std::vector<PyMethodDef> t;    // accumulator of PyMethodDef's
        PyMethodDef *mt;        // Actual method table produced when full

        static PyMethodDef method( const char* method_name, PyCFunction f, int flags=1, const char* doc="" );

    private:
        //
        // prevent the compiler generating these unwanted functions
        //
        MethodTable( const MethodTable &m );    //unimplemented
        void operator=( const MethodTable &m );    //unimplemented

    }; // end class MethodTable

    // Note: Python calls noargs as varargs buts args==NULL
    extern "C" typedef PyObject *(*method_noargs_call_handler_t)( PyObject *_self, PyObject * );
    extern "C" typedef PyObject *(*method_varargs_call_handler_t)( PyObject *_self, PyObject *_args );
    extern "C" typedef PyObject *(*method_keyword_call_handler_t)( PyObject *_self, PyObject *_args, PyObject *_dict );

    template<class T>
    class MethodDefExt
    {
    public:
        typedef Object (T::*method_noargs_function_t)();
        typedef Object (T::*method_varargs_function_t)( const Tuple &args );
        typedef Object (T::*method_keyword_function_t)( const Tuple &args, const Dict &kws );

        // NOARGS
        MethodDefExt
        (
            const char *_name,
            method_noargs_function_t _function,
            method_noargs_call_handler_t _handler,
            const char *_doc
        )
        {
            ext_meth_def.ml_name = const_cast<char *>( _name );
            ext_meth_def.ml_meth = reinterpret_cast<method_varargs_call_handler_t>( _handler );
            ext_meth_def.ml_flags = METH_NOARGS;
            ext_meth_def.ml_doc = const_cast<char *>( _doc );

            ext_noargs_function = _function;
            ext_varargs_function = NULL;
            ext_keyword_function = NULL;
        }

        // VARARGS
        MethodDefExt
        (
            const char *_name,
            method_varargs_function_t _function,
            method_varargs_call_handler_t _handler,
            const char *_doc
        )
        {
            ext_meth_def.ml_name = const_cast<char *>( _name );
            ext_meth_def.ml_meth = reinterpret_cast<method_varargs_call_handler_t>( _handler );
            ext_meth_def.ml_flags = METH_VARARGS;
            ext_meth_def.ml_doc = const_cast<char *>( _doc );

            ext_noargs_function = NULL;
            ext_varargs_function = _function;
            ext_keyword_function = NULL;
        }

        // VARARGS + KEYWORD
        MethodDefExt
        (
            const char *_name,
            method_keyword_function_t _function,
            method_keyword_call_handler_t _handler,
            const char *_doc
        )
        {
            ext_meth_def.ml_name = const_cast<char *>( _name );
            ext_meth_def.ml_meth = reinterpret_cast<method_varargs_call_handler_t>(reinterpret_cast<void (*) (void)>(_handler));
            ext_meth_def.ml_flags = METH_VARARGS|METH_KEYWORDS;
            ext_meth_def.ml_doc = const_cast<char *>( _doc );

            ext_noargs_function = NULL;
            ext_varargs_function = NULL;
            ext_keyword_function = _function;
        }

        ~MethodDefExt()
        {}

        PyMethodDef ext_meth_def;
        method_noargs_function_t ext_noargs_function;
        method_varargs_function_t ext_varargs_function;
        method_keyword_function_t ext_keyword_function;
        Object py_method;
    };
} // Namespace Py

#include "CXX/Python3/ExtensionModule.hxx"
#include "CXX/Python3/PythonType.hxx"
#include "CXX/Python3/ExtensionTypeBase.hxx"
#include "CXX/Python3/ExtensionOldType.hxx"
#include "CXX/Python3/ExtensionType.hxx"

// End of CXX_Extensions.h
#endif
