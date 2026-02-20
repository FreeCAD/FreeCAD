// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include <App/PropertyContainer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/MeasureManager.h>
#include <Base/Tools.h>
#include <Base/Precision.h>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <GeomAPI_IntSS.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <BRep_Tool.hxx>

#include "MeasureAngle.h"
#include "Mod/Part/App/PartFeature.h"

using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureAngle, Measure::MeasureBase)


MeasureAngle::MeasureAngle()
{
    ADD_PROPERTY_TYPE(Element1, (nullptr), "Measurement", App::Prop_None, "First element of the measurement");
    Element1.setScope(App::LinkScope::Global);
    Element1.setAllowExternal(true);

    ADD_PROPERTY_TYPE(
        Element2,
        (nullptr),
        "Measurement",
        App::Prop_None,
        "Second element of the measurement"
    );
    Element2.setScope(App::LinkScope::Global);
    Element2.setAllowExternal(true);

    ADD_PROPERTY_TYPE(
        Angle,
        (0.0),
        "Measurement",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
        "Angle between the two elements"
    );
    Angle.setUnit(Base::Unit::Angle);
}

MeasureAngle::~MeasureAngle() = default;


bool MeasureAngle::isValidSelection(const App::MeasureSelection& selection)
{
    if (selection.size() != 2) {
        return false;
    }

    for (auto element : selection) {
        auto type = App::MeasureManager::getMeasureElementType(element);

        if (type == App::MeasureElementType::INVALID) {
            return false;
        }

        if (!(type == App::MeasureElementType::LINE || type == App::MeasureElementType::PLANE
              || type == App::MeasureElementType::LINESEGMENT)) {
            return false;
        }
    }
    return true;
}

bool MeasureAngle::isPrioritizedSelection(const App::MeasureSelection& selection)
{
    if (selection.size() != 2) {
        return false;
    }

    // Check if the two elements are parallel
    auto element1 = selection.at(0);
    auto objT1 = element1.object;
    App::DocumentObject* ob1 = objT1.getObject();
    std::string sub1 = objT1.getSubName();
    Base::Vector3d vec1;
    getVec(*ob1, sub1, vec1);

    auto element2 = selection.at(1);
    auto objT2 = element2.object;
    App::DocumentObject* ob2 = objT2.getObject();
    std::string sub2 = objT2.getSubName();
    Base::Vector3d vec2;
    getVec(*ob2, sub2, vec2);


    double angle = std::fmod(vec1.GetAngle(vec2), std::numbers::pi);
    return angle > Base::Precision::Angular();
}


void MeasureAngle::parseSelection(const App::MeasureSelection& selection)
{

    assert(selection.size() >= 2);

    auto element1 = selection.at(0);
    auto objT1 = element1.object;
    App::DocumentObject* ob1 = objT1.getObject();
    const std::vector<std::string> elems1 = {objT1.getSubName()};
    Element1.setValue(ob1, elems1);

    auto element2 = selection.at(1);
    auto objT2 = element2.object;
    App::DocumentObject* ob2 = objT2.getObject();
    const std::vector<std::string> elems2 = {objT2.getSubName()};
    Element2.setValue(ob2, elems2);
}


bool MeasureAngle::getVec(App::DocumentObject& ob, std::string& subName, Base::Vector3d& vecOut)
{
    App::SubObjectT subject {&ob, subName.c_str()};
    auto info = getMeasureInfo(subject);
    if (!info || !info->valid) {
        return false;
    }

    auto angleInfo = std::dynamic_pointer_cast<Part::MeasureAngleInfo>(info);
    vecOut = angleInfo->orientation;
    return true;
}

Base::Vector3d MeasureAngle::getLoc(App::DocumentObject& ob, std::string& subName)
{
    App::SubObjectT subject {&ob, subName.c_str()};
    auto info = getMeasureInfo(subject);
    if (!info || !info->valid) {
        return Base::Vector3d();
    }

    auto angleInfo = std::dynamic_pointer_cast<Part::MeasureAngleInfo>(info);
    return angleInfo->position;
}

gp_Vec MeasureAngle::vector1()
{

    App::DocumentObject* ob = Element1.getValue();
    std::vector<std::string> subs = Element1.getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return {};
    }

    Base::Vector3d vec;
    getVec(*ob, subs.at(0), vec);
    return gp_Vec(vec.x, vec.y, vec.z);
}

gp_Vec MeasureAngle::vector2()
{
    App::DocumentObject* ob = Element2.getValue();
    std::vector<std::string> subs = Element2.getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return gp_Vec();
    }

    Base::Vector3d vec;
    getVec(*ob, subs.at(0), vec);
    return gp_Vec(vec.x, vec.y, vec.z);
}

