/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
#include <limits>
#include <Adaptor3d_IsoCurve.hxx>
#include <BRepAdaptor_CompCurve.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepGProp.hxx>
#include <BRepGProp_Face.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_IntCS.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Precision.hxx>
#include <Standard_Version.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <cmath>  //OvG: Required for log10
#include <gp_Cylinder.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#if OCC_VERSION_HEX < 0x070600
#include <Adaptor3d_HSurface.hxx>
#include <BRepAdaptor_HSurface.hxx>
#endif
#endif

#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/FeaturePythonPyImp.h>
#include <App/Datums.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Tools.h>

#include "FemConstraint.h"
#include "FemTools.h"


using namespace Fem;
namespace sp = std::placeholders;

#if OCC_VERSION_HEX >= 0x070600
using Adaptor3d_HSurface = Adaptor3d_Surface;
using BRepAdaptor_HSurface = BRepAdaptor_Surface;
#endif

static const App::PropertyFloatConstraint::Constraints scaleConstraint = {
    0.0,
    std::numeric_limits<double>::max(),
    0.1};

PROPERTY_SOURCE(Fem::Constraint, App::DocumentObject)

Constraint::Constraint()
    : sizeFactor {1}
{
    ADD_PROPERTY_TYPE(References,
                      (nullptr, nullptr),
                      "Constraint",
                      (App::PropertyType)(App::Prop_None),
                      "Elements where the constraint is applied");
    ADD_PROPERTY_TYPE(NormalDirection,
                      (Base::Vector3d(0, 0, 1)),
                      "Constraint",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Normal direction pointing outside of solid");
    ADD_PROPERTY_TYPE(Scale,
                      (1),
                      "Constraint",
                      App::PropertyType(App::Prop_None),
                      "Scale used for drawing constraints");
    ADD_PROPERTY_TYPE(Points,
                      (Base::Vector3d()),
                      "Constraint",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output | App::Prop_Hidden),
                      "Points where symbols are drawn");
    ADD_PROPERTY_TYPE(Normals,
                      (Base::Vector3d()),
                      "Constraint",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output | App::Prop_Hidden),
                      "Normals where symbols are drawn");

    Scale.setConstraints(&scaleConstraint);

    Points.setValues(std::vector<Base::Vector3d>());
    Normals.setValues(std::vector<Base::Vector3d>());

    References.setScope(App::LinkScope::Global);

    App::SuppressibleExtension::initExtension(this);
}

Constraint::~Constraint()
{
    connDocChangedObject.disconnect();
}

App::DocumentObjectExecReturn* Constraint::execute()
{
    try {
        References.touch();
        Scale.touch();
        return StdReturn;
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString(), this);
    }
}

// Provide the ability to determine how big to draw constraint arrows etc.
// Try to get symbol size equal to 1/5 of the characteristic length of
// the object. Typical symbol size is 5, so use 1/25 of the characteristic length.
double Constraint::calcSizeFactor(double characLen) const
{
    double l = characLen / 25.0;
    l = ((round(l)) > 1) ? round(l) : l;
    return (l > Precision::Confusion() ? l : 1);
}

float Constraint::getScaleFactor() const
{
    return Scale.getValue() * sizeFactor;
}

constexpr int CONSTRAINTSTEPLIMIT = 50;

void Constraint::onChanged(const App::Property* prop)
{
    if (prop == &References) {
        // If References are changed, recalculate the normal direction. If no useful reference is
        // found, use z axis or previous value. If several faces are selected, only the first one is
        // used
        std::vector<App::DocumentObject*> Objects = References.getValues();
        std::vector<std::string> SubElements = References.getSubValues();

        // Extract geometry from References
        TopoDS_Shape sh;

        bool execute = this->isRecomputing();
        for (std::size_t i = 0; i < Objects.size(); i++) {
            App::DocumentObject* obj = Objects[i];
            Part::Feature* feat = static_cast<Part::Feature*>(obj);
            sh = Tools::getFeatureSubShape(feat, SubElements[i].c_str(), !execute);
            if (!sh.IsNull() && sh.ShapeType() == TopAbs_FACE) {
                // Get face normal in center point
                TopoDS_Face face = TopoDS::Face(sh);
                BRepGProp_Face props(face);
                gp_Vec normal;
                gp_Pnt center;
                double u1, u2, v1, v2;
                props.Bounds(u1, u2, v1, v2);
                props.Normal((u1 + u2) / 2.0, (v1 + v2) / 2.0, center, normal);
                normal.Normalize();
                NormalDirection.setValue(normal.X(), normal.Y(), normal.Z());
                // One face is enough...
                break;
            }
        }

        std::vector<Base::Vector3d> points;
        std::vector<Base::Vector3d> normals;
        if (getPoints(points, normals, &sizeFactor)) {
            Points.setValues(points);
            Normals.setValues(normals);
            Points.touch();
        }
    }

    App::DocumentObject::onChanged(prop);
}

