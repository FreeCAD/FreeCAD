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

#include <Mod/TechDraw/App/Preferences.h>

#include "QGIViewSymbol.h"
#include "ViewProviderSymbol.h"

using namespace TechDrawGui;
using namespace TechDraw;


PROPERTY_SOURCE(TechDrawGui::ViewProviderSymbol, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderSymbol::ViewProviderSymbol()
{
    sPixmap = "TechDraw_TreeSymbol";

    ADD_PROPERTY_TYPE(LegacyScaling, (Preferences::useLegacySvgScaling()), "Svg Scaling", App::Prop_None,
      "If true, Svg will be scaled using the original (v1.0 and earlier) scaling method.\
 If false, a more accurate method will be used.");

}

void ViewProviderSymbol::updateData(const App::Property* prop)
{
    TechDraw::DrawViewSymbol *obj = getViewObject();
    if (prop == &obj->Scale
        || prop == &obj->Rotation
        || prop == &obj->Symbol
        || prop == &obj->EditableTexts) {
        onGuiRepaint(obj);
    }

    ViewProviderDrawingView::updateData(prop);
}

void ViewProviderSymbol::onChanged(const App::Property* prop)
{
    if (prop == &(LegacyScaling)) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }

    ViewProviderDrawingView::onChanged(prop);
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
    // svg files from Draft/BIM arrive in old scale
    LegacyScaling.setValue(true);
}


//**************************************************************************
// Arch view

PROPERTY_SOURCE(TechDrawGui::ViewProviderArch, TechDrawGui::ViewProviderSymbol)


ViewProviderArch::ViewProviderArch()
{
    sPixmap = "actions/TechDraw_ArchView.svg";
    // svg files from Draft/BIM arrive in old scale
    LegacyScaling.setValue(true);
}

