/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include <App/DocumentPy.h>
#include <App/DocumentObjectPy.h>
#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include "DocumentThread.h"
#include "DocumentProtector.h"
#include "DocumentProtectorPy.h"

namespace Sandbox {
class PythonBaseClass : public Py::PythonClass< PythonBaseClass >
{
public:
    PythonBaseClass( Py::PythonClassInstance *self, Py::Tuple &args, Py::Dict &kwds )
    : Py::PythonClass< PythonBaseClass >::PythonClass( self, args, kwds )
    , m_value( "default value" )
    {
        std::cout << "PythonBaseClass c'tor Called with " << args.length() << " normal arguments." << std::endl;
        Py::List names( kwds.keys() );
        std::cout << "and with " << names.length() << " keyword arguments:" << std::endl;
        for( Py::List::size_type i=0; i< names.length(); i++ )
        {
            Py::String name( names[i] );
            std::cout << "    " << name << std::endl;
        }
        m_array.push_back(Py::Long(2));
        m_array.push_back(Py::Float(3.0));
        m_array.push_back(Py::String("4.0"));
    }

    virtual ~PythonBaseClass()
    {
        std::cout << "~PythonBaseClass." << std::endl;
    }

    static void init_type(void)
    {
        behaviors().name( "PythonBaseClass" );
        behaviors().doc( "documentation for PythonBaseClass class" );
        behaviors().supportGetattro();
        behaviors().supportSetattro();
        behaviors().supportSequenceType();

        PYCXX_ADD_NOARGS_METHOD( func_noargs, PythonBaseClass_func_noargs, "docs for PythonBaseClass_func_noargs" );
        PYCXX_ADD_VARARGS_METHOD( func_varargs, PythonBaseClass_func_varargs, "docs for PythonBaseClass_func_varargs" );
        PYCXX_ADD_KEYWORDS_METHOD( func_keyword, PythonBaseClass_func_keyword, "docs for PythonBaseClass_func_keyword" );

        PYCXX_ADD_NOARGS_METHOD( func_noargs_raise_exception, PythonBaseClass_func_noargs_raise_exception,
          "docs for PythonBaseClass_func_noargs_raise_exception" );

        // Call to make the type ready for use
        behaviors().readyType();
    }

    Py::Object PythonBaseClass_func_noargs( void )
    {
        std::cout << "PythonBaseClass_func_noargs Called." << std::endl;
        std::cout << "value ref count " << m_value.reference_count() << std::endl;
        return Py::None();
    }
    PYCXX_NOARGS_METHOD_DECL( PythonBaseClass, PythonBaseClass_func_noargs )

    Py::Object PythonBaseClass_func_varargs( const Py::Tuple &args )
    {
        std::cout << "PythonBaseClass_func_varargs Called with " << args.length() << " normal arguments." << std::endl;
        return Py::None();
    }
    PYCXX_VARARGS_METHOD_DECL( PythonBaseClass, PythonBaseClass_func_varargs )

    Py::Object PythonBaseClass_func_keyword( const Py::Tuple &args, const Py::Dict &kwds )
    {
        std::cout << "PythonBaseClass_func_keyword Called with " << args.length() << " normal arguments." << std::endl;
        Py::List names( kwds.keys() );
        std::cout << "and with " << names.length() << " keyword arguments:" << std::endl;
        for( Py::List::size_type i=0; i< names.length(); i++ )
        {
            Py::String name( names[i] );
            std::cout << "    " << name << std::endl;
        }
        return Py::None();
    }
    PYCXX_KEYWORDS_METHOD_DECL( PythonBaseClass, PythonBaseClass_func_keyword )

    Py::Object PythonBaseClass_func_noargs_raise_exception( void )
    {
        std::cout << "PythonBaseClass_func_noargs_raise_exception Called." << std::endl;
        throw Py::RuntimeError( "its an error" );
        return Py::None();
    }
    PYCXX_NOARGS_METHOD_DECL( PythonBaseClass, PythonBaseClass_func_noargs_raise_exception )

