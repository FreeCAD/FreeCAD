// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <QDialog>
#include <QObject>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/Unit.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>

#include <BRep_Tool.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Sketcher3D/App/Constraint3D.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DlgEditConstraintValue.h"
#include "DrawSketchHandlerLine3D.h"
#include "DrawSketchHandlerPoint3D.h"
#include "DrawSketchHandlerPolyline3D.h"
#include "Utils.h"
#include "ViewProviderSketch3D.h"


namespace Sketcher3DGui
{

DEF_STD_CMD_A(CmdSketcher3DCreateSketch)

CmdSketcher3DCreateSketch::CmdSketcher3DCreateSketch()
    : Command("Sketcher3D_CreateSketch")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Create 3D sketch");
    sToolTipText = QT_TR_NOOP("Creates a new 3D sketch");
    sWhatsThis = "Sketcher3D_CreateSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_NewSketch";
}

void CmdSketcher3DCreateSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    std::string FeatName = getUniqueObjectName("Sketch3D");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create a new 3D sketch"));
    doCommand(
        Doc,
        "App.activeDocument().addObject('Sketcher3D::Sketch3DObject', '%s')",
        FeatName.c_str()
    );
    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
    commitCommand();
}

bool CmdSketcher3DCreateSketch::isActive()
{
    return hasActiveDocument();
}

// Editmode tool

DEF_STD_CMD_A(CmdSketcher3DCreatePoint)

CmdSketcher3DCreatePoint::CmdSketcher3DCreatePoint()
    : Command("Sketcher3D_CreatePoint")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Point");
    sToolTipText = QT_TR_NOOP("Create a 3D point");
    sWhatsThis = "Sketcher3D_CreatePoint";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePoint";
    eType = ForEdit;
}

void CmdSketcher3DCreatePoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    vp->activateHandler(std::make_unique<DrawSketchHandlerPoint3D>());
}

bool CmdSketcher3DCreatePoint::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DCreateLine)

CmdSketcher3DCreateLine::CmdSketcher3DCreateLine()
    : Command("Sketcher3D_CreateLine")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Line");
    sToolTipText = QT_TR_NOOP("Create a 3D line segment");
    sWhatsThis = "Sketcher3D_CreateLine";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateLine";
    eType = ForEdit;
}

void CmdSketcher3DCreateLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    vp->activateHandler(std::make_unique<DrawSketchHandlerLine3D>());
}

bool CmdSketcher3DCreateLine::isActive()
{
    return isSketch3DInEdit();
}


DEF_STD_CMD_A(CmdSketcher3DCreatePolyline)

CmdSketcher3DCreatePolyline::CmdSketcher3DCreatePolyline()
    : Command("Sketcher3D_CreatePolyline")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Polyline");
    sToolTipText = QT_TR_NOOP("Create a 3D polyline");
    sWhatsThis = "Sketcher3D_CreatePolyline";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePolyline";
    eType = ForEdit;
}

void CmdSketcher3DCreatePolyline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    vp->activateHandler(std::make_unique<DrawSketchHandlerPolyline3D>());
}

bool CmdSketcher3DCreatePolyline::isActive()
{
    return isSketch3DInEdit();
}

// ---------------------------------------------------------------------------
// Selection helpers
// ---------------------------------------------------------------------------

struct Sketch3DCollectedSelection
{
    std::vector<Sketcher3D::GeoElementId3D> points;
    std::vector<Sketcher3D::GeoElementId3D> lines;
};

Sketch3DCollectedSelection collectSketch3DSelection(
    Sketcher3D::Sketch3DObject* sketch,
    bool wantPoints,
    bool wantLines
)
{
    Sketch3DCollectedSelection result;
    if (!sketch || (!wantPoints && !wantLines)) {
        return result;
    }

    auto& geos = sketch->Geometry.getValues();
    auto sels = Gui::Selection().getSelectionEx(nullptr, Sketcher3D::Sketch3DObject::getClassTypeId());
    auto shape = sketch->Shape.getShape();
    for (auto& s : sels) {
        if (s.getObject() != sketch) {
            continue;
        }
        for (auto& subname : s.getSubNames()) {
            TopoDS_Shape sub;
            try {
                sub = shape.getSubShape(subname.c_str(), /*silent=*/true);
            }
            catch (const Standard_Failure&) {
                continue;
            }
            if (sub.IsNull()) {
                continue;
            }
            auto id = sketch->resolveSubName(subname);
            if (!id.isValid()) {
                continue;
            }

            if (wantPoints && sub.ShapeType() == TopAbs_VERTEX) {
                result.points.push_back(id);
            }
            else if (wantLines && sub.ShapeType() == TopAbs_EDGE) {
                if (id.GeoId >= 0 && id.GeoId < static_cast<int>(geos.size())
                    && dynamic_cast<const Part::GeomLineSegment*>(geos[id.GeoId])) {
                    result.lines.push_back(id);
                }
            }
        }
    }

    return result;
}