void Constraint::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
    if (Obj.isDerivedFrom<App::GeoFeature>()
        && (Prop.isDerivedFrom<App::PropertyPlacement>() || Obj.isRemoving())) {
        for (const auto ref : References.getValues()) {
            auto v = ref->getInListEx(true);
            if ((&Obj == ref) || (std::ranges::find(v, &Obj) != v.end())) {
                this->touch();
                return;
            }
        }
    }
}

void Constraint::onSettingDocument()
{
    App::Document* doc = getDocument();
    if (doc) {
        connDocChangedObject = doc->signalChangedObject.connect(
            std::bind(&Constraint::slotChangedObject, this, sp::_1, sp::_2));
    }

    App::DocumentObject::onSettingDocument();
}

void Constraint::unsetupObject()
{
    connDocChangedObject.disconnect();
}

void Constraint::onDocumentRestored()
{
    // This seems to be the only way to make the ViewProvider display the constraint
    References.touch();
    App::DocumentObject::onDocumentRestored();
}

void Constraint::handleChangedPropertyType(Base::XMLReader& reader,
                                           const char* TypeName,
                                           App::Property* prop)
{
    // Old integer Scale is equal to sizeFactor, now  Scale*sizeFactor is used to scale the symbol
    if (prop == &Scale && strcmp(TypeName, "App::PropertyInteger") == 0) {
        Scale.setValue(1.0f);
    }
    else {
        App::DocumentObject::handleChangedPropertyType(reader, TypeName, prop);
    }
}

