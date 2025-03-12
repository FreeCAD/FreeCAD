//
//  cxx_exceptions.cxx
//
#include <CXX/Exception.hxx>
#include <CXX/Extensions.hxx>

#include <map>

namespace Py
{
typedef void (*throw_exception_func_t)( void );

std::map<void *, throw_exception_func_t> py_exc_type_to_exc_func;

void addPythonException( ExtensionExceptionType &py_exc_type, throw_exception_func_t func )
{
    py_exc_type_to_exc_func.insert( std::make_pair( py_exc_type.ptr(), func ) );
}

void addPythonException( PyObject *py_exc_type, throw_exception_func_t func )
{
    py_exc_type_to_exc_func.insert( std::make_pair( py_exc_type, func ) );
}

void ifPyErrorThrowCxxException()
{
    if( PyErr_Occurred() )
    {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch( &ptype, &pvalue, &ptrace );
        PyErr_Restore( ptype, pvalue, ptrace );

        Object q( ptype );

        std::map<void *, throw_exception_func_t>::iterator func = py_exc_type_to_exc_func.find( ptype );
        if( func != py_exc_type_to_exc_func.end() )
        {
#ifdef PYCXX_DEBUG
            std::cout << "ifPyErrorThrowCxxException found throwFunc: " << q << std::endl;
#endif
            (func->second)();
        }
        else
        {
#ifdef PYCXX_DEBUG
            std::cout << "ifPyErrorThrowCxxException no throwFunc: " << q << std::endl;
#endif
            throw Exception();
        }
    }
}

void initExceptions()
{
    static bool init_done = false;
    if( init_done )
    {
        return;
    }

#define PYCXX_STANDARD_EXCEPTION( eclass, bclass ) \
    addPythonException( eclass::exceptionType(), eclass::throwFunc );

#include <CXX/Python3/cxx_standard_exceptions.hxx>

#undef PYCXX_STANDARD_EXCEPTION

    init_done = true;
}


} // end of namespace Py
