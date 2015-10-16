/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  velocityprofile_trap.cxx

                        velocityprofile_trap.cxx -  description
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
 *		$Id: velocityprofile_trap.cpp,v 1.1.1.1.2.7 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


//#include "error.h"
#include "velocityprofile_trap.hpp"

namespace KDL {


VelocityProfile_Trap::VelocityProfile_Trap(double _maxvel,double _maxacc):
          a1(0), a2(0), a3(0),
          b1(0), b2(0), b3(0),
          c1(0), c2(0), c3(0),
          duration(0), t1(0), t2(0),
		  maxvel(_maxvel),maxacc(_maxacc),
          startpos(0), endpos(0)

{}
		// constructs motion profile class with <maxvel> as parameter of the
		// trajectory.

void VelocityProfile_Trap::SetProfile(double pos1,double pos2) {
	startpos = pos1;
	endpos   = pos2;
	t1 = maxvel/maxacc;
	double s       = sign(endpos-startpos);
	double deltax1 = s*maxacc*sqr(t1)/2.0;
	double deltaT  = (endpos-startpos-2.0*deltax1) / (s*maxvel);
	if (deltaT > 0.0) {
		// plan a complete profile :
		duration = 2*t1+deltaT;
		t2 = duration - t1;
	} else {
		// plan an incomplete profile :
		t1 = ::sqrt((endpos-startpos)/s/maxacc);
		duration = t1*2.0;
		t2=t1;
	}
	a3 = s*maxacc/2.0;
	a2 = 0;
	a1 = startpos;

	b3 = 0;
	b2 = a2+2*a3*t1 - 2.0*b3*t1;
	b1 = a1+t1*(a2+a3*t1) - t1*(b2+t1*b3);

	c3 = -s*maxacc/2.0;
	c2 = b2+2*b3*t2 - 2.0*c3*t2;
	c1 = b1+t2*(b2+b3*t2) - t2*(c2+t2*c3);
}

void VelocityProfile_Trap::SetProfileDuration(
	double pos1,double pos2,double newduration) {
	// duration should be longer than originally planned duration
    // Fastest :
	SetProfile(pos1,pos2);
    // Must be Slower  :
	double factor = duration/newduration;
    if (factor > 1)
        return; // do not exceed max
	a2*=factor;
	a3*=factor*factor;
	b2*=factor;
	b3*=factor*factor;
	c2*=factor;
	c3*=factor*factor;
	duration = newduration;
	t1 /= factor;
	t2 /= factor;
}

void VelocityProfile_Trap::SetProfileVelocity(
	double pos1,double pos2,double newvelocity) {
    // Max velocity
	SetProfile(pos1,pos2);
    // Must be Slower  :
	double factor = newvelocity;		// valid = [KDL::epsilon, 1.0]
	if (1.0 < factor) factor = 1.0;
	if (KDL::epsilon > factor) factor = KDL::epsilon;
	a2*=factor;
	a3*=factor*factor;
	b2*=factor;
	b3*=factor*factor;
	c2*=factor;
	c3*=factor*factor;
	duration = duration / factor;
	t1 /= factor;
	t2 /= factor;
}

void VelocityProfile_Trap::SetMax(double _maxvel,double _maxacc)
{
    maxvel = _maxvel; maxacc = _maxacc;
}

double VelocityProfile_Trap::Duration() const {
	return duration;
}

double VelocityProfile_Trap::Pos(double time) const {
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
double VelocityProfile_Trap::Vel(double time) const {
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

double VelocityProfile_Trap::Acc(double time) const {
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

VelocityProfile* VelocityProfile_Trap::Clone() const {
    VelocityProfile_Trap* res =  new VelocityProfile_Trap(maxvel,maxacc);
    res->SetProfileDuration( this->startpos, this->endpos, this->duration );
	return res;
}

VelocityProfile_Trap::~VelocityProfile_Trap() {}


void VelocityProfile_Trap::Write(std::ostream& os) const {
	os << "TRAPEZOIDAL[" << maxvel << "," << maxacc <<"]";
}





}

