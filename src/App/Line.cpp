/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2015     *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif


#include "Line.h"

using namespace App;


PROPERTY_SOURCE(App::Line, App::GeoFeature)


//===========================================================================
// Feature
//===========================================================================

Line::Line(void)
{
    ADD_PROPERTY(LineType,(""));
    //placement can't be changed
    Placement.StatusBits.set(3, true);
    //line can not be deleted by user
    StatusBits.set(ObjectStatus::Undeletable, true);   

}

Line::~Line(void)
{
}

Base::BoundBox3d Line::getBoundBox()
{
    return Base::BoundBox3d(-10, -10, -10, 10, 10, 10);
}






