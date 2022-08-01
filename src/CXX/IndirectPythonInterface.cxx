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
static int _IsInstance( PyObject *op, PyTypeObject *type )
{
    return PyObject_IsInstance( op, reinterpret_cast<PyObject *>( type ) );
}

PYCXX_EXPORT bool _CFunction_Check( PyObject *op )   { return _IsInstance( op, _CFunction_Type() ) > 0; }
PYCXX_EXPORT bool _Complex_Check( PyObject *op )     { return _IsInstance( op, _Complex_Type() ) > 0; }
PYCXX_EXPORT bool _Dict_Check( PyObject *op )        { return _IsInstance( op, _Dict_Type() ) > 0; }
PYCXX_EXPORT bool _Float_Check( PyObject *op )       { return _IsInstance( op, _Float_Type() ) > 0; }
#if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT bool _Function_Check( PyObject *op )    { return _IsInstance( op, _Function_Type() ) > 0; }
#endif
PYCXX_EXPORT bool _Boolean_Check( PyObject *op )     { return _IsInstance( op, _Bool_Type() ) > 0; }
PYCXX_EXPORT bool _List_Check( PyObject *op )        { return _IsInstance( op, _List_Type() ) > 0; }
PYCXX_EXPORT bool _Long_Check( PyObject *op )        { return _IsInstance( op, _Long_Type() ) > 0; }
#if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT bool _Method_Check( PyObject *op )      { return _IsInstance( op, _Method_Type() ) > 0; }
#endif
PYCXX_EXPORT bool _Module_Check( PyObject *op )      { return _IsInstance( op, _Module_Type() ) > 0; }
PYCXX_EXPORT bool _Range_Check( PyObject *op )       { return _IsInstance( op, _Range_Type() ) > 0; }
PYCXX_EXPORT bool _Slice_Check( PyObject *op )       { return _IsInstance( op, _Slice_Type() ) > 0; }
PYCXX_EXPORT bool _TraceBack_Check( PyObject *op )   { return _IsInstance( op, _TraceBack_Type() ) > 0; }
PYCXX_EXPORT bool _Tuple_Check( PyObject *op )       { return _IsInstance( op, _Tuple_Type() ) > 0; }
PYCXX_EXPORT bool _Type_Check( PyObject *op )        { return _IsInstance( op, _Type_Type() ) > 0; }
PYCXX_EXPORT bool _Unicode_Check( PyObject *op )     { return _IsInstance( op, _Unicode_Type() ) > 0; }
#if PY_MAJOR_VERSION == 2
PYCXX_EXPORT bool _String_Check( PyObject *op )      { return _IsInstance( op, _String_Type() ) > 0; }
PYCXX_EXPORT bool _Int_Check( PyObject *op )         { return _IsInstance( op, _Int_Type() ) > 0; }
PYCXX_EXPORT bool _CObject_Check( PyObject *op )     { return _IsInstance( op, _CObject_Type() ) > 0; }
#endif
#if PY_MAJOR_VERSION >= 3
PYCXX_EXPORT bool _Bytes_Check( PyObject *op )       { return _IsInstance( op, _Bytes_Type() ) > 0; }
#endif

#if defined(PY_WIN32_DELAYLOAD_PYTHON_DLL)

# if defined(MS_WINDOWS)
#  include <windows.h>


static HMODULE python_dll;

#  define PYCXX_STANDARD_EXCEPTION( eclass, bclass ) \
    static PyObject *ptr_Exc_##eclass = NULL;

#  if PY_MAJOR_VERSION == 2
#   include "CXX/Python2/cxx_standard_exceptions.hxx"
#  else
#   include "CXX/Python3/cxx_standard_exceptions.hxx"
#  endif

#  undef PYCXX_STANDARD_EXCEPTION

