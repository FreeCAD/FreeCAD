// SPDX-License-Identifier: LGPL-2.1-or-later

/*****************************************************************************
 * \file
 *      provides I/O operations on FrameVels classes
 *
 *  \author
 *      Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version
 *      ORO_Geometry V2
 *
 *  \par History
 *      - $log$
 *
 *  \par Release
 *      $Id: rframes_io.h,v 1.1.1.1 2002/08/26 14:14:21 rmoreas Exp $
 *      $Name:  $
 ****************************************************************************/
#pragma once

#include "utilities/utility_io.h"
#include "utilities/rall1d_io.h"

#include "framevel_io.hpp"
#include "frames_io.hpp"

namespace KDL {

// Output...
inline std::ostream& operator << (std::ostream& os,const VectorVel& r) {
    os << "{" << r.p << "," << r.v << "}" << std::endl;
    return os;
}

inline std::ostream& operator << (std::ostream& os,const RotationVel& r) {
    os << "{" << std::endl << r.R << "," <<std::endl << r.w << std::endl << "}" << std::endl;
    return os;
}


inline std::ostream& operator << (std::ostream& os,const FrameVel& r) {
    os << "{" << std::endl << r.M << "," << std::endl << r.p << std::endl << "}" << std::endl;
    return os;
}

inline std::ostream& operator << (std::ostream& os,const TwistVel& r) {
    os << "{" << std::endl << r.vel << "," << std::endl << r.rot << std::endl << "}" << std::endl;
    return os;
}


} // namespace Frame