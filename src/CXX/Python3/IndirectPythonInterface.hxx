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

#ifndef __CXX_INDIRECT_PYTHON_INTERFACE__HXX__
# define __CXX_INDIRECT_PYTHON_INTERFACE__HXX__

# include "CXX/WrapPython.h"
#include "CXX/Config.hxx"

namespace Py
{
bool InitialisePythonIndirectInterface();

//
//    Wrap Exception variables as function calls
//
PYCXX_EXPORT PyObject * _Exc_BaseException();

# define PYCXX_STANDARD_EXCEPTION( eclass, bclass ) \
    PYCXX_EXPORT PyObject * _Exc_##eclass();

# include "CXX/Python3/cxx_standard_exceptions.hxx"
# undef PYCXX_STANDARD_EXCEPTION

//
//    Wrap Object variables as function calls
//
PYCXX_EXPORT PyObject * _None();

PYCXX_EXPORT PyObject * _False();
PYCXX_EXPORT PyObject * _True();

//
//    Wrap Type variables as function calls
//
PYCXX_EXPORT PyTypeObject * _List_Type();
PYCXX_EXPORT bool _List_Check( PyObject *o );

PYCXX_EXPORT PyTypeObject * _Buffer_Type();
PYCXX_EXPORT bool _Buffer_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Class_Type();
PYCXX_EXPORT bool _Class_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Instance_Type();
PYCXX_EXPORT bool _Instance_Check( PyObject *op );

# if !defined( Py_LIMITED_API )
PYCXX_EXPORT PyTypeObject * _Method_Type();
PYCXX_EXPORT bool _Method_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Function_Type();
PYCXX_EXPORT bool _Function_Check( PyObject *op );
# endif

PYCXX_EXPORT PyTypeObject * _Complex_Type();
PYCXX_EXPORT bool _Complex_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Dict_Type();
PYCXX_EXPORT bool _Dict_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _File_Type();
PYCXX_EXPORT bool _File_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Float_Type();
PYCXX_EXPORT bool _Float_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Frame_Type();
PYCXX_EXPORT bool _Frame_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Bool_Type();
PYCXX_EXPORT bool _Boolean_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Int_Type();
PYCXX_EXPORT bool _Int_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _List_Type();
PYCXX_EXPORT bool _List_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Long_Type();
PYCXX_EXPORT bool _Long_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _CFunction_Type();
PYCXX_EXPORT bool _CFunction_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Module_Type();
PYCXX_EXPORT bool _Module_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Type_Type();
PYCXX_EXPORT bool _Type_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Range_Type();
PYCXX_EXPORT bool _Range_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Slice_Type();
PYCXX_EXPORT bool _Slice_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Unicode_Type();
PYCXX_EXPORT bool _Unicode_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _Bytes_Type();
PYCXX_EXPORT bool _Bytes_Check( PyObject *op );

PYCXX_EXPORT PyTypeObject * _TraceBack_Type();
PYCXX_EXPORT bool _TraceBack_Check( PyObject *v );

PYCXX_EXPORT PyTypeObject * _Tuple_Type();
PYCXX_EXPORT bool _Tuple_Check( PyObject *op );

# if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT int &_Py_DebugFlag();
PYCXX_EXPORT int &_Py_InteractiveFlag();
PYCXX_EXPORT int &_Py_OptimizeFlag();
PYCXX_EXPORT int &_Py_NoSiteFlag();
PYCXX_EXPORT int &_Py_TabcheckFlag();
PYCXX_EXPORT int &_Py_VerboseFlag();

#  if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 7
PYCXX_EXPORT const char *__Py_PackageContext();
#  else
PYCXX_EXPORT char *__Py_PackageContext();
#  endif
# endif

PYCXX_EXPORT void _XINCREF( PyObject *op );
PYCXX_EXPORT void _XDECREF( PyObject *op );
}

#endif    // __CXX_INDIRECT_PYTHON_INTERFACE__HXX__
