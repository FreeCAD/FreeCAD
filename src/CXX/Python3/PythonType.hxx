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

#ifndef __CXX_PythonType__h
#define __CXX_PythonType__h

#if defined( Py_LIMITED_API )
#include <unordered_map>
#endif

namespace Py
{
    class PYCXX_EXPORT PythonType
    {
    public:
        // if you define one sequence method you must define
        // all of them except the assigns

        PythonType( size_t base_size, int itemsize, const char *default_name );
        virtual ~PythonType();

        const char *getName() const;
        const char *getDoc() const;

        PyTypeObject *type_object() const;
        PythonType &name( const char *nam );
        PythonType &doc( const char *d );

        PythonType &supportClass( void );
#if defined( PYCXX_PYTHON_2TO3 ) && !defined( Py_LIMITED_API ) && PY_MINOR_VERSION <= 7
        PythonType &supportPrint( void );
#endif
        PythonType &supportGetattr( void );
        PythonType &supportSetattr( void );
        PythonType &supportGetattro( void );
        PythonType &supportSetattro( void );
#ifdef PYCXX_PYTHON_2TO3
        PythonType &supportCompare( void );
#endif
        PythonType &supportRichCompare( void );
        PythonType &supportRepr( void );
        PythonType &supportStr( void );
        PythonType &supportHash( void );
        PythonType &supportCall( void );

#define B( n ) (1<<(n))
        enum {
            support_iter_iter =                 B(0),
            support_iter_iternext =             B(1)
        };
        PythonType &supportIter( int methods_to_support=
                        support_iter_iter |
                        support_iter_iternext );

        enum {
            support_sequence_length =           B(0),
            support_sequence_repeat =           B(1),
            support_sequence_item =             B(2),
            support_sequence_slice =            B(3),
            support_sequence_concat =           B(4),
            support_sequence_ass_item =         B(5),
            support_sequence_ass_slice =        B(6),
            support_sequence_inplace_concat =   B(7),
            support_sequence_inplace_repeat =   B(8),
            support_sequence_contains =         B(9)
        };
        PythonType &supportSequenceType( int methods_to_support=
                        support_sequence_length |
                        support_sequence_repeat |
                        support_sequence_item |
                        support_sequence_slice |
                        support_sequence_concat
                        );

        enum {
            support_mapping_length =            B(0),
            support_mapping_subscript =         B(1),
            support_mapping_ass_subscript =     B(2)
        };
        PythonType &supportMappingType( int methods_to_support=
                        support_mapping_length |
                        support_mapping_subscript
                        );

        enum {
            support_number_add =                B(0),
            support_number_subtract =           B(1),
            support_number_multiply =           B(2),
            support_number_remainder =          B(3),
            support_number_divmod =             B(4),
            support_number_power =              B(5),
            support_number_negative =           B(6),
            support_number_positive =           B(7),
            support_number_absolute =           B(8),
            support_number_invert =             B(9),
            support_number_lshift =             B(10),
            support_number_rshift =             B(11),
            support_number_and =                B(12),
            support_number_xor =                B(13),
            support_number_or =                 B(14),
            support_number_int =                B(15),
            support_number_float =              B(16),
            support_number_floor_divide =       B(17),
            support_number_true_divide =        B(18),
            support_number_index =              B(19),
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
            support_number_matrix_multiply =    B(20),
#endif

            // start a new bit mask for inplace that avoid using more then 32 bits in methods_to_support
            support_number_inplace_floor_divide = B(0),
            support_number_inplace_true_divide = B(1),
            support_number_inplace_add =        B(2),
            support_number_inplace_subtract =   B(3),
            support_number_inplace_multiply =   B(4),
            support_number_inplace_remainder =  B(5),
            support_number_inplace_power =      B(6),
            support_number_inplace_lshift =     B(7),
            support_number_inplace_rshift =     B(8),
            support_number_inplace_and =        B(9),
            support_number_inplace_xor =        B(10),
            support_number_inplace_or =         B(11)
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5
            ,
            support_number_inplace_matrix_multiply = B(12)
#endif
        };
        PythonType &supportNumberType(
            int methods_to_support=
                support_number_add |
                support_number_subtract |
                support_number_multiply |
                support_number_remainder |
                support_number_divmod |
                support_number_power |
                support_number_negative |
                support_number_positive |
                support_number_absolute |
                support_number_invert |
                support_number_lshift |
                support_number_rshift |
                support_number_and |
                support_number_xor |
                support_number_or |
                support_number_int |
                support_number_float,
            int inplace_methods_to_support=0
            );

#if !defined( Py_LIMITED_API )
        enum {
            support_buffer_getbuffer =          B(0),
            support_buffer_releasebuffer =      B(1)
        };
        PythonType &supportBufferType( int methods_to_support=
                    support_buffer_getbuffer |
                    support_buffer_releasebuffer
                    );
#endif
#undef B

        PythonType &set_tp_dealloc( void (*tp_dealloc)( PyObject * ) );
        PythonType &set_tp_init( int (*tp_init)( PyObject *self, PyObject *args, PyObject *kwds ) );
        PythonType &set_tp_new( PyObject *(*tp_new)( PyTypeObject *subtype, PyObject *args, PyObject *kwds ) );
        PythonType &set_methods( PyMethodDef *methods );

        // call once all support functions have been called to ready the type
        bool readyType();

    protected:
#if defined( Py_LIMITED_API )
        std::unordered_map<int, void*>  slots;
        PyType_Spec                     *spec;
        PyTypeObject                    *tp_object;
#else
        PyTypeObject            *table;
        PySequenceMethods       *sequence_table;
        PyMappingMethods        *mapping_table;
        PyNumberMethods         *number_table;
        PyBufferProcs           *buffer_table;
#endif

    private:
        //
        // prevent the compiler generating these unwanted functions
        //
        PythonType( const PythonType &tb );     // unimplemented
        void operator=( const PythonType &t );  // unimplemented

    };

} // Namespace Py

// End of __CXX_PythonType__h
#endif
