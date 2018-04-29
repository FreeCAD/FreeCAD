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
#include "CXX/Config.hxx"
#include "CXX/IndirectPythonInterface.hxx"

#include <string>
#include <iostream>

// This mimics the Python structure, in order to minimize confusion
namespace Py
{
    class ExtensionExceptionType;

    class Object;

    class PYCXX_EXPORT Exception
    {
    public:
        Exception( ExtensionExceptionType &exception, const std::string& reason );
        Exception( ExtensionExceptionType &exception, Object &reason );

        explicit Exception ()
        {}
        
        // This overloaded constructor will be removed in future PyCXX versions
        //Exception (const std::string& reason)
        //{
        //    PyErr_SetString (Py::_Exc_RuntimeError(), reason.c_str());
        //}
        
        Exception (PyObject* exception, const std::string& reason)
        {
            PyErr_SetString (exception, reason.c_str());
        }
        
        Exception (PyObject* exception, Object &reason);        

        void clear() // clear the error
        // technically but not philosophically const
        {
            PyErr_Clear();
        }
    };
    
    
    // Abstract
    class PYCXX_EXPORT StandardError: public Exception
    {
    protected: 
        explicit StandardError()
        {}
    };
    
    class PYCXX_EXPORT LookupError: public StandardError
    {
    protected: 
        explicit LookupError()
        {}
    };
    
    class PYCXX_EXPORT ArithmeticError: public StandardError
    {
    protected: 
        explicit ArithmeticError()
        {}
    };
    
    class PYCXX_EXPORT EnvironmentError: public StandardError
    {
    protected: 
        explicit EnvironmentError()
        {}
    };
    
    // Concrete
    
    class PYCXX_EXPORT TypeError: public StandardError
    {
    public:
        TypeError (const std::string& reason)
            : StandardError()
        {
            PyErr_SetString (Py::_Exc_TypeError(),reason.c_str());
        }
    };
    
    class PYCXX_EXPORT IndexError: public LookupError
    {
    public:
        IndexError (const std::string& reason)
            : LookupError()
        {
            PyErr_SetString (Py::_Exc_IndexError(), reason.c_str());
        }
    };
    
    class PYCXX_EXPORT AttributeError: public StandardError
    {
    public:
        AttributeError (const std::string& reason)
            : StandardError()
        {
            PyErr_SetString (Py::_Exc_AttributeError(), reason.c_str());
        }        
    };
    
    class PYCXX_EXPORT NameError: public StandardError
    {
    public:
        NameError (const std::string& reason)
            : StandardError()
        {
            PyErr_SetString (Py::_Exc_NameError(), reason.c_str());
        }
    };
    
    class PYCXX_EXPORT RuntimeError: public StandardError
    {
    public:
        RuntimeError (const std::string& reason)
            : StandardError()
        {
            PyErr_SetString (Py::_Exc_RuntimeError(), reason.c_str());
        }
    };
    
    class NotImplementedError: public StandardError
    {
    public:
        NotImplementedError (const std::string& reason)
            : StandardError()
        {
            PyErr_SetString (Py::_Exc_NotImplementedError(), reason.c_str());
        }
    };
    
    class PYCXX_EXPORT SystemError: public StandardError
    {
    public:
        SystemError (const std::string& reason)
            : StandardError()
        {
            PyErr_SetString (Py::_Exc_SystemError(),reason.c_str());
        }
    };
    
    class PYCXX_EXPORT KeyError: public LookupError
    {
    public:
        KeyError (const std::string& reason)
            : LookupError()
        {
            PyErr_SetString (Py::_Exc_KeyError(),reason.c_str());
        }
    };
    
    
    class PYCXX_EXPORT ValueError: public StandardError
    {
    public:
        ValueError (const std::string& reason)
            : StandardError()
        {
            PyErr_SetString (Py::_Exc_ValueError(), reason.c_str());
        }
    };
    
    class PYCXX_EXPORT OverflowError: public ArithmeticError
    {
    public:
        OverflowError (const std::string& reason)
            : ArithmeticError()
        {
            PyErr_SetString (Py::_Exc_OverflowError(), reason.c_str());
        }        
    };
    
    class PYCXX_EXPORT ZeroDivisionError: public ArithmeticError
    {
    public:
        ZeroDivisionError (const std::string& reason)
            : ArithmeticError() 
        {
            PyErr_SetString (Py::_Exc_ZeroDivisionError(), reason.c_str());
        }
    };
    
    class PYCXX_EXPORT FloatingPointError: public ArithmeticError
    {
    public:
        FloatingPointError (const std::string& reason)
            : ArithmeticError() 
        {
            PyErr_SetString (Py::_Exc_FloatingPointError(), reason.c_str());
        }
    };
    
    class PYCXX_EXPORT MemoryError: public StandardError
    {
    public:
        MemoryError (const std::string& reason)
            : StandardError()
        {
            PyErr_SetString (Py::_Exc_MemoryError(), reason.c_str());
        }    
    };
    
    class PYCXX_EXPORT SystemExit: public StandardError
    {
    public:
        SystemExit (const std::string& reason)
            : StandardError() 
        {
            PyErr_SetString (Py::_Exc_SystemExit(),reason.c_str());
        }
    };

}// Py

#endif
