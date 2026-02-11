// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#include "ViewProviderMainPart.h"
#include <Mod/PartDesign/App/FeatureMainPart.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderMainPart, PartGui::ViewProviderPart)

ViewProviderMainPart::ViewProviderMainPart()
{}

ViewProviderMainPart::~ViewProviderMainPart()
{}

std::vector<App::DocumentObject*> ViewProviderMainPart::claimChildren(void) const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(getObject<PartDesign::MainPart>()->Sketch.getValue());

    return temp;
}
