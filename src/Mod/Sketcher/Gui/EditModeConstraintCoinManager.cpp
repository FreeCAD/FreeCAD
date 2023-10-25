/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
#include <QPainter>
#include <QRegularExpression>
#include <memory>

#include <Inventor/SbImage.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoInfo.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTranslation.h>
#endif  // #ifndef _PreComp_

#include <Base/Converter.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/Vector3D.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Inventor/SmSwitchboard.h>
#include <Gui/SoDatumLabel.h>
#include <Gui/Tools.h>
#include <Gui/Utilities.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/GeoList.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

#include "EditModeConstraintCoinManager.h"
#include "SoZoomTranslation.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "ViewProviderSketchCoinAttorney.h"


using namespace Gui;
using namespace SketcherGui;
using namespace Sketcher;

//**************************** EditModeConstraintCoinManager class ******************************

EditModeConstraintCoinManager::EditModeConstraintCoinManager(
    ViewProviderSketch& vp,
    DrawingParameters& drawingParams,
    GeometryLayerParameters& geometryLayerParams,
    ConstraintParameters& constraintParams,
    EditModeScenegraphNodes& editModeScenegraph,
    CoinMapping& coinMap)
    : viewProvider(vp)
    , drawingParameters(drawingParams)
    , geometryLayerParameters(geometryLayerParams)
    , constraintParameters(constraintParams)
    , editModeScenegraphNodes(editModeScenegraph)
    , coinMapping(coinMap)
{}

EditModeConstraintCoinManager::~EditModeConstraintCoinManager()
{}

void EditModeConstraintCoinManager::updateVirtualSpace()
{
    const std::vector<Sketcher::Constraint*>& constrlist =
        ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    bool isshownvirtualspace = ViewProviderSketchCoinAttorney::isShownVirtualSpace(viewProvider);

    if (constrlist.size() == vConstrType.size()) {

        editModeScenegraphNodes.constrGroup->enable.setNum(constrlist.size());

        SbBool* sws = editModeScenegraphNodes.constrGroup->enable.startEditing();

        for (size_t i = 0; i < constrlist.size(); i++) {
            sws[i] = !(constrlist[i]->isInVirtualSpace
                       != isshownvirtualspace);  // XOR of constraint mode and VP mode
        }


        editModeScenegraphNodes.constrGroup->enable.finishEditing();
    }
}

