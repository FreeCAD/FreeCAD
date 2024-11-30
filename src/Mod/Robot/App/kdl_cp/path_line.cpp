/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  path_line.cxx

                        path_line.cxx -  description
                           -------------------
    begin                : Mon May 10 2004
    copyright            : (C) 2004 Erwin Aertbelien
    email                : erwin.aertbelien@mech.kuleuven.ac.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/
/*****************************************************************************
 *  \author
 *  	Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version
 *		ORO_Geometry V0.2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: path_line.cpp,v 1.1.1.1.2.3 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/
// clazy:excludeall=rule-of-two-soft


#include "path_line.hpp"

namespace KDL {

Path_Line::Path_Line(const Frame& startpos,
		   const Frame& endpos,
		   RotationalInterpolation* _orient,
		   double _eqradius,
           bool _aggregate ):
			   orient(_orient),
			   V_base_start(startpos.p),
			   V_base_end(endpos.p),
			   eqradius(_eqradius),
               aggregate(_aggregate)
   {
	   	V_start_end = V_base_end - V_base_start;
	   	double dist = V_start_end.Normalize();
		orient->SetStartEnd(startpos.M,endpos.M);
		double alpha = orient->Angle();

		// See what has the slowest eq. motion, and adapt
		// the other to this slower motion
		// use eqradius to transform between rot and transl.

        // Only modify if non zero (prevent division by zero)
		if ( alpha != 0 && alpha*eqradius > dist) {
			// rotational_interpolation is the limitation
			pathlength = alpha*eqradius;
			scalerot   = 1/eqradius;
			scalelin   = dist/pathlength;
		} else if ( dist != 0 ) {
			// translation is the limitation
			pathlength = dist;
			scalerot   = alpha/pathlength;
			scalelin   = 1;
		} else {
            // both were zero
            pathlength = 0;
            scalerot   = 1;
            scalelin   = 1;
        }
   }

Path_Line::Path_Line(const Frame& startpos,
		   const Twist& starttwist,
		   RotationalInterpolation* _orient,
		   double _eqradius,
           bool _aggregate ):
			   orient(_orient),
			   V_base_start(startpos.p),
			   V_base_end(startpos.p + starttwist.vel),
			   eqradius(_eqradius),
               aggregate(_aggregate)
   {
       // startframe and starttwist are expressed in Wo.
       // after 1 time unit, startframe has translated over starttwist.vel
       // and rotated over starttwist.rot.Norm() (both vectors can be zero)
       // Thus the frame on the path after 1 time unit is defined by
       // startframe.Integrate(starttwist, 1);
	   	V_start_end = V_base_end - V_base_start;
	   	double dist = V_start_end.Normalize(); // distance traveled during 1 time unit
		orient->SetStartEnd(startpos.M, (startpos*Frame( Rotation::Rot(starttwist.rot, starttwist.rot.Norm() ), starttwist.vel )).M);
		double alpha = orient->Angle();        // rotation during 1 time unit

		// See what has the slowest eq. motion, and adapt
		// the other to this slower motion
		// use eqradius to transform between rot and transl.
        // Only modify if non zero (prevent division by zero)
		if ( alpha != 0 && alpha*eqradius > dist) {
			// rotational_interpolation is the limitation
			pathlength = alpha*eqradius;
			scalerot   = 1/eqradius;
			scalelin   = dist/pathlength;
		} else if ( dist != 0 ) {
			// translation is the limitation
			pathlength = dist;
			scalerot   = alpha/pathlength;
			scalelin   = 1;
		} else {
            // both were zero
            pathlength = 0;
            scalerot   = 1;
            scalelin   = 1;
        }
   }

double Path_Line::LengthToS(double length) {
	return length/scalelin;
}
double Path_Line::PathLength(){
	return pathlength;
}
Frame Path_Line::Pos(double s) const  {
	return Frame(orient->Pos(s*scalerot),V_base_start + V_start_end*s*scalelin );
}

Twist Path_Line::Vel(double s,double sd) const  {
	return Twist( V_start_end*sd*scalelin, orient->Vel(s*scalerot,sd*scalerot) );
}

Twist Path_Line::Acc(double s,double sd,double sdd) const  {
	return Twist( V_start_end*sdd*scalelin, orient->Acc(s*scalerot,sd*scalerot,sdd*scalerot) );
}


Path_Line::~Path_Line() {
    if (aggregate)
        delete orient;
}

Path* Path_Line::Clone() {
    if (aggregate )
        return new Path_Line(
                             Frame(orient->Pos(0),V_base_start),
                             Frame(orient->Pos(pathlength*scalerot),V_base_end),
                             orient->Clone(),
                             eqradius,
                             true
                             );
    // else :
    return new Path_Line(
                         Frame(orient->Pos(0),V_base_start),
                         Frame(orient->Pos(pathlength*scalerot),V_base_end),
                         orient,
                         eqradius,
                         false
                         );

}

void Path_Line::Write(std::ostream& os)  {
	os << "LINE[ ";
	os << "  " << Frame(orient->Pos(0),V_base_start) << std::endl;
	os << "  " << Frame(orient->Pos(pathlength*scalerot),V_base_end) << std::endl;
	os << "  ";orient->Write(os);
	os << "  " << eqradius;
	os << "]"  << std::endl;
}


}