static PyTypeObject *ptr__CFunction_Type = NULL;
static PyTypeObject *ptr__Complex_Type = NULL;
static PyTypeObject *ptr__Dict_Type = NULL;
static PyTypeObject *ptr__Float_Type = NULL;
#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
static PyTypeObject *ptr__Function_Type = NULL;
#  endif
static PyTypeObject *ptr__Bool_Type = NULL;
static PyTypeObject *ptr__List_Type = NULL;
static PyTypeObject *ptr__Long_Type = NULL;
#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
static PyTypeObject *ptr__Method_Type = NULL;
#  endif
static PyTypeObject *ptr__Module_Type = NULL;
static PyTypeObject *ptr__Range_Type = NULL;
static PyTypeObject *ptr__Slice_Type = NULL;
static PyTypeObject *ptr__TraceBack_Type = NULL;
static PyTypeObject *ptr__Tuple_Type = NULL;
static PyTypeObject *ptr__Type_Type = NULL;
static PyTypeObject *ptr__Unicode_Type = NULL;
#  if PY_MAJOR_VERSION == 2
static PyTypeObject *ptr__Int_Type = NULL;
static PyTypeObject *ptr__String_Type = NULL;
static PyTypeObject *ptr__CObject_Type = NULL;
#  endif
#  if PY_MAJOR_VERSION >= 3
static PyTypeObject *ptr__Bytes_Type = NULL;
#  endif

#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
static int *ptr_Py_DebugFlag = NULL;
static int *ptr_Py_InteractiveFlag = NULL;
static int *ptr_Py_OptimizeFlag = NULL;
static int *ptr_Py_NoSiteFlag = NULL;
static int *ptr_Py_VerboseFlag = NULL;

#   if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 7
static const char **ptr__Py_PackageContext = NULL;
#   else
static char **ptr__Py_PackageContext = NULL;
#   endif
#  endif

#  ifdef Py_REF_DEBUG
int *ptr_Py_RefTotal;
#  endif


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

#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 7
static char **GetConstCharPointer_as_ConstCharPointerPointer( const char *name )
{
    FARPROC addr = GetProcAddress( python_dll, name );
    if( addr == NULL )
        throw GetAddressException( name );

    return (const char **)addr;
}
#endif


#  ifdef _DEBUG
static const char python_dll_name_format[] = "PYTHON%1.1d%1.1d_D.DLL";
#  else
static const char python_dll_name_format[] = "PYTHON%1.1d%1.1d.DLL";
#  endif

