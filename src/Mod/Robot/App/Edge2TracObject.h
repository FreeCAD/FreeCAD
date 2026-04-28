// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <App/PropertyGeo.h>

#include <Mod/Robot/RobotGlobal.h>

#include "TrajectoryObject.h"


namespace Robot
{

class RobotExport Edge2TracObject: public TrajectoryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Robot::TrajectoryObject);

public:
    /// Constructor
    Edge2TracObject();

    App::PropertyLinkSub Source;
    App::PropertyFloatConstraint SegValue;
    App::PropertyBool UseRotation;

    /// set by execute with the number of clusters found
    int NbrOfCluster;
    /// set by execute with the number of all edges
    int NbrOfEdges;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "RobotGui::ViewProviderEdge2TracObject";
    }
    App::DocumentObjectExecReturn* execute() override;

protected:
    /// get called by the container when a property has changed
    void onChanged(const App::Property* prop) override;
};

}  // namespace Robot
