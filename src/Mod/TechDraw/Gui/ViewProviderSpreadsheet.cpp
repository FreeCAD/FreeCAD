/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include "ViewProviderSpreadsheet.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderSpreadsheet, TechDrawGui::ViewProviderSymbol)

//**************************************************************************
// Construction/Destruction

ViewProviderSpreadsheet::ViewProviderSpreadsheet()
{
    sPixmap = "TechDraw_Tree_Spreadsheet";
}

ViewProviderSpreadsheet::~ViewProviderSpreadsheet()
{
}

void ViewProviderSpreadsheet::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderSymbol::attach(pcFeat);
}

void ViewProviderSpreadsheet::setDisplayMode(const char* ModeName)
{
    ViewProviderSymbol::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderSpreadsheet::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderSymbol::getDisplayModes();

    return StrList;
}

void ViewProviderSpreadsheet::updateData(const App::Property* prop)
{
    ViewProviderSymbol::updateData(prop);
}

TechDraw::DrawViewSpreadsheet* ViewProviderSpreadsheet::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewSpreadsheet*>(pcObject);
}
