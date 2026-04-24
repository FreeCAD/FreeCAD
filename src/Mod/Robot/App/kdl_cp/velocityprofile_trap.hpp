// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  velocityprofile_trap.h

                        velocityprofile_trap.h -  description
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
 *		$Id: velocityprofile_trap.h,v 1.1.1.1.2.5 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#pragma once

#include "velocityprofile.hpp"




namespace KDL {



	/**
	 * A Trapezoidal VelocityProfile implementation.
	 * @ingroup Motion
	 */
class VelocityProfile_Trap : public VelocityProfile
	{
		// For "running" a motion profile :
		double a1,a2,a3; // coef. from ^0 -> ^2 of first part
		double b1,b2,b3; // of 2nd part
		double c1,c2,c3; // of 3th part
		double duration;
		double t1,t2;

		// specification of the motion profile :
		double maxvel;
		double maxacc;
		double startpos;
		double endpos;
	public:

		VelocityProfile_Trap(double _maxvel=0,double _maxacc=0);
		// constructs motion profile class with <maxvel> and <maxacc> as parameters of the
		// trajectory.

		virtual void SetProfile(double pos1,double pos2);

		virtual void SetProfileDuration(
			double pos1,double pos2,double newduration
		);

		/** Compute trapezoidal profile at a given fraction of max velocity
			@param pos1 Position to start from
			@param pos2 Position to end at
			@param newvelocity Fraction of max velocity to use during the
			non-ramp, flat-velocity part of the profile.
			@param KDL::epsilon <= newvelocity <= 1.0 (forcibly clamped to
			this range internally)
		*/
		virtual void SetProfileVelocity(
			double pos1,double pos2,double newvelocity
		);

        virtual void SetMax(double _maxvel,double _maxacc);
		virtual double Duration() const;
		virtual double Pos(double time) const;
		virtual double Vel(double time) const;
		virtual double Acc(double time) const;
		virtual void Write(std::ostream& os) const;
		virtual VelocityProfile* Clone() const;
		// returns copy of current VelocityProfile object. (virtual constructor)
		virtual ~VelocityProfile_Trap();
	};






/* Niet OK
	class VelocityProfile_Trap : public VelocityProfile {
		double maxvel;
		double maxacc;
		double _t1,_t2,_T,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10;

		void PrepTraj(double p1,double v1,double p2,double v2,
		double acc,double vel,double t1,double t2,double T);
		// Internal method. Sets the parameters <_t1>,..<c10> with the given
		// arguments.
	public:
		VelocityProfile_Trap(double _maxvel,double _maxacc):
		  maxvel(_maxvel),maxacc(_maxacc) {}
		// constructs motion profile class with max velocity <maxvel>,
		// and max acceleration <maxacc> as parameter of the
		// trajectory.

		void SetProfile(double pos1,double pos2);
		virtual void SetProfileDuration(double pos1,double pos2,double duration);
		virtual double Duration() ;
		virtual double Pos(double time);
		virtual double Vel(double time);
	};
*/

}