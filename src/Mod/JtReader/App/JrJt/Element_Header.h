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


#ifndef Element_Header_HEADER
#define Element_Header_HEADER

#include <assert.h>
#include <istream>
#include <stdint.h>

#include "Context.h"
#include "GUID.h"
#include "I32.h"
#include "UChar.h"


using namespace std;


struct Element_Header
{
    Element_Header() {};

    Element_Header(Context& cont, bool zLib = false)
    {
        read(cont, zLib);
    };

    inline void read(Context& cont, bool zLib = false)
    {
        // only zip less implemented so far...
        assert(zLib == false);

        Element_Length.read(cont);
        Object_Type_ID.read(cont);
        Object_Base_Type.read(cont);
    };

    I32 Element_Length;
    GUID Object_Type_ID;
    UChar Object_Base_Type;
};


#endif
