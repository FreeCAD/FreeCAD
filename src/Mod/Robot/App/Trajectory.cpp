/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <memory>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>

#include "kdl_cp/chain.hpp"
#include "kdl_cp/path_line.hpp"
#include "kdl_cp/path_roundedcomposite.hpp"
#include "kdl_cp/trajectory_composite.hpp"
#include "kdl_cp/rotational_interpolation_sa.hpp"
#include "kdl_cp/velocityprofile_trap.hpp"
#include "kdl_cp/trajectory_segment.hpp"
#include "kdl_cp/path_roundedcomposite.hpp"
#include "kdl_cp/utilities/error.h"

#include "Trajectory.h"
#include "RobotAlgos.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
    #define M_PI    3.14159265358979323846 /* pi */
#endif

#ifndef M_PI_2
    #define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif

using namespace Robot;
using namespace Base;
//using namespace KDL;


TYPESYSTEM_SOURCE(Robot::Trajectory , Base::Persistence);

Trajectory::Trajectory()
:pcTrajectory(0)
{

}

Trajectory::Trajectory(const Trajectory& Trac)
:vpcWaypoints(Trac.vpcWaypoints.size()),pcTrajectory(0)
{
    operator=(Trac);
}

Trajectory::~Trajectory()
{
    for(std::vector<Waypoint*>::iterator it = vpcWaypoints.begin();it!=vpcWaypoints.end();++it)
        delete ( *it );
    delete pcTrajectory;
}

Trajectory &Trajectory::operator=(const Trajectory& Trac)
{
    if (this == &Trac)
        return *this;

    for(std::vector<Waypoint*>::iterator it = vpcWaypoints.begin();it!=vpcWaypoints.end();++it)
        delete ( *it );
    vpcWaypoints.clear();
    vpcWaypoints.resize(Trac.vpcWaypoints.size());

    int i=0;
    for (std::vector<Waypoint*>::const_iterator it=Trac.vpcWaypoints.begin();it!=Trac.vpcWaypoints.end();++it,i++)
        vpcWaypoints[i] = new Waypoint(**it);

    generateTrajectory();
    return *this;
}

double Trajectory::getLength(int n) const
{
    if(pcTrajectory)
        if(n<0)
            return pcTrajectory->GetPath()->PathLength();
        else
            return pcTrajectory->Get(n)->GetPath()->PathLength();
    else
        return 0;
}

double Trajectory::getDuration(int n) const
{
    if(pcTrajectory)
        if(n<0)
            return pcTrajectory->Duration();
        else
            return pcTrajectory->Get(n)->Duration();
    else
        return 0;
}

Placement Trajectory::getPosition(double time)const
{
    if(pcTrajectory)
        return Placement(toPlacement(pcTrajectory->Pos(time)));
    else
        return Placement();
}

double Trajectory::getVelocity(double time)const
{
    if(pcTrajectory){
        KDL::Vector vec = pcTrajectory->Vel(time).vel;
        Base::Vector3d vec2(vec[0],vec[1],vec[2]);
        return vec2.Length();
    }else
        return 0;
}

void Trajectory::deleteLast(unsigned int n)
{
    for(unsigned int i=0;i<=n;i++){
        delete(*vpcWaypoints.rbegin());
        vpcWaypoints.pop_back();
    }

}

