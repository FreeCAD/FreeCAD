// SPDX-License-Identifier: LGPL-2.1-or-later

/**************************************************************************
 *   Copyright (c) 2014 Juergen Riegel <juergen.riegel@web.de>             *
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

#pragma once

#include "Context.h"
#include <istream>
#include <stdint.h>

using namespace std;

struct U32
{
    U32() {};

    U32(uint32_t ui)
        : _U32(ui)
    {}

    U32(Context& cont)
    {
        read(cont);
    }

    inline operator uint32_t() const
    {
        return _U32;
    }

    inline void read(Context& cont)
    {
        cont.Strm.read((char*)&_U32, 4);
    }

    uint32_t _U32;
};