void EditModeConstraintCoinManager::processConstraints(const GeoListFacade& geolistfacade)
{
    const auto& constrlist = ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    auto zConstrH = ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
        * drawingParameters.zConstr;

    // After an undo/redo it can happen that we have an empty geometry list but a non-empty
    // constraint list In this case just ignore the constraints. (See bug #0000421)
    if (geolistfacade.geomlist.size() <= 2 && !constrlist.empty()) {
        rebuildConstraintNodes(geolistfacade);
        return;
    }

    int extGeoCount = geolistfacade.getExternalCount();
    int intGeoCount = geolistfacade.getInternalCount();

    // reset point if the constraint type has changed
Restart:
    // check if a new constraint arrived
    if (constrlist.size() != vConstrType.size()) {
        rebuildConstraintNodes(geolistfacade);
    }

    assert(int(constrlist.size()) == editModeScenegraphNodes.constrGroup->getNumChildren());
    assert(int(vConstrType.size()) == editModeScenegraphNodes.constrGroup->getNumChildren());

    // update the virtual space
    updateVirtualSpace();

    auto getNormal = [](const GeoListFacade& geolistfacade,
                        const int geoid,
                        const Base::Vector3d& pointoncurve) {
        auto geom = geolistfacade.getGeometryFromGeoId(geoid);
        auto curve = dynamic_cast<const Part::GeomCurve*>(geom);

        auto line = dynamic_cast<const Part::GeomLineSegment*>(curve);

        if (line) {
            Base::Vector3d linedir = line->getEndPoint() - line->getStartPoint();
            return Base::Vector3d(-linedir.y, linedir.x, 0);
        }
        else {
            Base::Vector3d normal;
            try {
                if (!(curve && curve->normalAt(pointoncurve, normal))) {
                    normal = Base::Vector3d(1, 0, 0);
                }
            }
            catch (const Base::CADKernelError&) {
                normal = Base::Vector3d(1, 0, 0);
            }

            return normal;
        }
    };

    // go through the constraints and update the position
    int i = 0;
    for (std::vector<Sketcher::Constraint*>::const_iterator it = constrlist.begin();
         it != constrlist.end();
         ++it, i++) {
        // check if the type has changed
        if ((*it)->Type != vConstrType[i]) {
            // clearing the type vector will force a rebuild of the visual nodes
            vConstrType.clear();
            // TODO: The 'goto' here is unsafe as it can happen that we cause an endless loop (see
            // bug #0001956).
            goto Restart;
        }
        try {  // because calculateNormalAtPoint, used in there, can throw
            // root separator for this constraint
            SoSeparator* sep =
                static_cast<SoSeparator*>(editModeScenegraphNodes.constrGroup->getChild(i));
            const Constraint* Constr = *it;

            if (Constr->First < -extGeoCount || Constr->First >= intGeoCount
                || (Constr->Second != GeoEnum::GeoUndef
                    && (Constr->Second < -extGeoCount || Constr->Second >= intGeoCount))
                || (Constr->Third != GeoEnum::GeoUndef
                    && (Constr->Third < -extGeoCount || Constr->Third >= intGeoCount))) {
                // Constraint can refer to non-existent geometry during undo/redo
                continue;
            }

            // distinguish different constraint types to build up
            switch (Constr->Type) {
                case Block:
                case Horizontal:  // write the new position of the Horizontal constraint Same as
                                  // vertical position.
                case Vertical:    // write the new position of the Vertical constraint
                {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    bool alignment = Constr->Type != Block && Constr->Second != GeoEnum::GeoUndef;

                    // get the geometry
                    const Part::Geometry* geo = geolistfacade.getGeometryFromGeoId(Constr->First);

                    if (!alignment) {
                        // Vertical & Horiz can only be a GeomLineSegment, but Blocked can be
                        // anything.
                        Base::Vector3d midpos;
                        Base::Vector3d dir;
                        Base::Vector3d norm;

                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment* lineSeg =
                                static_cast<const Part::GeomLineSegment*>(geo);

                            // calculate the half distance between the start and endpoint
                            midpos = ((lineSeg->getEndPoint() + lineSeg->getStartPoint()) / 2);

                            // Get a set of vectors perpendicular and tangential to these
                            dir = (lineSeg->getEndPoint() - lineSeg->getStartPoint()).Normalize();

                            norm = Base::Vector3d(-dir.y, dir.x, 0);
                        }
                        else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                            const Part::GeomBSplineCurve* bsp =
                                static_cast<const Part::GeomBSplineCurve*>(geo);
                            midpos = Base::Vector3d(0, 0, 0);

                            std::vector<Base::Vector3d> poles = bsp->getPoles();

                            // Move center of gravity towards start not to collide with bspline
                            // degree information.
                            double ws = 1.0 / poles.size();
                            double w = 1.0;

                            for (std::vector<Base::Vector3d>::iterator it = poles.begin();
                                 it != poles.end();
                                 ++it) {
                                midpos += w * (*it);
                                w -= ws;
                            }

                            midpos /= poles.size();

                            dir = (bsp->getEndPoint() - bsp->getStartPoint()).Normalize();
                            norm = Base::Vector3d(-dir.y, dir.x, 0);
                        }
                        else {
                            double ra = 0, rb = 0;
                            double angle,
                                angleplus = 0.;  // angle = rotation of object as a whole; angleplus
                                                 // = arc angle (t parameter for ellipses).
                            if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle* circle =
                                    static_cast<const Part::GeomCircle*>(geo);
                                ra = circle->getRadius();
                                angle = M_PI / 4;
                                midpos = circle->getCenter();
                            }
                            else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle* arc =
                                    static_cast<const Part::GeomArcOfCircle*>(geo);
                                ra = arc->getRadius();
                                double startangle, endangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                angle = (startangle + endangle) / 2;
                                midpos = arc->getCenter();
                            }
                            else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                const Part::GeomEllipse* ellipse =
                                    static_cast<const Part::GeomEllipse*>(geo);
                                ra = ellipse->getMajorRadius();
                                rb = ellipse->getMinorRadius();
                                Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                angle = atan2(majdir.y, majdir.x);
                                angleplus = M_PI / 4;
                                midpos = ellipse->getCenter();
                            }
                            else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                const Part::GeomArcOfEllipse* aoe =
                                    static_cast<const Part::GeomArcOfEllipse*>(geo);
                                ra = aoe->getMajorRadius();
                                rb = aoe->getMinorRadius();
                                double startangle, endangle;
                                aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = aoe->getMajorAxisDir();
                                angle = atan2(majdir.y, majdir.x);
                                angleplus = (startangle + endangle) / 2;
                                midpos = aoe->getCenter();
                            }
                            else if (geo->getTypeId()
                                     == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                const Part::GeomArcOfHyperbola* aoh =
                                    static_cast<const Part::GeomArcOfHyperbola*>(geo);
                                ra = aoh->getMajorRadius();
                                rb = aoh->getMinorRadius();
                                double startangle, endangle;
                                aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = aoh->getMajorAxisDir();
                                angle = atan2(majdir.y, majdir.x);
                                angleplus = (startangle + endangle) / 2;
                                midpos = aoh->getCenter();
                            }
                            else if (geo->getTypeId()
                                     == Part::GeomArcOfParabola::getClassTypeId()) {
                                const Part::GeomArcOfParabola* aop =
                                    static_cast<const Part::GeomArcOfParabola*>(geo);
                                ra = aop->getFocal();
                                double startangle, endangle;
                                aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = -aop->getXAxisDir();
                                angle = atan2(majdir.y, majdir.x);
                                angleplus = (startangle + endangle) / 2;
                                midpos = aop->getFocus();
                            }
                            else {
                                break;
                            }

                            if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()
                                || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                                || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {

                                Base::Vector3d majDir, minDir, rvec;
                                majDir = Base::Vector3d(cos(angle),
                                                        sin(angle),
                                                        0);  // direction of major axis of ellipse
                                minDir = Base::Vector3d(-majDir.y,
                                                        majDir.x,
                                                        0);  // direction of minor axis of ellipse
                                rvec =
                                    (ra * cos(angleplus)) * majDir + (rb * sin(angleplus)) * minDir;
                                midpos += rvec;
                                rvec.Normalize();
                                norm = rvec;
                                dir = Base::Vector3d(
                                    -rvec.y,
                                    rvec.x,
                                    0);  // DeepSOIC: I'm not sure what dir is supposed to mean.
                            }
                            else {
                                norm = Base::Vector3d(cos(angle), sin(angle), 0);
                                dir = Base::Vector3d(-norm.y, norm.x, 0);
                                midpos += ra * norm;
                            }
                        }

                        Base::Vector3d relpos = seekConstraintPosition(
                            midpos,
                            norm,
                            dir,
                            2.5,
                            editModeScenegraphNodes.constrGroup->getChild(i));

                        auto translation = static_cast<SoZoomTranslation*>(sep->getChild(
                            static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                        translation->abPos =
                            SbVec3f(midpos.x, midpos.y, zConstrH);  // Absolute Reference

                        // Reference Position that is scaled according to zoom
                        translation->translation = SbVec3f(relpos.x, relpos.y, 0);
                    }
                    else {
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                        assert(Constr->FirstPos != Sketcher::PointPos::none
                               && Constr->SecondPos != Sketcher::PointPos::none);

                        Base::Vector3d midpos1, dir1, norm1;
                        Base::Vector3d midpos2, dir2, norm2;

                        midpos1 = geolistfacade.getPoint(Constr->First, Constr->FirstPos);
                        midpos2 = geolistfacade.getPoint(Constr->Second, Constr->SecondPos);

                        dir1 = (midpos2 - midpos1).Normalize();
                        dir2 = -dir1;
                        norm1 = Base::Vector3d(-dir1.y, dir1.x, 0.);
                        norm2 = norm1;

                        Base::Vector3d relpos1 = seekConstraintPosition(
                            midpos1,
                            norm1,
                            dir1,
                            4.0,
                            editModeScenegraphNodes.constrGroup->getChild(i));

                        auto translation = static_cast<SoZoomTranslation*>(sep->getChild(
                            static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                        translation->abPos = SbVec3f(midpos1.x, midpos1.y, zConstrH);
                        translation->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                        Base::Vector3d relpos2 = seekConstraintPosition(
                            midpos2,
                            norm2,
                            dir2,
                            4.0,
                            editModeScenegraphNodes.constrGroup->getChild(i));

                        Base::Vector3d secondPos = midpos2 - midpos1;

                        translation = static_cast<SoZoomTranslation*>(sep->getChild(
                            static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));

                        translation->abPos = SbVec3f(secondPos.x, secondPos.y, zConstrH);
                        translation->translation =
                            SbVec3f(relpos2.x - relpos1.x, relpos2.y - relpos1.y, 0);
                    }
                } break;
                case Perpendicular: {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                    // get the geometry
                    const Part::Geometry* geo1 = geolistfacade.getGeometryFromGeoId(Constr->First);
                    const Part::Geometry* geo2 = geolistfacade.getGeometryFromGeoId(Constr->Second);
                    Base::Vector3d midpos1, dir1, norm1;
                    Base::Vector3d midpos2, dir2, norm2;
                    bool twoIcons = false;  // a very local flag. It's set to true to indicate that
                                            // the second dir+norm are valid and should be used

                    if (Constr->Third != GeoEnum::GeoUndef ||  // perpty via point
                        Constr->FirstPos
                            != Sketcher::PointPos::none) {  // endpoint-to-curve or
                                                            // endpoint-to-endpoint perpty

                        int ptGeoId;
                        Sketcher::PointPos ptPosId;
                        do {  // dummy loop to use break =) Maybe goto?
                            ptGeoId = Constr->First;
                            ptPosId = Constr->FirstPos;
                            if (ptPosId != Sketcher::PointPos::none) {
                                break;
                            }
                            ptGeoId = Constr->Second;
                            ptPosId = Constr->SecondPos;
                            if (ptPosId != Sketcher::PointPos::none) {
                                break;
                            }
                            ptGeoId = Constr->Third;
                            ptPosId = Constr->ThirdPos;
                            if (ptPosId != Sketcher::PointPos::none) {
                                break;
                            }
                            assert(0);  // no point found!
                        } while (false);

                        midpos1 = geolistfacade.getPoint(ptGeoId, ptPosId);

                        norm1 = getNormal(geolistfacade, Constr->Second, midpos1);

                        // TODO: Check the method above. This was the old one making use of the
                        // solver.
                        // norm1 = getSolvedSketch().calculateNormalAtPoint(Constr->Second,
                        // midpos1.x, midpos1.y);

                        norm1.Normalize();
                        dir1 = norm1;
                        dir1.RotateZ(-M_PI / 2.0);
                    }
                    else if (Constr->FirstPos == Sketcher::PointPos::none) {

                        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment* lineSeg1 =
                                static_cast<const Part::GeomLineSegment*>(geo1);
                            midpos1 = ((lineSeg1->getEndPoint() + lineSeg1->getStartPoint()) / 2);
                            dir1 =
                                (lineSeg1->getEndPoint() - lineSeg1->getStartPoint()).Normalize();
                            norm1 = Base::Vector3d(-dir1.y, dir1.x, 0.);
                        }
                        else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arc =
                                static_cast<const Part::GeomArcOfCircle*>(geo1);
                            double startangle, endangle, midangle;
                            arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                            midangle = (startangle + endangle) / 2;
                            norm1 = Base::Vector3d(cos(midangle), sin(midangle), 0);
                            dir1 = Base::Vector3d(-norm1.y, norm1.x, 0);
                            midpos1 = arc->getCenter() + arc->getRadius() * norm1;
                        }
                        else if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle* circle =
                                static_cast<const Part::GeomCircle*>(geo1);
                            norm1 = Base::Vector3d(cos(M_PI / 4), sin(M_PI / 4), 0);
                            dir1 = Base::Vector3d(-norm1.y, norm1.x, 0);
                            midpos1 = circle->getCenter() + circle->getRadius() * norm1;
                        }
                        else {
                            break;
                        }

                        if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment* lineSeg2 =
                                static_cast<const Part::GeomLineSegment*>(geo2);
                            midpos2 = ((lineSeg2->getEndPoint() + lineSeg2->getStartPoint()) / 2);
                            dir2 =
                                (lineSeg2->getEndPoint() - lineSeg2->getStartPoint()).Normalize();
                            norm2 = Base::Vector3d(-dir2.y, dir2.x, 0.);
                        }
                        else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arc =
                                static_cast<const Part::GeomArcOfCircle*>(geo2);
                            double startangle, endangle, midangle;
                            arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                            midangle = (startangle + endangle) / 2;
                            norm2 = Base::Vector3d(cos(midangle), sin(midangle), 0);
                            dir2 = Base::Vector3d(-norm2.y, norm2.x, 0);
                            midpos2 = arc->getCenter() + arc->getRadius() * norm2;
                        }
                        else if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle* circle =
                                static_cast<const Part::GeomCircle*>(geo2);
                            norm2 = Base::Vector3d(cos(M_PI / 4), sin(M_PI / 4), 0);
                            dir2 = Base::Vector3d(-norm2.y, norm2.x, 0);
                            midpos2 = circle->getCenter() + circle->getRadius() * norm2;
                        }
                        else {
                            break;
                        }
                        twoIcons = true;
                    }

                    Base::Vector3d relpos1 =
                        seekConstraintPosition(midpos1,
                                               norm1,
                                               dir1,
                                               4.0,
                                               editModeScenegraphNodes.constrGroup->getChild(i));

                    auto translation = static_cast<SoZoomTranslation*>(sep->getChild(
                        static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                    translation->abPos = SbVec3f(midpos1.x, midpos1.y, zConstrH);
                    translation->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                    if (twoIcons) {
                        Base::Vector3d relpos2 = seekConstraintPosition(
                            midpos2,
                            norm2,
                            dir2,
                            4.0,
                            editModeScenegraphNodes.constrGroup->getChild(i));

                        Base::Vector3d secondPos = midpos2 - midpos1;
                        auto translation = static_cast<SoZoomTranslation*>(sep->getChild(
                            static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));
                        translation->abPos = SbVec3f(secondPos.x, secondPos.y, zConstrH);
                        translation->translation =
                            SbVec3f(relpos2.x - relpos1.x, relpos2.y - relpos1.y, 0);
                    }

                } break;
                case Parallel:
                case Equal: {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                    // get the geometry
                    const Part::Geometry* geo1 = geolistfacade.getGeometryFromGeoId(Constr->First);
                    const Part::Geometry* geo2 = geolistfacade.getGeometryFromGeoId(Constr->Second);

                    Base::Vector3d midpos1, dir1, norm1;
                    Base::Vector3d midpos2, dir2, norm2;
                    if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId()
                        || geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                        if (Constr->Type == Equal) {
                            double r1a = 0, r1b = 0, r2a = 0, r2b = 0;
                            double angle1,
                                angle1plus = 0., angle2,
                                angle2plus =
                                    0.;  // angle1 = rotation of object as a whole; angle1plus = arc
                                         // angle (t parameter for ellipses).
                            if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle* circle =
                                    static_cast<const Part::GeomCircle*>(geo1);
                                r1a = circle->getRadius();
                                angle1 = M_PI / 4;
                                midpos1 = circle->getCenter();
                            }
                            else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle* arc =
                                    static_cast<const Part::GeomArcOfCircle*>(geo1);
                                r1a = arc->getRadius();
                                double startangle, endangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                angle1 = (startangle + endangle) / 2;
                                midpos1 = arc->getCenter();
                            }
                            else if (geo1->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                const Part::GeomEllipse* ellipse =
                                    static_cast<const Part::GeomEllipse*>(geo1);
                                r1a = ellipse->getMajorRadius();
                                r1b = ellipse->getMinorRadius();
                                Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                angle1 = atan2(majdir.y, majdir.x);
                                angle1plus = M_PI / 4;
                                midpos1 = ellipse->getCenter();
                            }
                            else if (geo1->getTypeId()
                                     == Part::GeomArcOfEllipse::getClassTypeId()) {
                                const Part::GeomArcOfEllipse* aoe =
                                    static_cast<const Part::GeomArcOfEllipse*>(geo1);
                                r1a = aoe->getMajorRadius();
                                r1b = aoe->getMinorRadius();
                                double startangle, endangle;
                                aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = aoe->getMajorAxisDir();
                                angle1 = atan2(majdir.y, majdir.x);
                                angle1plus = (startangle + endangle) / 2;
                                midpos1 = aoe->getCenter();
                            }
                            else if (geo1->getTypeId()
                                     == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                const Part::GeomArcOfHyperbola* aoh =
                                    static_cast<const Part::GeomArcOfHyperbola*>(geo1);
                                r1a = aoh->getMajorRadius();
                                r1b = aoh->getMinorRadius();
                                double startangle, endangle;
                                aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = aoh->getMajorAxisDir();
                                angle1 = atan2(majdir.y, majdir.x);
                                angle1plus = (startangle + endangle) / 2;
                                midpos1 = aoh->getCenter();
                            }
                            else if (geo1->getTypeId()
                                     == Part::GeomArcOfParabola::getClassTypeId()) {
                                const Part::GeomArcOfParabola* aop =
                                    static_cast<const Part::GeomArcOfParabola*>(geo1);
                                r1a = aop->getFocal();
                                double startangle, endangle;
                                aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = -aop->getXAxisDir();
                                angle1 = atan2(majdir.y, majdir.x);
                                angle1plus = (startangle + endangle) / 2;
                                midpos1 = aop->getFocus();
                            }
                            else {
                                break;
                            }

                            if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle* circle =
                                    static_cast<const Part::GeomCircle*>(geo2);
                                r2a = circle->getRadius();
                                angle2 = M_PI / 4;
                                midpos2 = circle->getCenter();
                            }
                            else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle* arc =
                                    static_cast<const Part::GeomArcOfCircle*>(geo2);
                                r2a = arc->getRadius();
                                double startangle, endangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                angle2 = (startangle + endangle) / 2;
                                midpos2 = arc->getCenter();
                            }
                            else if (geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                const Part::GeomEllipse* ellipse =
                                    static_cast<const Part::GeomEllipse*>(geo2);
                                r2a = ellipse->getMajorRadius();
                                r2b = ellipse->getMinorRadius();
                                Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                angle2 = atan2(majdir.y, majdir.x);
                                angle2plus = M_PI / 4;
                                midpos2 = ellipse->getCenter();
                            }
                            else if (geo2->getTypeId()
                                     == Part::GeomArcOfEllipse::getClassTypeId()) {
                                const Part::GeomArcOfEllipse* aoe =
                                    static_cast<const Part::GeomArcOfEllipse*>(geo2);
                                r2a = aoe->getMajorRadius();
                                r2b = aoe->getMinorRadius();
                                double startangle, endangle;
                                aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = aoe->getMajorAxisDir();
                                angle2 = atan2(majdir.y, majdir.x);
                                angle2plus = (startangle + endangle) / 2;
                                midpos2 = aoe->getCenter();
                            }
                            else if (geo2->getTypeId()
                                     == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                const Part::GeomArcOfHyperbola* aoh =
                                    static_cast<const Part::GeomArcOfHyperbola*>(geo2);
                                r2a = aoh->getMajorRadius();
                                r2b = aoh->getMinorRadius();
                                double startangle, endangle;
                                aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = aoh->getMajorAxisDir();
                                angle2 = atan2(majdir.y, majdir.x);
                                angle2plus = (startangle + endangle) / 2;
                                midpos2 = aoh->getCenter();
                            }
                            else if (geo2->getTypeId()
                                     == Part::GeomArcOfParabola::getClassTypeId()) {
                                const Part::GeomArcOfParabola* aop =
                                    static_cast<const Part::GeomArcOfParabola*>(geo2);
                                r2a = aop->getFocal();
                                double startangle, endangle;
                                aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                Base::Vector3d majdir = -aop->getXAxisDir();
                                angle2 = atan2(majdir.y, majdir.x);
                                angle2plus = (startangle + endangle) / 2;
                                midpos2 = aop->getFocus();
                            }
                            else {
                                break;
                            }

                            if (geo1->getTypeId() == Part::GeomEllipse::getClassTypeId()
                                || geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                                || geo1->getTypeId()
                                    == Part::GeomArcOfHyperbola::getClassTypeId()) {

                                Base::Vector3d majDir, minDir, rvec;
                                majDir = Base::Vector3d(cos(angle1),
                                                        sin(angle1),
                                                        0);  // direction of major axis of ellipse
                                minDir = Base::Vector3d(-majDir.y,
                                                        majDir.x,
                                                        0);  // direction of minor axis of ellipse
                                rvec = (r1a * cos(angle1plus)) * majDir
                                    + (r1b * sin(angle1plus)) * minDir;
                                midpos1 += rvec;
                                rvec.Normalize();
                                norm1 = rvec;
                                dir1 = Base::Vector3d(
                                    -rvec.y,
                                    rvec.x,
                                    0);  // DeepSOIC: I'm not sure what dir is supposed to mean.
                            }
                            else {
                                norm1 = Base::Vector3d(cos(angle1), sin(angle1), 0);
                                dir1 = Base::Vector3d(-norm1.y, norm1.x, 0);
                                midpos1 += r1a * norm1;
                            }


                            if (geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()
                                || geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                                || geo2->getTypeId()
                                    == Part::GeomArcOfHyperbola::getClassTypeId()) {

                                Base::Vector3d majDir, minDir, rvec;
                                majDir = Base::Vector3d(cos(angle2),
                                                        sin(angle2),
                                                        0);  // direction of major axis of ellipse
                                minDir = Base::Vector3d(-majDir.y,
                                                        majDir.x,
                                                        0);  // direction of minor axis of ellipse
                                rvec = (r2a * cos(angle2plus)) * majDir
                                    + (r2b * sin(angle2plus)) * minDir;
                                midpos2 += rvec;
                                rvec.Normalize();
                                norm2 = rvec;
                                dir2 = Base::Vector3d(-rvec.y, rvec.x, 0);
                            }
                            else {
                                norm2 = Base::Vector3d(cos(angle2), sin(angle2), 0);
                                dir2 = Base::Vector3d(-norm2.y, norm2.x, 0);
                                midpos2 += r2a * norm2;
                            }
                        }
                        else {  // Parallel can only apply to a GeomLineSegment
                            break;
                        }
                    }
                    else {
                        const Part::GeomLineSegment* lineSeg1 =
                            static_cast<const Part::GeomLineSegment*>(geo1);
                        const Part::GeomLineSegment* lineSeg2 =
                            static_cast<const Part::GeomLineSegment*>(geo2);

                        // calculate the half distance between the start and endpoint
                        midpos1 = ((lineSeg1->getEndPoint() + lineSeg1->getStartPoint()) / 2);
                        midpos2 = ((lineSeg2->getEndPoint() + lineSeg2->getStartPoint()) / 2);
                        // Get a set of vectors perpendicular and tangential to these
                        dir1 = (lineSeg1->getEndPoint() - lineSeg1->getStartPoint()).Normalize();
                        dir2 = (lineSeg2->getEndPoint() - lineSeg2->getStartPoint()).Normalize();
                        norm1 = Base::Vector3d(-dir1.y, dir1.x, 0.);
                        norm2 = Base::Vector3d(-dir2.y, dir2.x, 0.);
                    }

                    Base::Vector3d relpos1 =
                        seekConstraintPosition(midpos1,
                                               norm1,
                                               dir1,
                                               4.0,
                                               editModeScenegraphNodes.constrGroup->getChild(i));
                    Base::Vector3d relpos2 =
                        seekConstraintPosition(midpos2,
                                               norm2,
                                               dir2,
                                               4.0,
                                               editModeScenegraphNodes.constrGroup->getChild(i));

                    auto translation = static_cast<SoZoomTranslation*>(sep->getChild(
                        static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                    translation->abPos =
                        SbVec3f(midpos1.x, midpos1.y, zConstrH);  // Absolute Reference

                    // Reference Position that is scaled according to zoom
                    translation->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                    Base::Vector3d secondPos = midpos2 - midpos1;

                    translation = static_cast<SoZoomTranslation*>(sep->getChild(
                        static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));

                    translation->abPos =
                        SbVec3f(secondPos.x, secondPos.y, zConstrH);  // Absolute Reference

                    // Reference Position that is scaled according to zoom
                    translation->translation =
                        SbVec3f(relpos2.x - relpos1.x, relpos2.y - relpos1.y, 0);

                } break;
                case Distance:
                case DistanceX:
                case DistanceY: {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    Base::Vector3d pnt1(0., 0., 0.), pnt2(0., 0., 0.);
                    if (Constr->SecondPos != Sketcher::PointPos::none) {  // point to point distance
                        pnt1 = geolistfacade.getPoint(Constr->First, Constr->FirstPos);
                        pnt2 = geolistfacade.getPoint(Constr->Second, Constr->SecondPos);
                    }
                    else if (Constr->Second != GeoEnum::GeoUndef) {
                        const Part::Geometry* geo =
                            geolistfacade.getGeometryFromGeoId(Constr->Second);
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment* lineSeg =
                                static_cast<const Part::GeomLineSegment*>(geo);
                            Base::Vector3d l2p1 = lineSeg->getStartPoint();
                            Base::Vector3d l2p2 = lineSeg->getEndPoint();
                            if (Constr->FirstPos
                                != Sketcher::PointPos::none) {  // point to line distance
                                pnt1 = geolistfacade.getPoint(Constr->First, Constr->FirstPos);
                                // calculate the projection of p1 onto line2
                                pnt2.ProjectToLine(pnt1 - l2p1, l2p2 - l2p1);
                                pnt2 += pnt1;
                            }
                            else {
                                const Part::Geometry* geo1 =
                                    geolistfacade.getGeometryFromGeoId(Constr->First);
                                if (geo1->getTypeId()
                                    == Part::GeomCircle::getClassTypeId()) {  // circle to line
                                                                              // distance
                                    const Part::GeomCircle* circleSeg =
                                        static_cast<const Part::GeomCircle*>(geo1);
                                    Base::Vector3d ct = circleSeg->getCenter();
                                    double radius = circleSeg->getRadius();
                                    pnt1.ProjectToLine(
                                        ct - l2p1,
                                        l2p2 - l2p1);  // project on the line translated to origin
                                    Base::Vector3d dir = pnt1;
                                    dir.Normalize();
                                    pnt1 += ct;
                                    pnt2 = ct + dir * radius;
                                }
                            }
                        }
                        else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::Geometry* geo1 =
                                geolistfacade.getGeometryFromGeoId(Constr->First);
                            if (geo1->getTypeId()
                                == Part::GeomCircle::getClassTypeId()) {  // circle to circle
                                                                          // distance
                                const Part::GeomCircle* circleSeg1 =
                                    static_cast<const Part::GeomCircle*>(geo1);
                                auto circleSeg2 = static_cast<const Part::GeomCircle*>(geo);
                                GetCirclesMinimalDistance(circleSeg1, circleSeg2, pnt1, pnt2);
                            }
                            else if (Constr->FirstPos
                                     != Sketcher::PointPos::none) {  // point to circle distance
                                auto circleSeg2 = static_cast<const Part::GeomCircle*>(geo);
                                pnt1 = geolistfacade.getPoint(Constr->First, Constr->FirstPos);
                                Base::Vector3d v = pnt1 - circleSeg2->getCenter();
                                v = v.Normalize();
                                pnt2 = circleSeg2->getCenter() + circleSeg2->getRadius() * v;
                            }
                        }
                        else {
                            break;
                        }
                    }
                    else if (Constr->FirstPos != Sketcher::PointPos::none) {
                        pnt2 = geolistfacade.getPoint(Constr->First, Constr->FirstPos);
                    }
                    else if (Constr->First != GeoEnum::GeoUndef) {
                        const Part::Geometry* geo =
                            geolistfacade.getGeometryFromGeoId(Constr->First);
                        if (geo->getTypeId()
                            == Part::GeomLineSegment::getClassTypeId()) {  // segment distance
                            const Part::GeomLineSegment* lineSeg =
                                static_cast<const Part::GeomLineSegment*>(geo);
                            pnt1 = lineSeg->getStartPoint();
                            pnt2 = lineSeg->getEndPoint();
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        break;
                    }

                    SoDatumLabel* asciiText = static_cast<SoDatumLabel*>(
                        sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));

                    // Get presentation string (w/o units if option is set)
                    asciiText->string =
                        SbString(getPresentationString(Constr).toUtf8().constData());

                    if (Constr->Type == Distance) {
                        asciiText->datumtype = SoDatumLabel::DISTANCE;
                    }
                    else if (Constr->Type == DistanceX) {
                        asciiText->datumtype = SoDatumLabel::DISTANCEX;
                    }
                    else if (Constr->Type == DistanceY) {
                        asciiText->datumtype = SoDatumLabel::DISTANCEY;
                    }

                    // Assign the Datum Points
                    asciiText->pnts.setNum(2);
                    SbVec3f* verts = asciiText->pnts.startEditing();

                    verts[0] = SbVec3f(pnt1.x, pnt1.y, zConstrH);
                    verts[1] = SbVec3f(pnt2.x, pnt2.y, zConstrH);

                    asciiText->pnts.finishEditing();

                    // Assign the Label Distance
                    asciiText->param1 = Constr->LabelDistance;
                    asciiText->param2 = Constr->LabelPosition;
                } break;
                case PointOnObject:
                case Tangent:
                case SnellsLaw: {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);

                    Base::Vector3d pos, relPos;
                    if (Constr->Type == PointOnObject || Constr->Type == SnellsLaw
                        || (Constr->Type == Tangent && Constr->Third != GeoEnum::GeoUndef)
                        ||  // Tangency via point
                        (Constr->Type == Tangent
                         && Constr->FirstPos
                             != Sketcher::PointPos::none)  // endpoint-to-curve or
                                                           // endpoint-to-endpoint tangency
                    ) {

                        // find the point of tangency/point that is on object
                        // just any point among first/second/third should be OK
                        int ptGeoId;
                        Sketcher::PointPos ptPosId;
                        do {  // dummy loop to use break =) Maybe goto?
                            ptGeoId = Constr->First;
                            ptPosId = Constr->FirstPos;
                            if (ptPosId != Sketcher::PointPos::none) {
                                break;
                            }
                            ptGeoId = Constr->Second;
                            ptPosId = Constr->SecondPos;
                            if (ptPosId != Sketcher::PointPos::none) {
                                break;
                            }
                            ptGeoId = Constr->Third;
                            ptPosId = Constr->ThirdPos;
                            if (ptPosId != Sketcher::PointPos::none) {
                                break;
                            }
                            assert(0);  // no point found!
                        } while (false);

                        pos = geolistfacade.getPoint(ptGeoId, ptPosId);
                        auto norm = getNormal(geolistfacade, Constr->Second, pos);

                        // TODO: Check substitution
                        // Base::Vector3d norm =
                        // getSolvedSketch().calculateNormalAtPoint(Constr->Second, pos.x, pos.y);
                        norm.Normalize();
                        Base::Vector3d dir = norm;
                        dir.RotateZ(-M_PI / 2.0);

                        relPos = seekConstraintPosition(
                            pos,
                            norm,
                            dir,
                            2.5,
                            editModeScenegraphNodes.constrGroup->getChild(i));

                        auto translation = static_cast<SoZoomTranslation*>(sep->getChild(
                            static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                        translation->abPos = SbVec3f(pos.x, pos.y, zConstrH);  // Absolute Reference
                        translation->translation = SbVec3f(relPos.x, relPos.y, 0);
                    }
                    else if (Constr->Type == Tangent) {
                        // get the geometry
                        const Part::Geometry* geo1 =
                            geolistfacade.getGeometryFromGeoId(Constr->First);
                        const Part::Geometry* geo2 =
                            geolistfacade.getGeometryFromGeoId(Constr->Second);

                        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                            && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment* lineSeg1 =
                                static_cast<const Part::GeomLineSegment*>(geo1);
                            const Part::GeomLineSegment* lineSeg2 =
                                static_cast<const Part::GeomLineSegment*>(geo2);
                            // tangency between two lines
                            Base::Vector3d midpos1 =
                                ((lineSeg1->getEndPoint() + lineSeg1->getStartPoint()) / 2);
                            Base::Vector3d midpos2 =
                                ((lineSeg2->getEndPoint() + lineSeg2->getStartPoint()) / 2);
                            Base::Vector3d dir1 =
                                (lineSeg1->getEndPoint() - lineSeg1->getStartPoint()).Normalize();
                            Base::Vector3d dir2 =
                                (lineSeg2->getEndPoint() - lineSeg2->getStartPoint()).Normalize();
                            Base::Vector3d norm1 = Base::Vector3d(-dir1.y, dir1.x, 0.f);
                            Base::Vector3d norm2 = Base::Vector3d(-dir2.y, dir2.x, 0.f);

                            Base::Vector3d relpos1 = seekConstraintPosition(
                                midpos1,
                                norm1,
                                dir1,
                                4.0,
                                editModeScenegraphNodes.constrGroup->getChild(i));
                            Base::Vector3d relpos2 = seekConstraintPosition(
                                midpos2,
                                norm2,
                                dir2,
                                4.0,
                                editModeScenegraphNodes.constrGroup->getChild(i));

                            auto translation = static_cast<SoZoomTranslation*>(sep->getChild(
                                static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                            translation->abPos =
                                SbVec3f(midpos1.x, midpos1.y, zConstrH);  // Absolute Reference
                            translation->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                            Base::Vector3d secondPos = midpos2 - midpos1;

                            translation = static_cast<SoZoomTranslation*>(sep->getChild(
                                static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));

                            translation->abPos =
                                SbVec3f(secondPos.x, secondPos.y, zConstrH);  // Absolute Reference
                            translation->translation =
                                SbVec3f(relpos2.x - relpos1.x, relpos2.y - relpos1.y, 0);

                            break;
                        }
                        else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            std::swap(geo1, geo2);
                        }

                        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment* lineSeg =
                                static_cast<const Part::GeomLineSegment*>(geo1);
                            Base::Vector3d dir =
                                (lineSeg->getEndPoint() - lineSeg->getStartPoint()).Normalize();
                            Base::Vector3d norm(-dir.y, dir.x, 0);
                            if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle* circle =
                                    static_cast<const Part::GeomCircle*>(geo2);
                                // tangency between a line and a circle
                                float length =
                                    (circle->getCenter() - lineSeg->getStartPoint()) * dir;

                                pos = lineSeg->getStartPoint() + dir * length;
                                relPos = norm * 1;  // TODO Huh?
                            }
                            else if (geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()
                                     || geo2->getTypeId()
                                         == Part::GeomArcOfEllipse::getClassTypeId()) {

                                Base::Vector3d center;
                                if (geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                    const Part::GeomEllipse* ellipse =
                                        static_cast<const Part::GeomEllipse*>(geo2);
                                    center = ellipse->getCenter();
                                }
                                else {
                                    const Part::GeomArcOfEllipse* aoc =
                                        static_cast<const Part::GeomArcOfEllipse*>(geo2);
                                    center = aoc->getCenter();
                                }

                                // tangency between a line and an ellipse
                                float length = (center - lineSeg->getStartPoint()) * dir;

                                pos = lineSeg->getStartPoint() + dir * length;
                                relPos = norm * 1;
                            }
                            else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle* arc =
                                    static_cast<const Part::GeomArcOfCircle*>(geo2);
                                // tangency between a line and an arc
                                float length = (arc->getCenter() - lineSeg->getStartPoint()) * dir;

                                pos = lineSeg->getStartPoint() + dir * length;
                                relPos = norm * 1;  // TODO Huh?
                            }
                        }

                        if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()
                            && geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle* circle1 =
                                static_cast<const Part::GeomCircle*>(geo1);
                            const Part::GeomCircle* circle2 =
                                static_cast<const Part::GeomCircle*>(geo2);
                            // tangency between two circles
                            Base::Vector3d dir =
                                (circle2->getCenter() - circle1->getCenter()).Normalize();
                            pos = circle1->getCenter() + dir * circle1->getRadius();
                            relPos = dir * 1;
                        }
                        else if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            std::swap(geo1, geo2);
                        }

                        if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()
                            && geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomCircle* circle =
                                static_cast<const Part::GeomCircle*>(geo1);
                            const Part::GeomArcOfCircle* arc =
                                static_cast<const Part::GeomArcOfCircle*>(geo2);
                            // tangency between a circle and an arc
                            Base::Vector3d dir =
                                (arc->getCenter() - circle->getCenter()).Normalize();
                            pos = circle->getCenter() + dir * circle->getRadius();
                            relPos = dir * 1;
                        }
                        else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                                 && geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arc1 =
                                static_cast<const Part::GeomArcOfCircle*>(geo1);
                            const Part::GeomArcOfCircle* arc2 =
                                static_cast<const Part::GeomArcOfCircle*>(geo2);
                            // tangency between two arcs
                            Base::Vector3d dir =
                                (arc2->getCenter() - arc1->getCenter()).Normalize();
                            pos = arc1->getCenter() + dir * arc1->getRadius();
                            relPos = dir * 1;
                        }
                        auto translation = static_cast<SoZoomTranslation*>(sep->getChild(
                            static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                        translation->abPos = SbVec3f(pos.x, pos.y, zConstrH);  // Absolute Reference
                        translation->translation = SbVec3f(relPos.x, relPos.y, 0);
                    }
                } break;
                case Symmetric: {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);

                    Base::Vector3d pnt1 = geolistfacade.getPoint(Constr->First, Constr->FirstPos);
                    Base::Vector3d pnt2 = geolistfacade.getPoint(Constr->Second, Constr->SecondPos);

                    SbVec3f p1(pnt1.x, pnt1.y, zConstrH);
                    SbVec3f p2(pnt2.x, pnt2.y, zConstrH);
                    SbVec3f dir = (p2 - p1);
                    dir.normalize();
                    SbVec3f norm(-dir[1], dir[0], 0);

                    SoDatumLabel* asciiText = static_cast<SoDatumLabel*>(
                        sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));
                    asciiText->datumtype = SoDatumLabel::SYMMETRIC;

                    asciiText->pnts.setNum(2);
                    SbVec3f* verts = asciiText->pnts.startEditing();

                    verts[0] = p1;
                    verts[1] = p2;

                    asciiText->pnts.finishEditing();

                    auto translation = static_cast<SoTranslation*>(sep->getChild(
                        static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

                    translation->translation = (p1 + p2) / 2;
                } break;
                case Angle: {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    assert((Constr->Second >= -extGeoCount && Constr->Second < intGeoCount)
                           || Constr->Second == GeoEnum::GeoUndef);

                    SbVec3f p0;
                    double startangle, range, endangle;
                    if (Constr->Second != GeoEnum::GeoUndef) {
                        Base::Vector3d dir1, dir2;
                        if (Constr->Third == GeoEnum::GeoUndef) {  // angle between two lines
                            const Part::Geometry* geo1 =
                                geolistfacade.getGeometryFromGeoId(Constr->First);
                            const Part::Geometry* geo2 =
                                geolistfacade.getGeometryFromGeoId(Constr->Second);
                            if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId()
                                || geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                                break;
                            }
                            const Part::GeomLineSegment* lineSeg1 =
                                static_cast<const Part::GeomLineSegment*>(geo1);
                            const Part::GeomLineSegment* lineSeg2 =
                                static_cast<const Part::GeomLineSegment*>(geo2);

                            bool flip1 = (Constr->FirstPos == PointPos::end);
                            bool flip2 = (Constr->SecondPos == PointPos::end);
                            dir1 = (flip1 ? -1. : 1.)
                                * (lineSeg1->getEndPoint() - lineSeg1->getStartPoint());
                            dir2 = (flip2 ? -1. : 1.)
                                * (lineSeg2->getEndPoint() - lineSeg2->getStartPoint());
                            Base::Vector3d pnt1 =
                                flip1 ? lineSeg1->getEndPoint() : lineSeg1->getStartPoint();
                            Base::Vector3d pnt2 =
                                flip2 ? lineSeg2->getEndPoint() : lineSeg2->getStartPoint();

                            // line-line intersection
                            {
                                double det = dir1.x * dir2.y - dir1.y * dir2.x;
                                if ((det > 0 ? det : -det) < 1e-10) {
                                    // lines are coincident (or parallel) and in this case the
                                    // center of the point pairs with the shortest distance is used
                                    Base::Vector3d p1[2], p2[2];
                                    p1[0] = lineSeg1->getStartPoint();
                                    p1[1] = lineSeg1->getEndPoint();
                                    p2[0] = lineSeg2->getStartPoint();
                                    p2[1] = lineSeg2->getEndPoint();
                                    double length = DBL_MAX;
                                    for (int i = 0; i <= 1; i++) {
                                        for (int j = 0; j <= 1; j++) {
                                            double tmp = (p2[j] - p1[i]).Length();
                                            if (tmp < length) {
                                                length = tmp;
                                                p0.setValue((p2[j].x + p1[i].x) / 2,
                                                            (p2[j].y + p1[i].y) / 2,
                                                            0);
                                            }
                                        }
                                    }
                                }
                                else {
                                    double c1 = dir1.y * pnt1.x - dir1.x * pnt1.y;
                                    double c2 = dir2.y * pnt2.x - dir2.x * pnt2.y;
                                    double x = (dir1.x * c2 - dir2.x * c1) / det;
                                    double y = (dir1.y * c2 - dir2.y * c1) / det;
                                    p0 = SbVec3f(x, y, 0);
                                }
                            }

                            range = Constr->getValue();  // WYSIWYG
                            startangle = atan2(dir1.y, dir1.x);
                        }
                        else {  // angle-via-point
                            Base::Vector3d p =
                                geolistfacade.getPoint(Constr->Third, Constr->ThirdPos);
                            p0 = SbVec3f(p.x, p.y, 0);
                            dir1 = getNormal(geolistfacade, Constr->First, p);
                            // TODO: Check
                            // dir1 = getSolvedSketch().calculateNormalAtPoint(Constr->First, p.x,
                            // p.y);
                            dir1.RotateZ(-M_PI / 2);  // convert to vector of tangency by rotating
                            dir2 = getNormal(geolistfacade, Constr->Second, p);
                            // TODO: Check
                            // dir2 = getSolvedSketch().calculateNormalAtPoint(Constr->Second, p.x,
                            // p.y);
                            dir2.RotateZ(-M_PI / 2);

                            startangle = atan2(dir1.y, dir1.x);
                            range = atan2(dir1.x * dir2.y - dir1.y * dir2.x,
                                          dir1.x * dir2.x + dir1.y * dir2.y);
                        }

                        endangle = startangle + range;
                    }
                    else if (Constr->First != GeoEnum::GeoUndef) {
                        const Part::Geometry* geo =
                            geolistfacade.getGeometryFromGeoId(Constr->First);
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment* lineSeg =
                                static_cast<const Part::GeomLineSegment*>(geo);
                            p0 = Base::convertTo<SbVec3f>(
                                (lineSeg->getEndPoint() + lineSeg->getStartPoint()) / 2);

                            Base::Vector3d dir = lineSeg->getEndPoint() - lineSeg->getStartPoint();
                            startangle = 0.;
                            range = atan2(dir.y, dir.x);
                            endangle = startangle + range;
                        }
                        else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arc =
                                static_cast<const Part::GeomArcOfCircle*>(geo);
                            p0 = Base::convertTo<SbVec3f>(arc->getCenter());

                            arc->getRange(startangle, endangle, /*emulateCCWXY=*/true);
                            range = endangle - startangle;
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        break;
                    }

                    SoDatumLabel* asciiText = static_cast<SoDatumLabel*>(
                        sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));
                    asciiText->string =
                        SbString(getPresentationString(Constr).toUtf8().constData());
                    asciiText->datumtype = SoDatumLabel::ANGLE;
                    asciiText->param1 = Constr->LabelDistance;
                    asciiText->param2 = startangle;
                    asciiText->param3 = range;

                    asciiText->pnts.setNum(2);
                    SbVec3f* verts = asciiText->pnts.startEditing();

                    verts[0] = p0;

                    asciiText->pnts.finishEditing();

                } break;
                case Diameter: {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                    Base::Vector3d pnt1(0., 0., 0.), pnt2(0., 0., 0.);
                    if (Constr->First != GeoEnum::GeoUndef) {
                        const Part::Geometry* geo =
                            geolistfacade.getGeometryFromGeoId(Constr->First);

                        if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arc =
                                static_cast<const Part::GeomArcOfCircle*>(geo);
                            double radius = arc->getRadius();
                            double angle = (double)Constr->LabelPosition;
                            if (angle == 10) {
                                double startangle, endangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                angle = (startangle + endangle) / 2;
                            }
                            Base::Vector3d center = arc->getCenter();
                            pnt1 = center - radius * Base::Vector3d(cos(angle), sin(angle), 0.);
                            pnt2 = center + radius * Base::Vector3d(cos(angle), sin(angle), 0.);
                        }
                        else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle* circle =
                                static_cast<const Part::GeomCircle*>(geo);
                            double radius = circle->getRadius();
                            double angle = (double)Constr->LabelPosition;
                            if (angle == 10) {
                                angle = 0;
                            }
                            Base::Vector3d center = circle->getCenter();
                            pnt1 = center - radius * Base::Vector3d(cos(angle), sin(angle), 0.);
                            pnt2 = center + radius * Base::Vector3d(cos(angle), sin(angle), 0.);
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        break;
                    }

                    SbVec3f p1(pnt1.x, pnt1.y, zConstrH);
                    SbVec3f p2(pnt2.x, pnt2.y, zConstrH);

                    SoDatumLabel* asciiText = static_cast<SoDatumLabel*>(
                        sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));

                    // Get display string with units hidden if so requested
                    asciiText->string =
                        SbString(getPresentationString(Constr).toUtf8().constData());

                    asciiText->datumtype = SoDatumLabel::DIAMETER;
                    asciiText->param1 = Constr->LabelDistance;
                    asciiText->param2 = Constr->LabelPosition;

                    asciiText->pnts.setNum(2);
                    SbVec3f* verts = asciiText->pnts.startEditing();

                    verts[0] = p1;
                    verts[1] = p2;

                    asciiText->pnts.finishEditing();
                } break;
                case Weight:
                case Radius: {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                    Base::Vector3d pnt1(0., 0., 0.), pnt2(0., 0., 0.);

                    if (Constr->First != GeoEnum::GeoUndef) {
                        const Part::Geometry* geo =
                            geolistfacade.getGeometryFromGeoId(Constr->First);

                        if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arc =
                                static_cast<const Part::GeomArcOfCircle*>(geo);
                            double radius = arc->getRadius();
                            double angle = (double)Constr->LabelPosition;
                            if (angle == 10) {
                                double startangle, endangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                angle = (startangle + endangle) / 2;
                            }
                            pnt1 = arc->getCenter();
                            pnt2 = pnt1 + radius * Base::Vector3d(cos(angle), sin(angle), 0.);
                        }
                        else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle* circle =
                                static_cast<const Part::GeomCircle*>(geo);
                            auto gf = GeometryFacade::getFacade(geo);

                            double radius;

                            radius = circle->getRadius();

                            double angle = (double)Constr->LabelPosition;
                            if (angle == 10) {
                                angle = 0;
                            }
                            pnt1 = circle->getCenter();
                            pnt2 = pnt1 + radius * Base::Vector3d(cos(angle), sin(angle), 0.);
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        break;
                    }

                    SbVec3f p1(pnt1.x, pnt1.y, zConstrH);
                    SbVec3f p2(pnt2.x, pnt2.y, zConstrH);

                    SoDatumLabel* asciiText = static_cast<SoDatumLabel*>(
                        sep->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));

                    // Get display string with units hidden if so requested
                    if (Constr->Type == Weight) {
                        asciiText->string =
                            SbString(QString::number(Constr->getValue()).toStdString().c_str());
                    }
                    else {
                        asciiText->string =
                            SbString(getPresentationString(Constr).toUtf8().constData());
                    }

                    asciiText->datumtype = SoDatumLabel::RADIUS;
                    asciiText->param1 = Constr->LabelDistance;
                    asciiText->param2 = Constr->LabelPosition;

                    asciiText->pnts.setNum(2);
                    SbVec3f* verts = asciiText->pnts.startEditing();

                    verts[0] = p1;
                    verts[1] = p2;

                    asciiText->pnts.finishEditing();
                } break;
                case Coincident:  // nothing to do for coincident
                case None:
                case InternalAlignment:
                case NumConstraintTypes:
                    break;
            }
        }
        catch (Base::Exception& e) {
            Base::Console().DeveloperError("EditModeConstraintCoinManager",
                                           "Exception during draw: %s\n",
                                           e.what());
            e.ReportException();
        }
        catch (...) {
            Base::Console().DeveloperError("EditModeConstraintCoinManager",
                                           "Exception during draw: unknown\n");
        }
    }
}

