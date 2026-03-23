// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  velocityprofile.h

                        velocityprofile.h -  description
                           -------------------
    begin                : Mon January 10 2005
    copyright            : (C) 2005 Erwin Aertbelien
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
 *
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
 *		$Id: velocityprofile.h,v 1.1.1.1.2.5 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#pragma once

#include "utilities/utility.h"
#include "utilities/utility_io.h"


namespace KDL {


    /**
     * A VelocityProfile stores the velocity profile that
     * is used within a trajectory. A velocity profile is the function that
     * expresses position, velocity and acceleration of a point on a curve
     * in function of time. It defines the how a point s moves on a path S.
     * @ingroup Motion
     */
class VelocityProfile
	{
	public:
		// trajectory parameters are set in constructor of
		// derived class

		virtual void SetProfile(double pos1,double pos2) = 0;
		// sets a trajectory from pos1 to pos2 as fast as possible

		virtual void SetProfileDuration(
			double pos1,double pos2,double duration) = 0;
		// Sets a trajectory from pos1 to pos2 in <duration> seconds.
        // @post new.Duration() will not be shorter than the one obtained
        //       from SetProfile(pos1,pos2).


		virtual double Duration() const = 0;
		// returns the duration of the motion in [sec]

		virtual double Pos(double time) const = 0;
		// returns the position at <time> in the units of the input
		// of the constructor of the derived class.

		virtual double Vel(double time) const = 0;
		// returns the velocity at <time> in the units of the input
		// of the constructor of the derived class.

		virtual double Acc(double time) const = 0;
		// returns the acceleration at <time> in the units of the input
		// of the constructor of the derived class.

		virtual void Write(std::ostream& os) const = 0;
		// Writes object to a stream.

		static VelocityProfile* Read(std::istream& is);
		// reads a VelocityProfile object from the stream and returns it.

		virtual VelocityProfile* Clone() const = 0;
		// returns copy of current VelocityProfile object. (virtual constructor)

		virtual ~VelocityProfile() {}
	};
}