/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:39 CET 2005  path_point.h

                        path_point.h -  description
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
*   ALTERNATIVE FOR trajectory_stationary.h/cpp
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
 *		$Id: path_point.h,v 1.1.2.3 2003/07/24 13:40:49 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#ifndef KDL_MOTION_PATH_POINT_H
#define KDL_MOTION_PATH_POINT_H

#include "path.hpp"
#include "rotational_interpolation.hpp"


namespace KDL {



/**
 * A Path consisting only of a point in space.
 * @ingroup Motion
 */
class Path_Point : public Path
	{
		Frame F_base_start;
	public:
		/**
		 * Constructs a Point Path
		 */
		Path_Point(const Frame& F_base_start);
		double LengthToS(double length);
		virtual double PathLength();
		virtual Frame Pos(double s) const;
		virtual Twist Vel(double s,double sd) const ;
		virtual Twist Acc(double s,double sd,double sdd) const;
		virtual void Write(std::ostream& os);
		virtual Path* Clone();

		/**
		 * gets an identifier indicating the type of this Path object
		 */
		virtual IdentifierType getIdentifier() const {
			return ID_POINT;
		}
		virtual ~Path_Point();
	};

}


#endif
