// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef FREECAD_TOPOSHAPEEXPANSIONHELPERS_H
#define FREECAD_TOPOSHAPEEXPANSIONHELPERS_H

#include <TopoDS_Shape.hxx>

namespace TopoShapeExpansionHelpers
{
std::pair<TopoDS_Shape, TopoDS_Shape> CreateTwoCubes();
}

#endif  // FREECAD_TOPOSHAPEEXPANSIONHELPERS_H
