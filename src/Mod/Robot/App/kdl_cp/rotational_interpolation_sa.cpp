/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  rotational_interpolation_sa.cxx

                        rotational_interpolation_sa.cxx -  description
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
 *		$Id: rotational_interpolation_singleaxis.cpp,v 1.1.1.1.2.2 2003/02/24 13:13:06 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


#include "rotational_interpolation_sa.hpp"
#include "trajectory.hpp"

namespace KDL {


RotationalInterpolation_SingleAxis::RotationalInterpolation_SingleAxis()
	{}

void RotationalInterpolation_SingleAxis::SetStartEnd(Rotation start,Rotation end) {
	R_base_start = start;
	R_base_end   = end;
	Rotation R_start_end = R_base_start.Inverse()*R_base_end;
	angle = R_start_end.GetRotAngle(rot_start_end);
}

Rotation RotationalInterpolation_SingleAxis::Pos(double theta) const {
	return R_base_start* Rotation::Rot2(rot_start_end,theta);
}

Vector RotationalInterpolation_SingleAxis::Vel(double /*theta*/,double thetad) const {
	return R_base_start * ( rot_start_end*thetad );
}

Vector RotationalInterpolation_SingleAxis::Acc(double /*theta*/,double /*thetad*/,double thetadd) const {
	return R_base_start * ( rot_start_end* thetadd);
}

double RotationalInterpolation_SingleAxis::Angle() {
	return angle;
}

void RotationalInterpolation_SingleAxis::Write(std::ostream& os) const {
	os << "SingleAxis[] " << std::endl;
}

RotationalInterpolation_SingleAxis::~RotationalInterpolation_SingleAxis() {
}


RotationalInterpolation* RotationalInterpolation_SingleAxis::Clone() const {
	return new RotationalInterpolation_SingleAxis();
}

}

