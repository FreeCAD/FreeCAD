/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
//this file originally part of Drawing workbench
//migrated to TechDraw workbench 2022-01-26 by Wandererfan

#ifndef TECHDRAW_EXPORT_H
#define TECHDRAW_EXPORT_H

#include <string>
#include <TopoDS_Edge.hxx>

#include <Mod/TechDraw/TechDrawGlobal.h>


class TopoDS_Shape;
class BRepAdaptor_Curve;

namespace TechDraw
{

class TechDrawExport TechDrawOutput
{
public:
    // If the curve is approximately a circle it will be returned,
    // otherwise a null edge is returned.
    TopoDS_Edge asCircle(const BRepAdaptor_Curve&) const;
    TopoDS_Edge asBSpline(const BRepAdaptor_Curve&, int maxDegree) const;
};

class TechDrawExport SVGOutput : public TechDrawOutput
{
public:
    SVGOutput();
    std::string exportEdges(const TopoDS_Shape&);

private:
    void printCircle(const BRepAdaptor_Curve&, std::ostream&);
    void printEllipse(const BRepAdaptor_Curve&, int id, std::ostream&);
    void printBSpline(const BRepAdaptor_Curve&, int id, std::ostream&);
    void printBezier(const BRepAdaptor_Curve&, int id, std::ostream&);
    void printGeneric(const BRepAdaptor_Curve&, int id, std::ostream&);
};

/* dxf output section - Dan Falck 2011/09/25  */
class TechDrawExport DXFOutput : public TechDrawOutput
{
public:
    DXFOutput();
    std::string exportEdges(const TopoDS_Shape&);

private:
    void printHeader(std::ostream& out);
    void printCircle(const BRepAdaptor_Curve&, std::ostream&);
    void printEllipse(const BRepAdaptor_Curve&, int id, std::ostream&);
    void printBSpline(const BRepAdaptor_Curve&, int id, std::ostream&);
    void printGeneric(const BRepAdaptor_Curve&, int id, std::ostream&);
};

} //namespace TechDraw

#endif // TECHDRAW_EXPORT_H