Base::Vector3d EditModeConstraintCoinManager::seekConstraintPosition(const Base::Vector3d& origPos,
                                                                     const Base::Vector3d& norm,
                                                                     const Base::Vector3d& dir,
                                                                     float step,
                                                                     const SoNode* constraint)
{

    auto rp = ViewProviderSketchCoinAttorney::getRayPickAction(viewProvider);

    float scaled_step = step * ViewProviderSketchCoinAttorney::getScaleFactor(viewProvider);

    int multiplier = 0;
    Base::Vector3d relPos, freePos;
    bool isConstraintAtPosition = true;
    while (isConstraintAtPosition && multiplier < 10) {
        // Calculate new position of constraint
        relPos = norm * 0.5f + dir * multiplier;
        freePos = origPos + relPos * scaled_step;

        // Prevent crash : https://forum.freecad.org/viewtopic.php?f=8&t=65305
        if (!rp) {
            return relPos * step;
        }

        rp->setRadius(0.1f);
        rp->setPickAll(true);
        rp->setRay(SbVec3f(freePos.x, freePos.y, -1.f), SbVec3f(0, 0, 1));
        // problem
        rp->apply(editModeScenegraphNodes.constrGroup);  // We could narrow it down to just the
                                                         // SoGroup containing the constraints

        // returns a copy of the point
        SoPickedPoint* pp = rp->getPickedPoint();
        const SoPickedPointList ppl = rp->getPickedPointList();

        if (ppl.getLength() <= 1 && pp) {
            SoPath* path = pp->getPath();
            int length = path->getLength();
            SoNode* tailFather1 = path->getNode(length - 2);
            SoNode* tailFather2 = path->getNode(length - 3);

            // checking if a constraint is the same as the one selected
            if (tailFather1 == constraint || tailFather2 == constraint) {
                isConstraintAtPosition = false;
            }
        }
        else {
            isConstraintAtPosition = false;
        }

        multiplier *= -1;  // search in both sides
        if (multiplier >= 0) {
            multiplier++;  // Increment the multiplier
        }
    }
    if (multiplier == 10) {
        relPos = norm * 0.5f;  // no free position found
    }
    return relPos * step;
}

