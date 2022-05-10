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


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "SmartPtrPy.h"
#include <CXX/Objects.hxx>


namespace Py {
    void SmartPtr::set(PyObject *pyob, bool owned)
    {
        release();
        p = pyob;
        if( !owned )
        {
            Py::_XINCREF( p );
        }
    }

    void SmartPtr::release()
    {
        Py::_XDECREF( p );
        p = nullptr;
    }

    SmartPtr::SmartPtr()
        : p( Py::_None() )
    {
        Py::_XINCREF( p );
    }

    SmartPtr::SmartPtr( PyObject *pyob, bool owned)
    : p( pyob )
    {
        if( !owned )
        {
            Py::_XINCREF( p );
        }
    }

    SmartPtr::SmartPtr( const SmartPtr &ob )
    : p( ob.p )
    {
        Py::_XINCREF( p );
    }

    SmartPtr &SmartPtr::operator=( const SmartPtr &rhs )
    {
        set( rhs.ptr() );
        return *this;
    }

    SmartPtr &SmartPtr::operator=( const Object &rhs )
    {
        set( rhs.ptr() );
        return *this;
    }

    SmartPtr &SmartPtr::operator=( PyObject *rhsp )
    {
        if( ptr() != rhsp )
            set( rhsp );

        return *this;
    }

    SmartPtr::~SmartPtr()
    {
        release();
    }

    PyObject *SmartPtr::operator*() const
    {
        return p;
    }

    PyObject *SmartPtr::ptr() const
    {
        return p;
    }

    bool SmartPtr::is( PyObject *pother ) const
    {  // identity test
        return p == pother;
    }

    bool SmartPtr::is( const SmartPtr &other ) const
    { // identity test
        return p == other.p;
    }

    bool SmartPtr::isNull() const
    {
        return p == nullptr;
    }

    BaseExport PyObject* new_reference_to(const SmartPtr& g) {
        PyObject* p = g.ptr();
        Py::_XINCREF(p);
        return p;
    }
}
