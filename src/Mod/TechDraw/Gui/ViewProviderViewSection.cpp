/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
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

#include <Mod/TechDraw/App/DrawView.h>
#include "ViewProviderViewSection.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderViewSection, TechDrawGui::ViewProviderViewPart)

//**************************************************************************
// Construction/Destruction

ViewProviderViewSection::ViewProviderViewSection()
{
    sPixmap = "Section";
}

ViewProviderViewSection::~ViewProviderViewSection()
{
}

void ViewProviderViewSection::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderViewSection::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderViewSection::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();

    return StrList;
}

void ViewProviderViewSection::updateData(const App::Property* prop)
{
    //Base::Console().Log("ViewProviderViewSection::updateData - Update View: %s\n",prop->getName());
    //
}

std::vector<App::DocumentObject*> ViewProviderViewSection::claimChildren(void) const
{
    return ViewProviderViewPart::claimChildren();
}

TechDraw::DrawView* ViewProviderViewSection::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawView*>(pcObject);
}
