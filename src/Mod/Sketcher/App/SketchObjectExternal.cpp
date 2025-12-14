// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <algorithm>
#include <cmath>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Section.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepOffsetAPI_NormalProjection.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Tool.hxx>
#include <ElCLib.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomConvert_BSplineCurveKnotSplitting.hxx>
#include <GeomLProp_CLProps.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Line.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_Plane.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax3.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Hypr.hxx>
#include <gp_Parab.hxx>
#include <gp_Pln.hxx>

#include <boost/algorithm/string/predicate.hpp>

#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>

#include <App/Document.h>
#include <App/ElementNamingUtils.h>
#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <App/ObjectIdentifier.h>
#include <App/Datums.h>
#include <App/Part.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Part/App/DatumFeature.h>

#include <memory>

#include "GeoEnum.h"
#include "SketchObject.h"
#include "ExternalGeometryFacade.h"
#include <Mod/Part/App/Datums.h>


#undef DEBUG
// #define DEBUG

// clang-format off
using namespace Sketcher;
using namespace Base;

FC_LOG_LEVEL_INIT("Sketch", true, true)

void SketchObject::initExternalGeo() {
    std::vector<Part::Geometry *> geos;
    auto HLine = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade();
    auto VLine = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade();
    HLine->getTypedGeometry()->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
    VLine->getTypedGeometry()->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(0,1,0));
    HLine->setConstruction(true);
    HLine->setId(-1);
    VLine->setConstruction(true);
    VLine->setId(-2);
    geos.push_back(HLine->getGeometry());
    geos.push_back(VLine->getGeometry());
    HLine->setOwner(false); // we have transferred the ownership to ExternalGeo
    VLine->setOwner(false); // we have transferred the ownership to ExternalGeo
    ExternalGeo.setValues(std::move(geos));
}

// clang-format on
int SketchObject::toggleExternalGeometryFlag(
    const std::vector<int>& geoIds,
    const std::vector<ExternalGeometryExtension::Flag>& flags
)
{
    if (flags.empty()) {
        return 0;
    }
    auto flag = flags.front();

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    bool update = false;
    bool touched = false;
    auto geos = ExternalGeo.getValues();
    std::set<int> idSet(geoIds.begin(), geoIds.end());
    for (auto geoId : geoIds) {
        if (geoId > GeoEnum::RefExt || -geoId - 1 >= ExternalGeo.getSize()) {
            continue;
        }
        if (!idSet.contains(geoId)) {
            continue;
        }
        idSet.erase(geoId);
        const int idx = -geoId - 1;
        auto& geo = geos[idx];
        const auto egf = ExternalGeometryFacade::getFacade(geo);
        const bool value = !egf->testFlag(flag);
        if (!egf->getRef().empty()) {
            for (auto relatedGeoId : getRelatedGeometry(geoId)) {
                if (relatedGeoId == geoId) {
                    continue;
                }
                int relatedIndex = -relatedGeoId - 1;
                auto& relatedGeometry = geos[relatedIndex];
                relatedGeometry = relatedGeometry->clone();
                auto relatedFacade = ExternalGeometryFacade::getFacade(relatedGeometry);
                for (auto& _flag : flags) {
                    relatedFacade->setFlag(_flag, value);
                }
                idSet.erase(relatedGeoId);
            }
        }
        geo = geo->clone();
        egf->setGeometry(geo);
        for (auto& _flag : flags) {
            egf->setFlag(_flag, value);
        }
        update = update || (value || flag != ExternalGeometryExtension::Frozen);
        touched = true;
    }

    if (!touched) {
        return -1;
    }
    ExternalGeo.setValues(geos);
    if (update) {
        rebuildExternalGeometry();
    }
    return 0;
}

bool SketchObject::isExternalAllowed(App::Document* pDoc, App::DocumentObject* pObj, eReasonList* rsn) const
{
    if (rsn) {
        *rsn = rlAllowed;
    }

    // Externals outside of the Document are NOT allowed
    if (this->getDocument() != pDoc) {
        if (rsn) {
            *rsn = rlOtherDoc;
        }
        return false;
    }

    // circular reference prevention
    try {
        if (!(this->testIfLinkDAGCompatible(pObj))) {
            if (rsn) {
                *rsn = rlCircularReference;
            }
            return false;
        }
    }
    catch (Base::Exception& e) {
        Base::Console().warning(
            "Probably, there is a circular reference in the document. Error: %s\n",
            e.what()
        );
        return true;  // prohibiting this reference won't remove the problem anyway...
    }


    // Note: Checking for the body of the support doesn't work when the support are the three base
    // planes
    Part::BodyBase* body_this = Part::BodyBase::findBodyOf(this);
    Part::BodyBase* body_obj = Part::BodyBase::findBodyOf(pObj);
    App::Part* part_this = App::Part::getPartOfObject(this);
    App::Part* part_obj = App::Part::getPartOfObject(pObj);
    if (part_this == part_obj) {  // either in the same part, or in the root of document
        if (!body_this) {
            return true;
        }
        else if (body_this == body_obj) {
            return true;
        }
        else {
            if (rsn) {
                *rsn = rlOtherBody;
            }
            return false;
        }
    }
    else {
        // cross-part link. Disallow, should be done via shapebinders only
        if (rsn) {
            *rsn = rlOtherPart;
        }
        return false;
    }
}

bool SketchObject::isCarbonCopyAllowed(
    App::Document* pDoc,
    App::DocumentObject* pObj,
    bool& xinv,
    bool& yinv,
    eReasonList* rsn
) const
{
    if (rsn) {
        *rsn = rlAllowed;
    }

    std::string sketchArchType("Sketcher::SketchObjectPython");

    // Only applicable to sketches
    if (!pObj->is<Sketcher::SketchObject>() && sketchArchType != pObj->getTypeId().getName()) {
        if (rsn) {
            *rsn = rlNotASketch;
        }
        return false;
    }


    auto* psObj = static_cast<SketchObject*>(pObj);

    // Sketches outside of the Document are NOT allowed
    if (this->getDocument() != pDoc) {
        if (rsn) {
            *rsn = rlOtherDoc;
        }
        return false;
    }

    // circular reference prevention
    try {
        if (!(this->testIfLinkDAGCompatible(pObj))) {
            if (rsn) {
                *rsn = rlCircularReference;
            }
            return false;
        }
    }
    catch (Base::Exception& e) {
        Base::Console().warning(
            "Probably, there is a circular reference in the document. Error: %s\n",
            e.what()
        );
        return true;  // prohibiting this reference won't remove the problem anyway...
    }


    // Note: Checking for the body of the support doesn't work when the support are the three base
    // planes
    Part::BodyBase* body_this = Part::BodyBase::findBodyOf(this);
    Part::BodyBase* body_obj = Part::BodyBase::findBodyOf(pObj);
    App::Part* part_this = App::Part::getPartOfObject(this);
    App::Part* part_obj = App::Part::getPartOfObject(pObj);
    if (part_this == part_obj) {  // either in the same part, or in the root of document
        if (body_this) {
            if (body_this != body_obj) {
                if (!this->allowOtherBody) {
                    if (rsn) {
                        *rsn = rlOtherBody;
                    }
                    return false;
                }
                // if the original sketch has external geometry AND it is not in this body prevent
                // link
                else if (psObj->getExternalGeometryCount() > 2) {
                    if (rsn) {
                        *rsn = rlOtherBodyWithLinks;
                    }
                    return false;
                }
            }
        }
    }
    else {
        // cross-part relation. Disallow, should be done via shapebinders only
        if (rsn) {
            *rsn = rlOtherPart;
        }
        return false;
    }


    const Rotation& srot = psObj->Placement.getValue().getRotation();
    const Rotation& lrot = this->Placement.getValue().getRotation();

    Base::Vector3d snormal(0, 0, 1);
    Base::Vector3d sx(1, 0, 0);
    Base::Vector3d sy(0, 1, 0);
    srot.multVec(snormal, snormal);
    srot.multVec(sx, sx);
    srot.multVec(sy, sy);

    Base::Vector3d lnormal(0, 0, 1);
    Base::Vector3d lx(1, 0, 0);
    Base::Vector3d ly(0, 1, 0);
    lrot.multVec(lnormal, lnormal);
    lrot.multVec(lx, lx);
    lrot.multVec(ly, ly);

    double dot = snormal * lnormal;
    double dotx = sx * lx;
    double doty = sy * ly;

    // the planes of the sketches must be parallel
    if (!allowUnaligned && fabs(fabs(dot) - 1) > Precision::Confusion()) {
        if (rsn) {
            *rsn = rlNonParallel;
        }
        return false;
    }

    // the axis must be aligned
    if (!allowUnaligned
        && ((fabs(fabs(dotx) - 1) > Precision::Confusion())
            || (fabs(fabs(doty) - 1) > Precision::Confusion()))) {
        if (rsn) {
            *rsn = rlAxesMisaligned;
        }
        return false;
    }


    // the origins of the sketches must be aligned or be the same
    Base::Vector3d ddir = (psObj->Placement.getValue().getPosition()
                           - this->Placement.getValue().getPosition())
                              .Normalize();

    double alignment = ddir * lnormal;

    if (!allowUnaligned && (fabs(fabs(alignment) - 1) > Precision::Confusion())
        && (psObj->Placement.getValue().getPosition() != this->Placement.getValue().getPosition())) {
        if (rsn) {
            *rsn = rlOriginsMisaligned;
        }
        return false;
    }

    xinv = allowUnaligned ? false : (fabs(dotx - 1) > Precision::Confusion());
    yinv = allowUnaligned ? false : (fabs(doty - 1) > Precision::Confusion());

    return true;
}
// clang-format off

