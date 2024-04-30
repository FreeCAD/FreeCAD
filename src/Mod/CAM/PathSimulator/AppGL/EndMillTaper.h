/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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

#ifndef __end_mill_taper_h__
#define __end_mill_taper_h__

#include "EndMill.h"

#define TAPER_MILL_PROFILE_VERTS 4

namespace MillSim
{
    class EndMillTaper :
        public EndMill
    {
    public:
        EndMillTaper(int toolid, float radius, int nslices, float TaperAngle, float flatRadius);

    private:
        float _profVerts[PROFILE_BUFFER_SIZE(TAPER_MILL_PROFILE_VERTS)];
    };
}
#endif
