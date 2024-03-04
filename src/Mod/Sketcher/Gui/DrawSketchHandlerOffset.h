/***************************************************************************
 *   Copyright (c) 2022 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>   *
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


#ifndef SKETCHERGUI_DrawSketchHandlerOffset_H
#define SKETCHERGUI_DrawSketchHandlerOffset_H

#include <QApplication>

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>

#include <Base/Exception.h>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include <Mod/Sketcher/App/GeometryFacade.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"


using namespace Sketcher;

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerOffset;

namespace ConstructionMethods
{

enum class OffsetConstructionMethod
{
    Arc,
    // Tangent,
    Intersection,
    End  // Must be the last one
};

/* OCC offer various modes as follows, but we use only Arc and Intersection as the rest are buggy.
enum class JoinMode {
    Arc,
    Tangent,
    Intersection
};
//We use Pipe by default only. Skin is buggy.
enum class ModeEnums {
    Skin,
    Pipe,
    RectoVerso
};*/
}  // namespace ConstructionMethods

using DSHOffsetController =
    DrawSketchDefaultWidgetController<DrawSketchHandlerOffset,
                                      StateMachines::OneSeekEnd,
                                      /*PAutoConstraintSize =*/0,
                                      /*OnViewParametersT =*/OnViewParameters<1, 1>,
                                      /*WidgetParametersT =*/WidgetParameters<0, 0>,
                                      /*WidgetCheckboxesT =*/WidgetCheckboxes<2, 2>,
                                      /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
                                      ConstructionMethods::OffsetConstructionMethod,
                                      /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHOffsetControllerBase = DSHOffsetController::ControllerBase;

using DrawSketchHandlerOffsetBase = DrawSketchControllableHandler<DSHOffsetController>;

class DrawSketchHandlerOffset: public DrawSketchHandlerOffsetBase
{
    friend DSHOffsetController;
    friend DSHOffsetControllerBase;

public:
    DrawSketchHandlerOffset(std::vector<int> listOfGeoIds,
                            ConstructionMethod constrMethod = ConstructionMethod::Arc)
        : DrawSketchHandlerOffsetBase(constrMethod)
        , listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , offsetLengthSet(false)
        , offsetConstraint(false)
        , onlySingleLines(true)
        , offsetLength(1.)
    {}

    ~DrawSketchHandlerOffset() override = default;


private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        if (state() == SelectMode::SeekFirst) {
            endpoint = onSketchPos;

            if (!offsetLengthSet) {
                findOffsetLength();
                toolWidgetManager.drawDoubleAtCursor(onSketchPos, offsetLength);
            }

            if (fabs(offsetLength) > Precision::Confusion()) {
                drawOffsetPreview();
            }
        }
    }

    void executeCommands() override
    {
        if (fabs(offsetLength) > Precision::Confusion()) {
            createOffset();
        }
    }

    std::string getToolName() const override
    {
        return "DSH_Offset";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_Offset");
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

    bool isWidgetVisible() const override
    {
        return true;
    };

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_Offset");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Offset parameters"));
    }

    void activated() override
    {
        DrawSketchDefaultHandler::activated();
        continuousMode = false;
        firstCurveCreated = getHighestCurveIndex() + 1;

        generateSourceWires();
    }

