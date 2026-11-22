#pragma once

#include <gp_Trsf.hxx>

namespace Base
{

template<>
inline gp_Trsf convertTo<gp_Trsf, Base::Placement>(const Base::Placement& placement)
{
    const auto matrix = placement.toMatrix();

    gp_Trsf transform;
    transform.SetValues(
        matrix[0][0],
        matrix[0][1],
        matrix[0][2],
        matrix[0][3],
        matrix[1][0],
        matrix[1][1],
        matrix[1][2],
        matrix[1][3],
        matrix[2][0],
        matrix[2][1],
        matrix[2][2],
        matrix[2][3]
    );

    return transform;
}

// Base::Placement convertTo<Base::Placement, gp_Trsf>(const gp_Trsf& transform) is defined in
// Part/App/Tools.h

}  // namespace Base
