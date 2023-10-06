/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <Precision.hxx>
#include <QPainter>
#include <cfloat>
#endif

#include <App/Application.h>
#include <Base/Tools.h>
#include <Base/Tools2D.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/DlgCheckableMessageBox.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Notifications.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "CommandConstraints.h"
#include "DrawSketchHandler.h"
#include "EditDatumDialog.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "ui_InsertDatum.h"
#include <Inventor/events/SoKeyboardEvent.h>

// Remove this after pre-commit hook is activated
// clang-format off
using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

/***** Creation Mode ************/
namespace SketcherGui
{
enum ConstraintCreationMode
{
    Driving,
    Reference
};
}

ConstraintCreationMode constraintCreationMode = Driving;

bool isCreateConstraintActive(Gui::Document* doc)
{
    if (doc) {
        // checks if a Sketch View provider is in Edit and is in no special mode
        if (doc->getInEdit()
            && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            if (static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())->getSketchMode()
                == ViewProviderSketch::STATUS_NONE) {
                if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId())
                    > 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Utility method to avoid repeating the same code over and over again
void finishDatumConstraint(Gui::Command* cmd,
                           Sketcher::SketchObject* sketch,
                           bool isDriving = true,
                           unsigned int numberofconstraints = 1)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");

    // Get the latest constraint
    const std::vector<Sketcher::Constraint*>& ConStr = sketch->Constraints.getValues();
    auto lastConstraintIndex = ConStr.size() - 1;
    Sketcher::Constraint* constr = ConStr[lastConstraintIndex];
    auto lastConstraintType = constr->Type;

    // Guess some reasonable distance for placing the datum text
    Gui::Document* doc = cmd->getActiveGuiDocument();
    float scaleFactor = 1.0;
    double labelPosition = 0.0;
    float labelPositionRandomness = 0.0;

    if (lastConstraintType == Radius || lastConstraintType == Diameter) {
        labelPosition = hGrp->GetFloat("RadiusDiameterConstraintDisplayBaseAngle", 15.0)
            * (M_PI / 180);// Get radius/diameter constraint display angle
        labelPositionRandomness =
            hGrp->GetFloat("RadiusDiameterConstraintDisplayAngleRandomness", 0.0)
            * (M_PI / 180);// Get randomness

        // Adds a random value around the base angle, so that possibly overlapping labels get likely
        // a different position.
        if (labelPositionRandomness != 0.0) {
            labelPosition = labelPosition
                + labelPositionRandomness
                    * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5);
        }
    }

    if (doc && doc->getInEdit()
        && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
        SketcherGui::ViewProviderSketch* vp =
            static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        scaleFactor = vp->getScaleFactor();

        int firstConstraintIndex = lastConstraintIndex - numberofconstraints + 1;

        for (int i = lastConstraintIndex; i >= firstConstraintIndex; i--) {
            ConStr[i]->LabelDistance = 2. * scaleFactor;

            if (lastConstraintType == Radius || lastConstraintType == Diameter) {
                const Part::Geometry* geo = sketch->getGeometry(ConStr[i]->First);

                if (geo && isCircle(*geo)) {
                    ConStr[i]->LabelPosition = labelPosition;
                }
            }
        }
        vp->draw(false, false);// Redraw
    }

    bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);

    // Ask for the value of the distance immediately
    if (show && isDriving) {
        EditDatumDialog editDatumDialog(sketch, ConStr.size() - 1);
        editDatumDialog.exec();
    }
    else {
        // no dialog was shown so commit the command
        cmd->commitCommand();
    }

    tryAutoRecompute(sketch);
    cmd->getSelection().clearSelection();
}

void showNoConstraintBetweenExternal(const App::DocumentObject* obj)
{
    Gui::TranslatedUserWarning(
        obj,
        QObject::tr("Wrong selection"),
        QObject::tr("Cannot add a constraint between two external geometries."));
}

void showNoConstraintBetweenFixedGeometry(const App::DocumentObject* obj)
{
    Gui::TranslatedUserWarning(obj,
                               QObject::tr("Wrong selection"),
                               QObject::tr("Cannot add a constraint between two fixed geometries. "
                                           "Fixed geometries include external geometry, "
                                           "blocked geometry, and special points "
                                           "such as B-spline knot points."));
}

bool isGeoConcentricCompatible(const Part::Geometry* geo)
{
    return (isEllipse(*geo) || isArcOfEllipse(*geo) || isCircle(*geo) || isArcOfCircle(*geo));
}

/// Makes an angle constraint between 2 lines
void SketcherGui::makeAngleBetweenTwoLines(Sketcher::SketchObject* Obj,
    Gui::Command* cmd,
    int geoId1,
    int geoId2)
{
    Sketcher::PointPos posId1 = Sketcher::PointPos::none;
    Sketcher::PointPos posId2 = Sketcher::PointPos::none;
    double actAngle;

    if (!calculateAngle(Obj, geoId1, geoId2, posId1, posId2, actAngle)) {
        return;
    }

    if (actAngle == 0.0) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Parallel lines"),
            QObject::tr("An angle constraint cannot be set for two parallel lines."));

        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add angle constraint"));
    Gui::cmdAppObjectArgs(Obj,
        "addConstraint(Sketcher.Constraint('Angle',%d,%d,%d,%d,%f))",
        geoId1,
        static_cast<int>(posId1),
        geoId2,
        static_cast<int>(posId2),
        actAngle);

    if (areBothPointsOrSegmentsFixed(Obj, geoId1, geoId2)
        || constraintCreationMode == Reference) {
        // it is a constraint on a external line, make it non-driving
        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

        Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
        finishDatumConstraint(cmd, Obj, false);
    }
    else {
        finishDatumConstraint(cmd, Obj, true);
    }
}



bool SketcherGui::calculateAngle(Sketcher::SketchObject* Obj, int& GeoId1, int& GeoId2, Sketcher::PointPos& PosId1, Sketcher::PointPos& PosId2, double& ActAngle)
{
    const Part::Geometry* geom1 = Obj->getGeometry(GeoId1);
    const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

    if (!(geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) ||
        !(geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId())) {
        return false;
    }

    const Part::GeomLineSegment* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geom1);
    const Part::GeomLineSegment* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geom2);

    // find the two closest line ends
    Base::Vector3d p1[2], p2[2];
    p1[0] = lineSeg1->getStartPoint();
    p1[1] = lineSeg1->getEndPoint();
    p2[0] = lineSeg2->getStartPoint();
    p2[1] = lineSeg2->getEndPoint();

    // Get the intersection point in 2d of the two lines if possible
    Base::Line2d line1(Base::Vector2d(p1[0].x, p1[0].y), Base::Vector2d(p1[1].x, p1[1].y));
    Base::Line2d line2(Base::Vector2d(p2[0].x, p2[0].y), Base::Vector2d(p2[1].x, p2[1].y));
    Base::Vector2d s;
    if (line1.Intersect(line2, s)) {
        // get the end points of the line segments that are closest to the intersection point
        Base::Vector3d s3d(s.x, s.y, p1[0].z);
        if (Base::DistanceP2(s3d, p1[0]) < Base::DistanceP2(s3d, p1[1]))
            PosId1 = Sketcher::PointPos::start;
        else
            PosId1 = Sketcher::PointPos::end;
        if (Base::DistanceP2(s3d, p2[0]) < Base::DistanceP2(s3d, p2[1]))
            PosId2 = Sketcher::PointPos::start;
        else
            PosId2 = Sketcher::PointPos::end;
    }
    else {
        // if all points are collinear
        double length = DBL_MAX;
        for (int i = 0; i <= 1; i++) {
            for (int j = 0; j <= 1; j++) {
                double tmp = Base::DistanceP2(p2[j], p1[i]);
                if (tmp < length) {
                    length = tmp;
                    PosId1 = i ? Sketcher::PointPos::end : Sketcher::PointPos::start;
                    PosId2 = j ? Sketcher::PointPos::end : Sketcher::PointPos::start;
                }
            }
        }
    }

    Base::Vector3d dir1 = ((PosId1 == Sketcher::PointPos::start) ? 1. : -1.) *
        (lineSeg1->getEndPoint() - lineSeg1->getStartPoint());
    Base::Vector3d dir2 = ((PosId2 == Sketcher::PointPos::start) ? 1. : -1.) *
        (lineSeg2->getEndPoint() - lineSeg2->getStartPoint());

    // check if the two lines are parallel
    Base::Vector3d dir3 = dir1 % dir2;
    if (dir3.Length() < Precision::Intersection()) {
        Base::Vector3d dist = (p1[0] - p2[0]) % dir1;
        if (dist.Sqr() > Precision::Intersection()) {
            ActAngle = 0.0;
            return true;
        }
    }

    ActAngle = atan2(dir1.x * dir2.y - dir1.y * dir2.x,
        dir1.y * dir2.y + dir1.x * dir2.x);

    if (ActAngle < 0) {
        ActAngle *= -1;
        std::swap(GeoId1, GeoId2);
        std::swap(PosId1, PosId2);
    }

    return true;
}


/// Makes a simple tangency constraint using extra point + tangent via point
/// ellipse => an ellipse
/// geom2 => any of an ellipse, an arc of ellipse, a circle, or an arc (of circle)
/// geoId1 => geoid of the ellipse
/// geoId2 => geoid of geom2
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void SketcherGui::makeTangentToEllipseviaNewPoint(Sketcher::SketchObject* Obj,
                                                  const Part::GeomEllipse* ellipse,
                                                  const Part::Geometry* geom2,
                                                  int geoId1,
                                                  int geoId2)
{

    Base::Vector3d center = ellipse->getCenter();
    double majord = ellipse->getMajorRadius();
    double minord = ellipse->getMinorRadius();
    double phi = atan2(ellipse->getMajorAxisDir().y, ellipse->getMajorAxisDir().x);

    Base::Vector3d center2;

    if (isEllipse(*geom2)) {
        center2 = (static_cast<const Part::GeomEllipse*>(geom2))->getCenter();
    }
    else if (isArcOfEllipse(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfEllipse*>(geom2))->getCenter();
    }
    else if (isCircle(*geom2)) {
        center2 = (static_cast<const Part::GeomCircle*>(geom2))->getCenter();
    }
    else if (isArcOfCircle(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfCircle*>(geom2))->getCenter();
    }

    Base::Vector3d direction = center2 - center;
    double tapprox =
        atan2(direction.y, direction.x) - phi;// we approximate the eccentric anomaly by the polar

    Base::Vector3d PoE = Base::Vector3d(
        center.x + majord * cos(tapprox) * cos(phi) - minord * sin(tapprox) * sin(phi),
        center.y + majord * cos(tapprox) * sin(phi) + minord * sin(tapprox) * cos(phi),
        0);

    try {
        // Add a point
        Gui::cmdAppObjectArgs(Obj, "addGeometry(Part.Point(App.Vector(%f,%f,0)))", PoE.x, PoE.y);
        int GeoIdPoint = Obj->getHighestCurveIndex();

        // Point on first object
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start),
                              geoId1);// constrain major axis
        // Point on second object
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start),
                              geoId2);// constrain major axis
        // tangent via point
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d))",
                              geoId1,
                              geoId2,
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start));
    }
    catch (const Base::Exception& e) {
        Gui::NotifyUserError(Obj,
                             QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                             e.what());
        Gui::Command::abortCommand();

        tryAutoRecompute(Obj);
        return;
    }

    Gui::Command::commitCommand();
    tryAutoRecompute(Obj);
}

/// Makes a simple tangency constraint using extra point + tangent via point
/// aoe => an arc of ellipse
/// geom2 => any of an arc of ellipse, a circle, or an arc (of circle)
/// geoId1 => geoid of the arc of ellipse
/// geoId2 => geoid of geom2
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void SketcherGui::makeTangentToArcOfEllipseviaNewPoint(Sketcher::SketchObject* Obj,
                                                       const Part::GeomArcOfEllipse* aoe,
                                                       const Part::Geometry* geom2,
                                                       int geoId1,
                                                       int geoId2)
{

    Base::Vector3d center = aoe->getCenter();
    double majord = aoe->getMajorRadius();
    double minord = aoe->getMinorRadius();
    double phi = atan2(aoe->getMajorAxisDir().y, aoe->getMajorAxisDir().x);

    Base::Vector3d center2;

    if (isArcOfEllipse(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfEllipse*>(geom2))->getCenter();
    }
    else if (isCircle(*geom2)) {
        center2 = (static_cast<const Part::GeomCircle*>(geom2))->getCenter();
    }
    else if (isArcOfCircle(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfCircle*>(geom2))->getCenter();
    }

    Base::Vector3d direction = center2 - center;
    double tapprox =
        atan2(direction.y, direction.x) - phi;// we approximate the eccentric anomaly by the polar

    Base::Vector3d PoE = Base::Vector3d(
        center.x + majord * cos(tapprox) * cos(phi) - minord * sin(tapprox) * sin(phi),
        center.y + majord * cos(tapprox) * sin(phi) + minord * sin(tapprox) * cos(phi),
        0);

    try {
        // Add a point
        Gui::cmdAppObjectArgs(Obj, "addGeometry(Part.Point(App.Vector(%f,%f,0)))", PoE.x, PoE.y);
        int GeoIdPoint = Obj->getHighestCurveIndex();

        // Point on first object
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start),
                              geoId1);// constrain major axis
        // Point on second object
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start),
                              geoId2);// constrain major axis
        // tangent via point
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d))",
                              geoId1,
                              geoId2,
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start));
    }
    catch (const Base::Exception& e) {
        Gui::NotifyUserError(Obj,
                             QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                             e.what());
        Gui::Command::abortCommand();

        tryAutoRecompute(Obj);
        return;
    }

    Gui::Command::commitCommand();
    tryAutoRecompute(Obj);
}

/// Makes a simple tangency constraint using extra point + tangent via point
/// aoh => an arc of hyperbola
/// geom2 => any of an arc of hyperbola, an arc of ellipse, a circle, or an arc (of circle)
/// geoId1 => geoid of the arc of hyperbola
/// geoId2 => geoid of geom2
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void SketcherGui::makeTangentToArcOfHyperbolaviaNewPoint(Sketcher::SketchObject* Obj,
                                                         const Part::GeomArcOfHyperbola* aoh,
                                                         const Part::Geometry* geom2,
                                                         int geoId1,
                                                         int geoId2)
{

    Base::Vector3d center = aoh->getCenter();
    double majord = aoh->getMajorRadius();
    double minord = aoh->getMinorRadius();
    Base::Vector3d dirmaj = aoh->getMajorAxisDir();
    double phi = atan2(dirmaj.y, dirmaj.x);
    double df = sqrt(majord * majord + minord * minord);
    Base::Vector3d focus = center + df * dirmaj;// positive focus

    Base::Vector3d center2;

    if (isArcOfHyperbola(*geom2)) {
        auto aoh2 = static_cast<const Part::GeomArcOfHyperbola*>(geom2);
        Base::Vector3d dirmaj2 = aoh2->getMajorAxisDir();
        double majord2 = aoh2->getMajorRadius();
        double minord2 = aoh2->getMinorRadius();
        double df2 = sqrt(majord2 * majord2 + minord2 * minord2);
        center2 = aoh2->getCenter() + df2 * dirmaj2;// positive focus
    }
    else if (isArcOfEllipse(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfEllipse*>(geom2))->getCenter();
    }
    else if (isEllipse(*geom2)) {
        center2 = (static_cast<const Part::GeomEllipse*>(geom2))->getCenter();
    }
    else if (isCircle(*geom2)) {
        center2 = (static_cast<const Part::GeomCircle*>(geom2))->getCenter();
    }
    else if (isArcOfCircle(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfCircle*>(geom2))->getCenter();
    }
    else if (isLineSegment(*geom2)) {
        auto l2 = static_cast<const Part::GeomLineSegment*>(geom2);
        center2 = (l2->getStartPoint() + l2->getEndPoint()) / 2;
    }

    Base::Vector3d direction = center2 - focus;
    double tapprox = atan2(direction.y, direction.x) - phi;

    Base::Vector3d PoH = Base::Vector3d(
        center.x + majord * cosh(tapprox) * cos(phi) - minord * sinh(tapprox) * sin(phi),
        center.y + majord * cosh(tapprox) * sin(phi) + minord * sinh(tapprox) * cos(phi),
        0);

    try {
        // Add a point
        Gui::cmdAppObjectArgs(Obj, "addGeometry(Part.Point(App.Vector(%f,%f,0)))", PoH.x, PoH.y);
        int GeoIdPoint = Obj->getHighestCurveIndex();

        // Point on first object
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start),
                              geoId1);// constrain major axis
        // Point on second object
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start),
                              geoId2);// constrain major axis
        // tangent via point
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d))",
                              geoId1,
                              geoId2,
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start));
    }
    catch (const Base::Exception& e) {
        Gui::NotifyUserError(Obj,
                             QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                             e.what());
        Gui::Command::abortCommand();

        tryAutoRecompute(Obj);
        return;
    }

    Gui::Command::commitCommand();

    tryAutoRecompute(Obj);
}

/// Makes a simple tangency constraint using extra point + tangent via point
/// aop => an arc of parabola
/// geom2 => any of an arc of parabola, an arc of hyperbola an arc of ellipse, a circle, or an arc
/// (of circle) geoId1 => geoid of the arc of parabola geoId2 => geoid of geom2 NOTE: A command must
/// be opened before calling this function, which this function commits or aborts as appropriate.
/// The reason is for compatibility reasons with other code e.g. "Autoconstraints" in
/// DrawSketchHandler.cpp
void SketcherGui::makeTangentToArcOfParabolaviaNewPoint(Sketcher::SketchObject* Obj,
                                                        const Part::GeomArcOfParabola* aop,
                                                        const Part::Geometry* geom2,
                                                        int geoId1,
                                                        int geoId2)
{

    Base::Vector3d focus = aop->getFocus();

    Base::Vector3d center2;

    if (isArcOfParabola(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfParabola*>(geom2))->getFocus();
    }
    else if (isArcOfHyperbola(*geom2)) {
        auto aoh2 = static_cast<const Part::GeomArcOfHyperbola*>(geom2);
        Base::Vector3d dirmaj2 = aoh2->getMajorAxisDir();
        double majord2 = aoh2->getMajorRadius();
        double minord2 = aoh2->getMinorRadius();
        double df2 = sqrt(majord2 * majord2 + minord2 * minord2);
        center2 = aoh2->getCenter() + df2 * dirmaj2;// positive focus
    }
    else if (isArcOfEllipse(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfEllipse*>(geom2))->getCenter();
    }
    else if (isEllipse(*geom2)) {
        center2 = (static_cast<const Part::GeomEllipse*>(geom2))->getCenter();
    }
    else if (isCircle(*geom2)) {
        center2 = (static_cast<const Part::GeomCircle*>(geom2))->getCenter();
    }
    else if (isArcOfCircle(*geom2)) {
        center2 = (static_cast<const Part::GeomArcOfCircle*>(geom2))->getCenter();
    }
    else if (isLineSegment(*geom2)) {
        auto l2 = static_cast<const Part::GeomLineSegment*>(geom2);
        center2 = (l2->getStartPoint() + l2->getEndPoint()) / 2;
    }

    Base::Vector3d direction = center2 - focus;

    Base::Vector3d PoP = focus + direction / 2;

    try {
        // Add a point
        Gui::cmdAppObjectArgs(Obj, "addGeometry(Part.Point(App.Vector(%f,%f,0)))", PoP.x, PoP.y);
        int GeoIdPoint = Obj->getHighestCurveIndex();

        // Point on first object
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start),
                              geoId1);// constrain major axis
        // Point on second object
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start),
                              geoId2);// constrain major axis
        // tangent via point
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d))",
                              geoId1,
                              geoId2,
                              GeoIdPoint,
                              static_cast<int>(Sketcher::PointPos::start));
    }
    catch (const Base::Exception& e) {
        Gui::NotifyUserError(Obj,
                             QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                             e.what());

        Gui::Command::abortCommand();

        tryAutoRecompute(Obj);
        return;
    }

    Gui::Command::commitCommand();
    tryAutoRecompute(Obj);
}

void SketcherGui::doEndpointTangency(Sketcher::SketchObject* Obj,
                                     int GeoId1,
                                     int GeoId2,
                                     PointPos PosId1,
                                     PointPos PosId2)
{
    // This code supports simple B-spline endpoint tangency to any other geometric curve
    const Part::Geometry* geom1 = Obj->getGeometry(GeoId1);
    const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

    if (geom1 && geom2 && (isBSplineCurve(*geom1) || isBSplineCurve(*geom2))) {
        if (! isBSplineCurve(*geom1)) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
        }
        // GeoId1 is the B-spline now
    }// end of code supports simple B-spline endpoint tangency

    Gui::cmdAppObjectArgs(Obj,
                          "addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d,%d))",
                          GeoId1,
                          static_cast<int>(PosId1),
                          GeoId2,
                          static_cast<int>(PosId2));
}

void SketcherGui::doEndpointToEdgeTangency(Sketcher::SketchObject* Obj,
                                           int GeoId1,
                                           PointPos PosId1,
                                           int GeoId2)
{
    Gui::cmdAppObjectArgs(Obj,
                          "addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d))",
                          GeoId1,
                          static_cast<int>(PosId1),
                          GeoId2);
}

void SketcherGui::notifyConstraintSubstitutions(const QString& message)
{
    Gui::Dialog::DlgCheckableMessageBox::showMessage(
        QObject::tr("Sketcher Constraint Substitution"),
        message,
        QLatin1String("User parameter:BaseApp/Preferences/Mod/Sketcher/General"),
        QLatin1String("NotifyConstraintSubstitutions"),
        true,// Default ParamEntry
        true,// checkbox state
        QObject::tr("Keep notifying me of constraint substitutions"));
}

// returns true if successful, false otherwise
bool addConstraintSafely(SketchObject* obj, std::function<void()> constraintadditionfunction)
{
    try {
        constraintadditionfunction();
    }
    catch (const Base::IndexError& e) {
        // Exceptions originating in Python have already been reported by the Interpreter as
        // Untranslated developer intended messages (i.e. to the Report View)
        // Example of exception carrying a static string with a pair in the "Notifications" context
        Gui::NotifyUserError(obj,
                             QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                             e.what());

        Gui::Command::abortCommand();

        tryAutoRecompute(obj);
        return false;
    }
    catch (const Base::Exception&) {
        Gui::TranslatedUserError(
            obj,
            QObject::tr("Error"),
            QObject::tr("Unexpected error. More information may be available in the Report View."));

        Gui::Command::abortCommand();

        tryAutoRecompute(obj);
        return false;
    }

    return true;
}

namespace SketcherGui
{

struct SelIdPair
{
    int GeoId;
    Sketcher::PointPos PosId;
};

struct SketchSelection
{
    enum GeoType
    {
        Point,
        Line,
        Circle,
        Arc
    };
    int setUp();
    struct SketchSelectionItem
    {
        GeoType type;
        int GeoId;
        bool Extern;
    };
    std::list<SketchSelectionItem> Items;
    QString ErrorMsg;
};

int SketchSelection::setUp()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    Sketcher::SketchObject* SketchObj = nullptr;
    std::vector<std::string> SketchSubNames;
    std::vector<std::string> SupportSubNames;

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() == 1) {
        // if one selectetd, only sketch allowed. should be done by activity of command
        if (!selection[0].getObject()->getTypeId().isDerivedFrom(
                Sketcher::SketchObject::getClassTypeId())) {
            ErrorMsg = QObject::tr("Only sketch and its support are allowed to be selected.");
            return -1;
        }

        SketchObj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
        SketchSubNames = selection[0].getSubNames();
    }
    else if (selection.size() == 2) {
        if (selection[0].getObject()->getTypeId().isDerivedFrom(
                Sketcher::SketchObject::getClassTypeId())) {
            SketchObj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
            // check if the none sketch object is the support of the sketch
            if (selection[1].getObject() != SketchObj->Support.getValue()) {
                ErrorMsg = QObject::tr("Only sketch and its support are allowed to be selected.");
                return -1;
            }
            // assume always a Part::Feature derived object as support
            assert(selection[1].getObject()->getTypeId().isDerivedFrom(
                Part::Feature::getClassTypeId()));
            SketchSubNames = selection[0].getSubNames();
            SupportSubNames = selection[1].getSubNames();
        }
        else if (selection[1].getObject()->getTypeId().isDerivedFrom(
                     Sketcher::SketchObject::getClassTypeId())) {
            SketchObj = static_cast<Sketcher::SketchObject*>(selection[1].getObject());
            // check if the none sketch object is the support of the sketch
            if (selection[0].getObject() != SketchObj->Support.getValue()) {
                ErrorMsg = QObject::tr("Only sketch and its support are allowed to be selected.");
                return -1;
            }
            // assume always a Part::Feature derived object as support
            assert(selection[0].getObject()->getTypeId().isDerivedFrom(
                Part::Feature::getClassTypeId()));
            SketchSubNames = selection[1].getSubNames();
            SupportSubNames = selection[0].getSubNames();
        }
        else {
            ErrorMsg = QObject::tr("One of the selected has to be on the sketch.");
            return -1;
        }
    }

    return Items.size();
}

}// namespace SketcherGui

/* Constrain commands =======================================================*/

namespace SketcherGui
{
/**
 * @brief The SelType enum
 * Types of sketch elements that can be (pre)selected. The root/origin and the
 * axes are put up separately so that they can be specifically disallowed, for
 * example, when in lock, horizontal, or vertical constraint modes.
 */
enum SelType
{
    SelUnknown = 0,
    SelVertex = 1,
    SelVertexOrRoot = 64,
    SelRoot = 2,
    SelEdge = 4,
    SelEdgeOrAxis = 128,
    SelHAxis = 8,
    SelVAxis = 16,
    SelExternalEdge = 32
};

/**
 * @brief The GenericConstraintSelection class
 * SelectionFilterGate with changeable filters. In a constraint creation mode
 * like point on object, if the first selection object can be a point, the next
 * has to be a curve for the constraint to make sense. Thus filters are
 * changeable so that same filter can be kept on while in one mode.
 */
class GenericConstraintSelection: public Gui::SelectionFilterGate
{
    App::DocumentObject* object;

public:
    explicit GenericConstraintSelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate(nullPointer())
        , object(obj)
        , allowedSelTypes(0)
    {}

    bool allow(App::Document*, App::DocumentObject* pObj, const char* sSubName) override
    {
        if (pObj != this->object) {
            return false;
        }
        if (!sSubName || sSubName[0] == '\0') {
            return false;
        }
        std::string element(sSubName);
        if ((allowedSelTypes & (SelRoot | SelVertexOrRoot) && element.substr(0, 9) == "RootPoint")
            || (allowedSelTypes & (SelVertex | SelVertexOrRoot) && element.substr(0, 6) == "Vertex")
            || (allowedSelTypes & (SelEdge | SelEdgeOrAxis) && element.substr(0, 4) == "Edge")
            || (allowedSelTypes & (SelHAxis | SelEdgeOrAxis) && element.substr(0, 6) == "H_Axis")
            || (allowedSelTypes & (SelVAxis | SelEdgeOrAxis) && element.substr(0, 6) == "V_Axis")
            || (allowedSelTypes & SelExternalEdge && element.substr(0, 12) == "ExternalEdge")) {
            return true;
        }

        return false;
    }

    void setAllowedSelTypes(unsigned int types)
    {
        if (types < 256) {
            allowedSelTypes = types;
        }
    }

protected:
    int allowedSelTypes;
};
}// namespace SketcherGui

/**
 * @brief The CmdSketcherConstraint class
 * Superclass for all sketcher constraints to ease generation of constraint
 * creation modes.
 */
class CmdSketcherConstraint: public Gui::Command
{
    friend class DrawSketchHandlerGenConstraint;

public:
    explicit CmdSketcherConstraint(const char* name)
        : Command(name)
    {}

    ~CmdSketcherConstraint() override
    {}

    const char* className() const override
    {
        return "CmdSketcherConstraint";
    }

protected:
    /**
     * @brief allowedSelSequences
     * Each element is a vector representing sequence of selections allowable.
     * DrawSketchHandlerGenConstraint will use these to filter elements and
     * generate sequences to be passed to applyConstraint().
     * Whenever any sequence is completed, applyConstraint() is called, so it's
     * best to keep them prefix-free.
     * Be mindful that when SelVertex and SelRoot are given preference over
     * SelVertexOrRoot, and similar for edges/axes. Thus if a vertex is selected
     * when SelVertex and SelVertexOrRoot are both applicable, only sequences with
     * SelVertex will be continue.
     *
     * TODO: Introduce structs to allow keeping first selection
     */
    std::vector<std::vector<SketcherGui::SelType>> allowedSelSequences;

    virtual void applyConstraint(std::vector<SelIdPair>&, int)
    {}
    void activated(int /*iMsg*/) override;
    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

class DrawSketchHandlerGenConstraint: public DrawSketchHandler
{
public:
    explicit DrawSketchHandlerGenConstraint(CmdSketcherConstraint* _cmd)
        : cmd(_cmd)
        , seqIndex(0)
    {}
    ~DrawSketchHandlerGenConstraint() override
    {
        Gui::Selection().rmvSelectionGate();
    }

    void mouseMove(Base::Vector2d /*onSketchPos*/) override
    {}

    bool pressButton(Base::Vector2d /*onSketchPos*/) override
    {
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        SelIdPair selIdPair;
        selIdPair.GeoId = GeoEnum::GeoUndef;
        selIdPair.PosId = Sketcher::PointPos::none;
        std::stringstream ss;
        SelType newSelType = SelUnknown;

        // For each SelType allowed, check if button is released there and assign it to selIdPair
        int VtId = getPreselectPoint();
        int CrvId = getPreselectCurve();
        int CrsId = getPreselectCross();
        if (allowedSelTypes & (SelRoot | SelVertexOrRoot) && CrsId == 0) {
            selIdPair.GeoId = Sketcher::GeoEnum::RtPnt;
            selIdPair.PosId = Sketcher::PointPos::start;
            newSelType = (allowedSelTypes & SelRoot) ? SelRoot : SelVertexOrRoot;
            ss << "RootPoint";
        }
        else if (allowedSelTypes & (SelVertex | SelVertexOrRoot) && VtId >= 0) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, selIdPair.GeoId, selIdPair.PosId);
            newSelType = (allowedSelTypes & SelVertex) ? SelVertex : SelVertexOrRoot;
            ss << "Vertex" << VtId + 1;
        }
        else if (allowedSelTypes & (SelEdge | SelEdgeOrAxis) && CrvId >= 0) {
            selIdPair.GeoId = CrvId;
            newSelType = (allowedSelTypes & SelEdge) ? SelEdge : SelEdgeOrAxis;
            ss << "Edge" << CrvId + 1;
        }
        else if (allowedSelTypes & (SelHAxis | SelEdgeOrAxis) && CrsId == 1) {
            selIdPair.GeoId = Sketcher::GeoEnum::HAxis;
            newSelType = (allowedSelTypes & SelHAxis) ? SelHAxis : SelEdgeOrAxis;
            ss << "H_Axis";
        }
        else if (allowedSelTypes & (SelVAxis | SelEdgeOrAxis) && CrsId == 2) {
            selIdPair.GeoId = Sketcher::GeoEnum::VAxis;
            newSelType = (allowedSelTypes & SelVAxis) ? SelVAxis : SelEdgeOrAxis;
            ss << "V_Axis";
        }
        else if (allowedSelTypes & SelExternalEdge && CrvId <= Sketcher::GeoEnum::RefExt) {
            // TODO: Figure out how this works
            selIdPair.GeoId = CrvId;
            newSelType = SelExternalEdge;
            ss << "ExternalEdge" << Sketcher::GeoEnum::RefExt + 1 - CrvId;
        }

        if (selIdPair.GeoId == GeoEnum::GeoUndef) {
            // If mouse is released on "blank" space, start over
            selSeq.clear();
            resetOngoingSequences();
            Gui::Selection().clearSelection();
        }
        else {
            // If mouse is released on something allowed, select it and move forward
            selSeq.push_back(selIdPair);
            Gui::Selection().addSelection(sketchgui->getSketchObject()->getDocument()->getName(),
                                          sketchgui->getSketchObject()->getNameInDocument(),
                                          ss.str().c_str(),
                                          onSketchPos.x,
                                          onSketchPos.y,
                                          0.f);
            _tempOnSequences.clear();
            allowedSelTypes = 0;
            for (std::set<int>::iterator token = ongoingSequences.begin();
                 token != ongoingSequences.end();
                 ++token) {
                if ((cmd->allowedSelSequences).at(*token).at(seqIndex) == newSelType) {
                    if (seqIndex == (cmd->allowedSelSequences).at(*token).size() - 1) {
                        // One of the sequences is completed. Pass to cmd->applyConstraint
                        cmd->applyConstraint(selSeq, *token);// replace arg 2 by ongoingToken

                        selSeq.clear();
                        resetOngoingSequences();

                        return true;
                    }
                    _tempOnSequences.insert(*token);
                    allowedSelTypes =
                        allowedSelTypes | (cmd->allowedSelSequences).at(*token).at(seqIndex + 1);
                }
            }

            // Progress to next seqIndex
            std::swap(_tempOnSequences, ongoingSequences);
            seqIndex++;
            selFilterGate->setAllowedSelTypes(allowedSelTypes);
        }

