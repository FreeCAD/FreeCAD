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
#include "CXX/Extensions.hxx"
#include "CXX/Exception.hxx"
#include "CXX/Objects.hxx"

#include <assert.h>

#ifdef PYCXX_DEBUG
//
//  Functions useful when debugging PyCXX
//
void bpt( void )
{
}

void printRefCount( PyObject *obj )
{
    std::cout << "RefCount of 0x" << std::hex << reinterpret_cast< unsigned long >( obj ) << std::dec << " is " << Py_REFCNT( obj ) << std::endl;
}
#endif

namespace Py
{

void Object::validate()
{
    // release pointer if not the right type
    if( !accepts( p ) )
    {
#if defined( _CPPRTTI ) || defined( __GNUG__ )
        std::string s( "PyCXX: Error creating object of type " );
        s += (typeid( *this )).name();

        if( p != NULL )
        {
            String from_repr = repr();
            s += " from ";
            s += from_repr.as_std_string();
        }
        else
        {
            s += " from (nil)";
        }
#endif
        release();

        // If error message already set
        ifPyErrorThrowCxxException();

        // Better error message if RTTI available
#if defined( _CPPRTTI ) || defined( __GNUG__ )
        throw TypeError( s );
#else
        throw TypeError( "PyCXX: type error." );
#endif
    }
}

//================================================================================
//
//    Implementation of MethodTable
//
//================================================================================

PyMethodDef MethodTable::method( const char *method_name, PyCFunction f, int flags, const char *doc )
{
    PyMethodDef m;
    m.ml_name = const_cast<char *>( method_name );
    m.ml_meth = f;
    m.ml_flags = flags;
    m.ml_doc = const_cast<char *>( doc );
    return m;
}

MethodTable::MethodTable()
{
    t.push_back( method( 0, 0, 0, 0 ) );
    mt = NULL;
}

MethodTable::~MethodTable()
{
    delete [] mt;
}

void MethodTable::add( const char *method_name, PyCFunction f, const char *doc, int flag )
{
    if( !mt )
    {
        t.insert( t.end()-1, method( method_name, f, flag, doc ) );
    }
    else
    {
        throw RuntimeError( "Too late to add a module method!" );
    }
}

PyMethodDef *MethodTable::table()
{
    if( !mt )
    {
        Py_ssize_t t1size = t.size();
        mt = new PyMethodDef[ t1size ];
        int j = 0;
        for( std::vector<PyMethodDef>::iterator i = t.begin(); i != t.end(); i++ )
        {
            mt[ j++ ] = *i;
        }
    }
    return mt;
}

//================================================================================
//
//    Implementation of ExtensionModule
//
//================================================================================
ExtensionModuleBase::ExtensionModuleBase( const char *name )
: m_module_name( name )
#if defined( Py_LIMITED_API )
, m_full_module_name( m_module_name )
#else
, m_full_module_name( __Py_PackageContext() != NULL ? std::string( __Py_PackageContext() ) : m_module_name )
#endif
, m_method_table()
//m_module_def
, m_module( NULL )
{}

ExtensionModuleBase::~ExtensionModuleBase()
{}

const std::string &ExtensionModuleBase::name() const
{
    return m_module_name;
}

const std::string &ExtensionModuleBase::fullName() const
{
    return m_full_module_name;
}

class ExtensionModuleBasePtr : public PythonExtension<ExtensionModuleBasePtr>
{
public:
    ExtensionModuleBasePtr( ExtensionModuleBase *_module )
    : module( _module )
    {}

    virtual ~ExtensionModuleBasePtr()
    {}

