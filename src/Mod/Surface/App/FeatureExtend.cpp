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
#include <GeomAPI_PointsToBSplineSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Precision.hxx>
#include <Standard_Version.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#endif

#include <Base/Tools.h>

#include "FeatureExtend.h"


using namespace Surface;

const App::PropertyIntegerConstraint::Constraints SampleRange = {2, INT_MAX, 1};
const App::PropertyFloatConstraint::Constraints ToleranceRange = {0.0, 10.0, 0.01};
const App::PropertyFloatConstraint::Constraints ExtendRange = {-0.5, 10.0, 0.01};
PROPERTY_SOURCE(Surface::Extend, Part::Spline)

Extend::Extend()
{
    ADD_PROPERTY(Face, (nullptr));
    Face.setScope(App::LinkScope::Global);
    ADD_PROPERTY(Tolerance, (0.1));
    Tolerance.setConstraints(&ToleranceRange);

    ADD_PROPERTY(ExtendUNeg, (0.05));
    ExtendUNeg.setConstraints(&ExtendRange);
    ADD_PROPERTY(ExtendUPos, (0.05));
    ExtendUPos.setConstraints(&ExtendRange);
    ADD_PROPERTY(ExtendUSymetric, (true));

    ADD_PROPERTY(ExtendVNeg, (0.05));
    ExtendVNeg.setConstraints(&ExtendRange);
    ADD_PROPERTY(ExtendVPos, (0.05));
    ExtendVPos.setConstraints(&ExtendRange);
    ADD_PROPERTY(ExtendVSymetric, (true));

    ADD_PROPERTY(SampleU, (32));
    SampleU.setConstraints(&SampleRange);
    ADD_PROPERTY(SampleV, (32));
    SampleV.setConstraints(&SampleRange);
}

short Extend::mustExecute() const
{
    if (Face.isTouched()) {
        return 1;
    }
    if (ExtendUNeg.isTouched()) {
        return 1;
    }
    if (ExtendUPos.isTouched()) {
        return 1;
    }
    if (ExtendVNeg.isTouched()) {
        return 1;
    }
    if (ExtendVPos.isTouched()) {
        return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn* Extend::execute()
{
    App::DocumentObject* part = Face.getValue();
    if (!part || !part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("No shape linked.");
    }
    const auto& faces = Face.getSubValues();
    if (faces.size() != 1) {
        return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");
    }

    TopoDS_Shape shape =
        static_cast<Part::Feature*>(part)->Shape.getShape().getSubShape(faces[0].c_str());
    if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE) {
        return new App::DocumentObjectExecReturn("Sub-shape is not a face.");
    }

    const TopoDS_Face& face = TopoDS::Face(shape);
    BRepAdaptor_Surface adapt(face);
    double u1 = adapt.FirstUParameter();
    double u2 = adapt.LastUParameter();
    double v1 = adapt.FirstVParameter();
    double v2 = adapt.LastVParameter();

    double ur = u2 - u1;
    double vr = v2 - v1;
    double eu1 = u1 - ur * ExtendUNeg.getValue();
    double eu2 = u2 + ur * ExtendUPos.getValue();
    double ev1 = v1 - vr * ExtendVNeg.getValue();
    double ev2 = v2 + vr * ExtendVPos.getValue();
    double eur = eu2 - eu1;
    double evr = ev2 - ev1;

    long numU = SampleU.getValue();
    long numV = SampleV.getValue();
    TColgp_Array2OfPnt approxPoints(1, numU, 1, numV);
    for (long u = 0; u < numU; u++) {
        double uu = eu1 + u * eur / (numU - 1);
        for (long v = 0; v < numV; v++) {
            double vv = ev1 + v * evr / (numV - 1);
            BRepLProp_SLProps prop(adapt, uu, vv, 0, Precision::Confusion());
            const gp_Pnt& pnt = prop.Value();
            approxPoints(u + 1, v + 1) = pnt;
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
    BRepBuilderAPI_MakeFace mkFace(surface, Precision::Confusion());

    Shape.setValue(mkFace.Face());

    return StdReturn;
}

void Extend::onChanged(const App::Property* prop)
{
    // using a mutex and lock to protect a recursive calling when setting the new values
    if (lockOnChangeMutex) {
        return;
    }
    Base::StateLocker lock(lockOnChangeMutex);

    if (ExtendUSymetric.getValue()) {
        if (prop == &ExtendUNeg || prop == &ExtendUPos) {
            auto changedValue = dynamic_cast<const App::PropertyFloat*>(prop);
            if (changedValue) {
                ExtendUNeg.setValue(changedValue->getValue());
                ExtendUPos.setValue(changedValue->getValue());
            }
        }
    }

    if (ExtendVSymetric.getValue()) {
        if (prop == &ExtendVNeg || prop == &ExtendVPos) {
            auto changedValue = dynamic_cast<const App::PropertyFloat*>(prop);
            if (changedValue) {
                ExtendVNeg.setValue(changedValue->getValue());
                ExtendVPos.setValue(changedValue->getValue());
            }
        }
    }
    Part::Spline::onChanged(prop);
}

void Extend::handleChangedPropertyName(Base::XMLReader& reader,
                                       const char* TypeName,
                                       const char* PropName)
{
    Base::Type type = Base::Type::fromName(TypeName);
    if (App::PropertyFloatConstraint::getClassTypeId() == type
        && strcmp(PropName, "ExtendU") == 0) {
        App::PropertyFloatConstraint v;
        v.Restore(reader);
        ExtendUNeg.setValue(v.getValue());
        ExtendUPos.setValue(v.getValue());
    }
    else if (App::PropertyFloatConstraint::getClassTypeId() == type
             && strcmp(PropName, "ExtendV") == 0) {
        App::PropertyFloatConstraint v;
        v.Restore(reader);
        ExtendVNeg.setValue(v.getValue());
        ExtendVPos.setValue(v.getValue());
    }
    else {
        Part::Spline::handleChangedPropertyName(reader, TypeName, PropName);
    }
}