        return true;
    }

private:
    void activated() override
    {
        selFilterGate = new GenericConstraintSelection(sketchgui->getObject());

        resetOngoingSequences();

        selSeq.clear();

        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(selFilterGate);

        // Constrain icon size in px
        qreal pixelRatio = devicePixelRatio();
        const unsigned long defaultCrosshairColor = 0xFFFFFF;
        unsigned long color = getCrosshairColor();
        auto colorMapping = std::map<unsigned long, unsigned long>();
        colorMapping[defaultCrosshairColor] = color;

        qreal fullIconWidth = 32 * pixelRatio;
        qreal iconWidth = 16 * pixelRatio;
        QPixmap cursorPixmap =
                    Gui::BitmapFactory().pixmapFromSvg("Sketcher_Crosshair",
                                                       QSizeF(fullIconWidth, fullIconWidth),
                                                       colorMapping),
                icon = Gui::BitmapFactory().pixmapFromSvg(cmd->getPixmap(),
                                                          QSizeF(iconWidth, iconWidth));
        QPainter cursorPainter;
        cursorPainter.begin(&cursorPixmap);
        cursorPainter.drawPixmap(16 * pixelRatio, 16 * pixelRatio, icon);
        cursorPainter.end();
        int hotX = 8;
        int hotY = 8;
        cursorPixmap.setDevicePixelRatio(pixelRatio);
        // only X11 needs hot point coordinates to be scaled
        if (qGuiApp->platformName() == QLatin1String("xcb")) {
            hotX *= pixelRatio;
            hotY *= pixelRatio;
        }
        setCursor(cursorPixmap, hotX, hotY, false);
    }

protected:
    CmdSketcherConstraint* cmd;

    GenericConstraintSelection* selFilterGate = nullptr;

    std::vector<SelIdPair> selSeq;
    unsigned int allowedSelTypes = 0;

    /// indices of currently ongoing sequences in cmd->allowedSequences
    std::set<int> ongoingSequences, _tempOnSequences;
    /// Index within the selection sequences active
    unsigned int seqIndex;

    void resetOngoingSequences()
    {
        ongoingSequences.clear();
        for (unsigned int i = 0; i < cmd->allowedSelSequences.size(); i++) {
            ongoingSequences.insert(i);
        }
        seqIndex = 0;

        // Estimate allowed selections from the first types in allowedSelTypes
        allowedSelTypes = 0;
        for (std::vector<std::vector<SelType>>::const_iterator it =
                 cmd->allowedSelSequences.begin();
             it != cmd->allowedSelSequences.end();
             ++it) {
            allowedSelTypes = allowedSelTypes | (*it).at(seqIndex);
        }
        selFilterGate->setAllowedSelTypes(allowedSelTypes);

        Gui::Selection().clearSelection();
    }
};

void CmdSketcherConstraint::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
    getSelection().clearSelection();
}

// Comp for dimension tools =============================================

class CmdSketcherCompDimensionTools : public Gui::GroupCommand
{
public:
    CmdSketcherCompDimensionTools()
        : GroupCommand("Sketcher_CompDimensionTools")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Dimension");
        sToolTipText = QT_TR_NOOP("Dimension tools.");
        sWhatsThis = "Sketcher_CompDimensionTools";
        sStatusTip = sToolTipText;
        eType = ForEdit;

        setCheckable(false);

        addCommand("Sketcher_Dimension");
        addCommand(); //separator
        addCommand("Sketcher_ConstrainLock");
        addCommand("Sketcher_ConstrainDistanceX");
        addCommand("Sketcher_ConstrainDistanceY");
        addCommand("Sketcher_ConstrainDistance");
        addCommand("Sketcher_ConstrainRadius");
        addCommand("Sketcher_ConstrainDiameter");
        addCommand("Sketcher_ConstrainRadiam");
        addCommand("Sketcher_ConstrainAngle");
    }

    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (mode) {
        case Reference:
            al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Dimension_Driven"));
            //al[1] is the separator
            al[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Lock_Driven"));
            al[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance_Driven"));
            al[4]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance_Driven"));
            al[5]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Length_Driven"));
            al[6]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius_Driven"));
            al[7]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter_Driven"));
            al[8]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radiam_Driven"));
            al[9]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle_Driven"));
            getAction()->setIcon(al[index]->icon());
            break;
        case Driving:
            al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Dimension"));
            //al[1] is the separator
            al[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Lock"));
            al[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance"));
            al[4]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance"));
            al[5]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Length"));
            al[6]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius"));
            al[7]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter"));
            al[8]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radiam"));
            al[9]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle"));
            getAction()->setIcon(al[index]->icon());
            break;
        }
    }

    const char* className() const override { return "CmdSketcherCompDimensionTools"; }
};

// Dimension tool =======================================================

class GeomSelectionSizes
{
public:
    GeomSelectionSizes(size_t s_pts, size_t s_lns, size_t s_cir, size_t s_ell) :
        s_pts(s_pts), s_lns(s_lns), s_cir(s_cir), s_ell(s_ell) {}
    ~GeomSelectionSizes() {}

    bool hasPoints()        const { return s_pts > 0; }
    bool hasLines()         const { return s_lns > 0; }
    bool hasCirclesOrArcs() const { return s_cir > 0; }
    bool hasEllipseAndCo()  const { return s_ell > 0; }

    bool has1Point()             const { return s_pts == 1 && s_lns == 0 && s_cir == 0 && s_ell == 0; }
    bool has2Points()            const { return s_pts == 2 && s_lns == 0 && s_cir == 0 && s_ell == 0; }
    bool has1Point1Line()        const { return s_pts == 1 && s_lns == 1 && s_cir == 0 && s_ell == 0; }
    bool has3Points()            const { return s_pts == 3 && s_lns == 0 && s_cir == 0 && s_ell == 0; }
    bool has4MorePoints()        const { return s_pts >= 4 && s_lns == 0 && s_cir == 0 && s_ell == 0; }
    bool has2Points1Line()       const { return s_pts == 2 && s_lns == 1 && s_cir == 0 && s_ell == 0; }
    bool has3MorePoints1Line()   const { return s_pts >= 3 && s_lns == 1 && s_cir == 0 && s_ell == 0; }
    bool has1Point1Circle()      const { return s_pts == 1 && s_lns == 0 && s_cir == 1 && s_ell == 0; }
    bool has1MorePoint1Ellipse() const { return s_pts >= 1 && s_lns == 0 && s_cir == 0 && s_ell == 1; }

    bool has1Line()              const { return s_pts == 0 && s_lns == 1 && s_cir == 0 && s_ell == 0; }
    bool has2Lines()             const { return s_pts == 0 && s_lns == 2 && s_cir == 0 && s_ell == 0; }
    bool has3MoreLines()         const { return s_pts == 0 && s_lns >= 3 && s_cir == 0 && s_ell == 0; }
    bool has1Line1Circle()       const { return s_pts == 0 && s_lns == 1 && s_cir == 1 && s_ell == 0; }
    bool has1Line2Circles()      const { return s_pts == 0 && s_lns == 1 && s_cir == 2 && s_ell == 0; }
    bool has1Line1Ellipse()      const { return s_pts == 0 && s_lns == 1 && s_cir == 0 && s_ell == 1; }

    bool has1Circle()            const { return s_pts == 0 && s_lns == 0 && s_cir == 1 && s_ell == 0; }
    bool has2Circles()           const { return s_pts == 0 && s_lns == 0 && s_cir == 2 && s_ell == 0; }
    bool has3MoreCircles()       const { return s_pts == 0 && s_lns == 0 && s_cir >= 3 && s_ell == 0; }
    bool has1Circle1Ellipse()    const { return s_pts == 0 && s_lns == 0 && s_cir == 1 && s_ell == 1; }

    bool has1Ellipse()           const { return s_pts == 0 && s_lns == 0 && s_cir == 0 && s_ell == 1; }
    bool has2MoreEllipses()      const { return s_pts == 0 && s_lns == 0 && s_cir == 0 && s_ell >= 2; }

    size_t s_pts, s_lns, s_cir, s_ell;
};

class DrawSketchHandlerDimension : public DrawSketchHandler
{
public:
    DrawSketchHandlerDimension(std::vector<std::string> SubNames)
        : specialConstraint(SpecialConstraint::None)
        , availableConstraint(AvailableConstraint::FIRST)
        , previousOnSketchPos(Base::Vector2d(0.f, 0.f))
        , selPoints({})
        , selLine({})
        , selCircleArc({})
        , selEllipseAndCo({})
        , initialSelection(std::move(SubNames))
        , numberOfConstraintsCreated(0)
    {
    }
    ~DrawSketchHandlerDimension() override
    {
    }

    enum class AvailableConstraint {
        FIRST,
        SECOND,
        THIRD,
        FOURTH,
        FIFTH,
        RESET
    };

    enum class SpecialConstraint {
        LineOr2PointsDistance,
        Block,
        None
    };

    void activated() override
    {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Dimension"));

        Obj = sketchgui->getSketchObject();

        // Constrain icon size in px
        qreal pixelRatio = devicePixelRatio();
        const unsigned long defaultCrosshairColor = 0xFFFFFF;
        unsigned long color = getCrosshairColor();
        auto colorMapping = std::map<unsigned long, unsigned long>();
        colorMapping[defaultCrosshairColor] = color;

        qreal fullIconWidth = 32 * pixelRatio;
        qreal iconWidth = 16 * pixelRatio;
        QPixmap cursorPixmap = Gui::BitmapFactory().pixmapFromSvg("Sketcher_Crosshair", QSizeF(fullIconWidth, fullIconWidth), colorMapping),
            icon = Gui::BitmapFactory().pixmapFromSvg("Constraint_Dimension", QSizeF(iconWidth, iconWidth));
        QPainter cursorPainter;
        cursorPainter.begin(&cursorPixmap);
        cursorPainter.drawPixmap(16 * pixelRatio, 16 * pixelRatio, icon);
        cursorPainter.end();
        int hotX = 8;
        int hotY = 8;
        cursorPixmap.setDevicePixelRatio(pixelRatio);
        // only X11 needs hot point coordinates to be scaled
        if (qGuiApp->platformName() == QLatin1String("xcb")) {
            hotX *= pixelRatio;
            hotY *= pixelRatio;
        }
        setCursor(cursorPixmap, hotX, hotY, false);

        handleInitialSelection();
    }

    void deactivated() override
    {
        Gui::Command::abortCommand();
        Obj->solve();
        sketchgui->draw(false, false); // Redraw
    }

    void registerPressedKey(bool pressed, int key) override
    {
        if (key == SoKeyboardEvent::M && pressed) {
            if (availableConstraint == AvailableConstraint::FIRST) {
                availableConstraint = AvailableConstraint::SECOND;
            }
            else if (availableConstraint == AvailableConstraint::SECOND) {
                availableConstraint = AvailableConstraint::THIRD;
            }
            else if (availableConstraint == AvailableConstraint::THIRD) {
                availableConstraint = AvailableConstraint::FOURTH;
            }
            else if (availableConstraint == AvailableConstraint::FOURTH) {
                availableConstraint = AvailableConstraint::FIFTH;
            }
            else if (availableConstraint == AvailableConstraint::FIFTH || availableConstraint == AvailableConstraint::RESET) {
                availableConstraint = AvailableConstraint::FIRST;
            }
            makeAppropriateConstraint(previousOnSketchPos);
        }
    }

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
        previousOnSketchPos = onSketchPos;

        //Change distance constraint based on position of mouse.
        if (specialConstraint == SpecialConstraint::LineOr2PointsDistance)
            updateDistanceType(onSketchPos);

        //Move constraints
        if (numberOfConstraintsCreated > 0) {
            bool oneMoved = false;
            for (int i = 0; i < numberOfConstraintsCreated; i++) {
                if (ConStr[ConStr.size() - 1 - i]->isDimensional()) {
                    Base::Vector2d pointWhereToMove = onSketchPos;

                    if (specialConstraint == SpecialConstraint::Block) {
                        if (i == 0)
                            pointWhereToMove.x = Obj->getPoint(selPoints[0].GeoId, selPoints[0].PosId).x;
                        else
                            pointWhereToMove.y = Obj->getPoint(selPoints[0].GeoId, selPoints[0].PosId).y;
                    }
                    moveConstraint(ConStr.size() - 1 - i, pointWhereToMove);
                    oneMoved = true;
                }
            }
            if (oneMoved)
                sketchgui->draw(false, false); // Redraw
        }
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos)
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        availableConstraint = AvailableConstraint::FIRST;
        SelIdPair selIdPair;
        selIdPair.GeoId = GeoEnum::GeoUndef;
        selIdPair.PosId = Sketcher::PointPos::none;
        std::stringstream ss;
        Base::Type newselGeoType = Base::Type::badType();

        int VtId = getPreselectPoint();
        int CrvId = getPreselectCurve();
        int CrsId = getPreselectCross();

        if (VtId >= 0) { //Vertex
            Obj->getGeoVertexIndex(VtId,
                selIdPair.GeoId, selIdPair.PosId);
            newselGeoType = Part::GeomPoint::getClassTypeId();
            ss << "Vertex" << VtId + 1;
        }
        else if (CrsId == 0) { //RootPoint
            selIdPair.GeoId = Sketcher::GeoEnum::RtPnt;
            selIdPair.PosId = Sketcher::PointPos::start;
            newselGeoType = Part::GeomPoint::getClassTypeId();
            ss << "RootPoint";
        }
        else if (CrsId == 1) { //H_Axis
            selIdPair.GeoId = Sketcher::GeoEnum::HAxis;
            newselGeoType = Part::GeomLineSegment::getClassTypeId();
            ss << "H_Axis";
        }
        else if (CrsId == 2) { //V_Axis
            selIdPair.GeoId = Sketcher::GeoEnum::VAxis;
            newselGeoType = Part::GeomLineSegment::getClassTypeId();
            ss << "V_Axis";
        }
        else if (CrvId >= 0 || CrvId <= Sketcher::GeoEnum::RefExt) { //Curves
            selIdPair.GeoId = CrvId;
            const Part::Geometry* geo = Obj->getGeometry(CrvId);

            newselGeoType = geo->getTypeId();

            if (CrvId >= 0) {
                ss << "Edge" << CrvId + 1;
            }
            else {
                ss << "ExternalEdge" << Sketcher::GeoEnum::RefExt + 1 - CrvId;
            }
        }

        if (selIdPair.GeoId == GeoEnum::GeoUndef) {
            // If mouse is released on "blank" space, finalize and start over
            finalizeCommand();
            return true;
        }

        std::vector<SelIdPair>& selVector = getSelectionVector(newselGeoType);

        if (notSelectedYet(selIdPair)) {
            //add the geometry to its type vector. Temporarily if not selAllowed
            selVector.push_back(selIdPair);

            bool selAllowed = makeAppropriateConstraint(onSketchPos);

            if (selAllowed) {
                // If mouse is released on something allowed, select it
                Gui::Selection().addSelection(Obj->getDocument()->getName(),
                    Obj->getNameInDocument(),
                    ss.str().c_str(), onSketchPos.x, onSketchPos.y, 0.f);
                sketchgui->draw(false, false); // Redraw
            }
            else {
                selVector.pop_back();
            }
        }
        else {
            //if it is already selected we unselect it.
            selVector.pop_back();
            if (!selectionEmpty()) {
                makeAppropriateConstraint(onSketchPos);
            }
            else {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Dimension"));
            }

            Gui::Selection().rmvSelection(Obj->getDocument()->getName(),
                Obj->getNameInDocument(),
                ss.str().c_str());
            sketchgui->draw(false, false); // Redraw
        }
        return true;
    }
protected:
    SpecialConstraint specialConstraint;
    AvailableConstraint availableConstraint;

    Base::Vector2d previousOnSketchPos;

    std::vector<SelIdPair> selPoints;
    std::vector<SelIdPair> selLine;
    std::vector<SelIdPair> selCircleArc;
    std::vector<SelIdPair> selEllipseAndCo;

    std::vector<std::string> initialSelection;

    int numberOfConstraintsCreated;

    Sketcher::SketchObject* Obj;

    void handleInitialSelection()
    {
        if (initialSelection.size() == 0) {
            return;
        }

        availableConstraint = AvailableConstraint::FIRST;

        // Add the selected elements to their corresponding selection vectors
        for (auto& selElement : initialSelection) {
            SelIdPair selIdPair;
            getIdsFromName(selElement, Obj, selIdPair.GeoId, selIdPair.PosId);

            Base::Type newselGeoType = Base::Type::badType();
            if (isEdge(selIdPair.GeoId, selIdPair.PosId)) {
                const Part::Geometry* geo = Obj->getGeometry(selIdPair.GeoId);
                newselGeoType = geo->getTypeId();
            }
            else if (isVertex(selIdPair.GeoId, selIdPair.PosId)) {
                newselGeoType = Part::GeomPoint::getClassTypeId();
            }

            std::vector<SelIdPair>& selVector = getSelectionVector(newselGeoType);

            //add the geometry to its type vector. Temporarily if not selAllowed
            selVector.push_back(selIdPair);
        }

        // See if the selection is valid
        bool selAllowed = makeAppropriateConstraint(Base::Vector2d(0.,0.));

        if (!selAllowed) {
            selPoints.clear();
            selLine.clear();
            selCircleArc.clear();
            selEllipseAndCo.clear();
        }
    }

    void finalizeCommand()
    {
        // Ask for the value of datum constraints
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);
        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

        bool commandHandledInEditDatum = false;
        for (int i = numberOfConstraintsCreated - 1; i >= 0; i--) {
            if (show && ConStr[ConStr.size() - 1 - i]->isDimensional() && ConStr[ConStr.size() - 1 - i]->isDriving) {
                commandHandledInEditDatum = true;
                EditDatumDialog editDatumDialog(sketchgui, ConStr.size() - 1 - i);
                editDatumDialog.exec();
                if (!editDatumDialog.isSuccess()) {
                    break;
                }
            }
        }

        if (!commandHandledInEditDatum)
            Gui::Command::commitCommand();

        // This code enables the continuous creation mode.
        bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
        if (continuousMode) {
            Gui::Selection().clearSelection();
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Dimension"));
            numberOfConstraintsCreated = 0;
            specialConstraint = SpecialConstraint::None;
            previousOnSketchPos = Base::Vector2d(0.f, 0.f);
            selPoints.clear();
            selLine.clear();
            selCircleArc.clear();
            selEllipseAndCo.clear();
        }
        else {
            sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
        }
    }

    std::vector<SelIdPair>& getSelectionVector(Base::Type selGeoType) {
        if (selGeoType == Part::GeomPoint::getClassTypeId()) {
            return selPoints;
        }
        else if (selGeoType == Part::GeomLineSegment::getClassTypeId()) {
            return selLine;
        }
        else if (selGeoType == Part::GeomArcOfCircle::getClassTypeId() ||
            selGeoType == Part::GeomCircle::getClassTypeId()) {
            return selCircleArc;
        }
        else if (selGeoType == Part::GeomEllipse::getClassTypeId() ||
            selGeoType == Part::GeomArcOfEllipse::getClassTypeId() ||
            selGeoType == Part::GeomArcOfHyperbola::getClassTypeId() ||
            selGeoType == Part::GeomArcOfParabola::getClassTypeId()) {
            return selEllipseAndCo;
        }

        static std::vector<SelIdPair> emptyVector;
        return emptyVector;
    }

    bool notSelectedYet(const SelIdPair& elem)
    {
        auto contains = [&](const std::vector<SelIdPair>& vec, const SelIdPair& elem) {
            for (const auto& x : vec)
            {
                if (x.GeoId == elem.GeoId && x.PosId == elem.PosId)
                    return true;
            }
            return false;
        };

        return !contains(selPoints, elem)
            && !contains(selLine, elem)
            && !contains(selCircleArc, elem)
            && !contains(selEllipseAndCo, elem);
    }

    bool selectionEmpty()
    {
        return selPoints.empty() && selLine.empty() && selCircleArc.empty() && selEllipseAndCo.empty();
    }

    bool makeAppropriateConstraint(Base::Vector2d onSketchPos) {
        bool selAllowed = false;

        GeomSelectionSizes selection(selPoints.size(), selLine.size(), selCircleArc.size(), selEllipseAndCo.size());

        if (selection.hasPoints()) {
            if (selection.has1Point()) { makeCts_1Point(selAllowed, onSketchPos); }
            else if (selection.has2Points()) { makeCts_2Point(selAllowed, onSketchPos); }
            else if (selection.has1Point1Line()) { makeCts_1Point1Line(selAllowed, onSketchPos); }
            else if (selection.has3Points()) { makeCts_3Point(selAllowed, selection.s_pts); }
            else if (selection.has4MorePoints()) { makeCts_4MorePoint(selAllowed, selection.s_pts); }
            else if (selection.has2Points1Line()) { makeCts_2Point1Line(selAllowed, onSketchPos, selection.s_pts); }
            else if (selection.has3MorePoints1Line()) { makeCts_3MorePoint1Line(selAllowed, onSketchPos, selection.s_pts); }
            else if (selection.has1Point1Circle()) { makeCts_1Point1Circle(selAllowed, onSketchPos); }
            else if (selection.has1MorePoint1Ellipse()) { makeCts_1MorePoint1Ellipse(selAllowed); }
        }
        else if (selection.hasLines()) {
            if (selection.has1Line()) { makeCts_1Line(selAllowed, onSketchPos); }
            else if (selection.has2Lines()) { makeCts_2Line(selAllowed, onSketchPos); }
            else if (selection.has3MoreLines()) { makeCts_3MoreLine(selAllowed, selection.s_lns); }
            else if (selection.has1Line1Circle()) { makeCts_1Line1Circle(selAllowed, onSketchPos); }
            else if (selection.has1Line2Circles()) { makeCts_1Line2Circle(selAllowed); }
            else if (selection.has1Line1Ellipse()) { makeCts_1Line1Ellipse(selAllowed); }
        }
        else if (selection.hasCirclesOrArcs()) {
            if (selection.has1Circle()) { makeCts_1Circle(selAllowed, onSketchPos); }
            else if (selection.has2Circles()) { makeCts_2Circle(selAllowed, onSketchPos); }
            else if (selection.has3MoreCircles()) { makeCts_3MoreCircle(selAllowed, selection.s_cir); }
            else if (selection.has1Circle1Ellipse()) { makeCts_1Circle1Ellipse(selAllowed); }
        }
        else if (selection.hasEllipseAndCo()) {
            if (selection.has1Ellipse()) { makeCts_1Ellipse(selAllowed); }
            else if (selection.has2MoreEllipses()) { makeCts_2MoreEllipse(selAllowed, selection.s_ell); }
        }
        return selAllowed;
    }

    void makeCts_1Point(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        //distance, lock
        if (availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 'Distance to origin' constraint"));
            createDistanceConstrain(selPoints[0].GeoId, selPoints[0].PosId, Sketcher::GeoEnum::RtPnt, Sketcher::PointPos::start, onSketchPos);
            selAllowed = true;
        }
        if (availableConstraint == AvailableConstraint::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add lock constraint"));
            specialConstraint = SpecialConstraint::Block;
            createDistanceXYConstrain(Sketcher::DistanceX, selPoints[0].GeoId, selPoints[0].PosId, Sketcher::GeoEnum::RtPnt, Sketcher::PointPos::start, onSketchPos);
            createDistanceXYConstrain(Sketcher::DistanceY, selPoints[0].GeoId, selPoints[0].PosId, Sketcher::GeoEnum::RtPnt, Sketcher::PointPos::start, onSketchPos);
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_2Point(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        //distance, horizontal, vertical
        if (availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Distance constraint"));
            createDistanceConstrain(selPoints[0].GeoId, selPoints[0].PosId, selPoints[1].GeoId, selPoints[1].PosId, onSketchPos);
            selAllowed = true;
        }
        if (availableConstraint == AvailableConstraint::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 'Horizontal' constraints"));
            createHorizontalConstrain(selPoints[0].GeoId, selPoints[0].PosId, selPoints[1].GeoId, selPoints[1].PosId);
        }
        if (availableConstraint == AvailableConstraint::THIRD) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 'Vertical' constraints"));
            createVerticalConstrain(selPoints[0].GeoId, selPoints[0].PosId, selPoints[1].GeoId, selPoints[1].PosId);
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Point1Line(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        //distance, Symmetry
        if (availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add point to line Distance constraint"));
            createDistanceConstrain(selPoints[0].GeoId, selPoints[0].PosId, selLine[0].GeoId, selLine[0].PosId, onSketchPos); // point to be on first parameter
            selAllowed = true;
        }
        if (availableConstraint == AvailableConstraint::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Symmetry constraint"));
            createSymmetryConstrain(selLine[0].GeoId, Sketcher::PointPos::start, selLine[0].GeoId, Sketcher::PointPos::end, selPoints[0].GeoId, selPoints[0].PosId);
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_3Point(bool& selAllowed, size_t s_pts)
    {
        //Horizontal, vertical, symmetry
        if (s_pts > 0 && availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 'Horizontal' constraints"));
            for (size_t i = 0; i < s_pts - 1; i++) {
                createHorizontalConstrain(selPoints[i].GeoId, selPoints[i].PosId, selPoints[i + 1].GeoId, selPoints[i + 1].PosId);
            }
            selAllowed = true;
        }
        if (s_pts > 0 && availableConstraint == AvailableConstraint::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 'Vertical' constraints"));
            for (size_t i = 0; i < s_pts - 1; i++) {
                createVerticalConstrain(selPoints[i].GeoId, selPoints[i].PosId, selPoints[i + 1].GeoId, selPoints[i + 1].PosId);
            }
        }
        if (availableConstraint == AvailableConstraint::THIRD) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Symmetry constraints"));
            createSymmetryConstrain(selPoints[0].GeoId, selPoints[0].PosId, selPoints[1].GeoId, selPoints[1].PosId, selPoints[2].GeoId, selPoints[2].PosId);
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_4MorePoint(bool& selAllowed, size_t s_pts)
    {
        //Horizontal, vertical
        if (s_pts > 0 && availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 'Horizontal' constraints"));
            for (size_t i = 0; i < s_pts - 1; i++) {
                createHorizontalConstrain(selPoints[i].GeoId, selPoints[i].PosId, selPoints[i + 1].GeoId, selPoints[i + 1].PosId);
            }
            selAllowed = true;
        }
        if (s_pts > 0 && availableConstraint == AvailableConstraint::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add 'Vertical' constraints"));
            for (size_t i = 0; i < s_pts - 1; i++) {
                createVerticalConstrain(selPoints[i].GeoId, selPoints[i].PosId, selPoints[i + 1].GeoId, selPoints[i + 1].PosId);
            }
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_2Point1Line(bool& selAllowed, Base::Vector2d onSketchPos, size_t s_pts)
    {
        //symmetry, distances
        if (availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Symmetry constraint"));
            createSymmetryConstrain(selPoints[0].GeoId, selPoints[0].PosId, selPoints[1].GeoId, selPoints[1].PosId, selLine[0].GeoId, selLine[0].PosId);
            selAllowed = true;
        }
        if (availableConstraint == AvailableConstraint::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Distance constraints"));
            for (size_t i = 0; i < s_pts; i++) {
                createDistanceConstrain(selPoints[i].GeoId, selPoints[i].PosId, selLine[0].GeoId, selLine[0].PosId, onSketchPos);
            }
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_3MorePoint1Line(bool& selAllowed, Base::Vector2d onSketchPos, size_t s_pts)
    {
        //distances
        if (availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Distance constraints"));
            for (size_t i = 0; i < s_pts; i++) {
                createDistanceConstrain(selPoints[i].GeoId, selPoints[i].PosId, selLine[0].GeoId, selLine[0].PosId, onSketchPos);
            }
            selAllowed = true;
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Point1Circle(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        //Distance. For now only circles not arcs!
        const Part::Geometry* geom = Obj->getGeometry(selCircleArc[0].GeoId);

        if (availableConstraint == AvailableConstraint::FIRST && isCircle(*geom)) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add length constraint"));
            createDistanceConstrain(selPoints[0].GeoId, selPoints[0].PosId, selCircleArc[0].GeoId, selCircleArc[0].PosId, onSketchPos);
            selAllowed = true;
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1MorePoint1Ellipse(bool& selAllowed)
    {
        Q_UNUSED(selAllowed)
        //distance between 1 point and ellipse/arc of... not supported yet.
        if (availableConstraint == AvailableConstraint::FIRST) {
            //nothing yet
            //availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Line(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        //axis can be selected but we don't want distance on axis!
        if ((selLine[0].GeoId != Sketcher::GeoEnum::VAxis && selLine[0].GeoId != Sketcher::GeoEnum::HAxis)) {
            //distance, horizontal, vertical, block
            if (availableConstraint == AvailableConstraint::FIRST) {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add length constraint"));
                createDistanceConstrain(selLine[0].GeoId, Sketcher::PointPos::start, selLine[0].GeoId, Sketcher::PointPos::end, onSketchPos);
                selAllowed = true;
            }
            if (availableConstraint == AvailableConstraint::SECOND) {
                if (isHorizontalVerticalBlock(selLine[0].GeoId)) {
                    //if the line has a vertical horizontal or block constraint then we don't switch to other modes as they are horizontal, vertical and block.
                    availableConstraint = AvailableConstraint::RESET;
                }
                else {
                    restartCommand(QT_TRANSLATE_NOOP("Command", "Add Horizontal constraint"));
                    createHorizontalConstrain(selLine[0].GeoId, Sketcher::PointPos::none, GeoEnum::GeoUndef, Sketcher::PointPos::none);
                }
            }
            if (availableConstraint == AvailableConstraint::THIRD) {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add Vertical constraint"));
                createVerticalConstrain(selLine[0].GeoId, Sketcher::PointPos::none, GeoEnum::GeoUndef, Sketcher::PointPos::none);
            }
            if (availableConstraint == AvailableConstraint::FOURTH) {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add Block constraint"));
                createBlockConstrain(selLine[0].GeoId);
                availableConstraint = AvailableConstraint::RESET;
            }
        }
        else {
            //But axis can still be selected
            selAllowed = true;
        }
    }

    void makeCts_2Line(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        //angle (if parallel: Distance (see in createAngleConstrain)), equal.
        if (availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Angle constraint"));
            createAngleConstrain(selLine[0].GeoId, selLine[1].GeoId, onSketchPos);
            selAllowed = true;
        }
        if (availableConstraint == AvailableConstraint::SECOND) {
            if (selLine[0].GeoId == Sketcher::GeoEnum::VAxis || selLine[1].GeoId == Sketcher::GeoEnum::VAxis
                || selLine[0].GeoId == Sketcher::GeoEnum::HAxis || selLine[1].GeoId == Sketcher::GeoEnum::HAxis) {
                //if one line is axis, then can't equal..
            }
            else {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add Equality constraint"));
                createEqualityConstrain(selLine[0].GeoId, selLine[1].GeoId);
            }
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_3MoreLine(bool& selAllowed, size_t s_lns)
    {
        //equality.
        if (s_lns > 0 && availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Equality constraints"));
            for (size_t i = 0; i < s_lns - 1; i++) {
                createEqualityConstrain(selLine[i].GeoId, selLine[i + 1].GeoId);
            }
            selAllowed = true;
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Line1Circle(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        //Distance. For now only circles not arcs!
        const Part::Geometry* geom = Obj->getGeometry(selCircleArc[0].GeoId);

        if (availableConstraint == AvailableConstraint::FIRST && isCircle(*geom)) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add length constraint"));
            createDistanceConstrain(selCircleArc[0].GeoId, selCircleArc[0].PosId, selLine[0].GeoId, selLine[0].PosId, onSketchPos); //Line second parameter
            selAllowed = true;
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Line2Circle(bool& selAllowed)
    {
        //symmetry.
        if (availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Symmetry constraints"));
            createSymmetryConstrain(selCircleArc[0].GeoId, Sketcher::PointPos::mid, selCircleArc[1].GeoId, Sketcher::PointPos::mid, selLine[0].GeoId, selLine[0].PosId);
            selAllowed = true;
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Line1Ellipse(bool& selAllowed)
    {
        Q_UNUSED(selAllowed)
        //TODO distance between line and ellipse/arc of... not supported yet.
        if (availableConstraint == AvailableConstraint::FIRST) {
            //selAllowed = true;
            //availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Circle(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        const Part::Geometry* geom = Obj->getGeometry(selCircleArc[0].GeoId);
        Q_UNUSED(geom)

        if (availableConstraint == AvailableConstraint::FIRST
            || availableConstraint == AvailableConstraint::SECOND) {
            //Radius/diameter. Mode changes in createRadiusDiameterConstrain.
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Radius constraint"));
            createRadiusDiameterConstrain(selCircleArc[0].GeoId, onSketchPos);
            selAllowed = true;
        }
        if (availableConstraint == AvailableConstraint::THIRD) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add arc angle constraint"));
            createArcAngleConstrain(selCircleArc[0].GeoId, onSketchPos);
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_2Circle(bool& selAllowed, Base::Vector2d onSketchPos)
    {
        //Distance, radial distance, equality
        //Distance: For now only circles not arcs!
        const Part::Geometry* geom = Obj->getGeometry(selCircleArc[0].GeoId);
        const Part::Geometry* geom2 = Obj->getGeometry(selCircleArc[1].GeoId);
        if (availableConstraint == AvailableConstraint::FIRST) {
            if (isCircle(*geom) && isCircle(*geom2)) {
                restartCommand(QT_TRANSLATE_NOOP("Command", "Add length constraint"));
                createDistanceConstrain(selCircleArc[0].GeoId, selCircleArc[0].PosId, selCircleArc[1].GeoId, selCircleArc[1].PosId, onSketchPos);
            }
            else {
                availableConstraint = AvailableConstraint::THIRD;
            }
            selAllowed = true;
        }
        if (availableConstraint == AvailableConstraint::SECOND) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add concentric and length constraint"));
            bool created = createCoincidenceConstrain(selCircleArc[0].GeoId, Sketcher::PointPos::mid, selCircleArc[1].GeoId, Sketcher::PointPos::mid);
            if (!created) { //Already concentric, so skip to next
                availableConstraint = AvailableConstraint::THIRD;
            }
            else {
                createDistanceConstrain(selCircleArc[0].GeoId, selCircleArc[0].PosId, selCircleArc[1].GeoId, selCircleArc[1].PosId, onSketchPos);
            }
        }
        if (availableConstraint == AvailableConstraint::THIRD) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Equality constraint"));
            createEqualityConstrain(selCircleArc[0].GeoId, selCircleArc[1].GeoId);
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_3MoreCircle(bool& selAllowed, size_t s_cir)
    {
        //equality.
        if (s_cir > 0 && availableConstraint == AvailableConstraint::FIRST) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Equality constraint"));
            for (size_t i = 0; i < s_cir - 1; i++) {
                createEqualityConstrain(selCircleArc[i].GeoId, selCircleArc[i + 1].GeoId);
            }
            selAllowed = true;
            availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Circle1Ellipse(bool& selAllowed)
    {
        Q_UNUSED(selAllowed)
        //TODO distance between circle and ellipse/arc of... not supported yet.
        if (availableConstraint == AvailableConstraint::FIRST) {
            //selAllowed = true;
            //availableConstraint = AvailableConstraint::RESET;
        }
    }

    void makeCts_1Ellipse(bool& selAllowed)
    {
        //One ellipse or arc of ellipse/hyperbola/parabola - no constrain to attribute
        selAllowed = true;
    }

    void makeCts_2MoreEllipse(bool& selAllowed, size_t s_ell)
    {
        //only ellipse or arc of of same kind, then equality of all radius.
        bool allTheSame = 1;
        const Part::Geometry* geom = Obj->getGeometry(selEllipseAndCo[0].GeoId);
        Base::Type typeOf = geom->getTypeId();
        for (size_t i = 1; i < s_ell; i++) {
            const Part::Geometry* geomi = Obj->getGeometry(selEllipseAndCo[i].GeoId);
            if (typeOf != geomi->getTypeId()) {
                allTheSame = 0;
            }
        }
        if (allTheSame) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Equality constraint"));
            for (size_t i = 1; i < s_ell; i++) {
                createEqualityConstrain(selEllipseAndCo[0].GeoId, selEllipseAndCo[i].GeoId);
            }
            selAllowed = true;
        }
    }

    void createDistanceConstrain(int GeoId1, Sketcher::PointPos PosId1, int GeoId2, Sketcher::PointPos PosId2, Base::Vector2d onSketchPos) {
        // If there's a point, it must be GeoId1. We could add a swap to make sure but as it's hardcoded it's not necessary.

        if (GeoId1 == GeoId2 || (PosId1 != Sketcher::PointPos::none && PosId2 != Sketcher::PointPos::none)) {
            specialConstraint = SpecialConstraint::LineOr2PointsDistance;
        }

        bool arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj, GeoId1) && isPointOrSegmentFixed(Obj, GeoId2);

        if (PosId1 != Sketcher::PointPos::none && PosId2 == Sketcher::PointPos::none) { // Point-line case and point-circle
            Base::Vector3d pnt = Obj->getPoint(GeoId1, PosId1);
            double ActDist = 0.;
            const Part::Geometry* geom = Obj->getGeometry(GeoId2);

            if (isLineSegment(*geom)) {
                auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
                Base::Vector3d pnt1 = lineSeg->getStartPoint();
                Base::Vector3d pnt2 = lineSeg->getEndPoint();
                Base::Vector3d d = pnt2 - pnt1;
                ActDist = std::abs(-pnt.x * d.y + pnt.y * d.x + pnt1.x * pnt2.y - pnt2.x * pnt1.y) / d.Length();
            }
            else if (isCircle(*geom)) {
                auto circle = static_cast<const Part::GeomCircle*>(geom);
                Base::Vector3d ct = circle->getCenter();
                Base::Vector3d di = ct - pnt;
                ActDist = std::abs(di.Length() - circle->getRadius());
            }

            Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f)) ",
                GeoId1, static_cast<int>(PosId1), GeoId2, ActDist);
        }
        else if (PosId1 == Sketcher::PointPos::none && PosId2 == Sketcher::PointPos::none) { // Circle - line, circle - circle cases
            const Part::Geometry* geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geo2 = Obj->getGeometry(GeoId2);

            if (isCircle(*geo1) && isLineSegment(*geo2)) { // Circle - line case
                auto circleSeg = static_cast<const Part::GeomCircle*>(geo1);
                double radius = circleSeg->getRadius();
                Base::Vector3d center = circleSeg->getCenter();

                auto lineSeg = static_cast<const Part::GeomLineSegment*>(geo2);
                Base::Vector3d pnt1 = lineSeg->getStartPoint();
                Base::Vector3d pnt2 = lineSeg->getEndPoint();
                Base::Vector3d d = pnt2 - pnt1;
                double ActDist =
                    std::abs(-center.x * d.y + center.y * d.x + pnt1.x * pnt2.y - pnt2.x * pnt1.y)
                    / d.Length()
                    - radius;

                Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Distance',%d,%d,%f))",
                    GeoId1, GeoId2, ActDist);
            }
            else if (isCircle(*geo1) && isCircle(*geo2)) { // Circle - circle case
                auto circleSeg1 = static_cast<const Part::GeomCircle*>(geo1);
                double radius1 = circleSeg1->getRadius();
                Base::Vector3d center1 = circleSeg1->getCenter();

                auto circleSeg2 = static_cast<const Part::GeomCircle*>(geo2);
                double radius2 = circleSeg2->getRadius();
                Base::Vector3d center2 = circleSeg2->getCenter();

                double ActDist = 0.;

                Base::Vector3d intercenter = center1 - center2;
                double intercenterdistance = intercenter.Length();

                if (intercenterdistance >= radius1 && intercenterdistance >= radius2) {

                    ActDist = intercenterdistance - radius1 - radius2;
                }
                else {
                    double bigradius = std::max(radius1, radius2);
                    double smallradius = std::min(radius1, radius2);

                    ActDist = bigradius - smallradius - intercenterdistance;
                }

                Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Distance',%d,%d,%f))",
                    GeoId1, GeoId2, ActDist);
            }
        }
        else { //both points
            Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);
            Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);

            Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2), (pnt2 - pnt1).Length());
        }

        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
        if (arebothpointsorsegmentsfixed
            || GeoId1 <= Sketcher::GeoEnum::RefExt
            || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving
            Gui::cmdAppObjectArgs(Obj, "setDriving(%i,%s)", ConStr.size() - 1, "False");
        }

        numberOfConstraintsCreated++;
        moveConstraint(ConStr.size() - 1, onSketchPos);
    }

    void createDistanceXYConstrain(Sketcher::ConstraintType type, int GeoId1, Sketcher::PointPos PosId1, int GeoId2, Sketcher::PointPos PosId2, Base::Vector2d onSketchPos) {
        Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);
        Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);
        double ActLength = pnt2.x - pnt1.x;

        if (type == Sketcher::DistanceY) {
            ActLength = pnt2.y - pnt1.y;
        }

        //negative sign avoidance: swap the points to make value positive
        if (ActLength < -Precision::Confusion()) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
            std::swap(pnt1, pnt2);
            ActLength = -ActLength;
        }

        if (type == Sketcher::DistanceY) {
            Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
                GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2), ActLength);
        }
        else {
            Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f)) ",
                GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2), ActLength);
        }

        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
        if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2) || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving
            Gui::cmdAppObjectArgs(Obj, "setDriving(%i,%s)",
                ConStr.size() - 1, "False");
        }

        numberOfConstraintsCreated++;
        moveConstraint(ConStr.size() - 1, onSketchPos);
    }

    void createRadiusDiameterConstrain(int GeoId, Base::Vector2d onSketchPos) {
        double radius = 0.0;
        bool isCircleGeom = true;

        const Part::Geometry* geom = Obj->getGeometry(GeoId);

        if (geom && isArcOfCircle(*geom)) {
            auto arc = static_cast<const Part::GeomArcOfCircle*>(geom);
            radius = arc->getRadius();
            isCircleGeom = false;
        }
        else if (geom && isCircle(*geom)) {
            auto circle = static_cast<const Part::GeomCircle*>(geom);
            radius = circle->getRadius();
        }

        if (isBsplinePole(geom)) {
            Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Weight',%d,%f)) ",
                GeoId, radius);
        }
        else {
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning");
            bool dimensioningDiameter = hGrp->GetBool("DimensioningDiameter", true);
            bool dimensioningRadius = hGrp->GetBool("DimensioningRadius", true);

            bool firstCstr = true;
            if (availableConstraint != AvailableConstraint::FIRST) {
                firstCstr = false;
                if (!isArcOfCircle(*geom)) {
                    //This way if key is pressed again it goes back to FIRST
                    availableConstraint = AvailableConstraint::RESET;
                }
            }

            if ((firstCstr && dimensioningRadius && !dimensioningDiameter) ||
                (!firstCstr && !dimensioningRadius && dimensioningDiameter) ||
                (firstCstr && dimensioningRadius && dimensioningDiameter && !isCircleGeom) ||
                (!firstCstr && dimensioningRadius && dimensioningDiameter && isCircleGeom) ) {
                Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                    GeoId, radius);
            }
            else {
                Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Diameter',%d,%f)) ",
                    GeoId, radius * 2);
            }
        }

        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
        bool fixed = isPointOrSegmentFixed(Obj, GeoId);
        if (fixed || constraintCreationMode == Reference || GeoId <= Sketcher::GeoEnum::RefExt) {
            Gui::cmdAppObjectArgs(Obj, "setDriving(%i,%s)", ConStr.size() - 1, "False");
        }

        moveConstraint(ConStr.size() - 1, onSketchPos);

        numberOfConstraintsCreated ++;
    }

    bool createCoincidenceConstrain(int GeoId1, Sketcher::PointPos PosId1, int GeoId2, Sketcher::PointPos PosId2) {
        // check if the edge already has a Block constraint
        if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
            return false;
        }

        // check if this coincidence is already enforced (even indirectly)
        bool constraintExists = Obj->arePointsCoincident(GeoId1, PosId1, GeoId2, PosId2);
        if (!constraintExists && (GeoId1 != GeoId2)) {
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Coincident', %d, %d, %d, %d)) ",
                GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2));

            numberOfConstraintsCreated++;
            return true;
        }
        return false;
    }

    void createEqualityConstrain(int GeoId1, int GeoId2) {
        // check if the edge already has a Block constraint
        if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
            return;
        }

        const Part::Geometry* geo1 = Obj->getGeometry(GeoId1);
        const Part::Geometry* geo2 = Obj->getGeometry(GeoId2);

        if ((isLineSegment(*geo1) && ! isLineSegment(*geo2))
            || (isArcOfHyperbola(*geo1) && ! isArcOfHyperbola(*geo2))
            || (isArcOfParabola(*geo1) && ! isArcOfParabola(*geo2))
            || (isBsplinePole(geo1) && !isBsplinePole(geo2))
            || ((isCircle(*geo1) || isArcOfCircle(*geo1)) && !(isCircle(*geo2) || isArcOfCircle(*geo2)))
            || ((isEllipse(*geo1) || isArcOfEllipse(*geo1)) && !(isEllipse(*geo2) || isArcOfEllipse(*geo2)))) {

            Gui::TranslatedUserWarning(Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Select two or more edges of similar type."));
            return;
        }

        Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Equal',%d,%d)) ",
            GeoId1, GeoId2);
        numberOfConstraintsCreated++;
    }

    void createAngleConstrain(int GeoId1, int GeoId2, Base::Vector2d onSketchPos) {
        Sketcher::PointPos PosId1 = Sketcher::PointPos::none;
        Sketcher::PointPos PosId2 = Sketcher::PointPos::none;
        double ActAngle;

        if (calculateAngle(Obj, GeoId1, GeoId2, PosId1, PosId2, ActAngle)) {
            return;
        }


        if (ActAngle == 0.0) {
            //Here we are sure that GeoIds are lines. So false means that lines are parallel, we change to distance
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Distance constraint"));
            createDistanceConstrain(selLine[1].GeoId, Sketcher::PointPos::start, selLine[0].GeoId, selLine[0].PosId, onSketchPos);
            return;
        }

        Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Angle',%d,%d,%d,%d,%f)) ",
            GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2), ActAngle);

        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
        if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2) || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving

            Gui::cmdAppObjectArgs(Obj, "setDriving(%i,%s)", ConStr.size() - 1, "False");
        }
        numberOfConstraintsCreated++;
        moveConstraint(ConStr.size() - 1, onSketchPos);
    }

    void createArcAngleConstrain(int GeoId, Base::Vector2d onSketchPos) {
        const Part::Geometry* geom = Obj->getGeometry(GeoId);
        if (isArcOfCircle(*geom)) {

            const auto* arc = static_cast<const Part::GeomArcOfCircle*>(geom);
            double angle = arc->getAngle(/*EmulateCCWXY=*/true);

            Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Angle',%d,%f))",
                GeoId, angle);

            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
            if (isPointOrSegmentFixed(Obj, GeoId) || constraintCreationMode == Reference) {
                // it is a constraint on a external line, make it non-driving
                Gui::cmdAppObjectArgs(Obj, "setDriving(%i,%s)", ConStr.size() - 1, "False");
            }
            numberOfConstraintsCreated++;
            moveConstraint(ConStr.size() - 1, onSketchPos);
        }
    }

    void createVerticalConstrain(int GeoId1, Sketcher::PointPos PosId1, int GeoId2, Sketcher::PointPos PosId2) {
        if (selLine.size() == 1) {
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Vertical',%d)) ", GeoId1);
        }
        else { //2points
            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                return;
            }
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Vertical',%d,%d,%d,%d)) "
                , GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2));
        }
        numberOfConstraintsCreated++;
        tryAutoRecompute(Obj);
    }
    void createHorizontalConstrain(int GeoId1, Sketcher::PointPos PosId1, int GeoId2, Sketcher::PointPos PosId2) {
        if (selLine.size() == 1) {
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Horizontal',%d)) ", GeoId1);
        }
        else { //2points
            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                return;
            }
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Horizontal',%d,%d,%d,%d)) "
                , GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2));
        }
        numberOfConstraintsCreated++;
        tryAutoRecompute(Obj);
    }

    void createBlockConstrain(int GeoId) {
        Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Block',%d)) ", GeoId);

        numberOfConstraintsCreated++;
        tryAutoRecompute(Obj);
    }

    bool isHorizontalVerticalBlock(int GeoId) {
        const std::vector< Sketcher::Constraint* >& vals = Obj->Constraints.getValues();

        // check if the edge already has a Horizontal/Vertical/Block constraint
        for (const auto& constraint : vals) {
            if ((constraint->Type == Sketcher::Horizontal || constraint->Type == Sketcher::Vertical || constraint->Type == Sketcher::Block)
                && constraint->First == GeoId) {
                return true;
            }
        }
        return false;
    }

    void createSymmetryConstrain(int GeoId1, Sketcher::PointPos PosId1, int GeoId2, Sketcher::PointPos PosId2, int GeoId3, Sketcher::PointPos PosId3) {
        if (selPoints.size() == 2 && selLine.size() == 1) {
            if (isEdge(GeoId1, PosId1) && isVertex(GeoId3, PosId3)) {
                std::swap(GeoId1, GeoId3);
                std::swap(PosId1, PosId3);
            }
            else if (isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {
                std::swap(GeoId2, GeoId3);
                std::swap(PosId2, PosId3);
            }

            if (areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3)) {
                return;
            }

            const Part::Geometry* geom = Obj->getGeometry(GeoId3);

            if (isLineSegment(*geom)) {
                if (GeoId1 == GeoId2 && GeoId2 == GeoId3) {
                    Gui::TranslatedUserWarning(Obj,
                        QObject::tr("Wrong selection"),
                        QObject::tr("Cannot add a symmetry constraint between a line and its end points!"));
                    return;
                }

                Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d)) ",
                    GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2), GeoId3);

                numberOfConstraintsCreated++;
                tryAutoRecompute(Obj);
            }
        }
        else {
            if (selPoints.size() == 1 && selLine.size() == 1) { //1line 1 point
                if (GeoId1 == GeoId3) {
                    Gui::TranslatedUserWarning(Obj,
                        QObject::tr("Wrong selection"),
                        QObject::tr("Cannot add a symmetry constraint between a line and its end points!"));
                    return;
                }
                if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                    return;
                }
            }
            else {
                if (areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3)) {
                    return;
                }
            }
            Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d,%d)) ",
                GeoId1, static_cast<int>(PosId1), GeoId2, static_cast<int>(PosId2), GeoId3, static_cast<int>(PosId3));

            numberOfConstraintsCreated++;
            tryAutoRecompute(Obj);
        }
    }

    void updateDistanceType(Base::Vector2d onSketchPos)
    {
        const std::vector< Sketcher::Constraint* >& vals = Obj->Constraints.getValues();
        Sketcher::ConstraintType type = vals[vals.size() - 1]->Type;

        Base::Vector3d pnt1, pnt2;
        bool addedOrigin = false;
        if (selPoints.size() == 1) {
            //Case of single point selected, for distance constraint. We add temporarily the origin in the vector.
            addedOrigin = true;
            SelIdPair selIdPair;
            selIdPair.GeoId = Sketcher::GeoEnum::RtPnt;
            selIdPair.PosId = Sketcher::PointPos::start;
            selPoints.push_back(selIdPair);
        }

        if (selLine.size() == 1) {
            pnt1 = Obj->getPoint(selLine[0].GeoId, Sketcher::PointPos::start);
            pnt2 = Obj->getPoint(selLine[0].GeoId, Sketcher::PointPos::end);
        }
        else {
            pnt1 = Obj->getPoint(selPoints[0].GeoId, selPoints[0].PosId);
            pnt2 = Obj->getPoint(selPoints[1].GeoId, selPoints[1].PosId);
        }

        double minX, minY, maxX, maxY;
        minX = min(pnt1.x, pnt2.x);
        maxX = max(pnt1.x, pnt2.x);
        minY = min(pnt1.y, pnt2.y);
        maxY = max(pnt1.y, pnt2.y);
        if (onSketchPos.x > minX && onSketchPos.x < maxX
            && (onSketchPos.y < minY || onSketchPos.y > maxY) && type != Sketcher::DistanceX) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceX constraint"));
            specialConstraint = SpecialConstraint::LineOr2PointsDistance;
            if (selLine.size() == 1) {
                createDistanceXYConstrain(Sketcher::DistanceX, selLine[0].GeoId, Sketcher::PointPos::start, selLine[0].GeoId, Sketcher::PointPos::end, onSketchPos);
            }
            else {
                createDistanceXYConstrain(Sketcher::DistanceX, selPoints[0].GeoId, selPoints[0].PosId, selPoints[1].GeoId, selPoints[1].PosId, onSketchPos);
            }
        }
        else if (onSketchPos.y > minY && onSketchPos.y < maxY
            && (onSketchPos.x < minX || onSketchPos.x > maxX) && type != Sketcher::DistanceY) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add DistanceY constraint"));
            specialConstraint = SpecialConstraint::LineOr2PointsDistance;
            if (selLine.size() == 1) {
                createDistanceXYConstrain(Sketcher::DistanceY, selLine[0].GeoId, Sketcher::PointPos::start, selLine[0].GeoId, Sketcher::PointPos::end, onSketchPos);
            }
            else {
                createDistanceXYConstrain(Sketcher::DistanceY, selPoints[0].GeoId, selPoints[0].PosId, selPoints[1].GeoId, selPoints[1].PosId, onSketchPos);
            }
        }
        else if ((((onSketchPos.y < minY || onSketchPos.y > maxY) && (onSketchPos.x < minX || onSketchPos.x > maxX))
            || (onSketchPos.y > minY && onSketchPos.y < maxY && onSketchPos.x > minX && onSketchPos.x < maxX)) && type != Sketcher::Distance) {
            restartCommand(QT_TRANSLATE_NOOP("Command", "Add Distance constraint"));
            if (selLine.size() == 1) {
                createDistanceConstrain(selLine[0].GeoId, Sketcher::PointPos::start, selLine[0].GeoId, Sketcher::PointPos::end, onSketchPos);
            }
            else {
                createDistanceConstrain(selPoints[0].GeoId, selPoints[0].PosId, selPoints[1].GeoId, selPoints[1].PosId, onSketchPos);
            }
        }

        if (addedOrigin) {
            //remove origin
            selPoints.pop_back();
        }
    }

    void restartCommand(const char* cstrName) {
        specialConstraint = SpecialConstraint::None;
        Gui::Command::abortCommand();
        Obj->solve();
        sketchgui->draw(false, false); // Redraw
        Gui::Command::openCommand(cstrName);

        numberOfConstraintsCreated = 0;
    }
};