int SketchObject::carbonCopy(App::DocumentObject* pObj, bool construction)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // so far only externals to the support of the sketch and datum features
    bool xinv = false, yinv = false;

    if (!isCarbonCopyAllowed(pObj->getDocument(), pObj, xinv, yinv))
        return -1;

    SketchObject* psObj = static_cast<SketchObject*>(pObj);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

    std::vector<Part::Geometry*> newVals(vals);

    std::vector<Constraint*> newcVals(cvals);

    int nextgeoid = vals.size();

    int nextextgeoid = getExternalGeometryCount();

    int nextcid = cvals.size();

    const std::vector<Part::Geometry*>& svals = psObj->getInternalGeometry();

    const std::vector<Sketcher::Constraint*>& scvals = psObj->Constraints.getValues();

    newVals.reserve(vals.size() + svals.size());
    newcVals.reserve(cvals.size() + scvals.size());

    std::map<int, int> extMap;
    if (psObj->ExternalGeo.getSize() > 1) {
        int i = -1;
        auto geos = this->ExternalGeo.getValues();
        std::string myName(this->getNameInDocument());
        myName += ".";
        for (const auto &geo : psObj->ExternalGeo.getValues()) {
            if (++i < 2) // skip h/v axes
                continue;
            else {
                auto egf = ExternalGeometryFacade::getFacade(geo);
                const auto &ref = egf->getRef();
                if (boost::starts_with(ref, myName)) {
                    int geoId;
                    PointPos posId;
                    if (this->geoIdFromShapeType(ref.c_str()+myName.size(), geoId, posId)) {
                        extMap[-i-1] = geoId;
                        continue;
                    }
                }
            }
            auto copy = geo->copy();
            auto egf = ExternalGeometryFacade::getFacade(copy);
            egf->setId(++geoLastId);
            if (!egf->getRef().empty()) {
                auto &refs = this->externalGeoRefMap[egf->getRef()];
                refs.push_back(geoLastId);
            }
            this->externalGeoMap[geoLastId] = (int)geos.size();
            geos.push_back(copy);
            extMap[-i-1] = -(int)geos.size();
        }
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard(App::Property::User3, &this->ExternalGeo);
        this->ExternalGeo.setValues(std::move(geos));
    }

    if (psObj->ExternalGeometry.getSize() > 0) {
        std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
        std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

        const std::vector<DocumentObject*> originalObjects = Objects;
        const std::vector<std::string> originalSubElements = SubElements;

        std::vector<DocumentObject*> sObjects = psObj->ExternalGeometry.getValues();
        std::vector<std::string> sSubElements = psObj->ExternalGeometry.getSubValues();

        if (Objects.size() != SubElements.size() || sObjects.size() != sSubElements.size()) {
            assert(0 /*counts of objects and subelements in external geometry links do not match*/);
            Base::Console().error("Internal error: counts of objects and subelements in external "
                                  "geometry links do not match\n");
            return -1;
        }

        int si = 0;
        for (auto& sobj : sObjects) {
            int i = 0;
            for (auto& obj : Objects) {
                if (obj == sobj && SubElements[i] == sSubElements[si]) {
                    Base::Console().error(
                        "Link to %s already exists in this sketch. Delete the link and try again\n",
                        sSubElements[si].c_str());
                    return -1;
                }

                i++;
            }

            Objects.push_back(sobj);
            SubElements.push_back(sSubElements[si]);

            si++;
        }

        ExternalGeometry.setValues(Objects, SubElements);

        try {
            rebuildExternalGeometry();
        }
        catch (const Base::Exception& e) {
            Base::Console().error("%s\n", e.what());
            // revert to original values
            ExternalGeometry.setValues(originalObjects, originalSubElements);
            return -1;
        }

        solverNeedsUpdate = true;
    }

    for (std::vector<Part::Geometry*>::const_iterator it = svals.begin(); it != svals.end(); ++it) {
        Part::Geometry* geoNew = (*it)->copy();
        generateId(geoNew);
        if (construction && !geoNew->is<Part::GeomPoint>()) {
            GeometryFacade::setConstruction(geoNew, true);
        }
        newVals.push_back(geoNew);
    }

    for (std::vector<Sketcher::Constraint*>::const_iterator it = scvals.begin(); it != scvals.end();
         ++it) {
        Sketcher::Constraint* newConstr = (*it)->copy();
        if ((*it)->First >= 0)
            newConstr->First += nextgeoid;
        if ((*it)->Second >= 0)
            newConstr->Second += nextgeoid;
        if ((*it)->Third >= 0)
            newConstr->Third += nextgeoid;

        if ((*it)->First < -2 && (*it)->First != GeoEnum::GeoUndef)
            newConstr->First -= (nextextgeoid - 2);
        if ((*it)->Second < -2 && (*it)->Second != GeoEnum::GeoUndef)
            newConstr->Second -= (nextextgeoid - 2);
        if ((*it)->Third < -2 && (*it)->Third != GeoEnum::GeoUndef)
            newConstr->Third -= (nextextgeoid - 2);

        newcVals.push_back(newConstr);
    }

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(std::move(newVals));
        this->Constraints.setValues(std::move(newcVals));
    }
    // we trigger now the update (before dealing with expressions)
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    int sourceid = 0;
    for (std::vector<Sketcher::Constraint*>::const_iterator it = scvals.begin(); it != scvals.end();
         ++it, nextcid++, sourceid++) {

        if ((*it)->isDimensional()) {
            // then we link its value to the parent
            if ((*it)->isDriving) {
                App::ObjectIdentifier spath;
                std::shared_ptr<App::Expression> expr;
                std::string scname = (*it)->Name;
                if (App::ExpressionParser::isTokenAnIndentifier(scname)) {
                    spath = App::ObjectIdentifier(psObj->Constraints)
                        << App::ObjectIdentifier::SimpleComponent(scname);
                    expr = std::shared_ptr<App::Expression>(App::Expression::parse(
                        this, spath.getDocumentObjectName().getString() + spath.toString()));
                }
                else {
                    spath = psObj->Constraints.createPath(sourceid);
                    expr = std::shared_ptr<App::Expression>(
                        App::Expression::parse(this,
                                               spath.getDocumentObjectName().getString()
                                                   + std::string(1, '.') + spath.toString()));
                }
                setExpression(Constraints.createPath(nextcid), std::move(expr));
            }
        }
    }

    // Solve even if `noRecomputes==false`, because recompute may fail, and leave the
    // sketch in an inconsistent state. A concrete example. If the copied sketch
    // has broken external geometry, its recomputation will fail. And because we
    // use expression for copied constraint to add dependency to the copied
    // sketch, this sketch will not be recomputed (because its dependency fails
    // to recompute).
    solve();

    return svals.size();
}

// clang-format on
int SketchObject::addExternal(App::DocumentObject* Obj, const char* SubName, bool defining, bool intersection)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // so far only externals to the support of the sketch and datum features
    if (!isExternalAllowed(Obj->getDocument(), Obj)) {
        return -1;
    }

    auto wholeShape = Part::Feature::getTopoShape(
        Obj,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    auto shape = wholeShape.getSubTopoShape(SubName, /*silent*/ true);
    TopAbs_ShapeEnum shapeType = TopAbs_SHAPE;
    if (shape.shapeType(/*silent*/ true) != TopAbs_FACE) {
        if (shape.hasSubShape(TopAbs_FACE)) {
            shapeType = TopAbs_FACE;
        }
        else if (shape.shapeType(/*silent*/ true) != TopAbs_EDGE && shape.hasSubShape(TopAbs_EDGE)) {
            shapeType = TopAbs_EDGE;
        }
    }

    if (shapeType != TopAbs_SHAPE) {
        std::string element = Part::TopoShape::shapeName(shapeType);
        std::size_t elementNameSize = element.size();
        int geometryCount = ExternalGeometry.getSize();

        gp_Pln sketchPlane;
        if (intersection) {
            Base::Placement Plm = Placement.getValue();
            Base::Vector3d Pos = Plm.getPosition();
            Base::Rotation Rot = Plm.getRotation();
            Base::Vector3d dN(0, 0, 1);
            Rot.multVec(dN, dN);
            Base::Vector3d dX(1, 0, 0);
            Rot.multVec(dX, dX);
            gp_Ax3 sketchAx3(
                gp_Pnt(Pos.x, Pos.y, Pos.z),
                gp_Dir(dN.x, dN.y, dN.z),
                gp_Dir(dX.x, dX.y, dX.z)
            );
            sketchPlane.SetPosition(sketchAx3);
        }
        for (const auto& subShape : shape.getSubShapes(shapeType)) {
            int idx = wholeShape.findShape(subShape);
            if (idx == 0) {
                continue;
            }
            if (intersection) {
                try {
                    FCBRepAlgoAPI_Section maker(subShape, sketchPlane);
                    if (!maker.IsDone() || maker.Shape().IsNull()) {
                        continue;
                    }
                }
                catch (Standard_Failure&) {
                    continue;
                }
            }
            element += std::to_string(idx);
            addExternal(Obj, element.c_str(), defining, intersection);
            element.resize(elementNameSize);
        }
        if (ExternalGeometry.getSize() == geometryCount) {
            return -1;
        }
        return geometryCount;
    }

    // get the actual lists of the externals
    std::vector<long> Types = ExternalTypes.getValues();
    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();
    Types.resize(Objects.size(), static_cast<long>(ExtType::Projection));

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string> originalSubElements = SubElements;

    if (Objects.size() != SubElements.size()) {
        assert(0 /*counts of objects and subelements in external geometry links do not match*/);
        Base::Console().error(
            "Internal error: counts of objects and subelements in external "
            "geometry links do not match\n"
        );
        return -1;
    }

    bool add = true;
    for (size_t i = 0; i < Objects.size(); ++i) {
        if (!(Objects[i] == Obj && std::string(SubName) == SubElements[i])) {
            continue;
        }
        if (Types[i] == static_cast<int>(ExtType::Both)
            || (Types[i] == static_cast<int>(ExtType::Projection) && !intersection)
            || (Types[i] == static_cast<int>(ExtType::Intersection) && intersection)) {
            Base::Console().error("Link to %s already exists in this sketch.\n", SubName);
            return -1;
        }
        // Case where projections are already there when adding intersections.
        add = false;
        Types[i] = static_cast<int>(ExtType::Both);
    }
    if (add) {
        // add the new ones
        Objects.push_back(Obj);
        SubElements.emplace_back(SubName);
        Types.push_back(static_cast<int>(intersection ? ExtType::Intersection : ExtType::Projection));
        if (intersection) {
        }

        // set the Link list.
        ExternalGeometry.setValues(Objects, SubElements);
    }
    ExternalTypes.setValues(Types);

    try {
        ExternalToAdd ext {Obj, std::string(SubName), defining, intersection};
        rebuildExternalGeometry(ext);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects, originalSubElements);
        return -1;
    }

    acceptGeometry();  // This may need to be refactored into onChanged for ExternalGeometry

    solverNeedsUpdate = true;
    return ExternalGeometry.getValues().size() - 1;
}
// clang-format off

int SketchObject::delExternal(int ExtGeoId)
{
    return delExternal(std::vector<int>{ExtGeoId});
}

int SketchObject::delExternal(const std::vector<int>& ExtGeoIds)
{
    std::set<long> geoIds;
    for (int ExtGeoId : ExtGeoIds) {
        int GeoId = ExtGeoId >= 0 ? GeoEnum::RefExt - ExtGeoId : ExtGeoId;
        if (GeoId > GeoEnum::RefExt || -GeoId - 1 >= ExternalGeo.getSize())
            return -1;

        auto geo = getGeometry(GeoId);
        if (!geo)
            return -1;

        auto egf = ExternalGeometryFacade::getFacade(geo);
        geoIds.insert(egf->getId());
        if (egf->getRef().size()) {
            auto& refs = externalGeoRefMap[egf->getRef()];
            geoIds.insert(refs.begin(), refs.end());
        }
    }

    delExternalPrivate(geoIds, true);
    return 0;
}

void SketchObject::delExternalPrivate(const std::set<long> &ids, bool removeRef) {

    Base::StateLocker lock(managedoperation, true); // no need to check input data validity as this is an sketchobject managed operation.

    std::set<std::string> refs;
    // Must sort in reverse order so as to delete geo from back to front to
    // avoid index change
    std::set<int, std::greater<int>> geoIds;

    for(auto id : ids) {
        auto it = externalGeoMap.find(id);
        if(it == externalGeoMap.end())
            continue;

        auto egf = ExternalGeometryFacade::getFacade(ExternalGeo[it->second]);
        if(removeRef && egf->getRef().size())
            refs.insert(egf->getRef());
        geoIds.insert(-it->second-1);
    }

    if(geoIds.empty())
        return;

    std::vector< Constraint * > newConstraints;
    for(auto cstr : Constraints.getValues()) {
        if(!geoIds.count(cstr->First) &&
           (cstr->Second==GeoEnum::GeoUndef || !geoIds.count(cstr->Second)) &&
           (cstr->Third==GeoEnum::GeoUndef || !geoIds.count(cstr->Third)))
        {
            bool cloned = false;
            int offset = 0;
            for(auto GeoId : geoIds) {
                GeoId += offset++;
                bool done = true;
                if (cstr->First < GeoId && cstr->First != GeoEnum::GeoUndef) {
                    if (!cloned) {
                        cloned = true;
                        cstr = cstr->clone();
                    }
                    cstr->First += 1;
                    done = false;
                }
                if (cstr->Second < GeoId && cstr->Second != GeoEnum::GeoUndef) {
                    if (!cloned) {
                        cloned = true;
                        cstr = cstr->clone();
                    }
                    cstr->Second += 1;
                    done = false;
                }
                if (cstr->Third < GeoId && cstr->Third != GeoEnum::GeoUndef) {
                    if (!cloned) {
                        cloned = true;
                        cstr = cstr->clone();
                    }
                    cstr->Third += 1;
                    done = false;
                }
                if(done) break;
            }
            newConstraints.push_back(cstr);
        }
    }

    auto geos = ExternalGeo.getValues();
    int offset = 0;
    for(auto geoId : geoIds) {
        int idx = -geoId-1;
        geos.erase(geos.begin()+idx-offset);
        ++offset;
    }

    if(refs.size()) {
        std::vector<std::string> newSubs;
        std::vector<App::DocumentObject*> newObjs;
        const auto &subs = ExternalGeometry.getSubValues();
        auto itSub = subs.begin();
        const auto &objs = ExternalGeometry.getValues();
        auto itObj = objs.begin();
        bool touched = false;
        assert(externalGeoRef.size() == objs.size());
        assert(externalGeoRef.size() == subs.size());
        for(auto it=externalGeoRef.begin();it!=externalGeoRef.end();++it,++itObj,++itSub) {
            if(refs.count(*it)) {
                if(!touched) {
                    touched = true;
                    if(newObjs.empty()) {
                        newObjs.insert(newObjs.end(),objs.begin(),itObj);
                        newSubs.insert(newSubs.end(),subs.begin(),itSub);
                    }
                }
            }else if(touched) {
                newObjs.push_back(*itObj);
                newSubs.push_back(*itSub);
            }
        }
        if(touched)
            ExternalGeometry.setValues(newObjs,newSubs);
    }

    ExternalGeo.setValues(std::move(geos));

    solverNeedsUpdate = true;
    Constraints.setValues(std::move(newConstraints));
    acceptGeometry(); // This may need to be refactored into OnChanged for ExternalGeometry.
}

