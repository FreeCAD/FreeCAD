/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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

#ifndef BASE_PYOBJECTBASE_H
#define BASE_PYOBJECTBASE_H

// Std. configurations

// (re-)defined in pyconfig.h
#if defined (_POSIX_C_SOURCE)
#   undef    _POSIX_C_SOURCE
#endif
#if defined (_XOPEN_SOURCE)
#   undef    _XOPEN_SOURCE
#endif

// needed header
#undef slots
#include <Python.h>
#ifdef FC_OS_MACOSX
#undef toupper
#undef tolower
#undef isupper
#undef islower
#undef isspace
#undef isalpha
#undef isalnum
#endif
#define slots
#include <iostream>
#include <bitset>

#include <typeinfo>
#include "Exception.h"
#if PY_MAJOR_VERSION > 2
#  ifndef PYCXX_PYTHON_2TO3
#  define PYCXX_PYTHON_2TO3
#  endif
#endif
#include <CXX/Objects.hxx>



/** Python static class macro for definition
 * sets up a static function entry in a class inheriting 
 * from PyObjectBase. Its a pure convenience macro. You can also do
 * it by hand if you want. It looks like that:
 * \code
 * static PyObject* X (PyObject *self,PyObject *args,PyObject *kwd);
 * \endcode
 * @param SFUNC is the static method name (use what you want)
 * @see PYFUNCIMP_S
 * @see FCPythonExport
 */
#define PYFUNCDEF_S(SFUNC)   static PyObject* SFUNC (PyObject *self,PyObject *args,PyObject *kwd)


/** Python static class macro for implementation
 * used to set up a implementation for PYFUNCDEF_S definition.
 * Its a pure convenience macro. You can also do
 * it by hand if you want. It looks like that:
 * \code
 * PyObject* CLASS::SFUNC (PyObject *self,PyObject *args,PyObject *kwd)
 * \endcode
 * see PYFUNCDEF_S for details 
 * @param CLASS is the class in which the macro take place.
 * @param SFUNC is the object method get implemented
 * @see PYFUNCDEF_S
 * @see FCPythonExport
 */
#define PYFUNCIMP_S(CLASS,SFUNC) PyObject* CLASS::SFUNC (PyObject *self,PyObject *args,PyObject *kwd)


/** Macro for initialization function of Python modules. 
 */
#if PY_MAJOR_VERSION >= 3
# define PyMOD_INIT_FUNC(name) PyMODINIT_FUNC PyInit_##name(void)
#else
# define PyMOD_INIT_FUNC(name) PyMODINIT_FUNC init##name(void)
#endif

#if PY_MAJOR_VERSION >= 3
# define PyMOD_Return(name) return name
#else
# define PyMOD_Return(name) return (void)name
#endif

/**
 * Union to convert from PyTypeObject to PyObject pointer.
 */
union PyType_Object {
    PyTypeObject *t;
    PyObject *o;
};



/*------------------------------
 * Basic defines
------------------------------*/
//typedef const char * version;     // define "version"


namespace Base
{

inline int streq(const char *A, const char *B)  // define "streq"
{ return strcmp(A,B) == 0;}


inline void Assert(int expr, char *msg)         // C++ assert
{
    if (!expr) 
    {
      fprintf(stderr, "%s\n", msg);
      exit(-1);
    };
}

}

/*------------------------------
 * Python defines
------------------------------*/

/// some basic python macros
#define Py_NEWARGS 1
/// return with no return value if nothing happens
#define Py_Return return Py_INCREF(Py_None), Py_None
/// returns an error
#define Py_Error(E, M)   _Py_Error(return(NULL),E,M)
#define _Py_Error(R, E, M)   {PyErr_SetString(E, M); R;}
/// returns an error
#define Py_ErrorObj(E, O)   _Py_ErrorObj(return(NULL),E,O)
#define _Py_ErrorObj(R, E, O)   {PyErr_SetObject(E, O); R;}
/// checks on a condition and returns an error on failure
#define Py_Try(F) {if (!(F)) return NULL;}
/// assert which returns with an error on failure
#define Py_Assert(A,E,M) {if (!(A)) {PyErr_SetString(E, M); return NULL;}}