DEF_STD_CMD_AU(CmdSketcherDimension)

CmdSketcherDimension::CmdSketcherDimension()
    : Command("Sketcher_Dimension")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Dimension");
    sToolTipText = QT_TR_NOOP("Constrain contextually based on your selection.\n"
        "Depending on your selection you might have several constraints available. You can cycle through them using M key.\n"
        "Left clicking on empty space will validate the current constraint. Right clicking or pressing Esc will cancel.");
    sWhatsThis = "Sketcher_Dimension";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Dimension";
    sAccel = "D";
    eType = ForEdit;
}

void CmdSketcherDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::AutoTransaction::setEnable(false);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    std::vector<std::string> SubNames = {};

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() == 1 && selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        SubNames = selection[0].getSubNames();
    }

    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerDimension(SubNames));
}

void CmdSketcherDimension::updateAction(int mode)
{
    switch (mode) {
    case Reference:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Dimension_Driven"));
        break;
    case Driving:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Dimension"));
        break;
    }
}

bool CmdSketcherDimension::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


// ============================================================================

class CmdSketcherConstrainHorizontal: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainHorizontal();
    ~CmdSketcherConstrainHorizontal() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainHorizontal";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainHorizontal::CmdSketcherConstrainHorizontal()
    : CmdSketcherConstraint("Sketcher_ConstrainHorizontal")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain horizontally");
    sToolTipText = QT_TR_NOOP("Create a horizontal constraint on the selected item");
    sWhatsThis = "Sketcher_ConstrainHorizontal";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Horizontal";
    sAccel = "H";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge}, {SelVertex, SelVertexOrRoot}, {SelRoot, SelVertex}};
}

void CmdSketcherConstrainHorizontal::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select an edge from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    std::vector<int> edgegeoids;
    std::vector<int> pointgeoids;
    std::vector<Sketcher::PointPos> pointpos;

    int fixedpoints = 0;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName((*it), Obj, GeoId, PosId);

        if (isEdge(GeoId, PosId)) {// it is an edge
            const Part::Geometry* geo = Obj->getGeometry(GeoId);

            if (! isLineSegment(*geo)) {
                Gui::TranslatedUserWarning(Obj,
                                           QObject::tr("Impossible constraint"),
                                           QObject::tr("The selected edge is not a line segment."));
                return;
            }

            // check if the edge already has a Horizontal/Vertical/Block constraint
            for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                 it != vals.end();
                 ++it) {
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == GeoId
                    && (*it)->FirstPos == Sketcher::PointPos::none) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Double constraint"),
                        QObject::tr("The selected edge already has a horizontal constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == GeoId
                    && (*it)->FirstPos == Sketcher::PointPos::none) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Impossible constraint"),
                        QObject::tr("The selected edge already has a vertical constraint!"));
                    return;
                }
                // check if the edge already has a Block constraint
                if ((*it)->Type == Sketcher::Block && (*it)->First == GeoId
                    && (*it)->FirstPos == Sketcher::PointPos::none) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Impossible constraint"),
                        QObject::tr("The selected edge already has a Block constraint!"));
                    return;
                }
            }
            edgegeoids.push_back(GeoId);
        }
        else if (isVertex(GeoId, PosId)) {
            // can be a point, a construction point, an external point or root

            if (isPointOrSegmentFixed(Obj, GeoId)) {
                fixedpoints++;
            }

            pointgeoids.push_back(GeoId);
            pointpos.push_back(PosId);
        }
    }

    if (edgegeoids.empty() && pointgeoids.empty()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Impossible constraint"),
            QObject::tr("The selected item(s) can't accept a horizontal constraint!"));
        return;
    }

    // if there is at least one edge selected, ignore the point alignment functionality
    if (!edgegeoids.empty()) {
        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal constraint"));
        for (std::vector<int>::iterator it = edgegeoids.begin(); it != edgegeoids.end(); it++) {
            // issue the actual commands to create the constraint
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Horizontal',%d))",
                                  *it);
        }
    }
    else if (fixedpoints <= 1) {// pointgeoids
        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal alignment"));
        std::vector<int>::iterator it;
        std::vector<Sketcher::PointPos>::iterator itp;
        for (it = pointgeoids.begin(), itp = pointpos.begin();
             it != std::prev(pointgeoids.end()) && itp != std::prev(pointpos.end());
             it++, itp++) {
            // issue the actual commands to create the constraint
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Horizontal',%d,%d,%d,%d))",
                                  *it,
                                  static_cast<int>(*itp),
                                  *std::next(it),
                                  static_cast<int>(*std::next(itp)));
        }
    }
    else {// vertex mode, fixedpoints > 1
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Impossible constraint"),
                                   QObject::tr("There are more than one fixed points selected. "
                                               "Select a maximum of one fixed point!"));
        return;
    }
    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainHorizontal::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    switch (seqIndex) {
        case 0:// {Edge}
        {
            // create the constraint
            const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

            int CrvId = selSeq.front().GeoId;
            if (CrvId != -1) {
                const Part::Geometry* geo = Obj->getGeometry(CrvId);

                if (! isLineSegment(*geo)) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Impossible constraint"),
                        QObject::tr("The selected edge is not a line segment."));
                    return;
                }

                // check if the edge already has a Horizontal/Vertical/Block constraint
                for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                     it != vals.end();
                     ++it) {
                    if ((*it)->Type == Sketcher::Horizontal && (*it)->First == CrvId
                        && (*it)->FirstPos == Sketcher::PointPos::none) {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Double constraint"),
                            QObject::tr("The selected edge already has a horizontal constraint!"));
                        return;
                    }
                    if ((*it)->Type == Sketcher::Vertical && (*it)->First == CrvId
                        && (*it)->FirstPos == Sketcher::PointPos::none) {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Impossible constraint"),
                            QObject::tr("The selected edge already has a vertical constraint!"));
                        return;
                    }
                    // check if the edge already has a Block constraint
                    if ((*it)->Type == Sketcher::Block && (*it)->First == CrvId
                        && (*it)->FirstPos == Sketcher::PointPos::none) {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Impossible constraint"),
                            QObject::tr("The selected edge already has a Block constraint!"));
                        return;
                    }
                }

                // undo command open
                Gui::Command::openCommand(
                    QT_TRANSLATE_NOOP("Command", "Add horizontal constraint"));
                // issue the actual commands to create the constraint
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addConstraint(Sketcher.Constraint('Horizontal',%d))",
                                      CrvId);
                // finish the transaction and update
                Gui::Command::commitCommand();

                tryAutoRecompute(Obj);
            }

            break;
        }

        case 1:// {SelVertex, SelVertexOrRoot}
        case 2:// {SelRoot, SelVertex}
        {
            int GeoId1, GeoId2;
            Sketcher::PointPos PosId1, PosId2;
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(1).PosId;

            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }

            // undo command open
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal alignment"));
            // issue the actual commands to create the constraint
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "addConstraint(Sketcher.Constraint('Horizontal',%d,%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2));
            // finish the transaction and update
            Gui::Command::commitCommand();

            tryAutoRecompute(Obj);

            break;
        }
    }
}

// ================================================================================

class CmdSketcherConstrainVertical: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainVertical();
    ~CmdSketcherConstrainVertical() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainVertical";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainVertical::CmdSketcherConstrainVertical()
    : CmdSketcherConstraint("Sketcher_ConstrainVertical")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain vertically");
    sToolTipText = QT_TR_NOOP("Create a vertical constraint on the selected item");
    sWhatsThis = "Sketcher_ConstrainVertical";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Vertical";
    sAccel = "V";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge}, {SelVertex, SelVertexOrRoot}, {SelRoot, SelVertex}};
}

void CmdSketcherConstrainVertical::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select an edge from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    std::vector<int> edgegeoids;
    std::vector<int> pointgeoids;
    std::vector<Sketcher::PointPos> pointpos;

    int fixedpoints = 0;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName((*it), Obj, GeoId, PosId);

        if (isEdge(GeoId, PosId)) {// it is an edge
            const Part::Geometry* geo = Obj->getGeometry(GeoId);

            if (! isLineSegment(*geo)) {
                Gui::TranslatedUserWarning(Obj,
                                           QObject::tr("Impossible constraint"),
                                           QObject::tr("The selected edge is not a line segment."));
                return;
            }

            // check if the edge already has a Horizontal/Vertical/Block constraint
            for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                 it != vals.end();
                 ++it) {
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == GeoId
                    && (*it)->FirstPos == Sketcher::PointPos::none) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Double constraint"),
                        QObject::tr("The selected edge already has a horizontal constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == GeoId
                    && (*it)->FirstPos == Sketcher::PointPos::none) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Impossible constraint"),
                        QObject::tr("The selected edge already has a horizontal constraint!"));
                    return;
                }
                // check if the edge already has a Block constraint
                if ((*it)->Type == Sketcher::Block && (*it)->First == GeoId
                    && (*it)->FirstPos == Sketcher::PointPos::none) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Impossible constraint"),
                        QObject::tr("The selected edge already has a Block constraint!"));
                    return;
                }
            }
            edgegeoids.push_back(GeoId);
        }
        else if (isVertex(GeoId, PosId)) {
            // can be a point, a construction point, an external point, root or a blocked geometry
            if (isPointOrSegmentFixed(Obj, GeoId)) {
                fixedpoints++;
            }

            pointgeoids.push_back(GeoId);
            pointpos.push_back(PosId);
        }
    }

    if (edgegeoids.empty() && pointgeoids.empty()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Impossible constraint"),
            QObject::tr("The selected item(s) can't accept a vertical constraint!"));
        return;
    }

    // if there is at least one edge selected, ignore the point alignment functionality
    if (!edgegeoids.empty()) {
        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Add vertical constraint"));
        for (std::vector<int>::iterator it = edgegeoids.begin(); it != edgegeoids.end(); it++) {
            // issue the actual commands to create the constraint
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Vertical',%d))",
                                  *it);
        }
    }
    else if (fixedpoints <= 1) {// vertex mode, maximum one fixed point
        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Add vertical alignment"));
        std::vector<int>::iterator it;
        std::vector<Sketcher::PointPos>::iterator itp;
        for (it = pointgeoids.begin(), itp = pointpos.begin();
             it != std::prev(pointgeoids.end()) && itp != std::prev(pointpos.end());
             it++, itp++) {
            // issue the actual commands to create the constraint
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Vertical',%d,%d,%d,%d))",
                                  *it,
                                  static_cast<int>(*itp),
                                  *std::next(it),
                                  static_cast<int>(*std::next(itp)));
        }
    }
    else {// vertex mode, fixedpoints > 1
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Impossible constraint"),
                                   QObject::tr("There are more than one fixed points selected. "
                                               "Select a maximum of one fixed point!"));
        return;
    }

    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainVertical::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    switch (seqIndex) {
        case 0:// {Edge}
        {
            // create the constraint
            const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

            int CrvId = selSeq.front().GeoId;
            if (CrvId != -1) {
                const Part::Geometry* geo = Obj->getGeometry(CrvId);

                if (! isLineSegment(*geo)) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Impossible constraint"),
                        QObject::tr("The selected edge is not a line segment."));
                    return;
                }

                // check if the edge already has a Horizontal or Vertical constraint
                for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                     it != vals.end();
                     ++it) {
                    if ((*it)->Type == Sketcher::Horizontal && (*it)->First == CrvId
                        && (*it)->FirstPos == Sketcher::PointPos::none) {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Impossible constraint"),
                            QObject::tr("The selected edge already has a horizontal constraint!"));
                        return;
                    }
                    if ((*it)->Type == Sketcher::Vertical && (*it)->First == CrvId
                        && (*it)->FirstPos == Sketcher::PointPos::none) {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Double constraint"),
                            QObject::tr("The selected edge already has a vertical constraint!"));
                        return;
                    }
                    // check if the edge already has a Block constraint
                    if ((*it)->Type == Sketcher::Block && (*it)->First == CrvId
                        && (*it)->FirstPos == Sketcher::PointPos::none) {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Impossible constraint"),
                            QObject::tr("The selected edge already has a Block constraint!"));
                        return;
                    }
                }

                // undo command open
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add vertical constraint"));
                // issue the actual commands to create the constraint
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addConstraint(Sketcher.Constraint('Vertical',%d))",
                                      CrvId);
                // finish the transaction and update
                Gui::Command::commitCommand();
                tryAutoRecompute(Obj);
            }

            break;
        }

        case 1:// {SelVertex, SelVertexOrRoot}
        case 2:// {SelRoot, SelVertex}
        {
            int GeoId1, GeoId2;
            Sketcher::PointPos PosId1, PosId2;
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(1).PosId;

            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }

            // undo command open
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add horizontal alignment"));
            // issue the actual commands to create the constraint
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "addConstraint(Sketcher.Constraint('Vertical',%d,%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2));
            // finish the transaction and update
            Gui::Command::commitCommand();

            tryAutoRecompute(Obj);

            break;
        }
    }
}