std::vector<Sketcher3D::GeoElementId3D> collectSelectedPointRefs(Sketcher3D::Sketch3DObject* sketch)
{
    return collectSketch3DSelection(sketch, true, false).points;
}

std::vector<Sketcher3D::GeoElementId3D> collectSelectedLineRefs(Sketcher3D::Sketch3DObject* sketch)
{
    return collectSketch3DSelection(sketch, false, true).lines;
}

bool collectSelectedPointAndLineRefs(
    Sketcher3D::Sketch3DObject* sketch,
    Sketcher3D::GeoElementId3D& pointRef,
    Sketcher3D::GeoElementId3D& lineRef
)
{
    auto sel = collectSketch3DSelection(sketch, true, true);
    if (sel.points.size() != 1 || sel.lines.size() != 1) {
        return false;
    }
    pointRef = sel.points[0];
    lineRef = sel.lines[0];
    return true;
}

// ---------------------------------------------------------------------------
// Constraints
// ---------------------------------------------------------------------------

DEF_STD_CMD_A(CmdSketcher3DConstrainDistance)

CmdSketcher3DConstrainDistance::CmdSketcher3DConstrainDistance()
    : Command("Sketcher3D_ConstrainDistance")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain distance");
    sToolTipText = QT_TR_NOOP("Force a 3D line length, or the distance between two 3D points");
    sWhatsThis = "Sketcher3D_ConstrainDistance";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Length";
    eType = ForEdit;
}

void CmdSketcher3DConstrainDistance::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    std::vector<Sketcher3D::GeoElementId3D> refs;
    std::vector<Base::Vector3d> positions;
    const auto sels
        = Gui::Selection().getSelectionEx(nullptr, Sketcher3D::Sketch3DObject::getClassTypeId());
    const Part::TopoShape& shape = sketch->Shape.getShape();
    const auto& geos = sketch->Geometry.getValues();
    for (const auto& s : sels) {
        if (s.getObject() != sketch) {
            continue;
        }
        for (const std::string& subname : s.getSubNames()) {
            TopoDS_Shape sub;
            try {
                sub = shape.getSubShape(subname.c_str(), /*silent=*/true);
            }
            catch (const Standard_Failure&) {
                continue;
            }
            if (sub.IsNull()) {
                continue;
            }

            auto id = sketch->resolveSubName(subname);
            if (!id.isValid()) {
                continue;
            }

            if (sub.ShapeType() == TopAbs_VERTEX) {
                const TopoDS_Vertex v = TopoDS::Vertex(sub);
                const gp_Pnt p = BRep_Tool::Pnt(v);
                refs.push_back(id);
                positions.emplace_back(p.X(), p.Y(), p.Z());
            }
            else if (
                sub.ShapeType() == TopAbs_EDGE && id.Kind == Sketcher3D::GeoKind::Line
                && id.Pos == Sketcher3D::PointPos::none && id.GeoId >= 0
                && id.GeoId < static_cast<int>(geos.size())
            ) {
                if (const auto* line = dynamic_cast<const Part::GeomLineSegment*>(geos[id.GeoId])) {
                    refs.emplace_back(id.GeoId, Sketcher3D::PointPos::start, Sketcher3D::GeoKind::Line);
                    positions.push_back(line->getStartPoint());
                    refs.emplace_back(id.GeoId, Sketcher3D::PointPos::end, Sketcher3D::GeoKind::Line);
                    positions.push_back(line->getEndPoint());
                }
            }
        }
    }

    if (refs.size() != 2) {
        Base::Console().warning(
            "Sketcher3D: select one 3D sketch line or exactly two 3D sketch points for Distance.\n"
        );
        return;
    }
    if (refs[0] == refs[1]) {
        Base::Console().warning("Sketcher3D: Distance needs two distinct points.\n");
        return;
    }

    const double seedValue = (positions[1] - positions[0]).Length();

    DlgEditConstraintValue dlg(
        QObject::tr("Constrain distance"),
        QObject::tr("Distance:"),
        seedValue,
        Base::Unit::Length,
        Gui::getMainWindow()
    );
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const double finalValue = dlg.value();

    openCommand(QT_TRANSLATE_NOOP("Command", "Constrain distance"));
    Sketcher3D::Constraint3D c;
    c.Type = Sketcher3D::Constraint3D::Distance3D;
    c.Value = finalValue;
    c.setElements({refs[0], refs[1]});
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    commitCommand();

    Gui::Selection().clearSelection();
}

