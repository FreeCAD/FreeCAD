/***************************************************************************
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

#ifndef GUID_HEADER
#define GUID_HEADER

#include <iostream>
#include <sstream>
#include <stdint.h>

#include "U16.h"
#include "U32.h"
#include "U8.h"


using namespace std;

#undef _C2

struct GUID
{
    GUID() {};

    GUID(uint32_t a1,
         uint16_t b1,
         uint16_t b2,
         uint8_t c1,
         uint8_t c2,
         uint8_t c3,
         uint8_t c4,
         uint8_t c5,
         uint8_t c6,
         uint8_t c7,
         uint8_t c8)
        : _A1(a1)
        , _B1(b1)
        , _B2(b2)
        , _C1(c1)
        , _C2(c2)
        , _C3(c3)
        , _C4(c4)
        , _C5(c5)
        , _C6(c6)
        , _C7(c7)
        , _C8(c8)
    {}

    GUID(Context& cont)
    {
        read(cont);
    }

    bool operator!=(const GUID& other) const
    {
        return !operator==(other);
    }

    bool operator==(const GUID& other) const
    {
        return _A1 == other._A1 && _B1 == other._B1 && _B2 == other._B2 && _C1 == other._C1
            && _C2 == other._C2 && _C3 == other._C3 && _C4 == other._C4 && _C5 == other._C5
            && _C6 == other._C6 && _C7 == other._C7 && _C8 == other._C8;
    }

    inline void read(Context& cont)
    {
        _A1.read(cont);

        _B1.read(cont);
        _B2.read(cont);

        _C1.read(cont);
        _C2.read(cont);
        _C3.read(cont);
        _C4.read(cont);
        _C5.read(cont);
        _C6.read(cont);
        _C7.read(cont);
        _C8.read(cont);
    }

    std::string toString() const
    {
        std::stringstream strm;
        strm << "{" << std::hex << _A1 << "-" << _B1 << _B2 << "-" << _C1 << _C2 << _C3 << _C4
             << _C5 << _C6 << _C7 << _C8 << "}";
        return strm.str();
    }

    U32 _A1;

    U16 _B1;
    U16 _B2;

    U8 _C1;
    U8 _C2;
    U8 _C3;
    U8 _C4;
    U8 _C5;
    U8 _C6;
    U8 _C7;
    U8 _C8;
};


#endif
