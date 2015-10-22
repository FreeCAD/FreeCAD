/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  path_point.cxx

                        path_point.cxx -  description
                           -------------------
    begin                : Mon May 10 2004
    copyright            : (C) 2004 Erwin Aertbelien
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
 *		$Id: path_point.cpp,v 1.1.2.3 2003/07/24 13:40:49 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


#include "path_point.hpp"

namespace KDL {

Path_Point::Path_Point(const Frame& startpos)
    :F_base_start(startpos)
   {
   }

double Path_Point::LengthToS(double length) {
	return length;
}
double Path_Point::PathLength(){
	return 0;
}
Frame Path_Point::Pos(double s) const  {
	return F_base_start;
}

Twist Path_Point::Vel(double s,double sd) const  {
	return Twist::Zero();
}

Twist Path_Point::Acc(double s,double sd,double sdd) const  {
	return Twist::Zero();
}

Path_Point::~Path_Point() {
}

Path* Path_Point::Clone() {
	return new Path_Point( F_base_start	);
}

void Path_Point::Write(std::ostream& os)  {
	os << "POINT[ "<< F_base_start  << "]"  << std::endl;
}


}