bool CmdSketcher3DConstrainDistance::isActive()
{
    return isSketch3DInEdit();
}

// helper for axisdistanceConstraint
void addAxisDistanceConstraint(
    Gui::Command* command,
    Sketcher3D::Constraint3D::Constraint3DType type,
    char axis,
    const QString& dialogTitle,
    const char* commandText
)
{
    if (!command) {
        return;
    }
    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    const auto refs = collectSelectedPointRefs(sketch);

    if (refs.empty() || refs.size() > 2) {
        Base::Console().warning("Sketcher3D: select one or two 3D sketch points for Distance%c.\n", axis);
        return;
    }
    if (refs.size() == 2 && refs[0] == refs[1]) {
        Base::Console().warning("Sketcher3D: Distance%c needs two distinct points.\n", axis);
        return;
    }

    std::vector<Base::Vector3d> positions;
    positions.reserve(refs.size());
    for (const auto& ref : refs) {
        Base::Vector3d point;
        if (!sketch->getPointAt(ref, point)) {
            return;
        }
        positions.push_back(point);
    }

    // One point: seed from the absolute coordinate (distance to the corresponding global plane).
    // Two points: seed from the signed component-wise delta.
    const Base::Vector3d ref = refs.size() == 2 ? (positions[1] - positions[0]) : positions[0];
    double seedValue = 0.0;
    switch (axis) {
        case 'X':
            seedValue = ref.x;
            break;
        case 'Y':
            seedValue = ref.y;
            break;
        case 'Z':
            seedValue = ref.z;
            break;
    }

    DlgEditConstraintValue dlg(
        dialogTitle,
        QObject::tr("Distance:"),
        seedValue,
        Base::Unit::Length,
        Gui::getMainWindow(),
        true
    );
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const double finalValue = dlg.value();

    command->openCommand(commandText);
    Sketcher3D::Constraint3D c;
    c.Type = type;
    c.Value = finalValue;
    if (refs.size() == 2) {
        c.setElements({refs[0], refs[1]});
    }
    else {
        c.setElements({refs[0]});
    }
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    command->commitCommand();

    Gui::Selection().clearSelection();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainDistanceX)

CmdSketcher3DConstrainDistanceX::CmdSketcher3DConstrainDistanceX()
    : Command("Sketcher3D_ConstrainDistanceX")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain distance X");
    sToolTipText = QT_TR_NOOP(
        "Force the X-axis distance: pick one point to lock its X coordinate, or two "
        "points to fix the signed X delta between them"
    );
    sWhatsThis = "Sketcher3D_ConstrainDistanceX";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_HorizontalDistance";
    eType = ForEdit;
}

void CmdSketcher3DConstrainDistanceX::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    addAxisDistanceConstraint(
        this,
        Sketcher3D::Constraint3D::DistanceX3D,
        'X',
        QObject::tr("Constrain distance X"),
        QT_TRANSLATE_NOOP("Command", "Constrain distance X")
    );
}

bool CmdSketcher3DConstrainDistanceX::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainDistanceY)

CmdSketcher3DConstrainDistanceY::CmdSketcher3DConstrainDistanceY()
    : Command("Sketcher3D_ConstrainDistanceY")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain distance Y");
    sToolTipText = QT_TR_NOOP(
        "Force the Y-axis distance: pick one point to lock its Y coordinate, or two "
        "points to fix the signed Y delta between them"
    );
    sWhatsThis = "Sketcher3D_ConstrainDistanceY";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_VerticalDistance";
    eType = ForEdit;
}

void CmdSketcher3DConstrainDistanceY::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    addAxisDistanceConstraint(
        this,
        Sketcher3D::Constraint3D::DistanceY3D,
        'Y',
        QObject::tr("Constrain distance Y"),
        QT_TRANSLATE_NOOP("Command", "Constrain distance Y")
    );
}

