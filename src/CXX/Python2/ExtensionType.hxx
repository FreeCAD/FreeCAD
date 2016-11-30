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

#ifndef __CXX_ExtensionClass__h
#define __CXX_ExtensionClass__h

#define PYCXX_NOARGS_METHOD_NAME( NAME ) _callNoArgsMethod__##NAME
#define PYCXX_VARARGS_METHOD_NAME( NAME ) _callVarArgsMethod__##NAME
#define PYCXX_KEYWORDS_METHOD_NAME( NAME ) _callKeywordsMethod__##NAME

#define PYCXX_NOARGS_METHOD_DECL( CLS, NAME ) \
    static PyObject *PYCXX_NOARGS_METHOD_NAME( NAME )( PyObject *_self, PyObject *, PyObject * ) \
    { \
        try \
        { \
            Py::PythonClassInstance *self_python = reinterpret_cast< Py::PythonClassInstance * >( _self ); \
            CLS *self = reinterpret_cast< CLS * >( self_python->m_pycxx_object ); \
            Py::Object r( (self->NAME)() ); \
            return Py::new_reference_to( r.ptr() ); \
        } \
        catch( Py::Exception & ) \
        { \
            return 0; \
        } \
    }
#define PYCXX_VARARGS_METHOD_DECL( CLS, NAME ) \
    static PyObject *PYCXX_VARARGS_METHOD_NAME( NAME )( PyObject *_self, PyObject *_a, PyObject * ) \
    { \
        try \
        { \
            Py::PythonClassInstance *self_python = reinterpret_cast< Py::PythonClassInstance * >( _self ); \
            CLS *self = reinterpret_cast< CLS * >( self_python->m_pycxx_object ); \
            Py::Tuple a( _a ); \
            Py::Object r( (self->NAME)( a ) ); \
            return Py::new_reference_to( r.ptr() ); \
        } \
        catch( Py::Exception & ) \
        { \
            return 0; \
        } \
    }
#define PYCXX_KEYWORDS_METHOD_DECL( CLS, NAME ) \
    static PyObject *PYCXX_KEYWORDS_METHOD_NAME( NAME )( PyObject *_self, PyObject *_a, PyObject *_k ) \
    { \
        try \
        { \
            Py::PythonClassInstance *self_python = reinterpret_cast< Py::PythonClassInstance * >( _self ); \
            CLS *self = reinterpret_cast< CLS * >( self_python->m_pycxx_object ); \
            Py::Tuple a( _a ); \
            Py::Dict k; \
            if( _k != NULL ) \
                k = _k; \
            Py::Object r( (self->NAME)( a, k ) ); \
            return Py::new_reference_to( r.ptr() ); \
        } \
        catch( Py::Exception & ) \
        { \
            return 0; \
        } \
    }

// need to support METH_STATIC and METH_CLASS

