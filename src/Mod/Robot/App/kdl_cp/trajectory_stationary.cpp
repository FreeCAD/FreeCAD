// SPDX-License-Identifier: LGPL-2.1-or-later

/*****************************************************************************
 *  \author
 *  	Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version
 *		LRL V0.2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: trajectory_stationary.cpp 22 2004-09-21 08:58:54Z eaertbellocal $
 *		$Name:  $
 ****************************************************************************/


#include "trajectory_stationary.hpp"

namespace KDL {

void Trajectory_Stationary::Write(std::ostream& os) const {
    os << "STATIONARY[ " << duration << std::endl;
    os << pos << std::endl;
	os << "]";
}

}
