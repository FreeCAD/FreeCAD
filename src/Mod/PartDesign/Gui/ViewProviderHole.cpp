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
# include <QMenu>
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Mod/PartDesign/App/FeatureHole.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ViewProviderHole.h"
#include "TaskHoleParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderHole,PartDesignGui::ViewProvider)

ViewProviderHole::ViewProviderHole()
{
    sPixmap = "PartDesign_Hole.svg";
}

ViewProviderHole::~ViewProviderHole() = default;

std::vector<App::DocumentObject*> ViewProviderHole::claimChildren()const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(getObject<PartDesign::Hole>()->Profile.getValue());

    return temp;
}

void ViewProviderHole::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Hole"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderHole::onDelete(const std::vector<std::string>& s)
{
    // get the Sketch
    PartDesign::Hole* pcHole = getObject<PartDesign::Hole>();
    Sketcher::SketchObject* pcSketch = nullptr;
    if (pcHole->Profile.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcHole->Profile.getValue());
    }

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch)) {
        Gui::Application::Instance->getViewProvider(pcSketch)->show();
    }

    return ViewProvider::onDelete(s);
}

TaskDlgFeatureParameters* ViewProviderHole::getEditDialog()
{
    return new TaskDlgHoleParameters(this);
}