void EditModeConstraintCoinManager::updateConstraintColor(
    const std::vector<Sketcher::Constraint*>& constraints)
{
    // Because coincident constraints are selected using the point color, we need to edit the point
    // materials.

    std::vector<int> PtNum;
    std::vector<SbColor*> pcolor;  // point color
    std::vector<int> CurvNum;
    std::vector<SbColor*> color;  // curve color

    for (int l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        PtNum.push_back(editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.getNum());
        pcolor.push_back(editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.startEditing());
        CurvNum.push_back(editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.getNum());
        color.push_back(editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.startEditing());
    }

    int maxNumberOfConstraints = std::min(editModeScenegraphNodes.constrGroup->getNumChildren(),
                                          static_cast<int>(constraints.size()));

    // colors of the constraints
    for (int i = 0; i < maxNumberOfConstraints; i++) {
        SoSeparator* s =
            static_cast<SoSeparator*>(editModeScenegraphNodes.constrGroup->getChild(i));

        // Check Constraint Type
        Sketcher::Constraint* constraint = constraints[i];
        ConstraintType type = constraint->Type;

        // It may happen that color updates are triggered by programmatic selection changes before a
        // command final update. Then constraints may have been changed and the color will be
        // updated as part
        if (type != vConstrType[i]) {
            break;
        }

        bool hasDatumLabel = (type == Sketcher::Angle || type == Sketcher::Radius
                              || type == Sketcher::Diameter || type == Sketcher::Weight
                              || type == Sketcher::Symmetric || type == Sketcher::Distance
                              || type == Sketcher::DistanceX || type == Sketcher::DistanceY);

        // Non DatumLabel Nodes will have a material excluding coincident
        bool hasMaterial = false;

        SoMaterial* m = nullptr;
        if (!hasDatumLabel && type != Sketcher::Coincident && type != Sketcher::InternalAlignment) {
            hasMaterial = true;
            m = static_cast<SoMaterial*>(
                s->getChild(static_cast<int>(ConstraintNodePosition::MaterialIndex)));
        }

        auto selectpoint = [this, pcolor, PtNum](int geoid, Sketcher::PointPos pos) {
            if (geoid >= 0) {
                auto multifieldIndex = coinMapping.getIndexLayer(geoid, pos);

                if (multifieldIndex != MultiFieldId::Invalid) {
                    int index = multifieldIndex.fieldIndex;
                    int layer = multifieldIndex.layerId;
                    if (layer < static_cast<int>(PtNum.size()) && index >= 0
                        && index < PtNum[layer]) {
                        pcolor[layer][index] = drawingParameters.SelectColor;
                    }
                }
            }
        };

        auto selectline = [this, color, CurvNum](int geoid) {
            if (geoid >= 0) {
                auto multifieldIndex = coinMapping.getIndexLayer(geoid, Sketcher::PointPos::none);

                if (multifieldIndex != MultiFieldId::Invalid) {
                    int index = multifieldIndex.fieldIndex;
                    int layer = multifieldIndex.layerId;
                    if (layer < static_cast<int>(CurvNum.size()) && index >= 0
                        && index < CurvNum[layer]) {
                        color[layer][index] = drawingParameters.SelectColor;
                    }
                }
            }
        };


        if (ViewProviderSketchCoinAttorney::isConstraintSelected(viewProvider, i)) {
            if (hasDatumLabel) {
                SoDatumLabel* l = static_cast<SoDatumLabel*>(
                    s->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));
                l->textColor = drawingParameters.SelectColor;
            }
            else if (hasMaterial) {
                m->diffuseColor = drawingParameters.SelectColor;
            }
            else if (type == Sketcher::Coincident) {
                selectpoint(constraint->First, constraint->FirstPos);
                selectpoint(constraint->Second, constraint->SecondPos);
            }
            else if (type == Sketcher::InternalAlignment) {
                switch (constraint->AlignmentType) {
                    case EllipseMajorDiameter:
                    case EllipseMinorDiameter:
                    case HyperbolaMajor:
                    case HyperbolaMinor:
                    case ParabolaFocalAxis: {
                        selectline(constraint->First);
                    } break;
                    case EllipseFocus1:
                    case EllipseFocus2:
                    case HyperbolaFocus:
                    case ParabolaFocus:
                    case BSplineControlPoint:
                    case BSplineKnotPoint: {
                        selectpoint(constraint->First, constraint->FirstPos);
                    } break;
                    default:
                        break;
                }
            }
        }
        else if (ViewProviderSketchCoinAttorney::isConstraintPreselected(viewProvider, i)) {
            if (hasDatumLabel) {
                SoDatumLabel* l = static_cast<SoDatumLabel*>(
                    s->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));
                l->textColor = drawingParameters.PreselectColor;
            }
            else if (hasMaterial) {
                m->diffuseColor = drawingParameters.PreselectColor;
            }
        }
        else {
            if (hasDatumLabel) {
                SoDatumLabel* l = static_cast<SoDatumLabel*>(
                    s->getChild(static_cast<int>(ConstraintNodePosition::DatumLabelIndex)));

                l->textColor = constraint->isActive
                    ? ViewProviderSketchCoinAttorney::constraintHasExpression(viewProvider, i)
                        ? drawingParameters.ExprBasedConstrDimColor
                        : (constraint->isDriving ? drawingParameters.ConstrDimColor
                                                 : drawingParameters.NonDrivingConstrDimColor)
                    : drawingParameters.DeactivatedConstrDimColor;
            }
            else if (hasMaterial) {
                m->diffuseColor = constraint->isActive
                    ? (constraint->isDriving ? drawingParameters.ConstrDimColor
                                             : drawingParameters.NonDrivingConstrDimColor)
                    : drawingParameters.DeactivatedConstrDimColor;
            }
        }
    }

    for (int l = 0; l < geometryLayerParameters.getCoinLayerCount(); l++) {
        editModeScenegraphNodes.PointsMaterials[l]->diffuseColor.finishEditing();
        editModeScenegraphNodes.CurvesMaterials[l]->diffuseColor.finishEditing();
    }
}