    Py::Object getattro( const Py::String &name_ )
    {
        std::string name( name_.as_std_string( "utf-8" ) );

        if( name == "value" )
        {
            return m_value;
        }
        else
        {
            return genericGetAttro( name_ );
        }
    }

    int setattro( const Py::String &name_, const Py::Object &value )
    {
        std::string name( name_.as_std_string( "utf-8" ) );

        if( name == "value" )
        {
            m_value = value;
            return 0;
        }
        else
        {
            return genericSetAttro( name_, value );
        }
    }
    virtual PyCxx_ssize_t sequence_length()
    {
        // len(x)
        return m_array.size();
    }
    virtual Py::Object sequence_concat(const Py::Object &)
    {
        // x + y
        throw Py::NotImplementedError("not yet implemented");
    }
    virtual Py::Object sequence_repeat(Py_ssize_t)
    {
        // x * 3
        throw Py::NotImplementedError("not yet implemented");
    }
    virtual Py::Object sequence_item(Py_ssize_t i)
    {
        // x[0]
        if (i >= static_cast<Py_ssize_t>(m_array.size()))
            throw Py::IndexError("index out of range");
        return m_array[i];
    }
    virtual Py::Object sequence_slice(Py_ssize_t, Py_ssize_t)
    {
        // x[0:3]
        throw Py::NotImplementedError("not yet implemented");
    }
    virtual int sequence_ass_item(Py_ssize_t i, const Py::Object & o)
    {
        // x[0] = y
        if (i >= static_cast<Py_ssize_t>(m_array.size()))
            throw Py::IndexError("index out of range");
        m_array[i] = o;
        return 0;
    }
    virtual int sequence_ass_slice(Py_ssize_t, Py_ssize_t, const Py::Object &)
    {
        // x[0:3] = y
        throw Py::NotImplementedError("not yet implemented");
    }

    Py::String m_value;
    std::vector<Py::Object> m_array;
};

/* module functions */

class Module : public Py::ExtensionModule<Module>
{

public:
    Module() : Py::ExtensionModule<Module>("Sandbox")
    {
        Sandbox::PythonBaseClass::init_type();
        Sandbox::DocumentProtectorPy::init_type();
        add_varargs_method("DocumentProtector",
            &Module::new_DocumentProtector,
            "DocumentProtector(Document)");
        Sandbox::DocumentObjectProtectorPy::init_type();
        add_varargs_method("DocumentObjectProtector",
            &Module::new_DocumentObjectProtector,
            "DocumentObjectProtector(DocumentObject)");
        initialize("This module is the Sandbox module"); // register with Python
        
        Py::Dict d( moduleDictionary() );
        Py::Object x( Sandbox::PythonBaseClass::type() );
        d["PythonBaseClass"] = x;
    }
    
    virtual ~Module() {}

private:
    Py::Object new_DocumentProtector(const Py::Tuple& args)
    {
        PyObject* o;
        if (!PyArg_ParseTuple(args.ptr(), "O!",&(App::DocumentPy::Type), &o))
            throw Py::Exception();
        App::DocumentPy* doc = static_cast<App::DocumentPy*>(o);
        return Py::asObject(new Sandbox::DocumentProtectorPy(doc));
    }
    Py::Object new_DocumentObjectProtector(const Py::Tuple& args)
    {
        PyObject* o;
        if (!PyArg_ParseTuple(args.ptr(), "O!",&(App::DocumentObjectPy::Type), &o))
            throw Py::Exception();
        App::DocumentObjectPy* obj = static_cast<App::DocumentObjectPy*>(o);
        return Py::asObject(new Sandbox::DocumentObjectProtectorPy(obj));
    }
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace Sandbox


/* Python entry */
PyMOD_INIT_FUNC(Sandbox)
{
    Sandbox::DocumentProtector  ::init();
    Sandbox::SandboxObject      ::init();

    // the following constructor call registers our extension module
    // with the Python runtime system
    PyObject* mod = Sandbox::initModule();
    Base::Console().Log("Loading Sandbox module... done\n");
    PyMOD_Return(mod);
}