// ======================================================================================

class CmdSketcherConstrainLock: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainLock();
    ~CmdSketcherConstrainLock() override
    {}
    void updateAction(int mode) override;
    const char* className() const override
    {
        return "CmdSketcherConstrainLock";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainLock::CmdSketcherConstrainLock()
    : CmdSketcherConstraint("Sketcher_ConstrainLock")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain lock");
    sToolTipText = QT_TR_NOOP("Create both a horizontal "
                              "and a vertical distance constraint\n"
                              "on the selected vertex");
    sWhatsThis = "Sketcher_ConstrainLock";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Lock";
    sAccel = "K, L";
    eType = ForEdit;

    allowedSelSequences = {{SelVertex}};
}

void CmdSketcherConstrainLock::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select vertices from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    std::vector<int> GeoId;
    std::vector<Sketcher::PointPos> PosId;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        int GeoIdt;
        Sketcher::PointPos PosIdt;
        getIdsFromName((*it), Obj, GeoIdt, PosIdt);
        GeoId.push_back(GeoIdt);
        PosId.push_back(PosIdt);

        if ((it != std::prev(SubNames.end())
             && (isEdge(GeoIdt, PosIdt) || (GeoIdt < 0 && GeoIdt >= Sketcher::GeoEnum::VAxis)))
            || (it == std::prev(SubNames.end()) && isEdge(GeoIdt, PosIdt))) {
            if (selection.size() == 1) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select one vertex from the sketch other than the origin."));
            }
            else {
                Gui::TranslatedUserWarning(Obj,
                                           QObject::tr("Wrong selection"),
                                           QObject::tr("Select only vertices from the sketch. The "
                                                       "last selected vertex may be the origin."));
            }
            // clear the selection (convenience)
            getSelection().clearSelection();
            return;
        }
    }

    int lastconstraintindex = Obj->Constraints.getSize() - 1;

    if (GeoId.size() == 1) {// absolute mode
        // check if the edge already has a Block constraint
        bool edgeisblocked = false;

        if (isPointOrSegmentFixed(Obj, GeoId[0])) {
            edgeisblocked = true;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId[0], PosId[0]);

        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Add 'Lock' constraint"));
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f))",
                              GeoId[0],
                              static_cast<int>(PosId[0]),
                              pnt.x);
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f))",
                              GeoId[0],
                              static_cast<int>(PosId[0]),
                              pnt.y);

        lastconstraintindex += 2;

        if (edgeisblocked || GeoId[0] <= Sketcher::GeoEnum::RefExt
            || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "setDriving(%d,%s)",
                                  lastconstraintindex - 1,
                                  "False");

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "setDriving(%d,%s)",
                                  lastconstraintindex,
                                  "False");
        }
    }
    else {
        std::vector<int>::const_iterator itg;
        std::vector<Sketcher::PointPos>::const_iterator itp;

        Base::Vector3d pntr = Obj->getPoint(GeoId.back(), PosId.back());

        // check if the edge already has a Block constraint
        bool refpointfixed = false;

        if (isPointOrSegmentFixed(Obj, GeoId.back())) {
            refpointfixed = true;
        }

        for (itg = GeoId.begin(), itp = PosId.begin();
             itg != std::prev(GeoId.end()) && itp != std::prev(PosId.end());
             ++itp, ++itg) {
            bool pointfixed = false;

            if (isPointOrSegmentFixed(Obj, *itg)) {
                pointfixed = true;
            }

            Base::Vector3d pnt = Obj->getPoint(*itg, *itp);

            // undo command open
            openCommand(QT_TRANSLATE_NOOP("Command", "Add relative 'Lock' constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f))",
                                  *itg,
                                  static_cast<int>(*itp),
                                  GeoId.back(),
                                  static_cast<int>(PosId.back()),
                                  pntr.x - pnt.x);

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f))",
                                  *itg,
                                  static_cast<int>(*itp),
                                  GeoId.back(),
                                  static_cast<int>(PosId.back()),
                                  pntr.y - pnt.y);
            lastconstraintindex += 2;

            if ((refpointfixed && pointfixed) || constraintCreationMode == Reference) {
                // it is a constraint on a external line, make it non-driving

                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "setDriving(%d,%s)",
                                      lastconstraintindex - 1,
                                      "False");

                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "setDriving(%d,%s)",
                                      lastconstraintindex,
                                      "False");
            }
        }
    }

    // finish the transaction and update
    commitCommand();
    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainLock::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    switch (seqIndex) {
        case 0:// {Vertex}
            // Create the constraints
            SketcherGui::ViewProviderSketch* sketchgui =
                static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
            Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

            // check if the edge already has a Block constraint
            bool pointfixed = false;

            if (isPointOrSegmentFixed(Obj, selSeq.front().GeoId)) {
                pointfixed = true;
            }

            Base::Vector3d pnt = Obj->getPoint(selSeq.front().GeoId, selSeq.front().PosId);

            // undo command open
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add fixed constraint"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "addConstraint(Sketcher.Constraint('DistanceX', %d, %d, %f))",
                                  selSeq.front().GeoId,
                                  static_cast<int>(selSeq.front().PosId),
                                  pnt.x);
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "addConstraint(Sketcher.Constraint('DistanceY', %d, %d, %f))",
                                  selSeq.front().GeoId,
                                  static_cast<int>(selSeq.front().PosId),
                                  pnt.y);

            if (pointfixed || constraintCreationMode == Reference) {
                // it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "setDriving(%d, %s)",
                                      ConStr.size() - 2,
                                      "False");

                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "setDriving(%d, %s)",
                                      ConStr.size() - 1,
                                      "False");
            }

            // finish the transaction and update
            Gui::Command::commitCommand();

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool autoRecompute = hGrp->GetBool("AutoRecompute", false);

            if (autoRecompute) {
                Gui::Command::updateActive();
            }
            break;
    }
}

void CmdSketcherConstrainLock::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction()) {
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Lock_Driven"));
            }
            break;
        case Driving:
            if (getAction()) {
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Lock"));
            }
            break;
    }
}

// ======================================================================================

class CmdSketcherConstrainBlock: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainBlock();
    ~CmdSketcherConstrainBlock() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainBlock";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainBlock::CmdSketcherConstrainBlock()
    : CmdSketcherConstraint("Sketcher_ConstrainBlock")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain block");
    sToolTipText = QT_TR_NOOP("Block the selected edge from moving");
    sWhatsThis = "Sketcher_ConstrainBlock";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Block";
    sAccel = "K, B";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge}};
}

void CmdSketcherConstrainBlock::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select vertices from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // Check that the solver does not report redundant/conflicting constraints
    if (Obj->getLastSolverStatus() != GCS::Success || Obj->getLastHasConflicts()
        || Obj->getLastHasRedundancies()) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong solver status"),
                                   QObject::tr("A Block constraint cannot be added "
                                               "if the sketch is unsolved "
                                               "or there are redundant and "
                                               "conflicting constraints."));
        return;
    }

    std::vector<int> GeoId;
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        int GeoIdt;
        Sketcher::PointPos PosIdt;
        getIdsFromName((*it), Obj, GeoIdt, PosIdt);

        if (isVertex(GeoIdt, PosIdt) || GeoIdt < 0) {
            if (selection.size() == 1) {
                Gui::TranslatedUserWarning(Obj,
                                           QObject::tr("Wrong selection"),
                                           QObject::tr("Select one edge from the sketch."));
            }
            else {
                Gui::TranslatedUserWarning(Obj,
                                           QObject::tr("Wrong selection"),
                                           QObject::tr("Select only edges from the sketch."));
            }
            // clear the selection
            getSelection().clearSelection();
            return;
        }

        // check if the edge already has a Block constraint
        if (checkConstraint(vals, Sketcher::Block, GeoIdt, Sketcher::PointPos::none)) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Double constraint"),
                QObject::tr("The selected edge already has a Block constraint!"));
            return;
        }

        GeoId.push_back(GeoIdt);
    }

    for (std::vector<int>::iterator itg = GeoId.begin(); itg != GeoId.end(); ++itg) {
        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Add 'Block' constraint"));

        bool safe = addConstraintSafely(Obj, [&]() {
            Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Block',%d))", (*itg));
        });

        if (!safe) {
            return;
        }
        else {
            commitCommand();
            tryAutoRecompute(Obj);
        }
    }

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainBlock::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    switch (seqIndex) {
        case 0:// {Edge}
        {
            // Create the constraints
            SketcherGui::ViewProviderSketch* sketchgui =
                static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());

            auto Obj = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());

            // check if the edge already has a Block constraint
            const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

            if (checkConstraint(vals,
                                Sketcher::Block,
                                selSeq.front().GeoId,
                                Sketcher::PointPos::none)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Double constraint"),
                    QObject::tr("The selected edge already has a Block constraint!"));
                return;
            }

            // undo command open
            openCommand(QT_TRANSLATE_NOOP("Command", "Add block constraint"));

            bool safe = addConstraintSafely(Obj, [&]() {
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addConstraint(Sketcher.Constraint('Block',%d))",
                                      selSeq.front().GeoId);
            });

            if (!safe) {
                return;
            }
            else {
                commitCommand();
                tryAutoRecompute(Obj);
            }
        } break;
        default:
            break;
    }
}


// ======================================================================================

/* XPM */
static const char* cursor_createcoincident[] = {"32 32 3 1",
                                                "+ c white",
                                                "# c red",
                                                ". c None",
                                                "......+.........................",
                                                "......+.........................",
                                                "......+.........................",
                                                "......+.........................",
                                                "......+.........................",
                                                "................................",
                                                "+++++...+++++...................",
                                                "................................",
                                                "......+.........................",
                                                "......+.........................",
                                                "......+.........................",
                                                "......+.........................",
                                                "......+.........................",
                                                "................................",
                                                "................................",
                                                "................................",
                                                ".................####...........",
                                                "................######..........",
                                                "...............########.........",
                                                "...............########.........",
                                                "...............########.........",
                                                "...............########.........",
                                                "................######..........",
                                                ".................####...........",
                                                "................................",
                                                "................................",
                                                "................................",
                                                "................................",
                                                "................................",
                                                "................................",
                                                "................................",
                                                "................................"};

class DrawSketchHandlerCoincident: public DrawSketchHandler
{
public:
    DrawSketchHandlerCoincident()
    {
        GeoId1 = GeoId2 = GeoEnum::GeoUndef;
        PosId1 = PosId2 = Sketcher::PointPos::none;
    }
    ~DrawSketchHandlerCoincident() override
    {
        Gui::Selection().rmvSelectionGate();
    }

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        int VtId = getPreselectPoint();
        int CrsId = getPreselectCross();
        std::stringstream ss;
        int GeoId_temp;
        Sketcher::PointPos PosId_temp;

        if (VtId != -1) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, GeoId_temp, PosId_temp);
            ss << "Vertex" << VtId + 1;
        }
        else if (CrsId == 0) {
            GeoId_temp = Sketcher::GeoEnum::RtPnt;
            PosId_temp = Sketcher::PointPos::start;
            ss << "RootPoint";
        }
        else {
            GeoId1 = GeoId2 = GeoEnum::GeoUndef;
            PosId1 = PosId2 = Sketcher::PointPos::none;
            Gui::Selection().clearSelection();

            return true;
        }

        if (GeoId1 == GeoEnum::GeoUndef) {
            GeoId1 = GeoId_temp;
            PosId1 = PosId_temp;
            Gui::Selection().addSelection(sketchgui->getSketchObject()->getDocument()->getName(),
                                          sketchgui->getSketchObject()->getNameInDocument(),
                                          ss.str().c_str(),
                                          onSketchPos.x,
                                          onSketchPos.y,
                                          0.f);
        }
        else {
            GeoId2 = GeoId_temp;
            PosId2 = PosId_temp;

            // Apply the constraint
            Sketcher::SketchObject* Obj =
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject());

            // undo command open
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add coincident constraint"));

            // check if this coincidence is already enforced (even indirectly)
            bool constraintExists = Obj->arePointsCoincident(GeoId1, PosId1, GeoId2, PosId2);
            if (!constraintExists && (GeoId1 != GeoId2)) {
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d))",
                    GeoId1,
                    static_cast<int>(PosId1),
                    GeoId2,
                    static_cast<int>(PosId2));
                Gui::Command::commitCommand();
            }
            else {
                Gui::Command::abortCommand();
            }
        }

        return true;
    }

private:
    void activated() override
    {
        Gui::Selection().rmvSelectionGate();
        GenericConstraintSelection* selFilterGate =
            new GenericConstraintSelection(sketchgui->getObject());
        selFilterGate->setAllowedSelTypes(SelVertex | SelRoot);
        Gui::Selection().addSelectionGate(selFilterGate);
        int hotX = 8;
        int hotY = 8;
        setCursor(QPixmap(cursor_createcoincident), hotX, hotY);
    }

protected:
    int GeoId1, GeoId2;
    Sketcher::PointPos PosId1, PosId2;
};

class CmdSketcherConstrainCoincident: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainCoincident();
    ~CmdSketcherConstrainCoincident() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainCoincident";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
    // returns true if a substitution took place
    static bool substituteConstraintCombinations(SketchObject* Obj,
                                                 int GeoId1,
                                                 PointPos PosId1,
                                                 int GeoId2,
                                                 PointPos PosId2);
};

CmdSketcherConstrainCoincident::CmdSketcherConstrainCoincident()
    : CmdSketcherConstraint("Sketcher_ConstrainCoincident")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain coincident");
    sToolTipText = QT_TR_NOOP("Create a coincident constraint between points, or a concentric "
                              "constraint between circles, arcs, and ellipses");
    sWhatsThis = "Sketcher_ConstrainCoincident";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_PointOnPoint";
    sAccel = "C";
    eType = ForEdit;

    allowedSelSequences = {{SelVertex, SelVertexOrRoot},
                           {SelRoot, SelVertex},
                           {SelEdge, SelEdge},
                           {SelEdge, SelExternalEdge},
                           {SelExternalEdge, SelEdge}};
}

bool CmdSketcherConstrainCoincident::substituteConstraintCombinations(SketchObject* Obj,
                                                                      int GeoId1,
                                                                      PointPos PosId1,
                                                                      int GeoId2,
                                                                      PointPos PosId2)
{
    // checks for direct and indirect coincidence constraints
    bool constraintExists = Obj->arePointsCoincident(GeoId1, PosId1, GeoId2, PosId2);

    const std::vector<Constraint*>& cvals = Obj->Constraints.getValues();

    // NOTE: This function does not either open or commit a command as it is used for group addition
    // it relies on such infrastructure being provided by the caller.

    int j = 0;
    for (std::vector<Constraint*>::const_iterator it = cvals.begin(); it != cvals.end();
         ++it, ++j) {
        if ((*it)->Type == Sketcher::Tangent && (*it)->Third == GeoEnum::GeoUndef
            && (((*it)->First == GeoId1 && (*it)->Second == GeoId2)
                || ((*it)->Second == GeoId1 && (*it)->First == GeoId2))) {
            if ((*it)->FirstPos == Sketcher::PointPos::none
                && (*it)->SecondPos == Sketcher::PointPos::none) {

                if (constraintExists) {
                    // try to remove any pre-existing direct coincident constraints
                    Gui::cmdAppObjectArgs(Obj,
                                          "delConstraintOnPoint(%d,%d)",
                                          GeoId1,
                                          static_cast<int>(PosId1));
                }

                Gui::cmdAppObjectArgs(Obj, "delConstraint(%d)", j);

                doEndpointTangency(Obj, GeoId1, GeoId2, PosId1, PosId2);

                notifyConstraintSubstitutions(
                    QObject::tr("Endpoint to endpoint tangency was applied instead."));

                getSelection().clearSelection();
                return true;
            }
            else if (isBsplineKnot(Obj, GeoId1) != isBsplineKnot(Obj, GeoId2)) {
                // Replace with knot-to-endpoint tangency

                if (isBsplineKnot(Obj, GeoId2)) {
                    std::swap(GeoId1, GeoId2);
                    std::swap(PosId1, PosId2);
                }

                // if a similar tangency already exists this must result in bad constraints
                if ((*it)->SecondPos == Sketcher::PointPos::none) {
                    Gui::cmdAppObjectArgs(Obj, "delConstraint(%d)", j);

                    doEndpointTangency(Obj, GeoId1, GeoId2, PosId1, PosId2);

                    notifyConstraintSubstitutions(
                        QObject::tr("B-spline knot to endpoint tangency was applied instead."));

                    getSelection().clearSelection();
                    return true;
                }
            }
        }
    }

    return false;
}

void CmdSketcherConstrainCoincident::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select two or more points from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() < 2) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select two or more vertices from the sketch."));
        return;
    }

    bool allConicsEdges = true;// If user selects only conics (circle, ellipse, arc, arcOfEllipse)
                               // then we make concentric constraint.
    bool atLeastOneEdge = false;
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(*it, Obj, GeoId, PosId);
        if (isEdge(GeoId, PosId)) {
            atLeastOneEdge = true;
            if (!isGeoConcentricCompatible(Obj->getGeometry(GeoId))) {
                allConicsEdges = false;
            }
        }
        else {
            allConicsEdges = false;// at least one point is selected, so concentric can't be
                                   // applied.
        }

        if (atLeastOneEdge && !allConicsEdges) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Select two or more vertices from the sketch for a coincident "
                            "constraint, or two or more circles, ellipses, arcs or arcs of ellipse "
                            "for a concentric constraint."));
            return;
        }
    }

    int GeoId1, GeoId2;
    Sketcher::PointPos PosId1, PosId2;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (allConicsEdges) {
        PosId1 = Sketcher::PointPos::mid;
    }

    // undo command open
    bool constraintsAdded = false;
    openCommand(QT_TRANSLATE_NOOP("Command", "Add coincident constraint"));
    for (std::size_t i = 1; i < SubNames.size(); i++) {
        getIdsFromName(SubNames[i], Obj, GeoId2, PosId2);
        if (allConicsEdges) {
            PosId2 = Sketcher::PointPos::mid;
        }

        // check if the edge already has a Block constraint
        if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
            showNoConstraintBetweenFixedGeometry(Obj);
            return;
        }

        // check if as a consequence of this command undesirable combinations of constraints would
        // arise and substitute them with more appropriate counterparts, examples:
        // - coincidence + tangency on edge
        // - point on object + tangency on edge
        if (substituteConstraintCombinations(Obj, GeoId1, PosId1, GeoId2, PosId2)) {
            constraintsAdded = true;
            break;
        }

        // check if this coincidence is already enforced (even indirectly)
        bool constraintExists = Obj->arePointsCoincident(GeoId1, PosId1, GeoId2, PosId2);

        if (!constraintExists) {
            constraintsAdded = true;
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2));
        }
    }

    // finish or abort the transaction and update
    if (constraintsAdded) {
        commitCommand();
    }
    else {
        abortCommand();
    }

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainCoincident::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = selSeq.at(0).GeoId, GeoId2 = selSeq.at(1).GeoId;
    Sketcher::PointPos PosId1 = selSeq.at(0).PosId, PosId2 = selSeq.at(1).PosId;

    switch (seqIndex) {
        case 0:// {SelVertex, SelVertexOrRoot}
        case 1:// {SelRoot, SelVertex}
            // Nothing specific.
            break;
        case 2:// {SelEdge, SelEdge}
        case 3:// {SelEdge, SelExternalEdge}
        case 4:// {SelExternalEdge, SelEdge}
            // Concentric for circles, ellipse, arc, arcofEllipse only.
            if (!isGeoConcentricCompatible(Obj->getGeometry(GeoId1))
                || !isGeoConcentricCompatible(Obj->getGeometry(GeoId2))) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr(
                        "Select two vertices from the sketch for a coincident constraint, or two "
                        "circles, ellipses, arcs or arcs of ellipse for a concentric constraint."));
                return;
            }
            PosId1 = Sketcher::PointPos::mid;
            PosId2 = Sketcher::PointPos::mid;
            break;
    }

    // check if the edge already has a Block constraint
    if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
        showNoConstraintBetweenFixedGeometry(Obj);
        return;
    }

    // undo command open
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add coincident constraint"));

    // check if this coincidence is already enforced (even indirectly)
    bool constraintExists = Obj->arePointsCoincident(GeoId1, PosId1, GeoId2, PosId2);
    if (substituteConstraintCombinations(Obj, GeoId1, PosId1, GeoId2, PosId2)) {}
    else if (!constraintExists && (GeoId1 != GeoId2)) {
        Gui::cmdAppObjectArgs(sketchgui->getObject(),
                              "addConstraint(Sketcher.Constraint('Coincident', %d, %d, %d, %d))",
                              GeoId1,
                              static_cast<int>(PosId1),
                              GeoId2,
                              static_cast<int>(PosId2));
    }
    else {
        Gui::Command::abortCommand();
        return;
    }
    Gui::Command::commitCommand();
    tryAutoRecompute(Obj);
}

// ======================================================================================

class CmdSketcherConstrainDistance: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainDistance();
    ~CmdSketcherConstrainDistance() override
    {}
    void updateAction(int mode) override;
    const char* className() const override
    {
        return "CmdSketcherConstrainDistance";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainDistance::CmdSketcherConstrainDistance()
    : CmdSketcherConstraint("Sketcher_ConstrainDistance")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain distance");
    sToolTipText = QT_TR_NOOP("Fix a length of a line or the distance between a line and a vertex "
                              "or between two circles");
    sWhatsThis = "Sketcher_ConstrainDistance";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Length";
    sAccel = "K, D";
    eType = ForEdit;

    allowedSelSequences = {{SelVertex, SelVertexOrRoot},
                           {SelRoot, SelVertex},
                           {SelEdge},
                           {SelExternalEdge},
                           {SelVertex, SelEdgeOrAxis},
                           {SelRoot, SelEdge},
                           {SelVertex, SelExternalEdge},
                           {SelRoot, SelExternalEdge},
                           {SelEdge, SelEdge}};
}

void CmdSketcherConstrainDistance::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));

            getSelection().clearSelection();
        }
        else {
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select vertices from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty() || SubNames.size() > 2) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select exactly one line or one point and one line "
                                               "or two points from the sketch."));
        return;
    }

    int GeoId1, GeoId2 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1, PosId2 = Sketcher::PointPos::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() == 2) {
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);
    }

    bool arebothpointsorsegmentsfixed = areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

    if (isVertex(GeoId1, PosId1)
        && (GeoId2 == Sketcher::GeoEnum::VAxis || GeoId2 == Sketcher::GeoEnum::HAxis)) {
        std::swap(GeoId1, GeoId2);
        std::swap(PosId1, PosId2);
    }

    if ((isVertex(GeoId1, PosId1) || GeoId1 == Sketcher::GeoEnum::VAxis
         || GeoId1 == Sketcher::GeoEnum::HAxis)
        && isVertex(GeoId2, PosId2)) {// point to point distance

        Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);

        if (GeoId1 == Sketcher::GeoEnum::HAxis && PosId1 == Sketcher::PointPos::none) {
            PosId1 = Sketcher::PointPos::start;

            openCommand(
                QT_TRANSLATE_NOOP("Command", "Add distance from horizontal axis constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2),
                                  pnt2.y);
        }
        else if (GeoId1 == Sketcher::GeoEnum::VAxis && PosId1 == Sketcher::PointPos::none) {
            PosId1 = Sketcher::PointPos::start;

            openCommand(QT_TRANSLATE_NOOP("Command", "Add distance from vertical axis constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2),
                                  pnt2.x);
        }
        else {
            Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);

            openCommand(QT_TRANSLATE_NOOP("Command", "Add point to point distance constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2),
                                  (pnt2 - pnt1).Length());
        }

        if (arebothpointsorsegmentsfixed || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "setDriving(%d,%s)",
                                  ConStr.size() - 1,
                                  "False");
            finishDatumConstraint(this, Obj, false);
        }
        else {
            finishDatumConstraint(this, Obj, true);
        }
        return;
    }
    else if ((isVertex(GeoId1, PosId1) && isEdge(GeoId2, PosId2))
             || (isEdge(GeoId1, PosId1) && isVertex(GeoId2, PosId2))) {// point to line distance
        if (isVertex(GeoId2, PosId2)) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
        }
        Base::Vector3d pnt = Obj->getPoint(GeoId1, PosId1);
        const Part::Geometry* geom = Obj->getGeometry(GeoId2);

        if (isLineSegment(*geom)) {
            auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
            Base::Vector3d pnt1 = lineSeg->getStartPoint();
            Base::Vector3d pnt2 = lineSeg->getEndPoint();
            Base::Vector3d d = pnt2 - pnt1;
            double ActDist =
                std::abs(-pnt.x * d.y + pnt.y * d.x + pnt1.x * pnt2.y - pnt2.x * pnt1.y)
                / d.Length();

            openCommand(QT_TRANSLATE_NOOP("Command", "Add point to line Distance constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  ActDist);

            if (arebothpointsorsegmentsfixed
                || constraintCreationMode
                    == Reference) {// it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "setDriving(%d,%s)",
                                      ConStr.size() - 1,
                                      "False");
                finishDatumConstraint(this, Obj, false);
            }
            else {
                finishDatumConstraint(this, Obj, true);
            }

            return;
        }
        else if (isCircle(*geom)) {
            auto circleSeg = static_cast<const Part::GeomCircle*>(geom);
            Base::Vector3d ct = circleSeg->getCenter();
            Base::Vector3d d = ct - pnt;
            double ActDist = std::abs(d.Length() - circleSeg->getRadius());

            openCommand(QT_TRANSLATE_NOOP("Command", "Add point to circle Distance constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  ActDist);

            if (arebothpointsorsegmentsfixed
                || constraintCreationMode
                    == Reference) {// it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "setDriving(%d,%s)",
                                      ConStr.size() - 1,
                                      "False");
                finishDatumConstraint(this, Obj, false);
            }
            else {
                finishDatumConstraint(this, Obj, true);
            }

            return;
        }
    }
    else if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2)) {
        const Part::Geometry* geom1 = Obj->getGeometry(GeoId1);
        const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

        if (isCircle(*geom1) && isCircle(*geom2)) {// circle to circle distance
            auto circleSeg1 = static_cast<const Part::GeomCircle*>(geom1);
            double radius1 = circleSeg1->getRadius();
            Base::Vector3d center1 = circleSeg1->getCenter();

            auto circleSeg2 = static_cast<const Part::GeomCircle*>(geom2);
            double radius2 = circleSeg2->getRadius();
            Base::Vector3d center2 = circleSeg2->getCenter();

            double ActDist = 0.;

            Base::Vector3d intercenter = center1 - center2;
            double intercenterdistance = intercenter.Length();

            if (intercenterdistance >= radius1 && intercenterdistance >= radius2) {

                ActDist = intercenterdistance - radius1 - radius2;
            }
            else {
                double bigradius = std::max(radius1, radius2);
                double smallradius = std::min(radius1, radius2);

                ActDist = bigradius - smallradius - intercenterdistance;
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add circle to circle distance constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Distance',%d,%d,%f))",
                                  GeoId1,
                                  GeoId2,
                                  ActDist);

            if (arebothpointsorsegmentsfixed
                || constraintCreationMode
                    == Reference) {// it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "setDriving(%d,%s)",
                                      ConStr.size() - 1,
                                      "False");
                finishDatumConstraint(this, Obj, false);
            }
            else {
                finishDatumConstraint(this, Obj, true);
            }

            return;
        }
        else if ((isCircle(*geom1) && isLineSegment(*geom2))
                 || (isLineSegment(*geom1) && isCircle(*geom2))) {// circle to line distance

            if (isLineSegment(*geom1)) {
                std::swap(geom1, geom2);// Assume circle is first
                std::swap(GeoId1, GeoId2);
            }

            auto circleSeg = static_cast<const Part::GeomCircle*>(geom1);
            double radius = circleSeg->getRadius();
            Base::Vector3d center = circleSeg->getCenter();

            auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom2);
            Base::Vector3d pnt1 = lineSeg->getStartPoint();
            Base::Vector3d pnt2 = lineSeg->getEndPoint();
            Base::Vector3d d = pnt2 - pnt1;
            double ActDist =
                std::abs(-center.x * d.y + center.y * d.x + pnt1.x * pnt2.y - pnt2.x * pnt1.y)
                    / d.Length()
                - radius;

            openCommand(QT_TRANSLATE_NOOP("Command", "Add circle to line distance constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Distance',%d,%d,%f)) ",
                                  GeoId1,
                                  GeoId2,
                                  ActDist);

            if (arebothpointsorsegmentsfixed
                || constraintCreationMode
                    == Reference) {// it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "setDriving(%i,%s)",
                                      ConStr.size() - 1,
                                      "False");
                finishDatumConstraint(this, Obj, false);
            }
            else {
                finishDatumConstraint(this, Obj, true);
            }

            return;
        }
    }
    else if (isEdge(GeoId1, PosId1)) {// line length
        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Cannot add a length constraint on an axis!"));
            return;
        }

        arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj, GeoId1);

        const Part::Geometry* geom = Obj->getGeometry(GeoId1);

        if (isLineSegment(*geom)) {
            auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
            double ActLength = (lineSeg->getEndPoint() - lineSeg->getStartPoint()).Length();

            openCommand(QT_TRANSLATE_NOOP("Command", "Add length constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Distance',%d,%f))",
                                  GeoId1,
                                  ActLength);

            // it is a constraint on a external line, make it non-driving
            if (arebothpointsorsegmentsfixed || GeoId1 <= Sketcher::GeoEnum::RefExt
                || constraintCreationMode == Reference) {
                const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "setDriving(%d,%s)",
                                      ConStr.size() - 1,
                                      "False");
                finishDatumConstraint(this, Obj, false);
            }
            else {
                finishDatumConstraint(this, Obj, true);
            }

            return;
        }
    }

    Gui::TranslatedUserWarning(Obj,
                               QObject::tr("Wrong selection"),
                               QObject::tr("Select exactly one line or one point and one line or "
                                           "two points or two circles from the sketch."));
    return;
}

