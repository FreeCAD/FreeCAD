// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <Gui/Selection/SoFCSelection.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Mod/Robot/RobotGlobal.h>


class SoCoordinate3;
class SoDragger;
class SoDrawStyle;
class SoJackDragger;
class SoLineSet;

namespace RobotGui
{

class RobotGuiExport ViewProviderTrajectory: public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(RobotGui::ViewProviderTrajectory);

public:
    /// constructor.
    ViewProviderTrajectory();

    /// destructor.
    ~ViewProviderTrajectory() override;

    void attach(App::DocumentObject* pcObject) override;
    void setDisplayMode(const char* ModeName) override;
    std::vector<std::string> getDisplayModes() const override;
    void updateData(const App::Property*) override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

protected:
    Gui::SoFCSelection* pcTrajectoryRoot;
    SoCoordinate3* pcCoords;
    SoDrawStyle* pcDrawStyle;
    SoLineSet* pcLines;
};

}  // namespace RobotGui
