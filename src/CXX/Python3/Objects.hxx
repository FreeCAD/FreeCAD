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

#ifndef __CXX_Objects__h
#define __CXX_Objects__h

#include "CXX/WrapPython.h"
#include "CXX/Version.hxx"
#include "CXX/Python3/Config.hxx"
#include "CXX/Python3/CxxDebug.hxx"
#include "CXX/Python3/Exception.hxx"

#include <iostream>
#include STR_STREAM
#include <string>
#include <iterator>
#include <utility>
#include <typeinfo>
#include <algorithm>

namespace Py
{
    typedef size_t sequence_index_type;    // type of an index into a sequence

    // Forward declarations
    class Object;
    class Type;
    template<TEMPLATE_TYPENAME T> class SeqBase;
    class Bytes;
    class String;
    class List;
    template<TEMPLATE_TYPENAME T> class MapBase;
    class Tuple;
    class Dict;

    //===========================================================================//
    // class Object
    // The purpose of this class is to serve as the most general kind of
    // Python object, for the purpose of writing C++ extensions in Python
    // Objects hold a PyObject* which they own. This pointer is always a
    // valid pointer to a Python object. In children we must maintain this behavior.
    //
    // Instructions on how to make your own class MyType descended from Object:
    // (0) Pick a base class, either Object or perhaps SeqBase<T> or MapBase<T>.
    //     This example assumes Object.

    // (1) Write a routine int MyType_Check( PyObject * ) modeled after PyInt_Check,
    //     PyFloat_Check, etc.

    // (2) Add method accepts:
    //     virtual bool accepts( PyObject *pyob ) const {
    //         return pyob && MyType_Check( pyob );
    // }

    // (3) Include the following constructor and copy constructor
    //
    /*
    explicit MyType( PyObject *pyob ): Object( pyob )
    {
        validate();
    }

    MyType( const Object &other ): Object( other.ptr() )
    {
        validate();
    }
    */

    // Alernate version for the constructor to allow for construction from owned pointers:
    /*
    explicit MyType( PyObject *pyob ): Object( pyob )
    {
        validate();
    }
    */

    // You may wish to add other constructors; see the classes below for examples.
    // Each constructor must use "set" to set the pointer
    // and end by validating the pointer you have created.

    //( 4 ) Each class needs at least these two assignment operators:
    /*
    MyType &operator=( const Object &rhs )
    {
        return *this = *rhs;
    }

    Mytype &operator=( PyObject *rhsp )
    {
        if( ptr() != rhsp )
            set( rhsp );
        return *this;
    }
    */
    // Note on accepts: constructors call the base class
    // version of a virtual when calling the base class constructor,
    // so the test has to be done explicitly in a descendent.

    // If you are inheriting from PythonExtension<T> to define an object
    // note that it contains PythonExtension<T>::check
    // which you can use in accepts when writing a wrapper class.
    // See Demo/range.h and Demo/range.cxx for an example.

    class PYCXX_EXPORT Object
    {
    private:
        // the pointer to the Python object
        // Only Object sets this directly.
        // The default constructor for Object sets it to Py_None and
        // child classes must use "set" to set it
        //
        PyObject *p;

    protected:
        void set( PyObject *pyob, bool owned = false )
        {
            release();
            p = pyob;
            if( !owned )
            {
                Py::_XINCREF( p );
            }
            validate();
        }

        void release()
        {
            Py::_XDECREF( p );
            p = NULL;
        }

        void validate();

    public:
        // Constructor acquires new ownership of pointer unless explicitly told not to.
        explicit Object( PyObject *pyob=Py::_None(), bool owned = false )
        : p( pyob )
        {
            if( !owned )
            {
                Py::_XINCREF( p );
            }
            validate();
        }

        // Copy constructor acquires new ownership of pointer
        Object( const Object &ob )
        : p( ob.p )
        {
            Py::_XINCREF( p );
            validate();
        }

        // Assignment acquires new ownership of pointer
        Object &operator=( const Object &rhs )
        {
            set( rhs.p );
            return *this;
        }

        Object &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );

            return *this;
        }

        // Destructor
        virtual ~Object()
        {
            release();
        }

        // Loaning the pointer to others, retain ownership
        PyObject *operator*() const
        {
            return p;
        }

        // Explicit reference_counting changes
        void increment_reference_count()
        {
            Py::_XINCREF( p );
        }

        void decrement_reference_count()
        {
            // not allowed to commit suicide, however
            if( reference_count() == 1 )
            {
                throw RuntimeError( "Object::decrement_reference_count error." );
            }
            Py::_XDECREF( p );
        }

        // Would like to call this pointer() but messes up STL in SeqBase<T>
        PyObject *ptr() const
        {
            return p;
        }

        //
        // Queries
        //

        // Can pyob be used in this object's constructor?
        virtual bool accepts( PyObject * /* pyob */ ) const
        {
            // allow any object or NULL
            return true;
        }

        Py_ssize_t reference_count() const
        { // the reference count
            return p ? p->ob_refcnt : 0;
        }

        Type type() const; // the type object associated with this one

        String str() const; // the str() representation

        std::string as_string() const;

        String repr() const; // the repr() representation

        List dir() const; // the dir() list

        bool hasAttr( const std::string &s ) const
        {
            return PyObject_HasAttrString( p, const_cast<char*>( s.c_str() ) ) ? true: false;
        }

        Object getAttr( const std::string &s ) const
        {
            return Object( PyObject_GetAttrString( p, const_cast<char*>( s.c_str() ) ), true );
        }

        Object callMemberFunction( const std::string &function_name ) const;
        Object callMemberFunction( const std::string &function_name, const Tuple &args ) const;
        Object callMemberFunction( const std::string &function_name, const Tuple &args, const Dict &kw ) const;

        Object getItem( const Object &key ) const
        {
            return Object( PyObject_GetItem( p, *key ), true );
        }

        Py_hash_t hashValue() const
        {
            return PyObject_Hash( p );
        }

        // convert to bool
        bool as_bool() const
        {
            return PyObject_IsTrue( ptr() ) != 0;
        }

        //operator bool() const
        //{
        //    return as_bool();
        //}

        // int print( FILE *fp, int flags=Py_Print_RAW )
        //{
        //    return PyObject_Print( p, fp, flags );
        //}

        bool is( PyObject *pother ) const
        {  // identity test
            return p == pother;
        }

        bool is( const Object &other ) const
        { // identity test
            return p == other.p;
        }

        bool isNull() const
        {
            return p == NULL;
        }

        bool isNone() const
        {
            return p == _None();
        }

        bool isCallable() const
        {
            return PyCallable_Check( p ) != 0;
        }

        bool isDict() const
        {
            return Py::_Dict_Check( p );
        }

        bool isList() const
        {
            return Py::_List_Check( p );
        }

        bool isMapping() const
        {
            return PyMapping_Check( p ) != 0;
        }

        bool isNumeric() const
        {
            return PyNumber_Check( p ) != 0;
        }

        bool isSequence() const
        {
            return PySequence_Check( p ) != 0;
        }

        bool isTrue() const
        {
            return PyObject_IsTrue( p ) != 0;
        }

        bool isType( const Type &t ) const;

        bool isTuple() const
        {
            return Py::_Tuple_Check( p );
        }

        bool isString() const
        {
            return Py::_Unicode_Check( p );
        }

        bool isBytes() const
        {
            return Py::_Bytes_Check( p );
        }

        bool isBoolean() const
        {
            return Py::_Boolean_Check( p );
        }

        // Commands
        void setAttr( const std::string &s, const Object &value )
        {
            if( PyObject_SetAttrString( p, const_cast<char*>( s.c_str() ), *value ) == -1 )
            {
                throw AttributeError( "setAttr failed." );
            }
        }

        void delAttr( const std::string &s )
        {
            if( PyObject_DelAttrString( p, const_cast<char*>( s.c_str() ) ) == -1 )
            {
                throw AttributeError( "delAttr failed." );
            }
        }

        // PyObject_SetItem is too weird to be using from C++
        // so it is intentionally omitted.

        void delItem( const Object &key )
        {
            if( PyObject_DelItem( p, *key ) == -1 )
            {
                // failed to link on Windows?
                throw KeyError( "delItem failed." );
            }
        }
        // Equality and comparison use PyObject_Compare

    };
    // End of class Object

    // Null can be return from when it is require to return NULL to Python from a method
    class PYCXX_EXPORT Null: public Object
    {
    public:
        Null()
        : Object( NULL )
        {
        }
        virtual ~Null()
        {
        }

        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob == NULL;
        }
    };

    //------------------------------------------------------------
    PYCXX_EXPORT bool operator==( const Object &o1, const Object &o2 );
    PYCXX_EXPORT bool operator!=( const Object &o1, const Object &o2 );
    PYCXX_EXPORT bool operator>=( const Object &o1, const Object &o2 );
    PYCXX_EXPORT bool operator<=( const Object &o1, const Object &o2 );
    PYCXX_EXPORT bool operator<( const Object &o1, const Object &o2 );
    PYCXX_EXPORT bool operator>( const Object &o1, const Object &o2 );

    //------------------------------------------------------------


    //
    //    Convert an owned Python pointer into a PyCXX Object
    //
    inline Object asObject( PyObject *p )
    {
        return Object( p, true );
    }

    // new_reference_to also overloaded below on Object
    inline PyObject *new_reference_to( PyObject *p )
    {
        Py::_XINCREF( p );
        return p;
    }

    inline PyObject *new_reference_to( const Object &g )
    {
        PyObject *p = g.ptr();
        Py::_XINCREF( p );
        return p;
    }

    // Python special None value
    inline Object None()
    {
        return Object( Py::_None() );
    }

    // Python special Boolean values
    inline Object False()
    {
        return Object( Py::_False() );
    }

    inline Object True()
    {
        return Object( Py::_True() );
    }

    // TMM: 31May'01 - Added the #ifndef so I can exlude iostreams.