void EditModeConstraintCoinManager::rebuildConstraintNodes()
{
    auto geolistfacade = ViewProviderSketchCoinAttorney::getGeoListFacade(viewProvider);

    rebuildConstraintNodes(geolistfacade);
}

void EditModeConstraintCoinManager::setConstraintSelectability(bool enabled /* = true */)
{
    if (enabled) {
        editModeScenegraphNodes.constrGrpSelect->style.setValue(SoPickStyle::SHAPE);
    }
    else {
        editModeScenegraphNodes.constrGrpSelect->style.setValue(SoPickStyle::UNPICKABLE);
    }
}

void EditModeConstraintCoinManager::rebuildConstraintNodes(const GeoListFacade& geolistfacade)
{
    const std::vector<Sketcher::Constraint*>& constrlist =
        ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    // clean up
    Gui::coinRemoveAllChildren(editModeScenegraphNodes.constrGroup);

    vConstrType.clear();

    // Get sketch normal
    Base::Vector3d RN(0, 0, 1);

    // move to position of Sketch
    Base::Placement Plz = ViewProviderSketchCoinAttorney::getEditingPlacement(viewProvider);
    Base::Rotation tmp(Plz.getRotation());
    tmp.multVec(RN, RN);
    Plz.setRotation(tmp);

    SbVec3f norm(RN.x, RN.y, RN.z);

    rebuildConstraintNodes(geolistfacade, constrlist, norm);
}

