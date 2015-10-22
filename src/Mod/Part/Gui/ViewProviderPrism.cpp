/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Base/Parameter.h>
#include "ViewProviderPrism.h"

using namespace PartGui;
using namespace std;


//**************************************************************************
// Construction/Destruction

PROPERTY_SOURCE(PartGui::ViewProviderPrism, PartGui::ViewProviderPart)


ViewProviderPrism::ViewProviderPrism()
{
    sPixmap = "Tree_Part_Prism.svg";
}

ViewProviderPrism::~ViewProviderPrism()
{

}

// **********************************************************************************

std::vector<std::string> ViewProviderPrism::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList;

    // add your own modes
    StrList.push_back("Flat Lines");
    StrList.push_back("Shaded");
    StrList.push_back("Wireframe");
    StrList.push_back("Points");

    return StrList;
}

//**************************************************************************
// Construction/Destruction

PROPERTY_SOURCE(PartGui::ViewProviderWedge, PartGui::ViewProviderPart)


ViewProviderWedge::ViewProviderWedge()
{
    sPixmap = "Tree_Part_Wedge.svg";
}

ViewProviderWedge::~ViewProviderWedge()
{

}

// **********************************************************************************

std::vector<std::string> ViewProviderWedge::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList;

    // add your own modes
    StrList.push_back("Flat Lines");
    StrList.push_back("Shaded");
    StrList.push_back("Wireframe");
    StrList.push_back("Points");

    return StrList;
}
