/***************************************************************************
 *   Copyright (c) 2023 Wandererfan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

//! a class for establishing our connection with the unified measurement facility

#ifndef SKETCHER_MEASURE_H
#define SKETCHER_MEASURE_H

#include <Mod/Sketcher/SketcherGlobal.h>


namespace Sketcher
{


class SketcherExport Measure
{
public:
    static void initialize();
};


}  // namespace Sketcher

#endif