void EditModeConstraintCoinManager::rebuildConstraintNodes(
    const GeoListFacade& geolistfacade,
    const std::vector<Sketcher::Constraint*> constrlist,
    SbVec3f norm)
{

    for (std::vector<Sketcher::Constraint*>::const_iterator it = constrlist.begin();
         it != constrlist.end();
         ++it) {
        // root separator for one constraint
        SoSeparator* sep = new SoSeparator();
        sep->ref();
        // no caching for frequently-changing data structures
        sep->renderCaching = SoSeparator::OFF;

        // every constrained visual node gets its own material for preselection and selection
        SoMaterial* mat = new SoMaterial;
        mat->ref();
        mat->diffuseColor = (*it)->isActive
            ? ((*it)->isDriving ? drawingParameters.ConstrDimColor
                                : drawingParameters.NonDrivingConstrDimColor)
            : drawingParameters.DeactivatedConstrDimColor;


        // distinguish different constraint types to build up
        switch ((*it)->Type) {
            case Distance:
            case DistanceX:
            case DistanceY:
            case Radius:
            case Diameter:
            case Weight:
            case Angle: {
                SoDatumLabel* text = new SoDatumLabel();
                text->norm.setValue(norm);
                text->string = "";
                text->textColor = (*it)->isActive
                    ? ((*it)->isDriving ? drawingParameters.ConstrDimColor
                                        : drawingParameters.NonDrivingConstrDimColor)
                    : drawingParameters.DeactivatedConstrDimColor;
                text->size.setValue(drawingParameters.labelFontSize);
                text->lineWidth = 2 * drawingParameters.pixelScalingFactor;
                text->useAntialiasing = false;
                SoAnnotation* anno = new SoAnnotation();
                anno->renderCaching = SoSeparator::OFF;
                anno->addChild(text);
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(text);
                editModeScenegraphNodes.constrGroup->addChild(anno);
                vConstrType.push_back((*it)->Type);
                // nodes not needed
                sep->unref();
                mat->unref();
                continue;  // jump to next constraint
            } break;
            case Horizontal:
            case Vertical:
            case Block: {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                sep->addChild(new SoInfo());

                // remember the type of this constraint node
                vConstrType.push_back((*it)->Type);
            } break;
            case Coincident:  // no visual for coincident so far
                vConstrType.push_back(Coincident);
                break;
            case Parallel:
            case Perpendicular:
            case Equal: {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                sep->addChild(new SoInfo());

                // remember the type of this constraint node
                vConstrType.push_back((*it)->Type);
            } break;
            case PointOnObject:
            case Tangent:
            case SnellsLaw: {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());

                if ((*it)->Type == Tangent) {
                    const Part::Geometry* geo1 = geolistfacade.getGeometryFromGeoId((*it)->First);
                    const Part::Geometry* geo2 = geolistfacade.getGeometryFromGeoId((*it)->Second);
                    if (!geo1 || !geo2) {
                        Base::Console().DeveloperWarning(
                            "EditModeConstraintCoinManager",
                            "Tangent constraint references non-existing geometry\n");
                    }
                    else if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                             && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                        sep->addChild(new SoZoomTranslation());
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                        sep->addChild(new SoImage());
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                        sep->addChild(new SoInfo());
                    }
                }

                vConstrType.push_back((*it)->Type);
            } break;
            case Symmetric: {
                SoDatumLabel* arrows = new SoDatumLabel();
                arrows->norm.setValue(norm);
                arrows->string = "";
                arrows->textColor = drawingParameters.ConstrDimColor;
                arrows->lineWidth = 2 * drawingParameters.pixelScalingFactor;

                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(arrows);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());

                vConstrType.push_back((*it)->Type);
            } break;
            case InternalAlignment: {
                vConstrType.push_back((*it)->Type);
            } break;
            default:
                vConstrType.push_back((*it)->Type);
        }

        editModeScenegraphNodes.constrGroup->addChild(sep);
        // decrement ref counter again
        sep->unref();
        mat->unref();
    }
}

QString EditModeConstraintCoinManager::getPresentationString(const Constraint* constraint)
{
    QString nameStr;           // name parameter string
    QString valueStr;          // dimensional value string
    QString presentationStr;   // final return string
    QString unitStr;           // the actual unit string
    QString baseUnitStr;       // the expected base unit string
    double factor;             // unit scaling factor, currently not used
    Base::UnitSystem unitSys;  // current unit system

    if (!constraint->isActive) {
        return QString::fromLatin1(" ");
    }

    // Get the current name parameter string of the constraint
    nameStr = QString::fromStdString(constraint->Name);

    // Get the current value string including units
    valueStr = constraint->getPresentationValue().getUserString(factor, unitStr);

    // Hide units if user has requested it, is being displayed in the base
    // units, and the schema being used has a clear base unit in the first
    // place. Otherwise, display units.
    if (constraintParameters.bHideUnits && constraint->Type != Sketcher::Angle) {
        // Only hide the default length unit. Right now there is not an easy way
        // to get that from the Unit system so we have to manually add it here.
        // Hopefully this can be added in the future so this code won't have to
        // be updated if a new units schema is added.
        unitSys = Base::UnitsApi::getSchema();

        // If this is a supported unit system then define what the base unit is.
        switch (unitSys) {
            case Base::UnitSystem::SI1:
            case Base::UnitSystem::MmMin:
                baseUnitStr = QString::fromLatin1("mm");
                break;

            case Base::UnitSystem::SI2:
                baseUnitStr = QString::fromLatin1("m");
                break;

            case Base::UnitSystem::ImperialDecimal:
                baseUnitStr = QString::fromLatin1("in");
                break;

            case Base::UnitSystem::Centimeters:
                baseUnitStr = QString::fromLatin1("cm");
                break;

            default:
                // Nothing to do
                break;
        }

        if (!baseUnitStr.isEmpty()) {
            // expected unit string matches actual unit string. remove.
            if (QString::compare(baseUnitStr, unitStr) == 0) {
                // Example code from: Mod/TechDraw/App/DrawViewDimension.cpp:372
                QRegularExpression rxUnits(
                    QString::fromUtf8(" \\D*$"));  // space + any non digits at end of string
                valueStr.remove(rxUnits);          // getUserString(defaultDecimals) without units
            }
        }
    }

    if (constraint->Type == Sketcher::Diameter) {
        valueStr.prepend(QChar(216));  // Diameter sign
    }
    else if (constraint->Type == Sketcher::Radius) {
        valueStr.prepend(QChar(82));  // Capital letter R
    }

    /**
    Create the representation string from the user defined format string
    Format options are:
    %N - the constraint name parameter
    %V - the value of the dimensional constraint, including any unit characters
    */
    if (constraintParameters.bShowDimensionalName && !nameStr.isEmpty()) {
        if (constraintParameters.sDimensionalStringFormat.contains(QLatin1String("%V"))
            || constraintParameters.sDimensionalStringFormat.contains(QLatin1String("%N"))) {
            presentationStr = constraintParameters.sDimensionalStringFormat;
            presentationStr.replace(QLatin1String("%N"), nameStr);
            presentationStr.replace(QLatin1String("%V"), valueStr);
        }
        else {
            // user defined format string does not contain any valid parameter, using default format
            // "%N = %V"
            presentationStr = nameStr + QLatin1String(" = ") + valueStr;
        }

        return presentationStr;
    }

    return valueStr;
}

std::set<int> EditModeConstraintCoinManager::detectPreselectionConstr(const SoPickedPoint* Point,
                                                                      const SbVec2s& cursorPos)
{
    std::set<int> constrIndices;
    SoPath* path = Point->getPath();

    // Get the constraints' tail
    SoNode* tailFather2 = path->getNode(path->getLength() - 3);

    if (tailFather2 != editModeScenegraphNodes.constrGroup) {
        return constrIndices;
    }


    SoNode* tail = path->getTail();
    SoNode* tailFather = path->getNode(path->getLength() - 2);

    for (int i = 0; i < editModeScenegraphNodes.constrGroup->getNumChildren(); ++i) {
        if (editModeScenegraphNodes.constrGroup->getChild(i) == tailFather) {
            SoSeparator* sep = static_cast<SoSeparator*>(tailFather);
            if (sep->getNumChildren()
                > static_cast<int>(ConstraintNodePosition::FirstConstraintIdIndex)) {
                SoInfo* constrIds = nullptr;
                if (tail
                    == sep->getChild(static_cast<int>(ConstraintNodePosition::FirstIconIndex))) {
                    // First icon was hit
                    constrIds = static_cast<SoInfo*>(sep->getChild(
                        static_cast<int>(ConstraintNodePosition::FirstConstraintIdIndex)));
                }
                else {
                    // Assume second icon was hit
                    if (static_cast<int>(ConstraintNodePosition::SecondConstraintIdIndex)
                        < sep->getNumChildren()) {
                        constrIds = static_cast<SoInfo*>(sep->getChild(
                            static_cast<int>(ConstraintNodePosition::SecondConstraintIdIndex)));
                    }
                }

                if (constrIds) {
                    QString constrIdsStr =
                        QString::fromLatin1(constrIds->string.getValue().getString());
                    if (combinedConstrBoxes.count(constrIdsStr) && dynamic_cast<SoImage*>(tail)) {
                        // If it's a combined constraint icon

                        // Screen dimensions of the icon
                        SbVec3s iconSize = getDisplayedSize(static_cast<SoImage*>(tail));
                        // Center of the icon
                        // SbVec2f iconCoords = viewer->screenCoordsOfPath(path);

                        // The use of the Path to get the screen coordinates to get the icon center
                        // coordinates does not work.
                        //
                        // This implementation relies on the use of ZoomTranslation to get the
                        // absolute and relative positions of the icons.
                        //
                        // In the case of second icons (the same constraint has two icons at two
                        // different positions), the translation vectors have to be added, as the
                        // second ZoomTranslation operates on top of the first.
                        //
                        // Coordinates are projected on the sketch plane and then to the screen in
                        // the interval [0 1] Then this result is converted to pixels using the
                        // scale factor.

                        SbVec3f absPos;
                        SbVec3f trans;
                        float scaleFactor;

                        auto translation = static_cast<SoZoomTranslation*>(
                            static_cast<SoSeparator*>(tailFather)
                                ->getChild(static_cast<int>(
                                    ConstraintNodePosition::FirstTranslationIndex)));

                        absPos = translation->abPos.getValue();

                        trans = translation->translation.getValue();

                        scaleFactor = translation->getScaleFactor();

                        if (tail
                            != sep->getChild(
                                static_cast<int>(ConstraintNodePosition::FirstIconIndex))) {
                            Base::Console().Log("SecondIcon\n");

                            auto translation2 = static_cast<SoZoomTranslation*>(
                                static_cast<SoSeparator*>(tailFather)
                                    ->getChild(static_cast<int>(
                                        ConstraintNodePosition::SecondTranslationIndex)));

                            absPos += translation2->abPos.getValue();

                            trans += translation2->translation.getValue();

                            scaleFactor = translation2->getScaleFactor();
                        }

                        // Only the translation is scaled because this is how SoZoomTranslation
                        // works
                        SbVec3f constrPos = absPos + scaleFactor * trans;

                        SbVec2f iconCoords = ViewProviderSketchCoinAttorney::getScreenCoordinates(
                            viewProvider,
                            SbVec2f(constrPos[0], constrPos[1]));

                        // cursorPos is SbVec2s in screen coordinates coming from SoEvent in
                        // mousemove
                        //
                        // Coordinates of the mouse cursor on the icon, origin at top-left for Qt
                        // but bottom-left for OIV.
                        // The coordinates are needed in Qt format, i.e. from top to bottom.
                        int iconX = cursorPos[0] - iconCoords[0] + iconSize[0] / 2,
                            iconY = cursorPos[1] - iconCoords[1] + iconSize[1] / 2;
                        iconY = iconSize[1] - iconY;

                        for (ConstrIconBBVec::iterator b =
                                 combinedConstrBoxes[constrIdsStr].begin();
                             b != combinedConstrBoxes[constrIdsStr].end();
                             ++b) {

#ifdef FC_DEBUG
                            // Useful code to debug coordinates and bounding boxes that does not
                            // need to be compiled in for any debug operations.

                            /*Base::Console().Log("Abs(%f,%f),Trans(%f,%f),Coords(%d,%d),iCoords(%f,%f),icon(%d,%d),isize(%d,%d),boundingbox([%d,%d],[%d,%d])\n",
                             * absPos[0],absPos[1],trans[0], trans[1], cursorPos[0], cursorPos[1],
                             * iconCoords[0], iconCoords[1], iconX, iconY, iconSize[0], iconSize[1],
                             * b->first.topLeft().x(),b->first.topLeft().y(),b->first.bottomRight().x(),b->first.bottomRight().y());*/
#endif

                            if (b->first.contains(iconX, iconY)) {
                                // We've found a bounding box that contains the mouse pointer!
                                for (std::set<int>::iterator k = b->second.begin();
                                     k != b->second.end();
                                     ++k) {
                                    constrIndices.insert(*k);
                                }
                            }
                        }
                    }
                    else {
                        // It's a constraint icon, not a combined one
                        QStringList constrIdStrings = constrIdsStr.split(QString::fromLatin1(","));
                        while (!constrIdStrings.empty()) {
                            auto constraintid = constrIdStrings.takeAt(0).toInt();
                            constrIndices.insert(constraintid);
                        }
                    }
                }
            }
            else {
                // other constraint icons - eg radius...
                constrIndices.clear();
                constrIndices.insert(i);
            }
            break;
        }
    }

    return constrIndices;
}