//--------------------------------------------------------------------------------
bool InitialisePythonIndirectInterface()
{
    char python_dll_name[sizeof(python_dll_name_format)];

    _snprintf( python_dll_name, sizeof(python_dll_name_format) / sizeof(char) - 1, python_dll_name_format, PY_MAJOR_VERSION, PY_MINOR_VERSION );

    python_dll = LoadLibraryA( python_dll_name );
    if( python_dll == NULL )
        return false;

    try
    {
#  ifdef Py_REF_DEBUG
    ptr_Py_RefTotal             = GetInt_as_IntPointer( "_Py_RefTotal" );
#  endif
#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
    ptr_Py_DebugFlag            = GetInt_as_IntPointer( "Py_DebugFlag" );
    ptr_Py_InteractiveFlag      = GetInt_as_IntPointer( "Py_InteractiveFlag" );
    ptr_Py_OptimizeFlag         = GetInt_as_IntPointer( "Py_OptimizeFlag" );
    ptr_Py_NoSiteFlag           = GetInt_as_IntPointer( "Py_NoSiteFlag" );
    ptr_Py_VerboseFlag          = GetInt_as_IntPointer( "Py_VerboseFlag" );

#    if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 7
    ptr__Py_PackageContext      = GetConstCharPointer_as_ConstCharPointerPointer( "_Py_PackageContext" );
#    else
    ptr__Py_PackageContext      = GetCharPointer_as_CharPointerPointer( "_Py_PackageContext" );
#    endif
#  endif

#  define PYCXX_STANDARD_EXCEPTION( eclass, bclass )
    ptr_Exc_#eclass = GetPyTypeObject_As_PyTypeObjectPointer( "PyExc_" #eclass );

#  if PY_MAJOR_VERSION == 2
#   include "CXX/Python2/cxx_standard_exceptions.hxx"
#  else
#   include "CXX/Python3/cxx_standard_exceptions.hxx"
#  endif

#  undef PYCXX_STANDARD_EXCEPTION

    ptr__PyNone                 = GetPyObject_As_PyObjectPointer( "_Py_NoneStruct" );

#  if PY_MAJOR_VERSION == 2
    ptr__PyFalse                = GetPyObject_As_PyObjectPointer( "_Py_ZeroStruct" );
#  else
    ptr__PyFalse                = GetPyObject_As_PyObjectPointer( "_Py_FalseStruct" );
#  endif
    ptr__PyTrue                 = GetPyObject_As_PyObjectPointer( "_Py_TrueStruct" );

    ptr__CFunction_Type         = GetPyTypeObject_As_PyTypeObjectPointer( "PyCFunction_Type" );
    ptr__Complex_Type           = GetPyTypeObject_As_PyTypeObjectPointer( "PyComplex_Type" );
    ptr__Dict_Type              = GetPyTypeObject_As_PyTypeObjectPointer( "PyDict_Type" );
    ptr__Float_Type             = GetPyTypeObject_As_PyTypeObjectPointer( "PyFloat_Type" );
#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
    ptr__Function_Type          = GetPyTypeObject_As_PyTypeObjectPointer( "PyFunction_Type" );
#  endif
    ptr__Bool_Type              = GetPyTypeObject_As_PyTypeObjectPointer( "PyBool_Type" );
    ptr__List_Type              = GetPyTypeObject_As_PyTypeObjectPointer( "PyList_Type" );
    ptr__Long_Type              = GetPyTypeObject_As_PyTypeObjectPointer( "PyLong_Type" );
#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
    ptr__Method_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyMethod_Type" );
#  endif
    ptr__Module_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyModule_Type" );
    ptr__Range_Type             = GetPyTypeObject_As_PyTypeObjectPointer( "PyRange_Type" );
    ptr__Slice_Type             = GetPyTypeObject_As_PyTypeObjectPointer( "PySlice_Type" );
    ptr__TraceBack_Type         = GetPyTypeObject_As_PyTypeObjectPointer( "PyTraceBack_Type" );
    ptr__Tuple_Type             = GetPyTypeObject_As_PyTypeObjectPointer( "PyTuple_Type" );
    ptr__Type_Type              = GetPyTypeObject_As_PyTypeObjectPointer( "PyType_Type" );
    ptr__Unicode_Type           = GetPyTypeObject_As_PyTypeObjectPointer( "PyUnicode_Type" );
#  if PY_MAJOR_VERSION == 2
    ptr__String_Type            = GetPyTypeObject_As_PyTypeObjectPointer( "PyString_Type" );
    ptr__Int_Type               = GetPyTypeObject_As_PyTypeObjectPointer( "PyInt_Type" );
    ptr__CObject_Type           = GetPyTypeObject_As_PyTypeObjectPointer( "PyCObject_Type" );
#  endif
#  if PY_MAJOR_VERSION >= 3
    ptr__Bytes_Type             = GetPyTypeObject_As_PyTypeObjectPointer( "PyBytes_Type" );
#  endif
    }
    catch( GetAddressException &e )
    {
        OutputDebugStringA( python_dll_name );
        OutputDebugStringA( " does not contain symbol " );
        OutputDebugStringA( e.name );
        OutputDebugStringA( "\n" );

        return false;
    }

    return true;
}

