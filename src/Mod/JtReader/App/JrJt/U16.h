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


#ifndef U16_HEADER
#define U16_HEADER

#include "Context.h"
#include <istream>
#include <stdint.h>

using namespace std;

struct U16
{
    U16() {};

    U16(uint16_t ui)
        : _U16(ui)
    {}

    U16(Context& cont)
    {
        read(cont);
    }

    inline void read(Context& cont)
    {
        cont.Strm.read((char*)&_U16, 2);
    }

    inline operator uint16_t() const
    {
        return _U16;
    }

    uint16_t _U16;
};


#endif
