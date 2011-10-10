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

#include "CXX/IndirectPythonInterface.hxx"

namespace Py
{
bool _Buffer_Check( PyObject *op ) { return (op)->ob_type == _Buffer_Type(); }
bool _CFunction_Check( PyObject *op ) { return (op)->ob_type == _CFunction_Type(); }
bool _Class_Check( PyObject *op ) { return (op)->ob_type == _Class_Type(); }
bool _CObject_Check( PyObject *op ) { return (op)->ob_type == _CObject_Type(); }
bool _Complex_Check( PyObject *op ) { return (op)->ob_type == _Complex_Type(); }
bool _Dict_Check( PyObject *op ) { return (op)->ob_type == _Dict_Type(); }
bool _File_Check( PyObject *op ) { return (op)->ob_type == _File_Type(); }
bool _Float_Check( PyObject *op ) { return (op)->ob_type == _Float_Type(); }
bool _Function_Check( PyObject *op ) { return (op)->ob_type == _Function_Type(); }
bool _Instance_Check( PyObject *op ) { return (op)->ob_type == _Instance_Type(); }
bool _Boolean_Check( PyObject *op ) { return (op)->ob_type == _Bool_Type(); }
bool _Int_Check( PyObject *op ) { return (op)->ob_type == _Int_Type(); }
bool _List_Check( PyObject *o ) { return o->ob_type == _List_Type(); }
bool _Long_Check( PyObject *op ) { return (op)->ob_type == _Long_Type(); }
bool _Method_Check( PyObject *op ) { return (op)->ob_type == _Method_Type(); }
bool _Module_Check( PyObject *op ) { return (op)->ob_type == _Module_Type(); }
bool _Range_Check( PyObject *op ) { return (op)->ob_type == _Range_Type(); }
bool _Slice_Check( PyObject *op ) { return (op)->ob_type == _Slice_Type(); }
bool _String_Check( PyObject *o ) { return o->ob_type == _String_Type(); }
bool _TraceBack_Check( PyObject *v ) { return (v)->ob_type == _TraceBack_Type(); }
bool _Tuple_Check( PyObject *op ) { return (op)->ob_type == _Tuple_Type(); }
bool _Type_Check( PyObject *op ) { return (op)->ob_type == _Type_Type(); }

#if PY_MAJOR_VERSION >= 2
bool _Unicode_Check( PyObject *op ) { return (op)->ob_type == _Unicode_Type(); }
#endif



#if defined(PY_WIN32_DELAYLOAD_PYTHON_DLL)

#if defined(MS_WINDOWS)
#include <windows.h>


static HMODULE python_dll;

static PyObject *ptr__Exc_ArithmeticError = NULL;
static PyObject *ptr__Exc_AssertionError = NULL;
static PyObject *ptr__Exc_AttributeError = NULL;
static PyObject *ptr__Exc_EnvironmentError = NULL;
static PyObject *ptr__Exc_EOFError = NULL;
static PyObject *ptr__Exc_Exception = NULL;
static PyObject *ptr__Exc_FloatingPointError = NULL;
static PyObject *ptr__Exc_ImportError = NULL;
static PyObject *ptr__Exc_IndexError = NULL;
static PyObject *ptr__Exc_IOError = NULL;
static PyObject *ptr__Exc_KeyboardInterrupt = NULL;
static PyObject *ptr__Exc_KeyError = NULL;
static PyObject *ptr__Exc_LookupError = NULL;
static PyObject *ptr__Exc_MemoryError = NULL;
static PyObject *ptr__Exc_MemoryErrorInst = NULL;
static PyObject *ptr__Exc_NameError = NULL;
static PyObject *ptr__Exc_NotImplementedError = NULL;
static PyObject *ptr__Exc_OSError = NULL;
static PyObject *ptr__Exc_OverflowError = NULL;
static PyObject *ptr__Exc_RuntimeError = NULL;
static PyObject *ptr__Exc_StandardError = NULL;
static PyObject *ptr__Exc_SyntaxError = NULL;
static PyObject *ptr__Exc_SystemError = NULL;
static PyObject *ptr__Exc_SystemExit = NULL;
static PyObject *ptr__Exc_TypeError = NULL;
static PyObject *ptr__Exc_ValueError = NULL;
static PyObject *ptr__Exc_ZeroDivisionError = NULL;

#ifdef MS_WINDOWS
static PyObject *ptr__Exc_WindowsError = NULL;
#endif

#if PY_MAJOR_VERSION >= 2
static PyObject *ptr__Exc_IndentationError = NULL;
static PyObject *ptr__Exc_TabError = NULL;
static PyObject *ptr__Exc_UnboundLocalError = NULL;
static PyObject *ptr__Exc_UnicodeError = NULL;
#endif

static PyObject *ptr__PyNone = NULL;

static PyObject *ptr__PyFalse = NULL;
static PyObject *ptr__PyTrue = NULL;

static PyTypeObject *ptr__Buffer_Type = NULL;
static PyTypeObject *ptr__CFunction_Type = NULL;
static PyTypeObject *ptr__Class_Type = NULL;
static PyTypeObject *ptr__CObject_Type = NULL;
static PyTypeObject *ptr__Complex_Type = NULL;
static PyTypeObject *ptr__Dict_Type = NULL;
static PyTypeObject *ptr__File_Type = NULL;
static PyTypeObject *ptr__Float_Type = NULL;
static PyTypeObject *ptr__Function_Type = NULL;
static PyTypeObject *ptr__Instance_Type = NULL;
static PyTypeObject *ptr__Int_Type = NULL;
static PyTypeObject *ptr__List_Type = NULL;
static PyTypeObject *ptr__Long_Type = NULL;
static PyTypeObject *ptr__Method_Type = NULL;
static PyTypeObject *ptr__Module_Type = NULL;
static PyTypeObject *ptr__Range_Type = NULL;
static PyTypeObject *ptr__Slice_Type = NULL;
static PyTypeObject *ptr__String_Type = NULL;
static PyTypeObject *ptr__TraceBack_Type = NULL;
static PyTypeObject *ptr__Tuple_Type = NULL;
static PyTypeObject *ptr__Type_Type = NULL;

#if PY_MAJOR_VERSION >= 2
static PyTypeObject *ptr__Unicode_Type = NULL;
#endif

static int *ptr_Py_DebugFlag = NULL;
static int *ptr_Py_InteractiveFlag = NULL;
static int *ptr_Py_OptimizeFlag = NULL;
static int *ptr_Py_NoSiteFlag = NULL;
static int *ptr_Py_TabcheckFlag = NULL;
static int *ptr_Py_VerboseFlag = NULL;

#if PY_MAJOR_VERSION >= 2
static int *ptr_Py_UnicodeFlag = NULL;
#endif

static char **ptr__Py_PackageContext = NULL;

#ifdef Py_REF_DEBUG
int *ptr_Py_RefTotal;
#endif


//--------------------------------------------------------------------------------
class GetAddressException
{
public:
    GetAddressException( const char *_name )
        : name( _name )
    {}
    virtual ~GetAddressException() {}
    const char *name;
};


//--------------------------------------------------------------------------------
static PyObject *GetPyObjectPointer_As_PyObjectPointer( const char *name )
{
    FARPROC addr = GetProcAddress( python_dll, name );
    if( addr == NULL )
        throw GetAddressException( name );

    return *(PyObject **)addr;
}

static PyObject *GetPyObject_As_PyObjectPointer( const char *name )
{
    FARPROC addr = GetProcAddress( python_dll, name );
    if( addr == NULL )
        throw GetAddressException( name );

    return (PyObject *)addr;
}

static PyTypeObject *GetPyTypeObjectPointer_As_PyTypeObjectPointer( const char *name )
{
    FARPROC addr = GetProcAddress( python_dll, name );
    if( addr == NULL )
        throw GetAddressException( name );

    return *(PyTypeObject **)addr;
}

static PyTypeObject *GetPyTypeObject_As_PyTypeObjectPointer( const char *name )
{
    FARPROC addr = GetProcAddress( python_dll, name );
    if( addr == NULL )
        throw GetAddressException( name );

    return (PyTypeObject *)addr;
}

static int *GetInt_as_IntPointer( const char *name )
{
    FARPROC addr = GetProcAddress( python_dll, name );
    if( addr == NULL )
        throw GetAddressException( name );

    return (int *)addr;
}

static char **GetCharPointer_as_CharPointerPointer( const char *name )
{
    FARPROC addr = GetProcAddress( python_dll, name );
    if( addr == NULL )
        throw GetAddressException( name );

    return (char **)addr;
}


#ifdef _DEBUG
static const char python_dll_name_format[] = "PYTHON%1.1d%1.1d_D.DLL";
#else
static const char python_dll_name_format[] = "PYTHON%1.1d%1.1d.DLL";
#endif

//--------------------------------------------------------------------------------
bool InitialisePythonIndirectInterface()
{
    char python_dll_name[sizeof(python_dll_name_format)];

    sprintf( python_dll_name, python_dll_name_format, PY_MAJOR_VERSION, PY_MINOR_VERSION );

    python_dll = LoadLibrary( python_dll_name );
    if( python_dll == NULL )
        return false;

    try
{
#ifdef Py_REF_DEBUG
    ptr_Py_RefTotal            = GetInt_as_IntPointer( "_Py_RefTotal" );
#endif
    ptr_Py_DebugFlag        = GetInt_as_IntPointer( "Py_DebugFlag" );
    ptr_Py_InteractiveFlag        = GetInt_as_IntPointer( "Py_InteractiveFlag" );
    ptr_Py_OptimizeFlag        = GetInt_as_IntPointer( "Py_OptimizeFlag" );
    ptr_Py_NoSiteFlag        = GetInt_as_IntPointer( "Py_NoSiteFlag" );
    ptr_Py_TabcheckFlag        = GetInt_as_IntPointer( "Py_TabcheckFlag" );
    ptr_Py_VerboseFlag        = GetInt_as_IntPointer( "Py_VerboseFlag" );
#if PY_MAJOR_VERSION >= 2
    ptr_Py_UnicodeFlag        = GetInt_as_IntPointer( "Py_UnicodeFlag" );
#endif
    ptr__Py_PackageContext        = GetCharPointer_as_CharPointerPointer( "_Py_PackageContext" );

    ptr__Exc_ArithmeticError    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_ArithmeticError" );
    ptr__Exc_AssertionError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_AssertionError" );
    ptr__Exc_AttributeError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_AttributeError" );
    ptr__Exc_EnvironmentError    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_EnvironmentError" );
    ptr__Exc_EOFError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_EOFError" );
    ptr__Exc_Exception        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_Exception" );
    ptr__Exc_FloatingPointError    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_FloatingPointError" );
    ptr__Exc_ImportError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_ImportError" );
    ptr__Exc_IndexError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_IndexError" );
    ptr__Exc_IOError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_IOError" );
    ptr__Exc_KeyboardInterrupt    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_KeyboardInterrupt" );
    ptr__Exc_KeyError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_KeyError" );
    ptr__Exc_LookupError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_LookupError" );
    ptr__Exc_MemoryError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_MemoryError" );
    ptr__Exc_MemoryErrorInst    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_MemoryErrorInst" );
    ptr__Exc_NameError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_NameError" );
    ptr__Exc_NotImplementedError    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_NotImplementedError" );
    ptr__Exc_OSError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_OSError" );
    ptr__Exc_OverflowError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_OverflowError" );
    ptr__Exc_RuntimeError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_RuntimeError" );
    ptr__Exc_StandardError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_StandardError" );
    ptr__Exc_SyntaxError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_SyntaxError" );
    ptr__Exc_SystemError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_SystemError" );
    ptr__Exc_SystemExit        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_SystemExit" );
    ptr__Exc_TypeError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_TypeError" );
    ptr__Exc_ValueError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_ValueError" );
#ifdef MS_WINDOWS
    ptr__Exc_WindowsError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_WindowsError" );
#endif
    ptr__Exc_ZeroDivisionError    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_ZeroDivisionError" );

#if PY_MAJOR_VERSION >= 2
    ptr__Exc_IndentationError    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_IndentationError" );
    ptr__Exc_TabError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_TabError" );
    ptr__Exc_UnboundLocalError    = GetPyObjectPointer_As_PyObjectPointer( "PyExc_UnboundLocalError" );
    ptr__Exc_UnicodeError        = GetPyObjectPointer_As_PyObjectPointer( "PyExc_UnicodeError" );
#endif
    ptr__PyNone            = GetPyObject_As_PyObjectPointer( "_Py_NoneStruct" );

    ptr__PyFalse            = GetPyObject_As_PyObjectPointer( "_Py_ZeroStruct" );
    ptr__PyTrue            = GetPyObject_As_PyObjectPointer( "_Py_TrueStruct" );

    ptr__Buffer_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyBuffer_Type" );
    ptr__CFunction_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyCFunction_Type" );
    ptr__Class_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyClass_Type" );
    ptr__CObject_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyCObject_Type" );
    ptr__Complex_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyComplex_Type" );
    ptr__Dict_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyDict_Type" );
    ptr__File_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyFile_Type" );
    ptr__Float_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyFloat_Type" );
    ptr__Function_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyFunction_Type" );
    ptr__Instance_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyInstance_Type" );
    ptr__Int_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyInt_Type" );
    ptr__List_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyList_Type" );
    ptr__Long_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyLong_Type" );
    ptr__Method_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyMethod_Type" );
    ptr__Module_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyModule_Type" );
    ptr__Range_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyRange_Type" );
    ptr__Slice_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PySlice_Type" );
    ptr__String_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyString_Type" );
    ptr__TraceBack_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyTraceBack_Type" );
    ptr__Tuple_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyTuple_Type" );
    ptr__Type_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyType_Type" );

#if PY_MAJOR_VERSION >= 2
    ptr__Unicode_Type        = GetPyTypeObject_As_PyTypeObjectPointer( "PyUnicode_Type" );
#endif
}
    catch( GetAddressException &e )
    {
        OutputDebugString( python_dll_name );
        OutputDebugString( " does not contain symbol ");
        OutputDebugString( e.name );
        OutputDebugString( "\n" );

        return false;
    }

    return true;
}

//
//    Wrap variables as function calls
//
PyObject * _Exc_ArithmeticError(){ return ptr__Exc_ArithmeticError; }
PyObject * _Exc_AssertionError(){ return ptr__Exc_AssertionError; }
PyObject * _Exc_AttributeError(){ return ptr__Exc_AttributeError; }
PyObject * _Exc_EnvironmentError(){ return ptr__Exc_EnvironmentError; }
PyObject * _Exc_EOFError()    { return ptr__Exc_EOFError; }
PyObject * _Exc_Exception()    { return ptr__Exc_Exception; }
PyObject * _Exc_FloatingPointError(){ return ptr__Exc_FloatingPointError; }
PyObject * _Exc_ImportError()    { return ptr__Exc_ImportError; }
PyObject * _Exc_IndexError()    { return ptr__Exc_IndexError; }
PyObject * _Exc_IOError()    { return ptr__Exc_IOError; }
PyObject * _Exc_KeyboardInterrupt(){ return ptr__Exc_KeyboardInterrupt; }
PyObject * _Exc_KeyError()    { return ptr__Exc_KeyError; }
PyObject * _Exc_LookupError()    { return ptr__Exc_LookupError; }
PyObject * _Exc_MemoryError()    { return ptr__Exc_MemoryError; }
PyObject * _Exc_MemoryErrorInst(){ return ptr__Exc_MemoryErrorInst; }
PyObject * _Exc_NameError()    { return ptr__Exc_NameError; }
PyObject * _Exc_NotImplementedError(){ return ptr__Exc_NotImplementedError; }
PyObject * _Exc_OSError()    { return ptr__Exc_OSError; }
PyObject * _Exc_OverflowError()    { return ptr__Exc_OverflowError; }
PyObject * _Exc_RuntimeError()    { return ptr__Exc_RuntimeError; }
PyObject * _Exc_StandardError()    { return ptr__Exc_StandardError; }
PyObject * _Exc_SyntaxError()    { return ptr__Exc_SyntaxError; }
PyObject * _Exc_SystemError()    { return ptr__Exc_SystemError; }
PyObject * _Exc_SystemExit()    { return ptr__Exc_SystemExit; }
PyObject * _Exc_TypeError()    { return ptr__Exc_TypeError; }
PyObject * _Exc_ValueError()    { return ptr__Exc_ValueError; }
#ifdef MS_WINDOWS
PyObject * _Exc_WindowsError()    { return ptr__Exc_WindowsError; }
#endif
PyObject * _Exc_ZeroDivisionError(){ return ptr__Exc_ZeroDivisionError; }

#if PY_MAJOR_VERSION >= 2
PyObject * _Exc_IndentationError(){ return ptr__Exc_IndentationError; }
PyObject * _Exc_TabError()    { return ptr__Exc_TabError; }
PyObject * _Exc_UnboundLocalError(){ return ptr__Exc_UnboundLocalError; }
PyObject * _Exc_UnicodeError()    { return ptr__Exc_UnicodeError; }
#endif

//
//    wrap items in Object.h
//
PyObject * _None() { return ptr__PyNone; }

PyObject * _False() { return ptr__PyFalse; }
PyObject * _True() { return ptr__PyTrue; }

PyTypeObject * _Buffer_Type()    { return ptr__Buffer_Type; }
PyTypeObject * _CFunction_Type(){ return ptr__CFunction_Type; }
PyTypeObject * _Class_Type()    { return ptr__Class_Type; }
PyTypeObject * _CObject_Type()    { return ptr__CObject_Type; }
PyTypeObject * _Complex_Type()    { return ptr__Complex_Type; }
PyTypeObject * _Dict_Type()    { return ptr__Dict_Type; }
PyTypeObject * _File_Type()    { return ptr__File_Type; }
PyTypeObject * _Float_Type()    { return ptr__Float_Type; }
PyTypeObject * _Function_Type()    { return ptr__Function_Type; }
PyTypeObject * _Instance_Type()    { return ptr__Instance_Type; }
PyTypeObject * _Bool_Type()    { return ptr__Bool_Type; }
PyTypeObject * _Int_Type()    { return ptr__Int_Type; }
PyTypeObject * _List_Type()    { return ptr__List_Type; }
PyTypeObject * _Long_Type()    { return ptr__Long_Type; }
PyTypeObject * _Method_Type()    { return ptr__Method_Type; }
PyTypeObject * _Module_Type()    { return ptr__Module_Type; }
PyTypeObject * _Range_Type()    { return ptr__Range_Type; }
PyTypeObject * _Slice_Type()    { return ptr__Slice_Type; }
PyTypeObject * _String_Type()    { return ptr__String_Type; }
PyTypeObject * _TraceBack_Type(){ return ptr__TraceBack_Type; }
PyTypeObject * _Tuple_Type()    { return ptr__Tuple_Type; }
PyTypeObject * _Type_Type()    { return ptr__Type_Type; }

#if PY_MAJOR_VERSION >= 2
PyTypeObject * _Unicode_Type()    { return ptr__Unicode_Type; }
#endif

char *__Py_PackageContext()    { return *ptr__Py_PackageContext; }


//
//    wrap the Python Flag variables
//
int &_Py_DebugFlag() { return *ptr_Py_DebugFlag; }
int &_Py_InteractiveFlag() { return *ptr_Py_InteractiveFlag; }
int &_Py_OptimizeFlag() { return *ptr_Py_OptimizeFlag; }
int &_Py_NoSiteFlag() { return *ptr_Py_NoSiteFlag; }
int &_Py_TabcheckFlag() { return *ptr_Py_TabcheckFlag; }
int &_Py_VerboseFlag() { return *ptr_Py_VerboseFlag; }
#if PY_MAJOR_VERSION >= 2
int &_Py_UnicodeFlag() { return *ptr_Py_UnicodeFlag; }
#endif

void _XINCREF( PyObject *op )
{
    // This function must match the contents of Py_XINCREF(op)
    if( op == NULL )
        return;

#ifdef Py_REF_DEBUG
    (*ptr_Py_RefTotal)++;
#endif
    (op)->ob_refcnt++;

}

void _XDECREF( PyObject *op )
{
    // This function must match the contents of Py_XDECREF(op);
    if( op == NULL )
        return;

#ifdef Py_REF_DEBUG
    (*ptr_Py_RefTotal)--;
#endif

    if (--(op)->ob_refcnt == 0)
        _Py_Dealloc((PyObject *)(op));
}


#else
#error "Can only delay load under Win32"
#endif

#else

//
//    Duplicated these declarations from rangeobject.h which is missing the
//    extern "C". This has been reported as a bug upto and include 2.1
//
extern "C" DL_IMPORT(PyTypeObject) PyRange_Type;
extern "C" DL_IMPORT(PyObject *) PyRange_New(long, long, long, int);


//================================================================================
//
//    Map onto Macros
//
//================================================================================

//
//    Wrap variables as function calls
//

PyObject * _Exc_ArithmeticError() { return ::PyExc_ArithmeticError; }
PyObject * _Exc_AssertionError() { return ::PyExc_AssertionError; }
PyObject * _Exc_AttributeError() { return ::PyExc_AttributeError; }
PyObject * _Exc_EnvironmentError() { return ::PyExc_EnvironmentError; }
PyObject * _Exc_EOFError() { return ::PyExc_EOFError; }
PyObject * _Exc_Exception() { return ::PyExc_Exception; }
PyObject * _Exc_FloatingPointError() { return ::PyExc_FloatingPointError; }
PyObject * _Exc_ImportError() { return ::PyExc_ImportError; }
PyObject * _Exc_IndexError() { return ::PyExc_IndexError; }
PyObject * _Exc_IOError() { return ::PyExc_IOError; }
PyObject * _Exc_KeyboardInterrupt() { return ::PyExc_KeyboardInterrupt; }
PyObject * _Exc_KeyError() { return ::PyExc_KeyError; }
PyObject * _Exc_LookupError() { return ::PyExc_LookupError; }
PyObject * _Exc_MemoryError() { return ::PyExc_MemoryError; }
PyObject * _Exc_MemoryErrorInst() { return ::PyExc_MemoryErrorInst; }
PyObject * _Exc_NameError() { return ::PyExc_NameError; }
PyObject * _Exc_NotImplementedError() { return ::PyExc_NotImplementedError; }
PyObject * _Exc_OSError() { return ::PyExc_OSError; }
PyObject * _Exc_OverflowError() { return ::PyExc_OverflowError; }
PyObject * _Exc_RuntimeError() { return ::PyExc_RuntimeError; }
PyObject * _Exc_StandardError() { return ::PyExc_StandardError; }
PyObject * _Exc_SyntaxError() { return ::PyExc_SyntaxError; }
PyObject * _Exc_SystemError() { return ::PyExc_SystemError; }
PyObject * _Exc_SystemExit() { return ::PyExc_SystemExit; }
PyObject * _Exc_TypeError() { return ::PyExc_TypeError; }
PyObject * _Exc_ValueError() { return ::PyExc_ValueError; }
PyObject * _Exc_ZeroDivisionError() { return ::PyExc_ZeroDivisionError; }

#ifdef MS_WINDOWS
PyObject * _Exc_WindowsError() { return ::PyExc_WindowsError; }
#endif


#if PY_MAJOR_VERSION >= 2
PyObject * _Exc_IndentationError() { return ::PyExc_IndentationError; }
PyObject * _Exc_TabError() { return ::PyExc_TabError; }
PyObject * _Exc_UnboundLocalError() { return ::PyExc_UnboundLocalError; }
PyObject * _Exc_UnicodeError() { return ::PyExc_UnicodeError; }
#endif


//
//    wrap items in Object.h
//
PyObject * _None() { return &::_Py_NoneStruct; }

PyObject * _False() { return Py_False; }
PyObject * _True() { return Py_True; }

PyTypeObject * _Buffer_Type() { return &PyBuffer_Type; }
PyTypeObject * _CFunction_Type() { return &PyCFunction_Type; }
PyTypeObject * _Class_Type() { return &PyClass_Type; }
PyTypeObject * _CObject_Type() { return &PyCObject_Type; }
PyTypeObject * _Complex_Type() { return &PyComplex_Type; }
PyTypeObject * _Dict_Type() { return &PyDict_Type; }
PyTypeObject * _File_Type() { return &PyFile_Type; }
PyTypeObject * _Float_Type() { return &PyFloat_Type; }
PyTypeObject * _Function_Type() { return &PyFunction_Type; }
PyTypeObject * _Instance_Type() { return &PyInstance_Type; }
PyTypeObject * _Bool_Type()    { return &PyBool_Type; }
PyTypeObject * _Int_Type() { return &PyInt_Type; }
PyTypeObject * _List_Type() { return &PyList_Type; }
PyTypeObject * _Long_Type() { return &PyLong_Type; }
PyTypeObject * _Method_Type() { return &PyMethod_Type; }
PyTypeObject * _Module_Type() { return &PyModule_Type; }
PyTypeObject * _Range_Type() { return &PyRange_Type; }
PyTypeObject * _Slice_Type() { return &PySlice_Type; }
PyTypeObject * _String_Type() { return &PyString_Type; }
PyTypeObject * _TraceBack_Type() { return &PyTraceBack_Type; }
PyTypeObject * _Tuple_Type() { return &PyTuple_Type; }
PyTypeObject * _Type_Type() { return &PyType_Type; }

#if PY_MAJOR_VERSION >= 2
PyTypeObject * _Unicode_Type() { return &PyUnicode_Type; }
#endif

//
//    wrap flags
//
int &_Py_DebugFlag()    { return Py_DebugFlag; }
int &_Py_InteractiveFlag(){ return Py_InteractiveFlag; }
int &_Py_OptimizeFlag()    { return Py_OptimizeFlag; }
int &_Py_NoSiteFlag()    { return Py_NoSiteFlag; }
int &_Py_TabcheckFlag()    { return Py_TabcheckFlag; }
int &_Py_VerboseFlag()    { return Py_VerboseFlag; }
#if PY_MAJOR_VERSION >= 2
int &_Py_UnicodeFlag()    { return Py_UnicodeFlag; }
#endif
char *__Py_PackageContext(){ return _Py_PackageContext; }

//
//    Needed to keep the abstactions for delayload interface
//
void _XINCREF( PyObject *op )
{
    Py_XINCREF(op);
}

void _XDECREF( PyObject *op )
{
    Py_XDECREF(op);
}

#endif
}