//
//    Wrap variables as function calls
//
PYCXX_EXPORT PyObject *_Exc_ArithmeticError()        { return ptr__Exc_ArithmeticError; }
PYCXX_EXPORT PyObject *_Exc_AssertionError()         { return ptr__Exc_AssertionError; }
PYCXX_EXPORT PyObject *_Exc_AttributeError()         { return ptr__Exc_AttributeError; }
PYCXX_EXPORT PyObject *_Exc_EnvironmentError()       { return ptr__Exc_EnvironmentError; }
PYCXX_EXPORT PyObject *_Exc_EOFError()               { return ptr__Exc_EOFError; }
PYCXX_EXPORT PyObject *_Exc_Exception()              { return ptr__Exc_Exception; }
PYCXX_EXPORT PyObject *_Exc_FloatingPointError()     { return ptr__Exc_FloatingPointError; }
PYCXX_EXPORT PyObject *_Exc_ImportError()            { return ptr__Exc_ImportError; }
PYCXX_EXPORT PyObject *_Exc_IndexError()             { return ptr__Exc_IndexError; }
PYCXX_EXPORT PyObject *_Exc_IOError()                { return ptr__Exc_IOError; }
PYCXX_EXPORT PyObject *_Exc_KeyboardInterrupt()      { return ptr__Exc_KeyboardInterrupt; }
PYCXX_EXPORT PyObject *_Exc_KeyError()               { return ptr__Exc_KeyError; }
PYCXX_EXPORT PyObject *_Exc_LookupError()            { return ptr__Exc_LookupError; }
PYCXX_EXPORT PyObject *_Exc_MemoryError()            { return ptr__Exc_MemoryError; }
PYCXX_EXPORT PyObject *_Exc_NameError()              { return ptr__Exc_NameError; }
PYCXX_EXPORT PyObject *_Exc_NotImplementedError()    { return ptr__Exc_NotImplementedError; }
PYCXX_EXPORT PyObject *_Exc_OSError()                { return ptr__Exc_OSError; }
PYCXX_EXPORT PyObject *_Exc_OverflowError()          { return ptr__Exc_OverflowError; }
PYCXX_EXPORT PyObject *_Exc_RuntimeError()           { return ptr__Exc_RuntimeError; }
#  if PY_MAJOR_VERSION == 2
PYCXX_EXPORT PyObject *_Exc_StandardError()          { return ptr__Exc_StandardError; }
#  endif
PYCXX_EXPORT PyObject *_Exc_SyntaxError()            { return ptr__Exc_SyntaxError; }
PYCXX_EXPORT PyObject *_Exc_SystemError()            { return ptr__Exc_SystemError; }
PYCXX_EXPORT PyObject *_Exc_SystemExit()             { return ptr__Exc_SystemExit; }
PYCXX_EXPORT PyObject *_Exc_TypeError()              { return ptr__Exc_TypeError; }
PYCXX_EXPORT PyObject *_Exc_ValueError()             { return ptr__Exc_ValueError; }
#  ifdef MS_WINDOWS
PYCXX_EXPORT PyObject *_Exc_WindowsError()           { return ptr__Exc_WindowsError; }
#  endif
PYCXX_EXPORT PyObject *_Exc_ZeroDivisionError()      { return ptr__Exc_ZeroDivisionError; }
PYCXX_EXPORT PyObject *_Exc_IndentationError()       { return ptr__Exc_IndentationError; }
PYCXX_EXPORT PyObject *_Exc_TabError()               { return ptr__Exc_TabError; }
PYCXX_EXPORT PyObject *_Exc_UnboundLocalError()      { return ptr__Exc_UnboundLocalError; }
PYCXX_EXPORT PyObject *_Exc_UnicodeError()           { return ptr__Exc_UnicodeError; }

//
//    wrap items in Object.h
//
PYCXX_EXPORT PyObject *_None()                       { return ptr__PyNone; }

PYCXX_EXPORT PyObject *_False()                      { return ptr__PyFalse; }
PYCXX_EXPORT PyObject *_True()                       { return ptr__PyTrue; }

