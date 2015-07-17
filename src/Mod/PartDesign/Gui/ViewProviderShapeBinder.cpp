/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <Mod/PartDesign/App/ShapeBinder.h>

#include "ViewProviderShapeBinder.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderShapeBinder,PartGui::ViewProviderPart)

ViewProviderShapeBinder::ViewProviderShapeBinder()
{
    sPixmap = "PartDesign_ShapeBinder.svg";
}

ViewProviderShapeBinder::~ViewProviderShapeBinder()
{

}

bool ViewProviderShapeBinder::setEdit(int ModNum) {
    return true;//PartGui::ViewProviderPartExt::setEdit(ModNum);
}

void ViewProviderShapeBinder::unsetEdit(int ModNum) {
    return;//PartGui::ViewProviderPartExt::unsetEdit(ModNum);
}