bool CmdSketcher3DConstrainDistanceY::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainDistanceZ)

CmdSketcher3DConstrainDistanceZ::CmdSketcher3DConstrainDistanceZ()
    : Command("Sketcher3D_ConstrainDistanceZ")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain distance Z");
    sToolTipText = QT_TR_NOOP(
        "Force the Z-axis distance: pick one point to lock its Z coordinate, or two "
        "points to fix the signed Z delta between them"
    );
    sWhatsThis = "Sketcher3D_ConstrainDistanceZ";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Length";
    eType = ForEdit;
}

void CmdSketcher3DConstrainDistanceZ::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    addAxisDistanceConstraint(
        this,
        Sketcher3D::Constraint3D::DistanceZ3D,
        'Z',
        QObject::tr("Constrain distance Z"),
        QT_TRANSLATE_NOOP("Command", "Constrain distance Z")
    );
}

bool CmdSketcher3DConstrainDistanceZ::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainCoincident)

CmdSketcher3DConstrainCoincident::CmdSketcher3DConstrainCoincident()
    : Command("Sketcher3D_ConstrainCoincident")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain coincident");
    sToolTipText = QT_TR_NOOP("Force two 3D points to share the same location");
    sWhatsThis = "Sketcher3D_ConstrainCoincident";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_PointOnPoint";
    eType = ForEdit;
}

void CmdSketcher3DConstrainCoincident::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    const auto refs = collectSelectedPointRefs(sketch);

    if (refs.size() != 2) {
        Base::Console().warning("Sketcher3D: select exactly two 3D sketch points for Coincident.\n");
        return;
    }
    if (refs[0] == refs[1]) {
        Base::Console().warning("Sketcher3D: Coincident needs two distinct points.\n");
        return;
    }
    // Refuse the constraint if the two endpoints are already coincident
    if (sketch->arePointsCoincident3D(refs[0], refs[1])) {
        Base::Console().warning("Sketcher3D: those points are already coincident.\n");
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Constrain coincident"));
    Sketcher3D::Constraint3D c;
    c.Type = Sketcher3D::Constraint3D::Coincident3D;
    c.setElements({refs[0], refs[1]});
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    commitCommand();

    Gui::Selection().clearSelection();
}

bool CmdSketcher3DConstrainCoincident::isActive()
{
    return isSketch3DInEdit();
}

// Finds the closest pair of endpoints between the two lines, sets
// which endpoint is tail and returns the angle between the two outward pointing
// direction vectors in [0, pi]. and store tail endpoint info in ref1.Pos and ref2.Pos.
bool calculateAngle3D(
    const Sketcher3D::Sketch3DObject* sketch,
    Sketcher3D::GeoElementId3D& ref1,
    Sketcher3D::GeoElementId3D& ref2,
    double& actAngle
)
{
    if (!sketch) {
        return false;
    }

    const auto& geos = sketch->Geometry.getValues();
    if (ref1.GeoId < 0 || ref1.GeoId >= static_cast<int>(geos.size()) || ref2.GeoId < 0
        || ref2.GeoId >= static_cast<int>(geos.size())) {
        return false;
    }
    const auto* line1 = dynamic_cast<const Part::GeomLineSegment*>(geos[ref1.GeoId]);
    const auto* line2 = dynamic_cast<const Part::GeomLineSegment*>(geos[ref2.GeoId]);
    if (!line1 || !line2) {
        return false;
    }

    const Base::Vector3d p1[2] = {line1->getStartPoint(), line1->getEndPoint()};
    const Base::Vector3d p2[2] = {line2->getStartPoint(), line2->getEndPoint()};

    // Find the pair of endpoints (one from each line) that are closest together.
    double minDist = std::numeric_limits<double>::max();
    int tailIdx1 = 0;
    int tailIdx2 = 0;
    for (int i = 0; i <= 1; ++i) {
        for (int j = 0; j <= 1; ++j) {
            const double d = (p1[i] - p2[j]).Length();
            if (d < minDist) {
                minDist = d;
                tailIdx1 = i;
                tailIdx2 = j;
            }
        }
    }

    // Store which endpoint is the tail
    ref1.Pos = tailIdx1 ? Sketcher3D::PointPos::end : Sketcher3D::PointPos::start;
    ref2.Pos = tailIdx2 ? Sketcher3D::PointPos::end : Sketcher3D::PointPos::start;

    // Direction vectors pointing AWAY from the shared vertex.
    const Base::Vector3d dir1 = tailIdx1 ? (p1[0] - p1[1]) : (p1[1] - p1[0]);
    const Base::Vector3d dir2 = tailIdx2 ? (p2[0] - p2[1]) : (p2[1] - p2[0]);

    if (dir1.Sqr() <= 1e-24 || dir2.Sqr() <= 1e-24) {
        return false;
    }

    // Angle between the two outward vectors in [0, pi].
    const double crossMag = (dir1 % dir2).Length();
    const double dot = dir1 * dir2;
    actAngle = std::atan2(crossMag, dot);
    return true;
}

