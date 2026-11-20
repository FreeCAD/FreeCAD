// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileNotice: Part of the xwzCAD project.

/***************************************************************************
 *                                                                         *
 *   xwzCAD is free software: you can redistribute it and/or modify        *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation, either version 2.1         *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   xwzCAD is distributed in the hope that it will be useful,             *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty           *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
 *   See the GNU Lesser General Public License for more details.           *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with xwzCAD. If not, see https://www.gnu.org/licenses   *
 *                                                                         *
 ***************************************************************************/

#ifndef PART_EDGESORT_H
#define PART_EDGESORT_H

#include <list>
#include <TopoDS_Edge.hxx>
#include <Mod/Part/PartGlobal.h>

namespace Part
{

class PartExport EdgeSort
{
public:
    using Wire = std::list<TopoDS_Edge>;
    EdgeSort(double tol3d, const std::list<TopoDS_Edge>& edges);
    Wire performOne();
    std::list<Wire> performAll();
    std::list<TopoDS_Edge> getRemainingEdges() const;

private:
    Wire sortEdges(double tol3d);

private:
    double tolerance;
    std::list<TopoDS_Edge> edges;
};

}  // namespace Part

#endif  // PART_EDGESORT_H
