/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <cassert>
#endif

#include "BaseClass.h"
#include "PyObjectBase.h"

using namespace Base;

Type BaseClass::classTypeId = Base::Type::badType();


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
BaseClass::BaseClass() = default;

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
BaseClass::~BaseClass() = default;


//**************************************************************************
// separator for other implementation aspects

void BaseClass::init()
{
    assert(BaseClass::classTypeId == Type::badType() && "don't init() twice!");
    /* Make sure superclass gets initialized before subclass. */
    /*assert(strcmp(#_parentclass_), "inherited"));*/
    /*Type parentType(Type::fromName(#_parentclass_));*/
    /*assert(parentType != Type::badType() && "you forgot init() on parentclass!");*/

    /* Set up entry in the type system. */
    BaseClass::classTypeId =
        Type::createType(Type::badType(),
                         "Base::BaseClass",
                         BaseClass::create);
}

Type BaseClass::getClassTypeId()
{
    return BaseClass::classTypeId;
}

Type BaseClass::getTypeId() const
{
    return BaseClass::classTypeId;
}


void BaseClass::initSubclass(Base::Type &toInit,const char* ClassName, const char *ParentName,
                             Type::instantiationMethod method)
{
    // don't init twice!
    assert(toInit == Base::Type::badType());
    // get the parent class
    Base::Type parentType(Base::Type::fromName(ParentName));
    // forgot init parent!
    assert(parentType != Base::Type::badType() );

    // create the new type
    toInit = Base::Type::createType(parentType, ClassName, method);
}

/**
 * This method returns the Python wrapper for a C++ object. It's in the responsibility of
 * the programmer to do the correct reference counting. Basically there are two ways how
 * to implement that: Either always return a new Python object then reference counting is
 * not a matter or return always the same Python object then the reference counter must be
 * incremented by one. However, it's absolutely forbidden to return always the same Python
 * object without incrementing the reference counter.
 *
 * The default implementation returns 'None'.
 */
PyObject *BaseClass::getPyObject()
{
    assert(0);
    Py_Return;
}

void BaseClass::setPyObject(PyObject *)
{
    assert(0);
}