double normalizedLineAngle(double angle)
{
    const double fullTurn = 2.0 * std::numbers::pi;
    angle = std::fmod(angle, fullTurn);
    if (angle < 0.0) {
        angle += fullTurn;
    }
    // Clamp because in 3d there is no referce plane so
    // clockwise vs counterclockwise dont matter.
    if (angle > std::numbers::pi) {
        angle = fullTurn - angle;
    }
    return angle;
}

DEF_STD_CMD_A(CmdSketcher3DConstrainParallel)

CmdSketcher3DConstrainParallel::CmdSketcher3DConstrainParallel()
    : Command("Sketcher3D_ConstrainParallel")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain parallel");
    sToolTipText = QT_TR_NOOP("Force two 3D lines to be parallel");
    sWhatsThis = "Sketcher3D_ConstrainParallel";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Parallel";
    eType = ForEdit;
}

void CmdSketcher3DConstrainParallel::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    const auto refs = collectSelectedLineRefs(sketch);

    if (refs.size() != 2) {
        Base::Console().warning("Sketcher3D: select exactly two 3D sketch lines for Parallel.\n");
        return;
    }
    if (refs[0] == refs[1]) {
        Base::Console().warning("Sketcher3D: Parallel needs two distinct lines.\n");
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Constrain parallel"));
    Sketcher3D::Constraint3D c;
    c.Type = Sketcher3D::Constraint3D::Parallel3D;
    c.setElements({refs[0], refs[1]});
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    commitCommand();

    Gui::Selection().clearSelection();
}

bool CmdSketcher3DConstrainParallel::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainEqualLength)

CmdSketcher3DConstrainEqualLength::CmdSketcher3DConstrainEqualLength()
    : Command("Sketcher3D_ConstrainEqualLength")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain equal length");
    sToolTipText = QT_TR_NOOP("Constrains the selected 3D lines to have equal length");
    sWhatsThis = "Sketcher3D_ConstrainEqualLength";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_EqualLength";
    eType = ForEdit;
}

void CmdSketcher3DConstrainEqualLength::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    auto refs = collectSelectedLineRefs(sketch);

    if (refs.size() < 2) {
        Base::Console().warning("Sketcher3D: select at least two 3D sketch lines for Equal Length.\n");
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Constrain equal length"));
    for (std::size_t i = 0; i + 1 < refs.size(); ++i) {
        Sketcher3D::Constraint3D c;
        c.Type = Sketcher3D::Constraint3D::EqualLength3D;
        c.setElements({refs[i], refs[i + 1]});
        sketch->addConstraint(c);
    }
    sketch->recomputeFeature();
    commitCommand();

    Gui::Selection().clearSelection();
}

bool CmdSketcher3DConstrainEqualLength::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainPointOnLine)

CmdSketcher3DConstrainPointOnLine::CmdSketcher3DConstrainPointOnLine()
    : Command("Sketcher3D_ConstrainPointOnLine")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain point on line");
    sToolTipText = QT_TR_NOOP("Force a 3D point to lie on a 3D line");
    sWhatsThis = "Sketcher3D_ConstrainPointOnLine";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_PointOnObject";
    eType = ForEdit;
}

void CmdSketcher3DConstrainPointOnLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    Sketcher3D::GeoElementId3D pointRef;
    Sketcher3D::GeoElementId3D lineRef;
    if (!collectSelectedPointAndLineRefs(sketch, pointRef, lineRef)) {
        Base::Console().warning(
            "Sketcher3D: select exactly one 3D point and one 3D line for Point on line.\n"
        );
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Constrain point on line"));
    Sketcher3D::Constraint3D c;
    c.Type = Sketcher3D::Constraint3D::PointOnLine3D;
    c.setElements({pointRef, lineRef});
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    commitCommand();

    Gui::Selection().clearSelection();
}

