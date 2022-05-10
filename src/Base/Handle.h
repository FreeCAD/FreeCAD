/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef BASE_HANDLE_H
#define BASE_HANDLE_H

#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif


class QAtomicInt;

namespace Base
{

/** Reference class
 *  Implementation of the reference counting pattern.
 *  Only able to instantiate with a class inheriting
 *  Base::Handled.
 */
template <class T>
class Reference
{
public:
    //**************************************************************************
    // construction & destruction

    /** Pointer and default constructor */
    Reference() : _toHandle(nullptr) {
    }

    Reference(T* p) : _toHandle(p) {
        if (_toHandle)
            _toHandle->ref();
    }

    /** Copy constructor */
    Reference(const Reference<T>& p) : _toHandle(p._toHandle) {
        if (_toHandle)
            _toHandle->ref();
    }

    /** destructor
     *  Release the reference counter which causes,
     *  in case of the last one, the referenced object to
     *  be destructed!
     */
    ~Reference() {
        if (_toHandle)
            _toHandle->unref();
    }

    //**************************************************************************
    // operator implementation

    /** Assign operator from a pointer */
    Reference <T>& operator=(T* p) {
        // check if we want to reassign the same object
        if (_toHandle == p)
            return *this;
        if (_toHandle)
            _toHandle->unref();
        _toHandle = p;
        if (_toHandle)
            _toHandle->ref();
        return *this;
    }

    /** Assign operator from a handle */
    Reference <T>& operator=(const Reference<T>& p) {
        // check if we want to reassign the same object
        if (_toHandle == p._toHandle)
            return *this;
        if (_toHandle)
            _toHandle->unref();
        _toHandle = p._toHandle;
        if (_toHandle)
            _toHandle->ref();
        return *this;
    }

    /** Dereference operator */
    T& operator*() const {
        return *_toHandle;
    }

    /** Dereference operator */
    T* operator->() const {
        return _toHandle;
    }

    operator T*() const {
        return _toHandle;
    }

    /** Lower operator, needed for sorting in maps and sets */
    bool operator<(const Reference<T>& p) const {
        return _toHandle < p._toHandle;
    }

    /** Equal operator */
    bool operator==(const Reference<T>& p) const {
        return _toHandle == p._toHandle;
    }

    bool operator!=(const Reference<T>& p) const {
        return _toHandle != p._toHandle;
    }


    //**************************************************************************
    // checking on the state

    /// Test if it handles something
    bool isValid() const {
        return _toHandle != nullptr;
    }

    /// Test if it does not handle anything
    bool isNull() const {
        return _toHandle == nullptr;
    }

    /// Get number of references on the object, including this one
    int getRefCount() const {
        if (_toHandle)
            return _toHandle->getRefCount();
        return 0;
    }

private:
    T *_toHandle; /** the pointer to the handled object */
};

/** Handled class
 *  Implementation of the reference counting pattern.
 */
class BaseExport Handled
{
public:
    Handled();
    virtual ~Handled();

    void ref() const;
    void unref() const;

    int getRefCount() const;
    const Handled& operator = (const Handled&);

private:
    Handled(const Handled&);

private:
    QAtomicInt* _lRefCount;
};

} // namespace Base

#endif // BASE_HANDLE_H
