/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel (juergen.riegel@web.de)              *
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

#include "Edge2TracObject.h"
//#include <App/DocumentObjectPy.h>
//#include <Base/Placement.h>
#include <Base/Sequencer.h>
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

PROPERTY_SOURCE(Robot::Edge2TracObject, Robot::TrajectoryObject)


Edge2TracObject::Edge2TracObject()
{

    ADD_PROPERTY_TYPE( Source,      (0)  , "Edge2Trac",Prop_None,"Edges to generate the Trajectory");
    ADD_PROPERTY_TYPE( SegValue,    (0.5), "Edge2Trac",Prop_None,"Max deviation from original geometry");
    ADD_PROPERTY_TYPE( UseRotation, (0)  , "Edge2Trac",Prop_None,"use orientation of the edge");
    NbrOfEdges = 0;
    NbrOfCluster = 0;
}

Edge2TracObject::~Edge2TracObject()
{
}

App::DocumentObjectExecReturn *Edge2TracObject::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *base = static_cast<Part::Feature*>(Source.getValue());
    const Part::TopoShape& TopShape = base->Shape.getShape();

    const std::vector<std::string>& SubVals = Source.getSubValuesStartsWith("Edge");
    if (SubVals.size() == 0)
        return new App::DocumentObjectExecReturn("No Edges specified");

    // container for all the edges 
    std::vector<TopoDS_Edge> edges;

    // run through the edge name and get the real objects from the TopoShape
    for (std::vector<std::string>::const_iterator it= SubVals.begin();it!=SubVals.end();++it) {
         TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(it->c_str()));
         edges.push_back(edge);
    }

    // instantiate an edge cluster sorter and get the result 
    Part::Edgecluster acluster(edges);
    Part::tEdgeClusterVector aclusteroutput = acluster.GetClusters();

    if(aclusteroutput.size() == 0)
        return new App::DocumentObjectExecReturn("No Edges specified");

    // set the number of cluster and edges 
    NbrOfCluster = aclusteroutput.size();
    NbrOfEdges = 0;
    for(std::vector<std::vector<TopoDS_Edge> >::const_iterator it=aclusteroutput.begin();it!=aclusteroutput.end();++it)
        NbrOfEdges += it->size();

    // trajectory to fill
    Robot::Trajectory trac;
    bool first = true;

    // cycle through the cluster
    for(std::vector<std::vector<TopoDS_Edge> >::const_iterator it=aclusteroutput.begin();it!=aclusteroutput.end();++it)
    {
        // cycle through the edges of the cluster
        for(std::vector<TopoDS_Edge>::const_iterator it2=it->begin();it2!=it->end();++it2)
        {
            BRepAdaptor_Curve adapt(*it2);
            
            switch(adapt.GetType())
            {
            case GeomAbs_Line:
                {
                // get start and end point
                gp_Pnt P1 = adapt.Value(adapt.FirstParameter());
                gp_Pnt P2 = adapt.Value(adapt.LastParameter());

                Base::Rotation R1;
                Base::Rotation R2;

                // if orientation is used 
                if(UseRotation.getValue()) {
                    // here get the orientation of the start and end point...
                    //R1 = ;
                    //R2 = ;

                }

                // if reverse orintation, switch the points
                if ( it2->Orientation() == TopAbs_REVERSED )
                {
                     //switch the points and orientation
                     gp_Pnt tmpP = P1;
                     Base::Rotation tmpR = R1;
                     P1 = P2;
                     R1 = R2;
                     R2 = tmpR;
                     P2 = tmpP;
                }
                if(first){
                    Waypoint wp("Pt",Base::Placement(Base::Vector3d(P1.X(),P1.Y(),P1.Z()),R1));
                    trac.addWaypoint(wp);
                    first = false;
                }
                Waypoint wp("Pt",Base::Placement(Base::Vector3d(P2.X(),P2.Y(),P2.Z()),R2));
                trac.addWaypoint(wp);
                break;
                }
            case GeomAbs_BSplineCurve:
                {
                Standard_Real Length    = CPnts_AbscissaPoint::Length(adapt);
                Standard_Real ParLength = adapt.LastParameter()-adapt.FirstParameter();
                Standard_Real NbrSegments = Round(Length / SegValue.getValue());

                Standard_Real beg = adapt.FirstParameter();
                Standard_Real end = adapt.LastParameter();
                Standard_Real stp = ParLength / NbrSegments;
				bool reversed = false;
                if (it2->Orientation() == TopAbs_REVERSED) {
                    std::swap(beg, end);
                    stp = - stp;
					reversed = true;
                }

                if (first) 
                    first = false;
                else
                    beg += stp;
                Base::SequencerLauncher seq("Create waypoints", static_cast<size_t>((end-beg)/stp));
				if(reversed)
				{
					for (;beg > end; beg += stp) {
						gp_Pnt P = adapt.Value(beg);
                        Base::Rotation R1;
                        // if orientation is used 
                        if(UseRotation.getValue()) {
                            // here get the orientation of the start and end point...
                            //R1 = ;
                        }

						Waypoint wp("Pt",Base::Placement(Base::Vector3d(P.X(),P.Y(),P.Z()),R1));
						trac.addWaypoint(wp);
						seq.next();
					}
				}
				else
				{
					for (;beg < end; beg += stp) {
						gp_Pnt P = adapt.Value(beg);
                        Base::Rotation R1;
                        // if orientation is used 
                        if(UseRotation.getValue()) {
                            // here get the orientation of the start and end point...
                            //R1 = ;
                        }
						Waypoint wp("Pt",Base::Placement(Base::Vector3d(P.X(),P.Y(),P.Z()),R1));
						trac.addWaypoint(wp);
						seq.next();
					}
				}
                
                } break;
            case GeomAbs_Circle:
                {
                Standard_Real Length    = CPnts_AbscissaPoint::Length(adapt);
                Standard_Real ParLength = adapt.LastParameter()-adapt.FirstParameter();
                Standard_Real NbrSegments = Round(Length / SegValue.getValue());
                Standard_Real SegLength   = ParLength / NbrSegments;
				
				if ( it2->Orientation() == TopAbs_REVERSED )
				{
					//Beginning and End switch
					double i = adapt.LastParameter();
					if(first) 
						first=false;
					else
						i -= SegLength;
					for (;i>adapt.FirstParameter();i-= SegLength){
						gp_Pnt P = adapt.Value(i);
                        Base::Rotation R1;
                        // if orientation is used 
                        if(UseRotation.getValue()) {
                            // here get the orientation of the start and end point...
                            //R1 = ;
                        }
						Waypoint wp("Pt",Base::Placement(Base::Vector3d(P.X(),P.Y(),P.Z()),R1));
						trac.addWaypoint(wp);
					}
				}
				else
				{
					double i = adapt.FirstParameter();
					if(first) 
						first=false;
					else
						i += SegLength;
					for (;i<adapt.LastParameter();i+= SegLength)
					{
						gp_Pnt P = adapt.Value(i);
                        Base::Rotation R1;
                        // if orientation is used 
                        if(UseRotation.getValue()) {
                            // here get the orientation of the start and end point...
                            //R1 = ;
                        }
						Waypoint wp("Pt",Base::Placement(Base::Vector3d(P.X(),P.Y(),P.Z()),R1));
						trac.addWaypoint(wp);
					}
					
				}
				break;
                }

            default:
                throw Base::TypeError("Unknown Edge type in Robot::Edge2TracObject::execute()");
            }
           


        }
    }

    Trajectory.setValue(trac);
    
    return App::DocumentObject::StdReturn;
}


//short Edge2TracObject::mustExecute(void) const
//{
//    return 0;
//}

void Edge2TracObject::onChanged(const Property* prop)
{
 
    App::GeoFeature::onChanged(prop);
}