    ExtensionModuleBase *module;
};

void initExceptions();

void ExtensionModuleBase::initialize( const char *module_doc )
{
    // init the exception code
    initExceptions();

    memset( &m_module_def, 0, sizeof( m_module_def ) );

    m_module_def.m_name = const_cast<char *>( m_module_name.c_str() );
    m_module_def.m_doc = const_cast<char *>( module_doc );
    m_module_def.m_methods = m_method_table.table();
    // where does module_ptr get passed in?

    m_module = PyModule_Create( &m_module_def );
}

Module ExtensionModuleBase::module( void ) const
{
    return Module( m_module );
}

Dict ExtensionModuleBase::moduleDictionary( void ) const
{
    return module().getDict();
}

Object ExtensionModuleBase::moduleObject( void ) const
{
    return Object( m_module );
}

//================================================================================
//
//    Implementation of PythonType
//
//================================================================================
extern "C"
{
    static void standard_dealloc( PyObject *p );
    //
    // All the following functions redirect the call from Python
    // onto the matching virtual function in PythonExtensionBase
    //
#if defined( PYCXX_PYTHON_2TO3 ) && !defined( Py_LIMITED_API ) && PY_MINOR_VERSION <= 7
    static int print_handler( PyObject *, FILE *, int );
#endif
    static PyObject *getattr_handler( PyObject *, char * );
    static int setattr_handler( PyObject *, char *, PyObject * );
    static PyObject *getattro_handler( PyObject *, PyObject * );
    static int setattro_handler( PyObject *, PyObject *, PyObject * );
    static PyObject *rich_compare_handler( PyObject *, PyObject *, int );
    static PyObject *repr_handler( PyObject * );
    static PyObject *str_handler( PyObject * );
    static Py_hash_t hash_handler( PyObject * );
    static PyObject *call_handler( PyObject *, PyObject *, PyObject * );
    static PyObject *iter_handler( PyObject * );
    static PyObject *iternext_handler( PyObject * );

    // Sequence methods
    static Py_ssize_t sequence_length_handler( PyObject * );
    static PyObject *sequence_concat_handler( PyObject *,PyObject * );
    static PyObject *sequence_repeat_handler( PyObject *, Py_ssize_t );
    static PyObject *sequence_item_handler( PyObject *, Py_ssize_t );
    static int sequence_ass_item_handler( PyObject *, Py_ssize_t, PyObject * );

    static PyObject *sequence_inplace_concat_handler( PyObject *, PyObject * );
    static PyObject *sequence_inplace_repeat_handler( PyObject *, Py_ssize_t );

    static int sequence_contains_handler( PyObject *, PyObject * );

    // Mapping
    static Py_ssize_t mapping_length_handler( PyObject * );
    static PyObject *mapping_subscript_handler( PyObject *, PyObject * );
    static int mapping_ass_subscript_handler( PyObject *, PyObject *, PyObject * );

    // Numeric methods
    static PyObject *number_negative_handler( PyObject * );
    static PyObject *number_positive_handler( PyObject * );
    static PyObject *number_absolute_handler( PyObject * );
    static PyObject *number_invert_handler( PyObject * );
    static PyObject *number_int_handler( PyObject * );
    static PyObject *number_float_handler( PyObject * );
    static PyObject *number_add_handler( PyObject *, PyObject * );
    static PyObject *number_subtract_handler( PyObject *, PyObject * );
    static PyObject *number_multiply_handler( PyObject *, PyObject * );
    static PyObject *number_remainder_handler( PyObject *, PyObject * );
    static PyObject *number_divmod_handler( PyObject *, PyObject * );
    static PyObject *number_lshift_handler( PyObject *, PyObject * );
    static PyObject *number_rshift_handler( PyObject *, PyObject * );
    static PyObject *number_and_handler( PyObject *, PyObject * );
    static PyObject *number_xor_handler( PyObject *, PyObject * );
    static PyObject *number_or_handler( PyObject *, PyObject * );
    static PyObject *number_power_handler( PyObject *, PyObject *, PyObject * );
    static PyObject *number_floor_divide_handler( PyObject *, PyObject * );
    static PyObject *number_true_divide_handler( PyObject *, PyObject * );
    static PyObject *number_index_handler( PyObject * );
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
    static PyObject *number_matrix_multiply_handler( PyObject *, PyObject * );
#endif

    static PyObject *number_inplace_add_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_subtract_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_multiply_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_remainder_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_power_handler( PyObject *, PyObject *, PyObject * );
    static PyObject *number_inplace_lshift_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_rshift_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_and_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_xor_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_or_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_floor_divide_handler( PyObject *, PyObject * );
    static PyObject *number_inplace_true_divide_handler( PyObject *, PyObject * );
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
    static PyObject *number_inplace_matrix_multiply_handler( PyObject *, PyObject * );
#endif

    // Buffer
#if !defined( Py_LIMITED_API )
    static int buffer_get_handler( PyObject *, Py_buffer *, int );
    static void buffer_release_handler( PyObject *, Py_buffer * );
#endif
}

extern "C" void standard_dealloc( PyObject *p )
{
    PyMem_DEL( p );
}

bool PythonType::readyType()
{
#if defined( Py_LIMITED_API )
    if( !tp_object )
    {
        std::vector<PyType_Slot> spec_slots( slots.size() + 1 );
        int index = 0;

        for (std::unordered_map<int, void*>::const_iterator i = slots.cbegin(); i != slots.cend(); i++)
        {
            spec_slots[ index ].slot = i->first;
            spec_slots[ index ].pfunc = i->second;
            index++;
        }
        spec_slots[ index ].slot = 0;
        spec->slots = spec_slots.data();
        tp_object = reinterpret_cast<PyTypeObject *>( PyType_FromSpec(spec) );
    }
    return tp_object != NULL;
#else
    return PyType_Ready( table ) >= 0;
#endif
}

#if defined( Py_LIMITED_API )
#define FILL_SEQUENCE_SLOT(slot) \
    if( methods_to_support&support_sequence_ ## slot ) { \
        slots[ Py_sq_ ## slot ] = reinterpret_cast<void *>( sequence_ ## slot ## _handler ); \
    }
#else
#define FILL_SEQUENCE_SLOT(slot) \
    if( methods_to_support&support_sequence_ ## slot ) { \
        sequence_table->sq_ ## slot = sequence_ ## slot ## _handler; \
    }
#endif

PythonType &PythonType::supportSequenceType( int methods_to_support ) {
#if !defined( Py_LIMITED_API )
    if(sequence_table)
    {
        return *this;
    }
    sequence_table = new PySequenceMethods;
    memset( sequence_table, 0, sizeof( PySequenceMethods ) );   // ensure new fields are 0
    table->tp_as_sequence = sequence_table;
#endif

    FILL_SEQUENCE_SLOT(length)
    FILL_SEQUENCE_SLOT(concat)
    FILL_SEQUENCE_SLOT(repeat)
    FILL_SEQUENCE_SLOT(item)
    FILL_SEQUENCE_SLOT(ass_item)
    FILL_SEQUENCE_SLOT(inplace_concat)
    FILL_SEQUENCE_SLOT(inplace_repeat)
    FILL_SEQUENCE_SLOT(contains)
    return *this;
}

#undef FILL_SEQUENCE_SLOT

#if defined( Py_LIMITED_API )
#define FILL_MAPPING_SLOT(slot) \
    if( methods_to_support&support_mapping_ ## slot ) { \
        slots[ Py_mp_ ## slot ] = reinterpret_cast<void *>( mapping_ ## slot ## _handler ); \
    }
#else
#define FILL_MAPPING_SLOT(slot) \
    if( methods_to_support&support_mapping_ ## slot ) { \
        mapping_table->mp_ ## slot = mapping_ ## slot ## _handler; \
    }
#endif

PythonType &PythonType::supportMappingType( int methods_to_support )
{
#if !defined( Py_LIMITED_API )
    if( mapping_table )
    {
        return *this;
    }
    mapping_table = new PyMappingMethods;
    memset( mapping_table, 0, sizeof( PyMappingMethods ) );   // ensure new fields are 0
    table->tp_as_mapping = mapping_table;
#endif
    FILL_MAPPING_SLOT(length)
    FILL_MAPPING_SLOT(subscript)
    FILL_MAPPING_SLOT(ass_subscript)
    return *this;
}

#undef FILL_MAPPING_SLOT

#if defined( Py_LIMITED_API )
#define FILL_NUMBER_SLOT(slot) \
    if( methods_to_support&support_number_ ## slot ) { \
        slots[ Py_nb_ ## slot ] = reinterpret_cast<void *>( number_ ## slot ## _handler ); \
    }
#define FILL_NUMBER_INPLACE_SLOT(slot) \
    if( inplace_methods_to_support&support_number_ ## slot ) { \
        slots[ Py_nb_ ## slot ] = reinterpret_cast<void *>( number_ ## slot ## _handler ); \
    }
#else
#define FILL_NUMBER_SLOT(slot) \
    if( methods_to_support&support_number_ ## slot ) { \
        number_table->nb_ ## slot = number_ ## slot ## _handler; \
    }
#define FILL_NUMBER_INPLACE_SLOT(slot) \
    if( inplace_methods_to_support&support_number_ ## slot ) { \
        number_table->nb_ ## slot = number_ ## slot ## _handler; \
    }
#endif

PythonType &PythonType::supportNumberType( int methods_to_support, int inplace_methods_to_support )
{
#if !defined( Py_LIMITED_API )
    if( number_table )
    {
        return *this;
    }
    number_table = new PyNumberMethods;
    memset( number_table, 0, sizeof( PyNumberMethods ) );   // ensure new fields are 0
    table->tp_as_number = number_table;
#endif

    FILL_NUMBER_SLOT(add)
    FILL_NUMBER_SLOT(subtract)
    FILL_NUMBER_SLOT(multiply)
    FILL_NUMBER_SLOT(remainder)
    FILL_NUMBER_SLOT(divmod)
    FILL_NUMBER_SLOT(power)
    FILL_NUMBER_SLOT(negative)
    FILL_NUMBER_SLOT(positive)
    FILL_NUMBER_SLOT(absolute)
    FILL_NUMBER_SLOT(invert)
    FILL_NUMBER_SLOT(lshift)
    FILL_NUMBER_SLOT(rshift)
    FILL_NUMBER_SLOT(and)
    FILL_NUMBER_SLOT(xor)
    FILL_NUMBER_SLOT(or)
    FILL_NUMBER_SLOT(int)
    FILL_NUMBER_SLOT(float)
    FILL_NUMBER_SLOT(floor_divide)
    FILL_NUMBER_SLOT(true_divide)
    FILL_NUMBER_SLOT(index)
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
    FILL_NUMBER_SLOT(matrix_multiply)
#endif

    FILL_NUMBER_INPLACE_SLOT(inplace_add)
    FILL_NUMBER_INPLACE_SLOT(inplace_subtract)
    FILL_NUMBER_INPLACE_SLOT(inplace_multiply)
    FILL_NUMBER_INPLACE_SLOT(inplace_remainder)
    FILL_NUMBER_INPLACE_SLOT(inplace_power)
    FILL_NUMBER_INPLACE_SLOT(inplace_lshift)
    FILL_NUMBER_INPLACE_SLOT(inplace_rshift)
    FILL_NUMBER_INPLACE_SLOT(inplace_and)
    FILL_NUMBER_INPLACE_SLOT(inplace_xor)
    FILL_NUMBER_INPLACE_SLOT(inplace_or)
    FILL_NUMBER_INPLACE_SLOT(inplace_floor_divide)
    FILL_NUMBER_INPLACE_SLOT(inplace_true_divide)
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
    FILL_NUMBER_INPLACE_SLOT(inplace_matrix_multiply)
#endif

    return *this;
}

#undef FILL_NUMBER_SLOT

#if !defined( Py_LIMITED_API )
PythonType &PythonType::supportBufferType( int methods_to_support )
{
    if( !buffer_table )
    {
        buffer_table = new PyBufferProcs;
        memset( buffer_table, 0, sizeof( PyBufferProcs ) );   // ensure new fields are 0
        table->tp_as_buffer = buffer_table;

        if( methods_to_support&support_buffer_getbuffer )
        {
            buffer_table->bf_getbuffer = buffer_get_handler;
        }
        if( methods_to_support&support_buffer_releasebuffer )
        {
            buffer_table->bf_releasebuffer = buffer_release_handler;
        }
    }
    return *this;
}
#endif

// if you define one sequence method you must define
// all of them except the assigns

#if defined( Py_LIMITED_API )
PythonType::PythonType( size_t basic_size, int itemsize, const char *default_name )
: spec( new PyType_Spec )
{
    memset( spec, 0, sizeof( PyType_Spec ) );
    spec->name = const_cast<char *>( default_name );
    spec->basicsize = basic_size;
    spec->itemsize = itemsize;
    spec->flags = Py_TPFLAGS_DEFAULT;

    slots[ Py_tp_dealloc ] = reinterpret_cast<void *>( standard_dealloc );

    tp_object = 0;
}

#else
PythonType::PythonType( size_t basic_size, int itemsize, const char *default_name )
: table( new PyTypeObject )
, sequence_table( NULL )
, mapping_table( NULL )
, number_table( NULL )
, buffer_table( NULL )
{
    // PyTypeObject is defined in <python-sources>/Include/object.h

    memset( table, 0, sizeof( PyTypeObject ) );   // ensure new fields are 0
    *reinterpret_cast<PyObject *>( table ) = py_object_initializer;
    reinterpret_cast<PyObject *>( table )->ob_type = _Type_Type();
    // QQQ table->ob_size = 0;
    table->tp_name = const_cast<char *>( default_name );
    table->tp_basicsize = basic_size;
    table->tp_itemsize = itemsize;

    // Methods to implement standard operations
    table->tp_dealloc = (destructor)standard_dealloc;
#if PY_VERSION_HEX < 0x03080000
    table->tp_print = 0;
#else
    table->tp_vectorcall_offset = 0;
#endif
    table->tp_getattr = 0;
    table->tp_setattr = 0;
    table->tp_repr = 0;

    // Method suites for standard classes
    table->tp_as_number = 0;
    table->tp_as_sequence = 0;
    table->tp_as_mapping =  0;

    // More standard operations (here for binary compatibility)
    table->tp_hash = 0;
    table->tp_call = 0;
    table->tp_str = 0;
    table->tp_getattro = 0;
    table->tp_setattro = 0;

    // Functions to access object as input/output buffer
    table->tp_as_buffer = 0;

    // Flags to define presence of optional/expanded features
    table->tp_flags = Py_TPFLAGS_DEFAULT;

    // Documentation string
    table->tp_doc = 0;

    table->tp_traverse = 0;

    // delete references to contained objects
    table->tp_clear = 0;

    // Assigned meaning in release 2.1
    // rich comparisons
    table->tp_richcompare = 0;
    // weak reference enabler
    table->tp_weaklistoffset = 0;

    // Iterators
    table->tp_iter = 0;
    table->tp_iternext = 0;

    // Attribute descriptor and subclassing stuff
    table->tp_methods = 0;
    table->tp_members = 0;
    table->tp_getset = 0;
    table->tp_base = 0;
    table->tp_dict = 0;
    table->tp_descr_get = 0;
    table->tp_descr_set = 0;
    table->tp_dictoffset = 0;
    table->tp_init = 0;
    table->tp_alloc = 0;
    table->tp_new = 0;
    table->tp_free = 0;     // Low-level free-memory routine
    table->tp_is_gc = 0;    // For PyObject_IS_GC
    table->tp_bases = 0;
    table->tp_mro = 0;      // method resolution order
    table->tp_cache = 0;
    table->tp_subclasses = 0;
    table->tp_weaklist = 0;
    table->tp_del = 0;

    // Type attribute cache version tag. Added in version 2.6
    table->tp_version_tag = 0;

#ifdef COUNT_ALLOCS
    table->tp_alloc = 0;
    table->tp_free = 0;
    table->tp_maxalloc = 0;
    table->tp_orev = 0;
    table->tp_next = 0;
#endif
}
#endif

PythonType::~PythonType()
{
#if defined( Py_LIMITED_API )
    delete spec;
    PyObject_Free( tp_object );
#else
    delete table;
    delete sequence_table;
    delete mapping_table;
    delete number_table;
    delete buffer_table;
#endif
}

PyTypeObject *PythonType::type_object() const
{
#if defined( Py_LIMITED_API )
    return tp_object;
#else
    return table;
#endif
}

PythonType &PythonType::name( const char *nam )
{
#if defined( Py_LIMITED_API )
    spec->name = nam;
#else
    table->tp_name = const_cast<char *>( nam );
#endif
    return *this;
}

const char *PythonType::getName() const
{
#if defined( Py_LIMITED_API )
    return spec->name;
#else
    return table->tp_name;
#endif
}

PythonType &PythonType::doc( const char *d )
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_doc ] = reinterpret_cast<void *>( const_cast<char *>( d ) );
#else
    table->tp_doc = const_cast<char *>( d );
#endif
    return *this;
}

const char *PythonType::getDoc() const
{
#if defined( Py_LIMITED_API )
    if( tp_object )
        return reinterpret_cast<char *>( PyType_GetSlot( tp_object, Py_tp_doc ) );

    std::unordered_map<int, void*>::const_iterator slot = slots.find( Py_tp_doc );
    if( slot == slots.end() )
        return NULL;
    return reinterpret_cast<char *>( slot->second );
#else
    return table->tp_doc;
#endif
}

PythonType &PythonType::set_tp_dealloc( void (*tp_dealloc)( PyObject *self ) )
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_dealloc ] = reinterpret_cast<void *>( tp_dealloc );
#else
    table->tp_dealloc = tp_dealloc;
#endif
    return *this;
}

PythonType &PythonType::set_tp_init( int (*tp_init)( PyObject *self, PyObject *args, PyObject *kwds ) )
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_init ] = reinterpret_cast<void *>( tp_init );
#else
    table->tp_init = tp_init;