bool Constraint::getPoints(std::vector<Base::Vector3d>& points,
                           std::vector<Base::Vector3d>& normals,
                           double* scale) const
{
    std::vector<App::DocumentObject*> Objects = References.getValues();
    std::vector<std::string> SubElements = References.getSubValues();

    // Extract geometry from References
    TopoDS_Shape sh;

    for (std::size_t i = 0; i < Objects.size(); i++) {
        Part::Feature* feat = static_cast<Part::Feature*>(Objects[i]);
        sh = Tools::getFeatureSubShape(feat, SubElements[i].c_str(), true);
        if (sh.IsNull()) {
            return false;
        }

        // Scale by bounding box of the object
        Bnd_Box box;
        BRepBndLib::Add(feat->Shape.getShape().getShape(), box);
        double l = sqrt(box.SquareExtent() / 3.0);
        *scale = this->calcSizeFactor(l);

        if (sh.ShapeType() == TopAbs_VERTEX) {
            const TopoDS_Vertex& vertex = TopoDS::Vertex(sh);
            gp_Pnt p = BRep_Tool::Pnt(vertex);
            points.emplace_back(p.X(), p.Y(), p.Z());
            normals.push_back(NormalDirection.getValue());
        }
        else if (sh.ShapeType() == TopAbs_EDGE) {
            BRepAdaptor_Curve curve(TopoDS::Edge(sh));
            double fp = curve.FirstParameter();
            double lp = curve.LastParameter();
            // Create points with 10 units distance, but at least one at the beginning and end of
            // the edge
            int steps;
            // OvG: Increase 10 units distance proportionately to l for larger objects.
            if (l >= 30) {
                steps = static_cast<int>(round(l / (10 * (*scale))));
                steps = steps < 3 ? 3 : steps;
            }
            else if (l >= 20) {
                steps = static_cast<int>(round(l / 10));
            }
            else {
                steps = 1;
            }

            // OvG: Place upper limit on number of steps
            steps = steps > CONSTRAINTSTEPLIMIT ? CONSTRAINTSTEPLIMIT : steps;
            double step = (lp - fp) / steps;
            for (int i = 0; i < steps + 1; i++) {
                // Parameter values must be in the range [fp, lp] (#0003683)
                gp_Pnt p = curve.Value(fp + i * step);
                points.emplace_back(p.X(), p.Y(), p.Z());
                normals.push_back(NormalDirection.getValue());
            }
        }
        else if (sh.ShapeType() == TopAbs_FACE) {
            TopoDS_Face face = TopoDS::Face(sh);

            // Surface boundaries
            BRepAdaptor_Surface surface(face);
            double ufp = surface.FirstUParameter();
            double ulp = surface.LastUParameter();
            double vfp = surface.FirstVParameter();
            double vlp = surface.LastVParameter();
            double l;
            double lv, lu;

            // Surface normals
            BRepGProp_Face props(face);
            gp_Vec normal;
            gp_Pnt center;

            // Get an estimate for the number of arrows by finding the average length of curves
            Handle(Adaptor3d_HSurface) hsurf;
            hsurf = new BRepAdaptor_HSurface(surface);

            Adaptor3d_IsoCurve isoc(hsurf);
            try {
                isoc.Load(GeomAbs_IsoU, ufp);
                l = GCPnts_AbscissaPoint::Length(isoc, Precision::Confusion());
            }
            catch (const Standard_Failure&) {
                gp_Pnt p1 = hsurf->Value(ufp, vfp);
                gp_Pnt p2 = hsurf->Value(ufp, vlp);
                l = p1.Distance(p2);
            }

            try {
                isoc.Load(GeomAbs_IsoU, ulp);
                lv = (l + GCPnts_AbscissaPoint::Length(isoc, Precision::Confusion())) / 2.0;
            }
            catch (const Standard_Failure&) {
                gp_Pnt p1 = hsurf->Value(ulp, vfp);
                gp_Pnt p2 = hsurf->Value(ulp, vlp);
                lv = (l + p1.Distance(p2)) / 2.0;
            }

            try {
                isoc.Load(GeomAbs_IsoV, vfp);
                l = GCPnts_AbscissaPoint::Length(isoc, Precision::Confusion());
            }
            catch (const Standard_Failure&) {
                gp_Pnt p1 = hsurf->Value(ufp, vfp);
                gp_Pnt p2 = hsurf->Value(ulp, vfp);
                l = p1.Distance(p2);
            }

            try {
                isoc.Load(GeomAbs_IsoV, vlp);
                lu = (l + GCPnts_AbscissaPoint::Length(isoc, Precision::Confusion())) / 2.0;
            }
            catch (const Standard_Failure&) {
                gp_Pnt p1 = hsurf->Value(ufp, vlp);
                gp_Pnt p2 = hsurf->Value(ulp, vlp);
                lu = (l + p1.Distance(p2)) / 2.0;
            }

            // OvG: Increase 10 units distance proportionately to lv for larger objects.
            int stepsv;
            if (lv >= 30) {
                stepsv = static_cast<int>(round(lv / (10 * (*scale))));
                stepsv = stepsv < 3 ? 3 : stepsv;
            }
            else if (lv >= 20.0) {
                stepsv = static_cast<int>(round(lv / 10));
            }
            else {
                // Minimum of three arrows to ensure (as much as possible) that at
                // least one is displayed
                stepsv = 2;
            }

            // OvG: Place upper limit on number of steps
            stepsv = stepsv > CONSTRAINTSTEPLIMIT ? CONSTRAINTSTEPLIMIT : stepsv;
            int stepsu;
            // OvG: Increase 10 units distance proportionately to lu for larger objects.
            if (lu >= 30) {
                stepsu = static_cast<int>(round(lu / (10 * (*scale))));
                stepsu = stepsu < 3 ? 3 : stepsu;
            }
            else if (lu >= 20.0) {
                stepsu = static_cast<int>(round(lu / 10));
            }
            else {
                stepsu = 2;
            }

            // OvG: Place upper limit on number of steps
            stepsu = stepsu > CONSTRAINTSTEPLIMIT ? CONSTRAINTSTEPLIMIT : stepsu;
            double stepv = (vlp - vfp) / stepsv;
            double stepu = (ulp - ufp) / stepsu;

            // Create points and normals
            auto fillPointsAndNormals = [&](Standard_Real u, Standard_Real v) {
                gp_Pnt p = surface.Value(u, v);
                BRepClass_FaceClassifier classifier(face, p, Precision::Confusion());
                if (classifier.State() != TopAbs_OUT) {
                    points.emplace_back(p.X(), p.Y(), p.Z());
                    props.Normal(u, v, center, normal);
                    if (normal.SquareMagnitude() > 0.0) {
                        normal.Normalize();
                    }
                    normals.emplace_back(normal.X(), normal.Y(), normal.Z());
                }
            };

            size_t prevSize = points.size();
            for (int i = 0; i < stepsv + 1; i++) {
                for (int j = 0; j < stepsu + 1; j++) {
                    double v = vfp + i * stepv;
                    double u = ufp + j * stepu;
                    fillPointsAndNormals(u, v);
                }
            }

            // it could happen that on a trimmed surface the steps on the iso-curves
            // are outside the surface, so no points are added.
            // In that case use points on the outer wire.
            // https://github.com/FreeCAD/FreeCAD/issues/6073
            if (prevSize == points.size()) {
                BRepAdaptor_CompCurve compCurve(BRepTools::OuterWire(face), Standard_True);
                GProp_GProps linProps;
                BRepGProp::LinearProperties(compCurve.Wire(), linProps);
                double outWireLength = linProps.Mass();
                int stepWire = stepsu + stepsv;
                // apply subshape transformation to the geometry
                gp_Trsf faceTrans = face.Location().Transformation();
                Handle(Geom_Geometry) transGeo =
                    surface.Surface().Surface()->Transformed(faceTrans);
                ShapeAnalysis_Surface surfAnalysis(Handle(Geom_Surface)::DownCast(transGeo));
                for (int i = 0; i < stepWire; ++i) {
                    gp_Pnt p = compCurve.Value(outWireLength * i / stepWire);
                    gp_Pnt2d pUV = surfAnalysis.ValueOfUV(p, Precision::Confusion());
                    fillPointsAndNormals(pUV.X(), pUV.Y());
                }
            }
        }
    }

    return true;
}

