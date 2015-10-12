/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:39 CET 2005  trajectory.h

                        trajectory.h -  description
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
 *		$Id: trajectory.h,v 1.1.1.1.2.5 2003/07/23 16:44:25 psoetens Exp $
 *		$Name:  $
 *  \todo
 *     Peter's remark : should seperate I/O from other routines in the
 *     motion/chain directories
 *     The problem is that the I/O uses virtual inheritance to write
 *     the trajectories/geometries/velocityprofiles/...
 *     Have no good solution for this, perhaps
 *          * #ifdef's
 *          * declaring dummy ostream/istream and change implementation file .cpp
 *          * declaring some sort of VISITOR object (containing an ostream) ,
 *            the classes contain code to pass this object around along its children
 *            a subroutine can then be called with overloading.
 *     PROBLEM : if you declare a friend you have to fully declare it  ==> exposing I/O with ostream/istream decl
 *     CONSEQUENCE : everything has to be declared public.
 ****************************************************************************/

#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include "frames.hpp"
#include "frames_io.hpp"
#include "path.hpp"
#include "velocityprofile.hpp"



namespace KDL {




	/**
	 * An abstract class that implements
	 * a trajectory contains a cartesian space trajectory and an underlying
	 * velocity profile.
	  * @ingroup Motion
	 */
	class Trajectory
	{
	public:
		virtual double Duration() const = 0;
		// The duration of the trajectory

		virtual Frame Pos(double time) const = 0;
		// Position of the trajectory at <time>.

		virtual Twist Vel(double time) const = 0;
		// The velocity of the trajectory at <time>.
		virtual Twist Acc(double time) const = 0;
		// The acceleration of the trajectory at <time>.

		virtual Trajectory* Clone() const = 0;
		virtual void Write(std::ostream& os) const = 0;
		static Trajectory* Read(std::istream& is);
		virtual ~Trajectory() {}
		// note : you cannot declare this destructor abstract
		// it is always called by the descendant's destructor !
	};



}


#endif
