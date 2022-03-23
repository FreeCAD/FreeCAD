/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  path.cxx

                        path.cxx -  description
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
 *		$Id: path.cpp,v 1.1.1.1.2.4 2003/07/18 14:49:50 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


#include "utilities/error.h"
#include "utilities/error_stack.h"
#include "path.hpp"
#include "path_line.hpp"
#include "path_point.hpp"
#include "path_circle.hpp"
#include "path_composite.hpp"
#include "path_roundedcomposite.hpp"
#include "path_cyclic_closed.hpp"
#include <memory>
#include <string.h>

namespace KDL {

using namespace std;


Path* Path::Read(istream& is) {
	// unique_ptr because exception can be thrown !
	IOTrace("Path::Read");
	char storage[64];
	EatWord(is,"[",storage,sizeof(storage));
	Eat(is,'[');
	if (strcmp(storage,"POINT")==0) {
		IOTrace("POINT");
		Frame startpos;
		is >> startpos;
		EatEnd(is,']');
		IOTracePop();
		IOTracePop();
		return new Path_Point(startpos);
	} else 	if (strcmp(storage,"LINE")==0) {
		IOTrace("LINE");
		Frame startpos;
		Frame endpos;
		is >> startpos;
		is >> endpos;
		unique_ptr<RotationalInterpolation> orient( RotationalInterpolation::Read(is) );
		double eqradius;
		is >> eqradius;
		EatEnd(is,']');
		IOTracePop();
		IOTracePop();
		return new Path_Line(startpos,endpos,orient.release(),eqradius);
	} else if (strcmp(storage,"CIRCLE")==0) {
		IOTrace("CIRCLE");
		Frame F_base_start;
		Vector V_base_center;
		Vector V_base_p;
		Rotation R_base_end;
		double alpha;
		double eqradius;
		is >> F_base_start;
		is >> V_base_center;
		is >> V_base_p;
		is >> R_base_end;
		is >> alpha;
		alpha *= deg2rad;
		unique_ptr<RotationalInterpolation> orient( RotationalInterpolation::Read(is) );
		is >> eqradius;
		EatEnd(is,']');
		IOTracePop();
		IOTracePop();
		return new Path_Circle(
						F_base_start,
						V_base_center,
						V_base_p,
						R_base_end,
						alpha,
						orient.release() ,
						eqradius
					);
	} else if (strcmp(storage,"ROUNDEDCOMPOSITE")==0) {
		IOTrace("ROUNDEDCOMPOSITE");
		double radius;
		is >> radius;
		double eqradius;
		is >> eqradius;
		unique_ptr<RotationalInterpolation> orient( RotationalInterpolation::Read(is) );
		unique_ptr<Path_RoundedComposite> tr(
			new Path_RoundedComposite(radius,eqradius,orient.release())
		);
		int size;
		is >> size;
		int i;
		for (i=0;i<size;i++) {
			Frame f;
			is >> f;
			tr->Add(f);
		}
		tr->Finish();
		EatEnd(is,']');
		IOTracePop();
		IOTracePop();
		return tr.release();
	} else if (strcmp(storage,"COMPOSITE")==0) {
		IOTrace("COMPOSITE");
		int size;
		unique_ptr<Path_Composite> tr( new Path_Composite() );
		is >> size;
		int i;
		for (i=0;i<size;i++) {
			tr->Add(Path::Read(is));
		}
		EatEnd(is,']');
		IOTracePop();
		IOTracePop();
		return tr.release();
	} else if (strcmp(storage,"CYCLIC_CLOSED")==0) {
		IOTrace("CYCLIC_CLOSED");
		int times;
		unique_ptr<Path> tr( Path::Read(is) );
		is >> times;
		EatEnd(is,']');
		IOTracePop();
		IOTracePop();
		return new Path_Cyclic_Closed(tr.release(),times);
	} else {
		throw Error_MotionIO_Unexpected_Traj();
	}
	return nullptr; // just to avoid the warning;
}



}

