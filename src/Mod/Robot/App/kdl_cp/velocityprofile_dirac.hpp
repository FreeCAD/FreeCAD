/***************************************************************************
  tag: Peter Soetens  Fri Feb 11 15:59:12 CET 2005  velocityprofile_dirac.h

                        velocityprofile_dirac.h -  description
                           -------------------
    begin                : Fri February 11 2005
    copyright            : (C) 2005 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

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


#ifndef MOTIONPROFILE_DIRAC_H
#define MOTIONPROFILE_DIRAC_H

#include "velocityprofile.hpp"


namespace KDL {
	/**
	 * A Dirac VelocityProfile generates an infinite velocity
	 * so that the position jumps from A to B in in infinite short time.
	 * In practice, this means that the maximum values are ignored and
	 * for any t : Vel(t) == 0 and Acc(t) == 0.
	 * Further Pos( -0 ) = pos1 and Pos( +0 ) = pos2.
	 *
	 * However, if a duration is given, it will create an unbound
	 * rectangular velocity profile for that duration, otherwise,
	 * Duration() == 0;
	 * @ingroup Motion
	 */
	class VelocityProfile_Dirac : public VelocityProfile
	{
		double p1,p2,t;
	public:
		void SetProfile(double pos1,double pos2);
		virtual void SetProfileDuration(double pos1,double pos2,double duration);
		virtual double Duration() const;
		virtual double Pos(double time) const;
		virtual double Vel(double time) const;
		virtual double Acc(double time) const;
		virtual void Write(std::ostream& os) const;
		virtual VelocityProfile* Clone() const {
			VelocityProfile_Dirac* res =  new VelocityProfile_Dirac();
			res->SetProfileDuration( p1, p2, t );
			return res;
		}

		virtual ~VelocityProfile_Dirac() {}
	};

}


#endif
