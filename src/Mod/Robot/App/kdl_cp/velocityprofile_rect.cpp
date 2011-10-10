/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  velocityprofile_rect.cxx

                        velocityprofile_rect.cxx -  description
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
 *		$Id: velocityprofile_rect.cpp,v 1.1.1.1.2.5 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


#include "utilities/error.h"
#include "velocityprofile_rect.hpp"

namespace KDL {


void VelocityProfile_Rectangular::SetProfile(
	double pos1,
	double pos2
	)
{
	double diff;
	diff = pos2-pos1;          // increment per sec.
    if ( diff != 0 )
        {
            v    = (diff>0)?maxvel:-maxvel;
            p    = pos1;               // start pos
            d    = diff/v;
        }
    else
        {
            v = 0;
            p = pos1;
            d = 0;
        }
}

    void VelocityProfile_Rectangular::SetMax( double vMax )
    {
        maxvel = vMax;
    }


void VelocityProfile_Rectangular::
	SetProfileDuration(double pos1,double pos2,double duration)
{
	double diff;
	diff = pos2-pos1;          // increment per sec.
    if ( diff != 0 )
        {
            v    = diff/duration;
            if (v > maxvel || duration==0 ) // safety.
                v=maxvel;
            p    = pos1;               // start pos
            d    = diff/v;
        }
    else
        {
            v    = 0;
            p    = pos1;
            d    = duration;
        }
}

double VelocityProfile_Rectangular::Duration() const {
	return d;
}

double VelocityProfile_Rectangular::Pos(double time) const {
    if (time < 0) {
        return p;
    } else if (time>d) {
        return v*d+p;
    } else {
        return v*time+p;
    }
}

double VelocityProfile_Rectangular::Vel(double time) const {
    if (time < 0) {
        return 0;
    } else if (time>d) {
        return 0;
    } else {
        return v;
    }
}

double VelocityProfile_Rectangular::Acc(double time) const {
	throw Error_MotionPlanning_Incompatible();
	return 0;
}


void VelocityProfile_Rectangular::Write(std::ostream& os) const {
	os << "CONSTVEL[" << maxvel << "]";
}


}

