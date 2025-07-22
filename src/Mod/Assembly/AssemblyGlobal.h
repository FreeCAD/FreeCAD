// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <FCGlobal.h>

#ifndef ASSEMBLY_GLOBAL_H
#define ASSEMBLY_GLOBAL_H


// Assembly
#ifndef AssemblyExport
#ifdef Assembly_EXPORTS
#define AssemblyExport FREECAD_DECL_EXPORT
#else
#define AssemblyExport FREECAD_DECL_IMPORT
#endif
#endif

// AssemblyGui
#ifndef AssemblyGuiExport
#ifdef AssemblyGui_EXPORTS
#define AssemblyGuiExport FREECAD_DECL_EXPORT
#else
#define AssemblyGuiExport FREECAD_DECL_IMPORT
#endif
#endif

#endif  // ASSEMBLY_GLOBAL_H
