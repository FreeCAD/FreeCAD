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

#ifndef __CXX_ExtensionOldType__h
#define __CXX_ExtensionOldType__h

namespace Py
{
    template<TEMPLATE_TYPENAME T> class PythonExtension
    : public PythonExtensionBase
    {
    public:
        static PyTypeObject *type_object()
        {
            return behaviors().type_object();
        }

        static bool check( PyObject *p )
        {
            // is p like me?
            return p->ob_type == type_object();
        }

        static bool check( const Object &ob )
        {
            return check( ob.ptr() );
        }

        //
        // every object needs getattr implemented
        // to support methods
        //
        virtual Object getattr( const char *name )
        {
            return getattr_methods( name );
        }

        PyObject *selfPtr()
        {
            return this;
        }

        Object self()
        {
            return asObject( this );
        }

    protected:
        explicit PythonExtension()
        : PythonExtensionBase()
        {
            PyObject_Init( this, type_object() );

            // every object must support getattr
            behaviors().supportGetattr();
        }

        virtual ~PythonExtension()
        {}

        static PythonType &behaviors()
        {
            static PythonType* p;
            if( p == NULL )
            {
#if defined( _CPPRTTI ) || defined( __GNUG__ )
                const char *default_name =( typeid( T ) ).name();
#else
                const char *default_name = "unknown";
#endif
                p = new PythonType( sizeof( T ), 0, default_name );
                p->set_tp_dealloc( extension_object_deallocator );
            }

            return *p;
        }

        typedef Object (T::*method_noargs_function_t)();
        typedef Object (T::*method_varargs_function_t)( const Tuple &args );
        typedef Object (T::*method_keyword_function_t)( const Tuple &args, const Dict &kws );
        typedef std::map<std::string, MethodDefExt<T> *> method_map_t;

        // support the default attributes, __name__, __doc__ and methods
        virtual Object getattr_default( const char *_name )
        {
            std::string name( _name );

            if( name == "__name__" && type_object()->tp_name != NULL )
            {
                return Py::String( type_object()->tp_name );
            }

            if( name == "__doc__" && type_object()->tp_doc != NULL )
            {
                return Py::String( type_object()->tp_doc );
            }

// trying to fake out being a class for help()
//            else if( name == "__bases__"  )
//            {
//                return Py::Tuple( 0 );
//            }
//            else if( name == "__module__"  )
//            {
//                return Py::Nothing();
//            }
//            else if( name == "__dict__"  )
//            {
//                return Py::Dict();
//            }

            return getattr_methods( _name );
        }

        // turn a name into function object
        virtual Object getattr_methods( const char *_name )
        {
            std::string name( _name );

            method_map_t &mm = methods();

            // see if name exists and get entry with method
            EXPLICIT_TYPENAME method_map_t::const_iterator i = mm.find( name );
            if( i == mm.end() )
            {
                if( name == "__methods__" )
                {
                    List methods;

                    i = mm.begin();
                    EXPLICIT_TYPENAME method_map_t::const_iterator i_end = mm.end();

                    for( ; i != i_end; ++i )
                        methods.append( String( (*i).first ) );

                    return methods;
                }

                throw AttributeError( name );
            }

            MethodDefExt<T> *method_def = i->second;

            Tuple self( 2 );

            self[0] = Object( this );
            self[1] = Object( PyCObject_FromVoidPtr( method_def, do_not_dealloc ), true );

            PyObject *func = PyCFunction_New( &method_def->ext_meth_def, self.ptr() );

            return Object(func, true);
        }

        // check that all methods added are unique
        static void check_unique_method_name( const char *name )
        {
            method_map_t &mm = methods();
            EXPLICIT_TYPENAME method_map_t::const_iterator i;
            i = mm.find( name );
            if( i != mm.end() )
                throw AttributeError( name );
        }

        static void add_noargs_method( const char *name, method_noargs_function_t function, const char *doc="" )
        {
            check_unique_method_name( name );
            method_map_t &mm = methods();
            mm[ std::string( name ) ] = new MethodDefExt<T>( name, function, method_noargs_call_handler, doc );
        }

        static void add_varargs_method( const char *name, method_varargs_function_t function, const char *doc="" )
        {
            check_unique_method_name( name );
            method_map_t &mm = methods();
            mm[ std::string( name ) ] = new MethodDefExt<T>( name, function, method_varargs_call_handler, doc );
        }

        static void add_keyword_method( const char *name, method_keyword_function_t function, const char *doc="" )
        {
            check_unique_method_name( name );
            method_map_t &mm = methods();
            mm[ std::string( name ) ] = new MethodDefExt<T>( name, function, method_keyword_call_handler, doc );
        }

    private:
        static method_map_t &methods( void )
        {
            static method_map_t *map_of_methods = NULL;
            if( map_of_methods == NULL )
                map_of_methods = new method_map_t;

            return *map_of_methods;
        }

        // Note: Python calls noargs as varargs buts args==NULL
        static PyObject *method_noargs_call_handler( PyObject *_self_and_name_tuple, PyObject * )
        {
            try
            {
                Tuple self_and_name_tuple( _self_and_name_tuple );

                PyObject *self_in_cobject = self_and_name_tuple[0].ptr();
                T *self = static_cast<T *>( self_in_cobject );
                MethodDefExt<T> *meth_def = reinterpret_cast<MethodDefExt<T> *>(
                                                PyCObject_AsVoidPtr( self_and_name_tuple[1].ptr() ) );
                Object result;

                // Adding try & catch in case of STL debug-mode exceptions.
                #ifdef _STLP_DEBUG
                try
                {
                    result = (self->*meth_def->ext_noargs_function)();
                }
                catch( std::__stl_debug_exception )
                {
                    // throw cxx::RuntimeError( sErrMsg );
                    throw RuntimeError( "Error message not set yet." );
                }
                #else
                result = (self->*meth_def->ext_noargs_function)();
                #endif // _STLP_DEBUG

                return new_reference_to( result.ptr() );
            }
            catch( Exception & )
            {
                return 0;
            }
        }

        static PyObject *method_varargs_call_handler( PyObject *_self_and_name_tuple, PyObject *_args )
        {
            try
            {
                Tuple self_and_name_tuple( _self_and_name_tuple );

                PyObject *self_in_cobject = self_and_name_tuple[0].ptr();
                T *self = static_cast<T *>( self_in_cobject );

                MethodDefExt<T> *meth_def = reinterpret_cast<MethodDefExt<T> *>(
                                                PyCObject_AsVoidPtr( self_and_name_tuple[1].ptr() ) );
                Tuple args( _args );

                Object result;

                // Adding try & catch in case of STL debug-mode exceptions.
                #ifdef _STLP_DEBUG
                try
                {
                    result = (self->*meth_def->ext_varargs_function)( args );
                }
                catch( std::__stl_debug_exception )
                {
                    throw RuntimeError( "Error message not set yet." );
                }
                #else
                result = (self->*meth_def->ext_varargs_function)( args );
                #endif // _STLP_DEBUG

                return new_reference_to( result.ptr() );
            }
            catch( Exception & )
            {
                return 0;
            }
        }

        static PyObject *method_keyword_call_handler( PyObject *_self_and_name_tuple, PyObject *_args, PyObject *_keywords )
        {
            try
            {
                Tuple self_and_name_tuple( _self_and_name_tuple );

                PyObject *self_in_cobject = self_and_name_tuple[0].ptr();
                T *self = static_cast<T *>( self_in_cobject );

                MethodDefExt<T> *meth_def = reinterpret_cast<MethodDefExt<T> *>(
                                                PyCObject_AsVoidPtr( self_and_name_tuple[1].ptr() ) );

                Tuple args( _args );

                // _keywords may be NULL so be careful about the way the dict is created
                Dict keywords;
                if( _keywords != NULL )
                    keywords = Dict( _keywords );

                Object result( ( self->*meth_def->ext_keyword_function )( args, keywords ) );

                return new_reference_to( result.ptr() );
            }
            catch( Exception & )
            {
                return 0;
            }
        }

        static void extension_object_deallocator( PyObject* t )
        {
            delete (T *)( t );
        }

        //
        // prevent the compiler generating these unwanted functions
        //
        explicit PythonExtension( const PythonExtension<T> &other );
        void operator=( const PythonExtension<T> &rhs );
    };

    //
    // ExtensionObject<T> is an Object that will accept only T's.
    //
    template<TEMPLATE_TYPENAME T>
    class ExtensionObject: public Object
    {
    public:

        explicit ExtensionObject( PyObject *pyob )
        : Object( pyob )
        {
            validate();
        }

        ExtensionObject( const ExtensionObject<T> &other )
        : Object( *other )
        {
            validate();
        }

        ExtensionObject( const Object &other )
        : Object( *other )
        {
            validate();
        }

        ExtensionObject &operator=( const Object &rhs )
        {
            return( *this = *rhs );
        }

        ExtensionObject &operator=( PyObject *rhsp )
        {
            if( ptr() != rhsp )
                set( rhsp );
            return *this;
        }

        virtual bool accepts( PyObject *pyob ) const
        {
            return( pyob && T::check( pyob ) );
        }

        //
        //    Obtain a pointer to the PythonExtension object
        //
        T *extensionObject( void )
        {
            return static_cast<T *>( ptr() );
        }
    };
} // Namespace Py

// End of __CXX_ExtensionOldType__h
#endif