// clang-format on
int SketchObject::delAllExternal()
{
    int count = 0;                      // the remaining count of the detached external geometry
    std::map<int, int> indexMap;        // the index map of the remain external geometry
    std::vector<Part::Geometry*> geos;  // the remaining external geometry
    for (int i = 0; i < ExternalGeo.getSize(); ++i) {
        auto geo = ExternalGeo[i];
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if (egf->getRef().empty()) {
            indexMap[i] = count++;
        }
        geos.push_back(geo);
    }
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

    const std::vector<DocumentObject*> originalObjects = Objects;
    const std::vector<std::string> originalSubElements = SubElements;

    Objects.clear();
    SubElements.clear();

    const std::vector<Constraint*>& constraints = Constraints.getValues();
    std::vector<Constraint*> newConstraints(0);

    for (const auto& constr : constraints) {
        if (constr->First > GeoEnum::RefExt
            && (constr->Second > GeoEnum::RefExt || constr->Second == GeoEnum::GeoUndef)
            && (constr->Third > GeoEnum::RefExt || constr->Third == GeoEnum::GeoUndef)) {
            Constraint* copiedConstr = constr->clone();

            newConstraints.push_back(copiedConstr);
        }
    }

    ExternalGeometry.setValues(Objects, SubElements);
    try {
        rebuildExternalGeometry();
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        // revert to original values
        ExternalGeometry.setValues(originalObjects, originalSubElements);
        for (Constraint* it : newConstraints) {
            delete it;
        }
        return -1;
    }

    ExternalGeometry.setValue(0);
    ExternalGeo.setValues(std::move(geos));
    solverNeedsUpdate = true;
    Constraints.setValues(std::move(newConstraints));
    acceptGeometry();  // This may need to be refactored into OnChanged for ExternalGeometry
    return 0;
}
// clang-format off

int SketchObject::delConstraintsToExternal(DeleteOptions options)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& constraints = Constraints.getValuesForce();
    std::vector<Constraint*> newConstraints(0);
    int GeoId = GeoEnum::RefExt, NullId = GeoEnum::GeoUndef;
    for (std::vector<Constraint*>::const_iterator it = constraints.begin(); it != constraints.end();
         ++it) {
        if ((*it)->First > GeoId && ((*it)->Second > GeoId || (*it)->Second == NullId)
            && ((*it)->Third > GeoId || (*it)->Third == NullId)) {
            newConstraints.push_back(*it);
        }
    }

    Constraints.setValues(std::move(newConstraints));
    Constraints.acceptGeometry(getCompleteGeometry());

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoFlag)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::attachExternal(
        const std::vector<int> &geoIds, App::DocumentObject *Obj, const char* SubName)
{
    if (!isExternalAllowed(Obj->getDocument(), Obj))
       return -1;

    std::set<std::string> detached;
    std::set<int> idSet;
    for (int geoId : geoIds) {
        if (geoId > GeoEnum::RefExt || -geoId - 1 >= ExternalGeo.getSize())
            continue;
        auto geo = getGeometry(geoId);
        if(!geo)
            continue;
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if(egf->getRef().size())
            detached.insert(egf->getRef());
        for(int id : getRelatedGeometry(geoId))
            idSet.insert(id);
    }

    auto geos = ExternalGeo.getValues();

    std::vector<DocumentObject*> Objects     = ExternalGeometry.getValues();
    auto itObj = Objects.begin();
    std::vector<std::string>     SubElements = ExternalGeometry.getSubValues();
    auto itSub = SubElements.begin();

    assert(Objects.size()==SubElements.size());
    assert(externalGeoRef.size() == Objects.size());

    for(auto &key : externalGeoRef) {
        if (*itObj == Obj  &&  *itSub == SubName){
            FC_ERR("Duplicate external element reference in " << getFullName() << ": " << key);
            return -1;
        }
        // detach old reference
        if(detached.count(key)) {
            itObj = Objects.erase(itObj);
            itSub = SubElements.erase(itSub);
        }else{
            ++itObj;
            ++itSub;
        }
    }

    // add the new ones
    Objects.push_back(Obj);
    SubElements.push_back(std::string(SubName));

    ExternalGeometry.setValues(Objects,SubElements);
    if(externalGeoRef.size()!=Objects.size())
        return -1;

    std::string ref = externalGeoRef.back();
    for(auto geoId : idSet) {
        auto &geo = geos[-geoId-1];
        geo = geo->clone();
        ExternalGeometryFacade::getFacade(geo)->setRef(ref);
    }

    ExternalGeo.setValues(std::move(geos));
    rebuildExternalGeometry();
    return ExternalGeometry.getSize()-1;
}

std::vector<int> SketchObject::getRelatedGeometry(int GeoId) const {
    std::vector<int> res;
    if(GeoId>GeoEnum::RefExt || -GeoId-1>=ExternalGeo.getSize())
        return res;
    auto geo = getGeometry(GeoId);
    if(!geo)
        return res;
    const std::string &ref = ExternalGeometryFacade::getFacade(geo)->getRef();
    if(!ref.size())
       return {GeoId};
    auto iter = externalGeoRefMap.find(ref);
    if(iter == externalGeoRefMap.end())
        return {GeoId};
    for(auto id : iter->second) {
        auto it = externalGeoMap.find(id);
        if(it!=externalGeoMap.end())
            res.push_back(-it->second-1);
    }
    return res;
}

int SketchObject::syncGeometry(const std::vector<int> &geoIds) {
    bool touched = false;
    auto geos = ExternalGeo.getValues();
    std::set<int> idSet;
    for(int geoId : geoIds) {
        auto geo = getGeometry(geoId);
        if(!geo || !ExternalGeometryFacade::getFacade(geo)->testFlag(ExternalGeometryExtension::Frozen))
            continue;
        for(int gid : getRelatedGeometry(geoId))
            idSet.insert(gid);
    }
    for(int geoId : idSet) {
        if(geoId <= GeoEnum::RefExt && -geoId-1 < ExternalGeo.getSize()) {
            auto &geo = geos[-geoId-1];
            geo = geo->clone();
            ExternalGeometryFacade::getFacade(geo)->setFlag(ExternalGeometryExtension::Sync);
            touched = true;
        }
    }
    if(touched)
        ExternalGeo.setValues(std::move(geos));
    return 0;
}

namespace {

// Auxiliary Method: returns vector projection in UV space of plane
static gp_Vec2d ProjVecOnPlane_UV(const gp_Vec& V, const gp_Pln& Pl)
{
    return gp_Vec2d(V.Dot(Pl.Position().XDirection()), V.Dot(Pl.Position().YDirection()));
}

// Auxiliary Method: returns vector projection in UVN space of plane
static gp_Vec ProjVecOnPlane_UVN(const gp_Vec& V, const gp_Pln& Pl)
{
    gp_Vec2d vector = ProjVecOnPlane_UV(V, Pl);
    return gp_Vec(vector.X(), vector.Y(), 0.0);
}


// Auxiliary Method: returns point projection in UV space of plane
static gp_Vec2d ProjPointOnPlane_UV(const gp_Pnt& P, const gp_Pln& Pl)
{
    gp_Vec OP = gp_Vec(Pl.Location(), P);
    return ProjVecOnPlane_UV(OP, Pl);
}

// Auxiliary Method: returns point projection in UVN space of plane
static gp_Vec ProjPointOnPlane_UVN(const gp_Pnt& P, const gp_Pln& Pl)
{
    gp_Vec2d vec2 = ProjPointOnPlane_UV(P, Pl);
    return gp_Vec(vec2.X(), vec2.Y(), 0.0);
}

// Auxiliary Method: returns point projection in XYZ space
static gp_Pnt ProjPointOnPlane_XYZ(const gp_Pnt& P, const gp_Pln& Pl)
{
    gp_Vec positionUVN = ProjPointOnPlane_UVN(P, Pl);
    return gp_Pnt((positionUVN.X() * Pl.Position().XDirection()
                   + positionUVN.Y() * Pl.Position().YDirection() + gp_Vec(Pl.Location().XYZ()))
                      .XYZ());
}

// Auxiliary method
Part::Geometry* projectLine(const BRepAdaptor_Curve& curve, const Handle(Geom_Plane) & gPlane,
                            const Base::Placement& invPlm)
{
    double first = curve.FirstParameter();

    if (fabs(first) > 1E99) {
        // TODO: What is OCE's definition of Infinite?
        // TODO: The clean way to do this is to handle a new sketch geometry Geom::Line
        // but its a lot of work to implement...
        first = -10000;
    }

    double last = curve.LastParameter();
    if (fabs(last) > 1E99) {
        last = +10000;
    }

    gp_Pnt P1 = curve.Value(first);
    gp_Pnt P2 = curve.Value(last);

    GeomAPI_ProjectPointOnSurf proj1(P1, gPlane);
    P1 = proj1.NearestPoint();
    GeomAPI_ProjectPointOnSurf proj2(P2, gPlane);
    P2 = proj2.NearestPoint();

    Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());
    Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());
    invPlm.multVec(p1, p1);
    invPlm.multVec(p2, p2);

    if (Base::Distance(p1, p2) < Precision::Confusion()) {
        Base::Vector3d p = (p1 + p2) / 2;
        Part::GeomPoint* point = new Part::GeomPoint(p);
        GeometryFacade::setConstruction(point, true);
        return point;
    }
    else {
        Part::GeomLineSegment* line = new Part::GeomLineSegment();
        line->setPoints(p1, p2);
        GeometryFacade::setConstruction(line, true);
        return line;
    }
}

}  // anonymous namespace

static Part::Geometry *fitArcs(std::vector<std::unique_ptr<Part::Geometry> > &arcs,
                               const gp_Pnt &P1,
                               const gp_Pnt &P2,
                               double tol)
{
    double radius = 0.0;
    double m = 0.0;
    Base::Vector3d center;
    for (auto &geo : arcs) {
        if (auto arc = freecad_cast<Part::GeomArcOfCircle*>(geo.get())) {
            if (radius == 0.0) {
                radius = arc->getRadius();
                center = arc->getCenter();
                double f = arc->getFirstParameter();
                double l = arc->getLastParameter();
                m = (l-f)*0.5 + f; // middle parameter
            } else if (std::abs(radius - arc->getRadius()) > tol)
                return nullptr;
        } else
            return nullptr;
    }
    if (radius == 0.0) {
        return nullptr;
    }
    if (P1.SquareDistance(P2) < Precision::Confusion()) {
        Part::GeomCircle* circle = new Part::GeomCircle();
        circle->setCenter(center);
        circle->setRadius(radius);
        return circle;
    }
    if (arcs.size() == 1) {
        auto res = arcs.front().release();
        arcs.clear();
        return res;
    }

    GeomLProp_CLProps prop(Handle(Geom_Curve)::DownCast(arcs.front()->handle()),m,0,Precision::Confusion());
    gp_Pnt midPoint = prop.Value();
    GC_MakeArcOfCircle arc(P1, midPoint, P2);
    auto geo = new Part::GeomArcOfCircle();
    geo->setHandle(arc.Value());
    return geo;
}