void CmdSketcherConstrainDistance::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::PointPos::none, PosId2 = Sketcher::PointPos::none;

    bool arebothpointsorsegmentsfixed = areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

    switch (seqIndex) {
        case 0:// {SelVertex, SelVertexOrRoot}
        case 1:// {SelRoot, SelVertex}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(1).PosId;

            Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);

            if (GeoId1 == Sketcher::GeoEnum::HAxis && PosId1 == Sketcher::PointPos::none) {
                PosId1 = Sketcher::PointPos::start;

                openCommand(
                    QT_TRANSLATE_NOOP("Command", "Add distance from horizontal axis constraint"));
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f))",
                    GeoId1,
                    static_cast<int>(PosId1),
                    GeoId2,
                    static_cast<int>(PosId2),
                    pnt2.y);
            }
            else if (GeoId1 == Sketcher::GeoEnum::VAxis && PosId1 == Sketcher::PointPos::none) {
                PosId1 = Sketcher::PointPos::start;

                openCommand(
                    QT_TRANSLATE_NOOP("Command", "Add distance from vertical axis constraint"));
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f))",
                    GeoId1,
                    static_cast<int>(PosId1),
                    GeoId2,
                    static_cast<int>(PosId2),
                    pnt2.x);
            }
            else {
                Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);

                openCommand(QT_TRANSLATE_NOOP("Command", "Add point to point distance constraint"));
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f))",
                    GeoId1,
                    static_cast<int>(PosId1),
                    GeoId2,
                    static_cast<int>(PosId2),
                    (pnt2 - pnt1).Length());
            }

            if (arebothpointsorsegmentsfixed || constraintCreationMode == Reference) {
                // it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
                finishDatumConstraint(this, Obj, false);
            }
            else {
                finishDatumConstraint(this, Obj, true);
            }

            return;
        }
        case 2:// {SelEdge}
        case 3:// {SelExternalEdge}
        {
            GeoId1 = GeoId2 = selSeq.at(0).GeoId;
            PosId1 = Sketcher::PointPos::start;
            PosId2 = Sketcher::PointPos::end;

            arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj, GeoId1);

            const Part::Geometry* geom = Obj->getGeometry(GeoId1);

            if (isLineSegment(*geom)) {
                auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
                double ActLength = (lineSeg->getEndPoint() - lineSeg->getStartPoint()).Length();

                openCommand(QT_TRANSLATE_NOOP("Command", "Add length constraint"));
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Distance',%d,%f))",
                                      GeoId1,
                                      ActLength);

                if (arebothpointsorsegmentsfixed || GeoId1 <= Sketcher::GeoEnum::RefExt
                    || constraintCreationMode == Reference) {
                    // it is a constraint on a external line, make it non-driving
                    const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                    Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
                    finishDatumConstraint(this, Obj, false);
                }
                else {
                    finishDatumConstraint(this, Obj, true);
                }
            }
            else if (isCircle(*geom)) {
                // allow this selection but do nothing as it needs 2 circles or 1 circle and 1 line
            }
            else {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("This constraint does not make sense for non-linear curves."));
            }

            return;
        }
        case 4:// {SelVertex, SelEdgeOrAxis}
        case 5:// {SelRoot, SelEdge}
        case 6:// {SelVertex, SelExternalEdge}
        case 7:// {SelRoot, SelExternalEdge}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(1).PosId;

            Base::Vector3d pnt = Obj->getPoint(GeoId1, PosId1);
            const Part::Geometry* geom = Obj->getGeometry(GeoId2);

            if (isLineSegment(*geom)) {
                auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
                Base::Vector3d pnt1 = lineSeg->getStartPoint();
                Base::Vector3d pnt2 = lineSeg->getEndPoint();
                Base::Vector3d d = pnt2 - pnt1;
                double ActDist =
                    std::abs(-pnt.x * d.y + pnt.y * d.x + pnt1.x * pnt2.y - pnt2.x * pnt1.y)
                    / d.Length();

                openCommand(QT_TRANSLATE_NOOP("Command", "Add point to line Distance constraint"));
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f))",
                                      GeoId1,
                                      static_cast<int>(PosId1),
                                      GeoId2,
                                      ActDist);

                if (arebothpointsorsegmentsfixed || constraintCreationMode == Reference) {
                    // it is a constraint on a external line, make it non-driving
                    const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                    Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
                    finishDatumConstraint(this, Obj, false);
                }
                else {
                    finishDatumConstraint(this, Obj, true);
                }
            }

            return;
        }
        case 8:// {SelEdge, SelEdge}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            const Part::Geometry* geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

            if (isCircle(*geom1) && isCircle(*geom2)) {// circle to circle distance
                auto circleSeg1 = static_cast<const Part::GeomCircle*>(geom1);
                double radius1 = circleSeg1->getRadius();
                Base::Vector3d center1 = circleSeg1->getCenter();

                auto circleSeg2 = static_cast<const Part::GeomCircle*>(geom2);
                double radius2 = circleSeg2->getRadius();
                Base::Vector3d center2 = circleSeg2->getCenter();

                double ActDist = 0.;

                Base::Vector3d intercenter = center1 - center2;
                double intercenterdistance = intercenter.Length();

                if (intercenterdistance >= radius1 && intercenterdistance >= radius2) {

                    ActDist = intercenterdistance - radius1 - radius2;
                }
                else {
                    double bigradius = std::max(radius1, radius2);
                    double smallradius = std::min(radius1, radius2);

                    ActDist = bigradius - smallradius - intercenterdistance;
                }

                openCommand(
                    QT_TRANSLATE_NOOP("Command", "Add circle to circle distance constraint"));
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Distance',%d,%d,%f))",
                                      GeoId1,
                                      GeoId2,
                                      ActDist);

                if (arebothpointsorsegmentsfixed
                    || constraintCreationMode
                        == Reference) {// it is a constraint on a external line, make it non-driving
                    const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                    Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
                    finishDatumConstraint(this, Obj, false);
                }
                else {
                    finishDatumConstraint(this, Obj, true);
                }

                return;
            }
            else {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select exactly one line or one point and one line or two points "
                                "or two circles from the sketch."));
            }
        }
        default:
            break;
    }
}

void CmdSketcherConstrainDistance::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_Length_Driven"));
            }
            break;
        case Driving:
            if (getAction()) {
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Length"));
            }
            break;
    }
}

// ======================================================================================

class CmdSketcherConstrainPointOnObject: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainPointOnObject();
    ~CmdSketcherConstrainPointOnObject() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainPointOnObject";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
    // returns true if a substitution took place
    static bool
    substituteConstraintCombinations(SketchObject* Obj, int GeoId1, PointPos PosId1, int GeoId2);
};

CmdSketcherConstrainPointOnObject::CmdSketcherConstrainPointOnObject()
    : CmdSketcherConstraint("Sketcher_ConstrainPointOnObject")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain point onto object");
    sToolTipText = QT_TR_NOOP("Fix a point onto an object");
    sWhatsThis = "Sketcher_ConstrainPointOnObject";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_PointOnObject";
    sAccel = "O";
    eType = ForEdit;

    allowedSelSequences = {{SelVertex, SelEdgeOrAxis},
                           {SelRoot, SelEdge},
                           {SelVertex, SelExternalEdge},
                           {SelEdge, SelVertexOrRoot},
                           {SelEdgeOrAxis, SelVertex},
                           {SelExternalEdge, SelVertex}};
}

bool CmdSketcherConstrainPointOnObject::substituteConstraintCombinations(SketchObject* Obj,
                                                                         int GeoId1,
                                                                         PointPos PosId1,
                                                                         int GeoId2)
{
    const std::vector<Constraint*>& cvals = Obj->Constraints.getValues();

    int cid = 0;
    for (std::vector<Constraint*>::const_iterator it = cvals.begin(); it != cvals.end();
         ++it, ++cid) {
        if ((*it)->Type == Sketcher::Tangent && (*it)->FirstPos == Sketcher::PointPos::none
            && (*it)->SecondPos == Sketcher::PointPos::none && (*it)->Third == GeoEnum::GeoUndef
            && (((*it)->First == GeoId1 && (*it)->Second == GeoId2)
                || ((*it)->Second == GeoId1 && (*it)->First == GeoId2))) {

            // NOTE: This function does not either open or commit a command as it is used for group
            // addition it relies on such infrastructure being provided by the caller.

            Gui::cmdAppObjectArgs(Obj, "delConstraint(%d)", cid);

            doEndpointToEdgeTangency(Obj, GeoId1, PosId1, GeoId2);

            notifyConstraintSubstitutions(
                QObject::tr("Endpoint to edge tangency was applied instead."));

            getSelection().clearSelection();
            return true;
        }
    }

    return false;
}

void CmdSketcherConstrainPointOnObject::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // count curves and points
    std::vector<SelIdPair> points;
    std::vector<SelIdPair> curves;
    for (std::size_t i = 0; i < SubNames.size(); i++) {
        SelIdPair id;
        getIdsFromName(SubNames[i], Obj, id.GeoId, id.PosId);
        if (isEdge(id.GeoId, id.PosId)) {
            curves.push_back(id);
        }
        if (isVertex(id.GeoId, id.PosId)) {
            points.push_back(id);
        }
    }

    if ((points.size() == 1 && !curves.empty()) || (!points.empty() && curves.size() == 1)) {

        openCommand(QT_TRANSLATE_NOOP("Command", "Add point on object constraint"));
        int cnt = 0;
        for (std::size_t iPnt = 0; iPnt < points.size(); iPnt++) {
            for (std::size_t iCrv = 0; iCrv < curves.size(); iCrv++) {
                if (areBothPointsOrSegmentsFixed(Obj, points[iPnt].GeoId, curves[iCrv].GeoId)) {
                    showNoConstraintBetweenFixedGeometry(Obj);
                    continue;
                }
                if (points[iPnt].GeoId == curves[iCrv].GeoId) {
                    continue;// constraining a point of an element onto the element is a bad idea...
                }

                const Part::Geometry* geom = Obj->getGeometry(curves[iCrv].GeoId);

                if (geom && isBsplinePole(geom)) {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Wrong selection"),
                        QObject::tr("Select an edge that is not a B-spline weight."));
                    abortCommand();

                    continue;
                }

                if (substituteConstraintCombinations(Obj,
                                                     points[iPnt].GeoId,
                                                     points[iPnt].PosId,
                                                     curves[iCrv].GeoId)) {
                    cnt++;
                    continue;
                }

                cnt++;
                Gui::cmdAppObjectArgs(
                    selection[0].getObject(),
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    points[iPnt].GeoId,
                    static_cast<int>(points[iPnt].PosId),
                    curves[iCrv].GeoId);
            }
        }
        if (cnt) {
            commitCommand();
            getSelection().clearSelection();
        }
        else {
            abortCommand();
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("None of the selected points were constrained "
                                                   "onto the respective curves, "
                                                   "because they are parts "
                                                   "of the same element, "
                                                   "because they are both external geometry, "
                                                   "or because the edge is not eligible."));
        }
        return;
    }

    Gui::TranslatedUserWarning(Obj,
                               QObject::tr("Wrong selection"),
                               QObject::tr("Select either one point and several curves, "
                                           "or one curve and several points."));
    return;
}

void CmdSketcherConstrainPointOnObject::applyConstraint(std::vector<SelIdPair>& selSeq,
                                                        int seqIndex)
{
    int GeoIdVt, GeoIdCrv;
    Sketcher::PointPos PosIdVt;

    switch (seqIndex) {
        case 0:// {SelVertex, SelEdgeOrAxis}
        case 1:// {SelRoot, SelEdge}
        case 2:// {SelVertex, SelExternalEdge}
            GeoIdVt = selSeq.at(0).GeoId;
            GeoIdCrv = selSeq.at(1).GeoId;
            PosIdVt = selSeq.at(0).PosId;

            break;
        case 3:// {SelEdge, SelVertexOrRoot}
        case 4:// {SelEdgeOrAxis, SelVertex}
        case 5:// {SelExternalEdge, SelVertex}
            GeoIdVt = selSeq.at(1).GeoId;
            GeoIdCrv = selSeq.at(0).GeoId;
            PosIdVt = selSeq.at(1).PosId;

            break;
        default:
            return;
    }

    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    openCommand(QT_TRANSLATE_NOOP("Command", "Add point on object constraint"));
    bool allOK = true;

    if (areBothPointsOrSegmentsFixed(Obj, GeoIdVt, GeoIdCrv)) {
        showNoConstraintBetweenFixedGeometry(Obj);
        allOK = false;
    }
    if (GeoIdVt == GeoIdCrv) {
        allOK = false;// constraining a point of an element onto the element is a bad idea...
    }

    const Part::Geometry* geom = Obj->getGeometry(GeoIdCrv);

    if (geom && isBsplinePole(geom)) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select an edge that is not a B-spline weight."));
        abortCommand();

        return;
    }

    if (allOK) {
        if (!substituteConstraintCombinations(Obj, GeoIdVt, PosIdVt, GeoIdCrv)) {
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                                  GeoIdVt,
                                  static_cast<int>(PosIdVt),
                                  GeoIdCrv);
        }

        commitCommand();
        tryAutoRecompute(Obj);
    }
    else {
        abortCommand();
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("None of the selected points "
                                               "were constrained onto the respective curves, "
                                               "either because they are parts of the same element, "
                                               "or because they are both external geometry."));
    }
    return;
}

// ======================================================================================

class CmdSketcherConstrainDistanceX: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainDistanceX();
    ~CmdSketcherConstrainDistanceX() override
    {}
    void updateAction(int mode) override;
    const char* className() const override
    {
        return "CmdSketcherConstrainDistanceX";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainDistanceX::CmdSketcherConstrainDistanceX()
    : CmdSketcherConstraint("Sketcher_ConstrainDistanceX")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain horizontal distance");
    sToolTipText = QT_TR_NOOP("Fix the horizontal distance "
                              "between two points or line ends");
    sWhatsThis = "Sketcher_ConstrainDistanceX";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_HorizontalDistance";
    sAccel = "L";
    eType = ForEdit;

    // Can't do single vertex because its a prefix for 2 vertices
    allowedSelSequences = {{SelVertex, SelVertexOrRoot},
                           {SelRoot, SelVertex},
                           {SelEdge},
                           {SelExternalEdge}};
}

void CmdSketcherConstrainDistanceX::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty() || SubNames.size() > 2) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one line or up to two points from the sketch."));
        return;
    }

    int GeoId1, GeoId2 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1, PosId2 = Sketcher::PointPos::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() == 2) {
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);
    }

    bool arebothpointsorsegmentsfixed = areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

    if (GeoId2 == Sketcher::GeoEnum::HAxis || GeoId2 == Sketcher::GeoEnum::VAxis) {
        std::swap(GeoId1, GeoId2);
        std::swap(PosId1, PosId2);
    }

    if (GeoId1 == Sketcher::GeoEnum::HAxis && PosId1 == Sketcher::PointPos::none) {
        // reject horizontal axis from selection
        GeoId1 = GeoEnum::GeoUndef;
    }
    else if (GeoId1 == Sketcher::GeoEnum::VAxis && PosId1 == Sketcher::PointPos::none) {
        GeoId1 = Sketcher::GeoEnum::HAxis;
        PosId1 = Sketcher::PointPos::start;
    }

    if (isEdge(GeoId1, PosId1) && GeoId2 == GeoEnum::GeoUndef) {
        // horizontal length of a line
        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a horizontal length constraint on an axis!"));
            return;
        }

        arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj, GeoId1);

        const Part::Geometry* geom = Obj->getGeometry(GeoId1);

        if (isLineSegment(*geom)) {
            // convert to as if two endpoints of the line have been selected
            PosId1 = Sketcher::PointPos::start;
            GeoId2 = GeoId1;
            PosId2 = Sketcher::PointPos::end;
        }
    }
    if (isVertex(GeoId1, PosId1) && isVertex(GeoId2, PosId2)) {
        // point to point horizontal distance
        Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);
        Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);
        double ActLength = pnt2.x - pnt1.x;

        // negative sign avoidance: swap the points to make value positive
        if (ActLength < -Precision::Confusion()) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
            std::swap(pnt1, pnt2);
            ActLength = -ActLength;
        }

        openCommand(
            QT_TRANSLATE_NOOP("Command", "Add point to point horizontal distance constraint"));
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f))",
                              GeoId1,
                              static_cast<int>(PosId1),
                              GeoId2,
                              static_cast<int>(PosId2),
                              ActLength);

        if (arebothpointsorsegmentsfixed || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "setDriving(%d,%s)",
                                  ConStr.size() - 1,
                                  "False");
            finishDatumConstraint(this, Obj, false);
        }
        else {
            finishDatumConstraint(this, Obj, true);
        }

        return;
    }
    else if (isVertex(GeoId1, PosId1) && GeoId2 == GeoEnum::GeoUndef) {
        // point on fixed x-coordinate

        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a fixed x-coordinate constraint on the origin point!"));
            return;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId1, PosId1);
        double ActX = pnt.x;

        arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj, GeoId1);

        openCommand(QT_TRANSLATE_NOOP("Command", "Add fixed x-coordinate constraint"));
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f))",
                              GeoId1,
                              static_cast<int>(PosId1),
                              ActX);

        if (arebothpointsorsegmentsfixed || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "setDriving(%d,%s)",
                                  ConStr.size() - 1,
                                  "False");
            finishDatumConstraint(this, Obj, false);
        }
        else {
            finishDatumConstraint(this, Obj, true);
        }

        return;
    }

    Gui::TranslatedUserWarning(
        Obj,
        QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one line or up to two points from the sketch."));
    return;
}

void CmdSketcherConstrainDistanceX::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::PointPos::none, PosId2 = Sketcher::PointPos::none;

    switch (seqIndex) {
        case 0:// {SelVertex, SelVertexOrRoot}
        case 1:// {SelRoot, SelVertex}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(1).PosId;
            break;
        }
        case 2:// {SelEdge}
        case 3:// {SelExternalEdge}
        {
            GeoId1 = GeoId2 = selSeq.at(0).GeoId;
            PosId1 = Sketcher::PointPos::start;
            PosId2 = Sketcher::PointPos::end;

            const Part::Geometry* geom = Obj->getGeometry(GeoId1);

            if (! isLineSegment(*geom)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr(
                        "This constraint only makes sense on a line segment or a pair of points."));
                return;
            }

            break;
        }
        default:
            break;
    }

    Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);
    Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);
    double ActLength = pnt2.x - pnt1.x;

    // negative sign avoidance: swap the points to make value positive
    if (ActLength < -Precision::Confusion()) {
        std::swap(GeoId1, GeoId2);
        std::swap(PosId1, PosId2);
        std::swap(pnt1, pnt2);
        ActLength = -ActLength;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Add point to point horizontal distance constraint"));
    Gui::cmdAppObjectArgs(Obj,
                          "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f))",
                          GeoId1,
                          static_cast<int>(PosId1),
                          GeoId2,
                          static_cast<int>(PosId2),
                          ActLength);

    if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2) || constraintCreationMode == Reference) {
        // it is a constraint on a external line, make it non-driving
        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

        Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
        finishDatumConstraint(this, Obj, false);
    }
    else {
        finishDatumConstraint(this, Obj, true);
    }
}

void CmdSketcherConstrainDistanceX::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance_Driven"));
            }
            break;
        case Driving:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance"));
            }
            break;
    }
}


// ======================================================================================

class CmdSketcherConstrainDistanceY: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainDistanceY();
    ~CmdSketcherConstrainDistanceY() override
    {}
    void updateAction(int mode) override;
    const char* className() const override
    {
        return "CmdSketcherConstrainDistanceY";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainDistanceY::CmdSketcherConstrainDistanceY()
    : CmdSketcherConstraint("Sketcher_ConstrainDistanceY")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain vertical distance");
    sToolTipText = QT_TR_NOOP("Fix the vertical distance between two points or line ends");
    sWhatsThis = "Sketcher_ConstrainDistanceY";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_VerticalDistance";
    sAccel = "I";
    eType = ForEdit;

    // Can't do single vertex because its a prefix for 2 vertices
    allowedSelSequences = {{SelVertex, SelVertexOrRoot},
                           {SelRoot, SelVertex},
                           {SelEdge},
                           {SelExternalEdge}};
}

void CmdSketcherConstrainDistanceY::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty() || SubNames.size() > 2) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one line or up to two points from the sketch."));
        return;
    }

    int GeoId1, GeoId2 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1, PosId2 = Sketcher::PointPos::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() == 2) {
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);
    }

    bool arebothpointsorsegmentsfixed = areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

    if (GeoId2 == Sketcher::GeoEnum::HAxis || GeoId2 == Sketcher::GeoEnum::VAxis) {
        std::swap(GeoId1, GeoId2);
        std::swap(PosId1, PosId2);
    }

    if (GeoId1 == Sketcher::GeoEnum::VAxis
        && PosId1 == Sketcher::PointPos::none) {// reject vertical axis from selection
        GeoId1 = GeoEnum::GeoUndef;
    }
    else if (GeoId1 == Sketcher::GeoEnum::HAxis && PosId1 == Sketcher::PointPos::none) {
        PosId1 = Sketcher::PointPos::start;
    }

    if (isEdge(GeoId1, PosId1) && GeoId2 == GeoEnum::GeoUndef) {// vertical length of a line
        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a vertical length constraint on an axis!"));
            return;
        }

        arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj, GeoId1);

        const Part::Geometry* geom = Obj->getGeometry(GeoId1);

        if (isLineSegment(*geom)) {
            // convert to as if two endpoints of the line have been selected
            PosId1 = Sketcher::PointPos::start;
            GeoId2 = GeoId1;
            PosId2 = Sketcher::PointPos::end;
        }
    }

    if (isVertex(GeoId1, PosId1) && isVertex(GeoId2, PosId2)) {
        // point to point vertical distance
        Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);
        Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);
        double ActLength = pnt2.y - pnt1.y;

        // negative sign avoidance: swap the points to make value positive
        if (ActLength < -Precision::Confusion()) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
            std::swap(pnt1, pnt2);
            ActLength = -ActLength;
        }

        openCommand(
            QT_TRANSLATE_NOOP("Command", "Add point to point vertical distance constraint"));
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f))",
                              GeoId1,
                              static_cast<int>(PosId1),
                              GeoId2,
                              static_cast<int>(PosId2),
                              ActLength);

        if (arebothpointsorsegmentsfixed || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "setDriving(%d,%s)",
                                  ConStr.size() - 1,
                                  "False");
            finishDatumConstraint(this, Obj, false);
        }
        else {
            finishDatumConstraint(this, Obj, true);
        }

        return;
    }
    else if (isVertex(GeoId1, PosId1) && GeoId2 == GeoEnum::GeoUndef) {
        // point on fixed y-coordinate
        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a fixed y-coordinate constraint on the origin point!"));
            return;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId1, PosId1);
        double ActY = pnt.y;

        arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj, GeoId1);

        openCommand(QT_TRANSLATE_NOOP("Command", "Add fixed y-coordinate constraint"));
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f))",
                              GeoId1,
                              static_cast<int>(PosId1),
                              ActY);

        if (GeoId1 <= Sketcher::GeoEnum::RefExt || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "setDriving(%d,%s)",
                                  ConStr.size() - 1,
                                  "False");
            finishDatumConstraint(this, Obj, false);
        }
        else {
            finishDatumConstraint(this, Obj, true);
        }

        return;
    }

    Gui::TranslatedUserWarning(
        Obj,
        QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one line or up to two points from the sketch."));
    return;
}

void CmdSketcherConstrainDistanceY::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::PointPos::none, PosId2 = Sketcher::PointPos::none;

    switch (seqIndex) {
        case 0:// {SelVertex, SelVertexOrRoot}
        case 1:// {SelRoot, SelVertex}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(1).PosId;
            break;
        }
        case 2:// {SelEdge}
        case 3:// {SelExternalEdge}
        {
            GeoId1 = GeoId2 = selSeq.at(0).GeoId;
            PosId1 = Sketcher::PointPos::start;
            PosId2 = Sketcher::PointPos::end;

            const Part::Geometry* geom = Obj->getGeometry(GeoId1);

            if (! isLineSegment(*geom)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr(
                        "This constraint only makes sense on a line segment or a pair of points."));
                return;
            }

            break;
        }
        default:
            break;
    }

    Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);
    Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);
    double ActLength = pnt2.y - pnt1.y;

    // negative sign avoidance: swap the points to make value positive
    if (ActLength < -Precision::Confusion()) {
        std::swap(GeoId1, GeoId2);
        std::swap(PosId1, PosId2);
        std::swap(pnt1, pnt2);
        ActLength = -ActLength;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Add point to point vertical distance constraint"));
    Gui::cmdAppObjectArgs(Obj,
                          "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f))",
                          GeoId1,
                          static_cast<int>(PosId1),
                          GeoId2,
                          static_cast<int>(PosId2),
                          ActLength);

    if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)
        || constraintCreationMode
            == Reference) {// it is a constraint on a external line, make it non-driving
        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

        Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
        finishDatumConstraint(this, Obj, false);
    }
    else {
        finishDatumConstraint(this, Obj, true);
    }
}

void CmdSketcherConstrainDistanceY::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance_Driven"));
            }
            break;
        case Driving:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance"));
            }
            break;
    }
}

//=================================================================================

class CmdSketcherConstrainParallel: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainParallel();
    ~CmdSketcherConstrainParallel() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainParallel";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainParallel::CmdSketcherConstrainParallel()
    : CmdSketcherConstraint("Sketcher_ConstrainParallel")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain parallel");
    sToolTipText = QT_TR_NOOP("Create a parallel constraint between two lines");
    sWhatsThis = "Sketcher_ConstrainParallel";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Parallel";
    sAccel = "P";
    eType = ForEdit;

    // TODO: Also needed: ExternalEdges
    allowedSelSequences = {{SelEdge, SelEdgeOrAxis},
                           {SelEdgeOrAxis, SelEdge},
                           {SelEdge, SelExternalEdge},
                           {SelExternalEdge, SelEdge}};
}

void CmdSketcherConstrainParallel::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select two or more lines from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // go through the selected subelements

    if (SubNames.size() < 2) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select at least two lines from the sketch."));
        return;
    }

    std::vector<int> ids;
    bool hasAlreadyExternal = false;
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {

        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(*it, Obj, GeoId, PosId);

        if (!isEdge(GeoId, PosId)) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select a valid line."));
            return;
        }
        else if (isPointOrSegmentFixed(Obj, GeoId)) {
            if (hasAlreadyExternal) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }
            else {
                hasAlreadyExternal = true;
            }
        }

        // Check that the curve is a line segment
        const Part::Geometry* geo = Obj->getGeometry(GeoId);

        if (! isLineSegment(*geo)) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("The selected edge is not a valid line."));
            return;
        }
        ids.push_back(GeoId);
    }

    // undo command open
    openCommand(QT_TRANSLATE_NOOP("Command", "Add parallel constraint"));
    for (int i = 0; i < int(ids.size() - 1); i++) {
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "addConstraint(Sketcher.Constraint('Parallel',%d,%d))",
                              ids[i],
                              ids[i + 1]);
    }
    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainParallel::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    switch (seqIndex) {
        case 0:// {SelEdge, SelEdgeOrAxis}
        case 1:// {SelEdgeOrAxis, SelEdge}
        case 2:// {SelEdge, SelExternalEdge}
        case 3:// {SelExternalEdge, SelEdge}
            // create the constraint
            SketcherGui::ViewProviderSketch* sketchgui =
                static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
            Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

            int GeoId1 = selSeq.at(0).GeoId, GeoId2 = selSeq.at(1).GeoId;

            // Check that the curves are line segments
            if (! isLineSegment(*(Obj->getGeometry(GeoId1))) || ! isLineSegment(*(Obj->getGeometry(GeoId2)))) {
                Gui::TranslatedUserWarning(Obj,
                                           QObject::tr("Wrong selection"),
                                           QObject::tr("The selected edge is not a valid line."));
                return;
            }

            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }

            // undo command open
            openCommand(QT_TRANSLATE_NOOP("Command", "Add parallel constraint"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "addConstraint(Sketcher.Constraint('Parallel',%d,%d))",
                                  GeoId1,
                                  GeoId2);
            // finish the transaction and update
            commitCommand();
            tryAutoRecompute(Obj);
    }
}

// ======================================================================================

class CmdSketcherConstrainPerpendicular: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainPerpendicular();
    ~CmdSketcherConstrainPerpendicular() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainPerpendicular";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainPerpendicular::CmdSketcherConstrainPerpendicular()
    : CmdSketcherConstraint("Sketcher_ConstrainPerpendicular")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain perpendicular");
    sToolTipText = QT_TR_NOOP("Create a perpendicular constraint between two lines");
    sWhatsThis = "Sketcher_ConstrainPerpendicular";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Perpendicular";
    sAccel = "N";
    eType = ForEdit;

    // TODO: there are two more combos: endpoint then curve and endpoint then endpoint
    allowedSelSequences = {{SelEdge, SelEdgeOrAxis},
                           {SelEdgeOrAxis, SelEdge},
                           {SelEdge, SelExternalEdge},
                           {SelExternalEdge, SelEdge},
                           {SelVertexOrRoot, SelEdge, SelEdgeOrAxis},
                           {SelVertexOrRoot, SelEdgeOrAxis, SelEdge},
                           {SelVertexOrRoot, SelEdge, SelExternalEdge},
                           {SelVertexOrRoot, SelExternalEdge, SelEdge},
                           {SelEdge, SelVertexOrRoot, SelEdgeOrAxis},
                           {SelEdgeOrAxis, SelVertexOrRoot, SelEdge},
                           {SelEdge, SelVertexOrRoot, SelExternalEdge},
                           {SelExternalEdge, SelVertexOrRoot, SelEdge}};
    ;
}

