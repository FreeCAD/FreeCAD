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

namespace Base {

///helper intermediate class to support assignment to arbitrary pyhandle by reference (see ConstraintSolver/App/ParaObject.cpp method setAttr)
class PyHandleBase : public Py::Object
{
public:
    virtual void setObject(PyObject* pyob, bool owned = false) = 0;
    void operator=(PyObject* pyob){
        setObject(pyob);
    }
    void operator=(Py::Object other){
        setObject(other.ptr());
    }
};

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
class UnsafePyHandle : public PyHandleBase
{
public:
    virtual bool typecheck(PyObject* pyob, bool throw_instead_of_return = false) const {
        if (! PyObject_TypeCheck(pyob, &(Base::PyObjectBase::Type))){
            if (throw_instead_of_return){
                std::stringstream ss;
                Py::Object ob(pyob);
                ss << "Can only accept " << PyObjectBase::Type.tp_name
                   << " or None, but got " << ob.type().as_string();
                throw Base::TypeError(ss.str());
            }
            return false;
        }
        if (! static_cast<Base::PyObjectBase*>(pyob)->isValid()){
            if (throw_instead_of_return){
                throw Base::ReferencesError("Can't accept a deleted object");
            }
            return false;
        }
        return true;
    }

    //We can't override Py::Object::set
    //so, mask out all methods to change pointer of Py::Object
    //it may still be possible to bypass with typecasts,
    //but at least that isn't straightforward.
    explicit UnsafePyHandle(PyObject* pyob, bool owned)
    {
        setObject(pyob, owned);
    }

    UnsafePyHandle( const Py::Object &other )
    {
        setObject(other.ptr());
    }

    /**
     * @brief isValid: if the C++ object was deleted but Py object is still
     * around, isValid will return false. Attempting to dereference the handle
     * in such state is dereferencing a null pointer (i.e., crash).
     * @return if it's ok to dereference the handle.
     */
    bool isValid(bool throw_instead_of_return = false) const {
        if (isNone()){
            if (throw_instead_of_return)
                throw Base::ReferencesError("PyHandle is None and can't be dereferenced");
            return false;
        }
        if (! static_cast<Base::PyObjectBase*>(this->ptr())->isValid()){
            if (throw_instead_of_return)
                throw Base::ReferencesError("Object referenced by PyHandle is deleted");
            return false;
        }
        return true;
    }

    CppType& operator*() const {
        return *cppptr;
    }
    CppType* operator->() const {
        return cppptr;
    }
    ///returns pointer to C++ object
    CppType* cptr() const {
        return cppptr;
    }

protected: //methods
    virtual void setObject(PyObject* pyob, bool owned = false){
        if (typecheck(pyob))
            cppptr = static_cast<CppType*>(static_cast<Base::PyObjectBase*>(pyob)->twinPtr());
        else if (pyob == Py_None) {
            cppptr = nullptr;
        } else {
            typecheck(pyob, /*throw=*/true);
        }
        this->set(pyob, owned);
    }

    virtual bool accepts(PyObject* pyob) const override {
        if (pyob == Py_None)
            return true;
        else
            return this->typecheck(pyob);
    }

protected://data
    CppType* cppptr;

};

/** PyHandle: a convenient way to use python references as smart pointers for C++ objects.
 *
 * PyHandle can also hold None + nullptr combo.
 *
 * Usage example:
 * ```cpp
 * //create a py object, and save it into the handle
 * PyHandle<Base::Placement, Base::PlacementPy> h (new Base::PlacementPy(new Base::Placement), true);
 * //use the handle as if it is a pointer to C++ object
 * h->getPosition()
 * //and when the handle is destroyed, so is the py object, if it isn't referred to by something else.
 * ```
 *
 * ## Memory management
 *
 * * when PyHandle is created, reference counter is incremented (except if
 * "owned" constructor argument is true).
 *
 * * when PyHandle is destroyed, reference counter is decremented (no matter
 * what "owned" value was passed to constructor). If resulting refcount
 * is zero, the object is deleted.
 *
 * * Py::new_reference_to(PyHandle) increments reference count and returns PyObject*
 *
 * Cheatsheet:
 *
 * * when constructing from "new SomethingSomethingPy(new SomethingSomething)",
 * use "owned". That is because SomethingSomethingPy sets reference counter
 * to 1 in its constructor.
 *
 * * when constructing from getPyObject, use "owned"
 *
 * * when constructing from PyObject* you got as an argument of ...PyImp method
 * implementation, don't use "owned".
 *
 * * when returning PyObject* from a getPyObject, use
 * Py::new_reference_to(<PyHangle> object).
 *
 * * when returning PyObject* from a ...PyImp method implementation, use
 * Py::new_reference_to(<PyHangle> object).
 *
 * The handle derives from PyCXX's Py::Object, so it can be conveniently used
 * for adding to tuples and the like, when programming python API.
 */
template<class CppType, class PyType>
class PyHandle: public UnsafePyHandle<CppType>{
public:
    virtual bool typecheck(PyObject* pyob, bool throw_instead_of_return = false) const override {
        if (! PyObject_TypeCheck(pyob, &(PyType::Type))){
            if (throw_instead_of_return){
                std::stringstream ss;
                Py::Object ob(pyob);
                ss << "Can only accept " << PyType::Type.tp_name
                   << " or None, but got " << ob.type().as_string();
                throw Base::TypeError(ss.str());
            }
            return false;
        }
        return UnsafePyHandle<CppType>::typecheck(pyob, throw_instead_of_return);
    }

    /**
     * @brief constructor
     * @param pyob
     * @param owned: specify True if constructing from "new reference".
     * particularly when creating a new object:
     * ```cpp
     * PyHandle<Placement, PlacemenyPy> hplm (new PlacementPy(...), true)
     *                                                              ^^^^
     * ```
     * For more info, see docs for PyCXX's Object's "owned" argument.
     */
    explicit PyHandle(PyObject* pyob, bool owned)
        : PyHandle(pyob, owned){}

    PyHandle( const Py::Object &other )
        : PyHandle(other){}

    CppType& operator*() const {
        this->isValid(/*throw=*/true);
        return *(this->cppptr);
    }
    CppType* operator->() const {
        this->isValid(/*throw=*/true);
        return this->cppptr;
    }

};

} //namespace

#endif