void SketchObject::validateExternalLinks()
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<DocumentObject*> Objects = ExternalGeometry.getValues();
    std::vector<std::string> SubElements = ExternalGeometry.getSubValues();

    bool rebuild = false;

    for (int i = 0; i < int(Objects.size()); i++) {
        const App::DocumentObject* Obj = Objects[i];
        const std::string SubElement = SubElements[i];

        TopoDS_Shape refSubShape;
        bool removeBadLink = false;
        try {
            if (Obj->isDerivedFrom<Part::Datum>()) {
                const Part::Datum* datum = static_cast<const Part::Datum*>(Obj);
                refSubShape = datum->getShape();
            }
            else if (Obj->isDerivedFrom<App::DatumElement>()) {
                // do nothing - shape will be calculated later during rebuild
            }
            else {
                const Part::Feature* refObj = static_cast<const Part::Feature*>(Obj);
                const Part::TopoShape& refShape = refObj->Shape.getShape();
                refSubShape = refShape.getSubShape(SubElement.c_str());
            }
        }
        catch (Base::IndexError& indexError) {
            removeBadLink = true;
            Base::Console().warning(
                this->getFullLabel(), (indexError.getMessage() + "\n").c_str());
        }
        catch (Base::ValueError& valueError) {
            removeBadLink = true;
            Base::Console().warning(
                this->getFullLabel(), (valueError.getMessage() + "\n").c_str());
        }
        catch (Standard_Failure&) {
            removeBadLink = true;
        }
        if (removeBadLink) {
            rebuild = true;
            Objects.erase(Objects.begin() + i);
            SubElements.erase(SubElements.begin() + i);

            const std::vector<Constraint*>& constraints = Constraints.getValues();
            std::vector<Constraint*> newConstraints(0);
            int GeoId = GeoEnum::RefExt - i;
            for (const auto& constr : constraints) {
                auto newConstr = getConstraintAfterDeletingGeo(constr, GeoId);
                if (newConstr) {
                    newConstraints.push_back(newConstr.release());
                }
            }

            Constraints.setValues(std::move(newConstraints));
            i--;// we deleted an item, so the next one took its place
        }
    }

    if (rebuild) {
        ExternalGeometry.setValues(Objects, SubElements);
        rebuildExternalGeometry();
        acceptGeometry();// This may need to be refactor to OnChanged for ExternalGeo
        solve(true);     // we have to update this sketch and everything depending on it.
    }
}

namespace {

void adjustParameterRange(const TopoDS_Edge &edge,
                                 Handle(Geom_Plane) gPlane,
                                 const gp_Trsf &mov,
                                 Handle(Geom_Curve) curve,
                                 double &firstParameter,
                                 double &lastParameter)
{
    // This function is to deal with the ambiguity of trimming a periodic
    // curve, e.g. given two points on a circle, whether to get the upper or
    // lower arc. Because projection orientation may swap the first and last
    // parameter of the original curve.
    //
    // We project the middle point of the original curve to the projected curve
    // to decide whether to flip the parameters.

    Handle(Geom_Curve) origCurve = BRepAdaptor_Curve(edge).Curve().Curve();

    // GeomAPI_ProjectPointOnCurve will project a point to an untransformed
    // curve, so make sure to obtain the point on an untransformed edge.
    auto e = edge.Located(TopLoc_Location());

    gp_Pnt firstPoint = BRep_Tool::Pnt(TopExp::FirstVertex(TopoDS::Edge(e)));
    double f = GeomAPI_ProjectPointOnCurve(firstPoint, origCurve).LowerDistanceParameter();

    gp_Pnt lastPoint = BRep_Tool::Pnt(TopExp::LastVertex(TopoDS::Edge(e)));
    double l = GeomAPI_ProjectPointOnCurve(lastPoint, origCurve).LowerDistanceParameter();

    auto adjustPeriodic = [](Handle(Geom_Curve) curve, double &f, double &l) {
        // Copied from Geom_TrimmedCurve::setTrim()
        if (curve->IsPeriodic()) {
            Standard_Real Udeb = curve->FirstParameter();
            Standard_Real Ufin = curve->LastParameter();
            // set f in the range Udeb , Ufin
            // set l in the range f , f + Period()
            ElCLib::AdjustPeriodic(Udeb, Ufin,
                    std::min(std::abs(f-l)/2,Precision::PConfusion()),
                    f, l);
        }
    };

    // Adjust for periodic curve to deal with orientation
    adjustPeriodic(origCurve, f, l);

    // Obtain the middle parameter in order to get the mid point of the arc
    double m = (l - f) * 0.5 + f;
    GeomLProp_CLProps prop(origCurve,m,0,Precision::Confusion());
    gp_Pnt midPoint = prop.Value();

    // Transform all three points to the world coordinate
    auto trsf = edge.Location().Transformation();
    midPoint.Transform(trsf);
    firstPoint.Transform(trsf);
    lastPoint.Transform(trsf);

    // Project the points to the sketch plane. Note the coordinates are still
    // in world coordinate system.
    gp_Pnt pm = GeomAPI_ProjectPointOnSurf(midPoint, gPlane).NearestPoint();
    gp_Pnt pf = GeomAPI_ProjectPointOnSurf(firstPoint, gPlane).NearestPoint();
    gp_Pnt pl = GeomAPI_ProjectPointOnSurf(lastPoint, gPlane).NearestPoint();

    // Transform the projected points to sketch plane local coordinates
    pm.Transform(mov);
    pf.Transform(mov);
    pl.Transform(mov);

    // Obtain the corresponding parameters for those points in the projected curve
    double f2 = GeomAPI_ProjectPointOnCurve(pf, curve).LowerDistanceParameter();
    double l2 = GeomAPI_ProjectPointOnCurve(pl, curve).LowerDistanceParameter();
    double m2 = GeomAPI_ProjectPointOnCurve(pm, curve).LowerDistanceParameter();

    firstParameter = f2;
    lastParameter = l2;

    adjustPeriodic(curve, f2, l2);
    adjustPeriodic(curve, f2, m2);
    // If the middle point is out of range, it means we need to choose the
    // other half of the arc.
    if (m2 > l2){
        std::swap(firstParameter, lastParameter);
    }
}

void processEdge2(TopoDS_Edge& projEdge, std::vector<std::unique_ptr<Part::Geometry>>& geos)
{
    BRepAdaptor_Curve projCurve(projEdge);
    if (projCurve.GetType() == GeomAbs_Line) {
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());
        Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());
        Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());

        if (Base::Distance(p1, p2) < Precision::Confusion()) {
            Base::Vector3d p = (p1 + p2) / 2;
            auto* point = new Part::GeomPoint(p);
            GeometryFacade::setConstruction(point, true);
            geos.emplace_back(point);
        }
        else {
            auto* line = new Part::GeomLineSegment();
            line->setPoints(p1, p2);
            GeometryFacade::setConstruction(line, true);
            geos.emplace_back(line);
        }
    }
    else if (projCurve.GetType() == GeomAbs_Circle) {
        gp_Circ c = projCurve.Circle();
        gp_Pnt p = c.Location();
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

        if (P1.SquareDistance(P2) < Precision::Confusion()) {
            auto* circle = new Part::GeomCircle();
            circle->setRadius(c.Radius());
            circle->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));

            GeometryFacade::setConstruction(circle, true);
            geos.emplace_back(circle);
        }
        else {
            auto* arc = new Part::GeomArcOfCircle();
            Handle(Geom_Curve) curve = new Geom_Circle(c);
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve,
                    projCurve.FirstParameter(),
                    projCurve.LastParameter());
            arc->setHandle(tCurve);
            GeometryFacade::setConstruction(arc, true);
            geos.emplace_back(arc);
        }
    }
    else if (projCurve.GetType() == GeomAbs_BSplineCurve) {
        // Unfortunately, a normal projection of a circle can also give
        // a Bspline Split the spline into arcs
        GeomConvert_BSplineCurveKnotSplitting bSplineSplitter(projCurve.BSpline(), 2);
        auto* bspline = new Part::GeomBSplineCurve(projCurve.BSpline());
        GeometryFacade::setConstruction(bspline, true);
        geos.emplace_back(bspline);
    }
    else if (projCurve.GetType() == GeomAbs_Hyperbola) {
        gp_Hypr e = projCurve.Hyperbola();
        gp_Pnt p = e.Location();
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

        gp_Dir normal = e.Axis().Direction();
        gp_Dir xdir = e.XAxis().Direction();
        gp_Ax2 xdirref(p, normal);

        if (P1.SquareDistance(P2) < Precision::Confusion()) {
            auto* hyperbola = new Part::GeomHyperbola();
            hyperbola->setMajorRadius(e.MajorRadius());
            hyperbola->setMinorRadius(e.MinorRadius());
            hyperbola->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));
            hyperbola->setAngleXU(-xdir.AngleWithRef(xdirref.XDirection(), normal));
            GeometryFacade::setConstruction(hyperbola, true);
            geos.emplace_back(hyperbola);
        }
        else {
            auto* aoh = new Part::GeomArcOfHyperbola();
            Handle(Geom_Curve) curve = new Geom_Hyperbola(e);
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve,
                    projCurve.FirstParameter(),
                    projCurve.LastParameter());
            aoh->setHandle(tCurve);
            GeometryFacade::setConstruction(aoh, true);
            geos.emplace_back(aoh);
        }
    }
    else if (projCurve.GetType() == GeomAbs_Parabola) {
        gp_Parab e = projCurve.Parabola();
        gp_Pnt p = e.Location();
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

        gp_Dir normal = e.Axis().Direction();
        gp_Dir xdir = e.XAxis().Direction();
        gp_Ax2 xdirref(p, normal);

        if (P1.SquareDistance(P2) < Precision::Confusion()) {
            auto* parabola = new Part::GeomParabola();
            parabola->setFocal(e.Focal());
            parabola->setCenter(Base::Vector3d(p.X(), p.Y(), p.Z()));
            parabola->setAngleXU(-xdir.AngleWithRef(xdirref.XDirection(), normal));
            GeometryFacade::setConstruction(parabola, true);
            geos.emplace_back(parabola);
        }
        else {
            auto* aop = new Part::GeomArcOfParabola();
            Handle(Geom_Curve) curve = new Geom_Parabola(e);
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve,
                    projCurve.FirstParameter(),
                    projCurve.LastParameter());
            aop->setHandle(tCurve);
            GeometryFacade::setConstruction(aop, true);
            geos.emplace_back(aop);
        }
    }
    else if (projCurve.GetType() == GeomAbs_Ellipse) {
        gp_Elips e = projCurve.Ellipse();
        gp_Pnt p = e.Location();
        gp_Pnt P1 = projCurve.Value(projCurve.FirstParameter());
        gp_Pnt P2 = projCurve.Value(projCurve.LastParameter());

        gp_Dir normal = gp_Dir(0, 0, 1);
        gp_Ax2 xdirref(p, normal);

        if (P1.SquareDistance(P2) < Precision::Confusion()) {
            auto* ellipse = new Part::GeomEllipse();
            Handle(Geom_Ellipse) curve = new Geom_Ellipse(e);
            ellipse->setHandle(curve);
            GeometryFacade::setConstruction(ellipse, true);
            geos.emplace_back(ellipse);
        }
        else {
            auto* aoe = new Part::GeomArcOfEllipse();
            Handle(Geom_Curve) curve = new Geom_Ellipse(e);
            Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(curve,
                    projCurve.FirstParameter(),
                    projCurve.LastParameter());
            aoe->setHandle(tCurve);
            GeometryFacade::setConstruction(aoe, true);
            geos.emplace_back(aoe);
        }
    }
    else {
        throw Base::NotImplementedError("Not yet supported geometry for external geometry");
    }
}