gp_Vec MeasureAngle::location1()
{

    App::DocumentObject* ob = Element1.getValue();
    std::vector<std::string> subs = Element1.getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return {};
    }
    auto temp = getLoc(*ob, subs.at(0));
    return {temp.x, temp.y, temp.z};
}
gp_Vec MeasureAngle::location2()
{
    App::DocumentObject* ob = Element2.getValue();
    std::vector<std::string> subs = Element2.getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return {};
    }

    auto temp = getLoc(*ob, subs.at(0));
    return {temp.x, temp.y, temp.z};
}

bool MeasureAngle::isGeometricalSame(const TopoDS_Edge& e1, const TopoDS_Edge& e2)
{
    TopoDS_Vertex v1_1, v1_2, v2_1, v2_2;
    TopExp::Vertices(e1, v1_1, v1_2);
    TopExp::Vertices(e2, v2_1, v2_2);

    gp_Pnt p1_1 = BRep_Tool::Pnt(v1_1);
    gp_Pnt p1_2 = BRep_Tool::Pnt(v1_2);
    gp_Pnt p2_1 = BRep_Tool::Pnt(v2_1);
    gp_Pnt p2_2 = BRep_Tool::Pnt(v2_2);

    double tol = Precision::Confusion();
    bool matchStartStart = p1_1.IsEqual(p2_1, tol) && p1_2.IsEqual(p2_2, tol);
    bool matchStartEnd = p1_1.IsEqual(p2_2, tol) && p1_2.IsEqual(p2_1, tol);

    return matchStartStart || matchStartEnd;
}

bool MeasureAngle::setOrigin()
{
    TopoDS_Shape s1 = Part::Feature::getShape(
        Element1.getValue(),
        Part::ShapeOption::NeedSubElement,
        Element1.getSubValues().at(0).c_str()
    );
    TopoDS_Shape s2 = Part::Feature::getShape(
        Element2.getValue(),
        Part::ShapeOption::NeedSubElement,
        Element2.getSubValues().at(0).c_str()
    );
    if (s1.IsNull() || s2.IsNull()) {
        return false;
    }

    if (mCase == FaceFace) {
        // find common edge
        TopExp_Explorer exp1(s1, TopAbs_EDGE);
        TopExp_Explorer exp2(s2, TopAbs_EDGE);
        while (exp1.More()) {
            auto ed1 = TopoDS::Edge(exp1.Current());
            exp2.Init(s2, TopAbs_EDGE);
            while (exp2.More()) {
                auto ed2 = TopoDS::Edge(exp2.Current());
                if (ed1.IsSame(ed2) || isGeometricalSame(ed1, ed2)) {
                    // calculate outOrigin from the common edge
                    TopoDS_Vertex v1, v2;
                    TopExp::Vertices(ed1, v1, v2);
                    outOrigin = gp_Pnt((BRep_Tool::Pnt(v1).XYZ() + BRep_Tool::Pnt(v2).XYZ()) / 2);
                    _isImgOrigin = false;
                    return true;
                }
                exp2.Next();
            }
            exp1.Next();
        }

        _isImgOrigin = true;

        // now try for imaginary origin
        gp_Vec loc1 = location1();
        gp_Vec loc2 = location2();
        gp_Vec vector1 = this->vector1();
        gp_Vec vector2 = this->vector2();

        Handle(Geom_Plane) P1 = new Geom_Plane(gp_Pln(gp_Pnt(loc1.XYZ()), gp_Dir(vector1)));
        Handle(Geom_Plane) P2 = new Geom_Plane(gp_Pln(gp_Pnt(loc2.XYZ()), gp_Dir(vector2)));
        GeomAPI_IntSS intersector(P1, P2, Precision::Confusion());
        if (intersector.IsDone() && intersector.NbLines() > 0) {
            Handle(Geom_Curve) curve = intersector.Line(1);

            gp_Pnt refPnt = loc1.XYZ();

            GeomAPI_ProjectPointOnCurve proj(refPnt, curve);
            if (proj.NbPoints() > 0) {
                outOrigin = proj.Point(1);
                return true;
            }
        }
    }
    else if (mCase == EdgeEdge) {
        TopoDS_Edge e1 = TopoDS::Edge(s1);
        TopoDS_Edge e2 = TopoDS::Edge(s2);
        TopoDS_Vertex common;

        if (TopExp::CommonVertex(e1, e2, common)) {
            outOrigin = BRep_Tool::Pnt(common);
            _isImgOrigin = false;
            return true;
        }

        _isImgOrigin = true;

        // now try for imaginary origin
        gp_Vec loc1 = location1();
        gp_Vec loc2 = location2();
        gp_Vec vector1 = this->vector1();
        gp_Vec vector2 = this->vector2();

        Handle(Geom_Line) line1 = new Geom_Line(loc1.XYZ(), vector1);
        Handle(Geom_Line) line2 = new Geom_Line(loc2.XYZ(), vector2);

        GeomAPI_ExtremaCurveCurve intersector(line1, line2);
        if (intersector.NbExtrema() > 0) {
            gp_Pnt p1, p2;
            intersector.NearestPoints(p1, p2);
            outOrigin = p1;
            _isImgOrigin = true;
            return true;
        }
    }
    else if (mCase == FaceEdge) {
        _isImgOrigin = true;
        Handle(Geom_Line) line1 = new Geom_Line(location1().XYZ(), vector1());
        Handle(Geom_Line) line2 = new Geom_Line(location2().XYZ(), vector2());

        GeomAPI_ExtremaCurveCurve intersector(line1, line2);
        if (intersector.NbExtrema() > 0) {
            gp_Pnt p1, p2;
            intersector.NearestPoints(p1, p2);
            outOrigin = p2;
            _isImgOrigin = true;
            return true;
        }
    }


    // cant reach here
    return false;
}


