/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  rotational_interpolation.h

                        rotational_interpolation.h -  description
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
 *		$Id: rotational_interpolation.h,v 1.1.1.1.2.2 2003/02/24 13:13:06 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#ifndef KDL_ROTATIONALINTERPOLATION_H
#define KDL_ROTATIONALINTERPOLATION_H

#include "frames.hpp"
#include "frames_io.hpp"

namespace KDL {

/**
 * RotationalInterpolation specifies the rotational part of a geometric trajectory
 * -   The different derived objects specify different methods for interpolating
 *    rotations.
 * - SetStartEnd should be called before
 *   using the other methods
 * - The start and end position do NOT belong to the persistent state !  The owner of this
 *   object is responsible for setting these each time
 * @ingroup Motion
 */
class RotationalInterpolation
	{
	public:
		/**
		 * Set the start and end rotational_interpolation
		 */
		virtual void SetStartEnd(Rotation start,Rotation end) = 0;

		/**
		 * - Returns the angle value to move from start to end.
		 * This should have units radians,
		 * - With Single Axis interp corresponds to the angle rotation
		 * - With Three Axis interp corresponds to the slowest of the three
		 * rotations.
		 */
		virtual double Angle() = 0;

		/**
		 * Returns the rotation matrix at angle theta
		 */
		virtual Rotation Pos(double theta) const = 0;

		/**
		 * Returns the rotational velocity at angle theta and with
		 * derivative of theta == thetad
		 */
		virtual Vector Vel(double theta,double thetad) const = 0;

		/**
		 * Returns the rotational acceleration at angle theta and with
		 * derivative of theta == thetad, and 2nd derivative of theta == thdd
		 */
		virtual Vector Acc(double theta,double thetad,double thetadd) const = 0;

		/**
		 * Writes one of the derived objects to the stream
		 */
		virtual void Write(std::ostream& os) const = 0;

		/**
		 * Reads one of the derived objects from the stream and returns a pointer
		 * (factory method)
		 */
		static RotationalInterpolation* Read(std::istream& is);

		/**
		 * virtual constructor,  construction by copying ..
		 */
		virtual RotationalInterpolation* Clone() const = 0;

		virtual ~RotationalInterpolation() {}
	};

}


#endif