void processEdge(const TopoDS_Edge& edge,
                 std::vector<std::unique_ptr<Part::Geometry>>& geos,
                 const Handle(Geom_Plane)& gPlane,
                 const Base::Placement& invPlm,
                 const gp_Trsf& mov,
                 const gp_Pln& sketchPlane,
                 const Base::Rotation& invRot,
                 gp_Ax3& sketchAx3,
                 TopoDS_Shape& aProjFace)
{
    using std::numbers::pi;

    BRepAdaptor_Curve curve(edge);
    if (curve.GetType() == GeomAbs_Line) {
        geos.emplace_back(projectLine(curve, gPlane, invPlm));
    }
    else if (curve.GetType() == GeomAbs_Circle) {
        gp_Dir vec1 = sketchPlane.Axis().Direction();
        gp_Dir vec2 = curve.Circle().Axis().Direction();

        // start point of arc of circle
        gp_Pnt beg = curve.Value(curve.FirstParameter());
        // end point of arc of circle
        gp_Pnt end = curve.Value(curve.LastParameter());

        if (vec1.IsParallel(vec2, Precision::Confusion())) {
            gp_Circ circle = curve.Circle();
            gp_Pnt cnt = circle.Location();

            GeomAPI_ProjectPointOnSurf proj(cnt, gPlane);
            cnt = proj.NearestPoint();
            circle.SetLocation(cnt);
            cnt.Transform(mov);
            circle.Transform(mov);

            if (beg.SquareDistance(end) < Precision::Confusion()) {
                auto* gCircle = new Part::GeomCircle();
                gCircle->setRadius(circle.Radius());
                gCircle->setCenter(Base::Vector3d(cnt.X(), cnt.Y(), cnt.Z()));

                GeometryFacade::setConstruction(gCircle, true);
                geos.emplace_back(gCircle);
            }
            else {
                auto* gArc = new Part::GeomArcOfCircle();
                Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
                Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(
                    hCircle, curve.FirstParameter(), curve.LastParameter());
                gArc->setHandle(tCurve);
                GeometryFacade::setConstruction(gArc, true);
                geos.emplace_back(gArc);
            }
        }
        else {
            // creates an ellipse or a segment
            gp_Circ origCircle = curve.Circle();

            if (vec1.IsNormal(vec2, Precision::Angular())) {
                // circle's normal vector in plane:
                // projection is a line
                // define center by projection
                gp_Pnt cnt = origCircle.Location();
                GeomAPI_ProjectPointOnSurf proj(cnt, gPlane);
                cnt = proj.NearestPoint();

                gp_Dir dirOrientation = gp_Dir(vec1 ^ vec2);
                gp_Dir dirLine(dirOrientation);

                auto* projectedSegment = new Part::GeomLineSegment();
                Geom_Line ligne(cnt, dirLine);// helper object to compute end points
                gp_Pnt P1, P2;                // end points of the segment, OCC style

                ligne.D0(-origCircle.Radius(), P1);
                ligne.D0(origCircle.Radius(), P2);

                if (!curve.IsClosed()) {// arc of circle
                    double alpha =
                        dirOrientation.AngleWithRef(curve.Circle().XAxis().Direction(),
                            curve.Circle().Axis().Direction());

                    double baseAngle = curve.FirstParameter();

                    int tours = 0;
                    double startAngle = baseAngle + alpha;
                    // bring startAngle back in [-pi/2 , 3pi/2[
                    while (startAngle < -pi / 2.0 && tours < 10) {
                        startAngle = baseAngle + ++tours * 2.0 * pi + alpha;
                    }
                    while (startAngle >= 3.0 * pi / 2.0 && tours > -10) {
                        startAngle = baseAngle + --tours * 2.0 * pi + alpha;
                    }

                    // apply same offset to end angle
                    double endAngle = curve.LastParameter() + startAngle - baseAngle;

                    if (startAngle <= 0.0) {
                        if (endAngle <= 0.0) {
                            P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                        }
                        else {
                            if (endAngle <= fabs(startAngle)) {
                                // P2 = P2 already defined
                                P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            }
                            else if (endAngle < pi) {
                                // P2 = P2, already defined
                                P1 = ProjPointOnPlane_XYZ(end, sketchPlane);
                            }
                            else {
                                // P1 = P1, already defined
                                // P2 = P2, already defined
                            }
                        }
                    }
                    else if (startAngle < pi) {
                        if (endAngle < pi) {
                            P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                        }
                        else if (endAngle < 2.0 * pi - startAngle) {
                            P2 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            // P1 = P1, already defined
                        }
                        else if (endAngle < 2.0 * pi) {
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                            // P1 = P1, already defined
                        }
                        else {
                            // P1 = P1, already defined
                            // P2 = P2, already defined
                        }
                    }
                    else {
                        if (endAngle < 2 * pi) {
                            P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                        }
                        else if (endAngle < 4 * pi - startAngle) {
                            P1 = ProjPointOnPlane_XYZ(beg, sketchPlane);
                            // P2 = P2, already defined
                        }
                        else if (endAngle < 3 * pi) {
                            // P1 = P1, already defined
                            P2 = ProjPointOnPlane_XYZ(end, sketchPlane);
                        }
                        else {
                            // P1 = P1, already defined
                            // P2 = P2, already defined
                        }
                    }
                }

                Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());// ends of segment FCAD style
                Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());
                invPlm.multVec(p1, p1);
                invPlm.multVec(p2, p2);

                projectedSegment->setPoints(p1, p2);
                GeometryFacade::setConstruction(projectedSegment, true);
                geos.emplace_back(projectedSegment);
            }
            else {// general case, full circle or arc of circle
                gp_Pnt cnt = origCircle.Location();
                GeomAPI_ProjectPointOnSurf proj(cnt, gPlane);
                // projection of circle center on sketch plane, 3D space
                cnt = proj.NearestPoint();
                // converting to FCAD style vector
                Base::Vector3d p(cnt.X(), cnt.Y(), cnt.Z());
                // transforming towards sketch's (x,y) coordinates
                invPlm.multVec(p, p);


                gp_Vec vecMajorAxis = vec1 ^ vec2;// major axis in 3D space

                double minorRadius;// TODO use data type of vectors around...
                double cosTheta;
                // cos of angle between the two planes, assuming vectirs are normalized
                // to 1
                cosTheta = fabs(vec1.Dot(vec2));
                minorRadius = origCircle.Radius() * cosTheta;

                // maj axis into FCAD style vector
                Base::Vector3d vectorMajorAxis(
                    vecMajorAxis.X(), vecMajorAxis.Y(), vecMajorAxis.Z());
                // transforming to sketch's (x,y) coordinates
                invRot.multVec(vectorMajorAxis, vectorMajorAxis);
                // back to OCC
                vecMajorAxis.SetXYZ(
                    gp_XYZ(vectorMajorAxis[0], vectorMajorAxis[1], vectorMajorAxis[2]));

                // NB: force normal of ellipse to be normal of sketch's plane.
                gp_Ax2 refFrameEllipse(
                    gp_Pnt(gp_XYZ(p[0], p[1], p[2])), gp_Vec(0, 0, 1), vecMajorAxis);

                gp_Elips elipsDest;
                elipsDest.SetPosition(refFrameEllipse);
                elipsDest.SetMajorRadius(origCircle.Radius());
                elipsDest.SetMinorRadius(minorRadius);

                Handle(Geom_Ellipse) projCurve = new Geom_Ellipse(elipsDest);

                if (beg.SquareDistance(end) < Precision::Confusion()) {
                    // projection is an ellipse
                    auto* ellipse = new Part::GeomEllipse();
                    ellipse->setHandle(projCurve);
                    GeometryFacade::setConstruction(ellipse, true);
                    geos.emplace_back(ellipse);
                }
                else {
                    // projection is an arc of ellipse
                    auto* aoe = new Part::GeomArcOfEllipse();
                    double firstParam, lastParam;
                    // adjust the parameter range to get the correct arc
                    adjustParameterRange(edge, gPlane, mov, projCurve, firstParam, lastParam);

                    Handle(Geom_TrimmedCurve) trimmedCurve = new Geom_TrimmedCurve(projCurve, firstParam, lastParam);
                    aoe->setHandle(trimmedCurve);
                    GeometryFacade::setConstruction(aoe, true);
                    geos.emplace_back(aoe);
                }
            }
        }
    }
    else if (curve.GetType() == GeomAbs_Ellipse) {

        gp_Pnt P1 = curve.Value(curve.FirstParameter());
        gp_Pnt P2 = curve.Value(curve.LastParameter());
        gp_Elips elipsOrig = curve.Ellipse();
        gp_Elips elipsDest;
        gp_Pnt origCenter = elipsOrig.Location();
        gp_Pnt destCenter = ProjPointOnPlane_UVN(origCenter, sketchPlane).XYZ();

        gp_Dir origAxisMajorDir = elipsOrig.XAxis().Direction();
        gp_Vec origAxisMajor = elipsOrig.MajorRadius() * gp_Vec(origAxisMajorDir);
        gp_Dir origAxisMinorDir = elipsOrig.YAxis().Direction();
        gp_Vec origAxisMinor = elipsOrig.MinorRadius() * gp_Vec(origAxisMinorDir);

        // Here, it used to be a test for parallel direction between the sketchplane and
        // the elipsOrig, in which the original ellipse would be copied and translated
        // to the new position. The problem with that approach is that for the sketcher
        // the normal vector is always (0,0,1). If the original ellipse was not on the
        // XY plane, the copy will not be either. Then, the dimensions would be wrong
        // because of the different major axis direction (which is not projected on the
        // XY plane). So here, we default to the more general ellipse construction
        // algorithm.
        //
        // Doing that solves:
        // https://forum.freecad.org/viewtopic.php?f=3&t=55284#p477522

        // GENERAL ELLIPSE CONSTRUCTION ALGORITHM
        //
        // look for major axis of projected ellipse
        //
        // t is the parameter along the origin ellipse
        //   OM(t) = origCenter
        //           + majorRadius * cos(t) * origAxisMajorDir
        //           + minorRadius * sin(t) * origAxisMinorDir
        gp_Vec2d PA = ProjVecOnPlane_UV(origAxisMajor, sketchPlane);
        gp_Vec2d PB = ProjVecOnPlane_UV(origAxisMinor, sketchPlane);
        double t_max = 2.0 * PA.Dot(PB) / (PA.SquareMagnitude() - PB.SquareMagnitude());
        t_max = 0.5 * atan(t_max);// gives new major axis is most cases, but not all
        double t_min = t_max + 0.5 * pi;

        // ON_max = OM(t_max) gives the point, which projected on the sketch plane,
        //     becomes the apoapse of the projected ellipse.
        gp_Vec ON_max = origAxisMajor * cos(t_max) + origAxisMinor * sin(t_max);
        gp_Vec ON_min = origAxisMajor * cos(t_min) + origAxisMinor * sin(t_min);
        gp_Vec destAxisMajor = ProjVecOnPlane_UVN(ON_max, sketchPlane);
        gp_Vec destAxisMinor = ProjVecOnPlane_UVN(ON_min, sketchPlane);

        double RDest = destAxisMajor.Magnitude();
        double rDest = destAxisMinor.Magnitude();

        if (RDest < rDest) {
            double rTmp = rDest;
            rDest = RDest;
            RDest = rTmp;
            gp_Vec axisTmp = destAxisMajor;
            destAxisMajor = destAxisMinor;
            destAxisMinor = axisTmp;
        }

        double sens = sketchAx3.Direction().Dot(elipsOrig.Position().Direction());
        int flip = sens > 0.0 ? 1.0 : -1.0;
        gp_Ax2 destCurveAx2(destCenter, gp_Dir(0, 0, flip), gp_Dir(destAxisMajor));

        // projection is a circle
        if ((RDest - rDest) < (double)Precision::Confusion()) {
            Handle(Geom_Circle) projCurve = new Geom_Circle(destCurveAx2, 0.5 * (rDest + RDest));
            if (P1.SquareDistance(P2) < Precision::Confusion()) {
                auto* circle = new Part::GeomCircle();
                circle->setHandle(projCurve);
                GeometryFacade::setConstruction(circle, true);
                geos.emplace_back(circle);
            }
            else {
                auto* arc = new Part::GeomArcOfCircle();
                double firstParam, lastParam;
                adjustParameterRange(edge, gPlane, mov, projCurve, firstParam, lastParam);
                Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(projCurve, firstParam, lastParam);
                arc->setHandle(tCurve);
                GeometryFacade::setConstruction(arc, true);
                geos.emplace_back(arc);
            }
        }
        else {
            if (sketchPlane.Position().Direction().IsNormal(
                elipsOrig.Position().Direction(), Precision::Angular())) {
                gp_Vec start = gp_Vec(destCenter.XYZ()) + destAxisMajor;
                gp_Vec end = gp_Vec(destCenter.XYZ()) - destAxisMajor;

                auto* projectedSegment = new Part::GeomLineSegment();
                projectedSegment->setPoints(
                    Base::Vector3d(start.X(), start.Y(), start.Z()),
                    Base::Vector3d(end.X(), end.Y(), end.Z()));
                GeometryFacade::setConstruction(projectedSegment, true);
                geos.emplace_back(projectedSegment);
            }
            else {

                elipsDest.SetPosition(destCurveAx2);
                elipsDest.SetMajorRadius(destAxisMajor.Magnitude());
                elipsDest.SetMinorRadius(destAxisMinor.Magnitude());

                Handle(Geom_Ellipse) projCurve = new Geom_Ellipse(elipsDest);

                if (P1.SquareDistance(P2) < Precision::Confusion()) {
                    auto* ellipse = new Part::GeomEllipse();
                    ellipse->setHandle(projCurve);
                    GeometryFacade::setConstruction(ellipse, true);
                    geos.emplace_back(ellipse);
                }
                else {
                    auto* aoe = new Part::GeomArcOfEllipse();
                    double firstParam, lastParam;
                    adjustParameterRange(edge, gPlane, mov, projCurve, firstParam, lastParam);

                    Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(projCurve, firstParam, lastParam);
                    aoe->setHandle(tCurve);
                    GeometryFacade::setConstruction(aoe, true);
                    geos.emplace_back(aoe);
                }
            }
        }
    }
    else {
        gp_Pln plane;
        auto shape = Part::TopoShape(edge);
        bool planar = shape.findPlane(plane);

        // Check if the edge is planar and plane is perpendicular to the projection plane
        if (planar && plane.Axis().IsNormal(sketchPlane.Axis(), Precision::Angular())) {
            // Project an edge to a line. Only works if the edge is planar and its plane is
            // perpendicular to the projection plane. OCC has trouble handling
            // BSpline projection to a straight line. Although it does correctly projects
            // the line including extreme bounds (not always a case), it will produce a BSpline with degree
            // more than one.
            //
            // The work around here is to use an aligned bounding box of the edge to get
            // the projection of the extremum points to construct the projected line.

            // First, transform the shape to the projection plane local coordinates.
            shape.setPlacement(invPlm * shape.getPlacement());

            // Align the z axis of the edge plane to the y axis of the projection
            // plane,  so that the extreme bound will be a line in the x axis direction
            // of the projection plane.
            double angle = plane.Axis().Direction().Angle(sketchPlane.YAxis().Direction());

            gp_Trsf trsf;
            if (fabs(angle) > Precision::Angular()) {
                trsf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(0, 0, 1)), angle);
                shape.move(trsf);
            }

            // Make a copy to work around OCC circular edge transformation bug
            shape = shape.makeElementCopy();

            // Obtain the bounding box (precise version!) and move the extreme points back
            // to the original location
            auto bbox = shape.getBoundBoxOptimal();
            if (!bbox.IsValid()){
                throw Base::CADKernelError("Invalid bounding box");
            }

            gp_Pnt p1(bbox.MinX, bbox.MinY, 0);
            gp_Pnt p2(bbox.MaxX, bbox.MaxY, 0);
            if (fabs(angle) > Precision::Angular()) {
                trsf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(0, 0, 1)), -angle);
                p1.Transform(trsf);
                p2.Transform(trsf);
            }

            Base::Vector3d P1(p1.X(), p1.Y(), 0);
            Base::Vector3d P2(p2.X(), p2.Y(), 0);

            // check for degenerated case when the line is collapsed to a point
            if (p1.SquareDistance(p2) < Precision::SquareConfusion()) {
                Part::GeomPoint* point = new Part::GeomPoint((P1 + P2) / 2);
                GeometryFacade::setConstruction(point, true);
                geos.emplace_back(point);
            }
            else {
                auto* projectedSegment = new Part::GeomLineSegment();
                projectedSegment->setPoints(P1, P2);
                GeometryFacade::setConstruction(projectedSegment, true);
                geos.emplace_back(projectedSegment);
            }
        }
        else {
            try {
                Part::TopoShape projShape;
                // Projection of the edge on parallel plane to the sketch plane is edge itself
                // all we need to do is match coordinate systems
                // for some reason OCC doesn't like to project a planar B-Spline to a plane parallel to it
                if (planar && plane.Axis().Direction().IsParallel(sketchPlane.Axis().Direction(), Precision::Confusion())) {
                    TopoDS_Edge projEdge = edge;

                    // We need to trim the curve in case we are projecting a B-Spline segment
                    if(curve.GetType() == GeomAbs_BSplineCurve){
                        double Param1 = curve.FirstParameter();
                        double Param2 = curve.LastParameter();

                        if (Param1 > Param2){
                            std::swap(Param1, Param2);
                        }

                        // trim curve in case we are projecting a segment
                        auto bsplineCurve = curve.BSpline();
                        if(Param2 - Param1 > Precision::Confusion()){
                            bsplineCurve->Segment(Param1, Param2);
                            projEdge = BRepBuilderAPI_MakeEdge(bsplineCurve).Edge();
                        }
                    }

                    projShape.setShape(projEdge);

                    // We can't use gp_Pln::Distance() because we need to
                    // know which side the plane is regarding the sketch
                    const gp_Pnt& aP = sketchPlane.Location();
                    const gp_Pnt& aLoc = plane.Location ();
                    const gp_Dir& aDir = plane.Axis().Direction();
                    double d = (aDir.X() * (aP.X() - aLoc.X()) +
                            aDir.Y() * (aP.Y() - aLoc.Y()) +
                            aDir.Z() * (aP.Z() - aLoc.Z()));

                    gp_Trsf trsf;
                    trsf.SetTranslation(gp_Vec(aDir) * d);
                    projShape.transformShape(Part::TopoShape::convert(trsf), /*copy*/false);
                } else {
                    // When planes not parallel or perpendicular, or edge is not planar
                    // normal projection is working just fine
                    BRepOffsetAPI_NormalProjection mkProj(aProjFace);
                    mkProj.Add(edge);
                    mkProj.Build();

                    projShape.setShape(mkProj.Projection());
                }
                if (!projShape.isNull() && projShape.hasSubShape(TopAbs_EDGE)) {
                    for (auto &e : projShape.getSubTopoShapes(TopAbs_EDGE)) {
                        // Transform copy of the edge to the sketch plane local coordinates
                        e.transformShape(invPlm.toMatrix(), /*copy*/true, /*checkScale*/true);
                        TopoDS_Edge projEdge = TopoDS::Edge(e.getShape());
                        processEdge2(projEdge, geos);
                    }
                }
            }
            catch (Standard_Failure& e) {
                throw Base::CADKernelError(e.GetMessageString());
            }
        }
    }
}

