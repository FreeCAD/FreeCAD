// SPDX-License-Identifier: LGPL-2.1-or-later

/*****************************************************************************
 * \file
 *      Defines I/O related routines to the FrameAccs classes defined in
 *      FrameAccs.h
 *
 *  \author
 *      Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version
 *      ORO_Geometry V0.2
 *
 *  \par History
 *      - $log$
 *
 *  \par Release
 *      $Id: rrframes_io.h,v 1.1.1.1 2002/08/26 14:14:21 rmoreas Exp $
 *      $Name:  $
 ****************************************************************************/
#pragma once

#include "utilities/utility_io.h"
#include "utilities/rall2d_io.h"

#include "frames_io.hpp"
#include "frameacc.hpp"

namespace KDL {


// Output...
inline std::ostream& operator << (std::ostream& os,const VectorAcc& r) {
    os << "{" << r.p << "," << r.v << "," << r.dv << "}" << std::endl;
    return os;
}

inline std::ostream& operator << (std::ostream& os,const RotationAcc& r) {
    os << "{" << std::endl << r.R << "," << std::endl << r.w <<
          "," << std::endl << r.dw << std::endl << "}" << std::endl;
    return os;
}


inline std::ostream& operator << (std::ostream& os,const FrameAcc& r) {
    os << "{" << std::endl << r.M << "," << std::endl << r.p << "}" << std::endl;
    return os;
}
inline std::ostream& operator << (std::ostream& os,const TwistAcc& r) {
    os << "{" << std::endl << r.vel << "," << std::endl << r.rot << std::endl << "}" << std::endl;
    return os;
}


} // namespace Frame