private:
    class CoincidencePointPos
    {
    public:
        PointPos firstPos1;
        PointPos secondPos1;
        PointPos firstPos2;
        PointPos secondPos2;
    };

    std::vector<int> listOfGeoIds;
    std::vector<std::vector<int>> vCC;
    std::vector<std::vector<int>> vCCO;
    Base::Vector2d endpoint, pointOnSourceWire;
    std::vector<TopoDS_Wire> sourceWires;

    bool deleteOriginal, offsetLengthSet, offsetConstraint, onlySingleLines;
    double offsetLength;
    int firstCurveCreated;

    TopoDS_Shape makeOffsetShape(bool allowOpenResult = false)
    {
        // in OCC the JointTypes are : Arc(0), Tangent(1), Intersection(2)
        short joinType =
            constructionMethod() == DrawSketchHandlerOffset::ConstructionMethod::Arc ? 0 : 2;

        // Offset will fail for single lines if we don't set a plane in ctor.
        // But if we set a plane, then the direction of offset is forced...
        // so we set a plane if and only if there are not a single sourceWires with more than single
        // line.
        BRepOffsetAPI_MakeOffset mkOffset;

        if (onlySingleLines) {
            TopoDS_Face workingPlane = BRepBuilderAPI_MakeFace(gp_Pln(gp::Origin(), gp::DZ()));
            mkOffset = BRepOffsetAPI_MakeOffset(workingPlane);
        }
        mkOffset.Init(GeomAbs_JoinType(joinType), allowOpenResult);

        for (TopoDS_Wire& wire : sourceWires) {
            mkOffset.AddWire(wire);
        }
        try {
#if defined(__GNUC__) && defined(FC_OS_LINUX)
            Base::SignalException se;
#endif
            mkOffset.Perform(offsetLength);
        }
        catch (Standard_Failure&) {
            throw;
        }
        catch (...) {
            throw Base::CADKernelError(
                "BRepOffsetAPI_MakeOffset has crashed! (Unknown exception caught)");
        }

        TopoDS_Shape offsetShape = mkOffset.Shape();

        if (offsetShape.IsNull()) {
            throw Base::CADKernelError("makeOffset2D: result of offsetting is null!");
        }

        // Copying shape to fix strange orientation behavior, OCC7.0.0. See bug #2699
        //  http://www.freecadweb.org/tracker/view.php?id=2699
        offsetShape = BRepBuilderAPI_Copy(offsetShape).Shape();
        return offsetShape;
    }

    Part::Geometry* curveToLine(BRepAdaptor_Curve curve)
    {
        double first = curve.FirstParameter();
        if (fabs(first) > 1E99) {
            first = -10000;
        }

        double last = curve.LastParameter();
        if (fabs(last) > 1E99) {
            last = +10000;
        }

        gp_Pnt P1 = curve.Value(first);
        gp_Pnt P2 = curve.Value(last);

        Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());
        Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());
        auto* line = new Part::GeomLineSegment();
        line->setPoints(p1, p2);
        GeometryFacade::setConstruction(line, false);
        return line;
    }
    Part::Geometry* curveToCircleOrArc(BRepAdaptor_Curve curve)
    {
        gp_Circ circle = curve.Circle();
        gp_Pnt cnt = circle.Location();
        gp_Pnt beg = curve.Value(curve.FirstParameter());
        gp_Pnt end = curve.Value(curve.LastParameter());

        if (beg.SquareDistance(end) < Precision::Confusion()) {
            auto* gCircle = new Part::GeomCircle();
            gCircle->setRadius(circle.Radius());
            gCircle->setCenter(Base::Vector3d(cnt.X(), cnt.Y(), cnt.Z()));

            GeometryFacade::setConstruction(gCircle, false);
            return gCircle;
        }
        else {
            auto* gArc = new Part::GeomArcOfCircle();
            Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
            Handle(Geom_TrimmedCurve) tCurve =
                new Geom_TrimmedCurve(hCircle, curve.FirstParameter(), curve.LastParameter());
            gArc->setHandle(tCurve);
            GeometryFacade::setConstruction(gArc, false);
            return gArc;
        }
    }
    Part::Geometry* curveToEllipseOrArc(BRepAdaptor_Curve curve)
    {
        gp_Elips ellipse = curve.Ellipse();
        // gp_Pnt cnt = ellipse.Location();
        gp_Pnt beg = curve.Value(curve.FirstParameter());
        gp_Pnt end = curve.Value(curve.LastParameter());

        if (beg.SquareDistance(end) < Precision::Confusion()) {
            auto* gEllipse = new Part::GeomEllipse();
            Handle(Geom_Ellipse) hEllipse = new Geom_Ellipse(ellipse);
            gEllipse->setHandle(hEllipse);
            GeometryFacade::setConstruction(gEllipse, false);
            return gEllipse;
        }
        else {
            Handle(Geom_Curve) hEllipse = new Geom_Ellipse(ellipse);
            Handle(Geom_TrimmedCurve) tCurve =
                new Geom_TrimmedCurve(hEllipse, curve.FirstParameter(), curve.LastParameter());
            auto* gArc = new Part::GeomArcOfEllipse();
            gArc->setHandle(tCurve);
            GeometryFacade::setConstruction(gArc, false);
            return gArc;
        }
    }

    void getOffsetGeos(std::vector<Part::Geometry*>& geometriesToAdd,
                       std::vector<int>& listOfOffsetGeoIds)
    {
        TopoDS_Shape offsetShape = makeOffsetShape();

        TopExp_Explorer expl(offsetShape, TopAbs_EDGE);
        int geoIdToAdd = firstCurveCreated;
        for (; expl.More(); expl.Next(), geoIdToAdd++) {

            const TopoDS_Edge& edge = TopoDS::Edge(expl.Current());
            BRepAdaptor_Curve curve(edge);
            if (curve.GetType() == GeomAbs_Line) {
                geometriesToAdd.push_back(curveToLine(curve));
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            else if (curve.GetType() == GeomAbs_Circle) {
                geometriesToAdd.push_back(curveToCircleOrArc(curve));
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            else if (curve.GetType() == GeomAbs_Ellipse) {
                geometriesToAdd.push_back(curveToEllipseOrArc(curve));
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            // TODO Bspline support
        }
    }

    void drawOffsetPreview()
    {
        std::vector<Part::Geometry*> geometriesToAdd;
        std::vector<int> listOfOffsetGeoIds;
        getOffsetGeos(geometriesToAdd, listOfOffsetGeoIds);

        drawEdit(geometriesToAdd);
    }

    void createOffset()
    {
        std::vector<Part::Geometry*> geometriesToAdd;
        std::vector<int> listOfOffsetGeoIds;
        getOffsetGeos(geometriesToAdd, listOfOffsetGeoIds);

        SketchObject* Obj = sketchgui->getSketchObject();
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Offset"));

        // Create geos
        Obj->addGeometry(std::move(geometriesToAdd));

        // Create coincident (& tangent) constraints
        jointOffsetCurves(listOfOffsetGeoIds);

        if (deleteOriginal) {
            deleteOriginalGeometries();
        }
        else if (offsetConstraint) {
            makeOffsetConstraint(listOfOffsetGeoIds);
        }

        Gui::Command::commitCommand();
    }

    void jointOffsetCurves(std::vector<int>& listOfOffsetGeoIds)
    {
        std::stringstream stream;
        stream << "conList = []\n";
        for (size_t i = 0; i < listOfOffsetGeoIds.size() - 1; i++) {
            for (size_t j = i + 1; j < listOfOffsetGeoIds.size(); j++) {
                // There's a bug with created arcs. They end points seems to swap at some point...
                // Here we make coincidences based on distance. So they must change after.
                Base::Vector3d firstStartPoint, firstEndPoint, secondStartPoint, secondEndPoint;
                if (!getFirstSecondPoints(listOfOffsetGeoIds[i], firstStartPoint, firstEndPoint)
                    || !getFirstSecondPoints(listOfOffsetGeoIds[j],
                                             secondStartPoint,
                                             secondEndPoint)) {
                    continue;
                }

                bool create = false;
                PointPos posi, posj;

                if ((firstStartPoint - secondStartPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = PointPos::start;
                    posj = PointPos::start;
                }
                else if ((firstStartPoint - secondEndPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = PointPos::start;
                    posj = PointPos::end;
                }
                else if ((firstEndPoint - secondStartPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = PointPos::end;
                    posj = PointPos::start;
                }
                else if ((firstEndPoint - secondEndPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = PointPos::end;
                    posj = PointPos::end;
                }

                if (create) {
                    bool tangent =
                        needTangent(listOfOffsetGeoIds[i], listOfOffsetGeoIds[j], posi, posj);
                    stream << "conList.append(Sketcher.Constraint('"
                           << (tangent ? "Tangent" : "Coincident");
                    stream << "'," << listOfOffsetGeoIds[i] << "," << static_cast<int>(posi) << ", "
                           << listOfOffsetGeoIds[j] << "," << static_cast<int>(posj) << "))\n";
                }
            }
        }

        stream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
        stream << "del conList\n";
        Gui::Command::doCommand(Gui::Command::Doc, stream.str().c_str());
    }

    bool needTangent(int geoId1, int geoId2, PointPos pos1, PointPos pos2)
    {
        // Todo: add cases for arcOfellipse parabolas hyperbolas bspline

        SketchObject* Obj = sketchgui->getSketchObject();
        const Part::Geometry* geo1 = Obj->getGeometry(geoId1);
        const Part::Geometry* geo2 = Obj->getGeometry(geoId2);

        if (!isArcOfCircle(*geo1) && !isArcOfCircle(*geo2)) {
            return false;
        }

        Base::Vector3d perpendicular1, perpendicular2, p1, p2;
        if (isArcOfCircle(*geo1)) {
            auto* arcOfCircle = static_cast<const Part::GeomArcOfCircle*>(geo1);
            p1 = pos1 == PointPos::start ? arcOfCircle->getStartPoint(true)
                                         : arcOfCircle->getEndPoint(true);

            perpendicular1.x = -(arcOfCircle->getCenter() - p1).y;
            perpendicular1.y = (arcOfCircle->getCenter() - p1).x;
        }
        else if (isLineSegment(*geo1)) {
            auto* line = static_cast<const Part::GeomLineSegment*>(geo1);
            perpendicular1 = line->getStartPoint() - line->getEndPoint();
        }
        else {
            return false;
        }

        if (isArcOfCircle(*geo2)) {
            auto* arcOfCircle = static_cast<const Part::GeomArcOfCircle*>(geo2);
            p2 = pos2 == PointPos::start ? arcOfCircle->getStartPoint(true)
                                         : arcOfCircle->getEndPoint(true);

            perpendicular2.x = -(arcOfCircle->getCenter() - p2).y;
            perpendicular2.y = (arcOfCircle->getCenter() - p2).x;
        }
        else if (isLineSegment(*geo2)) {
            auto* line = static_cast<const Part::GeomLineSegment*>(geo2);
            perpendicular2 = line->getStartPoint() - line->getEndPoint();
        }
        else {
            return false;
        }

        // if lines are parallel
        if ((perpendicular1 % perpendicular2).Length() < Precision::Confusion()) {
            return true;
        }

        return false;
    }

    void deleteOriginalGeometries()
    {
        std::stringstream stream;
        for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
            stream << listOfGeoIds[j] << ",";
        }
        stream << listOfGeoIds[listOfGeoIds.size() - 1];
        try {
            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                  "delGeometries([%s])",
                                  stream.str().c_str());
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("%s\n", e.what());
        }
    }

    void makeOffsetConstraint(std::vector<int>& listOfOffsetGeoIds)
    {
        SketchObject* Obj = sketchgui->getSketchObject();

        std::stringstream stream;
        stream << "conList = []\n";
        // We separate the constraints of new lines in case the construction lines are not needed.
        std::stringstream newLinesStream;
        newLinesStream << "conList2 = []\n";

        vCCO = generatevCC(listOfOffsetGeoIds);

        int geoIdCandidate1, geoIdCandidate2;

        int newCurveCounter = 0;
        int prevCurveCounter = 0;
        std::vector<Part::Geometry*> geometriesToAdd;
        for (auto& curve : vCCO) {
            // Check if curve is closed. Note as we use pipe it should always be closed but in case
            // we enable 'Skin' in the future.
            bool closed = isCurveClosed(curve);
            bool atLeastOneLine = false;
            bool rerunFirstAfterThis = false;
            bool rerunningFirst = false;
            bool inTangentGroup = false;

            for (size_t j = 0; j < curve.size(); j++) {

                // Tangent constraint is constraining the offset already. So if there are tangents
                // we should not create the construction lines. Hence the code below.
                bool createLine = true;
                bool forceCreate = false;
                if (!inTangentGroup && (!closed || j != 0 || rerunningFirst)) {
                    createLine = true;
                    atLeastOneLine = true;
                }
                else {
                    // include case of j == 0 and closed curve, because if required the line
                    // will be made after last.
                    createLine = false;
                }

                if (j + 1 < curve.size()) {
                    inTangentGroup = areTangentCoincident(curve[j], curve[j + 1]);
                }
                else if (j == curve.size() - 1 && closed) {
                    // Case of last geoId for closed curves.
                    inTangentGroup = areTangentCoincident(curve[j], curve[0]);
                    if (inTangentGroup) {
                        if (!atLeastOneLine) {  // We need at least one line
                            createLine = true;
                            forceCreate = true;
                        }
                    }
                    else {
                        // We rerun the for at j=0 after this run to create line for j = 0.
                        rerunFirstAfterThis = true;
                    }
                }

                const Part::Geometry* geo = Obj->getGeometry(curve[j]);
                for (auto geoId : listOfGeoIds) {
                    // Check if geoId is the offsetted curve giving curve[j].
                    const Part::Geometry* geo2 = Obj->getGeometry(geoId);

                    if (isCircle(*geo) && isCircle(*geo2)) {
                        auto* circle = static_cast<const Part::GeomCircle*>(geo);
                        auto* circle2 = static_cast<const Part::GeomCircle*>(geo2);
                        Base::Vector3d p1 = circle->getCenter();
                        Base::Vector3d p2 = circle2->getCenter();
                        if ((p1 - p2).Length() < Precision::Confusion()) {
                            // coincidence of center
                            stream << "conList.append(Sketcher.Constraint('Coincident'," << curve[j]
                                   << ",3, " << geoId << ",3))\n";

                            // Create line between both circles.
                            auto* line = new Part::GeomLineSegment();
                            p1.x = p1.x + circle->getRadius();
                            p2.x = p2.x + circle2->getRadius();
                            line->setPoints(p1, p2);
                            GeometryFacade::setConstruction(line, true);
                            geometriesToAdd.push_back(line);
                            newCurveCounter++;
                            newLinesStream << "conList2.append(Sketcher.Constraint('Perpendicular',"
                                           << getHighestCurveIndex() + newCurveCounter << ", "
                                           << curve[j] << "))\n";
                            newLinesStream << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                           << getHighestCurveIndex() + newCurveCounter << ",1, "
                                           << curve[j] << "))\n";
                            newLinesStream << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                           << getHighestCurveIndex() + newCurveCounter << ",2, "
                                           << geoId << "))\n";

                            geoIdCandidate1 = curve[j];
                            geoIdCandidate2 = geoId;

                            break;
                        }
                    }
                    else if (isEllipse(*geo) && isEllipse(*geo2)) {
                        // same as circle but 2 lines
                    }
                    else if (isLineSegment(*geo) && isLineSegment(*geo2)) {
                        auto* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo);
                        auto* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);
                        Base::Vector3d p1[2], p2[2];
                        p1[0] = lineSeg1->getStartPoint();
                        p1[1] = lineSeg1->getEndPoint();
                        p2[0] = lineSeg2->getStartPoint();
                        p2[1] = lineSeg2->getEndPoint();
                        // if lines are parallel
                        if (((p1[1] - p1[0]) % (p2[1] - p2[0])).Length()
                            < Precision::Intersection()) {
                            // If the lines are space by offsetLength distance
                            Base::Vector3d projectedP;
                            projectedP.ProjectToLine(p1[0] - p2[0], p2[1] - p2[0]);

                            if ((projectedP).Length() - fabs(offsetLength)
                                < Precision::Confusion()) {
                                if (!forceCreate && !rerunningFirst) {
                                    stream << "conList.append(Sketcher.Constraint('Parallel',"
                                           << curve[j] << ", " << geoId << "))\n";
                                }

                                // We don't need a construction line if the line has a tangent at
                                // one end. Unless it's the first line that we're making.
                                if (createLine) {
                                    auto* line = new Part::GeomLineSegment();
                                    line->setPoints(p1[0], p1[0] + projectedP);
                                    GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;

                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('Perpendicular',"
                                        << getHighestCurveIndex() + newCurveCounter << ", "
                                        << curve[j] << "))\n";
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                        << getHighestCurveIndex() + newCurveCounter << ",1, "
                                        << curve[j] << "))\n";
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                        << getHighestCurveIndex() + newCurveCounter << ",2, "
                                        << geoId << "))\n";

                                    geoIdCandidate1 = curve[j];
                                    geoIdCandidate2 = geoId;
                                }
                                break;
                            }
                        }
                    }
                    else if (isArcOfCircle(*geo)) {
                        // multiple cases because arc join mode creates arcs or circle.
                        auto* arcOfCircle = static_cast<const Part::GeomArcOfCircle*>(geo);
                        Base::Vector3d p1 = arcOfCircle->getCenter();

                        if (isArcOfCircle(*geo2)) {
                            auto* arcOfCircle2 = static_cast<const Part::GeomArcOfCircle*>(geo2);
                            Base::Vector3d p2 = arcOfCircle2->getCenter();
                            Base::Vector3d p3 = arcOfCircle2->getStartPoint(true);
                            Base::Vector3d p4 = arcOfCircle2->getEndPoint(true);

                            if ((p1 - p2).Length() < Precision::Confusion()) {
                                // coincidence of center. Offset arc is the offset of an arc
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << curve[j] << ",3, " << geoId << ",3))\n";
                                if (createLine) {
                                    // Create line between both circles.
                                    auto* line = new Part::GeomLineSegment();
                                    p1.x = p1.x + arcOfCircle->getRadius();
                                    p2.x = p2.x + arcOfCircle2->getRadius();
                                    line->setPoints(p1, p2);
                                    GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('Perpendicular',"
                                        << getHighestCurveIndex() + newCurveCounter << ", "
                                        << curve[j] << "))\n";
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                        << getHighestCurveIndex() + newCurveCounter << ",1, "
                                        << curve[j] << "))\n";
                                    newLinesStream
                                        << "conList2.append(Sketcher.Constraint('PointOnObject',"
                                        << getHighestCurveIndex() + newCurveCounter << ",2, "
                                        << geoId << "))\n";

                                    geoIdCandidate1 = curve[j];
                                    geoIdCandidate2 = geoId;
                                }
                                break;
                            }
                            else if ((p1 - p3).Length() < Precision::Confusion()) {
                                // coincidence of center to startpoint. offset arc is created arc
                                // join
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << curve[j] << ",3, " << geoId << ", 1))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius',"
                                           << curve[j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }
                            else if ((p1 - p4).Length() < Precision::Confusion()) {
                                // coincidence of center to startpoint
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << curve[j] << ",3, " << geoId << ", 2))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius',"
                                           << curve[j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }
                        }
                        else if (isLineSegment(*geo2)
                                 || geo2->getTypeId() == Part::GeomArcOfConic::getClassTypeId()
                                 || isBSplineCurve(*geo2)) {
                            // cases where arc is created by arc join mode.
                            Base::Vector3d p2, p3;

                            if (getFirstSecondPoints(geoId, p2, p3)) {
                                bool startCoincidence = (p1 - p2).Length() < Precision::Confusion();
                                bool endCoincidence = (p1 - p3).Length() < Precision::Confusion();

                                if (startCoincidence || endCoincidence) {
                                    // coincidence of center to startpoint
                                    stream << "conList.append(Sketcher.Constraint('Coincident',"
                                           << curve[j] << ", 3, " << geoId << ", "
                                           << (startCoincidence ? 1 : 2) << "))\n";

                                    geoIdCandidate1 = curve[j];
                                    geoIdCandidate2 = geoId;

                                    break;
                                }
                            }
                        }
                    }
                    else if (isArcOfEllipse(*geo) && isArcOfEllipse(*geo2)) {
                        // const Part::GeomArcOfEllipse* arcOfEllipse = static_cast<const
                        // Part::GeomArcOfEllipse*>(geo2);
                    }
                    else if (isArcOfHyperbola(*geo) && isArcOfHyperbola(*geo2)) {
                        // const Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<const
                        // Part::GeomArcOfHyperbola*>(geo2);
                    }
                    else if (isArcOfParabola(*geo) && isArcOfParabola(*geo2)) {
                        // const Part::GeomArcOfParabola* arcOfParabola = static_cast<const
                        // Part::GeomArcOfParabola*>(geo2);
                    }
                    else if (isBSplineCurve(*geo) && isBSplineCurve(*geo2)) {}
                }
                if (newCurveCounter != prevCurveCounter) {
                    prevCurveCounter = newCurveCounter;
                    if (newCurveCounter != 1) {
                        stream << "conList.append(Sketcher.Constraint('Equal',"
                               << getHighestCurveIndex() + newCurveCounter << ", "
                               << getHighestCurveIndex() + 1 << "))\n";
                    }
                }


                if (rerunningFirst) {
                    break;
                }

                if (rerunFirstAfterThis) {
                    j = -1;  // j will be incremented to 0 after new loop
                    rerunningFirst = true;
                }
            }
        }

        if (newCurveCounter >= 2) {
            stream << "conList.append(Sketcher.Constraint('Distance'," << getHighestCurveIndex() + 1
                   << ", " << fabs(offsetLength) << "))\n";

            Obj->addGeometry(std::move(geometriesToAdd));

            newLinesStream << Gui::Command::getObjectCmd(sketchgui->getObject())
                           << ".addConstraint(conList2)\n";
            newLinesStream << "del conList2\n";
            Gui::Command::doCommand(Gui::Command::Doc, newLinesStream.str().c_str());
        }
        else {
            // If there is a single construction line, then its not needed.
            const Part::Geometry* geo = Obj->getGeometry(geoIdCandidate1);

            if (isCircle(*geo)) {
                stream << "conList.append(Sketcher.Constraint('Distance'," << geoIdCandidate1
                       << ", " << geoIdCandidate2 << ", " << fabs(offsetLength) << "))\n";
            }
            else if (isLineSegment(*geo)) {
                stream << "conList.append(Sketcher.Constraint('Distance'," << geoIdCandidate1
                       << ", 1," << geoIdCandidate2 << ", " << fabs(offsetLength) << "))\n";
            }
            else if (isArcOfCircle(*geo)) {
                const Part::Geometry* geo2 = Obj->getGeometry(geoIdCandidate2);
                if (isArcOfCircle(*geo2)) {
                    stream << "conList.append(Sketcher.Constraint('Distance'," << geoIdCandidate1
                           << ", 1," << geoIdCandidate2 << ", 1, " << fabs(offsetLength) << "))\n";
                }
                else if (isLineSegment(*geo2)) {
                    stream << "conList.append(Sketcher.Constraint('Distance'," << geoIdCandidate1
                           << ", 3," << geoIdCandidate1 << ", 1, " << fabs(offsetLength) << "))\n";
                }
            }
        }

        stream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
        stream << "del conList\n";
        Gui::Command::doCommand(Gui::Command::Doc, stream.str().c_str());
    }

    std::vector<std::vector<int>> generatevCC(std::vector<int>& listOfGeo)
    {
        // This function separates all the selected geometries into separate continuous curves.
        SketchObject* Obj = sketchgui->getSketchObject();
        std::vector<std::vector<int>> vcc;

        for (auto geoId : listOfGeo) {
            std::vector<int> vecOfGeoIds;
            const Part::Geometry* geo = Obj->getGeometry(geoId);
            if (isCircle(*geo) || isEllipse(*geo)) {
                vecOfGeoIds.push_back(geoId);
                vcc.push_back(vecOfGeoIds);
                continue;
            }

            bool inserted = false;
            int insertedIn = -1;
            for (size_t j = 0; j < vcc.size(); j++) {
                for (size_t k = 0; k < vcc[j].size(); k++) {
                    if (!areCoincident(geoId, vcc[j][k])) {
                        continue;
                    }

                    if (inserted && insertedIn != int(j)) {
                        // if it's already inserted in another continuous curve then we need
                        // to merge both curves together. There're 2 cases, it could have
                        // been inserted at the end or at the beginning.
                        if (vcc[insertedIn][0] == geoId) {
                            // Two cases. Either the coincident is at the beginning or at
                            // the end.
                            if (k == 0) {
                                std::reverse(vcc[j].begin(), vcc[j].end());
                            }
                            vcc[j].insert(vcc[j].end(),
                                          vcc[insertedIn].begin(),
                                          vcc[insertedIn].end());
                            vcc.erase(vcc.begin() + insertedIn);
                        }
                        else {
                            if (k != 0) {  // ie k is  vcc[j].size()-1
                                std::reverse(vcc[j].begin(), vcc[j].end());
                            }
                            vcc[insertedIn].insert(vcc[insertedIn].end(),
                                                   vcc[j].begin(),
                                                   vcc[j].end());
                            vcc.erase(vcc.begin() + j);
                        }
                        j--;
                    }
                    else {
                        // we need to get the curves in the correct order.
                        if (k == vcc[j].size() - 1) {
                            vcc[j].push_back(geoId);
                        }
                        else {
                            // in this case k should actually be 0.
                            vcc[j].insert(vcc[j].begin() + k, geoId);
                        }
                        insertedIn = j;
                        inserted = true;
                    }
                    // printCCeVec();
                    break;
                }
            }
            if (!inserted) {
                vecOfGeoIds.push_back(geoId);
                vcc.push_back(vecOfGeoIds);
            }
        }
        return vcc;
    }

    void generateSourceWires()
    {
        vCC = generatevCC(listOfGeoIds);

        SketchObject* Obj = sketchgui->getSketchObject();

        for (auto& CC : vCC) {
            BRepBuilderAPI_MakeWire mkWire;
            for (auto& curve : CC) {
                mkWire.Add(TopoDS::Edge(Obj->getGeometry(curve)->toShape()));
            }

            // Here we make sure that if possible the first wire is not a single line.
            if (CC.size() == 1 && isLineSegment(*Obj->getGeometry(CC[0]))) {
                sourceWires.push_back(mkWire.Wire());
            }
            else {
                sourceWires.insert(sourceWires.begin(), mkWire.Wire());
                onlySingleLines = false;
            }
        }
    }

    void findOffsetLength()
    {
        double newOffsetLength = DBL_MAX;

        BRepBuilderAPI_MakeVertex mkVertex({endpoint.x, endpoint.y, 0.0});
        TopoDS_Vertex vertex = mkVertex.Vertex();
        for (auto& wire : sourceWires) {
            BRepExtrema_DistShapeShape distTool(wire, vertex);
            if (distTool.IsDone()) {
                double distance = distTool.Value();
                if (distance == std::min(distance, newOffsetLength)) {
                    newOffsetLength = distance;

                    gp_Pnt pnt = distTool.PointOnShape1(1);
                    pointOnSourceWire = Base::Vector2d(pnt.X(), pnt.Y());

                    // find direction
                    if (BRep_Tool::IsClosed(wire)) {
                        TopoDS_Face aFace = BRepBuilderAPI_MakeFace(wire);
                        BRepClass_FaceClassifier checkPoint(aFace,
                                                            {endpoint.x, endpoint.y, 0.0},
                                                            Precision::Confusion());
                        if (checkPoint.State() == TopAbs_IN) {
                            newOffsetLength = -newOffsetLength;
                        }
                    }
                }
            }
        }

        if (newOffsetLength != DBL_MAX) {
            offsetLength = newOffsetLength;
        }
    }

    bool getFirstSecondPoints(int geoId, Base::Vector3d& startPoint, Base::Vector3d& endPoint)
    {
        const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(geoId);

        if (isLineSegment(*geo)) {
            const auto* line = static_cast<const Part::GeomLineSegment*>(geo);
            startPoint = line->getStartPoint();
            endPoint = line->getEndPoint();
            return true;
        }
        else if (isArcOfCircle(*geo) || isArcOfEllipse(*geo) || isArcOfHyperbola(*geo)
                 || isArcOfParabola(*geo)) {
            const auto* arcOfConic = static_cast<const Part::GeomArcOfConic*>(geo);
            startPoint = arcOfConic->getStartPoint(true);
            endPoint = arcOfConic->getEndPoint(true);
            return true;
        }
        else if (isBSplineCurve(*geo)) {
            const auto* bSpline = static_cast<const Part::GeomBSplineCurve*>(geo);
            startPoint = bSpline->getStartPoint();
            endPoint = bSpline->getEndPoint();
            return true;
        }
        return false;
    }

    CoincidencePointPos checkForCoincidence(int geoId1, int geoId2, bool tangentOnly = false)
    {
        // This function looks up for 2 coincidence between 2 edges (arc + line can have 2)
        SketchObject* Obj = sketchgui->getSketchObject();
        const std::vector<Constraint*>& vals = Obj->Constraints.getValues();
        CoincidencePointPos positions;
        positions.firstPos1 = PointPos::none;
        positions.secondPos1 = PointPos::none;
        positions.firstPos2 = PointPos::none;
        positions.secondPos2 = PointPos::none;
        bool firstCoincidenceFound = false;
        for (auto* cstr : vals) {
            if (((tangentOnly || cstr->Type != Coincident) && cstr->Type != Tangent)
                || cstr->FirstPos == PointPos::mid || cstr->FirstPos == PointPos::none
                || cstr->SecondPos == PointPos::mid || cstr->SecondPos == PointPos::none) {
                continue;
            }

            if ((cstr->First == geoId1 && cstr->Second == geoId2)
                || (cstr->First == geoId2 && cstr->Second == geoId1)) {
                if (!firstCoincidenceFound) {
                    positions.firstPos1 = cstr->First == geoId1 ? cstr->FirstPos : cstr->SecondPos;
                    positions.secondPos1 = cstr->First == geoId2 ? cstr->FirstPos : cstr->SecondPos;
                    firstCoincidenceFound = true;
                }
                else {
                    positions.firstPos2 = cstr->First == geoId1 ? cstr->FirstPos : cstr->SecondPos;
                    positions.secondPos2 = cstr->First == geoId2 ? cstr->FirstPos : cstr->SecondPos;
                    break;
                }
            }
        }
        return positions;
    }

    bool areCoincident(int geoId1, int geoId2)
    {
        CoincidencePointPos ppc = checkForCoincidence(geoId1, geoId2);
        return ppc.firstPos1 != PointPos::none;
    }

    bool areTangentCoincident(int geoId1, int geoId2)
    {
        CoincidencePointPos ppc = checkForCoincidence(geoId1, geoId2, true);
        return ppc.firstPos1 != PointPos::none;
    }

    bool isCurveClosed(std::vector<int>& curve)
    {
        bool closed = false;
        if (curve.size() > 2) {
            closed = areCoincident(curve[0], curve[curve.size() - 1]);
        }
        else if (curve.size() == 2) {
            // if only 2 elements, we need to check if they close end to end.
            CoincidencePointPos cpp = checkForCoincidence(curve[0], curve[curve.size() - 1]);
            closed = cpp.firstPos1 != PointPos::none && cpp.firstPos2 != PointPos::none;
        }
        return closed;
    }

    // debug only
    /*void printCCeVec()
    {
        for (size_t j = 0; j < vCC.size(); j++) {
            Base::Console().Warning("curve %d{", j);
            for (size_t k = 0; k < vCC[j].size(); k++) {
                Base::Console().Warning("%d, ", vCC[j][k]);
            }
            Base::Console().Warning("}\n");
        }
    }*/
};

template<>
auto DSHOffsetControllerBase::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
            return SelectMode::SeekFirst;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template<>
void DSHOffsetController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {QApplication::translate("Sketcher_CreateOffset", "Arc"),
                             QApplication::translate("Sketcher_CreateOffset", "Intersection")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        toolWidget->setComboboxItemIcon(WCombobox::FirstCombo,
                                        0,
                                        Gui::BitmapFactory().iconFromTheme("Sketcher_OffsetArc"));
        toolWidget->setComboboxItemIcon(
            WCombobox::FirstCombo,
            1,
            Gui::BitmapFactory().iconFromTheme("Sketcher_OffsetIntersection"));

        toolWidget->setCheckboxLabel(WCheckbox::FirstBox,
                                     QApplication::translate("TaskSketcherTool_c1_offset",
                                                             "Delete original geometries (U)"));
        toolWidget->setCheckboxLabel(
            WCheckbox::SecondBox,
            QApplication::translate("TaskSketcherTool_c2_offset", "Add offset constraint (J)"));
    }

    onViewParameters[OnViewParameter::First]->setLabelType(
        Gui::SoDatumLabel::DISTANCE,
        Gui::EditableDatumLabel::Function::Dimensioning);
}

template<>
void DSHOffsetControllerBase::adaptDrawingToOnViewParameterChange(int labelindex, double value)
{
    switch (labelindex) {
        case OnViewParameter::First: {
            if (value == 0.) {
                // Do not accept 0.
                unsetOnViewParameter(onViewParameters[OnViewParameter::First].get());

                Gui::NotifyUserError(
                    handler->sketchgui->getSketchObject(),
                    QT_TRANSLATE_NOOP("Notifications", "Invalid Value"),
                    QT_TRANSLATE_NOOP("Notifications", "Offset value can't be 0."));
            }
            else {
                handler->offsetLengthSet = true;
                handler->offsetLength = value;
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHOffsetController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    switch (checkboxindex) {
        case WCheckbox::FirstBox:
            handler->deleteOriginal = value;

            // Both options cannot be enabled at the same time.
            if (value && toolWidget->getCheckboxChecked(WCheckbox::SecondBox)) {
                toolWidget->setCheckboxChecked(WCheckbox::SecondBox, false);
            }
            break;

        case WCheckbox::SecondBox:
            handler->offsetConstraint = value;

            // Both options cannot be enabled at the same time.
            if (value && toolWidget->getCheckboxChecked(WCheckbox::FirstBox)) {
                toolWidget->setCheckboxChecked(WCheckbox::FirstBox, false);
            }
            break;
    }
}

/* doEnforceControlParameters : The tool validates after offset length is set. So we don't need to
 * enforce it. Besides it is hard to override onsketchpos such that it is at offsetLength from the
 * curve. As we do not override the pos, we need to use offsetLengthSet to prevent rewrite of
 * offsetLength.*/

template<>
void DSHOffsetController::adaptParameters(Base::Vector2d onSketchPos)
{
    Q_UNUSED(onSketchPos)

    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (!onViewParameters[OnViewParameter::First]->isSet) {
                setOnViewParameterValue(OnViewParameter::First, handler->offsetLength);
            }

            onViewParameters[OnViewParameter::First]->setPoints(
                Base::Vector3d(handler->endpoint.x, handler->endpoint.y, 0.),
                Base::Vector3d(handler->pointOnSourceWire.x, handler->pointOnSourceWire.y, 0.));
        } break;
        default:
            break;
    }
}

template<>
void DSHOffsetController::doChangeDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet) {
                handler->setState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerOffset_H