bool CmdSketcher3DConstrainPointOnLine::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainMidpoint)

CmdSketcher3DConstrainMidpoint::CmdSketcher3DConstrainMidpoint()
    : Command("Sketcher3D_ConstrainMidpoint")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain midpoint");
    sToolTipText = QT_TR_NOOP("Force a 3D point to be the midpoint of a 3D line");
    sWhatsThis = "Sketcher3D_ConstrainMidpoint";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Symmetric";
    eType = ForEdit;
}

void CmdSketcher3DConstrainMidpoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    Sketcher3D::GeoElementId3D pointRef;
    Sketcher3D::GeoElementId3D lineRef;
    if (!collectSelectedPointAndLineRefs(sketch, pointRef, lineRef)) {
        Base::Console().warning(
            "Sketcher3D: select exactly one 3D point and one 3D line for Midpoint.\n"
        );
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Constrain midpoint"));
    Sketcher3D::Constraint3D c;
    c.Type = Sketcher3D::Constraint3D::Midpoint3D;
    c.setElements({pointRef, lineRef});
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    commitCommand();

    Gui::Selection().clearSelection();
}

bool CmdSketcher3DConstrainMidpoint::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainAngle)

CmdSketcher3DConstrainAngle::CmdSketcher3DConstrainAngle()
    : Command("Sketcher3D_ConstrainAngle")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain angle");
    sToolTipText = QT_TR_NOOP("Force the smaller angle between two 3D lines");
    sWhatsThis = "Sketcher3D_ConstrainAngle";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_InternalAngle";
    eType = ForEdit;
}

void CmdSketcher3DConstrainAngle::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    const auto refs = collectSelectedLineRefs(sketch);

    if (refs.size() != 2) {
        Base::Console().warning("Sketcher3D: select exactly two 3D sketch lines for Angle.\n");
        return;
    }
    if (refs[0] == refs[1]) {
        Base::Console().warning("Sketcher3D: Angle needs two distinct lines.\n");
        return;
    }

    Sketcher3D::GeoElementId3D ref1 = refs[0];
    Sketcher3D::GeoElementId3D ref2 = refs[1];
    double actAngle = 0.0;
    if (!calculateAngle3D(sketch, ref1, ref2, actAngle)) {
        Base::Console().warning("Sketcher3D: Angle needs two non-zero length lines.\n");
        return;
    }
    if (actAngle == 0.0) {
        Base::Console().warning(
            "Sketcher3D: An angle constraint cannot be set for two parallel lines.\n"
        );
        return;
    }

    DlgEditConstraintValue dlg(
        QObject::tr("Constrain angle"),
        QObject::tr("Angle:"),
        Base::toDegrees(actAngle),
        Base::Unit::Angle,
        Gui::getMainWindow()
    );
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Constrain angle"));
    Sketcher3D::Constraint3D c;
    c.Type = Sketcher3D::Constraint3D::Angle3D;
    c.Value = normalizedLineAngle(Base::toRadians(dlg.value()));
    c.setElements({ref1, ref2});
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    commitCommand();

    Gui::Selection().clearSelection();
}

bool CmdSketcher3DConstrainAngle::isActive()
{
    return isSketch3DInEdit();
}

void addAlongConstraint(
    Gui::Command* command,
    Sketcher3D::Constraint3D::Constraint3DType type,
    const char* axis,
    const char* commandText
)
{
    if (!command) {
        return;
    }
    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    const auto refs = collectSelectedLineRefs(sketch);
    if (refs.size() != 1) {
        Base::Console().warning("Sketcher3D: select exactly one 3D sketch line for Along%s.\n", axis);
        return;
    }

    command->openCommand(commandText);
    Sketcher3D::Constraint3D c;
    c.Type = type;
    c.setElements({refs[0]});
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    command->commitCommand();

    Gui::Selection().clearSelection();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainAlongX)

CmdSketcher3DConstrainAlongX::CmdSketcher3DConstrainAlongX()
    : Command("Sketcher3D_ConstrainAlongX")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain along X");
    sToolTipText = QT_TR_NOOP("Force a 3D line to be parallel to the global X axis");
    sWhatsThis = "Sketcher3D_ConstrainAlongX";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Horizontal";
    eType = ForEdit;
}

void CmdSketcher3DConstrainAlongX::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    addAlongConstraint(
        this,
        Sketcher3D::Constraint3D::AlongX,
        "X",
        QT_TRANSLATE_NOOP("Command", "Constrain along X")
    );
}

