/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  velocityprofile.cxx

                        velocityprofile.cxx -  description
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
 *		$Id: velocityprofile.cpp,v 1.1.1.1.2.3 2003/02/24 13:13:06 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


#include "utilities/error.h"
#include "utilities/error_stack.h"
#include "velocityprofile_rect.hpp"
#include "velocityprofile_dirac.hpp"
#include "velocityprofile_trap.hpp"
#include "velocityprofile_traphalf.hpp"
#include <string.h>

namespace KDL {

using namespace std;

VelocityProfile* VelocityProfile::Read(istream& is) {
	IOTrace("VelocityProfile::Read");
	char storage[25];
	EatWord(is,"[",storage,sizeof(storage));
	Eat(is,'[');
	if (strcmp(storage,"DIRACVEL")==0) {
		Eat(is,']');
		IOTracePop();
		return new VelocityProfile_Dirac();
	} else if (strcmp(storage,"CONSTVEL")==0) {
		double vel;
		is >> vel;
		Eat(is,']');
		IOTracePop();
		return new VelocityProfile_Rectangular(vel);
	} else if (strcmp(storage,"TRAPEZOIDAL")==0) {
		double maxvel;
		double maxacc;
 		is >> maxvel;
		Eat(is,',');
		is >> maxacc;
		Eat(is,']');
		IOTracePop();
		return new VelocityProfile_Trap(maxvel,maxacc);
	} else if (strcmp(storage,"TRAPEZOIDALHALF")==0) {
		double maxvel;
		double maxacc;
 		is >> maxvel;
		Eat(is,',');
		is >> maxacc;
		Eat(is,',');
		bool starting;
		is >> starting;
		Eat(is,']');
		IOTracePop();
		return new VelocityProfile_TrapHalf(maxvel,maxacc,starting);
	}
	else {
		throw Error_MotionIO_Unexpected_MotProf();
	}
    return 0;
}



}
