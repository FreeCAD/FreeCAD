/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/nodes/SoMaterial.h>
#endif

#include <App/PropertyStandard.h>

#include "ViewProviderBuilder.h"
#include "SoFCSelection.h"
#include "Window.h"


using namespace Gui;

std::map<Base::Type, Base::Type> ViewProviderBuilder::_prop_to_view;

ViewProviderBuilder::ViewProviderBuilder() = default;

ViewProviderBuilder::~ViewProviderBuilder() = default;

void ViewProviderBuilder::add(const Base::Type& prop, const Base::Type& view)
{
    _prop_to_view[prop] = view;
}

ViewProvider* ViewProviderBuilder::create(const Base::Type& type)
{
    std::map<Base::Type, Base::Type>::iterator it = _prop_to_view.find(type);
    if (it != _prop_to_view.end())
        return static_cast<ViewProvider*>(it->second.createInstance());
    return nullptr;
}

Gui::SoFCSelection* ViewProviderBuilder::createSelection()
{
    auto sel = new Gui::SoFCSelection();

    float transparency;
    ParameterGrp::handle hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("View");
    bool enablePre = hGrp->GetBool("EnablePreselection", true);
    bool enableSel = hGrp->GetBool("EnableSelection", true);
    if (!enablePre) {
        sel->highlightMode = Gui::SoFCSelection::OFF;
    }
    else {
        // Search for a user defined value with the current color as default
        SbColor highlightColor = sel->colorHighlight.getValue();
        auto highlight = (unsigned long)(highlightColor.getPackedValue());
        highlight = hGrp->GetUnsigned("HighlightColor", highlight);
        highlightColor.setPackedValue((uint32_t)highlight, transparency);
        sel->colorHighlight.setValue(highlightColor);
    }
    if (!enableSel) {
        sel->selectionMode = Gui::SoFCSelection::SEL_OFF;
    }
    else {
        // Do the same with the selection color
        SbColor selectionColor = sel->colorSelection.getValue();
        auto selection = (unsigned long)(selectionColor.getPackedValue());
        selection = hGrp->GetUnsigned("SelectionColor", selection);
        selectionColor.setPackedValue((uint32_t)selection, transparency);
        sel->colorSelection.setValue(selectionColor);
    }

    return sel;
}

// --------------------------------------

ViewProviderColorBuilder::ViewProviderColorBuilder() = default;

ViewProviderColorBuilder::~ViewProviderColorBuilder() = default;

void ViewProviderColorBuilder::buildNodes(const App::Property* prop, std::vector<SoNode*>& node) const
{
    const auto color = static_cast<const App::PropertyColorList*>(prop);
    const std::vector<App::Color>& val = color->getValues();
    unsigned long i=0;

    auto material = new SoMaterial();
    material->diffuseColor.setNum(val.size());

    SbColor* colors = material->diffuseColor.startEditing();
    for (const auto & it : val) {
        colors[i].setValue(it.r, it.g, it.b);
        i++;
    }
    material->diffuseColor.finishEditing();
    node.push_back(material);
}
