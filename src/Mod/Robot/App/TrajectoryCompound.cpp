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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "TrajectoryCompound.h"
//#include <App/DocumentObjectPy.h>
//#include <Base/Placement.h>
#include <Mod/Part/App/edgecluster.h>
#include <Mod/Part/App/PartFeature.h>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <CPnts_AbscissaPoint.hxx>
#include <TopExp.hxx>
#include "Waypoint.h"
#include "Trajectory.h"

using namespace Robot;
using namespace App;

PROPERTY_SOURCE(Robot::TrajectoryCompound, Robot::TrajectoryObject)


TrajectoryCompound::TrajectoryCompound()
{

    ADD_PROPERTY_TYPE( Source,      (nullptr)   , "Compound",Prop_None,"list of trajectories to combine");

}

TrajectoryCompound::~TrajectoryCompound()
{
}

App::DocumentObjectExecReturn *TrajectoryCompound::execute(void)
{
    const std::vector<DocumentObject*> &Tracs = Source.getValues();
    Robot::Trajectory result;

    for (std::vector<DocumentObject*>::const_iterator it= Tracs.begin();it!=Tracs.end();++it) {
        if ((*it)->getTypeId().isDerivedFrom(Robot::TrajectoryObject::getClassTypeId())){
            const std::vector<Waypoint*> &wps = static_cast<Robot::TrajectoryObject*>(*it)->Trajectory.getValue().getWaypoints();
            for (std::vector<Waypoint*>::const_iterator it2= wps.begin();it2!=wps.end();++it2) {
                result.addWaypoint(**it2);
            }
        }else
            return new App::DocumentObjectExecReturn("Not all objects in compound are trajectories!");
    }

    Trajectory.setValue(result);
    
    return App::DocumentObject::StdReturn;
}


//short TrajectoryCompound::mustExecute(void) const
//{
//    return 0;
//}

//void TrajectoryCompound::onChanged(const Property* prop)
//{
// 
//    App::GeoFeature::onChanged(prop);
//}
