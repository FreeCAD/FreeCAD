/***************************************************************************
 *   Copyright (c) 2023 Wanderer Fan <wandererfan@gmail.com>               *
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

#include "ViewProviderScale.h"
#include <Mod/Part/App/FeatureScale.h>


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderScale,PartGui::ViewProviderPart)

ViewProviderScale::ViewProviderScale()
{
    sPixmap = "Part_Scale.svg";
}

ViewProviderScale::~ViewProviderScale() = default;

std::vector<App::DocumentObject*> ViewProviderScale::claimChildren()const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(static_cast<Part::Scale*>(getObject())->Base.getValue());

    return temp;
}
