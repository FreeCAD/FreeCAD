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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <cassert>
# include <iostream>
#endif

#include <QAtomicInt>

#include "Handle.h"


using namespace Base;

//**************************************************************************
// Construction/Destruction

Handled::Handled()
  : _lRefCount(new QAtomicInt(0))
{
}

Handled::~Handled()
{
    if (static_cast<int>(*_lRefCount) != 0)
        std::cerr << "Reference counter of deleted object is not zero!!!!!" << std::endl;
    delete _lRefCount;
}

void Handled::ref() const
{
    _lRefCount->ref();
}

void Handled::unref() const
{
    assert(*_lRefCount > 0);
    if (!_lRefCount->deref()) {
        delete this;
    }
}

int Handled::unrefNoDelete() const
{
    int res = _lRefCount->deref();
    assert(res>=0);
    return res;
}

int Handled::getRefCount() const
{
    return static_cast<int>(*_lRefCount);
}

Handled& Handled::operator = (const Handled&)
{
    // we must not assign _lRefCount
    return *this;
}
