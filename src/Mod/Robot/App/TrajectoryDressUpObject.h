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

#ifndef ROBOT_TrajectoryDressUpObject_H
#define ROBOT_TrajectoryDressUpObject_H

#include <App/PropertyGeo.h>
#include <App/PropertyUnits.h>

#include "TrajectoryObject.h"


namespace Robot
{

class RobotExport TrajectoryDressUpObject: public TrajectoryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Robot::TrajectoryObject);

public:
    /// Constructor
    TrajectoryDressUpObject();

    App::PropertyLink Source;
    App::PropertySpeed Speed;
    App::PropertyBool UseSpeed;
    App::PropertyAcceleration Acceleration;
    App::PropertyBool UseAcceleration;
    App::PropertyEnumeration ContType;
    App::PropertyPlacement PosAdd;
    App::PropertyEnumeration AddType;


    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "RobotGui::ViewProviderTrajectoryDressUp";
    }
    App::DocumentObjectExecReturn* execute() override;

    static const char* ContTypeEnums[];
    static const char* AddTypeEnums[];

protected:
    /// get called by the container when a property has changed
    void onChanged(const App::Property* prop) override;
};

}  // namespace Robot


#endif  // ROBOT_ROBOTOBJECT_H