SbVec3s EditModeConstraintCoinManager::getDisplayedSize(const SoImage* iconPtr) const
{
    SbVec3s iconSize = iconPtr->image.getValue().getSize();

    if (iconPtr->width.getValue() != -1) {
        iconSize[0] = iconPtr->width.getValue();
    }
    if (iconPtr->height.getValue() != -1) {
        iconSize[1] = iconPtr->height.getValue();
    }
    return iconSize;
}

// public function that triggers drawing of most constraint icons
void EditModeConstraintCoinManager::drawConstraintIcons()
{
    auto geolistfacade = ViewProviderSketchCoinAttorney::getGeoListFacade(viewProvider);

    drawConstraintIcons(geolistfacade);
}

void EditModeConstraintCoinManager::drawConstraintIcons(const GeoListFacade& geolistfacade)
{
    const std::vector<Sketcher::Constraint*>& constraints =
        ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    std::vector<constrIconQueueItem> iconQueue;

    int maxNumberOfConstraints = std::min(editModeScenegraphNodes.constrGroup->getNumChildren(),
                                          static_cast<int>(constraints.size()));

    for (int constrId = 0; constrId < maxNumberOfConstraints; ++constrId) {
        Sketcher::Constraint* constraint = constraints[constrId];

        // Check if Icon Should be created
        bool multipleIcons = false;

        QString icoType = iconTypeFromConstraint(constraint);
        if (icoType.isEmpty()) {
            continue;
        }

        if (constraint->Type != vConstrType[constrId]) {
            break;
        }

        switch (constraint->Type) {

            case Tangent: {  // second icon is available only for colinear line segments
                const Part::Geometry* geo1 = geolistfacade.getGeometryFromGeoId(constraint->First);
                const Part::Geometry* geo2 = geolistfacade.getGeometryFromGeoId(constraint->Second);
                if (geo1 && geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() && geo2
                    && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    multipleIcons = true;
                }
            } break;
            case Horizontal:
            case Vertical: {  // second icon is available only for point alignment
                if (constraint->Second != GeoEnum::GeoUndef
                    && constraint->FirstPos != Sketcher::PointPos::none
                    && constraint->SecondPos != Sketcher::PointPos::none) {
                    multipleIcons = true;
                }
            } break;
            case Parallel:
                multipleIcons = true;
                break;
            case Perpendicular:
                // second icon is available only when there is no common point
                if (constraint->FirstPos == Sketcher::PointPos::none
                    && constraint->Third == GeoEnum::GeoUndef) {
                    multipleIcons = true;
                }
                break;
            case Equal:
                multipleIcons = true;
                break;
            default:
                break;
        }

        // Double-check that we can safely access the Inventor nodes
        if (constrId >= editModeScenegraphNodes.constrGroup->getNumChildren()) {
            Base::Console().DeveloperWarning(
                "EditModeConstraintManager",
                "Can't update constraint icons because view is not in sync with sketch\n");
            break;
        }

        // Find the Constraint Icon SoImage Node
        SoSeparator* sep =
            static_cast<SoSeparator*>(editModeScenegraphNodes.constrGroup->getChild(constrId));
        int numChildren = sep->getNumChildren();

        SbVec3f absPos;
        // Somewhat hacky - we use SoZoomTranslations for most types of icon,
        // but symmetry icons use SoTranslations...
        SoTranslation* translationPtr = static_cast<SoTranslation*>(
            sep->getChild(static_cast<int>(ConstraintNodePosition::FirstTranslationIndex)));

        if (dynamic_cast<SoZoomTranslation*>(translationPtr)) {
            absPos = static_cast<SoZoomTranslation*>(translationPtr)->abPos.getValue();
        }
        else {
            absPos = translationPtr->translation.getValue();
        }

        SoImage* coinIconPtr = dynamic_cast<SoImage*>(
            sep->getChild(static_cast<int>(ConstraintNodePosition::FirstIconIndex)));
        SoInfo* infoPtr = static_cast<SoInfo*>(
            sep->getChild(static_cast<int>(ConstraintNodePosition::FirstConstraintIdIndex)));

        constrIconQueueItem thisIcon;
        thisIcon.type = icoType;
        thisIcon.constraintId = constrId;
        thisIcon.position = absPos;
        thisIcon.destination = coinIconPtr;
        thisIcon.infoPtr = infoPtr;
        thisIcon.visible = constraint->isInVirtualSpace
            == ViewProviderSketchCoinAttorney::isShownVirtualSpace(viewProvider);

        if (constraint->Type == Symmetric) {
            Base::Vector3d startingpoint =
                geolistfacade.getPoint(constraint->First, constraint->FirstPos);
            Base::Vector3d endpoint =
                geolistfacade.getPoint(constraint->Second, constraint->SecondPos);

            SbVec3f pos0(startingpoint.x, startingpoint.y, startingpoint.z);
            SbVec3f pos1(endpoint.x, endpoint.y, endpoint.z);

            thisIcon.iconRotation =
                ViewProviderSketchCoinAttorney::getRotation(viewProvider, pos0, pos1);
        }
        else {
            thisIcon.iconRotation = 0;
        }

        if (multipleIcons) {
            if (constraint->Name.empty()) {
                thisIcon.label = QString::number(constrId + 1);
            }
            else {
                thisIcon.label = QString::fromUtf8(constraint->Name.c_str());
            }
            iconQueue.push_back(thisIcon);

            // Note that the second translation is meant to be applied after the first.
            // So, to get the position of the second icon, we add the two translations together
            //
            // See note ~30 lines up.
            if (numChildren > static_cast<int>(ConstraintNodePosition::SecondConstraintIdIndex)) {
                translationPtr = static_cast<SoTranslation*>(sep->getChild(
                    static_cast<int>(ConstraintNodePosition::SecondTranslationIndex)));
                if (dynamic_cast<SoZoomTranslation*>(translationPtr)) {
                    thisIcon.position +=
                        static_cast<SoZoomTranslation*>(translationPtr)->abPos.getValue();
                }
                else {
                    thisIcon.position += translationPtr->translation.getValue();
                }

                thisIcon.destination = dynamic_cast<SoImage*>(
                    sep->getChild(static_cast<int>(ConstraintNodePosition::SecondIconIndex)));
                thisIcon.infoPtr = static_cast<SoInfo*>(sep->getChild(
                    static_cast<int>(ConstraintNodePosition::SecondConstraintIdIndex)));
            }
        }
        else {
            if (constraint->Name.empty()) {
                thisIcon.label = QString();
            }
            else {
                thisIcon.label = QString::fromUtf8(constraint->Name.c_str());
            }
        }

        iconQueue.push_back(thisIcon);
    }

    combineConstraintIcons(iconQueue);
}

void EditModeConstraintCoinManager::combineConstraintIcons(IconQueue iconQueue)
{
    // getScaleFactor gives us a ratio of pixels per some kind of real units
    float maxDistSquared = pow(ViewProviderSketchCoinAttorney::getScaleFactor(viewProvider), 2);

    // There's room for optimisation here; we could reuse the combined icons...
    combinedConstrBoxes.clear();

    while (!iconQueue.empty()) {
        // A group starts with an item popped off the back of our initial queue
        IconQueue thisGroup;
        thisGroup.push_back(iconQueue.back());
        constrIconQueueItem init = iconQueue.back();
        iconQueue.pop_back();

        // we group only icons not being Symmetry icons, because we want those on the line
        // and only icons that are visible
        if (init.type != QString::fromLatin1("Constraint_Symmetric") && init.visible) {

            IconQueue::iterator i = iconQueue.begin();


            while (i != iconQueue.end()) {
                if ((*i).visible) {
                    bool addedToGroup = false;

                    for (IconQueue::iterator j = thisGroup.begin(); j != thisGroup.end(); ++j) {
                        float distSquared = pow(i->position[0] - j->position[0], 2)
                            + pow(i->position[1] - j->position[1], 2);
                        if (distSquared <= maxDistSquared
                            && (*i).type != QString::fromLatin1("Constraint_Symmetric")) {
                            // Found an icon in iconQueue that's close enough to
                            // a member of thisGroup, so move it into thisGroup
                            thisGroup.push_back(*i);
                            i = iconQueue.erase(i);
                            addedToGroup = true;
                            break;
                        }
                    }

                    if (addedToGroup) {
                        if (i == iconQueue.end()) {
                            // We just got the last icon out of iconQueue
                            break;
                        }
                        else {
                            // Start looking through the iconQueue again, in case
                            // we have an icon that's now close enough to thisGroup
                            i = iconQueue.begin();
                        }
                    }
                    else {
                        ++i;
                    }
                }
                else {  // if !visible we skip it
                    i++;
                }
            }
        }

        if (thisGroup.size() == 1) {
            drawTypicalConstraintIcon(thisGroup[0]);
        }
        else {
            drawMergedConstraintIcons(thisGroup);
        }
    }
}

