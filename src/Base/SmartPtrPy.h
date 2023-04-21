/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PY_SMARTPTRPY_H
#define PY_SMARTPTRPY_H

#include <FCGlobal.h>

// forward declarations
using PyObject = struct _object;

namespace Py {
class Object;

/**
 * \brief This is a stripped-down version of Py::Object.
 * The purpose of this class is to avoid to include any other header file
 * and therefore the PyObject is forward declared and the implementation
 * is separated from the class declaration.
 *
 * \author Werner Mayer
 */
class BaseExport SmartPtr
{
private:
    PyObject *p;

protected:
    void set(PyObject *pyob, bool owned = false);
    void release();

public:
    SmartPtr();

    // Constructor acquires new ownership of pointer unless explicitly told not to.
    explicit SmartPtr(PyObject *pyob, bool owned = false);

    // Copy constructor acquires new ownership of pointer
    SmartPtr(const SmartPtr &ob);

    // Assignment acquires new ownership of pointer
    SmartPtr &operator=( const SmartPtr &rhs );
    SmartPtr &operator=( const Object &rhs );

    SmartPtr &operator=(PyObject *rhsp);

    // Destructor
    virtual ~SmartPtr();

    // Loaning the pointer to others, retain ownership
    PyObject *operator*() const;

    // Would like to call this pointer() but messes up STL in SeqBase<T>
    PyObject *ptr() const;
    //
    // Queries
    //

    bool is(PyObject *pother) const;
    bool is(const SmartPtr &other) const;

    bool isNull() const;
};

BaseExport PyObject* new_reference_to(const SmartPtr&);

}

#endif // PY_SMARTPTRPY_H