std::vector<TopoDS_Shape> projectShape(const TopoDS_Shape& inShape, const gp_Ax3& viewAxis)
{
    std::vector<TopoDS_Shape> res;
    Handle(HLRBRep_Algo) brep_hlr;
    try {
        brep_hlr = new HLRBRep_Algo();
        brep_hlr->Add(inShape);

        gp_Trsf aTrsf;
        aTrsf.SetTransformation(viewAxis);
        HLRAlgo_Projector projector(aTrsf, false, 1);

        brep_hlr->Projector(projector);
        brep_hlr->Update();
        brep_hlr->Hide();
    }
    catch (const Standard_Failure& e) {
        Base::Console().error("GO::projectShape - OCC error - %s - while projecting shape\n",
            e.GetMessageString());
        throw Base::RuntimeError("SketchObject::projectShape - OCC error");
    }
    catch (...) {
        throw Base::RuntimeError("SketchObject::projectShape - unknown error");
    }

    try {
        HLRBRep_HLRToShape hlrToShape(brep_hlr);
        if (!hlrToShape.VCompound().IsNull()) {
            //TopAbs_COMPOUND to TopAbs_EDGE
            res.push_back(hlrToShape.VCompound());
        }

        if (!hlrToShape.Rg1LineVCompound().IsNull()) {
            res.push_back(hlrToShape.Rg1LineVCompound());
        }

        if (!hlrToShape.OutLineVCompound().IsNull()) {
            res.push_back(hlrToShape.OutLineVCompound());
        }

        if (!hlrToShape.IsoLineVCompound().IsNull()) {
            res.push_back(hlrToShape.IsoLineVCompound());
        }

        if (!hlrToShape.HCompound().IsNull()) {
            res.push_back(hlrToShape.HCompound());
        }

        if (!hlrToShape.Rg1LineHCompound().IsNull()) {
            res.push_back(hlrToShape.Rg1LineHCompound());
        }

        if (!hlrToShape.OutLineHCompound().IsNull()) {
            res.push_back(hlrToShape.OutLineHCompound());
        }

        if (!hlrToShape.IsoLineHCompound().IsNull()) {
            res.push_back(hlrToShape.IsoLineHCompound());
        }
    }
    catch (const Standard_Failure&) {
        throw Base::RuntimeError(
            "SketchObject::projectShape - OCC error occurred while extracting edges");
    }
    catch (...) {
        throw Base::RuntimeError(
            "SketchObject::projectShape - unknown error occurred while extracting edges");
    }

    return res;
}

void processFace (const Rotation& invRot,
                  const Placement& invPlm,
                  const gp_Trsf& mov,
                  const gp_Pln& sketchPlane,
                  const Handle(Geom_Plane)& gPlane,
                  gp_Ax3& sketchAx3,
                  TopoDS_Shape& aProjFace,
                  std::vector<std::unique_ptr<Part::Geometry>>& geos,
                  TopoDS_Shape& refSubShape)
{
    const TopoDS_Face& face = TopoDS::Face(refSubShape);
    BRepAdaptor_Surface surface(face);
    if (surface.GetType() == GeomAbs_Plane) {
        // Check that the plane is perpendicular to the sketch plane
        Geom_Plane plane = surface.Plane();
        gp_Dir dnormal = plane.Axis().Direction();
        gp_Dir snormal = sketchPlane.Axis().Direction();

        // Extract all edges from the face
        TopExp_Explorer edgeExp;
        for (edgeExp.Init(face, TopAbs_EDGE); edgeExp.More(); edgeExp.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(edgeExp.Current());
            // Process each edge
            processEdge(edge, geos, gPlane, invPlm, mov, sketchPlane, invRot, sketchAx3, aProjFace);
        }

        if (fabs(dnormal.Angle(snormal) - std::numbers::pi/2) < Precision::Confusion()) {
            // The face is normal to the sketch plane
            // We don't want to keep the projection of all the edges of the face.
            // We need a single line that goes from min to max of all the projections.
            bool initialized = false;
            Vector3d start, end;
            // Lambda to determine if a point should replace start or end
            auto updateExtremes = [&](const Vector3d& point) {
                if ((point - start).Length() < (point - end).Length()) {
                    // `point` is closer to `start` than `end`, check if it's further out than `start`
                    if ((point - end).Length() > (end - start).Length()) {
                        start = point;
                    }
                }
                else {
                    // `point` is closer to `end`, check if it's further out than `end`
                    if ((point - start).Length() > (end - start).Length()) {
                        end = point;
                    }
                }
            };
            for (auto& geo : geos) {
                auto* line = dynamic_cast<Part::GeomLineSegment*>(geo.get());
                if (!line) {
                    // The face being normal to the sketch, we should have
                    // only lines. This is just a fail-safe in case there's a
                    // straight bspline or something like this.
                    continue;
                }
                if (!initialized) {
                    start = line->getStartPoint();
                    end = line->getEndPoint();
                    initialized = true;
                    continue;
                }

                updateExtremes(line->getStartPoint());
                updateExtremes(line->getEndPoint());
            }
            if (initialized) {
                auto* unifiedLine = new Part::GeomLineSegment();
                unifiedLine->setPoints(start, end);
                geos.clear(); // Clear other segments
                geos.emplace_back(unifiedLine);
            }
            else {
                // In case we have not initialized, perhaps the projections were
                // only straight bsplines.
                // Then we use the old method that will give a line with 20000 length:
                // Get vector that is normal to both sketch plane normal and plane normal.
                // This is the line's direction
                gp_Dir lnormal = dnormal.Crossed(snormal);
                BRepBuilderAPI_MakeEdge builder(gp_Lin(plane.Location(), lnormal));
                builder.Build();
                if (builder.IsDone()) {
                    const TopoDS_Edge& edge = TopoDS::Edge(builder.Shape());
                    BRepAdaptor_Curve curve(edge);
                    if (curve.GetType() == GeomAbs_Line) {
                        geos.emplace_back(projectLine(curve, gPlane, invPlm));
                    }
                }
            }
        }
    }
    else {
        std::vector<TopoDS_Shape> res = projectShape(face, sketchAx3);
        for (auto& resShape : res) {
            TopExp_Explorer explorer(resShape, TopAbs_EDGE);
            while (explorer.More()) {
                TopoDS_Edge projEdge = TopoDS::Edge(explorer.Current());
                processEdge2(projEdge, geos);
                explorer.Next();
            }
        }
    }
}

}  // anonymous namespace