#ifndef CXX_NO_IOSTREAMS
    PYCXX_EXPORT std::ostream &operator<<( std::ostream &os, const Object &ob );
#endif

    // Class Type
    class PYCXX_EXPORT Type: public Object
    {
    public:
        explicit Type( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        Type( const Object &ob )
        : Object( *ob )
        {
            validate();
        }

        Type( const Type &t )
        : Object( t )
        {
            validate();
        }

        Type &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Type &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && Py::_Type_Check( pyob );
        }
    };

    // ===============================================
    // class boolean
    class PYCXX_EXPORT Boolean: public Object
    {
    public:
        // Constructor
        Boolean( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        Boolean( const Boolean &ob )
        : Object( *ob )
        {
            validate();
        }

        // create from bool
        Boolean( bool v=false )
        {
            set( PyBool_FromLong( v ? 1 : 0 ), true );
            validate();
        }

        explicit Boolean( const Object &ob )
        : Object( *ob )
        {
            validate();
        }

        // Assignment acquires new ownership of pointer
        Boolean &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Boolean &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            // accepts any object that can be converted to a boolean
            return pyob && PyObject_IsTrue( pyob ) != -1;
        }

        Boolean &operator=( bool v )
        {
            set( PyBool_FromLong( v ? 1 : 0 ), true );
            return *this;
        }

        operator bool() const
        {
            return as_bool();
        }
    };

    // ===============================================
    // class Long
    class PYCXX_EXPORT Long: public Object
    {
    public:
        // Constructor
        explicit Long( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        Long( const Long &ob )
        : Object( ob.ptr() )
        {
            validate();
        }

        // try to create from any object
        explicit Long( const Object &ob )
        : Object( PyNumber_Long( *ob ), true )
        {
            validate();
        }

        // create from long
        explicit Long( long v = 0L )
        : Object( PyLong_FromLong( v ), true )
        {
            validate();
        }

        // create from unsigned long
        explicit Long( unsigned long v )
        : Object( PyLong_FromUnsignedLong( v ), true )
        {
            validate();
        }

        // create from int
        explicit Long( int v )
        : Object( PyLong_FromLong( static_cast<long>( v ) ), true )
        {
            validate();
        }

#ifdef HAVE_LONG_LONG
        // create from long long
        explicit Long( PY_LONG_LONG v )
        : Object( PyLong_FromLongLong( v ), true )
        {
            validate();
        }

        // create from unsigned long long
        explicit Long( unsigned PY_LONG_LONG v )
        : Object( PyLong_FromUnsignedLongLong( v ), true )
        {
            validate();
        }
#endif

        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && Py::_Long_Check( pyob );
        }

        // Assignment acquires new ownership of pointer
        Long &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Long &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( PyNumber_Long( rhsp ), true );
            return *this;
        }

        // assign from an int
        Long &operator=( int v )
        {
            set( PyLong_FromLong( long( v ) ), true );
            return *this;
        }

        // assign from long
        Long &operator=( long v )
        {
            set( PyLong_FromLong( v ), true );
            return *this;
        }

        // assign from unsigned long
        Long &operator=( unsigned long v )
        {
            set( PyLong_FromUnsignedLong( v ), true );
            return *this;
        }

#ifdef HAVE_LONG_LONG
        Long &operator=( PY_LONG_LONG v )
        {
            set( PyLong_FromLongLong( v ), true );
            return *this;
        }

        Long &operator=( unsigned PY_LONG_LONG v )
        {
            set( PyLong_FromUnsignedLongLong( v ), true );
            return *this;
        }
#endif

        //operator bool() const
        //{
        //    return as_bool();
        //}

        // convert to long
        long as_long() const
        {
            return PyLong_AsLong( ptr() );
        }

        operator long() const
        {
            return as_long();
        }

        operator int() const
        {
            return static_cast<int>( as_long() );
        }

        // convert to unsigned
        long as_unsigned_long() const
        {
            return PyLong_AsUnsignedLong( ptr() );
        }

        operator unsigned long() const
        {
            return as_unsigned_long();
        }

        double as_double() const
        {
            return PyLong_AsDouble( ptr() );
        }

        operator double() const
        {
            return as_double();
        }

#ifdef HAVE_LONG_LONG
        PY_LONG_LONG as_long_long() const
        {
            return PyLong_AsLongLong( ptr() );
        }

        operator PY_LONG_LONG() const
        {
           return as_long_long();
        }

        unsigned PY_LONG_LONG as_unsigned_long_long() const
        {
            return PyLong_AsUnsignedLongLong( ptr() );
        }

        operator unsigned PY_LONG_LONG() const
        {
            return as_unsigned_long_long();
        }
#endif

        // prefix ++
        Long operator++()
        {
            set( PyNumber_Add( ptr(), *Long( 1 ) ) );
            return *this;
        }

        // postfix ++
        Long operator++( int )
        {
            Long a = *this;
            set( PyNumber_Add( ptr(), *Long( 1 ) ) );
            return a;
        }

        // prefix --
        Long operator--()
        {
            set( PyNumber_Subtract( ptr(), *Long( 1 ) ) );
            return *this;
        }

        // postfix --
        Long operator--( int )
        {
            Long a = *this;
            set( PyNumber_Subtract( ptr(), *Long( 1 ) ) );
            return a;
        }
    };

#ifdef PYCXX_PYTHON_2TO3
    // PyCXX for Python2 had an Int and LongLong classes
    typedef Long Int;
#ifdef HAVE_LONG_LONG
    typedef Long LongLong;
#endif
#endif

#if 1
    //------------------------------------------------------------
    // compare operators
    bool operator!=( const Long &a, const Long &b );
    bool operator!=( const Long &a, int b );
    bool operator!=( const Long &a, long b );
    bool operator!=( int a, const Long &b );
    bool operator!=( long a, const Long &b );
    //------------------------------
    bool operator==( const Long &a, const Long &b );
    bool operator==( const Long &a, int b );
    bool operator==( const Long &a, long b );
    bool operator==( int a, const Long &b );
    bool operator==( long a, const Long &b );
    //------------------------------
    bool operator>( const Long &a, const Long &b );
    bool operator>( const Long &a, int b );
    bool operator>( const Long &a, long b );
    bool operator>( int a, const Long &b );
    bool operator>( long a, const Long &b );
    //------------------------------
    bool operator>=( const Long &a, const Long &b );
    bool operator>=( const Long &a, int b );
    bool operator>=( const Long &a, long b );
    bool operator>=( int a, const Long &b );
    bool operator>=( long a, const Long &b );
    //------------------------------
    bool operator<( const Long &a, const Long &b );
    bool operator<( const Long &a, int b );
    bool operator<( const Long &a, long b );
    bool operator<( int a, const Long &b );
    bool operator<( long a, const Long &b );
    //------------------------------
    bool operator<=( const Long &a, const Long &b );
    bool operator<=( int a, const Long &b );
    bool operator<=( long a, const Long &b );
    bool operator<=( const Long &a, int b );
    bool operator<=( const Long &a, long b );

#ifdef HAVE_LONG_LONG
    //------------------------------
    bool operator!=( const Long &a, PY_LONG_LONG b );
    bool operator!=( PY_LONG_LONG a, const Long &b );
    //------------------------------
    bool operator==( const Long &a, PY_LONG_LONG b );
    bool operator==( PY_LONG_LONG a, const Long &b );
    //------------------------------
    bool operator>( const Long &a, PY_LONG_LONG b );
    bool operator>( PY_LONG_LONG a, const Long &b );
    //------------------------------
    bool operator>=( const Long &a, PY_LONG_LONG b );
    bool operator>=( PY_LONG_LONG a, const Long &b );
    //------------------------------
    bool operator<( const Long &a, PY_LONG_LONG b );
    bool operator<( PY_LONG_LONG a, const Long &b );
    //------------------------------
    bool operator<=( const Long &a, PY_LONG_LONG b );
    bool operator<=( PY_LONG_LONG a, const Long &b );
