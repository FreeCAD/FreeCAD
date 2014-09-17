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

#ifndef __CXX_ExtensionTypeBase__h
#define __CXX_ExtensionTypeBase__h

namespace Py
{
    // Class PythonExtension is what you inherit from to create
    // a new Python extension type. You give your class itself
    // as the template paramter.

    // There are two ways that extension objects can get destroyed.
    // 1. Their reference count goes to zero
    // 2. Someone does an explicit delete on a pointer.
    // In(1) the problem is to get the destructor called 
    //      We register a special deallocator in the Python type object
    //      (see behaviors()) to do this.
    // In(2) there is no problem, the dtor gets called.

    // PythonExtension does not use the usual Python heap allocator, 
    // instead using new/delete. We do the setting of the type object
    // and reference count, usually done by PyObject_New, in the 
    // base class ctor.

    // This special deallocator does a delete on the pointer.

    class PythonExtensionBase : public PyObject
    {
    public:
        PythonExtensionBase();
        virtual ~PythonExtensionBase();

    public:
        // object 
        virtual void reinit( Tuple &args, Dict &kwds );

        // object basics
#ifdef PYCXX_PYTHON_2TO3
        virtual int print( FILE *, int );
#endif
        virtual Object getattr( const char * );
        virtual int setattr( const char *, const Object & );
        virtual Object getattro( const String & );
        Object genericGetAttro( const String & );
        virtual int setattro( const String &, const Object & );
        int genericSetAttro( const String &, const Object & );
        virtual int compare( const Object & );
        virtual Object rich_compare( const Object &, int );
        virtual Object repr();
        virtual Object str();
        virtual long hash();
        virtual Object call( const Object &, const Object & );
        virtual Object iter();
        virtual PyObject *iternext();

        // Sequence methods
        virtual int sequence_length();
        virtual Object sequence_concat( const Object & );
        virtual Object sequence_repeat( Py_ssize_t );
        virtual Object sequence_item( Py_ssize_t );
        virtual int sequence_ass_item( Py_ssize_t, const Object & );

        // Mapping
        virtual int mapping_length();
        virtual Object mapping_subscript( const Object & );
        virtual int mapping_ass_subscript( const Object &, const Object & );

        // Number
        virtual Object number_negative();
        virtual Object number_positive();
        virtual Object number_absolute();
        virtual Object number_invert();
        virtual Object number_int();
        virtual Object number_float();
        virtual Object number_long();
        virtual Object number_add( const Object & );
        virtual Object number_subtract( const Object & );
        virtual Object number_multiply( const Object & );
        virtual Object number_remainder( const Object & );
        virtual Object number_divmod( const Object & );
        virtual Object number_lshift( const Object & );
        virtual Object number_rshift( const Object & );
        virtual Object number_and( const Object & );
        virtual Object number_xor( const Object & );
        virtual Object number_or( const Object & );
        virtual Object number_power( const Object &, const Object & );

        // Buffer
        virtual int buffer_get( Py_buffer *, int flags );
        virtual int buffer_release( Py_buffer *buf );

    public:
        // helper functions to call function fn_name with 0 to 9 args
        Object callOnSelf( const std::string &fn_name );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1 );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1, const Object &arg2 );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1, const Object &arg2, const Object &arg3 );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1, const Object &arg2, const Object &arg3,
                                const Object &arg4 );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1, const Object &arg2, const Object &arg3,
                                const Object &arg4, const Object &arg5 );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1, const Object &arg2, const Object &arg3,
                                const Object &arg4, const Object &arg5, const Object &arg6 );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1, const Object &arg2, const Object &arg3,
                                const Object &arg4, const Object &arg5, const Object &arg6,
                                const Object &arg7 );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1, const Object &arg2, const Object &arg3,
                                const Object &arg4, const Object &arg5, const Object &arg6,
                                const Object &arg7, const Object &arg8 );
        Object callOnSelf( const std::string &fn_name,
                                const Object &arg1, const Object &arg2, const Object &arg3,
                                const Object &arg4, const Object &arg5, const Object &arg6,
                                const Object &arg7, const Object &arg8, const Object &arg9 );

    public:
        virtual PyObject *selfPtr() = 0;
        virtual Object self() = 0;

    private:
        void missing_method( void );
        static PyObject *method_call_handler( PyObject *self, PyObject *args );
    };

} // Namespace Py

// End of __CXX_ExtensionTypeBase__h
#endif
