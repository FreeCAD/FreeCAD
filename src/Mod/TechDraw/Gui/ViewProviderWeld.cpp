/***************************************************************************
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/SoFCSelection.h>
#include <Gui/Selection.h>

#include "ViewProviderWeld.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderWeld, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderWeld::ViewProviderWeld()
{
    sPixmap = "actions/techdraw-weldsymbol";
}

ViewProviderWeld::~ViewProviderWeld()
{
}

void ViewProviderWeld::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);
}

void ViewProviderWeld::setDisplayMode(const char* ModeName)
{
    ViewProviderDrawingView::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderWeld::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDrawingView::getDisplayModes();

    return StrList;
}

void ViewProviderWeld::updateData(const App::Property* prop)
{
    ViewProviderDrawingView::updateData(prop);
}

std::vector<App::DocumentObject*> ViewProviderWeld::claimChildren(void) const
{
    // Collect any child Document Objects and put them in the right place in the Feature tree
    // valid children of a DrawWeldSymbol are:
    //    - DrawTiles
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &tiles = getFeature()->getInList();
    try {
        for(std::vector<App::DocumentObject *>::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawTile::getClassTypeId())) {
                temp.push_back((*it));
            }
        }
      return temp;
    } catch (...) {
        std::vector<App::DocumentObject*> tmp;
        return tmp;
    }
}

TechDraw::DrawWeldSymbol* ViewProviderWeld::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawWeldSymbol*>(pcObject);
}

TechDraw::DrawWeldSymbol* ViewProviderWeld::getFeature() const
{
    return getViewObject();
}
