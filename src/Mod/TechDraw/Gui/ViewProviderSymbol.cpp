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

#include "ViewProviderSymbol.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderSymbol, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderSymbol::ViewProviderSymbol()
{
    sPixmap = "TechDraw_Tree_Symbol";
}

ViewProviderSymbol::~ViewProviderSymbol()
{
}

void ViewProviderSymbol::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);
}

void ViewProviderSymbol::setDisplayMode(const char* ModeName)
{
    ViewProviderDrawingView::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderSymbol::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDrawingView::getDisplayModes();

    return StrList;
}

void ViewProviderSymbol::updateData(const App::Property* prop)
{
    ViewProviderDrawingView::updateData(prop);
}

TechDraw::DrawViewSymbol* ViewProviderSymbol::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewSymbol*>(pcObject);
}

//**************************************************************************
// Draft view

PROPERTY_SOURCE(TechDrawGui::ViewProviderDraft, TechDrawGui::ViewProviderSymbol)


ViewProviderDraft::ViewProviderDraft()
{
    sPixmap = "actions/techdraw-DraftView.svg";
}

ViewProviderDraft::~ViewProviderDraft()
{
}

//**************************************************************************
// Arch view

PROPERTY_SOURCE(TechDrawGui::ViewProviderArch, TechDrawGui::ViewProviderSymbol)


ViewProviderArch::ViewProviderArch()
{
    sPixmap = "actions/techdraw-ArchView.svg";
}

ViewProviderArch::~ViewProviderArch()
{
}
