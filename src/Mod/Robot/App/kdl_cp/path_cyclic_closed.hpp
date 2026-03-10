// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  path_cyclic_closed.h

                        path_cyclic_closed.h -  description
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
 *		ORO_Geometry V2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: path_cyclic_closed.h,v 1.1.1.1.2.3 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#pragma once

#include "frames.hpp"
#include "frames_io.hpp"
#include "path.hpp"
#include <vector>


namespace KDL {

	 /**
	  * A Path representing a closed circular movement,
	  * which is traversed a number of times.
	  * @ingroup Motion
	  */
	 class Path_Cyclic_Closed : public Path
	{
		int times;
		Path* geom;
		bool aggregate;
	public:
		Path_Cyclic_Closed(Path* _geom,int _times, bool _aggregate=true);
		virtual double LengthToS(double length);
		virtual double PathLength();
		virtual Frame Pos(double s) const;
		virtual Twist Vel(double s,double sd) const;
		virtual Twist Acc(double s,double sd,double sdd) const;

		virtual void Write(std::ostream& os);
		static Path* Read(std::istream& is);
		virtual Path* Clone();
		/**
		 * gets an identifier indicating the type of this Path object
		 */
		virtual IdentifierType getIdentifier() const {
			return ID_CYCLIC_CLOSED;
		}
		virtual ~Path_Cyclic_Closed();
	};



}