void Trajectory::generateTrajectory(void)
{
    if (vpcWaypoints.size()==0)
        return;

    // delete the old and create a new one
    if (pcTrajectory)
        delete (pcTrajectory);
    pcTrajectory = new KDL::Trajectory_Composite();

    // pointer to the pieces while iterating
    std::unique_ptr<KDL::Trajectory_Segment> pcTrak;
    std::unique_ptr<KDL::VelocityProfile> pcVelPrf;
    std::unique_ptr<KDL::Path_RoundedComposite> pcRoundComp;
    KDL::Frame Last;

    try {
        // handle the first waypoint special
        bool first=true;

        for (std::vector<Waypoint*>::const_iterator it = vpcWaypoints.begin();it!=vpcWaypoints.end();++it) {
            if (first) {
                Last = toFrame((*it)->EndPos);
                first = false;
            }
            else {
                // destinct the type of movement
                switch((*it)->Type){
                case Waypoint::LINE:
                case Waypoint::PTP:{
                    KDL::Frame Next = toFrame((*it)->EndPos);
                    // continues the movement until no continuous waypoint or the end
                    bool Cont = (*it)->Cont && !(it==--vpcWaypoints.end());
                    // start of a continue block
                    if (Cont && !pcRoundComp) {
                        pcRoundComp.reset(new KDL::Path_RoundedComposite(3, 3,
                                          new KDL::RotationalInterpolation_SingleAxis()));
                        // the velocity of the first waypoint is used
                        pcVelPrf.reset(new KDL::VelocityProfile_Trap((*it)->Velocity,(*it)->Accelaration));
                        pcRoundComp->Add(Last);
                        pcRoundComp->Add(Next);

                    // continue a continues block
                    }
                    else if (Cont && pcRoundComp) {
                        pcRoundComp->Add(Next);
                        // end a continuous block
                    }
                    else if (Cont==false && pcRoundComp) {
                        // add the last one
                        pcRoundComp->Add(Next);
                        pcRoundComp->Finish();
                        pcVelPrf->SetProfile(0,pcRoundComp->PathLength());
                        pcTrak.reset(new KDL::Trajectory_Segment(pcRoundComp.release(),pcVelPrf.release()));

                        // normal block
                    }
                    else if (Cont==false && !pcRoundComp){
                        KDL::Path* pcPath;
                        pcPath = new KDL::Path_Line(Last,
                                                    Next,
                                                    new KDL::RotationalInterpolation_SingleAxis(),
                                                    1.0,
                                                    true
                                                    );

                        pcVelPrf.reset(new KDL::VelocityProfile_Trap((*it)->Velocity,(*it)->Accelaration));
                        pcVelPrf->SetProfile(0,pcPath->PathLength());
                        pcTrak.reset(new KDL::Trajectory_Segment(pcPath,pcVelPrf.release()));
                    }
                    Last = Next;
                    break;}
                case Waypoint::WAIT:
                    break;
                default:
                    break;
                }

                // add the segment if no continuous block is running
                if (!pcRoundComp && pcTrak)
                    pcTrajectory->Add(pcTrak.release());
            }
        }
    }
    catch (KDL::Error &e) {
        throw Base::RuntimeError(e.Description());
    }
}

std::string Trajectory::getUniqueWaypointName(const char *Name) const
{
    if (!Name || *Name == '\0')
        return std::string();

    // check for first character whether it's a digit
    std::string CleanName = Name;
    if (!CleanName.empty() && CleanName[0] >= 48 && CleanName[0] <= 57)
        CleanName[0] = '_';
    // strip illegal chars
    for (std::string::iterator it = CleanName.begin(); it != CleanName.end(); ++it) {
        if (!((*it>=48 && *it<=57) ||  // number
             (*it>=65 && *it<=90)  ||  // uppercase letter
             (*it>=97 && *it<=122)))   // lowercase letter
             *it = '_'; // it's neither number nor letter
    }

    // name in use?
    std::vector<Robot::Waypoint*>::const_iterator it;
    for(it = vpcWaypoints.begin();it!=vpcWaypoints.end();++it)
        if((*it)->Name == CleanName) break;

    if (it == vpcWaypoints.end()) {
        // if not, name is OK
        return CleanName;
    }
    else {
        // find highest suffix
        int nSuff = 0;
        for(it = vpcWaypoints.begin();it!=vpcWaypoints.end();++it) {
            const std::string &ObjName = (*it)->Name;
            if (ObjName.substr(0, CleanName.length()) == CleanName) { // same prefix
                std::string clSuffix(ObjName.substr(CleanName.length()));
                if (clSuffix.size() > 0) {
                    std::string::size_type nPos = clSuffix.find_first_not_of("0123456789");
                    if (nPos==std::string::npos)
                        nSuff = std::max<int>(nSuff, std::atol(clSuffix.c_str()));
                }
            }
        }

        std::stringstream str;
        str << CleanName << (nSuff + 1);
        return str.str();
    }
}

void Trajectory::addWaypoint(const Waypoint &WPnt)
{
    std::string UniqueName = getUniqueWaypointName(WPnt.Name.c_str());
    Waypoint *tmp = new Waypoint(WPnt);
    tmp->Name = UniqueName;
    vpcWaypoints.push_back(tmp);
}




unsigned int Trajectory::getMemSize (void) const
{
	return 0;
}

void Trajectory::Save (Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Trajectory count=\"" <<  getSize() <<"\">" << std::endl;
    writer.incInd();
    for(unsigned int i = 0;i<getSize(); i++)
        vpcWaypoints[i]->Save(writer);
    writer.decInd();
    writer.Stream() << writer.ind() << "</Trajectory>" << std::endl ;

}

void Trajectory::Restore(XMLReader &reader)
{
    vpcWaypoints.clear();
    // read my element
    reader.readElement("Trajectory");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");
    vpcWaypoints.resize(count);

    for (int i = 0; i < count; i++) {
        Waypoint *tmp = new Waypoint();
        tmp->Restore(reader);
        vpcWaypoints[i] = tmp;
    }
    generateTrajectory();
}