PYCXX_EXPORT PyTypeObject *_CFunction_Type()         { return ptr__CFunction_Type; }
PYCXX_EXPORT PyTypeObject *_Complex_Type()           { return ptr__Complex_Type; }
PYCXX_EXPORT PyTypeObject *_Dict_Type()              { return ptr__Dict_Type; }
PYCXX_EXPORT PyTypeObject *_Float_Type()             { return ptr__Float_Type; }
#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT PyTypeObject *_Function_Type()          { return ptr__Function_Type; }
#  endif
PYCXX_EXPORT PyTypeObject *_Bool_Type()              { return ptr__Bool_Type; }
PYCXX_EXPORT PyTypeObject *_List_Type()              { return ptr__List_Type; }
PYCXX_EXPORT PyTypeObject *_Long_Type()              { return ptr__Long_Type; }
#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT PyTypeObject *_Method_Type()            { return ptr__Method_Type; }
#  endif
PYCXX_EXPORT PyTypeObject *_Module_Type()            { return ptr__Module_Type; }
PYCXX_EXPORT PyTypeObject *_Range_Type()             { return ptr__Range_Type; }
PYCXX_EXPORT PyTypeObject *_Slice_Type()             { return ptr__Slice_Type; }
PYCXX_EXPORT PyTypeObject *_TraceBack_Type()         { return ptr__TraceBack_Type; }
PYCXX_EXPORT PyTypeObject *_Tuple_Type()             { return ptr__Tuple_Type; }
PYCXX_EXPORT PyTypeObject *_Type_Type()              { return ptr__Type_Type; }
PYCXX_EXPORT PyTypeObject *_Unicode_Type()           { return ptr__Unicode_Type; }
#  if PY_MAJOR_VERSION == 2
PYCXX_EXPORT PyTypeObject *_String_Type()            { return ptr__String_Type; }
PYCXX_EXPORT PyTypeObject *_Int_Type()               { return ptr__Int_Type; }
PYCXX_EXPORT PyTypeObject *_CObject_Type()           { return ptr__CObject_Type; }
#  endif
#  if PY_MAJOR_VERSION >= 3
PYCXX_EXPORT PyTypeObject *_Bytes_Type()             { return ptr__Bytes_Type; }
#  endif

//
//    wrap the Python Flag variables
//
#  if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT int &_Py_DebugFlag()                    { return *ptr_Py_DebugFlag; }
PYCXX_EXPORT int &_Py_InteractiveFlag()              { return *ptr_Py_InteractiveFlag; }
PYCXX_EXPORT int &_Py_OptimizeFlag()                 { return *ptr_Py_OptimizeFlag; }
PYCXX_EXPORT int &_Py_NoSiteFlag()                   { return *ptr_Py_NoSiteFlag; }
PYCXX_EXPORT int &_Py_VerboseFlag()                  { return *ptr_Py_VerboseFlag; }
#  endif

#  if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 7
PYCXX_EXPORT const char *__Py_PackageContext()       { return *ptr__Py_PackageContext; }
#  else
PYCXX_EXPORT char *__Py_PackageContext()             { return *ptr__Py_PackageContext; }
#  endif

#  if 0
#   define Py_INCREF(op) (                         \
    _Py_INC_REFTOTAL  _Py_REF_DEBUG_COMMA       \
    ((PyObject*)(op))->ob_refcnt++)

#   define Py_DECREF(op)                           \
    if (_Py_DEC_REFTOTAL  _Py_REF_DEBUG_COMMA   \
        --((PyObject*)(op))->ob_refcnt != 0)    \
        _Py_CHECK_REFCNT(op)                    \
    else                                        \
        _Py_Dealloc((PyObject *)(op))
#  endif

void _XINCREF( PyObject *op )
{
    // This function must match the contents of Py_XINCREF(op)
    if( op == NULL )
        return;

#  ifdef Py_REF_DEBUG
    (*ptr_Py_RefTotal)++;
#  endif
    (op)->ob_refcnt++;

}

void _XDECREF( PyObject *op )
{
    // This function must match the contents of Py_XDECREF(op);
    if( op == NULL )
        return;

#  ifdef Py_REF_DEBUG
    (*ptr_Py_RefTotal)--;
#  endif

    if (--(op)->ob_refcnt == 0)
        _Py_Dealloc((PyObject *)(op));
}


# else
#  error "Can only delay load under Win32"
# endif

#else

//================================================================================
//
//    Map onto Macros
//
//================================================================================

