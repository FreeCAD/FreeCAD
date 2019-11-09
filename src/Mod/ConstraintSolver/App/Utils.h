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

#define GCSExport        //temporary!
#include <FCConfig.h>    //

#ifndef FREECAD_CONSTRAINTSOLVER_UTILS_H
#define FREECAD_CONSTRAINTSOLVER_UTILS_H

#include <Base/PyHandle.h>

namespace GCS {

using Base::UnsafePyHandle;

///an expressive way to append one std::vector to another
template<typename Vec>
inline void extend(Vec& to, const Vec& what){
    to.insert(to.begin, what.begin, what.end);
}

///converts std::vector (or whatever container) to Py::List. The elements must have getPyObject method.
template<class Vec>
inline Py::List asPyList(Vec& vec){ //vec is usually not changed... unless getPyObject changes the object, which does happen to some
    Py::List ret;
    for (auto &v : vec) {
        ret.append(Py::Object(v.getPyObject(), true));
    }
    return ret;
}
//
///** UnsafePyHandle: a convenient way to use python references as smart pointers for C++ objects.
// * "Unsafe" stands for lack of runtime checks: type checking is limited, and dereferencing the object
// * if the C++ object was deleted will crash the application.
// *
// * Usage example:
// *     UnsafePyHandle<Base::Placement> h (new Base::PlacementPy(new Base::Placement));
// *     h->getPosition()
// *
// * The handle derives from Py::Object, so it can be conveniently used for adding to tuples and the like, when programming python API.
// *
// * Only minimal typechecking is done:
// * * test if the python object is derived from PyObjectBase
// * (filters out things like None)
// * It's up to the coder to ensure correct type of the underlying C++ twin object.
// *
// * If you are referencing things with C++ memory management (e.g.
// * DocumentObject), the pointer will become zero when the object is deleted, so
// * you might want to check isValid.
// */
//template <class CppType>
//class GCSExport UnsafePyHandle : public Py::Object
//{
//private:
//    CppType* cppptr;
//protected:
//    virtual void setObject(PyObject* pyob){
//        if (typecheck(pyob))
//            cppptr = static_cast<CppType*>(static_cast<Base::PyObjectBase*>(pyob)->twinPtr());
//        else
//            throw Base::TypeError("Not a freecad object");
//        this->set(pyob);
//    }
//
//public:
//    virtual bool typecheck(PyObject* pyob) {
//        if (! PyObject_TypeCheck(pyob, &(Base::PyObjectBase::Type)))
//            return false;
//        if (! static_cast<Base::PyObjectBase*>(pyob)->isValid())
//            return false;
//        return true;
//    }
//
//    //We can't override Py::Object::set
//    //so, mask out all methods to change pointer of Py::Object
//    //it may still be possible to bypass with typecasts,
//    //but at least that isn't straightforward.
//    explicit UnsafePyHandle(PyObject* pyob)
//    {
//        setObject(pyob);
//    }
//
//    UnsafePyHandle( const Object &other )
//    {
//        setObject(other.ptr());
//    }
//
//    UnsafePyHandle& operator=(PyObject* pyob){
//        setObject(pyob);
//    }
//
//    UnsafePyHandle& operator=(Py::Object other){
//        setObject(other.ptr());
//    }
//
//    /**
//     * @brief isValid: if the C++ object was deleted but Py object is still
//     * around, isValid will return false. Attempting to dereference the handle
//     * in such state is dereferencing a null pointer (i.e., crash).
//     * @return if it's ok to dereference the handle.
//     */
//    bool isValid() const {
//        return static_cast<Base::PyObjectBase*>(this->ptr())->isValid();
//    }
//
//    CppType& operator*() const {
//        return *cppptr;
//    }
//    CppType* operator->() const {
//        return cppptr;
//    }
//
//    bool operator==(UnsafePyHandle<CppType> &other){return ptr() == other.ptr();}
//};
//
} //namespace

#endif
