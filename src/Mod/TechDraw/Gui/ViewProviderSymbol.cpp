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

#include <App/DocumentObject.h>
#include "ViewProviderSymbol.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderSymbol, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderSymbol::ViewProviderSymbol()
{
    sPixmap = "TechDraw_TreeSymbol";
}

ViewProviderSymbol::~ViewProviderSymbol()
{
}

void ViewProviderSymbol::updateData(const App::Property* prop)
{
    if (prop == &getViewObject()->Scale) {
        onGuiRepaint(getViewObject());
    } else if (prop == &getViewObject()->Rotation) {
        onGuiRepaint(getViewObject());
    } else if (prop == &getViewObject()->Symbol) {
        onGuiRepaint(getViewObject());
    } else if (prop == &getViewObject()->EditableTexts) {
        onGuiRepaint(getViewObject());
    }
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
    sPixmap = "actions/TechDraw_DraftView.svg";
}

ViewProviderDraft::~ViewProviderDraft()
{
}

//**************************************************************************
// Arch view

PROPERTY_SOURCE(TechDrawGui::ViewProviderArch, TechDrawGui::ViewProviderSymbol)


ViewProviderArch::ViewProviderArch()
{
    sPixmap = "actions/TechDraw_ArchView.svg";
}

ViewProviderArch::~ViewProviderArch()
{
}
