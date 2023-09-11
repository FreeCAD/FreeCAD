/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <QAction>
#include <QMenu>
#include <sstream>

#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoSeparator.h>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Mod/Robot/App/TrajectoryObject.h>

#include "ViewProviderTrajectory.h"


using namespace Gui;
using namespace RobotGui;
using namespace Robot;

PROPERTY_SOURCE(RobotGui::ViewProviderTrajectory, Gui::ViewProviderGeometryObject)

ViewProviderTrajectory::ViewProviderTrajectory()
{

    pcTrajectoryRoot = new Gui::SoFCSelection();
    pcTrajectoryRoot->highlightMode = Gui::SoFCSelection::OFF;
    pcTrajectoryRoot->selectionMode = Gui::SoFCSelection::SEL_OFF;
    // pcRobotRoot->style = Gui::SoFCSelection::BOX;
    pcTrajectoryRoot->ref();

    pcCoords = new SoCoordinate3();
    pcCoords->ref();
    pcDrawStyle = new SoDrawStyle();
    pcDrawStyle->ref();
    pcDrawStyle->style = SoDrawStyle::LINES;
    pcDrawStyle->lineWidth = 2;

    pcLines = new SoLineSet;
    pcLines->ref();
}

ViewProviderTrajectory::~ViewProviderTrajectory()
{
    pcTrajectoryRoot->unref();
    pcCoords->unref();
    pcDrawStyle->unref();
    pcLines->unref();
}

void ViewProviderTrajectory::attach(App::DocumentObject* pcObj)
{
    ViewProviderGeometryObject::attach(pcObj);

    // Draw trajectory lines
    SoSeparator* linesep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    linesep->addChild(basecol);
    linesep->addChild(pcCoords);
    linesep->addChild(pcLines);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "CROSS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 5));
    linesep->addChild(markcol);
    linesep->addChild(marker);

    pcTrajectoryRoot->addChild(linesep);

    addDisplayMaskMode(pcTrajectoryRoot, "Waypoints");
    pcTrajectoryRoot->objectName = pcObj->getNameInDocument();
    pcTrajectoryRoot->documentName = pcObj->getDocument()->getName();
    pcTrajectoryRoot->subElementName = "Main";
}

void ViewProviderTrajectory::setDisplayMode(const char* ModeName)
{
    if (strcmp("Waypoints", ModeName) == 0) {
        setDisplayMaskMode("Waypoints");
    }
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderTrajectory::getDisplayModes() const
{
    std::vector<std::string> StrList;
    StrList.emplace_back("Waypoints");
    return StrList;
}

void ViewProviderTrajectory::updateData(const App::Property* prop)
{
    Robot::TrajectoryObject* pcTracObj = static_cast<Robot::TrajectoryObject*>(pcObject);
    if (prop == &pcTracObj->Trajectory) {
        const Trajectory& trak = pcTracObj->Trajectory.getValue();

        pcCoords->point.deleteValues(0);
        pcCoords->point.setNum(trak.getSize());

        for (unsigned int i = 0; i < trak.getSize(); ++i) {
            Base::Vector3d pos = trak.getWaypoint(i).EndPos.getPosition();
            pcCoords->point.set1Value(i, pos.x, pos.y, pos.z);
        }
        pcLines->numVertices.set1Value(0, trak.getSize());
    }
    else if (prop == &pcTracObj->Base) {
        Base::Placement loc = *(&pcTracObj->Base.getValue());
    }
}

void ViewProviderTrajectory::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act = menu->addAction(QObject::tr("Modify"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
}
