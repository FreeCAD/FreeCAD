/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  velocityprofile_traphalf.cxx

                        velocityprofile_traphalf.cxx -  description
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
 *		$Id: velocityprofile_traphalf.cpp,v 1.1.1.1.2.5 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


//#include "error.h"
#include "velocityprofile_traphalf.hpp"

namespace KDL {


VelocityProfile_TrapHalf::VelocityProfile_TrapHalf(double _maxvel,double _maxacc,bool _starting):
		  maxvel(_maxvel),maxacc(_maxacc),starting(_starting) {}

void VelocityProfile_TrapHalf::SetMax(double _maxvel,double _maxacc, bool _starting)
{
    maxvel = _maxvel; maxacc = _maxacc; starting = _starting;
}

void VelocityProfile_TrapHalf::PlanProfile1(double v,double a) {
	a3 = 0;
	a2 = 0;
	a1 = startpos;
	b3 = a/2.0;
	b2 = -a*t1;
	b1 = startpos + a*t1*t1/2.0;
	c3 = 0;
	c2 = v;
	c1 = endpos - v*duration;
}

void VelocityProfile_TrapHalf::PlanProfile2(double v,double a) {
	a3 = 0;
	a2 = v;
	a1 = startpos;
	b3 = -a/2.0;
	b2 = a*t2;
	b1 = endpos - a*t2*t2/2.0;
	c3 = 0;
	c2 = 0;
	c1 = endpos;
}

void VelocityProfile_TrapHalf::SetProfile(double pos1,double pos2) {
	startpos        = pos1;
	endpos          = pos2;
	double s        = sign(endpos-startpos);
	// check that the maxvel is obtainable
	double vel = std::min(maxvel, sqrt(2.0*s*(endpos-startpos)*maxacc));
	duration		= s*(endpos-startpos)/vel+vel/maxacc/2.0;
	if (starting) {
		t1 = 0;
		t2 = vel/maxacc;
		PlanProfile1(vel*s,maxacc*s);
	} else {
		t1 = duration-vel/maxacc;
		t2 = duration;
		PlanProfile2(s*vel,s*maxacc);
	}
}

void VelocityProfile_TrapHalf::SetProfileDuration(
	double pos1,double pos2,double newduration)
{
    SetProfile(pos1,pos2);
    double factor = duration/newduration;

    if ( factor > 1 )
        return;

	double s        = sign(endpos-startpos);
	double tmp      = 2.0*s*(endpos-startpos)/maxvel;
	double v        = s*maxvel;
	duration        = newduration;
	if (starting) {
		if (tmp > duration) {
			t1 = 0;
			double a = v*v/2.0/(v*duration-(endpos-startpos));
			t2 = v/a;
			PlanProfile1(v,a);
		} else {
			t2 = duration;
			double a = v*v/2.0/(endpos-startpos);
			t1 = t2-v/a;
			PlanProfile1(v,a);
		}
	} else {
		if (tmp > duration) {
			t2 = duration;
			double a = v*v/2.0/(v*duration-(endpos-startpos));
			t1 = t2-v/a;
			PlanProfile2(v,a);
		} else {
			double a = v*v/2.0/(endpos-startpos);
			t1 = 0;
			t2 = v/a;
			PlanProfile2(v,a);
		}
	}
}

double VelocityProfile_TrapHalf::Duration() const {
	return duration;
}

double VelocityProfile_TrapHalf::Pos(double time) const {
	if (time < 0) {
		return startpos;
	} else if (time<t1) {
		return a1+time*(a2+a3*time);
	} else if (time<t2) {
		return b1+time*(b2+b3*time);
	} else if (time<=duration) {
		return c1+time*(c2+c3*time);
	} else {
		return endpos;
	}
}
double VelocityProfile_TrapHalf::Vel(double time) const {
	if (time < 0) {
		return 0;
	} else if (time<t1) {
		return a2+2*a3*time;
	} else if (time<t2) {
		return b2+2*b3*time;
	} else if (time<=duration) {
		return c2+2*c3*time;
	} else {
		return 0;
	}
}

double VelocityProfile_TrapHalf::Acc(double time) const {
	if (time < 0) {
		return 0;
	} else if (time<t1) {
		return 2*a3;
	} else if (time<t2) {
		return 2*b3;
	} else if (time<=duration) {
		return 2*c3;
	} else {
		return 0;
	}
}

VelocityProfile* VelocityProfile_TrapHalf::Clone() const {
    VelocityProfile_TrapHalf* res =  new VelocityProfile_TrapHalf(maxvel,maxacc, starting);
    res->SetProfileDuration( this->startpos, this->endpos, this->duration );
	return res;
}

VelocityProfile_TrapHalf::~VelocityProfile_TrapHalf() {}


void VelocityProfile_TrapHalf::Write(std::ostream& os) const {
	os << "TRAPEZOIDALHALF[" << maxvel << "," << maxacc << "," << starting << "]";
}





}