#endif
    return *this;
}

PythonType &PythonType::set_tp_new( PyObject *(*tp_new)( PyTypeObject *subtype, PyObject *args, PyObject *kwds ) )
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_new ] = reinterpret_cast<void *>( tp_new );
#else
    table->tp_new = tp_new;
#endif
    return *this;
}

PythonType &PythonType::set_methods( PyMethodDef *methods )
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_methods ] = reinterpret_cast<void *>( methods );
#else
    table->tp_methods = methods;
#endif
    return *this;
}

PythonType &PythonType::supportClass()
{
#if defined( Py_LIMITED_API )
    spec->flags |= Py_TPFLAGS_BASETYPE;
#else
    table->tp_flags |= Py_TPFLAGS_BASETYPE;
#endif
    return *this;
}

#if defined( PYCXX_PYTHON_2TO3 ) && !defined( Py_LIMITED_API ) && PY_MINOR_VERSION <= 7
PythonType &PythonType::supportPrint()
{
#if PY_VERSION_HEX < 0x03080000
    table->tp_print = print_handler;
#endif
    return *this;
}
#endif

PythonType &PythonType::supportGetattr()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_getattr ] = reinterpret_cast<void *>( getattr_handler );
#else
    table->tp_getattr = getattr_handler;
#endif
    return *this;
}

