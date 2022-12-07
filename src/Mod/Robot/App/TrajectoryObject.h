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

#ifndef ROBOT_TrajectoryObject_H
#define ROBOT_TrajectoryObject_H

#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>

#include "Trajectory.h"
#include "PropertyTrajectory.h"


namespace Robot
{

class RobotExport TrajectoryObject : public App::GeoFeature
{
    PROPERTY_HEADER(Robot::TrajectoryObject);

public:
    /// Constructor
    TrajectoryObject(void);
    virtual ~TrajectoryObject();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "RobotGui::ViewProviderTrajectory";
    }
    virtual App::DocumentObjectExecReturn *execute(void) {
        return App::DocumentObject::StdReturn;
    }
    virtual short mustExecute(void) const;
    virtual PyObject *getPyObject(void);

	App::PropertyPlacement Base;
	PropertyTrajectory     Trajectory;


protected:
    /// get called by the container when a property has changed
    virtual void onChanged (const App::Property* prop);

};

} //namespace Robot


#endif // ROBOT_ROBOTOBJECT_H