void SketchObject::rebuildExternalGeometry(std::optional<ExternalToAdd> extToAdd)
{
    Base::StateLocker lock(managedoperation, true); // no need to check input data validity as this is an sketchobject managed operation.

    // Analyze the state of existing external geometries to infer the desired state for new ones.
    // If any geometry from a source link is "defining", we'll treat the whole link as "defining".
    std::map<std::string, bool> linkIsDefiningMap;
    for (const auto& geo : ExternalGeo.getValues()) {
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if (!egf->getRef().empty()) {
            bool isDefining = egf->testFlag(ExternalGeometryExtension::Defining);
            if (linkIsDefiningMap.find(egf->getRef()) == linkIsDefiningMap.end()) {
                linkIsDefiningMap[egf->getRef()] = isDefining;
            }
            else {
                linkIsDefiningMap[egf->getRef()] = linkIsDefiningMap[egf->getRef()] && isDefining;
            }
        }
    }

    // get the actual lists of the externals
    auto Types       = ExternalTypes.getValues();
    auto Objects     = ExternalGeometry.getValues();
    auto SubElements = ExternalGeometry.getSubValues();
    assert(externalGeoRef.size() == Objects.size());
    auto keys = externalGeoRef;

    // re-check for any missing geometry element. The code here has a side
    // effect that the linked external geometry will continue to work even if
    // ExternalGeometry is wiped out.
    for(auto &geo : ExternalGeo.getValues()) {
        auto egf = ExternalGeometryFacade::getFacade(geo);
        if(egf->getRef().size() && egf->testFlag(ExternalGeometryExtension::Missing)) {
            const std::string &ref = egf->getRef();
            auto pos = ref.find('.');
            if(pos == std::string::npos)
                continue;
            std::string objName = ref.substr(0,pos);
            auto obj = getDocument()->getObject(objName.c_str());
            if(!obj)
                continue;
            App::ElementNamePair elementName;
            App::GeoFeature::resolveElement(obj,ref.c_str()+pos+1,elementName);
            if(elementName.oldName.size()
                    && !App::GeoFeature::hasMissingElement(elementName.oldName.c_str()))
            {
                Objects.push_back(obj);
                SubElements.push_back(elementName.oldName);
                keys.push_back(ref);
            }
        }
    }

    Base::Placement Plm = Placement.getValue();
    Base::Vector3d Pos = Plm.getPosition();
    Base::Rotation Rot = Plm.getRotation();
    Base::Rotation invRot = Rot.inverse();
    Base::Vector3d dN(0, 0, 1);
    Rot.multVec(dN, dN);
    Base::Vector3d dX(1, 0, 0);
    Rot.multVec(dX, dX);

    Base::Placement invPlm = Plm.inverse();
    Base::Matrix4D invMat = invPlm.toMatrix();
    gp_Trsf mov;
    mov.SetValues(invMat[0][0],
                  invMat[0][1],
                  invMat[0][2],
                  invMat[0][3],
                  invMat[1][0],
                  invMat[1][1],
                  invMat[1][2],
                  invMat[1][3],
                  invMat[2][0],
                  invMat[2][1],
                  invMat[2][2],
                  invMat[2][3]);

    gp_Ax3 sketchAx3(
        gp_Pnt(Pos.x, Pos.y, Pos.z), gp_Dir(dN.x, dN.y, dN.z), gp_Dir(dX.x, dX.y, dX.z));
    gp_Pln sketchPlane(sketchAx3);

    Handle(Geom_Plane) gPlane = new Geom_Plane(sketchPlane);
    BRepBuilderAPI_MakeFace mkFace(sketchPlane);
    TopoDS_Shape aProjFace = mkFace.Shape();

    Types.resize(Objects.size(), static_cast<long>(ExtType::Projection));

    std::set<std::string> refSet;
    // We use a vector here to keep the order (roughly) the same as ExternalGeometry
    std::vector<std::vector<std::unique_ptr<Part::Geometry> > > newGeos;
    newGeos.reserve(Objects.size());
    for (int i=0; i < int(Objects.size()); i++) {
        const App::DocumentObject *Obj=Objects[i];
        const std::string &SubElement=SubElements[i];
        const std::string &key = keys[i];

        bool beingCreated = false;
        if (extToAdd) {
            beingCreated = extToAdd->obj == Obj && extToAdd->subname == SubElement;
        }

        bool projection = Types[i] == (int)ExtType::Projection || Types[i] == (int)ExtType::Both;
        bool intersection = Types[i] == (int)ExtType::Intersection || Types[i] == (int)ExtType::Both;

        // Skip frozen geometries
        bool frozen = false;
        bool sync = false;
        for(auto id : externalGeoRefMap[key]) {
            auto it = externalGeoMap.find(id);
            if(it != externalGeoMap.end()) {
                auto egf = ExternalGeometryFacade::getFacade(ExternalGeo[it->second]);
                if(egf->testFlag(ExternalGeometryExtension::Frozen)) {
                    frozen = true;
                }
                if (egf->testFlag(ExternalGeometryExtension::Sync)) {
                    sync = true;
                }
            }
        }
        if(frozen && !sync) {
            refSet.insert(std::move(key));
            continue;
        }
        if (!Obj || !Obj->getNameInDocument()) {
            continue;
        }

        std::vector<std::unique_ptr<Part::Geometry> > geos;

        auto importVertex = [&](const TopoDS_Shape& refSubShape) {
            gp_Pnt P = BRep_Tool::Pnt(TopoDS::Vertex(refSubShape));
            GeomAPI_ProjectPointOnSurf proj(P, gPlane);
            P = proj.NearestPoint();
            Base::Vector3d p(P.X(), P.Y(), P.Z());
            invPlm.multVec(p, p);

            Part::GeomPoint* point = new Part::GeomPoint(p);
            GeometryFacade::setConstruction(point, true);
            geos.emplace_back(point);
        };

        try {
            TopoDS_Shape refSubShape;

            if (Obj->isDerivedFrom<Part::Datum>()) {
                auto* datum = static_cast<const Part::Datum*>(Obj);
                refSubShape = datum->getShape();
            }
            else if (Obj->isDerivedFrom<Part::Feature>()) {
                auto* refObj = static_cast<const Part::Feature*>(Obj);
                const Part::TopoShape& refShape = refObj->Shape.getShape();
                refSubShape = refShape.getSubShape(SubElement.c_str());
            }
            else if (Obj->isDerivedFrom<App::Plane>()) {
                auto* pl = static_cast<const App::Plane*>(Obj);
                Base::Placement plm = pl->Placement.getValue();
                Base::Vector3d base = plm.getPosition();
                Base::Rotation rot = plm.getRotation();
                Base::Vector3d normal(0, 0, 1);
                rot.multVec(normal, normal);
                gp_Pln plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
                BRepBuilderAPI_MakeFace fBuilder(plane);
                if (!fBuilder.IsDone())
                    throw Base::RuntimeError(
                        "Sketcher: addExternal(): Failed to build face from App::Plane");

                TopoDS_Face f = TopoDS::Face(fBuilder.Shape());
                refSubShape = f;
            }
            else if (Obj->isDerivedFrom<Part::DatumLine>()) {
                auto* line = static_cast<const Part::DatumLine*>(Obj);
                Base::Placement plm = line->Placement.getValue();
                Base::Vector3d base = plm.getPosition();
                Base::Vector3d dir = line->getDirection();
                gp_Lin l(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
                BRepBuilderAPI_MakeEdge eBuilder(l);
                if (!eBuilder.IsDone()) {
                    throw Base::RuntimeError(
                        "Sketcher: addExternal(): Failed to build edge from Part::DatumLine");
                }

                TopoDS_Edge e = TopoDS::Edge(eBuilder.Shape());
                refSubShape = e;
            }
            else if (Obj->isDerivedFrom<Part::DatumPoint>()) {
                auto* point = static_cast<const Part::DatumPoint*>(Obj);
                Base::Placement plm = point->Placement.getValue();
                Base::Vector3d base = plm.getPosition();
                gp_Pnt p(base.x, base.y, base.z);
                BRepBuilderAPI_MakeVertex eBuilder(p);
                if (!eBuilder.IsDone()) {
                    throw Base::RuntimeError(
                        "Sketcher: addExternal(): Failed to build vertex from Part::DatumPoint");
                }

                TopoDS_Vertex v = TopoDS::Vertex(eBuilder.Shape());
                refSubShape = v;
            }
            else {
                throw Base::TypeError(
                    "Datum feature type is not yet supported as external geometry for a sketch");
            }

            if (projection && !refSubShape.IsNull()) {
                switch (refSubShape.ShapeType()) {
                case TopAbs_FACE: {
                    processFace(invRot, invPlm, mov, sketchPlane, gPlane, sketchAx3, aProjFace, geos, refSubShape);
                } break;
                case TopAbs_EDGE: {
                    const TopoDS_Edge& edge = TopoDS::Edge(refSubShape);
                    processEdge(edge, geos, gPlane, invPlm, mov, sketchPlane, invRot, sketchAx3, aProjFace);
                } break;
                case TopAbs_VERTEX: {
                    importVertex(refSubShape);
                } break;
                default:
                    throw Base::TypeError("Unknown type of geometry");
                    break;
                }
                if (beingCreated && !extToAdd->intersection) {
                    // We are adding the projections, so we need to initialize those
                    for (auto& geo : geos) {
                        auto egf = ExternalGeometryFacade::getFacade(geo.get());
                        egf->setFlag(ExternalGeometryExtension::Defining, extToAdd->defining);
                    }
                }
            }
            int projSize = geos.size();

            if (intersection && !refSubShape.IsNull()) {
                FCBRepAlgoAPI_Section maker(refSubShape, sketchPlane);
                maker.Approximation(Standard_True);
                if (!maker.IsDone())
                    FC_THROWM(Base::CADKernelError, "Failed to get intersection");
                Part::TopoShape intersectionShape(maker.Shape());
                auto edges = intersectionShape.getSubTopoShapes(TopAbs_EDGE);
                for (const auto& s : edges) {
                    TopoDS_Edge edge = TopoDS::Edge(s.getShape());
                    processEdge(edge, geos, gPlane, invPlm, mov, sketchPlane, invRot, sketchAx3, aProjFace);
                }
                // Section of some face (e.g. sphere) produce more than one arcs
                // from the same circle. So we try to fit the arcs with a single
                // circle/arc.
                if (refSubShape.ShapeType() == TopAbs_FACE && geos.size() > 1) {
                    auto wires = Part::TopoShape().makeElementWires(edges);
                    if (wires.countSubShapes(TopAbs_WIRE) == 1) {
                        TopoDS_Vertex firstVertex, lastVertex;
                        BRepTools_WireExplorer exp(TopoDS::Wire(wires.getSubShape(TopAbs_WIRE, 1)));
                        firstVertex = exp.CurrentVertex();
                        while (!exp.More())
                            exp.Next();
                        lastVertex = exp.CurrentVertex();
                        gp_Pnt P1 = BRep_Tool::Pnt(firstVertex);
                        gp_Pnt P2 = BRep_Tool::Pnt(lastVertex);
                        if (auto geo = fitArcs(geos, P1, P2, ArcFitTolerance.getValue())) {
                            geos.clear();
                            geos.emplace_back(geo);
                        }
                    }
                }
                for (const auto& s : intersectionShape.getSubShapes(TopAbs_VERTEX, TopAbs_EDGE)) {
                    importVertex(s);
                }

                if (beingCreated && extToAdd->intersection) {
                    // We are adding the projections, so we need to initialize those
                    for (size_t i = projSize; i < geos.size(); ++i) {
                        auto egf = ExternalGeometryFacade::getFacade(geos[i].get());
                        egf->setFlag(ExternalGeometryExtension::Defining, extToAdd->defining);
                    }
                }
            }

        } catch (Base::Exception &e) {
            FC_ERR("Failed to project external geometry in "
                   << getFullName() << ": " << key << std::endl << e.what());
            continue;
        } catch (Standard_Failure &e) {
            FC_ERR("Failed to project external geometry in "
                   << getFullName() << ": " << key << std::endl << e.GetMessageString());
            continue;
        } catch (std::exception &e) {
            FC_ERR("Failed to project external geometry in "
                   << getFullName() << ": " << key << std::endl << e.what());
            continue;
        } catch (...) {
            FC_ERR("Failed to project external geometry in "
                   << getFullName() << ": " << key << std::endl << "Unknown exception");
            continue;
        }
        if (geos.empty()) {
            continue;
        }

        if(!refSet.emplace(key).second) {
            FC_WARN("Duplicated external reference in " << getFullName() << ": " << key);
            continue;
        }

        for (auto& geo : geos) {
            ExternalGeometryFacade::getFacade(geo.get())->setRef(key);
        }
        newGeos.push_back(std::move(geos));
    }

    // allocate unique geometry id
    for(auto &geos : newGeos) {
        auto egf = ExternalGeometryFacade::getFacade(geos.front().get());
        auto &refs = externalGeoRefMap[egf->getRef()];
        while(refs.size() < geos.size())
            refs.push_back(++geoLastId);

        // In case a projection reduces output geometries, delete them
        std::set<long> geoIds;
        geoIds.insert(refs.begin()+geos.size(),refs.end());

        // Sync id and ref of the new geometries
        int i = 0;
        for(auto &geo : geos)
            GeometryFacade::setId(geo.get(), refs[i++]);

        delExternalPrivate(geoIds,false);
    }

    auto geoms = ExternalGeo.getValues();

    // now update the geometries
    for(auto &geos : newGeos) {
        if (geos.empty()) {
            continue;
        }

        // Get the reference key for this group of geometries. All geos in this vector share the same ref.
        const std::string& key = ExternalGeometryFacade::getFacade(geos.front().get())->getRef();
        auto itKey = linkIsDefiningMap.find(key);
        bool hasLinkState = itKey != linkIsDefiningMap.end();
        bool isLinkDefining = hasLinkState ? itKey->second : false;

        for(auto &geo : geos) {
            auto it = externalGeoMap.find(GeometryFacade::getId(geo.get()));
            if(it == externalGeoMap.end()) {
                // This is a new geometries.
                // Set its defining state based on the inferred state of its parent link.
                if (hasLinkState) {
                    ExternalGeometryFacade::getFacade(geo.get())->setFlag(ExternalGeometryExtension::Defining, isLinkDefining);
                }
                geoms.push_back(geo.release());
                continue;
            }
            // This is an existing geometry. Update it while keeping the old flags
            ExternalGeometryFacade::copyFlags(geoms[it->second], geo.get());
            geoms[it->second] = geo.release();
        }
    }

    // Check for any missing references
    bool hasError = false;
    for(auto geo : geoms) {
        auto egf = ExternalGeometryFacade::getFacade(geo);
        egf->setFlag(ExternalGeometryExtension::Sync,false);
        if(egf->getRef().empty())
            continue;
        if(!refSet.count(egf->getRef())) {
            FC_ERR( "External geometry " << getFullName() << ".e" << egf->getId()
                    << " missing reference: " << egf->getRef());
            hasError = true;
            egf->setFlag(ExternalGeometryExtension::Missing,true);
        } else {
            egf->setFlag(ExternalGeometryExtension::Missing,false);
        }
    }

    ExternalGeo.setValues(std::move(geoms));
    rebuildVertexIndex();

    // clean up geometry reference
    if(refSet.size() != (size_t)ExternalGeometry.getSize()) {
        if(refSet.size() < keys.size()) {
            auto itObj = Objects.begin();
            auto itSub = SubElements.begin();
            for(auto &ref : keys) {
                if(!refSet.count(ref)) {
                    itObj = Objects.erase(itObj);
                    itSub = SubElements.erase(itSub);
                }else {
                    ++itObj;
                    ++itSub;
                }
            }
        }
        ExternalGeometry.setValues(Objects,SubElements);
    }

    solverNeedsUpdate=true;
    Constraints.acceptGeometry(getCompleteGeometry());

    if (hasError && this->isRecomputing()) {
        throw Base::RuntimeError("Missing external geometry reference");
    }
}

void SketchObject::fixExternalGeometry(const std::vector<int> &geoIds) {
    std::set<int> idSet(geoIds.begin(),geoIds.end());
    auto geos = ExternalGeo.getValues();
    auto objs = ExternalGeometry.getValues();
    auto subs = ExternalGeometry.getSubValues();
    bool touched = false;
    for(int i=2;i<(int)geos.size();++i) {
        auto &geo = geos[i];
        auto egf = ExternalGeometryFacade::getFacade(geo);
        int GeoId = -i-1;
        if(egf->getRef().empty()
                || !egf->testFlag(ExternalGeometryExtension::Missing)
                || (idSet.size() && !idSet.count(GeoId)))
            continue;
        std::string ref = egf->getRef();
        auto pos = ref.find('.');
        if(pos == std::string::npos) {
            FC_ERR("Invalid geometry reference " << ref);
            continue;
        }
        std::string objName = ref.substr(0,pos);
        auto obj = getDocument()->getObject(objName.c_str());
        if(!obj) {
            FC_ERR("Cannot find object in reference " << ref);
            continue;
        }

        auto elements = Part::Feature::getRelatedElements(obj,ref.c_str()+pos+1);
        if(!elements.size()) {
            FC_ERR("No related reference found for " << ref);
            continue;
        }

        geo = geo->clone();
        egf->setGeometry(geo);
        egf->setFlag(ExternalGeometryExtension::Missing,false);
        ref = objName + "." + Data::ComplexGeoData::elementMapPrefix();
        elements.front().name.appendToBuffer(ref);
        egf->setRef(ref);
        objs.push_back(obj);
        subs.emplace_back();
        elements.front().index.appendToStringBuffer(subs.back());
        touched = true;
    }

    if(touched) {
        ExternalGeo.setValues(geos);
        ExternalGeometry.setValues(objs,subs);
        rebuildExternalGeometry();
    }
}

void SketchObject::updateGeometryRefs() {
    const auto &objs = ExternalGeometry.getValues();
    const auto &subs = ExternalGeometry.getSubValues();
    const auto &shadows = ExternalGeometry.getShadowSubs();
    assert(subs.size() == shadows.size());
    std::vector<std::string> originalRefs;
    std::map<std::string,std::string> refMap;
    if(updateGeoRef) {
        assert(externalGeoRef.size() == objs.size());
        updateGeoRef = false;
        originalRefs = std::move(externalGeoRef);
    }
    externalGeoRef.clear();
    std::unordered_map<std::string, int> legacyMap;
    for(int i=0;i<(int)objs.size();++i) {
        auto obj = objs[i];
        const std::string& sub = shadows[i].newName.empty() ? subs[i] : shadows[i].newName;
        externalGeoRef.emplace_back(obj->getNameInDocument());
        auto &key = externalGeoRef.back();
        key += '.';

        legacyMap[key + Data::oldElementName(sub.c_str())] = i;

        if (!obj->isDerivedFrom<Part::Datum>()) {
            key += Data::newElementName(sub.c_str());
        }
        if (!originalRefs.empty() && originalRefs[i] != key) {
            refMap[originalRefs[i]] = key;
        }
    }
    bool touched = false;
    auto geos = ExternalGeo.getValues();
    if(refMap.empty()) {
        for(auto geo : geos) {
            auto egf = ExternalGeometryFacade::getFacade(geo);
            if (egf->getRefIndex() < 0) {
                if (egf->getId() < 0 && !egf->getRef().empty()) {
                    // NOLINTNEXTLINE
                    FC_ERR("External geometry reference corrupted in " << getFullName()
                            << " Please check.");
                    // This could happen if someone saved the sketch containing
                    // external geometries using some rogue releases during the
                    // migration period. As a remedy, We re-initiate the
                    // external geometry here to trigger rebuild later, with
                    // call to rebuildExternalGeometry()
                    initExternalGeo();
                    return;
                }
                auto it = legacyMap.find(egf->getRef());
                if (it != legacyMap.end() && egf->getRef() != externalGeoRef[it->second]) {
                    if(getDocument() && !getDocument()->isPerformingTransaction()) {
                        // FIXME: this is a bug. Find out when and why does this happen
                        //
                        // Amendment: maybe the original bug is because of not
                        // handling external geometry changes during undo/redo,
                        // which should be considered as normal. So warning only
                        // if not undo/redo.
                        //
                        // NOLINTNEXTLINE
                        FC_WARN("Update legacy external reference "
                                << egf->getRef() << " -> " << externalGeoRef[it->second] << " in "
                                << getFullName());
                    }
                    else {
                        // NOLINTNEXTLINE
                        FC_LOG("Update undo/redo external reference "
                               << egf->getRef() << " -> " << externalGeoRef[it->second] << " in "
                               << getFullName());
                    }
                    touched = true;
                    egf->setRef(externalGeoRef[it->second]);
                }
                continue;
            }
            if (egf->getRefIndex() < (int)externalGeoRef.size()
                && egf->getRef() != externalGeoRef[egf->getRefIndex()]) {
                touched = true;
                egf->setRef(externalGeoRef[egf->getRefIndex()]);
            }
            egf->setRefIndex(-1);
        }
    }else{
        for(auto &v : refMap) {
            auto it = externalGeoRefMap.find(v.first);
            if (it == externalGeoRefMap.end()) {
                continue;
            }
            for (long id : it->second) {
                auto iter = externalGeoMap.find(id);
                if(iter!=externalGeoMap.end()) {
                    auto &geo = geos[iter->second];
                    geo = geo->clone();
                    auto egf = ExternalGeometryFacade::getFacade(geo);
                    // NOLINTNEXTLINE
                    FC_LOG(getFullName() << " ref change on ExternalEdge" << iter->second - 1 << ' '
                                         << egf->getRef() << " -> " << v.second);
                    egf->setRef(v.second);
                    touched = true;
                }
            }
        }
    }
    if (touched) {
        ExternalGeo.setValues(std::move(geos));
    }
}

std::string SketchObject::getGeometryReference(int GeoId) const {
    auto geo = getGeometry(GeoId);
    if (!geo) {
        return {};
    }
    auto egf = ExternalGeometryFacade::getFacade(geo);
    if (egf->getRef().empty()) {
        return {};
    }

    const std::string &ref = egf->getRef();

    if (egf->testFlag(ExternalGeometryExtension::Missing)) {
        return std::string("? ") + ref;
    }

    auto pos = ref.find('.');
    if (pos == std::string::npos) {
        return ref;
    }
    std::string objName = ref.substr(0, pos);
    auto obj = getDocument()->getObject(objName.c_str());
    if (!obj) {
        return ref;
    }

    App::ElementNamePair elementName;
    App::GeoFeature::resolveElement(obj, ref.c_str() + pos + 1, elementName);
    if (!elementName.oldName.empty()) {
        return objName + "." + elementName.oldName;
    }
    return ref;
}
