// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <gp.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

// Constants not provided by OCC. Use gp::Origin() and gp::DX(), gp::DY(), gp::DZ() for the
// origin and the positive axis directions.
namespace Part::BaseGeometry
{

inline const gp_Dir X_NEG_DIR {-1, 0, 0};
inline const gp_Dir Y_NEG_DIR {0, -1, 0};
inline const gp_Dir Z_NEG_DIR {0, 0, -1};

inline const gp_Vec X_POS_VEC {gp::DX()};
inline const gp_Vec X_NEG_VEC {X_NEG_DIR};

inline const gp_Vec Y_POS_VEC {gp::DY()};
inline const gp_Vec Y_NEG_VEC {Y_NEG_DIR};

inline const gp_Vec Z_POS_VEC {gp::DZ()};
inline const gp_Vec Z_NEG_VEC {Z_NEG_DIR};

// Unlike gp::YOZ(), gp::ZOX() and gp::XOY(), the main direction is the named axis
inline const gp_Ax2 X_AXIS {gp::Origin(), gp::DX()};
inline const gp_Ax2 Y_AXIS {gp::Origin(), gp::DY()};
inline const gp_Ax2 Z_AXIS {gp::Origin(), gp::DZ()};

inline const gp_Pln XY_PLANE {gp::XOY()};
inline const gp_Pln XZ_PLANE {gp::Origin(), gp::DY()};
inline const gp_Pln YZ_PLANE {gp::YOZ()};

}  // namespace Part::BaseGeometry
