// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2021 Werner Mayer <wmayer@users.sourceforge.net>                       *
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


#ifndef SPREADSHEET_GLOBAL_H
#define SPREADSHEET_GLOBAL_H

#include <FCGlobal.h>


// Spreadsheet
#ifndef SpreadsheetExport
# ifdef Spreadsheet_EXPORTS
#  define SpreadsheetExport FREECAD_DECL_EXPORT
# else
#  define SpreadsheetExport FREECAD_DECL_IMPORT
# endif
#endif

// SpreadsheetGui
#ifndef SpreadsheetGuiExport
# ifdef SpreadsheetGui_EXPORTS
#  define SpreadsheetGuiExport FREECAD_DECL_EXPORT
# else
#  define SpreadsheetGuiExport FREECAD_DECL_IMPORT
# endif
#endif

#endif  // SPREADSHEET_GLOBAL_H