bool CmdSketcher3DConstrainAlongX::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainAlongY)

CmdSketcher3DConstrainAlongY::CmdSketcher3DConstrainAlongY()
    : Command("Sketcher3D_ConstrainAlongY")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain along Y");
    sToolTipText = QT_TR_NOOP("Force a 3D line to be parallel to the global Y axis");
    sWhatsThis = "Sketcher3D_ConstrainAlongY";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Vertical";
    eType = ForEdit;
}

void CmdSketcher3DConstrainAlongY::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    addAlongConstraint(
        this,
        Sketcher3D::Constraint3D::AlongY,
        "Y",
        QT_TRANSLATE_NOOP("Command", "Constrain along Y")
    );
}

bool CmdSketcher3DConstrainAlongY::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DConstrainAlongZ)

CmdSketcher3DConstrainAlongZ::CmdSketcher3DConstrainAlongZ()
    : Command("Sketcher3D_ConstrainAlongZ")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain along Z");
    sToolTipText = QT_TR_NOOP("Force a 3D line to be parallel to the global Z axis");
    sWhatsThis = "Sketcher3D_ConstrainAlongZ";
    sStatusTip = sToolTipText;
    sPixmap = "Std_Axis";
    eType = ForEdit;
}

void CmdSketcher3DConstrainAlongZ::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    addAlongConstraint(
        this,
        Sketcher3D::Constraint3D::AlongZ,
        "Z",
        QT_TRANSLATE_NOOP("Command", "Constrain along Z")
    );
}

bool CmdSketcher3DConstrainAlongZ::isActive()
{
    return isSketch3DInEdit();
}

class CmdSketcher3DCompDimensionTools: public Gui::GroupCommand
{
public:
    CmdSketcher3DCompDimensionTools()
        : GroupCommand("Sketcher3D_CompDimensionTools")
    {
        sAppModule = "Sketcher3D";
        sGroup = QT_TR_NOOP("Sketcher3D");
        sMenuText = QT_TR_NOOP("Dimension");
        sToolTipText = QT_TR_NOOP("Dimension tools");
        sWhatsThis = "Sketcher3D_CompDimensionTools";
        sStatusTip = sToolTipText;
        eType = ForEdit;

        setCheckable(false);
        setRememberLast(false);

        addCommand("Sketcher3D_ConstrainDistance");
        addCommand("Sketcher3D_ConstrainAngle");
        addCommand("Sketcher3D_ConstrainDistanceX");
        addCommand("Sketcher3D_ConstrainDistanceY");
        addCommand("Sketcher3D_ConstrainDistanceZ");
    }

    const char* className() const override
    {
        return "CmdSketcher3DCompDimensionTools";
    }

    bool isActive() override
    {
        return isSketch3DInEdit();
    }
};

class CmdSketcher3DCompParallel: public Gui::GroupCommand
{
public:
    CmdSketcher3DCompParallel()
        : GroupCommand("Sketcher3D_CompParallel")
    {
        sAppModule = "Sketcher3D";
        sGroup = QT_TR_NOOP("Sketcher3D");
        sMenuText = QT_TR_NOOP("Parallel axis constraint");
        sToolTipText = QT_TR_NOOP("Constrain selected 3D lines parallel or along a global axis");
        sWhatsThis = "Sketcher3D_CompParallel";
        sStatusTip = sToolTipText;
        eType = ForEdit;

        setCheckable(false);
        setRememberLast(false);

        addCommand("Sketcher3D_ConstrainParallel");
        addCommand("Sketcher3D_ConstrainAlongX");
        addCommand("Sketcher3D_ConstrainAlongY");
        addCommand("Sketcher3D_ConstrainAlongZ");
    }

    const char* className() const override
    {
        return "CmdSketcher3DCompParallel";
    }

    bool isActive() override
    {
        return isSketch3DInEdit();
    }
};

}  // namespace Sketcher3DGui


void CreateSketcher3DCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreateSketch());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreatePoint());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreateLine());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreatePolyline());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainDistance());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainAngle());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainDistanceX());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainDistanceY());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainDistanceZ());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainCoincident());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainParallel());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainAlongX());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainAlongY());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainAlongZ());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainEqualLength());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainPointOnLine());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainMidpoint());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCompDimensionTools());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCompParallel());
}
