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

#ifndef ROBOT_ROBOTOBJECT_H
#define ROBOT_ROBOTOBJECT_H

#include <App/GeoFeature.h>
#include <App/PropertyFile.h>

#include "Robot6Axis.h"


namespace Robot
{

class RobotExport RobotObject: public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Robot::RobotObject);

public:
    /// Constructor
    RobotObject();

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "RobotGui::ViewProviderRobotObject";
    }
    App::DocumentObjectExecReturn* execute() override
    {
        return App::DocumentObject::StdReturn;
    }
    short mustExecute() const override;
    PyObject* getPyObject() override;

    void Save(Base::Writer& /*writer*/) const override;
    void Restore(Base::XMLReader& /*reader*/) override;

    Robot6Axis& getRobot()
    {
        return robot;
    }

    App::PropertyFileIncluded RobotVrmlFile;
    App::PropertyFileIncluded RobotKinematicFile;

    App::PropertyFloat Axis1, Axis2, Axis3, Axis4, Axis5, Axis6;

    App::PropertyPlacement Base;
    App::PropertyPlacement Tool;
    App::PropertyLink ToolShape;
    App::PropertyPlacement ToolBase;
    App::PropertyPlacement Tcp;
    // App::PropertyPlacement Position;

    App::PropertyString Error;
    App::PropertyFloatList Home;

protected:
    /// get called by the container when a property has changed
    void onChanged(const App::Property* prop) override;

    Robot6Axis robot;

    bool block {false};
};

}  // namespace Robot


#endif  // ROBOT_ROBOTOBJECT_H
