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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <gp_Pln.hxx>
# include <gp_Lin.hxx>
# include <Geom_Plane.hxx>
# include <GeomAPI_IntSS.hxx>
# include <Geom_Line.hxx>
# include <Precision.hxx>
#endif

#include <Base/Vector3D.h>
#include "Tools.h"

void Part::closestPointsOnLines(const gp_Lin& lin1, const gp_Lin& lin2, gp_Pnt& p1, gp_Pnt& p2)
{
    // they might be the same point
    gp_Vec v1(lin1.Direction());
    gp_Vec v2(lin2.Direction());
    gp_Vec v3(lin2.Location(), lin1.Location());

    double a = v1*v1;
    double b = v1*v2;
    double c = v2*v2;
    double d = v1*v3;
    double e = v2*v3;
    double D = a*c - b*b;
    double s, t;

    // D = (v1 x v2) * (v1 x v2)
    if (D < Precision::Angular()){
        // the lines are considered parallel
        s = 0.0;
        t = (b>c ? d/b : e/c);
    }
    else {
        s = (b*e - c*d) / D;
        t = (a*e - b*d) / D;
    }

    p1 = lin1.Location().XYZ() + s * v1.XYZ();
    p2 = lin2.Location().XYZ() + t * v2.XYZ();
}

bool Part::intersect(const gp_Pln& pln1, const gp_Pln& pln2, gp_Lin& lin)
{
    bool found = false;
    Handle (Geom_Plane) gp1 = new Geom_Plane(pln1);
    Handle (Geom_Plane) gp2 = new Geom_Plane(pln2);

    GeomAPI_IntSS intSS(gp1, gp2, Precision::Confusion());
    if (intSS.IsDone()) {
        int numSol = intSS.NbLines();
        if (numSol > 0) {
            Handle_Geom_Curve curve = intSS.Line(1);
            lin = Handle_Geom_Line::DownCast(curve)->Lin();
            found = true;
        }
    }

    return found;
}
