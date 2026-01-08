// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2024 Werner Mayer <wmayer@users.sourceforge.net>                       *
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


#include <Mod/Import/ImportGlobal.h>

#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>

namespace App
{
class DocumentObject;
}

namespace Import
{

/**
 * A class to assist with exporting sketches to dxf.
 */
class ImportExport SketchExportHelper
{
public:
    static TopoDS_Shape projectShape(const TopoDS_Shape& inShape, const gp_Ax2& projectionCS);
    static bool isSketch(App::DocumentObject* obj);
    static TopoDS_Shape getFlatSketchXY(App::DocumentObject* obj);
};

}  // namespace Import
