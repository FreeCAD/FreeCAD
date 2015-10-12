/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  path.h

                        path.h -  description
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
 *		$Id: path.h,v 1.1.1.1.2.3 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#ifndef KDL_MOTION_PATH_H
#define KDL_MOTION_PATH_H

#include "frames.hpp"

#include <vector>

#include "frames_io.hpp"

namespace KDL {

/**
 * The specification of the path of a trajectory.
 */
class Path
	{
	public:
		enum IdentifierType {
			ID_LINE=1,
			ID_CIRCLE=2,
			ID_COMPOSITE=3,
			ID_ROUNDED_COMPOSITE=4,
			ID_POINT=5,
			ID_CYCLIC_CLOSED=6
		};
		/**
		 * LengthToS() converts a physical length along the trajectory
		 * to the parameter s used in Pos, Vel and Acc.  This is used because
		 * in cases with large rotations the parameter s does NOT correspond to
		 * the lineair length along the trajectory.
		 * User should be sure that the lineair distance travelled by this
		 * path object is NOT zero, when using this method !
		 * (e.g. the case of only rotational change)
		 * throws Error_MotionPlanning_Not_Applicable if used on composed
		 * path objects.
		 * @ingroup Motion
		 */
		virtual double LengthToS(double length)  = 0;

		/**
		 * Returns the total path length of the trajectory
		 * (has dimension LENGTH)
		 * This is not always a physical length , ie when dealing with rotations
		 * that are dominant.
		 */
		virtual double PathLength() = 0;

		/**
		 * Returns the Frame at the current path length s
		 */
		virtual Frame Pos(double s) const = 0;

		/**
		 * Returns the velocity twist at path length s theta and with
		 * derivative of s == sd
		 */
		virtual Twist Vel(double s,double sd) const  = 0;

		/**
		 * Returns the acceleration twist at path length s and with
		 * derivative of s == sd, and 2nd derivative of s == sdd
		 */
		virtual Twist Acc(double s,double sd,double sdd) const  = 0;

		/**
		 * Writes one of the derived objects to the stream
		 */
		virtual void Write(std::ostream& os)  = 0;

		/**
		 * Reads one of the derived objects from the stream and returns a pointer
		 * (factory method)
		 */
		static Path* Read(std::istream& is);

		/**
		 * Virtual constructor, constructing by copying,
		 * Returns a deep copy of this Path Object
		 */
		virtual Path* Clone() = 0;

		/**
		 * gets an identifier indicating the type of this Path object
		 */
		virtual IdentifierType getIdentifier() const=0;

		virtual ~Path() {}
	};

}


#endif
