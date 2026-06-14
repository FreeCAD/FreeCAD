// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <cassert>
#include <iostream>

#include "Handle.h"


using namespace Base;

//**************************************************************************
// Construction/Destruction

Handled::Handled()
{}

Handled::~Handled()
{
    if (_refCount.load(std::memory_order_relaxed) != 0) {
        std::cerr << "Reference counter of deleted object is not zero!!!!!" << std::endl;
    }
}

void Handled::ref() const
{
    _refCount.fetch_add(1, std::memory_order_relaxed);
}

void Handled::unref() const
{
    const int prev = _refCount.fetch_sub(1, std::memory_order_acq_rel);
    assert(prev > 0);
    if (prev == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int Handled::unrefNoDelete() const
{
    const int prev = _refCount.fetch_sub(1, std::memory_order_acq_rel);
    assert(prev > 0);
    const int after = prev - 1;
    return after != 0;
}

int Handled::getRefCount() const
{
    return _refCount.load(std::memory_order_relaxed);
}

Handled& Handled::operator=(const Handled& /*unused*/)
{
    // we must not assign _refCount
    return *this;
}
