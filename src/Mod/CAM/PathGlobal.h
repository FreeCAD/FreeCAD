// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021 Werner Mayer
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
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

#pragma once


// Path
#ifndef PathExport
# ifdef Path_EXPORTS
#  define PathExport FREECAD_DECL_EXPORT
# else
#  define PathExport FREECAD_DECL_IMPORT
# endif
#endif

// PathGui
#ifndef PathGuiExport
# ifdef PathGui_EXPORTS
#  define PathGuiExport FREECAD_DECL_EXPORT
# else
#  define PathGuiExport FREECAD_DECL_IMPORT
# endif
#endif

// PathSimulator
#ifndef PathSimulatorExport
# ifdef PathSimulator_EXPORTS
#  define PathSimulatorExport FREECAD_DECL_EXPORT
# else
#  define PathSimulatorExport FREECAD_DECL_IMPORT
# endif
#endif

// CAMSimulator (new GL simulator)
#ifndef CAMSimulatorExport
# ifdef CAMSimulator_EXPORTS
#  define CAMSimulatorExport FREECAD_DECL_EXPORT
# else
#  define CAMSimulatorExport FREECAD_DECL_IMPORT
# endif
#endif