void CmdSketcherConstrainPerpendicular::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            QString strBasicHelp =
                QObject::tr("There is a number of ways this constraint can be applied.\n\n"
                            "Accepted combinations: two curves; an endpoint and a curve; two "
                            "endpoints; two curves and a point.",
                            /*disambig.:*/ "perpendicular constraint");
            QString strError =
                QObject::tr("Select some geometry from the sketch.", "perpendicular constraint");
            strError.append(QString::fromLatin1("\n\n"));
            strError.append(strBasicHelp);
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       std::move(strError));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (!Obj || (SubNames.size() != 2 && SubNames.size() != 3)) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Wrong number of selected objects!"));
        return;
    }

    int GeoId1, GeoId2, GeoId3;
    Sketcher::PointPos PosId1, PosId2, PosId3;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (areBothPointsOrSegmentsFixed(Obj,
                                     GeoId1,
                                     GeoId2)) {// checkBothExternal displays error message
        showNoConstraintBetweenFixedGeometry(Obj);
        return;
    }

    if (SubNames.size() == 3) {// perpendicular via point
        getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);
        // let's sink the point to be GeoId3. We want to keep the order the two curves have been
        // selected in.
        if (isVertex(GeoId1, PosId1)) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
        };
        if (isVertex(GeoId2, PosId2)) {
            std::swap(GeoId2, GeoId3);
            std::swap(PosId2, PosId3);
        };

        if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

            if (isBsplinePole(Obj, GeoId1) || isBsplinePole(Obj, GeoId2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add perpendicular constraint"));

            bool safe = addConstraintSafely(Obj, [&]() {
                // add missing point-on-object constraints
                if (!IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)) {
                    Gui::cmdAppObjectArgs(
                        selection[0].getObject(),
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoId3,
                        static_cast<int>(PosId3),
                        GeoId1);
                };

                if (!IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)) {
                    Gui::cmdAppObjectArgs(
                        selection[0].getObject(),
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoId3,
                        static_cast<int>(PosId3),
                        GeoId2);
                };

                if (!IsPointAlreadyOnCurve(
                        GeoId1,
                        GeoId3,
                        PosId3,
                        Obj)) {// FIXME: it's a good idea to add a check if the sketch is solved
                    Gui::cmdAppObjectArgs(
                        selection[0].getObject(),
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoId3,
                        static_cast<int>(PosId3),
                        GeoId1);
                };

                Gui::cmdAppObjectArgs(
                    selection[0].getObject(),
                    "addConstraint(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,%d))",
                    GeoId1,
                    GeoId2,
                    GeoId3,
                    static_cast<int>(PosId3));
            });

            if (!safe) {
                return;
            }
            else {
                commitCommand();
                tryAutoRecompute(Obj);
            }

            getSelection().clearSelection();

            return;
        };

        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("With 3 objects, there must be 2 curves and 1 point."));
    }
    else if (SubNames.size() == 2) {
        if (isVertex(GeoId1, PosId1)
            && isVertex(GeoId2, PosId2)) {// endpoint-to-endpoint perpendicularity

            if (isSimpleVertex(Obj, GeoId1, PosId1) || isSimpleVertex(Obj, GeoId2, PosId2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr(
                        "Cannot add a perpendicularity constraint at an unconnected point!"));
                return;
            }

            // This code supports simple B-spline endpoint perp to any other geometric curve
            const Part::Geometry* geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

            if (geom1 && geom2 && (isBSplineCurve(*geom1) || isBSplineCurve(*geom2))) {
                if (! isBSplineCurve(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                    std::swap(PosId1, PosId2);
                }
                // GeoId1 is the B-spline now
            }// end of code supports simple B-spline endpoint tangency

            openCommand(QT_TRANSLATE_NOOP("Command", "Add perpendicular constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Perpendicular',%d,%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2));
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
        else if ((isVertex(GeoId1, PosId1) && isEdge(GeoId2, PosId2))
                 || (isEdge(GeoId1, PosId1) && isVertex(GeoId2, PosId2))) {// endpoint-to-curve
            if (isVertex(GeoId2, PosId2)) {
                std::swap(GeoId1, GeoId2);
                std::swap(PosId1, PosId2);
            }

            if (isSimpleVertex(Obj, GeoId1, PosId1)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr(
                        "Cannot add a perpendicularity constraint at an unconnected point!"));
                return;
            }

            const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

            if (geom2 && isBSplineCurve(*geom2)) {
                // unsupported until normal to B-spline at any point implemented.
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Perpendicular to B-spline edge currently unsupported."));
                return;
            }

            if (isBsplinePole(geom2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add perpendicularity constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Perpendicular',%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
        else if (isEdge(GeoId1, PosId1)
                 && isEdge(GeoId2, PosId2)) {// simple perpendicularity between GeoId1 and GeoId2

            const Part::Geometry* geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geo2 = Obj->getGeometry(GeoId2);
            if (!geo1 || !geo2) {
                return;
            }

            if (! isLineSegment(*geo1) && ! isLineSegment(*geo2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("One of the selected edges should be a line."));
                return;
            }

            if (isBSplineCurve(*geo1) || isBSplineCurve(*geo2)) {
                // unsupported until tangent to B-spline at any point implemented.
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Perpendicular to B-spline edge currently unsupported."));
                return;
            }

            if (isLineSegment(*geo1)) {
                std::swap(GeoId1, GeoId2);
            }

            if (isBsplinePole(Obj, GeoId1)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            // GeoId2 is the line
            geo1 = Obj->getGeometry(GeoId1);
            geo2 = Obj->getGeometry(GeoId2);

            if (isEllipse(*geo1) || isArcOfEllipse(*geo1) || isArcOfHyperbola(*geo1) || isArcOfParabola(*geo1)) {
                Base::Vector3d center;
                Base::Vector3d majdir;
                Base::Vector3d focus;
                double majord = 0;
                double minord = 0;
                double phi = 0;

                if (isEllipse(*geo1)) {
                    auto ellipse = static_cast<const Part::GeomEllipse*>(geo1);
                    center = ellipse->getCenter();
                    majord = ellipse->getMajorRadius();
                    minord = ellipse->getMinorRadius();
                    majdir = ellipse->getMajorAxisDir();
                    phi = atan2(majdir.y, majdir.x);
                }
                else if (isArcOfEllipse(*geo1)) {
                    auto aoe = static_cast<const Part::GeomArcOfEllipse*>(geo1);
                    center = aoe->getCenter();
                    majord = aoe->getMajorRadius();
                    minord = aoe->getMinorRadius();
                    majdir = aoe->getMajorAxisDir();
                    phi = atan2(majdir.y, majdir.x);
                }
                else if (isArcOfHyperbola(*geo1)) {
                    auto aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo1);
                    center = aoh->getCenter();
                    majord = aoh->getMajorRadius();
                    minord = aoh->getMinorRadius();
                    majdir = aoh->getMajorAxisDir();
                    phi = atan2(majdir.y, majdir.x);
                }
                else if (isArcOfParabola(*geo1)) {
                    auto aop = static_cast<const Part::GeomArcOfParabola*>(geo1);
                    center = aop->getCenter();
                    focus = aop->getFocus();
                }

                const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo2);

                Base::Vector3d point1 = line->getStartPoint();
                Base::Vector3d PoO;

                if (isArcOfHyperbola(*geo1)) {
                    double df = sqrt(majord * majord + minord * minord);
                    Base::Vector3d direction = point1 - (center + majdir * df);// towards the focus
                    double tapprox = atan2(direction.y, direction.x) - phi;

                    PoO = Base::Vector3d(center.x + majord * cosh(tapprox) * cos(phi)
                                             - minord * sinh(tapprox) * sin(phi),
                                         center.y + majord * cosh(tapprox) * sin(phi)
                                             + minord * sinh(tapprox) * cos(phi),
                                         0);
                }
                else if (isArcOfParabola(*geo1)) {
                    Base::Vector3d direction = point1 - focus;// towards the focus

                    PoO = point1 + direction / 2;
                }
                else {
                    Base::Vector3d direction = point1 - center;
                    double tapprox = atan2(direction.y, direction.x)
                        - phi;// we approximate the eccentric anomaly by the polar

                    PoO = Base::Vector3d(center.x + majord * cos(tapprox) * cos(phi)
                                             - minord * sin(tapprox) * sin(phi),
                                         center.y + majord * cos(tapprox) * sin(phi)
                                             + minord * sin(tapprox) * cos(phi),
                                         0);
                }
                openCommand(QT_TRANSLATE_NOOP("Command", "Add perpendicular constraint"));

                try {
                    // Add a point
                    Gui::cmdAppObjectArgs(Obj,
                                          "addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                                          PoO.x,
                                          PoO.y);
                    int GeoIdPoint = Obj->getHighestCurveIndex();

                    // Point on first object (ellipse, arc of ellipse)
                    Gui::cmdAppObjectArgs(
                        selection[0].getObject(),
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoIdPoint,
                        static_cast<int>(Sketcher::PointPos::start),
                        GeoId1);
                    // Point on second object
                    Gui::cmdAppObjectArgs(
                        selection[0].getObject(),
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoIdPoint,
                        static_cast<int>(Sketcher::PointPos::start),
                        GeoId2);
                    // add constraint: Perpendicular-via-point
                    Gui::cmdAppObjectArgs(
                        Obj,
                        "addConstraint(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,%d))",
                        GeoId1,
                        GeoId2,
                        GeoIdPoint,
                        static_cast<int>(Sketcher::PointPos::start));
                }
                catch (const Base::Exception& e) {
                    Gui::NotifyUserError(Obj,
                                         QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                                         e.what());
                    Gui::Command::abortCommand();

                    tryAutoRecompute(Obj);
                    return;
                }

                commitCommand();
                tryAutoRecompute(Obj);

                getSelection().clearSelection();
                return;
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add perpendicular constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Perpendicular',%d,%d))",
                                  GeoId1,
                                  GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
    }

    return;
}

void CmdSketcherConstrainPerpendicular::applyConstraint(std::vector<SelIdPair>& selSeq,
                                                        int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef, GeoId3 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::PointPos::none, PosId2 = Sketcher::PointPos::none,
                       PosId3 = Sketcher::PointPos::none;

    switch (seqIndex) {
        case 0:// {SelEdge, SelEdgeOrAxis}
        case 1:// {SelEdgeOrAxis, SelEdge}
        case 2:// {SelEdge, SelExternalEdge}
        case 3:// {SelExternalEdge, SelEdge}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;

            // check if the edge already has a Block constraint
            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }

            const Part::Geometry* geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geo2 = Obj->getGeometry(GeoId2);
            if (!geo1 || !geo2) {
                return;
            }

            if (! isLineSegment(*geo1) && ! isLineSegment(*geo2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("One of the selected edges should be a line."));
                return;
            }

            if (isBSplineCurve(*geo1) || isBSplineCurve(*geo2)) {
                // unsupported until tangent to B-spline at any point implemented.
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Perpendicular to B-spline edge currently unsupported."));
                return;
            }

            if (isLineSegment(*geo1)) {
                std::swap(GeoId1, GeoId2);
            }

            if (isBsplinePole(Obj, GeoId1)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            // GeoId2 is the line
            geo1 = Obj->getGeometry(GeoId1);
            geo2 = Obj->getGeometry(GeoId2);

            if (isEllipse(*geo1) || isArcOfEllipse(*geo1) || isArcOfHyperbola(*geo1) || isArcOfParabola(*geo1)) {
                Base::Vector3d center;
                Base::Vector3d majdir;
                Base::Vector3d focus;
                double majord = 0;
                double minord = 0;
                double phi = 0;

                if (isEllipse(*geo1)) {
                    auto ellipse = static_cast<const Part::GeomEllipse*>(geo1);
                    center = ellipse->getCenter();
                    majord = ellipse->getMajorRadius();
                    minord = ellipse->getMinorRadius();
                    majdir = ellipse->getMajorAxisDir();
                    phi = atan2(majdir.y, majdir.x);
                }
                else if (isArcOfEllipse(*geo1)) {
                    auto aoe = static_cast<const Part::GeomArcOfEllipse*>(geo1);
                    center = aoe->getCenter();
                    majord = aoe->getMajorRadius();
                    minord = aoe->getMinorRadius();
                    majdir = aoe->getMajorAxisDir();
                    phi = atan2(majdir.y, majdir.x);
                }
                else if (isArcOfHyperbola(*geo1)) {
                    auto aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo1);
                    center = aoh->getCenter();
                    majord = aoh->getMajorRadius();
                    minord = aoh->getMinorRadius();
                    majdir = aoh->getMajorAxisDir();
                    phi = atan2(majdir.y, majdir.x);
                }
                else if (isArcOfParabola(*geo1)) {
                    auto aop = static_cast<const Part::GeomArcOfParabola*>(geo1);
                    center = aop->getCenter();
                    focus = aop->getFocus();
                }

                const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo2);

                Base::Vector3d point1 = line->getStartPoint();
                Base::Vector3d PoO;

                if (isArcOfHyperbola(*geo1)) {
                    double df = sqrt(majord * majord + minord * minord);
                    Base::Vector3d direction = point1 - (center + majdir * df);// towards the focus
                    double tapprox = atan2(direction.y, direction.x) - phi;

                    PoO = Base::Vector3d(center.x + majord * cosh(tapprox) * cos(phi)
                                             - minord * sinh(tapprox) * sin(phi),
                                         center.y + majord * cosh(tapprox) * sin(phi)
                                             + minord * sinh(tapprox) * cos(phi),
                                         0);
                }
                else if (isArcOfParabola(*geo1)) {
                    Base::Vector3d direction = point1 - focus;// towards the focus

                    PoO = point1 + direction / 2;
                }
                else {
                    Base::Vector3d direction = point1 - center;
                    double tapprox = atan2(direction.y, direction.x)
                        - phi;// we approximate the eccentric anomaly by the polar

                    PoO = Base::Vector3d(center.x + majord * cos(tapprox) * cos(phi)
                                             - minord * sin(tapprox) * sin(phi),
                                         center.y + majord * cos(tapprox) * sin(phi)
                                             + minord * sin(tapprox) * cos(phi),
                                         0);
                }
                openCommand(QT_TRANSLATE_NOOP("Command", "Add perpendicular constraint"));

                try {
                    // Add a point
                    Gui::cmdAppObjectArgs(Obj,
                                          "addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                                          PoO.x,
                                          PoO.y);
                    int GeoIdPoint = Obj->getHighestCurveIndex();

                    // Point on first object (ellipse, arc of ellipse)
                    Gui::cmdAppObjectArgs(
                        Obj,
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoIdPoint,
                        static_cast<int>(Sketcher::PointPos::start),
                        GeoId1);
                    // Point on second object
                    Gui::cmdAppObjectArgs(
                        Obj,
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoIdPoint,
                        static_cast<int>(Sketcher::PointPos::start),
                        GeoId2);

                    // add constraint: Perpendicular-via-point
                    Gui::cmdAppObjectArgs(
                        Obj,
                        "addConstraint(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,%d))",
                        GeoId1,
                        GeoId2,
                        GeoIdPoint,
                        static_cast<int>(Sketcher::PointPos::start));

                    commitCommand();
                }
                catch (const Base::Exception& e) {
                    Gui::NotifyUserError(Obj,
                                         QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                                         e.what());
                    Gui::Command::abortCommand();
                }

                tryAutoRecompute(Obj);

                getSelection().clearSelection();
                return;
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add perpendicular constraint"));
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('Perpendicular',%d,%d))",
                                  GeoId1,
                                  GeoId2);
            commitCommand();

            tryAutoRecompute(Obj);
            return;
        }
        case 4:// {SelVertexOrRoot, SelEdge, SelEdgeOrAxis}
        case 5:// {SelVertexOrRoot, SelEdgeOrAxis, SelEdge}
        case 6:// {SelVertexOrRoot, SelEdge, SelExternalEdge}
        case 7:// {SelVertexOrRoot, SelExternalEdge, SelEdge}
        {
            // let's sink the point to be GeoId3.
            GeoId1 = selSeq.at(1).GeoId;
            GeoId2 = selSeq.at(2).GeoId;
            GeoId3 = selSeq.at(0).GeoId;
            PosId3 = selSeq.at(0).PosId;

            break;
        }
        case 8: // {SelEdge, SelVertexOrRoot, SelEdgeOrAxis}
        case 9: // {SelEdgeOrAxis, SelVertexOrRoot, SelEdge}
        case 10:// {SelEdge, SelVertexOrRoot, SelExternalEdge}
        case 11:// {SelExternalEdge, SelVertexOrRoot, SelEdge}
        {
            // let's sink the point to be GeoId3.
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(2).GeoId;
            GeoId3 = selSeq.at(1).GeoId;
            PosId3 = selSeq.at(1).PosId;

            break;
        }
        default:
            return;
    }

    if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

        // check if the edge already has a Block constraint
        if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
            showNoConstraintBetweenFixedGeometry(Obj);
            return;
        }

        if (isBsplinePole(Obj, GeoId1) || isBsplinePole(Obj, GeoId2)) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Select an edge that is not a B-spline weight."));
            return;
        }

        openCommand(QT_TRANSLATE_NOOP("Command", "Add perpendicular constraint"));

        bool safe = addConstraintSafely(Obj, [&]() {
            // add missing point-on-object constraints
            if (!IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)) {
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId1);
            };

            if (!IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)) {
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId2);
            };

            if (!IsPointAlreadyOnCurve(
                    GeoId1,
                    GeoId3,
                    PosId3,
                    Obj)) {// FIXME: it's a good idea to add a check if the sketch is solved
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId1);
            };

            Gui::cmdAppObjectArgs(
                Obj,
                "addConstraint(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,%d))",
                GeoId1,
                GeoId2,
                GeoId3,
                static_cast<int>(PosId3));
        });

        if (!safe) {
            return;
        }
        else {
            commitCommand();
            tryAutoRecompute(Obj);
        }

        getSelection().clearSelection();

        return;
    };
}

// ======================================================================================

class CmdSketcherConstrainTangent: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainTangent();
    ~CmdSketcherConstrainTangent() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainTangent";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
    // returns true if a substitution took place
    static bool substituteConstraintCombinations(SketchObject* Obj, int GeoId1, int GeoId2);
};

CmdSketcherConstrainTangent::CmdSketcherConstrainTangent()
    : CmdSketcherConstraint("Sketcher_ConstrainTangent")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain tangent");
    sToolTipText = QT_TR_NOOP("Create a tangent constraint between two entities");
    sWhatsThis = "Sketcher_ConstrainTangent";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Tangent";
    sAccel = "T";
    eType = ForEdit;

    allowedSelSequences = {
        {SelEdge, SelEdgeOrAxis},
        {SelEdgeOrAxis, SelEdge},
        {SelEdge, SelExternalEdge},
        {SelExternalEdge, SelEdge}, /* Two Curves */
        {SelVertexOrRoot, SelEdge, SelEdgeOrAxis},
        {SelVertexOrRoot, SelEdgeOrAxis, SelEdge},
        {SelVertexOrRoot, SelEdge, SelExternalEdge},
        {SelVertexOrRoot, SelExternalEdge, SelEdge},
        {SelEdge, SelVertexOrRoot, SelEdgeOrAxis},
        {SelEdgeOrAxis, SelVertexOrRoot, SelEdge},
        {SelEdge, SelVertexOrRoot, SelExternalEdge},
        {SelExternalEdge, SelVertexOrRoot, SelEdge}, /* Two Curves and a Point */
        {SelVertexOrRoot, SelVertex} /*Two Endpoints*/ /*No Place for One Endpoint and One Curve*/};
}

bool CmdSketcherConstrainTangent::substituteConstraintCombinations(SketchObject* Obj,
                                                                   int GeoId1,
                                                                   int GeoId2)
{
    const std::vector<Constraint*>& cvals = Obj->Constraints.getValues();

    int cid = 0;
    for (std::vector<Constraint*>::const_iterator it = cvals.begin(); it != cvals.end();
         ++it, ++cid) {
        if ((*it)->Type == Sketcher::Coincident
            && (((*it)->First == GeoId1 && (*it)->Second == GeoId2)
                || ((*it)->Second == GeoId1 && (*it)->First == GeoId2))) {

            // save values because 'doEndpointTangency' changes the
            // constraint property and thus invalidates this iterator
            int first = (*it)->First;
            int firstpos = static_cast<int>((*it)->FirstPos);

            Gui::Command::openCommand(
                QT_TRANSLATE_NOOP("Command", "Swap coincident+tangency with ptp tangency"));

            doEndpointTangency(Obj, (*it)->First, (*it)->Second, (*it)->FirstPos, (*it)->SecondPos);

            Gui::cmdAppObjectArgs(Obj, "delConstraintOnPoint(%d,%d)", first, firstpos);

            commitCommand();
            Obj->solve();// The substitution requires a solve() so that the autoremove redundants
                         // works when Autorecompute not active.
            tryAutoRecomputeIfNotSolve(Obj);

            notifyConstraintSubstitutions(QObject::tr("Endpoint to endpoint tangency was applied. "
                                                      "The coincident constraint was deleted."));

            getSelection().clearSelection();
            return true;
        }
        else if ((*it)->Type == Sketcher::PointOnObject
                 && (((*it)->First == GeoId1 && (*it)->Second == GeoId2)
                     || ((*it)->Second == GeoId1 && (*it)->First == GeoId2))) {

            Gui::Command::openCommand(
                QT_TRANSLATE_NOOP("Command",
                                  "Swap PointOnObject+tangency with point to curve tangency"));

            doEndpointToEdgeTangency(Obj, (*it)->First, (*it)->FirstPos, (*it)->Second);

            Gui::cmdAppObjectArgs(Obj,
                                  "delConstraint(%d)",
                                  cid);// remove the preexisting point on object constraint.

            commitCommand();

            // A substitution requires a solve() so that the autoremove redundants works when
            // Autorecompute not active. However, delConstraint includes such solve() internally. So
            // at this point it is already solved.
            tryAutoRecomputeIfNotSolve(Obj);

            notifyConstraintSubstitutions(QObject::tr("Endpoint to edge tangency was applied. The "
                                                      "point on object constraint was deleted."));

            getSelection().clearSelection();
            return true;
        }
    }

    return false;
}

void CmdSketcherConstrainTangent::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            QString strBasicHelp =
                QObject::tr("There are a number of ways this constraint can be applied.\n\n"
                            "Accepted combinations: two curves; an endpoint and a curve; two "
                            "endpoints; two curves and a point.",
                            /*disambig.:*/ "tangent constraint");
            QString strError =
                QObject::tr("Select some geometry from the sketch.", "tangent constraint");
            strError.append(QString::fromLatin1("\n\n"));
            strError.append(strBasicHelp);
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       std::move(strError));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() != 2 && SubNames.size() != 3) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Wrong number of selected objects!"));
        return;
    }

    int GeoId1, GeoId2, GeoId3;
    Sketcher::PointPos PosId1, PosId2, PosId3;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (areBothPointsOrSegmentsFixed(Obj,
                                     GeoId1,
                                     GeoId2)) {// checkBothExternal displays error message
        showNoConstraintBetweenFixedGeometry(Obj);
        return;
    }
    if (SubNames.size() == 3) {// tangent via point
        getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);
        // let's sink the point to be GeoId3. We want to keep the order the two curves have been
        // selected in.
        if (isVertex(GeoId1, PosId1)) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
        };
        if (isVertex(GeoId2, PosId2)) {
            std::swap(GeoId2, GeoId3);
            std::swap(PosId2, PosId3);
        };

        if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

            if (isBsplinePole(Obj, GeoId1) || isBsplinePole(Obj, GeoId2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add tangent constraint"));

            bool safe = addConstraintSafely(Obj, [&]() {
                // add missing point-on-object constraints
                if (!IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)) {
                    Gui::cmdAppObjectArgs(
                        selection[0].getObject(),
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoId3,
                        static_cast<int>(PosId3),
                        GeoId1);
                };

                if (!IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)) {
                    Gui::cmdAppObjectArgs(
                        selection[0].getObject(),
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoId3,
                        static_cast<int>(PosId3),
                        GeoId2);
                };

                if (!IsPointAlreadyOnCurve(
                        GeoId1,
                        GeoId3,
                        PosId3,
                        Obj)) {// FIXME: it's a good idea to add a check if the sketch is solved
                    Gui::cmdAppObjectArgs(
                        selection[0].getObject(),
                        "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                        GeoId3,
                        static_cast<int>(PosId3),
                        GeoId1);
                };

                Gui::cmdAppObjectArgs(
                    selection[0].getObject(),
                    "addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d))",
                    GeoId1,
                    GeoId2,
                    GeoId3,
                    static_cast<int>(PosId3));
            });

            if (!safe) {
                return;
            }
            else {
                commitCommand();
                tryAutoRecompute(Obj);
            }

            getSelection().clearSelection();

            return;
        };

        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("With 3 objects, there must be 2 curves and 1 point."));
    }
    else if (SubNames.size() == 2) {

        if (isVertex(GeoId1, PosId1) && isVertex(GeoId2, PosId2)) {// endpoint-to-endpoint tangency

            if (isBsplineKnot(Obj, GeoId2)) {
                std::swap(GeoId1, GeoId2);
                std::swap(PosId1, PosId2);
            }

            if (isSimpleVertex(Obj, GeoId1, PosId1) || isSimpleVertex(Obj, GeoId2, PosId2)) {

                if (isBsplineKnot(Obj, GeoId1)) {
                    const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

                    if (! geom2 || ! isLineSegment(*geom2)) {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Wrong selection"),
                            QObject::tr("Tangent constraint at B-spline knot is only supported "
                                        "with lines!"));
                        return;
                    }
                }
                else {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Wrong selection"),
                        QObject::tr("Cannot add a tangency constraint at an unconnected point!"));
                    return;
                }
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add tangent constraint"));
            doEndpointTangency(Obj, GeoId1, GeoId2, PosId1, PosId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
        else if ((isVertex(GeoId1, PosId1) && isEdge(GeoId2, PosId2))
                 || (isEdge(GeoId1, PosId1)
                     && isVertex(GeoId2, PosId2))) {// endpoint-to-curve/knot-to-curve tangency
            if (isVertex(GeoId2, PosId2)) {
                std::swap(GeoId1, GeoId2);
                std::swap(PosId1, PosId2);
            }

            if (isSimpleVertex(Obj, GeoId1, PosId1)) {
                if (isBsplineKnot(Obj, GeoId1)) {
                    const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

                    if (!geom2 || ! isLineSegment(*geom2)) {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Wrong selection"),
                            QObject::tr("Tangent constraint at B-spline knot is only supported "
                                        "with lines!"));
                        return;
                    }
                }
                else {
                    Gui::TranslatedUserWarning(
                        Obj,
                        QObject::tr("Wrong selection"),
                        QObject::tr("Cannot add a tangency constraint at an unconnected point!"));
                    return;
                }
            }

            const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

            if (geom2 && isBSplineCurve(*geom2)) {
                // unsupported until tangent to B-spline at any point implemented.
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Tangency to B-spline edge currently unsupported."));
                return;
            }

            if (isBsplinePole(geom2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add tangent constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
        else if (isEdge(GeoId1, PosId1)
                 && isEdge(GeoId2, PosId2)) {// simple tangency between GeoId1 and GeoId2

            const Part::Geometry* geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

            if (geom1 && geom2 && (isBSplineCurve(*geom1) || isBSplineCurve(*geom2))) {
                // unsupported until tangent to B-spline at any point implemented.
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Tangency to B-spline edge currently unsupported."));
                return;
            }

            if (isBsplinePole(geom1) || isBsplinePole(geom2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            // check if as a consequence of this command undesirable combinations of constraints
            // would arise and substitute them with more appropriate counterparts, examples:
            // - coincidence + tangency on edge
            // - point on object + tangency on edge
            if (substituteConstraintCombinations(Obj, GeoId1, GeoId2)) {
                return;
            }

            if (geom1 && geom2 && (isEllipse(*geom1) || isEllipse(*geom2))) {
                if (! isEllipse(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                }

                // GeoId1 is the ellipse
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if (isEllipse(*geom2) || isArcOfEllipse(*geom2) || isCircle(*geom2) || isArcOfCircle(*geom2)) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToEllipseviaNewPoint(Obj,
                                                    static_cast<const Part::GeomEllipse*>(geom1),
                                                    geom2,
                                                    GeoId1,
                                                    GeoId2);
                    getSelection().clearSelection();
                    return;
                }
                else if (isArcOfHyperbola(*geom2)) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfHyperbolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfHyperbola*>(geom2),
                        geom1,
                        GeoId2,
                        GeoId1);
                    getSelection().clearSelection();
                    return;
                }
                else if (isArcOfParabola(*geom2)) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfParabolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfParabola*>(geom2),
                        geom1,
                        GeoId2,
                        GeoId1);
                    getSelection().clearSelection();
                    return;
                }
            }
            else if (geom1 && geom2 && (isArcOfEllipse(*geom1) || isArcOfEllipse(*geom2))) {
                if (! isArcOfEllipse(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                }

                // GeoId1 is the arc of ellipse
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if (isArcOfHyperbola(*geom2) || isArcOfEllipse(*geom2)
                    || isCircle(*geom2) || isArcOfCircle(*geom2) || isLineSegment(*geom2)) {

                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfEllipseviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfEllipse*>(geom1),
                        geom2,
                        GeoId1,
                        GeoId2);

                    getSelection().clearSelection();
                    return;
                }
                else if (isArcOfParabola(*geom2)) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfParabolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfParabola*>(geom2),
                        geom1,
                        GeoId2,
                        GeoId1);
                    getSelection().clearSelection();
                    return;
                }
            }
            else if (geom1 && geom2 && (isArcOfHyperbola(*geom1) || isArcOfHyperbola(*geom2))) {
                if (! isArcOfHyperbola(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                }

                // GeoId1 is the arc of hyperbola
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if (isArcOfHyperbola(*geom2) || isArcOfEllipse(*geom2) || isCircle(*geom2)
                    || isArcOfCircle(*geom2) || isLineSegment(*geom2)) {

                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfHyperbolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfHyperbola*>(geom1),
                        geom2,
                        GeoId1,
                        GeoId2);
                    getSelection().clearSelection();
                    return;
                }
                else if (isArcOfParabola(*geom2)) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfParabolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfParabola*>(geom2),
                        geom1,
                        GeoId2,
                        GeoId1);
                    getSelection().clearSelection();
                    return;
                }
            }
            else if (geom1 && geom2 && (isArcOfParabola(*geom1) || isArcOfParabola(*geom2))) {
                if (! isArcOfParabola(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                }

                // GeoId1 is the arc of hyperbola
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if (isArcOfParabola(*geom2) || isArcOfHyperbola(*geom2)
                    || isArcOfEllipse(*geom2) || isCircle(*geom2)
                    || isArcOfCircle(*geom2) || isLineSegment(*geom2)) {

                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfParabolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfParabola*>(geom1),
                        geom2,
                        GeoId1,
                        GeoId2);
                    getSelection().clearSelection();
                    return;
                }
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add tangent constraint"));
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Tangent',%d,%d))",
                                  GeoId1,
                                  GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
    }

    return;
}

void CmdSketcherConstrainTangent::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef, GeoId3 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::PointPos::none, PosId2 = Sketcher::PointPos::none,
                       PosId3 = Sketcher::PointPos::none;

    switch (seqIndex) {
        case 0:// {SelEdge, SelEdgeOrAxis}
        case 1:// {SelEdgeOrAxis, SelEdge}
        case 2:// {SelEdge, SelExternalEdge}
        case 3:// {SelExternalEdge, SelEdge}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;

            // check if the edge already has a Block constraint
            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }

            const Part::Geometry* geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

            if (geom1 && geom2 && (isBSplineCurve(*geom1) || isBSplineCurve(*geom2))) {
                // unsupported until tangent to B-spline at any point implemented.
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Tangency to B-spline edge currently unsupported."));
                return;
            }

            if (isBsplinePole(geom1) || isBsplinePole(geom2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            // check if as a consequence of this command undesirable combinations of constraints
            // would arise and substitute them with more appropriate counterparts, examples:
            // - coincidence + tangency on edge
            // - point on object + tangency on edge
            if (substituteConstraintCombinations(Obj, GeoId1, GeoId2)) {
                return;
            }

            if (geom1 && geom2 && (isEllipse(*geom1) || isEllipse(*geom2))) {
                if (! isEllipse(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                }

                // GeoId1 is the ellipse
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if (isEllipse(*geom2) || isArcOfEllipse(*geom2)
                    || isCircle(*geom2) || isArcOfCircle(*geom2)) {

                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToEllipseviaNewPoint(Obj,
                                                    static_cast<const Part::GeomEllipse*>(geom1),
                                                    geom2,
                                                    GeoId1,
                                                    GeoId2);
                    getSelection().clearSelection();
                    return;
                }
                else if (isArcOfHyperbola(*geom2)) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfHyperbolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfHyperbola*>(geom2),
                        geom1,
                        GeoId2,
                        GeoId1);
                    getSelection().clearSelection();
                    return;
                }
                else if (isArcOfParabola(*geom2)) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfParabolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfParabola*>(geom2),
                        geom1,
                        GeoId2,
                        GeoId1);
                    getSelection().clearSelection();
                    return;
                }
            }
            else if (geom1 && geom2 && (isArcOfHyperbola(*geom1) || isArcOfHyperbola(*geom2))) {
                if (! isArcOfHyperbola(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                }

                // GeoId1 is the arc of hyperbola
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if (isArcOfHyperbola(*geom2) || isArcOfEllipse(*geom2) || isCircle(*geom2)
                   || isArcOfCircle(*geom2) || isLineSegment(*geom2)) {

                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfHyperbolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfHyperbola*>(geom1),
                        geom2,
                        GeoId1,
                        GeoId2);
                    getSelection().clearSelection();
                    return;
                }
                else if (isArcOfParabola(*geom2)) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfParabolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfParabola*>(geom2),
                        geom1,
                        GeoId2,
                        GeoId1);
                    getSelection().clearSelection();
                    return;
                }
            }
            else if (geom1 && geom2 && (isArcOfParabola(*geom1) || isArcOfParabola(*geom2))) {
                if (! isArcOfParabola(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                }

                // GeoId1 is the arc of hyperbola
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if (isArcOfParabola(*geom2) || isArcOfHyperbola(*geom2) || isArcOfEllipse(*geom2)
                   || isCircle(*geom2) || isArcOfCircle(*geom2) || isLineSegment(*geom2)) {

                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add tangent constraint point"));
                    makeTangentToArcOfParabolaviaNewPoint(
                        Obj,
                        static_cast<const Part::GeomArcOfParabola*>(geom1),
                        geom2,
                        GeoId1,
                        GeoId2);
                    getSelection().clearSelection();
                    return;
                }
            }

            openCommand(QT_TRANSLATE_NOOP("Command", "Add tangent constraint"));
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('Tangent',%d,%d))",
                                  GeoId1,
                                  GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            return;
        }
        case 4:// {SelVertexOrRoot, SelEdge, SelEdgeOrAxis}
        case 5:// {SelVertexOrRoot, SelEdgeOrAxis, SelEdge}
        case 6:// {SelVertexOrRoot, SelEdge, SelExternalEdge}
        case 7:// {SelVertexOrRoot, SelExternalEdge, SelEdge}
        {
            // let's sink the point to be GeoId3.
            GeoId1 = selSeq.at(1).GeoId;
            GeoId2 = selSeq.at(2).GeoId;
            GeoId3 = selSeq.at(0).GeoId;
            PosId3 = selSeq.at(0).PosId;

            break;
        }
        case 8: // {SelEdge, SelVertexOrRoot, SelEdgeOrAxis}
        case 9: // {SelEdgeOrAxis, SelVertexOrRoot, SelEdge}
        case 10:// {SelEdge, SelVertexOrRoot, SelExternalEdge}
        case 11:// {SelExternalEdge, SelVertexOrRoot, SelEdge}
        {
            // let's sink the point to be GeoId3.
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(2).GeoId;
            GeoId3 = selSeq.at(1).GeoId;
            PosId3 = selSeq.at(1).PosId;

            break;
        }
        case 12:// {SelVertexOrRoot, SelVertex}
        {
            // Different notation than the previous places
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(1).PosId;

            // check if the edge already has a Block constraint
            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }

            if (isSimpleVertex(Obj, GeoId1, PosId1) || isSimpleVertex(Obj, GeoId2, PosId2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Cannot add a tangency constraint at an unconnected point!"));
                return;
            }

            // This code supports simple B-spline endpoint tangency to any other geometric curve
            const Part::Geometry* geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geom2 = Obj->getGeometry(GeoId2);

            if (geom1 && geom2 && (isBSplineCurve(*geom1) || isBSplineCurve(*geom2))) {
                if (! isBSplineCurve(*geom1)) {
                    std::swap(GeoId1, GeoId2);
                    std::swap(PosId1, PosId2);
                }
                // GeoId1 is the B-spline now
            }// end of code supports simple B-spline endpoint tangency

            openCommand(QT_TRANSLATE_NOOP("Command", "Add tangent constraint"));
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2));
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
        default:
            return;
    }

    if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

        // check if the edge already has a Block constraint
        if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
            showNoConstraintBetweenFixedGeometry(Obj);
            return;
        }

        if (isBsplinePole(Obj, GeoId1) || isBsplinePole(Obj, GeoId2)) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Select an edge that is not a B-spline weight."));
            return;
        }

        openCommand(QT_TRANSLATE_NOOP("Command", "Add tangent constraint"));

        bool safe = addConstraintSafely(Obj, [&]() {
            // add missing point-on-object constraints
            if (!IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)) {
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId1);
            };

            if (!IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)) {
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId2);
            };

            if (!IsPointAlreadyOnCurve(
                    GeoId1,
                    GeoId3,
                    PosId3,
                    Obj)) {// FIXME: it's a good idea to add a check if the sketch is solved
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId1);
            };

            Gui::cmdAppObjectArgs(
                Obj,
                "addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d))",
                GeoId1,
                GeoId2,
                GeoId3,
                static_cast<int>(PosId3));
        });

        if (!safe) {
            return;
        }
        else {
            commitCommand();
            tryAutoRecompute(Obj);
        }

        getSelection().clearSelection();

        return;
    };
}