/// This must be the first line of each PyC++ class
#define Py_Header                                           \
public:                                                     \
    static PyTypeObject   Type;                             \
    static PyMethodDef    Methods[];                        \
    virtual PyTypeObject *GetType(void) {return &Type;}

/*------------------------------
 * PyObjectBase
------------------------------*/

namespace Base
{


/** The PyObjectBase class, exports the class as a python type
 *  PyObjectBase is the base class for all C++ classes which
 *  need to get exported into the python namespace. This class is 
 *  very important because nearly all important classes in FreeCAD
 *  are visible in python for macro recording and automation purpose.
 *  The class App::Document is a good expample for an exported class.
 *  There are some convenience macros to make it easier to inherit
 *  from this class and defining new methods exported to python.
 *  PYFUNCDEF_D defines a new exported method.
 *  PYFUNCIMP_D defines the implementation of the new exported method.
 *  In the implementation you can use Py_Return, Py_Error, Py_Try and Py_Assert.
 *  PYMETHODEDEF makes the entry in the python method table.
 *  @see Document
 *  @see PYFUNCDEF_D
 *  @see PYFUNCIMP_D
 *  @see PYMETHODEDEF
 *  @see Py_Return
 *  @see Py_Error
 *  @see Py_Try
 *  @see Py_Assert  
 */
class BaseExport PyObjectBase : public PyObject 
{
    /** Py_Header struct from python.h.
     *  Every PyObjectBase object is also a python object. So you can use
     *  every Python C-Library function also on a PyObjectBase object
     */
    Py_Header

    enum Status {
        Valid = 0,
        Immutable = 1,
        Notify = 2,
        NoTrack = 3
    };

protected:
    /// destructor
    virtual ~PyObjectBase();

public:  
    /** Constructor
     *  Sets the Type of the object (for inheritance) and decrease the
     *  the reference count of the PyObject.
     */
    PyObjectBase(void*, PyTypeObject *T);
    /// Wrapper for the Python destructor
    static void PyDestructor(PyObject *P)   // python wrapper
    {  delete ((PyObjectBase *) P);  }
    /// incref method wrapper (see python extending manual)
    PyObjectBase* IncRef(void) {Py_INCREF(this);return this;}
    /// decref method wrapper (see python extending manual)	
    PyObjectBase* DecRef(void) {Py_DECREF(this);return this;}

    /** GetAttribute implementation
     *  This method implements the retrieval of object attributes.
     *  If you want to implement attributes in your class, reimplement
     *  this method.
     *  You have to call the method of the base class.
     *  Note: if you reimplement _gettattr() in a inheriting class you
     *  need to call the method of the base class! Otherwise even the 
     *  methods of the object will disappear!
     */
    virtual PyObject *_getattr(const char *attr);
    /// static wrapper for pythons _getattro()
    static  PyObject *__getattro(PyObject * PyObj, PyObject *attro);

    /** SetAttribute implementation
     *  This method implements the setting of object attributes.
     *  If you want to implement attributes in your class, reimplement
     *  this method.
     *  You have to call the method of the base class.
     */
    virtual int _setattr(const char *attro, PyObject *value);    // _setattr method
    /// static wrapper for pythons _setattro(). // This should be the entry in Type. 
    static  int __setattro(PyObject *PyObj, PyObject *attro, PyObject *value);

    /** _repr method
    * Override this method to return a string object with some
    * information about the object.
    * \code
    * PyObject *MeshFeaturePy::_repr(void)
    * {
    *   std::stringstream a;
    *   a << "MeshFeature: [ ";
    *   a << "some really important info about the object!";
    *   a << "]" << std::endl;
    *   return Py_BuildValue("s", a.str().c_str());
    * }
    * \endcode
    */
    virtual PyObject *_repr(void);
    /// python wrapper for the _repr() function
    static  PyObject *__repr(PyObject *PyObj)	{
        if (!((PyObjectBase*) PyObj)->isValid()){
            PyErr_Format(PyExc_ReferenceError, "Cannot print representation of deleted object");
            return NULL;
        }
        return ((PyObjectBase*) PyObj)->_repr();
    }

    /** PyInit method
    * Override this method to initialize a newly created
    * instance of the class (Constructor)
    */
    virtual int PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
    {
        return 0;
    }
    /// python wrapper for the _repr() function
    static  int __PyInit(PyObject* self, PyObject* args, PyObject* kwd)
    {
        return ((PyObjectBase*) self)->PyInit(args, kwd);
    }

