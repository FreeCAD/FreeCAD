/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <QAction>
# include <QMenu>
#endif

#include <Mod/PartDesign/App/FeatureHelix.h>
#include <Gui/BitmapFactory.h>

#include <Gui/Application.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "TaskHelixParameters.h"
#include "ViewProviderHelix.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderHelix,PartDesignGui::ViewProvider)


ViewProviderHelix::ViewProviderHelix() = default;

ViewProviderHelix::~ViewProviderHelix() = default;

void ViewProviderHelix::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit helix"));
    PartDesignGui::ViewProviderAddSub::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters *ViewProviderHelix::getEditDialog()
{
    return new TaskDlgHelixParameters( this );
}

QIcon ViewProviderHelix::getIcon() const {
    QString str = QString::fromLatin1("PartDesign_");
    auto* prim = static_cast<PartDesign::Helix*>(getObject());
    if(prim->getAddSubType() == PartDesign::FeatureAddSub::Additive)
        str += QString::fromLatin1("Additive");
    else
        str += QString::fromLatin1("Subtractive");

    str += QString::fromLatin1("Helix.svg");
    return PartDesignGui::ViewProvider::mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap(str.toStdString().c_str()));
}

bool ViewProviderHelix::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        auto* prim = static_cast<PartDesign::Helix*>(getObject());
        setPreviewDisplayMode(TaskHelixParameters::showPreview(prim));
    }
    return ViewProviderAddSub::setEdit(ModNum);
}

void ViewProviderHelix::unsetEdit(int ModNum)
{
    setPreviewDisplayMode(false);
    // Rely on parent class to:
    // restitute old workbench (set setEdit above) and close the dialog if exiting editing
    PartDesignGui::ViewProvider::unsetEdit(ModNum);
}

std::vector<App::DocumentObject*> ViewProviderHelix::claimChildren() const {
    std::vector<App::DocumentObject*> temp;
    App::DocumentObject* sketch = static_cast<PartDesign::ProfileBased*>(getObject())->Profile.getValue();
    if (sketch && sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        temp.push_back(sketch);

    return temp;
}

bool ViewProviderHelix::onDelete(const std::vector<std::string> &s) {
    PartDesign::ProfileBased* feature = static_cast<PartDesign::ProfileBased*>(getObject());

    // get the Sketch
    Sketcher::SketchObject *pcSketch = nullptr;
    if (feature->Profile.getValue())
        pcSketch = static_cast<Sketcher::SketchObject*>(feature->Profile.getValue());

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();

    return ViewProvider::onDelete(s);
}

