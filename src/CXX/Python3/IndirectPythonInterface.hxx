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
#define __CXX_INDIRECT_PYTHON_INTERFACE__HXX__

#include "CXX/WrapPython.h"

namespace Py
{
bool InitialisePythonIndirectInterface();

//
//    Wrap Exception variables as function calls
//
PyObject * _Exc_Exception();
PyObject * _Exc_StandardError();
PyObject * _Exc_ArithmeticError();
PyObject * _Exc_LookupError();

PyObject * _Exc_AssertionError();
PyObject * _Exc_AttributeError();
PyObject * _Exc_EOFError();
PyObject * _Exc_FloatingPointError();
PyObject * _Exc_EnvironmentError();
PyObject * _Exc_IOError();
PyObject * _Exc_OSError();
PyObject * _Exc_ImportError();
PyObject * _Exc_IndexError();
PyObject * _Exc_KeyError();
PyObject * _Exc_KeyboardInterrupt();
PyObject * _Exc_MemoryError();
PyObject * _Exc_NameError();
PyObject * _Exc_OverflowError();
PyObject * _Exc_RuntimeError();
PyObject * _Exc_NotImplementedError();
PyObject * _Exc_SyntaxError();
PyObject * _Exc_SystemError();
PyObject * _Exc_SystemExit();
PyObject * _Exc_TypeError();
PyObject * _Exc_ValueError();
PyObject * _Exc_ZeroDivisionError();
#ifdef MS_WINDOWS
PyObject * _Exc_WindowsError();
#endif

PyObject * _Exc_IndentationError();
PyObject * _Exc_TabError();
PyObject * _Exc_UnboundLocalError();
PyObject * _Exc_UnicodeError();

//
//    Wrap Object variables as function calls
//
PyObject * _None();

PyObject * _False();
PyObject * _True();

//
//    Wrap Type variables as function calls
//
PyTypeObject * _List_Type();
bool _List_Check( PyObject *o );

PyTypeObject * _Buffer_Type();
bool _Buffer_Check( PyObject *op );

PyTypeObject * _Class_Type();
bool _Class_Check( PyObject *op );

PyTypeObject * _Instance_Type();
bool _Instance_Check( PyObject *op );

PyTypeObject * _Method_Type();
bool _Method_Check( PyObject *op );

PyTypeObject * _Complex_Type();
bool _Complex_Check( PyObject *op );

PyTypeObject * _Dict_Type();
bool _Dict_Check( PyObject *op );

PyTypeObject * _File_Type();
bool _File_Check( PyObject *op );

PyTypeObject * _Float_Type();
bool _Float_Check( PyObject *op );

PyTypeObject * _Frame_Type();
bool _Frame_Check( PyObject *op );

PyTypeObject * _Function_Type();
bool _Function_Check( PyObject *op );

PyTypeObject * _Bool_Type();
bool _Boolean_Check( PyObject *op );

PyTypeObject * _Int_Type();
bool _Int_Check( PyObject *op );

PyTypeObject * _List_Type();
bool _List_Check( PyObject *op );

PyTypeObject * _Long_Type();
bool _Long_Check( PyObject *op );

PyTypeObject * _CFunction_Type();
bool _CFunction_Check( PyObject *op );

PyTypeObject * _Module_Type();
bool _Module_Check( PyObject *op );

PyTypeObject * _Type_Type();
bool _Type_Check( PyObject *op );

PyTypeObject * _Range_Type();
bool _Range_Check( PyObject *op );

PyTypeObject * _Slice_Type();
bool _Slice_Check( PyObject *op );

PyTypeObject * _Unicode_Type();
bool _Unicode_Check( PyObject *op );

PyTypeObject * _Bytes_Type();
bool _Bytes_Check( PyObject *op );

PyTypeObject * _TraceBack_Type();
bool _TraceBack_Check( PyObject *v );

PyTypeObject * _Tuple_Type();
bool _Tuple_Check( PyObject *op );

int &_Py_DebugFlag();
int &_Py_InteractiveFlag();
int &_Py_OptimizeFlag();
int &_Py_NoSiteFlag();
int &_Py_TabcheckFlag();
int &_Py_VerboseFlag();
int &_Py_UnicodeFlag();

void _XINCREF( PyObject *op );
void _XDECREF( PyObject *op );

char *__Py_PackageContext();
};

#endif    // __CXX_INDIRECT_PYTHON_INTERFACE__HXX__
