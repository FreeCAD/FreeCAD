/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  path_roundedcomposite.h

                        path_roundedcomposite.h -  description
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
 *		$Id: path_roundedcomposite.h,v 1.1.1.1.2.3 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#ifndef KDL_MOTION_ROUNDEDCOMPOSITE_H
#define KDL_MOTION_ROUNDEDCOMPOSITE_H

#include "path.hpp"
#include "path_composite.hpp"
#include "rotational_interpolation.hpp"

namespace KDL {

/**
 * The specification of a path, composed of
 * way-points with rounded corners.
 * @ingroup Motion
 */
class Path_RoundedComposite : public Path
	{
		/** a Path_Composite is aggregated to hold the rounded trajectory
		 * with circles and lines
		 */
		Path_Composite* comp;


		double radius;
		double eqradius;
		RotationalInterpolation* orient;
		// cached from underlying path objects for generating the rounding :
		Frame F_base_start;
		Frame F_base_via;
		//Frame F_base_end;
		int nrofpoints;

		bool aggregate;
	public:

		/**
		 * @param radius : radius of the rounding circles
		 * @param _eqradius : equivalent radius to compare rotations/velocities
		 * @param _orient   : method of rotational_interpolation interpolation
		 * @param _aggregate : default True
		 */
		Path_RoundedComposite(double radius,double _eqradius,RotationalInterpolation* _orient, bool _aggregate=true);

		/**
		 * Adds a point to this rounded composite, between to adjecent points
		 * a Path_Line will be created, between two lines there will be
		 * rounding with the given radius with a Path_Circle
		 * Can throw Error_MotionPlanning_Not_Feasible object
		 */
		void Add(const Frame& F_base_point);

		/**
		 * to be called after the last line is added to finish up
		 * the work
		 */
		void Finish();


		virtual double LengthToS(double length);

		/**
		 * Returns the total path length of the trajectory
		 * (has dimension LENGTH)
		 * This is not always a physical length , ie when dealing with rotations
		 * that are dominant.
		 */
		virtual double PathLength();

		/**
		 * Returns the Frame at the current path length s
		 */
		virtual Frame Pos(double s) const;

		/**
		 * Returns the velocity twist at path length s theta and with
		 * derivative of s == sd
		 */
		virtual Twist Vel(double s,double sd) const;

		/**
		 * Returns the acceleration twist at path length s and with
		 * derivative of s == sd, and 2nd derivative of s == sdd
		 */
		virtual Twist Acc(double s,double sd,double sdd) const;

		/**
		 * virtual constructor, constructing by copying.
		 * In this case it returns the Clone() of the aggregated Path_Composite
		 * because this is all one ever will need.
		 */
		virtual Path* Clone();

		/**
		 * Writes one of the derived objects to the stream
		 */
		virtual void Write(std::ostream& os);

		virtual ~Path_RoundedComposite();
	};



}


#endif
