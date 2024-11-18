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

#ifndef Segment_Header_HEADER
#define Segment_Header_HEADER

#include <istream>
#include <stdint.h>

#include "Context.h"
#include "GUID.h"
#include "I32.h"
#include "U16.h"
#include "U32.h"


using namespace std;


struct Segment_Header
{
    Segment_Header() {};

    Segment_Header(Context& cont)
    {
        read(cont);
    };

    inline void read(Context& cont)
    {
        Segment_ID.read(cont);
        Segment_Type.read(cont);
        Segment_Length.read(cont);
    };

    GUID Segment_ID;
    I32 Segment_Type;
    I32 Segment_Length;
};


#endif