PythonType &PythonType::supportSetattr()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_setattr ] = reinterpret_cast<void *>( setattr_handler );
#else
    table->tp_setattr = setattr_handler;
#endif
    return *this;
}

PythonType &PythonType::supportGetattro()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_getattro ] = reinterpret_cast<void *>( getattro_handler );
#else
    table->tp_getattro = getattro_handler;
#endif
    return *this;
}

PythonType &PythonType::supportSetattro()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_setattro ] = reinterpret_cast<void *>( setattro_handler );
#else
    table->tp_setattro = setattro_handler;
#endif
    return *this;
}

#ifdef PYCXX_PYTHON_2TO3
PythonType &PythonType::supportCompare( void )
{
    return *this;
}
#endif


PythonType &PythonType::supportRichCompare()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_richcompare ] = reinterpret_cast<void *>( rich_compare_handler );
#else
    table->tp_richcompare = rich_compare_handler;
#endif
    return *this;
}

PythonType &PythonType::supportRepr()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_repr ] = reinterpret_cast<void *>( repr_handler );
#else
    table->tp_repr = repr_handler;
#endif
    return *this;
}

PythonType &PythonType::supportStr()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_str ] = reinterpret_cast<void *>( str_handler );
#else
    table->tp_str = str_handler;
#endif
    return *this;
}

PythonType &PythonType::supportHash()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_hash ] = reinterpret_cast<void *>( hash_handler );
#else
    table->tp_hash = hash_handler;
#endif
    return *this;
}

PythonType &PythonType::supportCall()
{
#if defined( Py_LIMITED_API )
    slots[ Py_tp_call ] = reinterpret_cast<void *>( call_handler );
#else
    table->tp_call = call_handler;
#endif
    return *this;
}

PythonType &PythonType::supportIter( int methods_to_support )
{
    if( methods_to_support&support_iter_iter )
    {
#if defined( Py_LIMITED_API )
        slots[ Py_tp_iter ] = reinterpret_cast<void *>( iter_handler );
#else
        table->tp_iter = iter_handler;
#endif
    }
    if( methods_to_support&support_iter_iternext )
    {
#if defined( Py_LIMITED_API )
        slots[ Py_tp_iternext ] = reinterpret_cast<void *>( iternext_handler );
#else
        table->tp_iternext = iternext_handler;
#endif
    }
    return *this;
}

//--------------------------------------------------------------------------------
//
//    Handlers
//
//--------------------------------------------------------------------------------
PythonExtensionBase *getPythonExtensionBase( PyObject *self )
{
    if(PyType_HasFeature(self->ob_type, Py_TPFLAGS_BASETYPE))
    {
        PythonClassInstance *instance = reinterpret_cast<PythonClassInstance *>( self );
        return instance->m_pycxx_object;
    }
    else
    {
        return static_cast<PythonExtensionBase *>( self );
    }
}

