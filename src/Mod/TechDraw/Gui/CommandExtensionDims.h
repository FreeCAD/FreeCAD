/***************************************************************************
 *   Copyright (c) 2021 edi                                                *
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

#include <Mod/TechDraw/TechDrawGlobal.h>

namespace TechDraw
{
    class ReferenceEntry;
    class DrawViewDimension;
}

namespace TechDrawGui {
    TechDraw::DrawViewDimension* makeArcLengthDimension(const TechDraw::ReferenceEntry& ref);

    std::vector<TechDraw::DrawViewDimension*> makeObliqueChainDimension(std::vector<TechDraw::ReferenceEntry> refs);
    std::vector<TechDraw::DrawViewDimension*> makeObliqueCoordDimension(std::vector<TechDraw::ReferenceEntry> refs);
}