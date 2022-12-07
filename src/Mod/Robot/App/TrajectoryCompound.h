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

#ifndef ROBOT_TrajectoryCompound_H
#define ROBOT_TrajectoryCompound_H

#include <App/PropertyLinks.h>

#include "TrajectoryObject.h"


namespace Robot
{

class RobotExport TrajectoryCompound : public TrajectoryObject
{
    PROPERTY_HEADER(Robot::TrajectoryObject);

public:
    /// Constructor
    TrajectoryCompound(void);
    virtual ~TrajectoryCompound();

    App::PropertyLinkList     Source;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "RobotGui::ViewProviderTrajectoryCompound";
    }
    virtual App::DocumentObjectExecReturn *execute(void);

protected:
    /// get called by the container when a property has changed
    //virtual void onChanged (const App::Property* prop);

};

} //namespace Robot


#endif // ROBOT_ROBOTOBJECT_H
