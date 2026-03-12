// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include <QMenu>


#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>
#include <Mod/PartDesign/App/FeatureLoft.h>

#include "ViewProviderLoft.h"
#include "TaskLoftParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderLoft, PartDesignGui::ViewProvider)

ViewProviderLoft::ViewProviderLoft() = default;

ViewProviderLoft::~ViewProviderLoft() = default;

std::vector<App::DocumentObject*> ViewProviderLoft::claimChildren() const
{
    std::vector<App::DocumentObject*> temp;

    PartDesign::Loft* pcLoft = getObject<PartDesign::Loft>();

    App::DocumentObject* sketch = pcLoft->getVerifiedSketch(true);
    if (sketch) {
        temp.push_back(sketch);
    }

    for (App::DocumentObject* obj : pcLoft->Sections.getValues()) {
        if (obj && obj->isDerivedFrom<Part::Part2DObject>()) {
            temp.push_back(obj);
        }
    }

    return temp;
}

void ViewProviderLoft::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Loft"));
    ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderLoft::getEditDialog()
{
    return new TaskDlgLoftParameters(this);
}

void ViewProviderLoft::highlightProfile(bool on)
{
    PartDesign::Loft* pcLoft = getObject<PartDesign::Loft>();
    highlightReferences(
        dynamic_cast<Part::Feature*>(pcLoft->Profile.getValue()),
        pcLoft->Profile.getSubValues(),
        on
    );
}

void ViewProviderLoft::highlightSection(bool on)
{
    PartDesign::Loft* pcLoft = getObject<PartDesign::Loft>();
    auto sections = pcLoft->Sections.getSubListValues();
    for (auto& it : sections) {
        // only take the entire shape when we have a sketch selected, but
        // not a point of the sketch
        auto subName = it.second.empty() ? "" : it.second.front();
        if (it.first->isDerivedFrom<Part::Part2DObject>() && subName.compare(0, 6, "Vertex") != 0) {
            it.second.clear();
        }
        highlightReferences(dynamic_cast<Part::Feature*>(it.first), it.second, on);
    }
}

void ViewProviderLoft::highlightReferences(ViewProviderLoft::Reference mode, bool on)
{
    switch (mode) {
        case Profile:
            highlightProfile(on);
            break;
        case Section:
            highlightSection(on);
            break;
        case Both:
            highlightProfile(on);
            highlightSection(on);
            break;
        default:
            break;
    }
}

void ViewProviderLoft::highlightReferences(
    Part::Feature* base,
    const std::vector<std::string>& elements,
    bool on
)
{
    if (!base) {
        return;
    }

    PartGui::ViewProviderPart* svp = dynamic_cast<PartGui::ViewProviderPart*>(
        Gui::Application::Instance->getViewProvider(base)
    );
    if (!svp) {
        return;
    }

    std::vector<Base::Color>& edgeColors = originalLineColors[base->getID()];

    if (on) {
        edgeColors = svp->LineColorArray.getValues();
        std::vector<Base::Color> colors = edgeColors;

        PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(), svp->LineColor.getValue());
        highlighter.getEdgeColors(elements, colors);
        svp->LineColorArray.setValues(colors);
    }
    else {
        svp->LineColorArray.setValues({svp->LineColor.getValue()});
        edgeColors.clear();
    }
}

QIcon ViewProviderLoft::getIcon() const
{
    QString str = QStringLiteral("PartDesign_");
    auto* prim = getObject<PartDesign::Loft>();
    if (prim->getAddSubType() == PartDesign::FeatureAddSub::Additive) {
        str += QStringLiteral("Additive");
    }
    else {
        str += QStringLiteral("Subtractive");
    }

    str += QStringLiteral("Loft.svg");
    return PartDesignGui::ViewProvider::mergeGreyableOverlayIcons(
        Gui::BitmapFactory().pixmap(str.toStdString().c_str())
    );
}
