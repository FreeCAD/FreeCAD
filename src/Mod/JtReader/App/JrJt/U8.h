// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2014 Juergen Riegel <juergen.riegel@web.de>                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef U8_HEADER
#define U8_HEADER

#include "Context.h"
#include <istream>
#include <stdint.h>

using namespace std;

struct U8
{
    U8() {};

    U8(uint8_t ui)
        : _U8(ui)
    {}

    U8(Context& cont)
    {
        read(cont);
    }

    inline void read(Context& cont)
    {
        cont.Strm.read((char*)&_U8, 1);
    }

    inline operator uint8_t() const
    {
        return _U8;
    }


    uint8_t _U8;
};


#endif