// ======================================================================================

class CmdSketcherConstrainRadius: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainRadius();
    ~CmdSketcherConstrainRadius() override
    {}
    void updateAction(int mode) override;
    const char* className() const override
    {
        return "CmdSketcherConstrainRadius";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainRadius::CmdSketcherConstrainRadius()
    : CmdSketcherConstraint("Sketcher_ConstrainRadius")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain radius");
    sToolTipText = QT_TR_NOOP("Fix the radius of a circle or an arc");
    sWhatsThis = "Sketcher_ConstrainRadius";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Radius";
    sAccel = "K, R";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge}, {SelExternalEdge}};
}

void CmdSketcherConstrainRadius::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    // check for which selected geometry the constraint can be applied
    std::vector<std::pair<int, double>> geoIdRadiusMap;
    std::vector<std::pair<int, double>> externalGeoIdRadiusMap;

    bool poles = false;
    bool nonpoles = false;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        bool issegmentfixed = false;
        int GeoId;

        if (it->size() > 4 && it->substr(0, 4) == "Edge") {
            GeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
            issegmentfixed = isPointOrSegmentFixed(Obj, GeoId);
        }
        else if (it->size() > 4 && it->substr(0, 12) == "ExternalEdge") {
            GeoId = -std::atoi(it->substr(12, 4000).c_str()) - 2;
            issegmentfixed = true;
        }
        else {
            continue;
        }

        const Part::Geometry* geom = Obj->getGeometry(GeoId);

        if (geom && isArcOfCircle(*geom)) {
            auto arc = static_cast<const Part::GeomArcOfCircle*>(geom);
            double radius = arc->getRadius();

            if (issegmentfixed) {
                externalGeoIdRadiusMap.emplace_back(GeoId, radius);
            }
            else {
                geoIdRadiusMap.emplace_back(GeoId, radius);
            }

            nonpoles = true;
        }
        else if (geom && isCircle(*geom)) {
            auto circle = static_cast<const Part::GeomCircle*>(geom);
            double radius = circle->getRadius();

            if (issegmentfixed) {
                externalGeoIdRadiusMap.emplace_back(GeoId, radius);
            }
            else {
                geoIdRadiusMap.emplace_back(GeoId, radius);
            }

            if (isBsplinePole(geom)) {
                poles = true;
            }
            else {
                nonpoles = true;
            }
        }
    }

    if (geoIdRadiusMap.empty() && externalGeoIdRadiusMap.empty()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    if (poles && nonpoles) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select either only one or more B-Spline poles or only one or more arcs or "
                        "circles from the sketch, but not mixed."));
        return;
    }

    bool commitNeeded = false;
    bool updateNeeded = false;
    bool commandopened = false;

    if (!externalGeoIdRadiusMap.empty()) {
        // Create the non-driving radius constraints now
        openCommand(QT_TRANSLATE_NOOP("Command", "Add radius constraint"));
        commandopened = true;
        unsigned int constrSize = 0;

        for (std::vector<std::pair<int, double>>::iterator it = externalGeoIdRadiusMap.begin();
             it != externalGeoIdRadiusMap.end();
             ++it) {

            if (nonpoles) {
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "addConstraint(Sketcher.Constraint('Radius',%d,%f))",
                                      it->first,
                                      it->second);
            }
            else {
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "addConstraint(Sketcher.Constraint('Weight',%d,%f))",
                                      it->first,
                                      it->second);
            }

            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            constrSize = ConStr.size();

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "setDriving(%d,%s)",
                                  constrSize - 1,
                                  "False");
        }

        finishDatumConstraint(this, Obj, false, externalGeoIdRadiusMap.size());

        commitNeeded = true;
        updateNeeded = true;
    }

    if (!geoIdRadiusMap.empty()) {
        if (geoIdRadiusMap.size() > 1 && constraintCreationMode == Driving) {

            int refGeoId = geoIdRadiusMap.front().first;
            double radius = geoIdRadiusMap.front().second;

            if (!commandopened) {
                openCommand(QT_TRANSLATE_NOOP("Command", "Add radius constraint"));
            }

            // Add the equality constraints
            for (std::vector<std::pair<int, double>>::iterator it = geoIdRadiusMap.begin() + 1;
                 it != geoIdRadiusMap.end();
                 ++it) {
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "addConstraint(Sketcher.Constraint('Equal',%d,%d))",
                                      refGeoId,
                                      it->first);
            }

            if (nonpoles) {
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "addConstraint(Sketcher.Constraint('Radius',%d,%f))",
                                      refGeoId,
                                      radius);
            }
            else {
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "addConstraint(Sketcher.Constraint('Weight',%d,%f))",
                                      refGeoId,
                                      radius);
            }
        }
        else {
            // Create the radius constraints now
            if (!commandopened) {
                openCommand(QT_TRANSLATE_NOOP("Command", "Add radius constraint"));
            }
            for (std::vector<std::pair<int, double>>::iterator it = geoIdRadiusMap.begin();
                 it != geoIdRadiusMap.end();
                 ++it) {
                if (nonpoles) {
                    Gui::cmdAppObjectArgs(selection[0].getObject(),
                                          "addConstraint(Sketcher.Constraint('Radius',%d,%f))",
                                          it->first,
                                          it->second);
                }
                else {
                    Gui::cmdAppObjectArgs(selection[0].getObject(),
                                          "addConstraint(Sketcher.Constraint('Weight',%d,%f))",
                                          it->first,
                                          it->second);
                }

                if (constraintCreationMode == Reference) {
                    const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
                    Gui::cmdAppObjectArgs(selection[0].getObject(),
                                          "setDriving(%d,%s)",
                                          ConStr.size() - 1,
                                          "False");
                }
            }
        }

        finishDatumConstraint(this, Obj, constraintCreationMode == Driving);

        // updateActive();
        getSelection().clearSelection();
    }

    if (commitNeeded) {
        commitCommand();
    }

    if (updateNeeded) {
        tryAutoRecomputeIfNotSolve(Obj);// we have to update the solver after this aborted addition.
    }
}

void CmdSketcherConstrainRadius::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId = selSeq.at(0).GeoId;
    double radius = 0.0;

    bool updateNeeded = false;

    switch (seqIndex) {
        case 0:// {SelEdge}
        case 1:// {SelExternalEdge}
        {
            const Part::Geometry* geom = Obj->getGeometry(GeoId);

            if (geom && isArcOfCircle(*geom)) {
                auto arc = static_cast<const Part::GeomArcOfCircle*>(geom);
                radius = arc->getRadius();
            }
            else if (geom && isCircle(*geom)) {
                auto circle = static_cast<const Part::GeomCircle*>(geom);
                radius = circle->getRadius();
            }
            else {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Constraint only applies to arcs or circles."));
                return;
            }

            // Create the radius constraint now
            openCommand(QT_TRANSLATE_NOOP("Command", "Add radius constraint"));

            bool ispole = isBsplinePole(geom);

            if (ispole) {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Weight',%d,%f))",
                                      GeoId,
                                      radius);
            }
            else {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Radius',%d,%f))",
                                      GeoId,
                                      radius);
            }

            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            bool fixed = isPointOrSegmentFixed(Obj, GeoId);
            if (fixed || constraintCreationMode == Reference) {
                Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");

                updateNeeded = true;// We do need to update the solver DoF after setting the
                                    // constraint driving.
            }

            finishDatumConstraint(this, Obj, constraintCreationMode == Driving && !fixed);

            // updateActive();
            getSelection().clearSelection();

            commitCommand();

            if (updateNeeded) {
                tryAutoRecomputeIfNotSolve(
                    Obj);// we have to update the solver after this aborted addition.
            }
        }
    }
}

void CmdSketcherConstrainRadius::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_Radius_Driven"));
            }
            break;
        case Driving:
            if (getAction()) {
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius"));
            }
            break;
    }
}

// ======================================================================================

class CmdSketcherConstrainDiameter: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainDiameter();
    ~CmdSketcherConstrainDiameter() override
    {}
    void updateAction(int mode) override;
    const char* className() const override
    {
        return "CmdSketcherConstrainDiameter";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainDiameter::CmdSketcherConstrainDiameter()
    : CmdSketcherConstraint("Sketcher_ConstrainDiameter")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain diameter");
    sToolTipText = QT_TR_NOOP("Fix the diameter of a circle or an arc");
    sWhatsThis = "Sketcher_ConstrainDiameter";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Diameter";
    sAccel = "K, O";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge}, {SelExternalEdge}};
}

void CmdSketcherConstrainDiameter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    // check for which selected geometry the constraint can be applied
    std::vector<std::pair<int, double>> geoIdDiameterMap;
    std::vector<std::pair<int, double>> externalGeoIdDiameterMap;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        bool issegmentfixed = false;
        int GeoId;

        if (it->size() > 4 && it->substr(0, 4) == "Edge") {
            GeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
            issegmentfixed = isPointOrSegmentFixed(Obj, GeoId);
        }
        else if (it->size() > 4 && it->substr(0, 12) == "ExternalEdge") {
            GeoId = -std::atoi(it->substr(12, 4000).c_str()) - 2;
            issegmentfixed = true;
        }
        else {
            continue;
        }

        const Part::Geometry* geom = Obj->getGeometry(GeoId);

        if (geom && isArcOfCircle(*geom)) {
            auto arc = static_cast<const Part::GeomArcOfCircle*>(geom);
            double radius = arc->getRadius();

            if (issegmentfixed) {
                externalGeoIdDiameterMap.emplace_back(GeoId, 2 * radius);
            }
            else {
                geoIdDiameterMap.emplace_back(GeoId, 2 * radius);
            }
        }
        else if (geom && isCircle(*geom)) {
            auto circle = static_cast<const Part::GeomCircle*>(geom);
            double radius = circle->getRadius();

            if (isBsplinePole(geom)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));

                continue;
            }

            if (issegmentfixed) {
                externalGeoIdDiameterMap.emplace_back(GeoId, 2 * radius);
            }
            else {
                geoIdDiameterMap.emplace_back(GeoId, 2 * radius);
            }
        }
    }

    if (geoIdDiameterMap.empty() && externalGeoIdDiameterMap.empty()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    bool commitNeeded = false;
    bool updateNeeded = false;
    bool commandopened = false;

    if (!externalGeoIdDiameterMap.empty()) {
        // Create the non-driving radius constraints now
        openCommand(QT_TRANSLATE_NOOP("Command", "Add diameter constraint"));
        commandopened = true;
        unsigned int constrSize = 0;

        for (std::vector<std::pair<int, double>>::iterator it = externalGeoIdDiameterMap.begin();
             it != externalGeoIdDiameterMap.end();
             ++it) {
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('Diameter',%d,%f))",
                                  it->first,
                                  it->second);

            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            constrSize = ConStr.size();

            Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", constrSize - 1, "False");
        }

        finishDatumConstraint(this, Obj, false, externalGeoIdDiameterMap.size());

        commitNeeded = true;
        updateNeeded = true;
    }

    if (!geoIdDiameterMap.empty()) {
        if (geoIdDiameterMap.size() > 1 && constraintCreationMode == Driving) {

            int refGeoId = geoIdDiameterMap.front().first;
            double diameter = geoIdDiameterMap.front().second;

            if (!commandopened) {
                openCommand(QT_TRANSLATE_NOOP("Command", "Add diameter constraint"));
            }

            // Add the equality constraints
            for (std::vector<std::pair<int, double>>::iterator it = geoIdDiameterMap.begin() + 1;
                 it != geoIdDiameterMap.end();
                 ++it) {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Equal',%d,%d))",
                                      refGeoId,
                                      it->first);
            }

            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('Diameter',%d,%f))",
                                  refGeoId,
                                  diameter);
        }
        else {
            // Create the diameter constraints now
            if (!commandopened) {
                openCommand(QT_TRANSLATE_NOOP("Command", "Add diameter constraint"));
            }
            for (std::vector<std::pair<int, double>>::iterator it = geoIdDiameterMap.begin();
                 it != geoIdDiameterMap.end();
                 ++it) {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Diameter',%d,%f))",
                                      it->first,
                                      it->second);

                if (constraintCreationMode == Reference) {

                    const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                    Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
                }
            }
        }

        finishDatumConstraint(this, Obj, constraintCreationMode == Driving);

        // updateActive();
        getSelection().clearSelection();
    }

    if (commitNeeded) {
        commitCommand();
    }

    if (updateNeeded) {
        tryAutoRecomputeIfNotSolve(Obj);// we have to update the solver after this aborted addition.
    }
}

void CmdSketcherConstrainDiameter::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId = selSeq.at(0).GeoId;
    double diameter = 0.0;

    bool updateNeeded = false;

    switch (seqIndex) {
        case 0:// {SelEdge}
        case 1:// {SelExternalEdge}
        {
            const Part::Geometry* geom = Obj->getGeometry(GeoId);

            if (geom && isArcOfCircle(*geom)) {
                auto arc = static_cast<const Part::GeomArcOfCircle*>(geom);
                diameter = 2 * arc->getRadius();
            }
            else if (geom && isCircle(*geom)) {
                auto circle = static_cast<const Part::GeomCircle*>(geom);
                diameter = 2 * circle->getRadius();
            }
            else {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Constraint only applies to arcs or circles."));
                return;
            }

            if (isBsplinePole(geom)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            // Create the diameter constraint now
            openCommand(QT_TRANSLATE_NOOP("Command", "Add diameter constraint"));
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('Diameter',%d,%f))",
                                  GeoId,
                                  diameter);

            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            bool fixed = isPointOrSegmentFixed(Obj, GeoId);
            if (fixed || constraintCreationMode == Reference) {
                Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
                updateNeeded = true;// We do need to update the solver DoF after setting the
                                    // constraint driving.
            }

            finishDatumConstraint(this, Obj, constraintCreationMode == Driving && !fixed);

            // updateActive();
            getSelection().clearSelection();

            commitCommand();

            if (updateNeeded) {
                tryAutoRecomputeIfNotSolve(
                    Obj);// we have to update the solver after this aborted addition.
            }
        }
    }
}

void CmdSketcherConstrainDiameter::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_Diameter_Driven"));
            }
            break;
        case Driving:
            if (getAction()) {
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter"));
            }
            break;
    }
}

// ======================================================================================

class CmdSketcherConstrainRadiam: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainRadiam();
    ~CmdSketcherConstrainRadiam() override
    {}
    void updateAction(int mode) override;
    const char* className() const override
    {
        return "CmdSketcherConstrainRadiam";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainRadiam::CmdSketcherConstrainRadiam()
    : CmdSketcherConstraint("Sketcher_ConstrainRadiam")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain auto radius/diameter");
    sToolTipText = QT_TR_NOOP(
        "Fix the diameter if a circle is chosen, or the radius if an arc/spline pole is chosen");
    sWhatsThis = "Sketcher_ConstrainRadiam";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Radiam";
    sAccel = "K, S";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge}, {SelExternalEdge}};
}

void CmdSketcherConstrainRadiam::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    // check for which selected geometry the constraint can be applied
    std::vector<std::pair<int, double>> geoIdRadiamMap;
    std::vector<std::pair<int, double>> externalGeoIdRadiamMap;

    bool poles = false;
    bool nonpoles = false;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        bool issegmentfixed = false;
        int GeoId;

        if (it->size() > 4 && it->substr(0, 4) == "Edge") {
            GeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
            issegmentfixed = isPointOrSegmentFixed(Obj, GeoId);
        }
        else if (it->size() > 4 && it->substr(0, 12) == "ExternalEdge") {
            GeoId = -std::atoi(it->substr(12, 4000).c_str()) - 2;
            issegmentfixed = true;
        }
        else {
            continue;
        }

        const Part::Geometry* geom = Obj->getGeometry(GeoId);
        double radius;

        if (geom && isArcOfCircle(*geom)) {
            auto arcir = static_cast<const Part::GeomArcOfCircle*>(geom);
            radius = arcir->getRadius();
            nonpoles = true;
        }
        else if (geom && isCircle(*geom)) {
            auto arcir = static_cast<const Part::GeomCircle*>(geom);
            radius = arcir->getRadius();
            if (isBsplinePole(geom)) {
                poles = true;
            }
            else {
                nonpoles = true;
            }
        }
        else {
            continue;
        }

        if (issegmentfixed) {
            externalGeoIdRadiamMap.emplace_back(GeoId, radius);
        }
        else {
            geoIdRadiamMap.emplace_back(GeoId, radius);
        }
    }

    if (geoIdRadiamMap.empty() && externalGeoIdRadiamMap.empty()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    if (poles && nonpoles) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select either only one or more B-Spline poles or only one or more arcs or "
                        "circles from the sketch, but not mixed."));
        return;
    }

    bool commitNeeded = false;
    bool updateNeeded = false;
    bool commandopened = false;

    if (!externalGeoIdRadiamMap.empty()) {
        // Create the non-driving radiam constraints now
        openCommand(QT_TRANSLATE_NOOP("Command", "Add radiam constraint"));
        commandopened = true;
        unsigned int constrSize = 0;

        for (std::vector<std::pair<int, double>>::iterator it = externalGeoIdRadiamMap.begin();
             it != externalGeoIdRadiamMap.end();
             ++it) {
            if (isArcOfCircle(*(Obj->getGeometry(it->first)))) {
                if (nonpoles) {
                    Gui::cmdAppObjectArgs(Obj,
                                          "addConstraint(Sketcher.Constraint('Radius',%d,%f))",
                                          it->first,
                                          it->second);
                }
                else {
                    Gui::cmdAppObjectArgs(Obj,
                                          "addConstraint(Sketcher.Constraint('Weight',%d,%f))",
                                          it->first,
                                          it->second);
                }
            }
            else {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Diameter',%d,%f))",
                                      it->first,
                                      it->second * 2);
            }

            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            constrSize = ConStr.size();

            Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", constrSize - 1, "False");
        }

        finishDatumConstraint(this, Obj, false, externalGeoIdRadiamMap.size());

        commitNeeded = true;
        updateNeeded = true;
    }

    if (!geoIdRadiamMap.empty()) {
        if (geoIdRadiamMap.size() > 1 && constraintCreationMode == Driving) {

            int refGeoId = geoIdRadiamMap.front().first;
            double radiam = geoIdRadiamMap.front().second;

            if (!commandopened) {
                openCommand(QT_TRANSLATE_NOOP("Command", "Add radiam constraint"));
            }

            // Add the equality constraints
            for (std::vector<std::pair<int, double>>::iterator it = geoIdRadiamMap.begin() + 1;
                 it != geoIdRadiamMap.end();
                 ++it) {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Equal',%d,%d))",
                                      refGeoId,
                                      it->first);
            }

            if (poles) {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Weight',%d,%f))",
                                      refGeoId,
                                      radiam);
            }
            else if (isCircle(*(Obj->getGeometry(refGeoId)))) {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Diameter',%d,%f))",
                                      refGeoId,
                                      radiam * 2);
            }
            else {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Radius',%d,%f))",
                                      refGeoId,
                                      radiam);
            }
        }
        else {
            // Create the radiam constraints now
            if (!commandopened) {
                openCommand(QT_TRANSLATE_NOOP("Command", "Add radiam constraint"));
            }
            for (std::vector<std::pair<int, double>>::iterator it = geoIdRadiamMap.begin();
                 it != geoIdRadiamMap.end();
                 ++it) {
                if (poles) {
                    Gui::cmdAppObjectArgs(Obj,
                                          "addConstraint(Sketcher.Constraint('Weight',%d,%f))",
                                          it->first,
                                          it->second);
                }
                else if (isCircle(*(Obj->getGeometry(it->first)))){
                    Gui::cmdAppObjectArgs(Obj,
                                          "addConstraint(Sketcher.Constraint('Diameter',%d,%f))",
                                          it->first,
                                          it->second * 2);
                }
                else {
                    Gui::cmdAppObjectArgs(Obj,
                                          "addConstraint(Sketcher.Constraint('Radius',%d,%f))",
                                          it->first,
                                          it->second);
                }

                if (constraintCreationMode == Reference) {

                    const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                    Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
                }
            }
        }

        finishDatumConstraint(this, Obj, constraintCreationMode == Driving);

        // updateActive();
        getSelection().clearSelection();
    }

    if (commitNeeded) {
        commitCommand();
    }

    if (updateNeeded) {
        tryAutoRecomputeIfNotSolve(Obj);// we have to update the solver after this aborted addition.
    }
}

void CmdSketcherConstrainRadiam::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId = selSeq.at(0).GeoId;
    double radiam = 0.0;

    bool updateNeeded = false;

    bool isCircleGeom = false;
    bool isPole = false;

    switch (seqIndex) {
        case 0:// {SelEdge}
        case 1:// {SelExternalEdge}
        {
            const Part::Geometry* geom = Obj->getGeometry(GeoId);

            if (geom && isArcOfCircle(*geom)) {
                auto arc = static_cast<const Part::GeomArcOfCircle*>(geom);
                radiam = arc->getRadius();
            }
            else if (geom && isCircle(*geom)) {
                auto circle = static_cast<const Part::GeomCircle*>(geom);
                radiam = circle->getRadius();
                isCircleGeom= true;
                if (isBsplinePole(geom)) {
                    isPole = true;
                }
            }
            else {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Constraint only applies to arcs or circles."));
                return;
            }

            // Create the radiam constraint now
            openCommand(QT_TRANSLATE_NOOP("Command", "Add radiam constraint"));

            if (isPole) {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Weight',%d,%f))",
                                      GeoId,
                                      radiam);
            }
            else if (isCircleGeom) {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Diameter',%d,%f))",
                                      GeoId,
                                      radiam * 2);
            }
            else {
                Gui::cmdAppObjectArgs(Obj,
                                      "addConstraint(Sketcher.Constraint('Radius',%d,%f))",
                                      GeoId,
                                      radiam);
            }

            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            bool fixed = isPointOrSegmentFixed(Obj, GeoId);
            if (fixed || constraintCreationMode == Reference) {
                Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
                updateNeeded = true;// We do need to update the solver DoF after setting the
                                    // constraint driving.
            }

            finishDatumConstraint(this, Obj, constraintCreationMode == Driving && !fixed);

            // updateActive();
            getSelection().clearSelection();

            commitCommand();

            if (updateNeeded) {
                tryAutoRecomputeIfNotSolve(
                    Obj);// we have to update the solver after this aborted addition.
            }
        }
    }
}

void CmdSketcherConstrainRadiam::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_Radiam_Driven"));
            }
            break;
        case Driving:
            if (getAction()) {
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radiam"));
            }
            break;
    }
}

// ======================================================================================

DEF_STD_CMD_ACLU(CmdSketcherCompConstrainRadDia)

CmdSketcherCompConstrainRadDia::CmdSketcherCompConstrainRadDia()
    : Command("Sketcher_CompConstrainRadDia")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain arc or circle");
    sToolTipText = QT_TR_NOOP("Constrain an arc or a circle");
    sWhatsThis = "Sketcher_CompConstrainRadDia";
    sStatusTip = sToolTipText;
    sAccel = "R";
    eType = ForEdit;
}

void CmdSketcherCompConstrainRadDia::activated(int iMsg)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    if (iMsg == 0) {
        rcCmdMgr.runCommandByName("Sketcher_ConstrainRadius");
    }
    else if (iMsg == 1) {
        rcCmdMgr.runCommandByName("Sketcher_ConstrainDiameter");
    }
    else if (iMsg == 2) {
        rcCmdMgr.runCommandByName("Sketcher_ConstrainRadiam");
    }
    else {
        return;
    }

    // Save new choice as default
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    hGrp->SetInt("CurRadDiaCons", iMsg);

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompConstrainRadDia::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc1 = pcAction->addAction(QString());
    arc1->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius"));
    QAction* arc2 = pcAction->addAction(QString());
    arc2->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter"));
    QAction* arc3 = pcAction->addAction(QString());
    arc3->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radiam"));

    _pcAction = pcAction;
    languageChange();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    int curRadDiaCons = hGrp->GetInt("CurRadDiaCons", 2);

    switch (curRadDiaCons) {
        case 0:
            pcAction->setIcon(arc1->icon());
            break;
        case 1:
            pcAction->setIcon(arc2->icon());
            break;
        default:
            pcAction->setIcon(arc3->icon());
            curRadDiaCons = 2;
    }
    pcAction->setProperty("defaultAction", QVariant(curRadDiaCons));
    pcAction->setShortcut(QString::fromLatin1(getAccel()));

    return pcAction;
}

void CmdSketcherCompConstrainRadDia::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction) {
        return;
    }

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
        case Reference:
            a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius_Driven"));
            a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter_Driven"));
            a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radiam_Driven"));
            getAction()->setIcon(a[index]->icon());
            break;
        case Driving:
            a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius"));
            a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter"));
            a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radiam"));
            getAction()->setIcon(a[index]->icon());
            break;
    }
}

void CmdSketcherCompConstrainRadDia::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdSketcherCompConstrainRadDia", "Constrain radius"));
    arc1->setToolTip(QApplication::translate("Sketcher_ConstrainRadius",
                                             "Fix the radius of a circle or an arc"));
    arc1->setStatusTip(QApplication::translate("Sketcher_ConstrainRadius",
                                               "Fix the radius of a circle or an arc"));
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdSketcherCompConstrainRadDia", "Constrain diameter"));
    arc2->setToolTip(QApplication::translate("Sketcher_ConstrainDiameter",
                                             "Fix the diameter of a circle or an arc"));
    arc2->setStatusTip(QApplication::translate("Sketcher_ConstrainDiameter",
                                               "Fix the diameter of a circle or an arc"));
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdSketcherCompConstrainRadDia",
                                          "Constrain auto radius/diameter"));
    arc3->setToolTip(QApplication::translate("Sketcher_ConstrainRadiam",
                                             "Fix the radius/diameter of a circle or an arc"));
    arc3->setStatusTip(QApplication::translate("Sketcher_ConstrainRadiam",
                                               "Fix the radius/diameter of a circle or an arc"));
}

bool CmdSketcherCompConstrainRadDia::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================

class CmdSketcherConstrainAngle: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainAngle();
    ~CmdSketcherConstrainAngle() override
    {}
    void updateAction(int mode) override;
    const char* className() const override
    {
        return "CmdSketcherConstrainAngle";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainAngle::CmdSketcherConstrainAngle()
    : CmdSketcherConstraint("Sketcher_ConstrainAngle")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain angle");
    sToolTipText = QT_TR_NOOP("Fix the angle of a line or the angle between two lines");
    sWhatsThis = "Sketcher_ConstrainAngle";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_InternalAngle";
    sAccel = "K, A";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge, SelEdgeOrAxis},
                           {SelEdgeOrAxis, SelEdge},
                           {SelEdge, SelExternalEdge},
                           {SelExternalEdge, SelEdge},
                           {SelExternalEdge, SelExternalEdge},
                           {SelEdge, SelVertexOrRoot, SelEdgeOrAxis},
                           {SelEdgeOrAxis, SelVertexOrRoot, SelEdge},
                           {SelEdge, SelVertexOrRoot, SelExternalEdge},
                           {SelExternalEdge, SelVertexOrRoot, SelEdge},
                           {SelExternalEdge, SelVertexOrRoot, SelExternalEdge},
                           {SelVertexOrRoot, SelEdge, SelEdgeOrAxis},
                           {SelVertexOrRoot, SelEdgeOrAxis, SelEdge},
                           {SelVertexOrRoot, SelEdge, SelExternalEdge},
                           {SelVertexOrRoot, SelExternalEdge, SelEdge},
                           {SelVertexOrRoot, SelExternalEdge, SelExternalEdge}};
}

void CmdSketcherConstrainAngle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // TODO: comprehensive messages, like in CmdSketcherConstrainTangent
    //  get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty() || SubNames.size() > 3) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr(
                "Select one or two lines from the sketch. Or select two edges and a point."));
        return;
    }

    int GeoId1, GeoId2 = GeoEnum::GeoUndef, GeoId3 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1, PosId2 = Sketcher::PointPos::none, PosId3 = Sketcher::PointPos::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() > 1) {
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);
    }
    if (SubNames.size() > 2) {
        getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);
    }

    if (SubNames.size() == 3) {// standalone implementation of angle-via-point

        // let's sink the point to be GeoId3. We want to keep the order the two curves have been
        // selected in.
        if (isVertex(GeoId1, PosId1)) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
        };
        if (isVertex(GeoId2, PosId2)) {
            std::swap(GeoId2, GeoId3);
            std::swap(PosId2, PosId3);
        };

        bool bothexternal = areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

        if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

            if (isBsplinePole(Obj, GeoId1) || isBsplinePole(Obj, GeoId2)) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select an edge that is not a B-spline weight."));
                return;
            }

            double ActAngle = 0.0;

            openCommand(QT_TRANSLATE_NOOP("Command", "Add angle constraint"));

            // add missing point-on-object constraints
            if (!IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)) {
                Gui::cmdAppObjectArgs(
                    selection[0].getObject(),
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId1);
            }
            if (!IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)) {
                Gui::cmdAppObjectArgs(
                    selection[0].getObject(),
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId2);
            }
            if (!IsPointAlreadyOnCurve(
                    GeoId1,
                    GeoId3,
                    PosId3,
                    Obj)) {// FIXME: it's a good idea to add a check if the sketch is solved
                Gui::cmdAppObjectArgs(
                    selection[0].getObject(),
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                    GeoId3,
                    static_cast<int>(PosId3),
                    GeoId1);
            }

            // assuming point-on-curves have been solved, calculate the angle.
            // DeepSOIC: this may be slow, but I wanted to reuse the conversion
            // from Geometry to GCS shapes that is done in Sketch
            Base::Vector3d p = Obj->getPoint(GeoId3, PosId3);
            ActAngle = Obj->calculateAngleViaPoint(GeoId1, GeoId2, p.x, p.y);

            // negative constraint value avoidance
            if (ActAngle < -Precision::Angular()) {
                std::swap(GeoId1, GeoId2);
                std::swap(PosId1, PosId2);
                ActAngle = -ActAngle;
            }

            Gui::cmdAppObjectArgs(
                selection[0].getObject(),
                "addConstraint(Sketcher.Constraint('AngleViaPoint',%d,%d,%d,%d,%f))",
                GeoId1,
                GeoId2,
                GeoId3,
                static_cast<int>(PosId3),
                ActAngle);

            if (bothexternal
                || constraintCreationMode
                    == Reference) {// it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "setDriving(%d,%s)",
                                      ConStr.size() - 1,
                                      "False");
                finishDatumConstraint(this, Obj, false);
            }
            else {
                finishDatumConstraint(this, Obj, true);
            }

            return;
        };
    }
    else if (SubNames.size() < 3) {

        bool bothexternal = areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

        if (isVertex(GeoId1, PosId1) && isEdge(GeoId2, PosId2)) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
        }

        if (isBsplinePole(Obj, GeoId1)
            || (GeoId2 != GeoEnum::GeoUndef && isBsplinePole(Obj, GeoId2))) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Select an edge that is not a B-spline weight."));
            return;
        }

        if (isEdge(GeoId2, PosId2)) {// line to line angle
            makeAngleBetweenTwoLines(Obj, this, GeoId1, GeoId2);
            return;
        }
        else if (isEdge(GeoId1, PosId1)) {// line angle or arc angle
            if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Cannot add an angle constraint on an axis!"));
                return;
            }

            const Part::Geometry* geom = Obj->getGeometry(GeoId1);

            if (isLineSegment(*geom)) {
                auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
                Base::Vector3d dir = lineSeg->getEndPoint() - lineSeg->getStartPoint();
                double ActAngle = atan2(dir.y, dir.x);

                openCommand(QT_TRANSLATE_NOOP("Command", "Add angle constraint"));
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "addConstraint(Sketcher.Constraint('Angle',%d,%f))",
                                      GeoId1,
                                      ActAngle);

                if (GeoId1 <= Sketcher::GeoEnum::RefExt || constraintCreationMode == Reference) {
                    // it is a constraint on a external line, make it non-driving
                    const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                    Gui::cmdAppObjectArgs(selection[0].getObject(),
                                          "setDriving(%d,%s)",
                                          ConStr.size() - 1,
                                          "False");
                    finishDatumConstraint(this, Obj, false);
                }
                else {
                    finishDatumConstraint(this, Obj, true);
                }

                return;
            }
            else if (isArcOfCircle(*geom)) {
                auto arc = static_cast<const Part::GeomArcOfCircle*>(geom);
                double angle = arc->getAngle(/*EmulateCCWXY=*/true);

                openCommand(QT_TRANSLATE_NOOP("Command", "Add angle constraint"));
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "addConstraint(Sketcher.Constraint('Angle',%d,%f))",
                                      GeoId1,
                                      angle);

                if (GeoId1 <= Sketcher::GeoEnum::RefExt || constraintCreationMode == Reference) {
                    // it is a constraint on a external line, make it non-driving
                    const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

                    Gui::cmdAppObjectArgs(selection[0].getObject(),
                                          "setDriving(%d,%s)",
                                          ConStr.size() - 1,
                                          "False");
                    finishDatumConstraint(this, Obj, false);
                }
                else {
                    finishDatumConstraint(this, Obj, true);
                }

                return;
            }
        }
    };

    Gui::TranslatedUserWarning(
        Obj,
        QObject::tr("Wrong selection"),
        QObject::tr("Select one or two lines from the sketch. Or select two edges and a point."));
    return;
}