bool MeasureAngle::getOrigin(gp_Pnt& outOrigin)
{
    outOrigin = this->outOrigin;
    return true;
}

bool MeasureAngle::setDirections()
{

    direction1 = vector1();
    direction2 = vector2();

    gp_Vec loc1 = location1();
    gp_Vec loc2 = location2();

    // For edges with a common vertex, we need to orient the vectors
    // so they point away from the common vertex
    if (mCase == EdgeEdge) {

        if (direction1.Dot(gp_Vec(outOrigin.XYZ()) - loc1) > 0) {
            direction1 = -direction1;
        }
        if (direction2.Dot(gp_Vec(outOrigin.XYZ()) - loc2) > 0) {
            direction2 = -direction2;
        }
    }
    else if (mCase == FaceFace) {

        gp_Vec between = loc2 - loc1;

        if (direction1.Dot(between) < 0) {
            direction1 = -direction1;
        }
        if (direction2.Dot(between) > 0) {
            direction2 = -direction2;
        }
    }
    else if (mCase == FaceEdge) {
        // here we take the face as deciding factor
        gp_Vec faceNormal;
        TopoDS_Shape s1 = Part::Feature::getShape(
            Element1.getValue(),
            Part::ShapeOption::NeedSubElement,
            Element1.getSubValues().at(0).c_str()
        );
        TopoDS_Shape s2 = Part::Feature::getShape(
            Element2.getValue(),
            Part::ShapeOption::NeedSubElement,
            Element2.getSubValues().at(0).c_str()
        );
        faceNormal = (s1.ShapeType() == TopAbs_FACE) ? vector1() : vector2();
        if (direction1.Dot(faceNormal) < 0) {
            direction1 = -direction1;
        }
        if (direction2.Dot(faceNormal) < 0) {
            direction2 = -direction2;
        }
    }
    else {
        // should not reach here
        return false;
    }

    return true;
}

bool MeasureAngle::getDirections(gp_Vec& dir1, gp_Vec& dir2)
{
    if (direction1.Magnitude() == 0 || direction2.Magnitude() == 0) {
        return false;
    }
    dir1 = direction1;
    dir2 = direction2;

    return true;
}

bool MeasureAngle::isImgOrigin()
{
    return _isImgOrigin;
}

App::DocumentObjectExecReturn* MeasureAngle::execute()
{
    App::DocumentObject* ob1 = Element1.getValue();
    std::vector<std::string> subs1 = Element1.getSubValues();

    App::DocumentObject* ob2 = Element2.getValue();
    std::vector<std::string> subs2 = Element2.getSubValues();

    _isImgOrigin = false;

    if (!ob1 || !ob1->isValid() || !ob2 || !ob2->isValid()) {
        return new App::DocumentObjectExecReturn("Submitted object(s) is not valid");
    }

    if (subs1.empty() || subs2.empty()) {
        return new App::DocumentObjectExecReturn("No geometry element picked");
    }

    TopoDS_Shape s1
        = Part::Feature::getShape(ob1, Part::ShapeOption::NeedSubElement, subs1.at(0).c_str());
    TopoDS_Shape s2
        = Part::Feature::getShape(ob2, Part::ShapeOption::NeedSubElement, subs2.at(0).c_str());
    if (s1.ShapeType() == TopAbs_FACE && s2.ShapeType() == TopAbs_FACE) {
        mCase = FaceFace;
    }
    else if (s1.ShapeType() == TopAbs_EDGE && s2.ShapeType() == TopAbs_EDGE) {
        mCase = EdgeEdge;
    }
    else {
        mCase = FaceEdge;
    }

    if (!setOrigin() || !setDirections()) {
        return new App::DocumentObjectExecReturn("Failed to Set Origin");
    }


    double angleRad = direction1.Angle(direction2);

    // because of face normal are perpendicular to face
    if (mCase == FaceFace) {
        angleRad = M_PI - angleRad;
    }

    Angle.setValue(Base::toDegrees(angleRad));

    return DocumentObject::StdReturn;
}

void MeasureAngle::onChanged(const App::Property* prop)
{

    if (prop == &Element1 || prop == &Element2) {
        if (!isRestoring()) {
            App::DocumentObjectExecReturn* ret = recompute();
            delete ret;
        }
    }
    DocumentObject::onChanged(prop);
}

//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureAngle::getSubject() const
{
    return {Element1.getValue()};
}
