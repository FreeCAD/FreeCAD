// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:39 CET 2005  rotational_interpolation_sa.h

                        rotational_interpolation_sa.h -  description
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
 *		$Id: rotational_interpolation_singleaxis.h,v 1.1.1.1.2.3 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#pragma once

#include "frames.hpp"
#include "frames_io.hpp"
#include "rotational_interpolation.hpp"


namespace KDL {


	 /**
	  * An interpolation algorithm which rotates a frame over the existing
	  * single rotation axis
	  * formed by start and end rotation. If more than one rotational axis
	  * exist, an arbitrary one will be chosen, therefore it is not recommended 
	  * to try to interpolate a 180 degrees rotation.
	  * @ingroup Motion
	  */
class RotationalInterpolation_SingleAxis: public RotationalInterpolation
	{
		Rotation R_base_start;
		Rotation R_base_end;
		Vector rot_start_end;
		double angle;
	public:
		RotationalInterpolation_SingleAxis();
		void SetStartEnd(Rotation start,Rotation end) override;
		double Angle() override;
		Rotation Pos(double th) const override;
		Vector Vel(double th,double thd) const override;
		Vector Acc(double th,double thd,double thdd)   const override;
		void Write(std::ostream& os) const override;
		RotationalInterpolation* Clone() const override;
		~RotationalInterpolation_SingleAxis() override;
	};

}