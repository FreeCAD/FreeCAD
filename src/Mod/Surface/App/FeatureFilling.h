// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller <Nathan.A.Mill[at]gmail.com>         *
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

#pragma once

#include <App/PropertyLinks.h>
#include <Mod/Part/App/FeaturePartSpline.h>
#include <Mod/Surface/SurfaceGlobal.h>


class BRepFill_Filling;

namespace Surface
{

class SurfaceExport Filling: public Part::Spline
{
    PROPERTY_HEADER_WITH_OVERRIDE(Surface::Filling);

public:
    Filling();

    // Properties of Curves
    App::PropertyLinkSubList BoundaryEdges;  // Boundary Edges (C0 is required for edges without a
                                             // corresponding face)
    App::PropertyStringList BoundaryFaces;   // Boundary Faces (C0, G1 and G2 are possible)
    App::PropertyIntegerList BoundaryOrder;  // Order of constraint on border faces
    App::PropertyLinkSubList UnboundEdges;   // Unbound constraint edges (C0 is required for edges
                                             // without a corresponding face)
    App::PropertyStringList UnboundFaces;   // Unbound constraint faces (C0, G1 and G2 are possible)
    App::PropertyIntegerList UnboundOrder;  // Order of constraint on curve faces
    App::PropertyLinkSubList FreeFaces;     // Free constraint faces
    App::PropertyIntegerList FreeOrder;     // Order of constraint on free faces
    App::PropertyLinkSubList Points;        // Constraint Points (on Surface)
    App::PropertyLinkSub InitialFace;       // Initial Face to use

    // Algorithm Variables
    App::PropertyInteger Degree;           // Starting degree
    App::PropertyInteger PointsOnCurve;    // Number of points on an edge for constraint
    App::PropertyInteger Iterations;       // Number of iterations
    App::PropertyBool Anisotropy;          // Anisotropy
    App::PropertyFloat Tolerance2d;        // 2D Tolerance
    App::PropertyFloat Tolerance3d;        // 3D Tolerance
    App::PropertyFloat TolAngular;         // G1 tolerance
    App::PropertyFloat TolCurvature;       // G2 tolerance
    App::PropertyInteger MaximumDegree;    // Maximum curve degree
    App::PropertyInteger MaximumSegments;  // Maximum number of segments

    // recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "SurfaceGui::ViewProviderFilling";
    }

private:
    void addConstraints(
        BRepFill_Filling& builder,
        const App::PropertyLinkSubList& edges,
        const App::PropertyStringList& faces,
        const App::PropertyIntegerList& orders,
        Standard_Boolean bnd
    );
    void addConstraints(
        BRepFill_Filling& builder,
        const App::PropertyLinkSubList& faces,
        const App::PropertyIntegerList& orders
    );
    void addConstraints(BRepFill_Filling& builder, const App::PropertyLinkSubList& points);
};

}  // Namespace Surface
