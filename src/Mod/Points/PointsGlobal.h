// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2019 Werner Mayer <wmayer@users.sourceforge.net>                       *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include <FCGlobal.h>

#ifndef POINTS_GLOBAL_H
# define POINTS_GLOBAL_H


// Points
# ifndef PointsExport
#  ifdef Points_EXPORTS
#   define PointsExport FREECAD_DECL_EXPORT
#  else
#   define PointsExport FREECAD_DECL_IMPORT
#  endif
# endif

// PointsGui
# ifndef PointsGuiExport
#  ifdef PointsGui_EXPORTS
#   define PointsGuiExport FREECAD_DECL_EXPORT
#  else
#   define PointsGuiExport FREECAD_DECL_IMPORT
#  endif
# endif

#endif  // POINTS_GLOBAL_H
