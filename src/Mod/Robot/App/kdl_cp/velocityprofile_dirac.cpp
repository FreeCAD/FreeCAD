/***************************************************************************
  tag: Peter Soetens  Mon May 10 19:10:36 CEST 2004  velocityprofile_dirac.cxx

                        velocityprofile_dirac.cxx -  description
                           -------------------
    begin                : Mon May 10 2004
    copyright            : (C) 2004 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

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

#include "utilities/error.h"
#include "velocityprofile_dirac.hpp"

namespace KDL {


    void VelocityProfile_Dirac::SetProfile(
                                           double pos1,
                                           double pos2
                                           )
    {
        p1 = pos1;
        p2 = pos2;
        t = 0;
    }

    void VelocityProfile_Dirac::
	SetProfileDuration(double pos1,double pos2,double duration)
    {
        SetProfile(pos1,pos2);
        t = duration;
    }

    double VelocityProfile_Dirac::Duration() const {
        return t;
    }

    double VelocityProfile_Dirac::Pos(double time) const {
        if ( t == 0 )
            return time == 0 ? p1 : p2;
        else
            return p1 + (( p2 - p1)/t)*time;
    }

    double VelocityProfile_Dirac::Vel(double time) const {
        if ( t == 0 )
            {
            throw Error_MotionPlanning_Incompatible();
            }
        else
            if ( 0 < time && time < t )
                return (p2-p1) / t;
        return 0;
    }

    double VelocityProfile_Dirac::Acc(double /*time*/) const {
        throw Error_MotionPlanning_Incompatible();
    }


    void VelocityProfile_Dirac::Write(std::ostream& os) const {
        os << "DIRACVEL[ ]";
    }



}

