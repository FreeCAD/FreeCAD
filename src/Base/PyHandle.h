/***************************************************************************
 *   Copyright (c) 2019 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

#ifndef FREECAD_BASE_PYHANDLE_H
#define FREECAD_BASE_PYHANDLE_H

#include <CXX/Objects.hxx>
#include "PyObjectBase.h"
#include "Exception.h"
#include <sstream>

#include <type_traits>

namespace Base {

/**
 * An unsafe version of PyHandle. It is designed to have the least possible
 * performance penalty of accessing the C++ object.
 *
 * Compared to PyHandle:
 *
 * * almost no typechecking (only checks against PyObjectBase)
 *
 * * doesn't check isValid on every dereference attempt. That is, if you
 * dereference it when it is None, you get a crash (dereference of null
 * pointer). And if the object was deleted, you get an even nastier situation
 * of unpredictable results, because the pointer isn't nulled out.
 */
 // It was made to become the main memory management construct of
 // ConstraintSolver. Performance is critical, as every readout of a number is
 // done through the handle.

template <class CppType>
class UnsafePyHandle 
{
public:

    // We need to validate because there is no guarantee that PyObject* is a PyObjectBase
    // The template ensures that this constructor does not take nullptr as an argument, but the one CppType * does.
    // If you want to use this constructor with the Python "equivalent" of nullptr you may use Py_None (which is not a nullptr)
    template <  typename PyObjectT,
                typename = typename std::enable_if<
                    std::is_base_of<PyObject, typename std::decay<PyObjectT>::type>::value
                >::type
    >
    explicit UnsafePyHandle(PyObjectT* pyob, bool owned = false)
    {
        set(pyob,owned);
    }

    // We need to validate because there is no guarantee that Py::Object is a PyObjectBase
    explicit UnsafePyHandle( const Py::Object &other )
    {
        set(other.ptr());
    }

    // Conversion/assignment from subclassed pointer
    UnsafePyHandle(CppType* obj)
    {
        *this = obj; // call operator=
    }
    
    void operator=(CppType* obj){
        if (obj)
            set(obj->getPyObject(), true);
        else
            makeNone();
    }
    
 
    /* Upcast and copy/assignment */
    
    // We do not need to validate because we asume the other object is valid
    //
    // N.B.: This function assumes a valid UnsafePyHandle< NewTypeT > object
    // 
    template <  typename NewTypeT,
                typename = typename std::enable_if<
                    std::is_base_of<CppType, typename std::decay<NewTypeT>::type>::value 
                >::type
    >
    UnsafePyHandle(const UnsafePyHandle< NewTypeT > &other) : pyobj(other.getHandledObject())
    {
        cppptr = other.cptr();
    }
    

    // N.B.: This function assumes a valid UnsafePyHandle< NewTypeT > object
    // 
    template <  typename NewTypeT,
                typename = typename std::enable_if<
                    std::is_base_of<CppType, typename std::decay<NewTypeT>::type>::value
             >::type
    >
    UnsafePyHandle& operator=(const UnsafePyHandle< NewTypeT > &rhs)
    {
        pyobj = rhs.ptr();
        
        cppptr = static_cast<CppType*>(rhs.cptr());
        
        return *this;
    }
    
    template <  typename NewTypeT,
                typename = typename std::enable_if<
                    std::is_base_of<typename std::decay<NewTypeT>::type, CppType>::value
             >::type
    >
    UnsafePyHandle<NewTypeT> & upcast() 
    {
        return reinterpret_cast<UnsafePyHandle<NewTypeT> &>(*this);
    }
    
    /* Downcast */
    template <  typename NewTypeT,
                typename = typename std::enable_if<
                    std::is_base_of<CppType, typename std::decay<NewTypeT>::type>::value
             >::type
    >
    UnsafePyHandle<NewTypeT> downcast() 
    {
        return UnsafePyHandle<NewTypeT>(this->ptr());
    }
    
    /* C++ pointer operations */
    
    CppType& operator*() const {
        return *cppptr;
    }
    
    CppType* operator->() const {
        return cppptr;
    }
    
    CppType* cptr() const {
        return cppptr;
    }
    
    /* Encapsulation exposing methods of Python */
    
    PyObject *ptr() const
    {
        return pyobj.ptr();
    }
    
    const Py::Object& getHandledObject() const
    {
        return pyobj;
    }
    
    /* Py::Object minimum interface */
    
    inline virtual void makeNone() // avoids exposing the type to a QObject assignment
    {
        pyobj = Py::None();
        cppptr = nullptr;
    }

    inline friend bool operator==( const UnsafePyHandle<CppType> &o1, const UnsafePyHandle<CppType> &o2 )
    {
        return o1.getHandledObject() == o2.getHandledObject();
    }
    
    inline bool isNone() const
    {
        return pyobj.isNone();
    }
    
    inline bool is( const UnsafePyHandle &other ) const
    {
        return ptr() == other.ptr();
    }
    
    /* PyObject checks and accepts */
    
    // Py_None or valid PyObjectBase
    bool accepts(PyObject* pyob) const {
        if (pyob == Py_None)
            return true;
        else
            return this->typecheck(pyob);
    }
    
    /**
    * @brief typecheck: Only a valid PyObjectBase is acceptable.
    * @return whether the type is acceptable or not.
    */
    bool typecheck(PyObject* pyob) const {
        if (! PyObject_TypeCheck(pyob, &(Base::PyObjectBase::Type))){
            return false;
        }
        if (! static_cast<Base::PyObjectBase*>(pyob)->isValid()){
            return false;
        }
        return true;
    }
    
    /**
    * @brief isValid: if the C++ object was deleted but Py object is still
    * around, isValid will return false. Attempting to dereference the handle
    * in such state is dereferencing a null pointer (i.e., crash).
    * @return if it's ok to dereference the handle.
    */
    bool isValid() const {
        if (isNone()){
            return false;
        }
        if (! static_cast<Base::PyObjectBase*>(this->ptr())->isValid()){
            return false;
        }
        return true;
    }

protected: //methods
            
    void set( PyObject *pyob, bool owned = false )
    {        
        if (typecheck(pyob))
            cppptr = static_cast<CppType*>(static_cast<Base::PyObjectBase*>(pyob)->twinPtr());
        else if (pyob == Py_None) {
            cppptr = nullptr;
        } else {
            throwInvalidType(pyob);
        }
        
        pyobj = pyob ;
        
        if(owned)
            pyobj.decrement_reference_count();
    }
    
    void throwInvalid() const {
        if (isNone()){
            throw Base::ReferencesError("PyHandle is None and can't be dereferenced");
        }
        if (! static_cast<Base::PyObjectBase*>(this->ptr())->isValid()){
            throw Base::ReferencesError("Object referenced by PyHandle is deleted");
        }
    }
    
    void throwInvalidType(PyObject* pyob) const {
        if (! PyObject_TypeCheck(pyob, &(Base::PyObjectBase::Type))){
            std::stringstream ss;
            Py::Object ob(pyob);
            ss << "Can only accept " << PyObjectBase::Type.tp_name
            << " or None, but got " << ob.type().as_string();
            throw Base::TypeError(ss.str());
        }
        if (! static_cast<Base::PyObjectBase*>(pyob)->isValid()){
            throw Base::ReferencesError("Can't accept a deleted object");
        }
    }
    
protected://data
    Py::Object pyobj;
    CppType* cppptr;

};

} //namespace

#endif
