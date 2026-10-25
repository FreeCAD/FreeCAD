#pragma once

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

namespace Part::BaseGeometry
{

inline const gp_Pnt ORIGIN {};

inline const gp_Dir X_DIR {1, 0, 0};
inline const gp_Dir Y_DIR {0, 1, 0};
inline const gp_Dir Z_DIR {0, 0, 1};

inline const gp_Dir X_POS_DIR {1, 0, 0};
inline const gp_Dir X_NEG_DIR {-1, 0, 0};

inline const gp_Dir Y_POS_DIR {0, 1, 0};
inline const gp_Dir Y_NEG_DIR {0, -1, 0};

inline const gp_Dir Z_POS_DIR {0, 0, 1};
inline const gp_Dir Z_NEG_DIR {0, 0, -1};

inline const gp_Vec X_POS_VEC {1, 0, 0};
inline const gp_Vec X_NEG_VEC {-1, 0, 0};

inline const gp_Vec Y_POS_VEC {0, 1, 0};
inline const gp_Vec Y_NEG_VEC {0, -1, 0};

inline const gp_Vec Z_POS_VEC {0, 0, 1};
inline const gp_Vec Z_NEG_VEC {0, 0, -1};

inline const gp_Ax2 X_AXIS {ORIGIN, X_DIR};
inline const gp_Ax2 Y_AXIS {ORIGIN, Y_DIR};
inline const gp_Ax2 Z_AXIS {ORIGIN, Z_DIR};

inline const gp_Pln XY_PLANE {ORIGIN, Z_DIR};
inline const gp_Pln XZ_PLANE {ORIGIN, Y_DIR};
inline const gp_Pln YZ_PLANE {ORIGIN, X_DIR};

}  // namespace Part::BaseGeometry
