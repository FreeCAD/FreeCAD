/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:39 CET 2005  trajectory_segment.h

                        trajectory_segment.h -  description
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
 *		$Id: trajectory_segment.h,v 1.1.1.1.2.5 2003/07/23 16:44:26 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#ifndef KDL_MOTION_TRAJECTORY_SEGMENT_H
#define KDL_MOTION_TRAJECTORY_SEGMENT_H

#include "frames.hpp"
#include "frames_io.hpp"
#include "trajectory.hpp"
#include "path.hpp"
#include "velocityprofile.hpp"


namespace KDL {


	/**
	 * Trajectory_Segment combines a VelocityProfile and a Path into a
	 * trajectory
	 * @ingroup Motion
	 */
	class Trajectory_Segment :  public Trajectory
	{
		VelocityProfile* motprof;
		Path*      geom;
		bool aggregate;
	public:
		/**
		 * This constructor assumes that \a geom and \<_motprof\> are initialised correctly.
		 */
		Trajectory_Segment(Path* _geom, VelocityProfile* _motprof, bool _aggregate=true);

		/**
		 * This constructor assumes that \a geom is initialised and \<_motprof\> needs to be
		 * set according to \a duration.
		 */
		Trajectory_Segment(Path* _geom, VelocityProfile* _motprof, double duration, bool _aggregate=true);

		virtual double Duration() const;
		// The duration of the trajectory

		virtual Frame Pos(double time) const;
		// Position of the trajectory at <time>.

		virtual Twist Vel(double time) const;
		// The velocity of the trajectory at <time>.
		virtual Twist Acc(double time) const;
		// The acceleration of the trajectory at <time>.

 		virtual Trajectory* Clone() const
			{
				if ( aggregate )
					return new Trajectory_Segment( geom->Clone(), motprof->Clone(), true );
				return new Trajectory_Segment( geom, motprof, false );
			}

		virtual void Write(std::ostream& os) const;

	    virtual Path* GetPath();

	    virtual VelocityProfile* GetProfile();


		virtual ~Trajectory_Segment();
	};



}


#endif