void EditModeConstraintCoinManager::drawMergedConstraintIcons(IconQueue iconQueue)
{
    for (IconQueue::iterator i = iconQueue.begin(); i != iconQueue.end(); ++i) {
        clearCoinImage(i->destination);
    }

    QImage compositeIcon;
    SoImage* thisDest = iconQueue[0].destination;
    SoInfo* thisInfo = iconQueue[0].infoPtr;

    // Tracks all constraint IDs that are combined into this icon
    QString idString;
    int lastVPad = 0;

    QStringList labels;
    std::vector<int> ids;
    QString thisType;
    QColor iconColor;
    QList<QColor> labelColors;
    int maxColorPriority;
    double iconRotation;

    ConstrIconBBVec boundingBoxes;
    while (!iconQueue.empty()) {
        IconQueue::iterator i = iconQueue.begin();

        labels.clear();
        labels.append(i->label);

        ids.clear();
        ids.push_back(i->constraintId);

        thisType = i->type;
        iconColor = constrColor(i->constraintId);
        labelColors.clear();
        labelColors.append(iconColor);
        iconRotation = i->iconRotation;

        maxColorPriority = constrColorPriority(i->constraintId);

        if (idString.length()) {
            idString.append(QString::fromLatin1(","));
        }
        idString.append(QString::number(i->constraintId));

        i = iconQueue.erase(i);
        while (i != iconQueue.end()) {
            if (i->type != thisType) {
                ++i;
                continue;
            }

            labels.append(i->label);
            ids.push_back(i->constraintId);
            labelColors.append(constrColor(i->constraintId));

            if (constrColorPriority(i->constraintId) > maxColorPriority) {
                maxColorPriority = constrColorPriority(i->constraintId);
                iconColor = constrColor(i->constraintId);
            }

            idString.append(QString::fromLatin1(",") + QString::number(i->constraintId));

            i = iconQueue.erase(i);
        }

        // To be inserted into edit->combinedConstBoxes
        std::vector<QRect> boundingBoxesVec;
        int oldHeight = 0;

        // Render the icon here.
        if (compositeIcon.isNull()) {
            compositeIcon = renderConstrIcon(thisType,
                                             iconColor,
                                             labels,
                                             labelColors,
                                             iconRotation,
                                             &boundingBoxesVec,
                                             &lastVPad);
        }
        else {
            int thisVPad;
            QImage partialIcon = renderConstrIcon(thisType,
                                                  iconColor,
                                                  labels,
                                                  labelColors,
                                                  iconRotation,
                                                  &boundingBoxesVec,
                                                  &thisVPad);

            // Stack vertically for now.  Down the road, it might make sense
            // to figure out the best orientation automatically.
            oldHeight = compositeIcon.height();

            // This is overkill for the currently used (20 July 2014) font,
            // since it always seems to have the same vertical pad, but this
            // might not always be the case.  The 3 pixel buffer might need
            // to vary depending on font size too...
            oldHeight -= std::max(lastVPad - 3, 0);

            compositeIcon = compositeIcon.copy(0,
                                               0,
                                               std::max(partialIcon.width(), compositeIcon.width()),
                                               partialIcon.height() + compositeIcon.height());

            QPainter qp(&compositeIcon);
            qp.drawImage(0, oldHeight, partialIcon);

            lastVPad = thisVPad;
        }

        // Add bounding boxes for the icon we just rendered to boundingBoxes
        std::vector<int>::iterator id = ids.begin();
        std::set<int> nextIds;
        for (std::vector<QRect>::iterator bb = boundingBoxesVec.begin();
             bb != boundingBoxesVec.end();
             ++bb) {
            nextIds.clear();

            if (bb == boundingBoxesVec.begin()) {
                // The first bounding box is for the icon at left, so assign
                // all IDs for that type of constraint to the icon.
                for (std::vector<int>::iterator j = ids.begin(); j != ids.end(); ++j) {
                    nextIds.insert(*j);
                }
            }
            else {
                nextIds.insert(*(id++));
            }

            ConstrIconBB newBB(bb->adjusted(0, oldHeight, 0, oldHeight), nextIds);

            boundingBoxes.push_back(newBB);
        }
    }

    combinedConstrBoxes[idString] = boundingBoxes;
    thisInfo->string.setValue(idString.toLatin1().data());
    sendConstraintIconToCoin(compositeIcon, thisDest);
}


/// Note: labels, labelColors, and boundingBoxes are all
/// assumed to be the same length.
QImage EditModeConstraintCoinManager::renderConstrIcon(const QString& type,
                                                       const QColor& iconColor,
                                                       const QStringList& labels,
                                                       const QList<QColor>& labelColors,
                                                       double iconRotation,
                                                       std::vector<QRect>* boundingBoxes,
                                                       int* vPad)
{
    // Constants to help create constraint icons
    QString joinStr = QString::fromLatin1(", ");

    QPixmap pxMap;
    std::stringstream constraintName;
    constraintName << type.toLatin1().data()
                   << drawingParameters.constraintIconSize;  // allow resizing by embedding size
    if (!Gui::BitmapFactory().findPixmapInCache(constraintName.str().c_str(), pxMap)) {
        pxMap = Gui::BitmapFactory().pixmapFromSvg(
            type.toLatin1().data(),
            QSizeF(drawingParameters.constraintIconSize, drawingParameters.constraintIconSize));
        Gui::BitmapFactory().addPixmapToCache(constraintName.str().c_str(),
                                              pxMap);  // Cache for speed, avoiding pixmapFromSvg
    }
    QImage icon = pxMap.toImage();

    QFont font = ViewProviderSketchCoinAttorney::getApplicationFont(viewProvider);
    font.setPixelSize(static_cast<int>(1.0 * drawingParameters.constraintIconSize));
    font.setBold(true);
    QFontMetrics qfm = QFontMetrics(font);

    int labelWidth = qfm.boundingRect(labels.join(joinStr)).width();
    // See Qt docs on qRect::bottom() for explanation of the +1
    int pxBelowBase = qfm.boundingRect(labels.join(joinStr)).bottom() + 1;

    if (vPad) {
        *vPad = pxBelowBase;
    }

    QTransform rotation;
    rotation.rotate(iconRotation);

    QImage roticon = icon.transformed(rotation);
    QImage image = roticon.copy(0, 0, roticon.width() + labelWidth, roticon.height() + pxBelowBase);

    // Make a bounding box for the icon
    if (boundingBoxes) {
        boundingBoxes->push_back(QRect(0, 0, roticon.width(), roticon.height()));
    }

    // Render the Icons
    QPainter qp(&image);
    qp.setCompositionMode(QPainter::CompositionMode_SourceIn);
    qp.fillRect(roticon.rect(), iconColor);

    // Render constraint label if necessary
    if (!labels.join(QString()).isEmpty()) {
        qp.setCompositionMode(QPainter::CompositionMode_SourceOver);
        qp.setFont(font);

        int cursorOffset = 0;

        // In Python: "for label, color in zip(labels, labelColors):"
        QStringList::const_iterator labelItr;
        QString labelStr;
        QList<QColor>::const_iterator colorItr;
        QRect labelBB;
        for (labelItr = labels.begin(), colorItr = labelColors.begin();
             labelItr != labels.end() && colorItr != labelColors.end();
             ++labelItr, ++colorItr) {

            qp.setPen(*colorItr);

            if (labelItr + 1 == labels.end()) {  // if this is the last label
                labelStr = *labelItr;
            }
            else {
                labelStr = *labelItr + joinStr;
            }

            // Note: text can sometimes draw to the left of the starting
            //       position, eg italic fonts.  Check QFontMetrics
            //       documentation for more info, but be mindful if the
            //       icon.width() is ever very small (or removed).
            qp.drawText(icon.width() + cursorOffset, icon.height(), labelStr);

            if (boundingBoxes) {
                labelBB = qfm.boundingRect(labelStr);
                labelBB.moveTo(icon.width() + cursorOffset,
                               icon.height() - qfm.height() + pxBelowBase);
                boundingBoxes->push_back(labelBB);
            }

            cursorOffset += Gui::QtTools::horizontalAdvance(qfm, labelStr);
        }
    }

    return image;
}

void EditModeConstraintCoinManager::drawTypicalConstraintIcon(const constrIconQueueItem& i)
{
    QColor color = constrColor(i.constraintId);

    QImage image = renderConstrIcon(i.type,
                                    color,
                                    QStringList(i.label),
                                    QList<QColor>() << color,
                                    i.iconRotation);

    i.infoPtr->string.setValue(QString::number(i.constraintId).toLatin1().data());
    sendConstraintIconToCoin(image, i.destination);
}

QString EditModeConstraintCoinManager::iconTypeFromConstraint(Constraint* constraint)
{
    /*! TODO: Consider pushing this functionality up into Constraint
     *
     Abdullah: Please, don't. An icon is visualisation information and
     does not belong in App, but in Gui. Rather consider refactoring it
     in a separate class dealing with visualisation of constraints.*/

    switch (constraint->Type) {
        case Horizontal:
            return QString::fromLatin1("Constraint_Horizontal");
        case Vertical:
            return QString::fromLatin1("Constraint_Vertical");
        case PointOnObject:
            return QString::fromLatin1("Constraint_PointOnObject");
        case Tangent:
            return QString::fromLatin1("Constraint_Tangent");
        case Parallel:
            return QString::fromLatin1("Constraint_Parallel");
        case Perpendicular:
            return QString::fromLatin1("Constraint_Perpendicular");
        case Equal:
            return QString::fromLatin1("Constraint_EqualLength");
        case Symmetric:
            return QString::fromLatin1("Constraint_Symmetric");
        case SnellsLaw:
            return QString::fromLatin1("Constraint_SnellsLaw");
        case Block:
            return QString::fromLatin1("Constraint_Block");
        default:
            return QString();
    }
}

void EditModeConstraintCoinManager::sendConstraintIconToCoin(const QImage& icon,
                                                             SoImage* soImagePtr)
{
    SoSFImage icondata = SoSFImage();

    Gui::BitmapFactory().convert(icon, icondata);

    SbVec2s iconSize(icon.width(), icon.height());

    int four = 4;
    soImagePtr->image.setValue(iconSize, 4, icondata.getValue(iconSize, four));

    // Set Image Alignment to Center
    soImagePtr->vertAlignment = SoImage::HALF;
    soImagePtr->horAlignment = SoImage::CENTER;
}

void EditModeConstraintCoinManager::clearCoinImage(SoImage* soImagePtr)
{
    soImagePtr->setToDefaults();
}

QColor EditModeConstraintCoinManager::constrColor(int constraintId)
{
    auto toQColor = [](auto sbcolor) -> QColor {
        return QColor((int)(sbcolor[0] * 255.0f),
                      (int)(sbcolor[1] * 255.0f),
                      (int)(sbcolor[2] * 255.0f));
    };

    const auto constraints = ViewProviderSketchCoinAttorney::getConstraints(viewProvider);

    if (ViewProviderSketchCoinAttorney::isConstraintPreselected(viewProvider, constraintId)) {
        return toQColor(drawingParameters.PreselectColor);
    }
    else if (ViewProviderSketchCoinAttorney::isConstraintSelected(viewProvider, constraintId)) {
        return toQColor(drawingParameters.SelectColor);
    }
    else if (!constraints[constraintId]->isActive) {
        return toQColor(drawingParameters.DeactivatedConstrDimColor);
    }
    else if (!constraints[constraintId]->isDriving) {
        return toQColor(drawingParameters.NonDrivingConstrDimColor);
    }
    else {
        return toQColor(drawingParameters.ConstrIcoColor);
    }
}

int EditModeConstraintCoinManager::constrColorPriority(int constraintId)
{
    if (ViewProviderSketchCoinAttorney::isConstraintPreselected(viewProvider, constraintId)) {
        return 3;
    }
    else if (ViewProviderSketchCoinAttorney::isConstraintSelected(viewProvider, constraintId)) {
        return 2;
    }
    else {
        return 1;
    }
}

SoSeparator* EditModeConstraintCoinManager::getConstraintIdSeparator(int i)
{
    return dynamic_cast<SoSeparator*>(editModeScenegraphNodes.constrGroup->getChild(i));
}

void EditModeConstraintCoinManager::createEditModeInventorNodes()
{
    // group node for the Constraint visual +++++++++++++++++++++++++++++++++++
    SoMaterialBinding* MtlBind = new SoMaterialBinding;
    MtlBind->setName("ConstraintMaterialBinding");
    MtlBind->value = SoMaterialBinding::OVERALL;
    editModeScenegraphNodes.EditRoot->addChild(MtlBind);

    // use small line width for the Constraints
    editModeScenegraphNodes.ConstraintDrawStyle = new SoDrawStyle;
    editModeScenegraphNodes.ConstraintDrawStyle->setName("ConstraintDrawStyle");
    editModeScenegraphNodes.ConstraintDrawStyle->lineWidth =
        1 * drawingParameters.pixelScalingFactor;
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.ConstraintDrawStyle);

    // add the group where all the constraints has its SoSeparator
    editModeScenegraphNodes.constrGrpSelect =
        new SoPickStyle();  // used to toggle constraints selectability
    editModeScenegraphNodes.constrGrpSelect->style.setValue(SoPickStyle::SHAPE);
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.constrGrpSelect);
    setConstraintSelectability();  // Ensure default value;

    editModeScenegraphNodes.constrGroup = new SmSwitchboard();
    editModeScenegraphNodes.constrGroup->setName("ConstraintGroup");
    editModeScenegraphNodes.EditRoot->addChild(editModeScenegraphNodes.constrGroup);

    SoPickStyle* ps = new SoPickStyle();  // used to following nodes aren't impacted
    ps->style.setValue(SoPickStyle::SHAPE);
    editModeScenegraphNodes.EditRoot->addChild(ps);
}