void CmdSketcherConstrainAngle::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef, GeoId3 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::PointPos::none, PosId2 = Sketcher::PointPos::none,
                       PosId3 = Sketcher::PointPos::none;

    switch (seqIndex) {
        case 0:// {SelEdge, SelEdgeOrAxis}
        case 1:// {SelEdgeOrAxis, SelEdge}
        case 2:// {SelEdge, SelExternalEdge}
        case 3:// {SelExternalEdge, SelEdge}
        case 4:// {SelExternalEdge, SelExternalEdge}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;

            makeAngleBetweenTwoLines(Obj, this, GeoId1, GeoId2);
            return;
        }
        case 5:// {SelEdge, SelVertexOrRoot, SelEdgeOrAxis}
        case 6:// {SelEdgeOrAxis, SelVertexOrRoot, SelEdge}
        case 7:// {SelEdge, SelVertexOrRoot, SelExternalEdge}
        case 8:// {SelExternalEdge, SelVertexOrRoot, SelEdge}
        case 9:// {SelExternalEdge, SelVertexOrRoot, SelExternalEdge}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(2).GeoId;
            GeoId3 = selSeq.at(1).GeoId;
            PosId3 = selSeq.at(1).PosId;
            break;
        }
        case 10:// {SelVertexOrRoot, SelEdge, SelEdgeOrAxis}
        case 11:// {SelVertexOrRoot, SelEdgeOrAxis, SelEdge}
        case 12:// {SelVertexOrRoot, SelEdge, SelExternalEdge}
        case 13:// {SelVertexOrRoot, SelExternalEdge, SelEdge}
        case 14:// {SelVertexOrRoot, SelExternalEdge, SelExternalEdge}
        {
            GeoId1 = selSeq.at(1).GeoId;
            GeoId2 = selSeq.at(2).GeoId;
            GeoId3 = selSeq.at(0).GeoId;
            PosId3 = selSeq.at(0).PosId;
            break;
        }
    }

    bool bothexternal = areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

    if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

        if (isBsplinePole(Obj, GeoId1) || isBsplinePole(Obj, GeoId2)) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Select an edge that is not a B-spline weight."));
            return;
        }

        double ActAngle = 0.0;

        openCommand(QT_TRANSLATE_NOOP("Command", "Add angle constraint"));

        // add missing point-on-object constraints
        if (!IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)) {
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                                  GeoId3,
                                  static_cast<int>(PosId3),
                                  GeoId1);
        }
        if (!IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)) {
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                                  GeoId3,
                                  static_cast<int>(PosId3),
                                  GeoId2);
        }
        if (!IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)) {
            // FIXME: it's a good idea to add a check if the sketch is solved
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                                  GeoId3,
                                  static_cast<int>(PosId3),
                                  GeoId1);
        }

        // assuming point-on-curves have been solved, calculate the angle.
        // DeepSOIC: this may be slow, but I wanted to reuse the conversion
        // from Geometry to GCS shapes that is done in Sketch
        Base::Vector3d p = Obj->getPoint(GeoId3, PosId3);
        ActAngle = Obj->calculateAngleViaPoint(GeoId1, GeoId2, p.x, p.y);

        // negative constraint value avoidance
        if (ActAngle < -Precision::Angular()) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
            ActAngle = -ActAngle;
        }

        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('AngleViaPoint',%d,%d,%d,%d,%f))",
                              GeoId1,
                              GeoId2,
                              GeoId3,
                              static_cast<int>(PosId3),
                              ActAngle);

        if (bothexternal || constraintCreationMode == Reference) {
            // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();

            Gui::cmdAppObjectArgs(Obj, "setDriving(%d,%s)", ConStr.size() - 1, "False");
            finishDatumConstraint(this, Obj, false);
        }
        else {
            finishDatumConstraint(this, Obj, true);
        }

        return;
    };
}

void CmdSketcherConstrainAngle::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle_Driven"));
            }
            break;
        case Driving:
            if (getAction()) {
                getAction()->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle"));
            }
            break;
    }
}

// ======================================================================================

class CmdSketcherConstrainEqual: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainEqual();
    ~CmdSketcherConstrainEqual() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainEqual";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainEqual::CmdSketcherConstrainEqual()
    : CmdSketcherConstraint("Sketcher_ConstrainEqual")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain equal");
    sToolTipText =
        QT_TR_NOOP("Create an equality constraint between two lines or between circles and arcs");
    sWhatsThis = "Sketcher_ConstrainEqual";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_EqualLength";
    sAccel = "E";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge, SelEdge},
                           {SelEdge, SelExternalEdge},
                           {SelExternalEdge, SelEdge}};// Only option for equal constraint
}

void CmdSketcherConstrainEqual::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select two edges from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // go through the selected subelements

    if (SubNames.size() < 2) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select at least two lines from the sketch."));
        return;
    }

    std::vector<int> ids;
    bool lineSel = false, arcSel = false, circSel = false, ellipsSel = false, arcEllipsSel = false,
         hasAlreadyExternal = false;
    bool hyperbSel = false, parabSel = false, weightSel = false;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {

        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(*it, Obj, GeoId, PosId);

        if (!isEdge(GeoId, PosId)) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select two or more compatible edges."));
            return;
        }
        else if (GeoId == Sketcher::GeoEnum::HAxis || GeoId == Sketcher::GeoEnum::VAxis) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Sketch axes cannot be used in equality constraints."));
            return;
        }
        else if (isPointOrSegmentFixed(Obj, GeoId)) {

            if (hasAlreadyExternal) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }
            else {
                hasAlreadyExternal = true;
            }
        }

        const Part::Geometry* geo = Obj->getGeometry(GeoId);

        if (isBSplineCurve(*geo)) {
            // unsupported as they are generally hereogeneus shapes
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Equality for B-spline edge currently unsupported."));
            return;
        }

        if (isLineSegment(*geo)) {
            lineSel = true;
        }
        else if (isArcOfCircle(*geo)) {
            arcSel = true;
        }
        else if (isCircle(*geo)) {
            if (isBsplinePole(geo)) {
                weightSel = true;
            }
            else {
                circSel = true;
            }
        }
        else if (isEllipse(*geo)) {
            ellipsSel = true;
        }
        else if (isArcOfEllipse(*geo)) {
            arcEllipsSel = true;
        }
        else if (isArcOfHyperbola(*geo)) {
            hyperbSel = true;
        }
        else if (isArcOfParabola(*geo)) {
            parabSel = true;
        }
        else {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select two or more edges of similar type."));
            return;
        }

        ids.push_back(GeoId);
    }

    // Check for heterogeneous groups in selection
    if ((lineSel
         && ((arcSel || circSel) || (ellipsSel || arcEllipsSel) || hyperbSel || parabSel || weightSel))
        || ((arcSel || circSel) && ((ellipsSel || arcEllipsSel) || hyperbSel || parabSel || weightSel))
        || ((ellipsSel || arcEllipsSel) && (hyperbSel || parabSel || weightSel))
        || (hyperbSel && (parabSel || weightSel)) || (parabSel && weightSel)) {

        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select two or more edges of similar type."));
        return;
    }

    // undo command open
    openCommand(QT_TRANSLATE_NOOP("Command", "Add equality constraint"));
    for (int i = 0; i < int(ids.size() - 1); i++) {
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "addConstraint(Sketcher.Constraint('Equal',%d,%d))",
                              ids[i],
                              ids[i + 1]);
    }
    // finish the transaction and update
    commitCommand();
    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainEqual::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef;

    switch (seqIndex) {
        case 0:// {SelEdge, SelEdge}
        case 1:// {SelEdge, SelExternalEdge}
        case 2:// {SelExternalEdge, SelEdge}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;

            // check if the edge already has a Block constraint
            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }

            const Part::Geometry* geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry* geo2 = Obj->getGeometry(GeoId2);

            if ((isLineSegment(*geo1) && ! isLineSegment(*geo2))
                || (isArcOfHyperbola(*geo1) && ! isArcOfHyperbola(*geo2))
                || (isArcOfParabola(*geo1) && ! isArcOfParabola(*geo2))
                || (isBsplinePole(geo1) && !isBsplinePole(geo2))
                || ((isCircle(*geo1) || isArcOfCircle(*geo1)) && !(isCircle(*geo2) || isArcOfCircle(*geo2)))
                || ((isEllipse(*geo1) || isArcOfEllipse(*geo1)) && !(isEllipse(*geo2) || isArcOfEllipse(*geo2)))) {

                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select two or more edges of similar type."));
                return;
            }

            // undo command open
            openCommand(QT_TRANSLATE_NOOP("Command", "Add equality constraint"));
            Gui::cmdAppObjectArgs(Obj,
                                  "addConstraint(Sketcher.Constraint('Equal',%d,%d))",
                                  GeoId1,
                                  GeoId2);
            // finish the transaction and update
            commitCommand();
            tryAutoRecompute(Obj);

            return;
        }
        default:
            break;
    }
}

// ======================================================================================

class CmdSketcherConstrainSymmetric: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainSymmetric();
    ~CmdSketcherConstrainSymmetric() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainSymmetric";
    }

protected:
    void activated(int iMsg) override;
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainSymmetric::CmdSketcherConstrainSymmetric()
    : CmdSketcherConstraint("Sketcher_ConstrainSymmetric")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain symmetrical");
    sToolTipText = QT_TR_NOOP("Create a symmetry constraint "
                              "between two points\n"
                              "with respect to a line or a third point");
    sWhatsThis = "Sketcher_ConstrainSymmetric";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Symmetric";
    sAccel = "S";
    eType = ForEdit;

    allowedSelSequences = {{SelEdge, SelVertexOrRoot},
                           {SelExternalEdge, SelVertex},
                           {SelVertex, SelEdge, SelVertexOrRoot},
                           {SelRoot, SelEdge, SelVertex},
                           {SelVertex, SelExternalEdge, SelVertexOrRoot},
                           {SelRoot, SelExternalEdge, SelVertex},
                           {SelVertex, SelEdgeOrAxis, SelVertex},
                           {SelVertex, SelVertexOrRoot, SelEdge},
                           {SelRoot, SelVertex, SelEdge},
                           {SelVertex, SelVertexOrRoot, SelExternalEdge},
                           {SelRoot, SelVertex, SelExternalEdge},
                           {SelVertex, SelVertex, SelEdgeOrAxis},
                           {SelVertex, SelVertexOrRoot, SelVertex},
                           {SelVertex, SelVertex, SelVertexOrRoot},
                           {SelVertexOrRoot, SelVertex, SelVertex}};
}

void CmdSketcherConstrainSymmetric::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerGenConstraint(this));
            getSelection().clearSelection();
        }
        else {
            Gui::TranslatedUserWarning(
                getActiveGuiDocument()->getDocument(),
                QObject::tr("Wrong selection"),
                QObject::tr("Select two points and a symmetry line, "
                            "two points and a symmetry point "
                            "or a line and a symmetry point from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() != 3 && SubNames.size() != 2) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select two points and a symmetry line, "
                                               "two points and a symmetry point "
                                               "or a line and a symmetry point from the sketch."));
        return;
    }

    int GeoId1, GeoId2, GeoId3;
    Sketcher::PointPos PosId1, PosId2, PosId3;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (SubNames.size() == 2) {
        if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
            showNoConstraintBetweenFixedGeometry(Obj);
            return;
        }
        if (isVertex(GeoId1, PosId1) && isEdge(GeoId2, PosId2)) {
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
        }
        if (isEdge(GeoId1, PosId1) && isVertex(GeoId2, PosId2)) {
            const Part::Geometry* geom = Obj->getGeometry(GeoId1);

            if (isLineSegment(*geom)) {
                if (GeoId1 == GeoId2) {
                    Gui::TranslatedUserWarning(Obj,
                                               QObject::tr("Wrong selection"),
                                               QObject::tr("Cannot add a symmetry constraint "
                                                           "between a line and its end points."));
                    return;
                }

                // undo command open
                openCommand(QT_TRANSLATE_NOOP("Command", "Add symmetric constraint"));
                Gui::cmdAppObjectArgs(
                    selection[0].getObject(),
                    "addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d,%d))",
                    GeoId1,
                    static_cast<int>(Sketcher::PointPos::start),
                    GeoId1,
                    static_cast<int>(Sketcher::PointPos::end),
                    GeoId2,
                    static_cast<int>(PosId2));

                // finish the transaction and update
                commitCommand();
                tryAutoRecompute(Obj);

                // clear the selection (convenience)
                getSelection().clearSelection();
                return;
            }
        }

        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select two points and a symmetry line, "
                                               "two points and a symmetry point "
                                               "or a line and a symmetry point from the sketch."));
        return;
    }

    getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);

    if (isEdge(GeoId1, PosId1) && isVertex(GeoId3, PosId3)) {
        std::swap(GeoId1, GeoId3);
        std::swap(PosId1, PosId3);
    }
    else if (isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {
        std::swap(GeoId2, GeoId3);
        std::swap(PosId2, PosId3);
    }

    if (areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3)) {
        showNoConstraintBetweenFixedGeometry(Obj);
        return;
    }

    if (isVertex(GeoId1, PosId1) && isVertex(GeoId2, PosId2)) {
        if (isEdge(GeoId3, PosId3)) {
            const Part::Geometry* geom = Obj->getGeometry(GeoId3);

            if (isLineSegment(*geom)) {
                if (GeoId1 == GeoId2 && GeoId2 == GeoId3) {
                    Gui::TranslatedUserWarning(Obj,
                                               QObject::tr("Wrong selection"),
                                               QObject::tr("Cannot add a symmetry constraint "
                                                           "between a line and its end points!"));
                    return;
                }

                // undo command open
                openCommand(QT_TRANSLATE_NOOP("Command", "Add symmetric constraint"));
                Gui::cmdAppObjectArgs(
                    selection[0].getObject(),
                    "addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d))",
                    GeoId1,
                    static_cast<int>(PosId1),
                    GeoId2,
                    static_cast<int>(PosId2),
                    GeoId3);

                // finish the transaction and update
                commitCommand();
                tryAutoRecompute(Obj);

                // clear the selection (convenience)
                getSelection().clearSelection();
                return;
            }
        }
        else if (isVertex(GeoId3, PosId3)) {
            // undo command open
            openCommand(QT_TRANSLATE_NOOP("Command", "Add symmetric constraint"));
            Gui::cmdAppObjectArgs(
                selection[0].getObject(),
                "addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d,%d))",
                GeoId1,
                static_cast<int>(PosId1),
                GeoId2,
                static_cast<int>(PosId2),
                GeoId3,
                static_cast<int>(PosId3));

            // finish the transaction and update
            commitCommand();
            tryAutoRecompute(Obj);

            // clear the selection (convenience)
            getSelection().clearSelection();
            return;
        }
    }

    Gui::TranslatedUserWarning(Obj,
                               QObject::tr("Wrong selection"),
                               QObject::tr("Select two points and a symmetry line, "
                                           "two points and a symmetry point "
                                           "or a line and a symmetry point from the sketch."));
}

void CmdSketcherConstrainSymmetric::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = GeoEnum::GeoUndef, GeoId2 = GeoEnum::GeoUndef, GeoId3 = GeoEnum::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::PointPos::none, PosId2 = Sketcher::PointPos::none,
                       PosId3 = Sketcher::PointPos::none;

    switch (seqIndex) {
        case 0:// {SelEdge, SelVertexOrRoot}
        case 1:// {SelExternalEdge, SelVertex}
        {
            GeoId1 = GeoId2 = selSeq.at(0).GeoId;
            GeoId3 = selSeq.at(1).GeoId;
            PosId1 = Sketcher::PointPos::start;
            PosId2 = Sketcher::PointPos::end;
            PosId3 = selSeq.at(1).PosId;

            if (GeoId1 == GeoId3) {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr(
                        "Cannot add a symmetry constraint between a line and its end points!"));
                return;
            }

            if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }
            break;
        }
        case 2: // {SelVertex, SelEdge, SelVertexOrRoot}
        case 3: // {SelRoot, SelEdge, SelVertex}
        case 4: // {SelVertex, SelExternalEdge, SelVertexOrRoot}
        case 5: // {SelRoot, SelExternalEdge, SelVertex}
        case 6: // {SelVertex, SelEdgeOrAxis, SelVertex}
        case 7: // {SelVertex, SelVertexOrRoot,SelEdge}
        case 8: // {SelRoot, SelVertex, SelEdge}
        case 9: // {SelVertex, SelVertexOrRoot, SelExternalEdge}
        case 10:// {SelRoot, SelVertex, SelExternalEdge}
        case 11:// {SelVertex, SelVertex, SelEdgeOrAxis}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(2).GeoId;
            GeoId3 = selSeq.at(1).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(2).PosId;
            PosId3 = selSeq.at(1).PosId;

            if (isEdge(GeoId1, PosId1) && isVertex(GeoId3, PosId3)) {
                std::swap(GeoId1, GeoId3);
                std::swap(PosId1, PosId3);
            }
            else if (isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {
                std::swap(GeoId2, GeoId3);
                std::swap(PosId2, PosId3);
            }

            if (areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }

            const Part::Geometry* geom = Obj->getGeometry(GeoId3);

            if (isLineSegment(*geom)) {
                if (GeoId1 == GeoId2 && GeoId2 == GeoId3) {
                    Gui::TranslatedUserWarning(Obj,
                                               QObject::tr("Wrong selection"),
                                               QObject::tr("Cannot add a symmetry constraint "
                                                           "between a line and its end points."));
                    return;
                }

                // undo command open
                openCommand(QT_TRANSLATE_NOOP("Command", "Add symmetric constraint"));
                Gui::cmdAppObjectArgs(
                    Obj,
                    "addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d))",
                    GeoId1,
                    static_cast<int>(PosId1),
                    GeoId2,
                    static_cast<int>(PosId2),
                    GeoId3);

                // finish the transaction and update
                commitCommand();
                tryAutoRecompute(Obj);
            }
            else {
                Gui::TranslatedUserWarning(
                    Obj,
                    QObject::tr("Wrong selection"),
                    QObject::tr("Select two points and a symmetry line, "
                                "two points and a symmetry point "
                                "or a line and a symmetry point from the sketch."));
            }
            return;
        }
        case 12:// {SelVertex, SelVertexOrRoot, SelVertex}
        case 13:// {SelVertex, SelVertex, SelVertexOrRoot}
        case 14:// {SelVertexOrRoot, SelVertex, SelVertex}
        {
            GeoId1 = selSeq.at(0).GeoId;
            GeoId2 = selSeq.at(1).GeoId;
            GeoId3 = selSeq.at(2).GeoId;
            PosId1 = selSeq.at(0).PosId;
            PosId2 = selSeq.at(1).PosId;
            PosId3 = selSeq.at(2).PosId;

            if (areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3)) {
                showNoConstraintBetweenFixedGeometry(Obj);
                return;
            }
            break;
        }
        default:
            break;
    }

    // undo command open
    openCommand(QT_TRANSLATE_NOOP("Command", "Add symmetric constraint"));
    Gui::cmdAppObjectArgs(Obj,
                          "addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d,%d))",
                          GeoId1,
                          static_cast<int>(PosId1),
                          GeoId2,
                          static_cast<int>(PosId2),
                          GeoId3,
                          static_cast<int>(PosId3));

    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
    return;
}

// ======================================================================================

DEF_STD_CMD_A(CmdSketcherConstrainSnellsLaw)

CmdSketcherConstrainSnellsLaw::CmdSketcherConstrainSnellsLaw()
    : Command("Sketcher_ConstrainSnellsLaw")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Constrain refraction (Snell's law)");
    sToolTipText = QT_TR_NOOP("Create a refraction law (Snell's law)"
                              "constraint between two endpoints of rays\n"
                              "and an edge as an interface.");
    sWhatsThis = "Sketcher_ConstrainSnellsLaw";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_SnellsLaw";
    sAccel = "K, W";
    eType = ForEdit;
}

void CmdSketcherConstrainSnellsLaw::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1
        || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        QString strHelp = QObject::tr("Select two endpoints of lines to act as rays, "
                                      "and an edge representing a boundary. "
                                      "The first selected point corresponds "
                                      "to index n1, second to n2, "
                                      "and datum value sets the ratio n2/n1.",
                                      "Constraint_SnellsLaw");

        const char dmbg[] = "Constraint_SnellsLaw";

        QString strError = QObject::tr("Selected objects are not just geometry "
                                       "from one sketch.",
                                       dmbg);

        strError.append(strHelp);
        Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   std::move(strError));
    }

    // get the needed lists and objects
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector<std::string>& SubNames = selection[0].getSubNames();

    if (SubNames.size() != 3) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Number of selected objects is not 3"));
        return;
    }

    int GeoId1, GeoId2, GeoId3;
    Sketcher::PointPos PosId1, PosId2, PosId3;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);
    getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);

    // sink the edge to be the last item
    if (isEdge(GeoId1, PosId1)) {
        std::swap(GeoId1, GeoId2);
        std::swap(PosId1, PosId2);
    }
    if (isEdge(GeoId2, PosId2)) {
        std::swap(GeoId2, GeoId3);
        std::swap(PosId2, PosId3);
    }

    // a bunch of validity checks
    if (areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3)) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Cannot create constraint with external geometry only."));
        return;
    }

    if (!(isVertex(GeoId1, PosId1) && !isSimpleVertex(Obj, GeoId1, PosId1)
          && isVertex(GeoId2, PosId2) && !isSimpleVertex(Obj, GeoId2, PosId2)
          && isEdge(GeoId3, PosId3))) {

        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Incompatible geometry is selected."));
        return;
    };

    const Part::Geometry* geo = Obj->getGeometry(GeoId3);

    if (geo && isBSplineCurve(*geo)) {
        // unsupported until normal to B-spline at any point implemented.
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("SnellsLaw on B-spline edge is currently unsupported."));
        return;
    }

    if (isBsplinePole(geo)) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select an edge that is not a B-spline weight."));
        return;
    }

    double n2divn1 = 0;

    // the essence.
    // Unlike other constraints, we'll ask for a value immediately.
    QDialog dlg(Gui::getMainWindow());
    Ui::InsertDatum ui_Datum;
    ui_Datum.setupUi(&dlg);
    dlg.setWindowTitle(EditDatumDialog::tr("Refractive index ratio"));
    ui_Datum.label->setText(EditDatumDialog::tr("Ratio n2/n1:"));
    Base::Quantity init_val;
    init_val.setUnit(Base::Unit());
    init_val.setValue(0.0);

    ui_Datum.labelEdit->setValue(init_val);
    ui_Datum.labelEdit->setParamGrpPath(
        QByteArray("User parameter:BaseApp/History/SketcherRefrIndexRatio"));
    ui_Datum.labelEdit->setEntryName(QByteArray("DatumValue"));
    ui_Datum.labelEdit->setToLastUsedValue();
    ui_Datum.labelEdit->selectNumber();
    ui_Datum.labelEdit->setSingleStep(0.05);
    // Unable to bind, because the constraint does not yet exist

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    ui_Datum.labelEdit->pushToHistory();

    Base::Quantity newQuant = ui_Datum.labelEdit->value();
    n2divn1 = newQuant.getValue();

    // add constraint
    openCommand(QT_TRANSLATE_NOOP("Command", "Add Snell's law constraint"));

    bool safe = addConstraintSafely(Obj, [&]() {
        if (!IsPointAlreadyOnCurve(GeoId2, GeoId1, PosId1, Obj)) {
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId2,
                                  static_cast<int>(PosId2));
        }

        if (!IsPointAlreadyOnCurve(GeoId3, GeoId1, PosId1, Obj)) {
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d))",
                                  GeoId1,
                                  static_cast<int>(PosId1),
                                  GeoId3);
        }

        Gui::cmdAppObjectArgs(
            selection[0].getObject(),
            "addConstraint(Sketcher.Constraint('SnellsLaw',%d,%d,%d,%d,%d,%.12f))",
            GeoId1,
            static_cast<int>(PosId1),
            GeoId2,
            static_cast<int>(PosId2),
            GeoId3,
            n2divn1);

        /*if (allexternal || constraintCreationMode==Reference) { // it is a constraint on a
        external line, make it non-driving const std::vector<Sketcher::Constraint *> &ConStr =
        Obj->Constraints.getValues();

            Gui::cmdAppObjectArgs(selection[0].getObject(),"setDriving(%i,%s)",
                ConStr.size()-1,"False");
        }*/
    });

    if (!safe) {
        return;
    }
    else {
        commitCommand();
        tryAutoRecompute(Obj);
    }

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherConstrainSnellsLaw::isActive()
{
    return isCreateConstraintActive(getActiveGuiDocument());
}

// ======================================================================================
/*** Creation Mode / Toggle to or from Reference ***/
DEF_STD_CMD_A(CmdSketcherToggleDrivingConstraint)

CmdSketcherToggleDrivingConstraint::CmdSketcherToggleDrivingConstraint()
    : Command("Sketcher_ToggleDrivingConstraint")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Toggle driving/reference constraint");
    sToolTipText = QT_TR_NOOP("Set the toolbar, "
                              "or the selected constraints,\n"
                              "into driving or reference mode");
    sWhatsThis = "Sketcher_ToggleDrivingConstraint";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_ToggleConstraint";
    sAccel = "K, X";
    eType = ForEdit;

    // list of toggle driving constraint commands
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainLock");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainDistance");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainDistanceX");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainDistanceY");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainRadius");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainDiameter");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainRadiam");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainAngle");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_CompConstrainRadDia");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_Dimension");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_CompDimensionTools");
    // rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainSnellsLaw");
}

void CmdSketcherToggleDrivingConstraint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool modeChange = true;

    std::vector<Gui::SelectionObject> selection;

    if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) > 0) {
        // Now we check whether we have a constraint selected or not.

        // get the selection
        selection = getSelection().getSelectionEx();

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() != 1
            || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
            Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select constraints from the sketch."));
            return;
        }

        Sketcher::SketchObject* Obj =
            static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        // get the needed lists and objects
        const std::vector<std::string>& SubNames = selection[0].getSubNames();
        if (SubNames.empty()) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select constraints from the sketch."));
            return;
        }

        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
             ++it) {
            // see if we have constraints, if we do it is not a mode change, but a toggle.
            if (it->size() > 10 && it->substr(0, 10) == "Constraint") {
                modeChange = false;
            }
        }
    }

    if (modeChange) {
        // Here starts the code for mode change
        Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

        if (constraintCreationMode == Driving) {
            constraintCreationMode = Reference;
        }
        else {
            constraintCreationMode = Driving;
        }

        rcCmdMgr.updateCommands("ToggleDrivingConstraint",
                                static_cast<int>(constraintCreationMode));
    }
    else// toggle the selected constraint(s)
    {
        Sketcher::SketchObject* Obj =
            static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        // get the needed lists and objects
        const std::vector<std::string>& SubNames = selection[0].getSubNames();
        if (SubNames.empty()) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select constraints from the sketch."));
            return;
        }

        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Toggle constraint to driving/reference"));

        int successful = SubNames.size();
        // go through the selected subelements
        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
             ++it) {
            // only handle constraints
            if (it->size() > 10 && it->substr(0, 10) == "Constraint") {
                int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(*it);
                try {
                    // issue the actual commands to toggle
                    Gui::cmdAppObjectArgs(selection[0].getObject(), "toggleDriving(%d)", ConstrId);
                }
                catch (const Base::Exception&) {
                    successful--;
                }
            }
        }

        if (successful > 0) {
            commitCommand();
        }
        else {
            abortCommand();
        }

        tryAutoRecompute(Obj);

        // clear the selection (convenience)
        getSelection().clearSelection();
    }
}

bool CmdSketcherToggleDrivingConstraint::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherToggleActiveConstraint)

CmdSketcherToggleActiveConstraint::CmdSketcherToggleActiveConstraint()
    : Command("Sketcher_ToggleActiveConstraint")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Activate/deactivate constraint");
    sToolTipText = QT_TR_NOOP("Activates or deactivates "
                              "the selected constraints");
    sWhatsThis = "Sketcher_ToggleActiveConstraint";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_ToggleActiveConstraint";
    sAccel = "K, Z";
    eType = ForEdit;
}

void CmdSketcherToggleActiveConstraint::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    std::vector<Gui::SelectionObject> selection;

    if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) > 0) {
        // Now we check whether we have a constraint selected or not.

        // get the selection
        selection = getSelection().getSelectionEx();

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() != 1
            || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
            Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select constraints from the sketch."));
            return;
        }

        Sketcher::SketchObject* Obj =
            static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        // get the needed lists and objects
        const std::vector<std::string>& SubNames = selection[0].getSubNames();
        if (SubNames.empty()) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select constraints from the sketch."));
            return;
        }

        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Activate/Deactivate constraint"));

        int successful = SubNames.size();

        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
             ++it) {

            if (it->size() > 10 && it->substr(0, 10) == "Constraint") {
                int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(*it);
                try {
                    // issue the actual commands to toggle
                    Gui::cmdAppObjectArgs(selection[0].getObject(), "toggleActive(%d)", ConstrId);
                }
                catch (const Base::Exception&) {
                    successful--;
                }
            }
        }

        if (successful > 0) {
            commitCommand();
        }
        else {
            abortCommand();
        }

        tryAutoRecompute(Obj);

        // clear the selection (convenience)
        getSelection().clearSelection();
    }
}

bool CmdSketcherToggleActiveConstraint::isActive()
{
    return isCreateConstraintActive(getActiveGuiDocument());
}

void CreateSketcherCommandsConstraints()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherConstrainHorizontal());
    rcCmdMgr.addCommand(new CmdSketcherConstrainVertical());
    rcCmdMgr.addCommand(new CmdSketcherConstrainLock());
    rcCmdMgr.addCommand(new CmdSketcherConstrainBlock());
    rcCmdMgr.addCommand(new CmdSketcherConstrainCoincident());
    rcCmdMgr.addCommand(new CmdSketcherDimension());
    rcCmdMgr.addCommand(new CmdSketcherConstrainParallel());
    rcCmdMgr.addCommand(new CmdSketcherConstrainPerpendicular());
    rcCmdMgr.addCommand(new CmdSketcherConstrainTangent());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistance());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistanceX());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistanceY());
    rcCmdMgr.addCommand(new CmdSketcherConstrainRadius());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDiameter());
    rcCmdMgr.addCommand(new CmdSketcherConstrainRadiam());
    rcCmdMgr.addCommand(new CmdSketcherCompConstrainRadDia());
    rcCmdMgr.addCommand(new CmdSketcherConstrainAngle());
    rcCmdMgr.addCommand(new CmdSketcherConstrainEqual());
    rcCmdMgr.addCommand(new CmdSketcherConstrainPointOnObject());
    rcCmdMgr.addCommand(new CmdSketcherConstrainSymmetric());
    rcCmdMgr.addCommand(new CmdSketcherConstrainSnellsLaw());
    rcCmdMgr.addCommand(new CmdSketcherToggleDrivingConstraint());
    rcCmdMgr.addCommand(new CmdSketcherToggleActiveConstraint());
    rcCmdMgr.addCommand(new CmdSketcherCompDimensionTools());
}
// clang-format on
