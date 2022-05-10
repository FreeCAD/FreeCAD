/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <QMenu>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>
#include <Mod/PartDesign/App/FeatureLoft.h>

#include "ViewProviderLoft.h"
#include "TaskLoftParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderLoft,PartDesignGui::ViewProvider)

ViewProviderLoft::ViewProviderLoft()
{
}

ViewProviderLoft::~ViewProviderLoft()
{
}

std::vector<App::DocumentObject*> ViewProviderLoft::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;

    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());

    App::DocumentObject* sketch = pcLoft->getVerifiedSketch(true);
    if (sketch != nullptr)
        temp.push_back(sketch);

    for(App::DocumentObject* obj : pcLoft->Sections.getValues()) {
        if (obj != nullptr && obj->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
            temp.push_back(obj);
    }

    return temp;
}

void ViewProviderLoft::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit loft"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderLoft::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default)
        setPreviewDisplayMode(true);

    return ViewProviderAddSub::setEdit(ModNum);
}

TaskDlgFeatureParameters* ViewProviderLoft::getEditDialog() {
    return new TaskDlgLoftParameters(this);
}


void ViewProviderLoft::unsetEdit(int ModNum) {
    setPreviewDisplayMode(false);
    ViewProviderAddSub::unsetEdit(ModNum);
}


bool ViewProviderLoft::onDelete(const std::vector<std::string> & /*s*/)
{/*
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());

    // get the Sketch
    Sketcher::SketchObject *pcSketch = 0;
    if (pcLoft->Sketch.getValue())
        pcSketch = static_cast<Sketcher::SketchObject*>(pcLoft->Sketch.getValue());

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();

    return ViewProvider::onDelete(s);*/
    return true;
}

void ViewProviderLoft::highlightProfile(bool on)
{
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());
    highlightReferences(dynamic_cast<Part::Feature*>(pcLoft->Profile.getValue()),
                        pcLoft->Profile.getSubValues(), on);
}

void ViewProviderLoft::highlightSection(bool on)
{
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());
    auto sections = pcLoft->Sections.getSubListValues();
    for (auto it : sections) {
        // only take the entire shape when we have a sketch selected, but
        // not a point of the sketch
        auto subName = it.second.empty() ? "" : it.second.front();
        if (it.first->isDerivedFrom(Part::Part2DObject::getClassTypeId()) && subName.compare(0, 6, "Vertex") != 0) {
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

void ViewProviderLoft::highlightReferences(Part::Feature* base, const std::vector<std::string>& elements, bool on)
{
    if (!base)
        return;

    PartGui::ViewProviderPart* svp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(base));
    if (!svp)
        return;

    std::vector<App::Color>& edgeColors = originalLineColors[base->getID()];

    if (on) {
        edgeColors = svp->LineColorArray.getValues();
        std::vector<App::Color> colors = edgeColors;

        PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(), svp->LineColor.getValue());
        highlighter.getEdgeColors(elements, colors);
        svp->LineColorArray.setValues(colors);
    }
    else {
        svp->LineColorArray.setValues({svp->LineColor.getValue()});
        edgeColors.clear();
    }
}

QIcon ViewProviderLoft::getIcon(void) const {
    QString str = QString::fromLatin1("PartDesign_");
    auto* prim = static_cast<PartDesign::Loft*>(getObject());
    if(prim->getAddSubType() == PartDesign::FeatureAddSub::Additive)
        str += QString::fromLatin1("Additive");
    else
        str += QString::fromLatin1("Subtractive");

    str += QString::fromLatin1("Loft.svg");
    return PartDesignGui::ViewProvider::mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap(str.toStdString().c_str()));
}