//
//    Wrap variables as function calls
//
# define PYCXX_STANDARD_EXCEPTION( eclass, bclass ) \
    PYCXX_EXPORT PyObject *_Exc_##eclass() { return ::PyExc_##eclass; }

# if PY_MAJOR_VERSION == 2
#  include "CXX/Python2/cxx_standard_exceptions.hxx"
# else
#  include "CXX/Python3/cxx_standard_exceptions.hxx"
# endif

# undef PYCXX_STANDARD_EXCEPTION

//
//    wrap items in Object.h
//
PYCXX_EXPORT PyObject *_None()                       { return &::_Py_NoneStruct; }

PYCXX_EXPORT PyObject *_False()                      { return Py_False; }
PYCXX_EXPORT PyObject *_True()                       { return Py_True; }

PYCXX_EXPORT PyTypeObject *_CFunction_Type()         { return &PyCFunction_Type; }
PYCXX_EXPORT PyTypeObject *_Complex_Type()           { return &PyComplex_Type; }
PYCXX_EXPORT PyTypeObject *_Dict_Type()              { return &PyDict_Type; }
PYCXX_EXPORT PyTypeObject *_Float_Type()             { return &PyFloat_Type; }
# if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT PyTypeObject *_Function_Type()          { return &PyFunction_Type; }
# endif
PYCXX_EXPORT PyTypeObject *_Bool_Type()              { return &PyBool_Type; }
PYCXX_EXPORT PyTypeObject *_List_Type()              { return &PyList_Type; }
PYCXX_EXPORT PyTypeObject *_Long_Type()              { return &PyLong_Type; }
# if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT PyTypeObject *_Method_Type()            { return &PyMethod_Type; }
# endif
PYCXX_EXPORT PyTypeObject *_Module_Type()            { return &PyModule_Type; }
PYCXX_EXPORT PyTypeObject *_Range_Type()             { return &PyRange_Type; }
PYCXX_EXPORT PyTypeObject *_Slice_Type()             { return &PySlice_Type; }
PYCXX_EXPORT PyTypeObject *_TraceBack_Type()         { return &PyTraceBack_Type; }
PYCXX_EXPORT PyTypeObject *_Tuple_Type()             { return &PyTuple_Type; }
PYCXX_EXPORT PyTypeObject *_Type_Type()              { return &PyType_Type; }
PYCXX_EXPORT PyTypeObject *_Unicode_Type()           { return &PyUnicode_Type; }
# if PY_MAJOR_VERSION == 2
PYCXX_EXPORT PyTypeObject *_String_Type()            { return &PyString_Type; }
PYCXX_EXPORT PyTypeObject *_Int_Type()               { return &PyInt_Type; }
PYCXX_EXPORT PyTypeObject *_CObject_Type()           { return &PyCObject_Type; }
# endif
# if PY_MAJOR_VERSION >= 3
PYCXX_EXPORT PyTypeObject *_Bytes_Type()             { return &PyBytes_Type; }
# endif

//
//    wrap flags
//
# if PY_MAJOR_VERSION == 2 || !defined( Py_LIMITED_API )
PYCXX_EXPORT int &_Py_DebugFlag()                    { return Py_DebugFlag; }
PYCXX_EXPORT int &_Py_InteractiveFlag()              { return Py_InteractiveFlag; }
PYCXX_EXPORT int &_Py_OptimizeFlag()                 { return Py_OptimizeFlag; }
PYCXX_EXPORT int &_Py_NoSiteFlag()                   { return Py_NoSiteFlag; }
PYCXX_EXPORT int &_Py_VerboseFlag()                  { return Py_VerboseFlag; }
#  if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 7
PYCXX_EXPORT const char *__Py_PackageContext()       { return _Py_PackageContext; }
#  else
PYCXX_EXPORT char *__Py_PackageContext()             { return _Py_PackageContext; }
#  endif
# endif

//
//    Needed to keep the abstractions for delayload interface
//
void _XINCREF( PyObject *op )
{
    Py_XINCREF( op );
}

void _XDECREF( PyObject *op )
{
    Py_XDECREF( op );
}

#endif
}
