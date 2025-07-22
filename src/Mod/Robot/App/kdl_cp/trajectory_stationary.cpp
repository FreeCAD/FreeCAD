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

    using namespace std;


void Trajectory_Stationary::Write(ostream& os) const {
	os << "STATIONARY[ " << duration << endl;
	os << pos << endl;
	os << "]";
}


}

