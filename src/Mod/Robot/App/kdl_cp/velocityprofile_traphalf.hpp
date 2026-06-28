// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  velocityprofile_traphalf.h

                        velocityprofile_traphalf.h -  description
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
 *		$Id: velocityprofile_traphalf.h,v 1.1.1.1.2.4 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 *  \par Status
 *      Experimental
 ****************************************************************************/

#pragma once

#include "velocityprofile.hpp"




namespace KDL {


	/**
	 * A 'Half' Trapezoidal VelocityProfile. A constructor flag
	 * indicates if the calculated profile should be starting
	 * or ending.
	 * @ingroup Motion
	 */
class VelocityProfile_TrapHalf : public VelocityProfile
	{
		// For "running" a motion profile :
		double a1,a2,a3; // coef. from ^0 -> ^2 of first part
		double b1,b2,b3; // of 2nd part
		double c1,c2,c3; // of 3th part
		double duration;
		double t1,t2;

		double startpos;
		double endpos;

		// Persistent state :
		double maxvel;
		double maxacc;
		bool   starting;

		void PlanProfile1(double v,double a);
		void PlanProfile2(double v,double a);
	public:

		/**
		 * \param _maxvel maximal velocity of the motion profile (positive)
		 * \param _maxacc maximal acceleration of the motion profile (positive)
		 * \param _starting this value is true when initial velocity is zero
		 *        and ending velocity is maxvel, is false for the reverse
		 */
		VelocityProfile_TrapHalf(double _maxvel=0,double _maxacc=0,bool _starting=true);

		void SetMax(double _maxvel,double _maxacc,bool _starting);

		/**
		 * Plans a 'Half' Trapezoidal VelocityProfile between pos1 and pos2.
		 * If the distance is too short between pos1 and pos2,
		 * only the acceleration phase is set and the max velocity is not reached.
		 *
		 * \param pos1 Starting position
		 * \param pos2 Ending position
		 *
		 * Can throw a Error_MotionPlanning_Not_Feasible
		 */
		virtual void SetProfile(double pos1,double pos2);

		/**
		 * Can be used to prolong the profile, there are two possible outcomes: in a first
		 * phase the acceleration is lowered as such that the end position and maximum velocity
		 * are reached at the given duration (newduration). In this case there is an acceleration part and a constant velocity part,
		 * when this reaches a minimum acceleration value at which the constant part disappears, the motion is stalled,
		 * in this case their is a non-motion part and an acceleration part.
		 *
		 *\param pos1 starting position
		 *\param pos2 ending position
		 *\param newduration the desired duration, if it is lower than the minimum duration, the minimum duration will be used instead of the given duration.
		 */
		virtual void SetProfileDuration(
			double pos1,double pos2,double newduration
		);

		virtual double Duration() const;
		virtual double Pos(double time) const;
		virtual double Vel(double time) const;
		virtual double Acc(double time) const;
		virtual void Write(std::ostream& os) const;
		virtual VelocityProfile* Clone() const;

		virtual ~VelocityProfile_TrapHalf();
	};



}