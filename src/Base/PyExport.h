// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


/** \file PyExport.h
 *  \brief the python object export base class
 *  \author Juergen Riegel
 *  \version 0.1
 *  \date    5.2001
 */

#pragma once

// (re-)defined in pyconfig.h
#if defined(_POSIX_C_SOURCE)
# undef _POSIX_C_SOURCE
#endif
#if defined(_XOPEN_SOURCE)
# undef _XOPEN_SOURCE
#endif

#include <Python.h>

#include <FCConfig.h>

#ifdef FC_OS_MACOSX
# undef toupper
# undef tolower
# undef isupper
# undef islower
# undef isspace
# undef isalpha
# undef isalnum
#endif

namespace Base
{
class PyObjectBase;

/** The PyHandler class
 * This class is the base class of all FreeCAD classes
 * which exports into the python space. This class handles the
 * creation referencing of the python export object.
 *
 * @remark GetPyObject() returns the associated Python object to any C++ subclasses. As we cannot
 * determine for sure if we can increment the returned Python object from outside of GetPyObject()
 * we have specified that GetPyObject() does already the increment of the reference counter
 * if needed.
 *
 * E.g. if GetPyObject() always returns a new Python object then no increment is necessary,
 * because at construction time the reference counter is already set to 1. If the Python
 * interpreter stores this object pointer into a local variable and destroys this variable
 * then the reference counter gets decremented (to 0) and the object gets destroyed automatically.
 * In case we didn't make this specification and increment the Python object from outside once
 * again then the reference counter would be set to 2 and there would be no chance to destroy the
 * object again.
 *
 * The other case is that we have a member variable in our C++ class that holds the Python object
 * then we either can create this Python in the constructor or create it the first  time when
 * GetPyObject() gets called. In the destructor then we must decrement the Python object to avoid a
 * memory leak while GetPyObject() then increments the Python object every time it gets called.
 *
 * @remark One big consequence of this specification is that the programmer must know whether the
 * Python interpreter gets the Python object or not. If the interpreter gets the object then it
 * decrements the counter later on when the internal variable is freed. In case the interpreter
 * doesn't get this object then the programmer must do the decrement on their own.
 *
 * @note To not to undermine this specification the programmer must make sure to get the Python
 * object always via GetPyObject().
 *
 * @see PyHandle
 * @
 */
// class BaseExport PyHandler
//{
// public:
//	void IncRef(void);
//	void DecRef(void);
//   virtual ~PyHandler();
//	virtual PyObjectBase *GetPyObject(void)=0;
//
// };


/** Python Object handle class
 * Using pointers on classes derived from PyObjectBase would
 * be potentionaly dangerous because you would have to take
 * care of the reference counting of python by your self. Hence
 * this class was designed. It takes care of references and
 * as long as a object of this class exists the handled class get
 * not destructed. That means a PyObjectBase derived object you can
 * only destruct by destructing all FCPyHandle and all python
 * references on it!
 * @see PyObjectBase
 */
template<class HandledType>
class PyHandle  // NOLINT
{
public:
    //**************************************************************************
    // construction destruction

    /** pointer and default constructor
     *  the good way would be not using pointer
     *  instead using a overwritten new operator in the
     *  HandledType class! But is not easy to enforce!
     */
    PyHandle(HandledType* ToHandle = 0L)
        : _pHandles(ToHandle)
    {
        if (_pHandles) {
            _pHandles->IncRef();
        }
    }

    /// Copy constructor
    PyHandle(const PyHandle<HandledType>& ToHandle)
        : _pHandles(ToHandle._pHandles)
    {
        if (_pHandles) {
            _pHandles->IncRef();
        }
    }

    /** destructor
     *  Release the reference count which cause,
     *  if was the last one, the referenced object to
     *  destruct!
     */
    ~PyHandle()
    {
        if (_pHandles) {
            _pHandles->DecRef();
        }
    }

    //**************************************************************************
    // operator implementation

    // assign operator from a pointer
    PyHandle<HandledType>& operator=(/*const*/ HandledType* other)
    {
        if (_pHandles) {
            _pHandles->DecRef();
        }
        // FIXME: Should be without "->_pHandles", shouldn't it? (Werner)
        _pHandles = other;  //_pHandles = other->_pHandles;
        if (_pHandles) {
            _pHandles->IncRef();
        }
        return *this;
    }

    // assign operator from a handle
    PyHandle<HandledType>& operator=(const PyHandle<HandledType>& other)
    {
        if (_pHandles) {
            _pHandles->DecRef();
        }
        _pHandles = other._pHandles;
        if (_pHandles) {
            _pHandles->IncRef();
        }
        return *this;
    }

    /// dereference operators
    HandledType& operator*()
    {
        return *_pHandles;
    }

    /// dereference operators
    HandledType* operator->()
    {
        return _pHandles;
    }

    /// dereference operators
    const HandledType& operator*() const
    {
        return _pHandles;
    }

    /// dereference operators
    const HandledType* operator->() const
    {
        return _pHandles;
    }

    /** lower operator
     *  needed for sorting in maps and sets
     */
    bool operator<(const PyHandle<HandledType>& other) const
    {
        // return _pHandles<&other;
        //  FIXME: Shouldn't we compare both pointers?. (Werner)
        return _pHandles < other._pHandles;
    }

    /// equal operator
    bool operator==(const PyHandle<HandledType>& other) const
    {
        // return _pHandles==&other;
        //  FIXME: Shouldn't we compare both pointers?. (Werner)
        return _pHandles == other._pHandles;
    }

    /// returns the type as PyObject
    PyObject* getPyObject() const
    {
        // return (PyObject*) _pHandles;
        // FIXME: Shouldn't we return the pointer's object?. (Werner)
        return const_cast<HandledType*>(_pHandles)->getPyObject();  // NOLINT
    }
    //**************************************************************************
    // checking on the state

    /// Test if it handles something
    bool IsValid() const
    {
        return _pHandles != 0;
    }

    /// Test if it not handles something
    bool IsNull() const
    {
        return _pHandles == 0;
    }

private:
    /// the pointer on the handled object
    HandledType* _pHandles;
};


}  // namespace Base
