/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  velocityprofile_rect.h

                        velocityprofile_rect.h -  description
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
 *		$Id: velocityprofile_rect.h,v 1.1.1.1.2.4 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#ifndef MOTIONPROFILE_RECT_H
#define MOTIONPROFILE_RECT_H

#include "velocityprofile.hpp"


namespace KDL {
	/**
	 * A rectangular VelocityProfile generates a constant velocity
	 * for moving from A to B.
	 * @ingroup Motion
	 */
	class VelocityProfile_Rectangular : public VelocityProfile
		// Defines a rectangular velocityprofile.
		// (i.e. constant velocity)
	{
		double d,p,v;
	public:
		double maxvel;

		VelocityProfile_Rectangular(double _maxvel=0):
		  maxvel(_maxvel) {}
		// constructs motion profile class with <maxvel> as parameter of the
		// trajectory.

		void SetMax( double _maxvel );
		void SetProfile(double pos1,double pos2);
		virtual void SetProfileDuration(
			double pos1,double pos2,double duration);
		virtual double Duration() const;
		virtual double Pos(double time) const;
		virtual double Vel(double time) const;
		virtual double Acc(double time) const;
		virtual void Write(std::ostream& os) const;
		virtual VelocityProfile* Clone() const{
			VelocityProfile_Rectangular* res =  new VelocityProfile_Rectangular(maxvel);
			res->SetProfileDuration( p, p+v*d, d );
			return res;
		}
		// returns copy of current VelocityProfile object. (virtual constructor)
		virtual ~VelocityProfile_Rectangular() {}
	};

}


#endif
