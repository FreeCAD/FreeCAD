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

namespace Py
{
    class PythonType
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
#ifdef PYCXX_PYTHON_2TO3
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
        PythonType &supportIter( void );

        PythonType &supportSequenceType( void );
        PythonType &supportMappingType( void );
        PythonType &supportNumberType( void );
        PythonType &supportBufferType( void );

        PythonType &set_tp_dealloc( void (*tp_dealloc)( PyObject * ) );
        PythonType &set_tp_init( int (*tp_init)( PyObject *self, PyObject *args, PyObject *kwds ) );
        PythonType &set_tp_new( PyObject *(*tp_new)( PyTypeObject *subtype, PyObject *args, PyObject *kwds ) );
        PythonType &set_methods( PyMethodDef *methods );

        // call once all support functions have been called to ready the type
        bool readyType();

    protected:
        void init_sequence();
        void init_mapping();
        void init_number();
        void init_buffer();

        PyTypeObject            *table;
        PySequenceMethods       *sequence_table;
        PyMappingMethods        *mapping_table;
        PyNumberMethods         *number_table;
        PyBufferProcs           *buffer_table;

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