#define PYCXX_ADD_NOARGS_METHOD( PYNAME, NAME, docs ) \
    add_method( #PYNAME, (PyCFunction)PYCXX_NOARGS_METHOD_NAME( NAME ), METH_NOARGS, docs )
#define PYCXX_ADD_VARARGS_METHOD( PYNAME, NAME, docs ) \
    add_method( #PYNAME, (PyCFunction)PYCXX_VARARGS_METHOD_NAME( NAME ), METH_VARARGS, docs )
#define PYCXX_ADD_KEYWORDS_METHOD( PYNAME, NAME, docs ) \
    add_method( #PYNAME, (PyCFunction)PYCXX_KEYWORDS_METHOD_NAME( NAME ), METH_VARARGS | METH_KEYWORDS, docs )

namespace Py
{
    PYCXX_EXPORT extern PythonExtensionBase *getPythonExtensionBase( PyObject *self );

    struct PythonClassInstance
    {
        PyObject_HEAD
        PythonExtensionBase *m_pycxx_object;
    };


    class PYCXX_EXPORT ExtensionClassMethodsTable
    {
    public:
        ExtensionClassMethodsTable()
        : m_methods_table( new PyMethodDef[ METHOD_TABLE_SIZE_INCREMENT ] )
        , m_methods_used( 0 )
        , m_methods_size( METHOD_TABLE_SIZE_INCREMENT )
        {
        }

        ~ExtensionClassMethodsTable()
        {
            delete[] m_methods_table;
        }

        // check that all methods added are unique
        void check_unique_method_name( const char *_name )
        {
            std::string name( _name );
            for( int i=0; i<m_methods_used; i++ )
            {
                if( name == m_methods_table[i].ml_name )
                {
                    throw AttributeError( name );
                }
            }
        }
        PyMethodDef *add_method( const char *name, PyCFunction function, int flags, const char *doc )
        {
            check_unique_method_name( name );

            // see if there is enough space for one more method
            if( m_methods_used == (m_methods_size-1) )
            {
                PyMethodDef *old_mt = m_methods_table;
                m_methods_size += METHOD_TABLE_SIZE_INCREMENT;
                PyMethodDef *new_mt = new PyMethodDef[ m_methods_size ];
                for( int i=0; i<m_methods_used; i++ )
                {
                    new_mt[ i ] = old_mt[ i ];
                }
                delete[] old_mt;
                m_methods_table = new_mt;
            }

            // add method into the table
            PyMethodDef *p = &m_methods_table[ m_methods_used ];
            p->ml_name = const_cast<char *>( name );
            p->ml_meth = function;
            p->ml_flags = flags;
            p->ml_doc = const_cast<char *>( doc );

            m_methods_used++;
            p++;

            // add the sentinel marking the table end
            p->ml_name = NULL;
            p->ml_meth = NULL;
            p->ml_flags = 0;
            p->ml_doc = NULL;

            return m_methods_table;
        }

    private:
        enum {METHOD_TABLE_SIZE_INCREMENT = 1};
        PyMethodDef *m_methods_table;
        int m_methods_used;
        int m_methods_size;
    };

    template<TEMPLATE_TYPENAME T> class PythonClass
    : public PythonExtensionBase
    {
    protected:
        explicit PythonClass( PythonClassInstance *self, Tuple &/*args*/, Dict &/*kwds*/ )
        : PythonExtensionBase()
        , m_class_instance( self )
        {
        }

        virtual ~PythonClass()
        {} 

        static ExtensionClassMethodsTable &methodTable()
        {
            static ExtensionClassMethodsTable *method_table;
            if( method_table == NULL )
                method_table = new ExtensionClassMethodsTable;
            return *method_table;
        }

        static void add_method( const char *name, PyCFunction function, int flags, const char *doc=NULL )
        {
            behaviors().set_methods( methodTable().add_method( name, function, flags, doc ) );
        }

        static PythonType &behaviors()
        {
            static PythonType *p;
            if( p == NULL ) 
            {
#if defined( _CPPRTTI ) || defined( __GNUG__ )
                const char *default_name = (typeid( T )).name();
#else
                const char *default_name = "unknown";
#endif
                p = new PythonType( sizeof( PythonClassInstance ), 0, default_name );
                p->set_tp_new( extension_object_new );
                p->set_tp_init( extension_object_init );
                p->set_tp_dealloc( extension_object_deallocator );
                // we are a class
                p->supportClass();

                // always support get and set attr
                p->supportGetattro();
                p->supportSetattro();
            }

            return *p;
        }

        static PyObject *extension_object_new( PyTypeObject *subtype, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
#ifdef PYCXX_DEBUG
            std::cout << "extension_object_new()" << std::endl;
#endif
            PythonClassInstance *o = reinterpret_cast<PythonClassInstance *>( subtype->tp_alloc( subtype, 0 ) );
            if( o == NULL )
                return NULL;

            o->m_pycxx_object = NULL;

            PyObject *self = reinterpret_cast<PyObject *>( o );
#ifdef PYCXX_DEBUG
            std::cout << "extension_object_new() => self=0x" << std::hex << reinterpret_cast< unsigned int >( self ) << std::dec << std::endl;
#endif
            return self;
        }

        static int extension_object_init( PyObject *_self, PyObject *args_, PyObject *kwds_ )
        {
            try
            {
                Py::Tuple args( args_ );
                Py::Dict kwds;
                if( kwds_ != NULL )
                    kwds = kwds_;

                PythonClassInstance *self = reinterpret_cast<PythonClassInstance *>( _self );
#ifdef PYCXX_DEBUG
                std::cout << "extension_object_init( self=0x" << std::hex << reinterpret_cast< unsigned int >( self ) << std::dec << " )" << std::endl;
                std::cout << "    self->m_pycxx_object=0x" << std::hex << reinterpret_cast< unsigned int >( self->m_pycxx_object ) << std::dec << std::endl;
#endif

                if( self->m_pycxx_object == NULL )
                {
                    self->m_pycxx_object = new T( self, args, kwds );
#ifdef PYCXX_DEBUG
                    std::cout << "    self->m_pycxx_object=0x" << std::hex << reinterpret_cast< unsigned int >( self->m_pycxx_object ) << std::dec << std::endl;
#endif
                }
                else
                {
#ifdef PYCXX_DEBUG
                    std::cout << "    reinit - self->m_pycxx_object=0x" << std::hex << reinterpret_cast< unsigned int >( self->m_pycxx_object ) << std::dec << std::endl;
#endif
                    self->m_pycxx_object->reinit( args, kwds );
                }
            }
            catch( Exception & )
            {
                return -1;
            }
            return 0;
        }

        static void extension_object_deallocator( PyObject *_self )
        {
            PythonClassInstance *self = reinterpret_cast< PythonClassInstance * >( _self );
#ifdef PYCXX_DEBUG
            std::cout << "extension_object_deallocator( self=0x" << std::hex << reinterpret_cast< unsigned int >( self ) << std::dec << " )" << std::endl;
            std::cout << "    self->m_pycxx_object=0x" << std::hex << reinterpret_cast< unsigned int >( self->m_pycxx_object ) << std::dec << std::endl;
#endif
            delete self->m_pycxx_object;
            _self->ob_type->tp_free( _self );
        }

    public:
        static PyTypeObject *type_object()
        {
            return behaviors().type_object();
        }

        static Object type()
        {
            return Object( reinterpret_cast<PyObject *>( behaviors().type_object() ) );
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

        virtual PyObject *selfPtr()
        {
            return reinterpret_cast<PyObject *>( m_class_instance );
        }

        virtual Object self()
        {
            return Object( reinterpret_cast<PyObject *>( m_class_instance ) );
        }

    protected:
    private:
        PythonClassInstance *m_class_instance;

    private:
        //
        // prevent the compiler generating these unwanted functions
        //
        explicit PythonClass( const PythonClass<T> &other );
        void operator=( const PythonClass<T> &rhs );
    };

    //
    // ExtensionObject<T> is an Object that will accept only T's.
    //
    template<TEMPLATE_TYPENAME T>
    class PythonClassObject: public Object
    {
    public:

        explicit PythonClassObject( PyObject *pyob )
        : Object( pyob )
        {
            validate();
        }

        PythonClassObject( const PythonClassObject<T> &other )
        : Object( *other )
        {
            validate();
        }

        PythonClassObject( const Object &other )
        : Object( *other )
        {
            validate();
        }

        PythonClassObject &operator=( const Object &rhs )
        {
            *this = *rhs;
            return *this;
        }

        PythonClassObject &operator=( PyObject *rhsp )
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
        T *getCxxObject( void )
        {
            return dynamic_cast< T * >( getPythonExtensionBase( ptr() ) );
        }
    };
} // Namespace Py

// End of __CXX_ExtensionClass__h
#endif
