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

namespace Base {

/** UnsafePyHandle: a convenient way to use python references as smart pointers for C++ objects.
 * "Unsafe" stands for lack of runtime checks: type checking is limited; dereferencing the object
 * if the C++ object was deleted will crash the application (you can check isValid() to guard
 * against the latter).
 *
 * Usage example:
 *     //create a py object, and save it into the handle
 *     UnsafePyHandle<Base::Placement> h (new Base::PlacementPy(new Base::Placement), true);
 *     //use the handle as if it is a pointer to C++ object
 *     h->getPosition()
 *     //and when the handle is destroyed, so is the py object, if it isn't referred to by something else.
 *
 * ## Memory management
 *
 * * when PyHandle is created, reference counter is incremented (except if
 * "new_reference" constructor argument is true).
 *
 * * when PyHandle is destroyed, reference counter is decremented (no matter
 * what "new_reference" value was passed to constructor). If resulting refcount
 * is zero, the object is deleted.
 *
 * * Py::new_reference_to(PyHandle) increments reference count and returns PyObject*
 *
 * Cheatsheet:
 *
 * * when constructing from "new SomethingSomethingPy(new SomethingSomething)",
 * use "new_reference". That is because SomethingSomethingPy sets reference counter
 * to 1 in its constructor.
 *
 * * when constructing from getPyObject, use "new_reference"
 *
 * * when constructing from PyObject* you got as an argument of ...PyImp method
 * implementation, don't use "new_reference".
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
 // Since it was made to become the main memory management construct of
 // ConstraintSolver, checks for isValid() are intentionally not done to maximize
 // performance. It is named "Unsafe" with the intention to have room for a safer
 // "PyHandle" replica.
template <class CppType>
class UnsafePyHandle : public Py::Object
{
private:
    CppType* cppptr;
protected:
    virtual void setObject(PyObject* pyob, bool new_reference = false){
        if (typecheck(pyob))
            cppptr = static_cast<CppType*>(static_cast<Base::PyObjectBase*>(pyob)->twinPtr());
        else
            throw Base::TypeError("Not a freecad object");
        this->set(pyob, new_reference);
    }

public:
    virtual bool typecheck(PyObject* pyob) {
        if (! PyObject_TypeCheck(pyob, &(Base::PyObjectBase::Type)))
            return false;
        if (! static_cast<Base::PyObjectBase*>(pyob)->isValid())
            return false;
        return true;
    }

    //We can't override Py::Object::set
    //so, mask out all methods to change pointer of Py::Object
    //it may still be possible to bypass with typecasts,
    //but at least that isn't straightforward.
    /**
     * @brief UnsafePyHandle constructor
     * @param pyob
     * @param new_reference: specify True if the reference is "owned" by the calling function,
     * particularly when creating a new object like UnsafePyHandle<Base::Placement> hplm (new PlacementPy(...))
     * For more info, see docs for PyCXX's Object's "owned" argument.
     */
    explicit UnsafePyHandle(PyObject* pyob, bool new_reference)
    {
        setObject(pyob, new_reference);
    }

    UnsafePyHandle( const Object &other )
    {
        setObject(other.ptr());
    }

    UnsafePyHandle& operator=(PyObject* pyob){
        setObject(pyob);
    }

    UnsafePyHandle& operator=(Py::Object other){
        setObject(other.ptr());
    }

    /**
     * @brief isValid: if the C++ object was deleted but Py object is still
     * around, isValid will return false. Attempting to dereference the handle
     * in such state is dereferencing a null pointer (i.e., crash).
     * @return if it's ok to dereference the handle.
     */
    bool isValid() const {
        return static_cast<Base::PyObjectBase*>(this->ptr())->isValid();
    }

    CppType& operator*() const {
        return *cppptr;
    }
    CppType* operator->() const {
        return cppptr;
    }

    bool operator==(UnsafePyHandle<CppType> &other){return ptr() == other.ptr();}
};

} //namespace

#endif