    void setInvalid() { 
        // first bit is not set, i.e. invalid
        StatusBits.reset(Valid);
        clearAttributes();
        _pcTwinPointer = 0;
    }

    bool isValid() {
        return StatusBits.test(Valid);
    }

    void setConst() {
        // second bit is set, i.e. immutable
        StatusBits.set(Immutable);
    }

    bool isConst() {
        return StatusBits.test(Immutable);
    }

    void setShouldNotify(bool on) {
        StatusBits.set(Notify, on);
    }

    bool shouldNotify() const {
        return StatusBits.test(Notify);
    }

    void startNotify();

    void setNotTracking(bool on=true) {
        StatusBits.set(NoTrack, on);
    }

    bool isNotTracking() const {
        return StatusBits.test(NoTrack);
    }

    typedef void* PointerType;

private:
    void setAttributeOf(const char* attr, PyObject* par);
    void resetAttribute();
    PyObject* getTrackedAttribute(const char* attr);
    void trackAttribute(const char* attr, PyObject* obj);
    void untrackAttribute(const char* attr);
    void clearAttributes();

protected:
    std::bitset<32> StatusBits;
    /// pointer to the handled class
    void * _pcTwinPointer;

private:
    PyObject* attrDict;
};




/** Python dynamic class macro for definition
 * sets up a static/dynamic function entry in a class inheriting 
 * from PyObjectBase. Its a pure convenience macro. You can also do
 * it by hand if you want. It looks like that:
 * \code
 * PyObject *PyGetGrp(PyObject *args);
 * static PyObject *sPyGetGrp(PyObject *self, PyObject *args, PyObject *kwd)
 *        {return ((FCPyParametrGrp*)self)->PyGetGrp(args);};
 * \endcode
 * first the method is defined which have the functionality then the
 * static wrapper is used to provide a callback for python. The call 
 * is simply mapped to the method.
 * @param CLASS is the class in which the macro take place.
 * @param DFUNC is the object method get defined and called
 * sDFUNC is the static method name (use what you want)
 * @see PYFUNCIMP_D
 * @see PyObjectBase
 */
#define PYFUNCDEF_D(CLASS,DFUNC)	PyObject * DFUNC (PyObject *args);  \
static PyObject * s##DFUNC (PyObject *self, PyObject *args, PyObject * /*kwd*/){return (( CLASS *)self)-> DFUNC (args);};

/** Python dynamic class macro for implementation
 * used to set up an implementation for PYFUNCDEF_D definition.
 * Its a pure convenience macro. You can also do
 * it by hand if you want. It looks like that:
 * \code
 * PyObject *FCPyParametrGrp::PyGetGrp(PyObject *args)
 * \endcode
 * see PYFUNCDEF_D for details * @param CLASS is the class in which the macro take place.
 * @param DFUNC is the object method get defined and called
 * @see PYFUNCDEF_D
 * @see PyObjectBase
 */
#define PYFUNCIMP_D(CLASS,DFUNC) PyObject* CLASS::DFUNC (PyObject *args)



/** Python dynamic class macro for the method list
 * used to fill the method list of a class derived from PyObjectBase.
 * Its a pure convenience macro. You can also do
 * it by hand if you want. It looks like that:
 * \code
 * PyMethodDef DocTypeStdPy::Methods[] = {
 * 	{"AddFeature",    (PyCFunction) sAddFeature,    Py_NEWARGS},
 * 	{"RemoveFeature", (PyCFunction) sRemoveFeature, Py_NEWARGS},
 *	{NULL, NULL}		
 * };
 * \endcode
 * instead of:
 * \code
 * PyMethodDef DocTypeStdPy::Methods[] = {
 *	PYMETHODEDEF(AddFeature)
 *	PYMETHODEDEF(RemoveFeature)
 *	{NULL, NULL}		
 * };
 * \endcode
 * see PYFUNCDEF_D for details 
 * @param FUNC is the object method get defined
 * @see PYFUNCDEF_D
 * @see PyObjectBase
 */
#define PYMETHODEDEF(FUNC)	{"" #FUNC "",(PyCFunction) s##FUNC,Py_NEWARGS},

BaseExport extern PyObject* BaseExceptionFreeCADError;
#define PY_FCERROR (Base::BaseExceptionFreeCADError ? \
 BaseExceptionFreeCADError : PyExc_RuntimeError)

