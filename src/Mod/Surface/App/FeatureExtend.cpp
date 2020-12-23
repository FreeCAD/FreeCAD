/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepLProp_SLProps.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <gp_Pnt.hxx>
#include <Geom_BSplineSurface.hxx>
#include <GeomAPI_PointsToBSplineSurface.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <Precision.hxx>
#include <Standard_Version.hxx>
#endif

#include "FeatureExtend.h"
#include <Base/Tools.h>
#include <Base/Exception.h>

using namespace Surface;

const App::PropertyIntegerConstraint::Constraints SampleRange = {2,INT_MAX,1};
const App::PropertyFloatConstraint::Constraints ToleranceRange = {0.0,10.0,0.01};
const App::PropertyFloatConstraint::Constraints ExtendRange = {-0.5,10.0,0.01};
PROPERTY_SOURCE(Surface::Extend, Part::Spline)

Extend::Extend()
{
    ADD_PROPERTY(Face,(0));
    Face.setScope(App::LinkScope::Global);
    ADD_PROPERTY(Tolerance, (0.1));
    Tolerance.setConstraints(&ToleranceRange);
    ADD_PROPERTY(ExtendU, (0.05));
    ExtendU.setConstraints(&ExtendRange);
    ADD_PROPERTY(ExtendV, (0.05));
    ExtendV.setConstraints(&ExtendRange);
    ADD_PROPERTY(SampleU, (32));
    SampleU.setConstraints(&SampleRange);
    ADD_PROPERTY(SampleV, (32));
    SampleV.setConstraints(&SampleRange);
}

Extend::~Extend()
{
}

short Extend::mustExecute() const
{
    if (Face.isTouched())
        return 1;
    if (ExtendU.isTouched())
        return 1;
    if (ExtendV.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Extend::execute(void)
{
    App::DocumentObject* part = Face.getValue();
    if (!part || !part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("No shape linked.");
    const auto& faces = Face.getSubValues();
    if (faces.size() != 1)
        return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");

    TopoDS_Shape shape = static_cast<Part::Feature*>(part)
            ->Shape.getShape().getSubShape(faces[0].c_str());
    if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE)
        return new App::DocumentObjectExecReturn("Sub-shape is not a face.");

    const TopoDS_Face& face = TopoDS::Face(shape);
    BRepAdaptor_Surface adapt(face);
    double u1 = adapt.FirstUParameter();
    double u2 = adapt.LastUParameter();
    double v1 = adapt.FirstVParameter();
    double v2 = adapt.LastVParameter();

    double ur = u2 - u1;
    double vr = v2 - v1;
    double eu1 = u1 - ur*ExtendU.getValue();
    double eu2 = u2 + ur*ExtendU.getValue();
    double ev1 = v1 - vr*ExtendV.getValue();
    double ev2 = v2 + vr*ExtendV.getValue();
    double eur = eu2 - eu1;
    double evr = ev2 - ev1;

    long numU = SampleU.getValue();
    long numV = SampleV.getValue();
    TColgp_Array2OfPnt approxPoints(1, numU, 1, numV);
    for (long u=0; u<numU; u++) {
        double uu = eu1 + u * eur / (numU-1);
        for (long v=0; v<numV; v++) {
            double vv = ev1 + v * evr / (numV-1);
            BRepLProp_SLProps prop(adapt,uu,vv,0,Precision::Confusion());
            const gp_Pnt& pnt = prop.Value();
            approxPoints(u+1, v+1) = pnt;
        }
    }

    Approx_ParametrizationType ParType = Approx_ChordLength;
    Standard_Integer DegMin = 3;
    Standard_Integer DegMax = 5;
    GeomAbs_Shape Continuity = GeomAbs_C2;
    Standard_Real Tol3d = Tolerance.getValue();

    GeomAPI_PointsToBSplineSurface approx;
    approx.Init(approxPoints, ParType, DegMin, DegMax, Continuity, Tol3d);

    Handle(Geom_BSplineSurface) surface(approx.Surface());
    BRepBuilderAPI_MakeFace mkFace(surface
#if OCC_VERSION_HEX >= 0x060502
      , Precision::Confusion()
#endif
    );

    Shape.setValue(mkFace.Face());

    return StdReturn;
}