Base::Vector3d Constraint::getBasePoint(const Base::Vector3d& base,
                                        const Base::Vector3d& axis,
                                        const App::PropertyLinkSub& location,
                                        const double& dist)
{
    // Get the point specified by Location and Distance
    App::DocumentObject* objLoc = location.getValue();
    std::vector<std::string> names = location.getSubValues();
    if (names.empty()) {
        return Base::Vector3d(0, 0, 0);
    }
    std::string subName = names.front();
    Part::Feature* featLoc = static_cast<Part::Feature*>(objLoc);
    TopoDS_Shape shloc = featLoc->Shape.getShape().getSubShape(subName.c_str());

    // Get a plane from the Location reference
    gp_Pln plane;
    gp_Dir cylaxis(axis.x, axis.y, axis.z);
    if (shloc.ShapeType() == TopAbs_FACE) {
        BRepAdaptor_Surface surface(TopoDS::Face(shloc));
        plane = surface.Plane();
    }
    else {
        BRepAdaptor_Curve curve(TopoDS::Edge(shloc));
        gp_Lin line = curve.Line();
        gp_Dir tang = line.Direction().Crossed(cylaxis);
        gp_Dir norm = line.Direction().Crossed(tang);
        plane = gp_Pln(line.Location(), norm);
    }

    // Translate the plane in direction of the cylinder (for positive values of Distance)
    Handle(Geom_Plane) pln = new Geom_Plane(plane);
    gp_Pnt cylbase(base.x, base.y, base.z);
    GeomAPI_ProjectPointOnSurf proj(cylbase, pln);
    if (!proj.IsDone()) {
        return Base::Vector3d(0, 0, 0);
    }

    gp_Pnt projPnt = proj.NearestPoint();
    if ((fabs(dist) > Precision::Confusion())
        && (projPnt.IsEqual(cylbase, Precision::Confusion()) == Standard_False)) {
        plane.Translate(gp_Vec(projPnt, cylbase).Normalized().Multiplied(dist));
    }
    Handle(Geom_Plane) plnt = new Geom_Plane(plane);

    // Intersect translated plane with cylinder axis
    Handle(Geom_Curve) crv = new Geom_Line(cylbase, cylaxis);
    GeomAPI_IntCS intersector(crv, plnt);
    if (!intersector.IsDone()) {
        return Base::Vector3d(0, 0, 0);
    }
    gp_Pnt inter = intersector.Point(1);
    return Base::Vector3d(inter.X(), inter.Y(), inter.Z());
}

const Base::Vector3d Constraint::getDirection(const App::PropertyLinkSub& direction)
{
    App::DocumentObject* obj = direction.getValue();
    if (!obj) {
        return Base::Vector3d(0, 0, 0);
    }

    if (obj->isDerivedFrom<App::Line>()) {
        Base::Vector3d vec = static_cast<App::Line*>(obj)->getDirection();
        return vec;
    }

    if (obj->isDerivedFrom<App::Plane>()) {
        Base::Vector3d vec = static_cast<App::Plane*>(obj)->getDirection();
        return vec;
    }

    if (!obj->isDerivedFrom<Part::Feature>()) {
        std::stringstream str;
        str << "Type is not a line, plane or Part object";
        throw Base::TypeError(str.str());
    }

    std::vector<std::string> names = direction.getSubValues();
    if (names.empty()) {
        return Base::Vector3d(0, 0, 0);
    }
    std::string subName = names.front();
    Part::Feature* feat = static_cast<Part::Feature*>(obj);
    const Part::TopoShape& shape = feat->Shape.getShape();
    if (shape.isNull()) {
        return Base::Vector3d(0, 0, 0);
    }
    TopoDS_Shape sh;
    try {
        sh = shape.getSubShape(subName.c_str());
    }
    catch (Standard_Failure&) {
        std::stringstream str;
        str << "No such sub-element '" << subName << "'";
        throw Base::AttributeError(str.str());
    }

    return Fem::Tools::getDirectionFromShape(sh);
}

// Python feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Fem::ConstraintPython, Fem::Constraint)
template<>
const char* Fem::ConstraintPython::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintPython";
}

template<>
PyObject* Fem::ConstraintPython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new App::FeaturePythonPyT<App::DocumentObjectPy>(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// explicit template instantiation
template class FemExport FeaturePythonT<Fem::Constraint>;

/// @endcond

}  // namespace App
