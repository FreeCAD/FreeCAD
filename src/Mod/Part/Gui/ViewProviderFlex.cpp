// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            *
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


#include "ViewProviderFlex.h"
#include <Mod/Part/App/FeatureFlex.h>


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderFlex, PartGui::ViewProviderPart)

ViewProviderFlex::ViewProviderFlex()
{
    sPixmap = "Part_Scale.svg";
}

ViewProviderFlex::~ViewProviderFlex() = default;

std::vector<App::DocumentObject*> ViewProviderFlex::claimChildren() const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(getObject<Part::Flex>()->Base.getValue());

    return temp;
}