#endif
#endif

    // ===============================================
    // class Float
    //
    class PYCXX_EXPORT Float: public Object
    {
    public:
        // Constructor
        explicit Float( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        Float( const Float &f )
        : Object( f )
        {
            validate();
        }

        // make from double
        explicit Float( double v=0.0 )
        : Object( PyFloat_FromDouble( v ), true )
        {
            validate();
        }

        // try to make from any object
        Float( const Object &ob )
        : Object( PyNumber_Float( *ob ), true )
        {
            validate();
        }

        Float &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Float &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( PyNumber_Float( rhsp ), true );
            return *this;
        }

        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && Py::_Float_Check( pyob );
        }

        double as_double() const
        {
            return PyFloat_AsDouble( ptr() );
        }

        // convert to double
        operator double() const
        {
            return as_double();
        }

        // assign from a double
        Float &operator=( double v )
        {
            set( PyFloat_FromDouble( v ), true );
            return *this;
        }
        // assign from an int
        Float &operator=( int v )
        {
            set( PyFloat_FromDouble( double( v ) ), true );
            return *this;
        }
        // assign from long
        Float &operator=( long v )
        {
            set( PyFloat_FromDouble( double( v ) ), true );
            return *this;
        }
        // assign from an Long
        Float &operator=( const Long &iob )
        {
            set( PyFloat_FromDouble( double( iob.as_long() ) ), true );
            return *this;
        }
    };

    //------------------------------------------------------------
    // compare operators
    bool operator!=( const Float &a, const Float &b );
    bool operator!=( const Float &a, double b );
    bool operator!=( double a, const Float &b );
    //------------------------------
    bool operator==( const Float &a, const Float &b );
    bool operator==( const Float &a, double b );
    bool operator==( double a, const Float &b );
    //------------------------------
    bool operator>( const Float &a, const Float &b );
    bool operator>( const Float &a, double b );
    bool operator>( double a, const Float &b );
    //------------------------------
    bool operator>=( const Float &a, const Float &b );
    bool operator>=( const Float &a, double b );
    bool operator>=( double a, const Float &b );
    //------------------------------
    bool operator<( const Float &a, const Float &b );
    bool operator<( const Float &a, double b );
    bool operator<( double a, const Float &b );
    //------------------------------
    bool operator<=( const Float &a, const Float &b );
    bool operator<=( double a, const Float &b );
    bool operator<=( const Float &a, double b );

    // ===============================================
    // class Complex
    class PYCXX_EXPORT Complex: public Object
    {
    public:
        // Constructor
        explicit Complex( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        Complex( const Complex &f )
        : Object( f )
        {
            validate();
        }

        // make from double
        explicit Complex( double v=0.0, double w=0.0 )
        :Object( PyComplex_FromDoubles( v, w ), true )
        {
            validate();
        }

        Complex &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Complex &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }
        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && Py::_Complex_Check( pyob );
        }
        // convert to Py_complex
        operator Py_complex() const
        {
            return PyComplex_AsCComplex( ptr() );
        }
        // assign from a Py_complex
        Complex &operator=( const Py_complex &v )
        {
            set( PyComplex_FromCComplex( v ), true );
            return *this;
        }
        // assign from a double
        Complex &operator=( double v )
        {
            set( PyComplex_FromDoubles( v, 0.0 ), true );
            return *this;
        }
        // assign from an int
        Complex &operator=( int v )
        {
            set( PyComplex_FromDoubles( double( v ), 0.0 ), true );
            return *this;
        }
        // assign from long
        Complex &operator=( long v )
        {
            set( PyComplex_FromDoubles( double( v ), 0.0 ), true );
            return *this;
        }
        // assign from an Long
        Complex &operator=( const Long &iob )
        {
            set( PyComplex_FromDoubles( double( iob.as_long() ), 0.0 ), true );
            return *this;
        }

        double real() const
        {
            return PyComplex_RealAsDouble( ptr() );
        }

        double imag() const
        {
            return PyComplex_ImagAsDouble( ptr() );
        }
    };
    // Sequences
    // Sequences are here represented as sequences of items of type T.
    // The base class SeqBase<T> represents that.
    // In basic Python T is always "Object".

    // seqref<T> is what you get if you get elements from a non-const SeqBase<T>.
    // Note: seqref<T> could probably be a nested class in SeqBase<T> but that might stress
    // some compilers needlessly. Simlarly for mapref later.

    // While this class is not intended for enduser use, it needs some public
    // constructors for the benefit of the STL.

    // See Scott Meyer's More Essential C++ for a description of proxies.
    // This application is even more complicated. We are doing an unusual thing
    // in having a double proxy. If we want the STL to work
    // properly we have to compromise by storing the rvalue inside. The
    // entire Object API is repeated so that things like s[i].isList() will
    // work properly.

    // Still, once in a while a weird compiler message may occur using expressions like x[i]
    // Changing them to Object( x[i] ) helps the compiler to understand that the
    // conversion of a seqref to an Object is wanted.

    template<TEMPLATE_TYPENAME T>
    class seqref
    {
    protected:
        SeqBase<T> &s; // the sequence
        size_t offset; // item number
        T the_item; // lvalue

    public:
        seqref( SeqBase<T> &seq, sequence_index_type j )
        : s( seq )
        , offset( j )
        , the_item( s.getItem( j ) )
        {}

        seqref( const seqref<T> &range )
        : s( range.s )
        , offset( range.offset )
        , the_item( range.the_item )
        {}

        // TMM: added this seqref ctor for use with STL algorithms
        seqref( Object &obj )
        : s( dynamic_cast< SeqBase<T>&>( obj ) )
        , offset( 0 )
        , the_item( s.getItem( offset ) )
        {}

        ~seqref()
        {}

        operator T() const
        { // rvalue
            return the_item;
        }

        seqref<T> &operator=( const seqref<T> &rhs )
        { //used as lvalue
            the_item = rhs.the_item;
            s.setItem( offset, the_item );
            return *this;
        }

        seqref<T> &operator=( const T &ob )
        { // used as lvalue
            the_item = ob;
            s.setItem( offset, ob );
            return *this;
        }

        // forward everything else to the item
        PyObject *ptr() const
        {
            return the_item.ptr();
        }

        int reference_count() const
        { // the reference count
            return the_item.reference_count();
        }

        Type type() const
        {
            return the_item.type();
        }

        String str() const;
        String repr() const;

        bool hasAttr( const std::string &attr_name ) const
        {
            return the_item.hasAttr( attr_name );
        }

        Object getAttr( const std::string &attr_name ) const
        {
            return the_item.getAttr( attr_name );
        }

        Object getItem( const Object &key ) const
        {
            return the_item.getItem( key );
        }

        long hashValue() const
        {
            return the_item.hashValue();
        }

        bool isCallable() const
        {
            return the_item.isCallable();
        }

        bool isInstance() const
        {
            return the_item.isInstance();
        }

        bool isDict() const
        {
            return the_item.isDict();
        }

        bool isList() const
        {
            return the_item.isList();
        }

        bool isMapping() const
        {
            return the_item.isMapping();
        }

        bool isNumeric() const
        {
            return the_item.isNumeric();
        }

        bool isSequence() const
        {
            return the_item.isSequence();
        }

        bool isTrue() const
        {
            return the_item.isTrue();
        }

        bool isType( const Type &t ) const
        {
            return the_item.isType( t );
        }

        bool isTuple() const
        {
            return the_item.isTuple();
        }

        bool isString() const
        {
            return the_item.isString();
        }
        // Commands
        void setAttr( const std::string &attr_name, const Object &value )
        {
            the_item.setAttr( attr_name, value );
        }

        void delAttr( const std::string &attr_name )
        {
            the_item.delAttr( attr_name );
        }

        void delItem( const Object &key )
        {
            the_item.delItem( key );
        }

        bool operator==( const Object &o2 ) const
        {
            return the_item == o2;
        }

        bool operator!=( const Object &o2 ) const
        {
            return the_item != o2;
        }

        bool operator>=( const Object &o2 ) const
        {
            return the_item >= o2;
        }

        bool operator<=( const Object &o2 ) const
        {
            return the_item <= o2;
        }

        bool operator<( const Object &o2 ) const
        {
            return the_item < o2;
        }

        bool operator>( const Object &o2 ) const
        {
            return the_item > o2;
        }
    }; // end of seqref


    // class SeqBase<T>
    // ...the base class for all sequence types

    template<TEMPLATE_TYPENAME T>
    class SeqBase: public Object
    {
    public:
        // STL definitions
        typedef size_t size_type;
        typedef seqref<T> reference;
        typedef T const_reference;
        typedef seqref<T> *pointer;
        typedef int difference_type;
        typedef T value_type;        // TMM: 26Jun'01

        virtual size_type max_size() const
        {
            return std::string::npos; // ?
        }

        virtual size_type capacity() const
        {
            return size();
        }

        virtual void swap( SeqBase<T> &c )
        {
            SeqBase<T> temp = c;
            c = ptr();
            set( temp.ptr() );
        }

        virtual size_type size() const
        {
            return PySequence_Length( ptr() );
        }

        explicit SeqBase<T>()
        :Object( PyTuple_New( 0 ), true )
        {
            validate();
        }

        explicit SeqBase<T>( PyObject *pyob, bool owned=false )
        : Object( pyob, owned )
        {
            validate();
        }

        SeqBase<T>( const Object &ob )
        : Object( ob )
        {
            validate();
        }

        // Assignment acquires new ownership of pointer
        SeqBase<T> &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        SeqBase<T> &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && PySequence_Check( pyob );
        }

        size_type length() const
        {
            return PySequence_Length( ptr() );
        }

        // Element access
        const T operator[]( sequence_index_type index ) const
        {
            return getItem( index );
        }

        seqref<T> operator[]( sequence_index_type index )
        {
            return seqref<T>( *this, index );
        }

        virtual T getItem( sequence_index_type i ) const
        {
            return T( asObject( PySequence_GetItem( ptr(), i ) ) );
        }

        virtual void setItem( sequence_index_type i, const T &ob )
        {
            if( PySequence_SetItem( ptr(), i, *ob ) == -1 )
            {
                throw Exception();
            }
        }

        SeqBase<T> repeat( int count ) const
        {
            return SeqBase<T>( PySequence_Repeat( ptr(), count ), true );
        }

        SeqBase<T> concat( const SeqBase<T> &other ) const
        {
            return SeqBase<T>( PySequence_Concat( ptr(), *other ), true );
        }

        // more STL compatability
        const T front() const
        {
            return getItem( 0 );
        }

        seqref<T> front()
        {
            return seqref<T>( *this, 0 );
        }

        const T back() const
        {
            return getItem( size()-1 );
        }

        seqref<T> back()
        {
            return seqref<T>( *this, size()-1 );
        }

        void verify_length( size_type required_size ) const
        {
            if( size() != required_size )
            {
                throw IndexError( "Unexpected SeqBase<T> length." );
            }
        }

        void verify_length( size_type min_size, size_type max_size ) const
        {
            size_type n = size();
            if( n < min_size || n > max_size )
            {
                throw IndexError( "Unexpected SeqBase<T> length." );
            }
        }

        class PYCXX_EXPORT iterator
        : public random_access_iterator_parent( seqref<T> )
        {
        protected:
            friend class SeqBase<T>;
            SeqBase<T> *seq;
            size_type count;

        public:
            ~iterator()
            {}

            iterator()
            : seq( 0 )
            , count( 0 )
            {}

            iterator( SeqBase<T> *s, size_type where )
            : seq( s )
            , count( where )
            {}

            iterator( const iterator &other )
            : seq( other.seq )
            , count( other.count )
            {}

            bool eql( const iterator &other ) const
            {
                return seq->ptr() == other.seq->ptr() && count == other.count;
            }

            bool neq( const iterator &other ) const
            {
                return seq->ptr() != other.seq->ptr() || count != other.count;
            }

            bool lss( const iterator &other ) const
            {
                return count < other.count;
            }

            bool gtr( const iterator &other ) const
            {
                return count > other.count;
            }

            bool leq( const iterator &other ) const
            {
                return count <= other.count;
            }

            bool geq( const iterator &other ) const
            {
                return count >= other.count;
            }

            seqref<T> operator*()
            {
                return seqref<T>( *seq, count );
            }

            seqref<T> operator[]( sequence_index_type i )
            {
                return seqref<T>( *seq, count + i );
            }

            iterator &operator=( const iterator &other )
            {
                if( this != &other )
                {
                    seq = other.seq;
                    count = other.count;
                }
                return *this;
            }

            iterator operator+( int n ) const
            {
                return iterator( seq, count + n );
            }

            iterator operator-( int n ) const
            {
                return iterator( seq, count - n );
            }

            iterator &operator+=( int n )
            {
                count = count + n;
                return *this;
            }

            iterator &operator-=( int n )
            {
                count = count - n;
                return *this;
            }

            int operator-( const iterator &other ) const
            {
                if( seq->ptr() != other.seq->ptr() )
                {
                    throw RuntimeError( "SeqBase<T>::iterator comparison error" );
                }
                return count - other.count;
            }

            // prefix ++
            iterator &operator++()
            {
                count++;
                return *this;
            }

            // postfix ++
            iterator operator++( int )
            {
                return iterator( seq, count++ );
            }

            // prefix --
            iterator &operator--()
            {
                count--;
                return *this;
            }

            // postfix --
            iterator operator--( int )
            {
                return iterator( seq, count-- );
            }

            std::string diagnose() const
            {
                std::OSTRSTREAM oss;
                oss << "iterator diagnosis " << seq << ", " << count << std::ends;
                return std::string( oss.str() );
            }

        };    // end of class SeqBase<T>::iterator

        iterator begin()
        {
            return iterator( this, 0 );
        }

        iterator end()
        {
            return iterator( this, length() );
        }

        class PYCXX_EXPORT const_iterator
        : public random_access_iterator_parent( const Object )
        {
        protected:
            friend class SeqBase<T>;
            const SeqBase<T> *seq;
            sequence_index_type count;

        public:
            ~const_iterator()
            {}

            const_iterator()
            : seq( 0 )
            , count( 0 )
            {}

            const_iterator( const SeqBase<T> *s, size_type where )
            : seq( s )
            , count( where )
            {}

            const_iterator( const const_iterator &other )
            : seq( other.seq )
            , count( other.count )
            {}

            const T operator*() const
            {
                return seq->getItem( count );
            }

            const T operator[]( sequence_index_type i ) const
            {
                return seq->getItem( count + i );
            }

            const_iterator &operator=( const const_iterator &other )
            {
                if( this != &other )
                {
                    seq = other.seq;
                    count = other.count;
                }
                return *this;
            }

            const_iterator operator+( int n ) const
            {
                return const_iterator( seq, count + n );
            }

            bool eql( const const_iterator &other ) const
            {
                return seq->ptr() == other.seq->ptr() && count == other.count;
            }

            bool neq( const const_iterator &other ) const
            {
                return seq->ptr() != other.seq->ptr() || count != other.count;
            }

            bool lss( const const_iterator &other ) const
            {
                return count < other.count;
            }

            bool gtr( const const_iterator &other ) const
            {
                return count > other.count;
            }

            bool leq( const const_iterator &other ) const
            {
                return count <= other.count;
            }

            bool geq( const const_iterator &other ) const
            {
                return count >= other.count;
            }

            const_iterator operator-( int n )
            {
                return const_iterator( seq, count - n );
            }

            const_iterator &operator+=( int n )
            {
                count = count + n;
                return *this;
            }

            const_iterator &operator-=( int n )
            {
                count = count - n;
                return *this;
            }

            int operator-( const const_iterator &other ) const
            {
                if( *seq != *other.seq )
                {
                    throw RuntimeError( "SeqBase<T>::const_iterator::- error" );
                }
                return count - other.count;
            }

            // prefix ++
            const_iterator &operator++()
            {
                count++;
                return *this;
            }

            // postfix ++
            const_iterator operator++( int )
            {
                return const_iterator( seq, count++ );
            }

            // prefix --
            const_iterator &operator--()
            {
                count--;
                return *this;
            }

            // postfix --
            const_iterator operator--( int )
            {
                return const_iterator( seq, count-- );
            }

        };    // end of class SeqBase<T>::const_iterator

        const_iterator begin() const
        {
            return const_iterator( this, 0 );
        }

        const_iterator end() const
        {
            return const_iterator( this, length() );
        }
    };

    // Here's an important typedef you might miss if reading too fast...
    typedef SeqBase<Object> Sequence;

    template <TEMPLATE_TYPENAME T> bool operator==( const EXPLICIT_TYPENAME SeqBase<T>::iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator!=( const EXPLICIT_TYPENAME SeqBase<T>::iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator< ( const EXPLICIT_TYPENAME SeqBase<T>::iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator> ( const EXPLICIT_TYPENAME SeqBase<T>::iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator<=( const EXPLICIT_TYPENAME SeqBase<T>::iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator>=( const EXPLICIT_TYPENAME SeqBase<T>::iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::iterator &right );

    template <TEMPLATE_TYPENAME T> bool operator==( const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator!=( const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator< ( const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator> ( const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator<=( const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator>=( const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &left, const EXPLICIT_TYPENAME SeqBase<T>::const_iterator &right ); 


    PYCXX_EXPORT extern bool operator==( const Sequence::iterator &left, const Sequence::iterator &right );
    PYCXX_EXPORT extern bool operator!=( const Sequence::iterator &left, const Sequence::iterator &right );
    PYCXX_EXPORT extern bool operator< ( const Sequence::iterator &left, const Sequence::iterator &right );
    PYCXX_EXPORT extern bool operator> ( const Sequence::iterator &left, const Sequence::iterator &right );
    PYCXX_EXPORT extern bool operator<=( const Sequence::iterator &left, const Sequence::iterator &right );
    PYCXX_EXPORT extern bool operator>=( const Sequence::iterator &left, const Sequence::iterator &right );

    PYCXX_EXPORT extern bool operator==( const Sequence::const_iterator &left, const Sequence::const_iterator &right );
    PYCXX_EXPORT extern bool operator!=( const Sequence::const_iterator &left, const Sequence::const_iterator &right );
    PYCXX_EXPORT extern bool operator< ( const Sequence::const_iterator &left, const Sequence::const_iterator &right );
    PYCXX_EXPORT extern bool operator> ( const Sequence::const_iterator &left, const Sequence::const_iterator &right );
    PYCXX_EXPORT extern bool operator<=( const Sequence::const_iterator &left, const Sequence::const_iterator &right );
    PYCXX_EXPORT extern bool operator>=( const Sequence::const_iterator &left, const Sequence::const_iterator &right ); 

    // ==================================================
    // class Char
    // Python strings return strings as individual elements.
    // I'll try having a class Char which is a String of length 1
    //
    typedef std::basic_string<Py_UNICODE> unicodestring;
    extern Py_UNICODE unicode_null_string[1];

    class PYCXX_EXPORT Byte: public Object
    {
    public:
        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob != NULL
                && Py::_Unicode_Check( pyob )
                && PySequence_Length( pyob ) == 1;
        }

        explicit Byte( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        Byte( const Object &ob )
        : Object( ob )
        {
            validate();
        }

        Byte( const std::string &v = "" )
        : Object( PyBytes_FromStringAndSize( const_cast<char*>( v.c_str() ), 1 ), true )
        {
            validate();
        }

        Byte( char v )
        : Object( PyBytes_FromStringAndSize( &v, 1 ), true )
        {
            validate();
        }

        // Assignment acquires new ownership of pointer
        Byte &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Byte &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        // Assignment from C string
        Byte &operator=( const std::string &v )
        {
            set( PyBytes_FromStringAndSize( const_cast<char*>( v.c_str() ),1 ), true );
            return *this;
        }

        Byte &operator=( char v )
        {
            set( PyUnicode_FromStringAndSize( &v, 1 ), true );
            return *this;
        }

        // Conversion
        operator Bytes() const;
    };

    class PYCXX_EXPORT Bytes: public SeqBase<Byte>
    {
    public:
        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob != NULL && Py::_Bytes_Check( pyob );
        }

        virtual size_type capacity() const
        {
            return max_size();
        }

        explicit Bytes( PyObject *pyob, bool owned = false )
        : SeqBase<Byte>( pyob, owned )
        {
            validate();
        }

        Bytes( const Object &ob )
        : SeqBase<Byte>( ob )
        {
            validate();
        }

        Bytes()
        : SeqBase<Byte>( PyBytes_FromStringAndSize( "", 0 ), true )
        {
            validate();
        }

        Bytes( const std::string &v )
        : SeqBase<Byte>( PyBytes_FromStringAndSize( const_cast<char*>( v.data() ), static_cast<int>( v.length() ) ), true )
        {
            validate();
        }

        Bytes( const std::string &v, Py_ssize_t vsize )
        : SeqBase<Byte>( PyBytes_FromStringAndSize( const_cast<char*>( v.data() ), static_cast<int>( vsize ) ), true )
        {
            validate();
        }

        Bytes( const char *v )
        : SeqBase<Byte>( PyBytes_FromString( v ), true )
        {
            validate();
        }

        Bytes( const char *v, Py_ssize_t vsize )
        : SeqBase<Byte>( PyBytes_FromStringAndSize( const_cast<char*>( v ), vsize ), true )
        {
            validate();
        }

        // Assignment acquires new ownership of pointer
        Bytes &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Bytes &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        // Assignment from C string
        Bytes &operator=( const std::string &v )
        {
            set( PyBytes_FromStringAndSize( const_cast<char*>( v.data() ), static_cast<int>( v.length() ) ), true );
            return *this;
        }

        String decode( const char *encoding, const char *error="strict" );

        // Queries
        virtual size_type size() const
        {
            return static_cast<size_type>( PyBytes_Size( ptr() ) );
        }

        operator std::string() const
        {
            return as_std_string();
        }

        std::string as_std_string() const
        {
            return std::string( PyBytes_AsString( ptr() ), static_cast<size_type>( PyBytes_Size( ptr() ) ) );
        }
    };

    class PYCXX_EXPORT Char: public Object
    {
    public:
        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob != 0 &&( Py::_Unicode_Check( pyob ) ) && PySequence_Length( pyob ) == 1;
        }

        explicit Char( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        Char( const Object &ob )
        : Object( ob )
        {
            validate();
        }

        Char( int v )
        : Object( PyUnicode_FromOrdinal( v ), true )
        {
            validate();
        }

        Char( Py_UNICODE v )
        : Object( PyUnicode_FromOrdinal( v ), true )
        {
            validate();
        }

        Char( const unicodestring &v )
        : Object( PyUnicode_FromUnicode( const_cast<Py_UNICODE*>( v.data() ),1 ), true )
        {
            validate();
        }

        // Assignment acquires new ownership of pointer
        Char &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Char &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        Char &operator=( const unicodestring &v )
        {
            set( PyUnicode_FromUnicode( const_cast<Py_UNICODE*>( v.data() ), 1 ), true );
            return *this;
        }

        Char &operator=( int v_ )
        {
            Py_UNICODE v( v_ );
            set( PyUnicode_FromUnicode( &v, 1 ), true );
            return *this;
        }

        Char &operator=( Py_UNICODE v )
        {
            set( PyUnicode_FromUnicode( &v, 1 ), true );
            return *this;
        }

        // Conversion
        operator String() const;
    };

    class PYCXX_EXPORT String: public SeqBase<Char>
    {
    public:
        virtual size_type capacity() const
        {
            return max_size();
        }

        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob != NULL && Py::_Unicode_Check( pyob );
        }

        explicit String( PyObject *pyob, bool owned = false )
        : SeqBase<Char>( pyob, owned )
        {
            validate();
        }

        String( const Object &ob )
        : SeqBase<Char>( ob )
        {
            validate();
        }

        String()
        : SeqBase<Char>( PyUnicode_FromString( "" ), true )
        {
            validate();
        }

        String( const char *latin1 )
        : SeqBase<Char>( PyUnicode_FromString( latin1 ), true )
        {
            validate();
        }

        String( const std::string &latin1 )
        : SeqBase<Char>( PyUnicode_FromStringAndSize( latin1.c_str(), latin1.size() ), true )
        {
            validate();
        }

        String( const char *latin1, Py_ssize_t size )
        : SeqBase<Char>( PyUnicode_FromStringAndSize( latin1, size ), true )
        {
            validate();
        }

        /* [Taken from Pythons's unicode.h]

           Many of these APIs take two arguments encoding and errors. These
           parameters encoding and errors have the same semantics as the ones
           of the builtin unicode() API. 

           Setting encoding to NULL causes the default encoding to be used.

           Error handling is set by errors which may also be set to NULL
           meaning to use the default handling defined for the codec. Default
           error handling for all builtin codecs is "strict" (ValueErrors are
           raised).

           The codecs all use a similar interface. Only deviation from the
           generic ones are documented.

        */
        String( const std::string &s, const char *encoding, const char *errors=NULL )
        : SeqBase<Char>( PyUnicode_Decode( s.c_str(), s.size(), encoding, errors ), true )
        {
            validate();
        }

        String( const char *s, const char *encoding, const char *errors=NULL )
        : SeqBase<Char>( PyUnicode_Decode( s, strlen(s), encoding, errors ), true )
        {
            validate();
        }

        String( const char *s, Py_ssize_t size, const char *encoding, const char *errors=NULL )
        : SeqBase<Char>( PyUnicode_Decode( s, size, encoding, errors ), true )
        {
            validate();
        }

        String( const Py_UNICODE *s, int length )
        : SeqBase<Char>( PyUnicode_FromUnicode( s, length ), true )
        {
            validate();
        }

        // Assignment acquires new ownership of pointer
        String &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        String &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        String &operator=( const unicodestring &v )
        {
            set( PyUnicode_FromUnicode( const_cast<Py_UNICODE *>( v.data() ), static_cast<int>( v.length() ) ), true );
            return *this;
        }

        // Encode
        Bytes encode( const char *encoding, const char *error="strict" ) const
        {
            return Bytes( PyUnicode_AsEncodedString( ptr(), encoding, error ), true );
        }

        // Queries
        virtual size_type size() const
        {
            return static_cast<size_type>( PyUnicode_GET_SIZE( ptr() ) );
        }

        unicodestring as_unicodestring() const
        {
            return unicodestring( PyUnicode_AS_UNICODE( ptr() ), static_cast<size_type>( PyUnicode_GET_SIZE( ptr() ) ) );
        }

        operator std::string() const
        {
            // use the default encoding
            return as_std_string( NULL );
        }

        std::string as_std_string( const char *encoding=NULL, const char *error="strict" ) const
        {
            Bytes b( encode( encoding, error ) );
            return b.as_std_string();
        }

        const Py_UNICODE *unicode_data() const
        {
            return PyUnicode_AS_UNICODE( ptr() );
        }
    };

    // ==================================================
    // class Tuple
    class PYCXX_EXPORT Tuple: public Sequence
    {
    public:
        virtual void setItem( sequence_index_type offset, const Object&ob )
        {
            // note PyTuple_SetItem is a thief...
            if( PyTuple_SetItem( ptr(), offset, new_reference_to( ob ) ) == -1 )
            {
                throw Exception();
            }
        }

        // Constructor
        explicit Tuple( PyObject *pyob, bool owned = false )
        : Sequence( pyob, owned )
        {
            validate();
        }

        Tuple( const Object &ob )
        : Sequence( ob )
        {
            validate();
        }

        // New tuple of a given size
        explicit Tuple( sequence_index_type size = 0 )
        {
            set( PyTuple_New( size ), true );
            validate();
            for( sequence_index_type i=0; i < size; i++ )
            {
                if( PyTuple_SetItem( ptr(), i, new_reference_to( Py::_None() ) ) == -1 )
                {
                    throw Exception();
                }
            }
        }
        // Tuple from any sequence
        explicit Tuple( const Sequence &s )
        {
            sequence_index_type limit( sequence_index_type( s.length() ) );

            set( PyTuple_New( limit ), true );
            validate();
            
            for( sequence_index_type i=0; i < limit; i++ )
            {
                if( PyTuple_SetItem( ptr(), i, new_reference_to( s[i] ) ) == -1 )
                {
                    throw Exception();
                }
            }
        }

        // Assignment acquires new ownership of pointer
        Tuple &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Tuple &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }
        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && Py::_Tuple_Check( pyob );
        }

        Tuple getSlice( int i, int j ) const
        {
            return Tuple( PySequence_GetSlice( ptr(), i, j ), true );
        }

    };

    class PYCXX_EXPORT TupleN: public Tuple
    {
    public:
        TupleN()
        : Tuple( (sequence_index_type)0 )
        {
        }

        TupleN( const Object &obj1 )
        : Tuple( 1 )
        {
            setItem( 0, obj1 );
        }

        TupleN( const Object &obj1, const Object &obj2 )
        : Tuple( 2 )
        {
            setItem( 0, obj1 );
            setItem( 1, obj2 );
        }

        TupleN( const Object &obj1, const Object &obj2, const Object &obj3 )
        : Tuple( 3 )
        {
            setItem( 0, obj1 );
            setItem( 1, obj2 );
            setItem( 2, obj3 );
        }

        TupleN( const Object &obj1, const Object &obj2, const Object &obj3,
                const Object &obj4 )
        : Tuple( 4 )
        {
            setItem( 0, obj1 );
            setItem( 1, obj2 );
            setItem( 2, obj3 );
            setItem( 3, obj4 );
        }

        TupleN( const Object &obj1, const Object &obj2, const Object &obj3,
                const Object &obj4, const Object &obj5 )
        : Tuple( 5 )
        {
            setItem( 0, obj1 );
            setItem( 1, obj2 );
            setItem( 2, obj3 );
            setItem( 3, obj4 );
            setItem( 4, obj5 );
        }

        TupleN( const Object &obj1, const Object &obj2, const Object &obj3,
                const Object &obj4, const Object &obj5, const Object &obj6 )
        : Tuple( 6 )
        {
            setItem( 0, obj1 );
            setItem( 1, obj2 );
            setItem( 2, obj3 );
            setItem( 3, obj4 );
            setItem( 4, obj5 );
            setItem( 5, obj6 );
        }

        TupleN( const Object &obj1, const Object &obj2, const Object &obj3,
                const Object &obj4, const Object &obj5, const Object &obj6,
                const Object &obj7 )
        : Tuple( 7 )
        {
            setItem( 0, obj1 );
            setItem( 1, obj2 );
            setItem( 2, obj3 );
            setItem( 3, obj4 );
            setItem( 4, obj5 );
            setItem( 5, obj6 );
            setItem( 6, obj7 );
        }

        TupleN( const Object &obj1, const Object &obj2, const Object &obj3,
                const Object &obj4, const Object &obj5, const Object &obj6,
                const Object &obj7, const Object &obj8 )
        : Tuple( 8 )
        {
            setItem( 0, obj1 );
            setItem( 1, obj2 );
            setItem( 2, obj3 );
            setItem( 3, obj4 );
            setItem( 4, obj5 );
            setItem( 5, obj6 );
            setItem( 6, obj7 );
            setItem( 7, obj8 );
        }

        TupleN( const Object &obj1, const Object &obj2, const Object &obj3,
                const Object &obj4, const Object &obj5, const Object &obj6,
                const Object &obj7, const Object &obj8, const Object &obj9 )
        : Tuple( 9 )
        {
            setItem( 0, obj1 );
            setItem( 1, obj2 );
            setItem( 2, obj3 );
            setItem( 3, obj4 );
            setItem( 4, obj5 );
            setItem( 5, obj6 );
            setItem( 6, obj7 );
            setItem( 7, obj8 );
            setItem( 8, obj9 );
        }

        virtual ~TupleN()
        { }
    };

    // ==================================================
    // class List

    class PYCXX_EXPORT List: public Sequence
    {
    public:
        // Constructor
        explicit List( PyObject *pyob, bool owned = false )
        : Sequence( pyob, owned )
        {
            validate();
        }
        List( const Object &ob )
        : Sequence( ob )
        {
            validate();
        }
        // Creation at a fixed size
        List( sequence_index_type size = 0 )
        {
            set( PyList_New( size ), true );
            validate();
            for( sequence_index_type i=0; i < size; i++ )
            {
                if( PyList_SetItem( ptr(), i, new_reference_to( Py::_None() ) ) == -1 )
                {
                    throw Exception();
                }
            }
        }

        // List from a sequence
        List( const Sequence &s )
        : Sequence()
        {
            sequence_index_type n =( int )s.length();
            set( PyList_New( n ), true );
            validate();
            for( sequence_index_type i=0; i < n; i++ )
            {
                if( PyList_SetItem( ptr(), i, new_reference_to( s[i] ) ) == -1 )
                {
                    throw Exception();
                }
            }
        }

        virtual size_type capacity() const
        {
            return max_size();
        }

        // Assignment acquires new ownership of pointer
        List &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        List &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && Py::_List_Check( pyob );
        }

        List getSlice( Py_ssize_t i, Py_ssize_t j ) const
        {
            return List( PyList_GetSlice( ptr(), i, j ), true );
        }

        void setSlice( Py_ssize_t i, Py_ssize_t j, const Object &v )
        {
            if( PyList_SetSlice( ptr(), i, j, *v ) == -1 )
            {
                throw Exception();
            }
        }

        void append( const Object &ob )
        {
            if( PyList_Append( ptr(), *ob ) == -1 )
            {
                throw Exception();
            }
        }

        void extend( const Object &ob )
        {
            setSlice( size(), size(), ob );
        }

        void insert( int i, const Object &ob )
        {
            if( PyList_Insert( ptr(), i, *ob ) == -1 )
            {
                throw Exception();
            }
        }

        void sort()
        {
            if( PyList_Sort( ptr() ) == -1 )
            {
                throw Exception();
            }
        }

        void reverse()
        {
            if( PyList_Reverse( ptr() ) == -1 )
            {
                throw Exception();
            }
        }
    };


    // Mappings
    // ==================================================
    template<TEMPLATE_TYPENAME T>
    class mapref
    {
    protected:
        MapBase<T> &s; // the map
        Object key; // item key
        T the_item;

    public:
        mapref<T>( MapBase<T> &map, const std::string &k )
        : s( map ), the_item()
        {
            key = String( k );
            if( map.hasKey( key ) ) the_item = map.getItem( key );
        }

        mapref<T>( MapBase<T> &map, const Object &k )
        : s( map ), key( k ), the_item()
        {
            if( map.hasKey( key ) ) the_item = map.getItem( key );
        }

        virtual ~mapref<T>()
        {}

        // MapBase<T> stuff
        // lvalue
        mapref<T> &operator=( const mapref<T> &other )
        {
            if( this != &other )
            {
                the_item = other.the_item;
                s.setItem( key, other.the_item );
            }
            return *this;
        }

        mapref<T> &operator=( const T &ob )
        {
            the_item = ob;
            s.setItem( key, ob );
            return *this;
        }

        // rvalue
        operator T() const
        {
            return the_item;
        }

        // forward everything else to the_item
        PyObject *ptr() const
        {
            return the_item.ptr();
        }

        int reference_count() const
        { // the mapref count
            return the_item.reference_count();
        }

        Type type() const
        {
            return the_item.type();
        }

        String str() const
        {
            return the_item.str();
        }

        String repr() const
        {
            return the_item.repr();
        }

        bool hasAttr( const std::string &attr_name ) const
        {
            return the_item.hasAttr( attr_name );
        }

        Object getAttr( const std::string &attr_name ) const
        {
            return the_item.getAttr( attr_name );
        }

        Object getItem( const Object &k ) const
        {
            return the_item.getItem( k );
        }

        long hashValue() const
        {
            return the_item.hashValue();
        }

        bool isCallable() const
        {
            return the_item.isCallable();
        }

        bool isInstance() const
        {
            return the_item.isInstance();
        }

        bool isList() const
        {
            return the_item.isList();
        }

        bool isMapping() const
        {
            return the_item.isMapping();
        }

        bool isNumeric() const
        {
            return the_item.isNumeric();
        }

        bool isSequence() const
        {
            return the_item.isSequence();
        }

        bool isTrue() const
        {
            return the_item.isTrue();
        }

        bool isType( const Type &t ) const
        {
            return the_item.isType( t );
        }

        bool isTuple() const
        {
            return the_item.isTuple();
        }

        bool isString() const
        {
            return the_item.isString();
        }

        // Commands
        void setAttr( const std::string &attr_name, const Object &value )
        {
            the_item.setAttr( attr_name, value );
        }

        void delAttr( const std::string &attr_name )
        {
            the_item.delAttr( attr_name );
        }

        void delItem( const Object &k )
        {
            the_item.delItem( k );
        }
    }; // end of mapref

#if 0
    // TMM: now for mapref<T>
    template< class T >
    bool operator==( const mapref<T> &left, const mapref<T> &right )
    {
        return true;    // NOT completed.
    }

    template< class T >
    bool operator!=( const mapref<T> &left, const mapref<T> &right )
    {
        return true;    // not completed.
    }
#endif

    template<TEMPLATE_TYPENAME T>
    class MapBase: public Object
    {
    protected:
        explicit MapBase<T>()
        {}
    public:
        // reference: proxy class for implementing []
        // TMM: 26Jun'01 - the types
        // If you assume that Python mapping is a hash_map...
        // hash_map::value_type is not assignable, but
        //( *it ).second = data must be a valid expression
        typedef size_t size_type;
        typedef Object key_type;
        typedef mapref<T> data_type;
        typedef std::pair< const T, T > value_type;
        typedef std::pair< const T, mapref<T> > reference;
        typedef const std::pair< const T, const T > const_reference;
        typedef std::pair< const T, mapref<T> > pointer;

        // Constructor
        explicit MapBase<T>( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        // TMM: 02Jul'01 - changed MapBase<T> to Object in next line
        MapBase<T>( const Object &ob )
        : Object( ob )
        {
            validate();
        }

        // Assignment acquires new ownership of pointer
        MapBase<T> &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        MapBase<T> &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }
        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && PyMapping_Check( pyob );
        }

        // Clear -- PyMapping Clear is missing
        //

        void clear()
        {
            List k = keys();
            for( List::iterator i = k.begin(); i != k.end(); i++ )
            {
                delItem( *i );
            }
        }

        virtual size_type size() const
        {
            return PyMapping_Length( ptr() );
        }

        // Element Access
        T operator[]( const std::string &key ) const
        {
            return getItem( key );
        }

        T operator[]( const Object &key ) const
        {
            return getItem( key );
        }

        mapref<T> operator[]( const char *key )
        {
            return mapref<T>( *this, key );
        }

        mapref<T> operator[]( const std::string &key )
        {
            return mapref<T>( *this, key );
        }

        mapref<T> operator[]( const Object &key )
        {
            return mapref<T>( *this, key );
        }

        size_type length() const
        {
            return PyMapping_Length( ptr() );
        }

        bool hasKey( const std::string &s ) const
        {
            return PyMapping_HasKeyString( ptr(),const_cast<char*>( s.c_str() ) ) != 0;
        }

        bool hasKey( const Object &s ) const
        {
            return PyMapping_HasKey( ptr(), s.ptr() ) != 0;
        }

        T getItem( const std::string &s ) const
        {
            return T( asObject( PyMapping_GetItemString( ptr(),const_cast<char*>( s.c_str() ) ) ) );
        }

        T getItem( const Object &s ) const
        {
            return T( asObject( PyObject_GetItem( ptr(), s.ptr() ) ) );
        }

        virtual void setItem( const char *s, const Object &ob )
        {
            if( PyMapping_SetItemString( ptr(), const_cast<char*>( s ), *ob ) == -1 )
            {
                throw Exception();
            }
        }

        virtual void setItem( const std::string &s, const Object &ob )
        {
            if( PyMapping_SetItemString( ptr(), const_cast<char*>( s.c_str() ), *ob ) == -1 )
            {
                throw Exception();
            }
        }

        virtual void setItem( const Object &s, const Object &ob )
        {
            if( PyObject_SetItem( ptr(), s.ptr(), ob.ptr() ) == -1 )
            {
                throw Exception();
            }
        }

        void delItem( const std::string &s )
        {
            if( PyMapping_DelItemString( ptr(), const_cast<char*>( s.c_str() ) ) == -1 )
            {
                throw Exception();
            }
        }

        void delItem( const Object &s )
        {
            if( PyMapping_DelItem( ptr(), *s ) == -1 )
            {
                throw Exception();
            }
        }

        // Queries
        List keys() const
        {
            return List( PyMapping_Keys( ptr() ), true );
        }

        List values() const
        {
            // each returned item is a (key, value) pair
            return List( PyMapping_Values( ptr() ), true );
        }

        List items() const
        {
            return List( PyMapping_Items( ptr() ), true );
        }

        class iterator
        {
            // : public forward_iterator_parent( std::pair<const T,T> ) {
        protected:
            typedef std::forward_iterator_tag iterator_category;
            typedef std::pair< const T, T > value_type;
            typedef int difference_type;
            typedef std::pair< const T, mapref<T> >    pointer;
            typedef std::pair< const T, mapref<T> >    reference;

            friend class MapBase<T>;
            //
            MapBase<T>      *map;
            List            keys;       // for iterating over the map
            size_type       pos;        // index into the keys

        public:
            ~iterator()
            {}

            iterator()
            : map( 0 )
            , keys()
            , pos( 0 )
            {}

            iterator( MapBase<T> *m, bool end = false )
            : map( m )
            , keys( m->keys() )
            , pos( end ? keys.length() : 0 )
            {}

            iterator( const iterator &other )
            : map( other.map )
            , keys( other.keys )
            , pos( other.pos )
            {}

            iterator( MapBase<T> *map_, List keys_, size_type pos_ )
            : map( map_ )
            , keys( keys_ )
            , pos( pos_ )
            {}

            reference operator*()
            {
                Object key = keys[ pos ];
                return std::make_pair( key, mapref<T>( *map, key ) );
            }

            iterator &operator=( const iterator &other )
            {
                if( this != &other )
                {
                    map = other.map;
                    keys = other.keys;
                    pos = other.pos;
                }
                return *this;
            }

            bool eql( const iterator &other ) const
            {
                return map->ptr() == other.map->ptr() && pos == other.pos;
            }

            bool neq( const iterator &other ) const
            {
                return map->ptr() != other.map->ptr() || pos != other.pos;
            }

            // pointer operator->() {
            //    return ;
            // }

            // prefix ++
            iterator &operator++()
            {
                pos++;
                return *this;
            }

            // postfix ++
            iterator operator++( int )
            {
                return iterator( map, keys, pos++ );
            }

            // prefix --
            iterator &operator--()
            {
                pos--;
                return *this;
            }

            // postfix --
            iterator operator--( int )
            { 
                return iterator( map, keys, pos-- );
            }

            std::string diagnose() const
            {
                std::OSTRSTREAM oss;
                oss << "iterator diagnosis " << map << ", " << pos << std::ends;
                return std::string( oss.str() );
            }
        };    // end of class MapBase<T>::iterator

        iterator begin()
        {
            return iterator( this, false );
        }

        iterator end()
        {
            return iterator( this, true );
        }

        class const_iterator
        {
        protected:
            typedef std::forward_iterator_tag iterator_category;
            typedef const std::pair< const T, T > value_type;
            typedef int difference_type;
            typedef const std::pair< const T, T > pointer;
            typedef const std::pair< const T, T > reference;

            friend class MapBase<T>;
            const MapBase<T>    *map;
            List                keys;   // for iterating over the map
            size_type           pos;    // index into the keys

        public:
            ~const_iterator()
            {}

            const_iterator()
            : map( 0 )
            , keys()
            , pos()
            {}

            const_iterator( const MapBase<T> *m, List k, size_type p )
            : map( m )
            , keys( k )
            , pos( p )
            {}

            const_iterator( const const_iterator &other )
            : map( other.map )
            , keys( other.keys )
            , pos( other.pos )
            {}

            bool eql( const const_iterator &other ) const
            {
                return map->ptr() == other.map->ptr() && pos == other.pos;
            }

            bool neq( const const_iterator &other ) const
            {
                return map->ptr() != other.map->ptr() || pos != other.pos;
            }


            //            const_reference    operator*() {
            //                Object key = *pos;
            //                return std::make_pair( key, map->[key] );
            // GCC < 3 barfes on this line at the '['.
            //         }

            const_reference operator*()
            {
                Object key = keys[ pos ];
                return std::make_pair( key, mapref<T>( *map, key ) );
            }

            const_iterator &operator=( const const_iterator &other )
            {
                if( this != &other )
                {
                    map = other.map;
                    keys = other.keys;
                    pos = other.pos;
                }
                return *this;
            }

            // prefix ++
            const_iterator &operator++()
            {
                pos++;
                return *this;
            }

            // postfix ++
            const_iterator operator++( int )
            {
                return const_iterator( map, keys, pos++ );
            }

            // prefix --
            const_iterator &operator--()
            {
                pos--;
                return *this;
            }

            // postfix --
            const_iterator operator--( int )
            {
                return const_iterator( map, keys, pos-- );
            }
        };    // end of class MapBase<T>::const_iterator

        const_iterator begin() const
        {
            return const_iterator( this, keys(), 0 );
        }

        const_iterator end() const
        {
            return const_iterator( this, keys(), length() );
        }

    };    // end of MapBase<T>

    typedef MapBase<Object> Mapping;

    template <TEMPLATE_TYPENAME T> bool operator==( const EXPLICIT_TYPENAME MapBase<T>::iterator &left, const EXPLICIT_TYPENAME MapBase<T>::iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator!=( const EXPLICIT_TYPENAME MapBase<T>::iterator &left, const EXPLICIT_TYPENAME MapBase<T>::iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator==( const EXPLICIT_TYPENAME MapBase<T>::const_iterator &left, const EXPLICIT_TYPENAME MapBase<T>::const_iterator &right );
    template <TEMPLATE_TYPENAME T> bool operator!=( const EXPLICIT_TYPENAME MapBase<T>::const_iterator &left, const EXPLICIT_TYPENAME MapBase<T>::const_iterator &right );

    PYCXX_EXPORT extern bool operator==( const Mapping::iterator &left, const Mapping::iterator &right );
    PYCXX_EXPORT extern bool operator!=( const Mapping::iterator &left, const Mapping::iterator &right );
    PYCXX_EXPORT extern bool operator==( const Mapping::const_iterator &left, const Mapping::const_iterator &right );
    PYCXX_EXPORT extern bool operator!=( const Mapping::const_iterator &left, const Mapping::const_iterator &right );


    // ==================================================
    // class Dict
    class PYCXX_EXPORT Dict: public Mapping
    {
    public:
        // Constructor
        explicit Dict( PyObject *pyob, bool owned=false )
        : Mapping( pyob, owned )
        {
            validate();
        }

        Dict( const Object &ob )
        : Mapping( ob )
        {
            validate();
        }

        // Creation
        Dict()
        {
            set( PyDict_New(), true );
            validate();
        }

        // Assignment acquires new ownership of pointer
        Dict &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Dict &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }
        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && Py::_Dict_Check( pyob );
        }
    };

    class PYCXX_EXPORT Callable: public Object
    {
    public:
        // Constructor
        explicit Callable()
        : Object()
        {}

        explicit Callable( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        Callable( const Object &ob )
        : Object( ob )
        {
            validate();
        }

        // Assignment acquires new ownership of pointer
        Callable &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Callable &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
            {
                set( rhsp );
            }
            return *this;
        }

        // Membership
        virtual bool accepts( PyObject *pyob ) const
        {
            return pyob && PyCallable_Check( pyob );
        }

        // Call
        Object apply( const Tuple &args ) const
        {
            PyObject *result = PyObject_CallObject( ptr(), args.ptr() );
            if( result == NULL )
            {
                throw Exception();
            }
            return asObject( result );
        }

        // Call with keywords
        Object apply( const Tuple &args, const Dict &kw ) const
        {
            PyObject *result = PyEval_CallObjectWithKeywords( ptr(), args.ptr(), kw.ptr() );
            if( result == NULL )
            {
                throw Exception();
            }
            return asObject( result );
        }

        Object apply( PyObject *pargs = 0 ) const
        {
            if( pargs == 0 )
            {
                return apply( Tuple() );
            }
            else
            {
                return apply( Tuple( pargs ) );
            }
        }
    };

    class PYCXX_EXPORT Module: public Object
    {
    public:
        explicit Module( PyObject *pyob, bool owned = false )
        : Object( pyob, owned )
        {
            validate();
        }

        // Construct from module name
        explicit Module( const std::string &s )
        : Object()
        {
            PyObject *m = PyImport_AddModule( const_cast<char *>( s.c_str() ) );
            set( m, false );
            validate();
        }

        // Copy constructor acquires new ownership of pointer
        Module( const Module &ob )
        : Object( *ob )
        {
            validate();
        }

        Module &operator=( const Object &rhs )
        {
            return *this = *rhs;
        }

        Module &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        Dict getDict() const
        {
            return Dict( PyModule_GetDict( ptr() ) );
            // Caution -- PyModule_GetDict returns borrowed reference!
        }
    };

    // Call function helper
    inline Object Object::callMemberFunction( const std::string &function_name ) const
    {
        Callable target( getAttr( function_name ) );
        Tuple args( (sequence_index_type)0 );
        return target.apply( args );
    }

    inline Object Object::callMemberFunction( const std::string &function_name, const Tuple &args ) const
    {
        Callable target( getAttr( function_name ) );
        return target.apply( args );
    }

    inline Object Object::callMemberFunction( const std::string &function_name, const Tuple &args, const Dict &kw ) const
    {
        Callable target( getAttr( function_name ) );
        return target.apply( args, kw );
    }

    // Numeric interface
    inline Object operator+( const Object &a )
    {
        return asObject( PyNumber_Positive( *a ) );
    }

    inline Object operator-( const Object &a )
    {
        return asObject( PyNumber_Negative( *a ) );
    }

    inline Object abs( const Object &a )
    {
        return asObject( PyNumber_Absolute( *a ) );
    }

    //------------------------------------------------------------
    // operator +
    inline Object operator+( const Object &a, const Object &b )
    {
        return asObject( PyNumber_Add( *a, *b ) );
    }

    inline Object operator+( const Object &a, int j )
    {
        return asObject( PyNumber_Add( *a, *Long( j ) ) );
    }

    inline Object operator+( const Object &a, long j )
    {
        return asObject( PyNumber_Add( *a, *Long( j ) ) );
    }

    inline Object operator+( const Object &a, double v )
    {
        return asObject( PyNumber_Add( *a, *Float( v ) ) );
    }

    inline Object operator+( int j, const Object &b )
    {
        return asObject( PyNumber_Add( *Long( j ), *b ) );
    }

    inline Object operator+( long j, const Object &b )
    {
        return asObject( PyNumber_Add( *Long( j ), *b ) );
    }

    inline Object operator+( double v, const Object &b )
    {
        return asObject( PyNumber_Add( *Float( v ), *b ) );
    }

    //------------------------------------------------------------
    // operator -
    inline Object operator-( const Object &a, const Object &b )
    {
        return asObject( PyNumber_Subtract( *a, *b ) );
    }

    inline Object operator-( const Object &a, int j )
    {
        return asObject( PyNumber_Subtract( *a, *Long( j ) ) );
    }

    inline Object operator-( const Object &a, double v )
    {
        return asObject( PyNumber_Subtract( *a, *Float( v ) ) );
    }

    inline Object operator-( int j, const Object &b )
    {
        return asObject( PyNumber_Subtract( *Long( j ), *b ) );
    }

    inline Object operator-( double v, const Object &b )
    {
        return asObject( PyNumber_Subtract( *Float( v ), *b ) );
    }

    //------------------------------------------------------------
    // operator *
    inline Object operator*( const Object &a, const Object &b )
    {
        return asObject( PyNumber_Multiply( *a, *b ) );
    }

    inline Object operator*( const Object &a, int j )
    {
        return asObject( PyNumber_Multiply( *a, *Long( j ) ) );
    }

    inline Object operator*( const Object &a, double v )
    {
        return asObject( PyNumber_Multiply( *a, *Float( v ) ) );
    }

    inline Object operator*( int j, const Object &b )
    {
        return asObject( PyNumber_Multiply( *Long( j ), *b ) );
    }

    inline Object operator*( double v, const Object &b )
    {
        return asObject( PyNumber_Multiply( *Float( v ), *b ) );
    }

    //------------------------------------------------------------
    // operator /
    inline Object operator/( const Object &a, const Object &b )
    {
        return asObject( PyNumber_TrueDivide( *a, *b ) );
    }

    inline Object operator/( const Object &a, int j )
    {
        return asObject( PyNumber_TrueDivide( *a, *Long( j ) ) );
    }

    inline Object operator/( const Object &a, double v )
    {
        return asObject( PyNumber_TrueDivide( *a, *Float( v ) ) );
    }

    inline Object operator/( int j, const Object &b )
    {
        return asObject( PyNumber_TrueDivide( *Long( j ), *b ) );
    }

    inline Object operator/( double v, const Object &b )
    {
        return asObject( PyNumber_TrueDivide( *Float( v ), *b ) );
    }

    //------------------------------------------------------------
    // operator %
    inline Object operator%( const Object &a, const Object &b )
    {
        return asObject( PyNumber_Remainder( *a, *b ) );
    }

    inline Object operator%( const Object &a, int j )
    {
        return asObject( PyNumber_Remainder( *a, *Long( j ) ) );
    }

    inline Object operator%( const Object &a, double v )
    {
        return asObject( PyNumber_Remainder( *a, *Float( v ) ) );
    }

    inline Object operator%( int j, const Object &b )
    {
        return asObject( PyNumber_Remainder( *Long( j ), *b ) );
    }

    inline Object operator%( double v, const Object &b )
    {
        return asObject( PyNumber_Remainder( *Float( v ), *b ) );
    }

    //------------------------------------------------------------
    // type
    inline Object type( const Exception &) // return the type of the error
    {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch( &ptype, &pvalue, &ptrace );
        Object result;
        if( ptype )
            result = ptype;
        PyErr_Restore( ptype, pvalue, ptrace );
        return result;
    }

    inline Object value( const Exception &) // return the value of the error
    {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch( &ptype, &pvalue, &ptrace );
        Object result;
        if( pvalue )
            result = pvalue;
        PyErr_Restore( ptype, pvalue, ptrace );
        return result;
    }

    inline Object trace( const Exception &) // return the traceback of the error
    {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch( &ptype, &pvalue, &ptrace );
        Object result;
        if( ptrace )
            result = ptrace;
        PyErr_Restore( ptype, pvalue, ptrace );
        return result;
    }

    template<TEMPLATE_TYPENAME T>
    String seqref<T>::str() const
    {
        return the_item.str();
    }

    template<TEMPLATE_TYPENAME T>
    String seqref<T>::repr() const
    {
        return the_item.repr();
    }

} // namespace Py
#endif    // __CXX_Objects__h