BaseExport extern PyObject* BaseExceptionFreeCADAbort;

/** Exception handling for python callback functions
 * Is a convenience macro to manage the exception handling of python callback
 * functions defined in classes inheriting PyObjectBase and using PYMETHODEDEF .
 * You can automate this:
 * \code
 * PYFUNCIMP_D(DocTypeStdPy,AddFeature)
 * {
 *   char *pstr;
 *   if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
 *      return NULL;                             // NULL triggers exception 
 * 
 *   try {
 *     Feature *pcFtr = _pcDocTypeStd->AddFeature(pstr);
 *   }catch(...)                                                        \
 *   {                                                                 \
 * 	 	Py_Error(Base::BaseExceptionFreeCADError,"Unknown C++ exception");          \
 *   }catch(FCException e) ..... // and so on....                                                               \
 * }
 * \endcode
 * with that:
 * \code
 * PYFUNCIMP_D(DocTypeStdPy,AddFeature)
 * {
 *   char *pstr;
 *   if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C 
 *      return NULL;                             // NULL triggers exception 
 * 
 *  PY_TRY {
 *    Feature *pcFtr = _pcDocTypeStd->AddFeature(pstr);
 *  }PY_CATCH;
 * }
 * \endcode
 * this catch maps all of the FreeCAD standard exception to a clear output for the 
 * Python exception.
 * @see PYMETHODEDEF
 * @see PyObjectBase
 */
#define PY_TRY	try 

#define _PY_CATCH(R)                                                \
    catch(Base::AbortException &e)                                  \
    {                                                               \
        e.ReportException();                                        \
        _Py_ErrorObj(R,Base::BaseExceptionFreeCADAbort,e.getPyObject());\
    }                                                               \
    catch(Base::Exception &e)                                       \
    {                                                               \
        e.ReportException();                                        \
        auto pye = e.getPyExceptionType();                          \
        if(!pye) {                                                  \
            pye = Base::BaseExceptionFreeCADError;                  \
            std::string str;                                        \
            str += "FreeCAD exception thrown (";                    \
            str += e.what();                                        \
            str += ")";                                             \
            e.setMessage(str);                                      \
        }                                                           \
        _Py_ErrorObj(R,pye,e.getPyObject());                        \
    }                                                               \
    catch(std::exception &e)                                        \
    {                                                               \
        std::string str;                                            \
        str += "STL exception thrown (";                            \
        str += e.what();                                            \
        str += ")";                                                 \
        Base::Console().Error(str.c_str());                         \
        _Py_Error(R,Base::BaseExceptionFreeCADError,str.c_str());   \
    }                                                               \
    catch(const Py::Exception&)                                     \
    {                                                               \
        R;                                                          \
    }                                                               \
    catch(const char *e)                                            \
    {                                                               \
        _Py_Error(R,Base::BaseExceptionFreeCADError,e);             \
    }                                                               \

#ifndef DONT_CATCH_CXX_EXCEPTIONS 
/// see docu of PY_TRY 
#  define PY_CATCH                                                  \
    _PY_CATCH(R)                                                    \
    catch(...)                                                      \
    {                                                               \
        _Py_Error(R,Base::BaseExceptionFreeCADError,"Unknown C++ exception"); \
    }

#else
/// see docu of PY_TRY 
#  define PY_CATCH _PY_CATCH(return(NULL))
#endif  // DONT_CATCH_CXX_EXCEPTIONS

/** Python helper class 
 *  This class encapsulate the Decoding of UTF8 to a python object.
 *  Including exception handling.
 */
inline PyObject * PyAsUnicodeObject(const char *str)
{
    // Returns a new reference, don't increment it!
    PyObject *p = PyUnicode_DecodeUTF8(str,strlen(str),0);
    if(!p)
        throw Base::UnicodeError("UTF8 conversion failure at PyAsUnicodeString()");
    return p;
}

inline PyObject * PyAsUnicodeObject(const std::string &str)
{
    return PyAsUnicodeObject(str.c_str());
}


} // namespace Base


#endif // BASE_PYOBJECTBASE_H