#if defined( PYCXX_PYTHON_2TO3 ) && !defined ( Py_LIMITED_API ) && PY_MINOR_VERSION <= 7
extern "C" int print_handler( PyObject *self, FILE *fp, int flags )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->print( fp, flags );
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}
#endif

extern "C" PyObject *getattr_handler( PyObject *self, char *name )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->getattr( name ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" int setattr_handler( PyObject *self, char *name, PyObject *value )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->setattr( name, Object( value ) );
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

extern "C" PyObject *getattro_handler( PyObject *self, PyObject *name )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->getattro( String( name ) ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" int setattro_handler( PyObject *self, PyObject *name, PyObject *value )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->setattro( String( name ), Object( value ) );
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

extern "C" PyObject *rich_compare_handler( PyObject *self, PyObject *other, int op )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->rich_compare( Object( other ), op ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" PyObject *repr_handler( PyObject *self )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->repr() );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" PyObject *str_handler( PyObject *self )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->str() );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" Py_hash_t hash_handler( PyObject *self )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->hash();
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

extern "C" PyObject *call_handler( PyObject *self, PyObject *args, PyObject *kw )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        if( kw != NULL )
            return new_reference_to( p->call( Object( args ), Object( kw ) ) );
        else
            return new_reference_to( p->call( Object( args ), Object() ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" PyObject *iter_handler( PyObject *self )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->iter() );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" PyObject *iternext_handler( PyObject *self )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->iternext();  // might be a NULL ptr on end of iteration
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}


// Sequence methods
extern "C" Py_ssize_t sequence_length_handler( PyObject *self )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->sequence_length();
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

extern "C" PyObject *sequence_concat_handler( PyObject *self, PyObject *other )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->sequence_concat( Object( other ) ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" PyObject *sequence_repeat_handler( PyObject *self, Py_ssize_t count )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->sequence_repeat( count ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" PyObject *sequence_item_handler( PyObject *self, Py_ssize_t index )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->sequence_item( index ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" int sequence_ass_item_handler( PyObject *self, Py_ssize_t index, PyObject *value )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->sequence_ass_item( index, Object( value ) );
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

extern "C" PyObject *sequence_inplace_concat_handler( PyObject *self, PyObject *o2 )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->sequence_inplace_concat( Object( o2 ) ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" PyObject *sequence_inplace_repeat_handler( PyObject *self, Py_ssize_t count )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->sequence_inplace_repeat( count ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" int sequence_contains_handler( PyObject *self, PyObject *value )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->sequence_contains( Object( value ) );
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

// Mapping
extern "C" Py_ssize_t mapping_length_handler( PyObject *self )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->mapping_length();
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

extern "C" PyObject *mapping_subscript_handler( PyObject *self, PyObject *key )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return new_reference_to( p->mapping_subscript( Object( key ) ) );
    }
    catch( BaseException & )
    {
        return NULL;    // indicate error
    }
}

extern "C" int mapping_ass_subscript_handler( PyObject *self, PyObject *key, PyObject *value )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->mapping_ass_subscript( Object( key ), Object( value ) );
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

// Number
#define NUMBER_UNARY( slot ) \
extern "C" PyObject *number_ ## slot ## _handler( PyObject *self ) \
{ \
    try \
    { \
        PythonExtensionBase *p = getPythonExtensionBase( self ); \
        return new_reference_to( p->number_ ## slot() ); \
    } \
    catch( BaseException & ) \
    { \
        return NULL; /* indicates error */ \
    } \
}

#define NUMBER_BINARY( slot ) \
extern "C" PyObject *number_ ## slot ## _handler( PyObject *self, PyObject *other ) \
{ \
    try \
    { \
        PythonExtensionBase *p = getPythonExtensionBase( self ); \
        return new_reference_to( p->number_ ## slot( Object( other ) ) ); \
    } \
    catch( BaseException & ) \
    { \
        return NULL; /* indicates error */ \
    } \
}
#define NUMBER_TERNARY( slot ) \
extern "C" PyObject *number_ ## slot ## _handler( PyObject *self, PyObject *other1, PyObject *other2 ) \
{ \
    try \
    { \
        PythonExtensionBase *p = getPythonExtensionBase( self ); \
        return new_reference_to( p->number_ ## slot( Object( other1 ), Object( other2 ) ) ); \
    } \
    catch( BaseException & ) \
    { \
        return NULL; /* indicates error */ \
    } \
}

NUMBER_UNARY( negative )
NUMBER_UNARY( positive )
NUMBER_UNARY( absolute )
NUMBER_UNARY( invert )
NUMBER_UNARY( int )
NUMBER_UNARY( float )
NUMBER_BINARY( add )
NUMBER_BINARY( subtract )
NUMBER_BINARY( multiply )
NUMBER_BINARY( remainder )
NUMBER_BINARY( divmod )
NUMBER_BINARY( lshift )
NUMBER_BINARY( rshift )
NUMBER_BINARY( and )
NUMBER_BINARY( xor )
NUMBER_BINARY( or )
NUMBER_TERNARY( power )
NUMBER_BINARY( floor_divide )
NUMBER_BINARY( true_divide )
NUMBER_UNARY( index )
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
NUMBER_BINARY( matrix_multiply )
#endif
NUMBER_BINARY( inplace_add )
NUMBER_BINARY( inplace_subtract )
NUMBER_BINARY( inplace_multiply )
NUMBER_BINARY( inplace_remainder )
NUMBER_TERNARY( inplace_power )
NUMBER_BINARY( inplace_lshift )
NUMBER_BINARY( inplace_rshift )
NUMBER_BINARY( inplace_and )
NUMBER_BINARY( inplace_xor )
NUMBER_BINARY( inplace_or )
NUMBER_BINARY( inplace_floor_divide )
NUMBER_BINARY( inplace_true_divide )
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
NUMBER_BINARY( inplace_matrix_multiply )
#endif

#undef NUMBER_UNARY
#undef NUMBER_BINARY
#undef NUMBER_TERNARY

// Buffer
#ifndef Py_LIMITED_API
extern "C" int buffer_get_handler( PyObject *self, Py_buffer *buf, int flags )
{
    try
    {
        PythonExtensionBase *p = getPythonExtensionBase( self );
        return p->buffer_get( buf, flags );
    }
    catch( BaseException & )
    {
        return -1;    // indicate error
    }
}

extern "C" void buffer_release_handler( PyObject *self, Py_buffer *buf )
{
    PythonExtensionBase *p = getPythonExtensionBase( self );
    p->buffer_release( buf );
    // NOTE: No way to indicate error to Python
}
#endif

//================================================================================
//
//    Implementation of PythonExtensionBase
//
//================================================================================
#define missing_method( method ) \
    throw RuntimeError( "Extension object missing implement of " #method );

PythonExtensionBase::PythonExtensionBase()
{
    ob_refcnt = 0;
}

PythonExtensionBase::~PythonExtensionBase()
{
    assert( ob_refcnt == 0 );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name )
{
    TupleN args;
    return  self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1 )
{
    TupleN args( arg1 );
    return  self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1, const Object &arg2 )
{
    TupleN args( arg1, arg2 );
    return self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1, const Object &arg2, const Object &arg3 )
{
    TupleN args( arg1, arg2, arg3 );
    return self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1, const Object &arg2, const Object &arg3,
                                            const Object &arg4 )
{
    TupleN args( arg1, arg2, arg3, arg4 );
    return self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1, const Object &arg2, const Object &arg3,
                                            const Object &arg4, const Object &arg5 )
{
    TupleN args( arg1, arg2, arg3, arg4, arg5 );
    return self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1, const Object &arg2, const Object &arg3,
                                            const Object &arg4, const Object &arg5, const Object &arg6 )
{
    TupleN args( arg1, arg2, arg3, arg4, arg5, arg6 );
    return self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1, const Object &arg2, const Object &arg3,
                                            const Object &arg4, const Object &arg5, const Object &arg6,
                                            const Object &arg7 )
{
    TupleN args( arg1, arg2, arg3, arg4, arg5, arg6, arg7 );
    return self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1, const Object &arg2, const Object &arg3,
                                            const Object &arg4, const Object &arg5, const Object &arg6,
                                            const Object &arg7, const Object &arg8 )
{
    TupleN args( arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 );
    return self().callMemberFunction( fn_name, args );
}

Object PythonExtensionBase::callOnSelf( const std::string &fn_name,
                                            const Object &arg1, const Object &arg2, const Object &arg3,
                                            const Object &arg4, const Object &arg5, const Object &arg6,
                                            const Object &arg7, const Object &arg8, const Object &arg9 )
{
    TupleN args( arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 );
    return self().callMemberFunction( fn_name, args );
}

void PythonExtensionBase::reinit( Tuple & /* args */, Dict & /* kwds */)
{
    throw RuntimeError( "Must not call __init__ twice on this class" );
}


Object PythonExtensionBase::genericGetAttro( const String &name )
{
    return asObject( PyObject_GenericGetAttr( selfPtr(), name.ptr() ) );
}

int PythonExtensionBase::genericSetAttro( const String &name, const Object &value )
{
    return PyObject_GenericSetAttr( selfPtr(), name.ptr(), value.ptr() );
}

#if defined( PYCXX_PYTHON_2TO3 ) && !defined( Py_LIMITED_API ) && PY_MINOR_VERSION <= 7
int PythonExtensionBase::print( FILE *, int )
{
    missing_method( print );
}
#endif

Object PythonExtensionBase::getattr( const char * )
{
    missing_method( getattr );
}

int PythonExtensionBase::setattr( const char *, const Object & )
{
    missing_method( setattr );
}

Object PythonExtensionBase::getattro( const String &name )
{
    return asObject( PyObject_GenericGetAttr( selfPtr(), name.ptr() ) );
}

int PythonExtensionBase::setattro( const String &name, const Object &value )
{
    return PyObject_GenericSetAttr( selfPtr(), name.ptr(), value.ptr() );
}


int PythonExtensionBase::compare( const Object & )
{
    missing_method( compare );
}

Object PythonExtensionBase::rich_compare( const Object &, int )
{
    missing_method( rich_compare );
}

Object PythonExtensionBase::repr()
{
    missing_method( repr );
}

Object PythonExtensionBase::str()
{
    missing_method( str );
}

long PythonExtensionBase::hash()
{
    missing_method( hash );
}

Object PythonExtensionBase::call( const Object &, const Object & )
{
    missing_method( call );
}

Object PythonExtensionBase::iter()
{
    missing_method( iter );
}

PyObject *PythonExtensionBase::iternext()
{
    missing_method( iternext );
}

// Sequence methods
PyCxx_ssize_t PythonExtensionBase::sequence_length()
{
    missing_method( sequence_length );
}

Object PythonExtensionBase::sequence_concat( const Object & )
{
    missing_method( sequence_concat );
}

Object PythonExtensionBase::sequence_repeat( Py_ssize_t )
{
    missing_method( sequence_repeat );
}

Object PythonExtensionBase::sequence_item( Py_ssize_t )
{
    missing_method( sequence_item );
}

int PythonExtensionBase::sequence_ass_item( Py_ssize_t, const Object & )
{
    missing_method( sequence_ass_item );
}

Object PythonExtensionBase::sequence_inplace_concat( const Object & )
{
    missing_method( sequence_inplace_concat );
}

Object PythonExtensionBase::sequence_inplace_repeat( Py_ssize_t )
{
    missing_method( sequence_inplace_repeat );
}

int PythonExtensionBase::sequence_contains( const Object & )
{
    missing_method( sequence_contains );
}

// Mapping
PyCxx_ssize_t PythonExtensionBase::mapping_length()
{
    missing_method( mapping_length );
}

Object PythonExtensionBase::mapping_subscript( const Object & )
{
    missing_method( mapping_subscript );
}

int PythonExtensionBase::mapping_ass_subscript( const Object &, const Object & )
{
    missing_method( mapping_ass_subscript );
}

// Number
#define NUMBER_UNARY( slot ) Object PythonExtensionBase::number_ ## slot() \
    { missing_method( number_ ## slot ); }
#define NUMBER_BINARY( slot ) Object PythonExtensionBase::number_ ## slot( const Object & ) \
    { missing_method( number_ ## slot ); }
#define NUMBER_TERNARY( slot ) Object PythonExtensionBase::number_ ## slot( const Object &, const Object & ) \
    { missing_method( number_ ## slot ); }

NUMBER_UNARY( negative )
NUMBER_UNARY( positive )
NUMBER_UNARY( absolute )
NUMBER_UNARY( invert )
NUMBER_UNARY( int )
NUMBER_UNARY( float )
NUMBER_BINARY( add )
NUMBER_BINARY( subtract )
NUMBER_BINARY( multiply )
NUMBER_BINARY( remainder )
NUMBER_BINARY( divmod )
NUMBER_BINARY( lshift )
NUMBER_BINARY( rshift )
NUMBER_BINARY( and )
NUMBER_BINARY( xor )
NUMBER_BINARY( or )
NUMBER_TERNARY( power )
NUMBER_BINARY( floor_divide )
NUMBER_BINARY( true_divide )
NUMBER_UNARY( index )
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
NUMBER_BINARY( matrix_multiply )
#endif

NUMBER_BINARY( inplace_add )
NUMBER_BINARY( inplace_subtract )
NUMBER_BINARY( inplace_multiply )
NUMBER_BINARY( inplace_remainder )
NUMBER_TERNARY( inplace_power )
NUMBER_BINARY( inplace_lshift )
NUMBER_BINARY( inplace_rshift )
NUMBER_BINARY( inplace_and )
NUMBER_BINARY( inplace_xor )
NUMBER_BINARY( inplace_or )
NUMBER_BINARY( inplace_floor_divide )
NUMBER_BINARY( inplace_true_divide )
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
NUMBER_BINARY( inplace_matrix_multiply )
#endif

#undef NUMBER_UNARY
#undef NUMBER_BINARY
#undef NUMBER_TERNARY


// Buffer
#ifndef Py_LIMITED_API
int PythonExtensionBase::buffer_get( Py_buffer * /*buf*/, int /*flags*/ )
{
    missing_method( buffer_get );
}

int PythonExtensionBase::buffer_release( Py_buffer * /*buf*/ )
{
    // This method is optional and only required if the buffer's
    // memory is dynamic.
    return 0;
}
#endif

//--------------------------------------------------------------------------------
//
//    Method call handlers for
//        PythonExtensionBase
//        ExtensionModuleBase
//
//--------------------------------------------------------------------------------
// Note: Python calls noargs as varargs buts args==NULL
extern "C" PyObject *method_noargs_call_handler( PyObject *_self_and_name_tuple, PyObject * )
{
    try
    {
        Tuple self_and_name_tuple( _self_and_name_tuple );

        PyObject *self_in_cobject = self_and_name_tuple[0].ptr();
        void *self_as_void = PyCapsule_GetPointer( self_in_cobject, NULL );
        if( self_as_void == NULL )
            return NULL;

        ExtensionModuleBase *self = static_cast<ExtensionModuleBase *>( self_as_void );

        Object result( self->invoke_method_noargs( PyCapsule_GetPointer( self_and_name_tuple[1].ptr(), NULL ) ) );

        return new_reference_to( result.ptr() );
    }
    catch( BaseException & )
    {
        return 0;
    }
}

extern "C" PyObject *method_varargs_call_handler( PyObject *_self_and_name_tuple, PyObject *_args )
{
    try
    {
        Tuple self_and_name_tuple( _self_and_name_tuple );

        PyObject *self_in_cobject = self_and_name_tuple[0].ptr();
        void *self_as_void = PyCapsule_GetPointer( self_in_cobject, NULL );
        if( self_as_void == NULL )
            return NULL;

        ExtensionModuleBase *self = static_cast<ExtensionModuleBase *>( self_as_void );
        Tuple args( _args );
        Object result
                (
                self->invoke_method_varargs
                    (
                    PyCapsule_GetPointer( self_and_name_tuple[1].ptr(), NULL ),
                    args
                    )
                );

        return new_reference_to( result.ptr() );
    }
    catch( BaseException & )
    {
        return 0;
    }
}

extern "C" PyObject *method_keyword_call_handler( PyObject *_self_and_name_tuple, PyObject *_args, PyObject *_keywords )
{
    try
    {
        Tuple self_and_name_tuple( _self_and_name_tuple );

        PyObject *self_in_cobject = self_and_name_tuple[0].ptr();
        void *self_as_void = PyCapsule_GetPointer( self_in_cobject, NULL );
        if( self_as_void == NULL )
            return NULL;

        ExtensionModuleBase *self = static_cast<ExtensionModuleBase *>( self_as_void );

        Tuple args( _args );

        if( _keywords == NULL )
        {
            Dict keywords;    // pass an empty dict

            Object result
                (
                self->invoke_method_keyword
                    (
                    PyCapsule_GetPointer( self_and_name_tuple[1].ptr(), NULL ),
                    args,
                    keywords
                    )
                );

            return new_reference_to( result.ptr() );
        }
        else
        {
            Dict keywords( _keywords ); // make dict

            Object result
                    (
                    self->invoke_method_keyword
                        (
                        PyCapsule_GetPointer( self_and_name_tuple[1].ptr(), NULL ),
                        args,
                        keywords
                        )
                    );

            return new_reference_to( result.ptr() );
        }
    }
    catch( BaseException & )
    {
        return 0;
    }
}


//--------------------------------------------------------------------------------
//
//    ExtensionExceptionType
//
//--------------------------------------------------------------------------------
ExtensionExceptionType::ExtensionExceptionType()
: Object()
{
}

void ExtensionExceptionType::init( ExtensionModuleBase &module, const std::string& name )
{
    std::string module_name( module.fullName() );
    module_name += ".";
    module_name += name;

    set( PyErr_NewException( const_cast<char *>( module_name.c_str() ), NULL, NULL ), true );
}

void ExtensionExceptionType::init( ExtensionModuleBase &module, const std::string& name, ExtensionExceptionType &parent )
 {
     std::string module_name( module.fullName() );
     module_name += ".";
     module_name += name;

    set( PyErr_NewException( const_cast<char *>( module_name.c_str() ), parent.ptr(), NULL ), true );
}

ExtensionExceptionType::~ExtensionExceptionType()
{
}

BaseException::BaseException( ExtensionExceptionType &exception, const std::string& reason )
{
    PyErr_SetString( exception.ptr(), reason.c_str() );
}

BaseException::BaseException( ExtensionExceptionType &exception, Object &reason )
{
    PyErr_SetObject( exception.ptr(), reason.ptr() );
}

BaseException::BaseException( PyObject *exception, Object &reason )
{
    PyErr_SetObject( exception, reason.ptr() );
}

BaseException::BaseException( PyObject *exception, const std::string &reason )
{
    PyErr_SetString( exception, reason.c_str() );
}

BaseException::BaseException()
{
}

void BaseException::clear()
{
    PyErr_Clear();
}

// is the exception this specific exception 'exc'
bool BaseException::matches( ExtensionExceptionType &exc )
{
    return PyErr_ExceptionMatches( exc.ptr() ) != 0;
}

Object BaseException::errorType()
{
    PyObject *type, *value, *traceback;
    PyErr_Fetch( &type, &value, &traceback );

    Object result( type );

    PyErr_Restore( type, value, traceback );
    return result;
}

Object BaseException::errorValue()
{
    PyObject *type, *value, *traceback;
    PyErr_Fetch( &type, &value, &traceback );

    Object result( value );

    PyErr_Restore( type, value, traceback );
    return result;
}


//------------------------------------------------------------

#if 1
//------------------------------------------------------------
// compare operators
bool operator!=( const Long &a, const Long &b )
{
    return a.as_long() != b.as_long();
}

bool operator!=( const Long &a, int b )
{
    return a.as_long() != b;
}

bool operator!=( const Long &a, long b )
{
    return a.as_long() != b;
}

bool operator!=( int a, const Long &b )
{
    return a != b.as_long();
}

bool operator!=( long a, const Long &b )
{
    return a != b.as_long();
}

//------------------------------
bool operator==( const Long &a, const Long &b )
{
    return a.as_long() == b.as_long();
}

bool operator==( const Long &a, int b )
{
    return a.as_long() == b;
}

bool operator==( const Long &a, long b )
{
    return a.as_long() == b;
}

bool operator==( int a, const Long &b )
{
    return a == b.as_long();
}

bool operator==( long a, const Long &b )
{
    return a == b.as_long();
}

//------------------------------
bool operator>( const Long &a, const Long &b )
{
    return a.as_long() > b.as_long();
}

bool operator>( const Long &a, int b )
{
    return a.as_long() > b;
}

bool operator>( const Long &a, long b )
{
    return a.as_long() > b;
}

bool operator>( int a, const Long &b )
{
    return a > b.as_long();
}

bool operator>( long a, const Long &b )
{
    return a > b.as_long();
}

//------------------------------
bool operator>=( const Long &a, const Long &b )
{
    return a.as_long() >= b.as_long();
}

bool operator>=( const Long &a, int b )
{
    return a.as_long() >= b;
}

bool operator>=( const Long &a, long b )
{
    return a.as_long() >= b;
}

bool operator>=( int a, const Long &b )
{
    return a >= b.as_long();
}

bool operator>=( long a, const Long &b )
{
    return a >= b.as_long();
}

//------------------------------
bool operator<( const Long &a, const Long &b )
{
    return a.as_long() < b.as_long();
}

bool operator<( const Long &a, int b )
{
    return a.as_long() < b;
}

bool operator<( const Long &a, long b )
{
    return a.as_long() < b;
}

bool operator<( int a, const Long &b )
{
    return a < b.as_long();
}

bool operator<( long a, const Long &b )
{
    return a < b.as_long();
}

//------------------------------
bool operator<=( const Long &a, const Long &b )
{
    return a.as_long() <= b.as_long();
}

bool operator<=( int a, const Long &b )
{
    return a <= b.as_long();
}

bool operator<=( long a, const Long &b )
{
    return a <= b.as_long();
}

bool operator<=( const Long &a, int b )
{
    return a.as_long() <= b;
}

bool operator<=( const Long &a, long b )
{
    return a.as_long() <= b;
}

#ifdef HAVE_LONG_LONG
//------------------------------
bool operator!=( const Long &a, PY_LONG_LONG b )
{
    return a.as_long_long() != b;
}

bool operator!=( PY_LONG_LONG a, const Long &b )
{
    return a != b.as_long_long();
}

//------------------------------
bool operator==( const Long &a, PY_LONG_LONG b )
{
    return a.as_long_long() == b;
}

bool operator==( PY_LONG_LONG a, const Long &b )
{
    return a == b.as_long_long();
}

//------------------------------
bool operator>( const Long &a, PY_LONG_LONG b )
{
    return a.as_long_long() > b;
}

bool operator>( PY_LONG_LONG a, const Long &b )
{
    return a > b.as_long_long();
}

//------------------------------
bool operator>=( const Long &a, PY_LONG_LONG b )
{
    return a.as_long_long() >= b;
}

bool operator>=( PY_LONG_LONG a, const Long &b )
{
    return a >= b.as_long_long();
}

//------------------------------
bool operator<( const Long &a, PY_LONG_LONG b )
{
    return a.as_long_long() < b;
}

bool operator<( PY_LONG_LONG a, const Long &b )
{
    return a < b.as_long_long();
}

//------------------------------
bool operator<=( const Long &a, PY_LONG_LONG b )
{
    return a.as_long_long() <= b;
}

bool operator<=( PY_LONG_LONG a, const Long &b )
{
    return a <= b.as_long_long();
}
#endif
#endif

//------------------------------------------------------------
// compare operators
bool operator!=( const Float &a, const Float &b )
{
    return a.as_double() != b.as_double();
}

bool operator!=( const Float &a, double b )
{
    return a.as_double() != b;
}

bool operator!=( double a, const Float &b )
{
    return a != b.as_double();
}

//------------------------------
bool operator==( const Float &a, const Float &b )
{
    return a.as_double() == b.as_double();
}

bool operator==( const Float &a, double b )
{
    return a.as_double() == b;
}

bool operator==( double a, const Float &b )
{
    return a == b.as_double();
}

//------------------------------
bool operator>( const Float &a, const Float &b )
{
    return a.as_double() > b.as_double();
}

bool operator>( const Float &a, double b )
{
    return a.as_double() > b;
}

bool operator>( double a, const Float &b )
{
    return a > b.as_double();
}

//------------------------------
bool operator>=( const Float &a, const Float &b )
{
    return a.as_double() >= b.as_double();
}

bool operator>=( const Float &a, double b )
{
    return a.as_double() >= b;
}

bool operator>=( double a, const Float &b )
{
    return a >= b.as_double();
}

//------------------------------
bool operator<( const Float &a, const Float &b )
{
    return a.as_double() < b.as_double();
}

bool operator<( const Float &a, double b )
{
    return a.as_double() < b;
}

bool operator<( double a, const Float &b )
{
    return a < b.as_double();
}

//------------------------------
bool operator<=( const Float &a, const Float &b )
{
    return a.as_double() <= b.as_double();
}

bool operator<=( double a, const Float &b )
{
    return a <= b.as_double();
}

bool operator<=( const Float &a, double b )
{
    return a.as_double() <= b;
}

}    // end of namespace Py
