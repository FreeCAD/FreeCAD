/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef PART_CROSSSECTION_H
#define PART_CROSSSECTION_H

#include <list>
#include <TopTools_IndexedMapOfShape.hxx>
#include <Mod/Part/PartGlobal.h>


class TopoDS_Shape;
class TopoDS_Wire;

namespace Part {

class PartExport CrossSection
{
public:
    CrossSection(double a, double b, double c, const TopoDS_Shape& s);
    std::list<TopoDS_Wire> slice(double d) const;

private:
    void sliceNonSolid(double d, const TopoDS_Shape&, std::list<TopoDS_Wire>& wires) const;
    void sliceSolid(double d, const TopoDS_Shape&, std::list<TopoDS_Wire>& wires) const;
    void connectEdges (const std::list<TopoDS_Edge>& edges, std::list<TopoDS_Wire>& wires) const;
    void connectWires (const TopTools_IndexedMapOfShape& wireMap, std::list<TopoDS_Wire>& wires) const;
    TopoDS_Wire fixWire(const TopoDS_Wire& wire) const;
    std::list<TopoDS_Wire> removeDuplicates(const std::list<TopoDS_Wire>& wires) const;

private:
    double a,b,c;
    const TopoDS_Shape& s;
};

}

#endif // PART_CROSSSECTION_H
