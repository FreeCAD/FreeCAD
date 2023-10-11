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

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

#include <Mod/Sketcher/App/GeometryFacade.h>

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

using DrawSketchHandlerOffsetBase =
    DrawSketchDefaultWidgetHandler<DrawSketchHandlerOffset,
                                   StateMachines::OneSeekEnd,
                                   /*PEditCurveSize =*/0,
                                   /*PAutoConstraintSize =*/0,
                                   /*OnViewParametersT =*/OnViewParameters<1, 1>,
                                   /*WidgetParametersT =*/WidgetParameters<0, 0>,
                                   /*WidgetCheckboxesT =*/WidgetCheckboxes<2, 2>,
                                   /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
                                   ConstructionMethods::OffsetConstructionMethod,
                                   /*bool PFirstComboboxIsConstructionMethod =*/true>;

class DrawSketchHandlerOffset: public DrawSketchHandlerOffsetBase
{
    friend DrawSketchHandlerOffsetBase;

public:
    DrawSketchHandlerOffset(std::vector<int> listOfGeoIds,
                            ConstructionMethod constrMethod = ConstructionMethod::Arc)
        : DrawSketchHandlerOffsetBase(constrMethod)
        , listOfGeoIds(listOfGeoIds)
        , endpoint(Base::Vector2d(0., 0.))
        , pointOnSourceWire(Base::Vector2d(0., 0.))
        , deleteOriginal(false)
        , offsetLengthSet(false)
        , offsetConstraint(false)
        , offsetLength(1.)
    {}

    virtual ~DrawSketchHandlerOffset() = default;


private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        if (state() == SelectMode::SeekFirst) {
            endpoint = onSketchPos;

            if (!offsetLengthSet) {
                findOffsetLength();
            }

            if (fabs(offsetLength) > Precision::Confusion()) {
                drawOffsetPreview();
            }
        }
    }

    virtual void executeCommands() override
    {
        if (fabs(offsetLength) > Precision::Confusion()) {
            createOffset();
        }
    }

    virtual std::string getToolName() const override
    {
        return "DSH_Offset";
    }

    virtual QString getCrosshairCursorSVGName() const override
    {
        if (constructionMethod() == DrawSketchHandlerOffset::ConstructionMethod::Arc) {
            return QString::fromLatin1("Sketcher_OffsetArc");
        }
        else {  // constructionMethod == DrawSketchHandlerOffset::ConstructionMethod::Intersection
            return QString::fromLatin1("Sketcher_OffsetIntersection");
        }
    }

    virtual void activated() override
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
        Sketcher::PointPos FirstGeoPos;
        Sketcher::PointPos SecondGeoPos;
        Sketcher::PointPos SecondCoincidenceFirstGeoPos;
        Sketcher::PointPos SecondCoincidenceSecondGeoPos;
    };

    std::vector<int> listOfGeoIds;
    std::vector<std::vector<int>> vCC;
    std::vector<std::vector<int>> vCCO;
    Base::Vector2d endpoint, pointOnSourceWire;
    std::vector<TopoDS_Wire> sourceWires;

    bool deleteOriginal, offsetLengthSet, offsetConstraint;
    double offsetLength;
    int firstCurveCreated;

    TopoDS_Shape makeOffsetShape(bool allowOpenResult = false)
    {
        // in OCC the JointTypes are : Arc(0), Tangent(1), Intersection(2)
        short joinType =
            constructionMethod() == DrawSketchHandlerOffset::ConstructionMethod::Arc ? 0 : 2;

        Part::BRepOffsetAPI_MakeOffsetFix mkOffset(GeomAbs_JoinType(joinType), allowOpenResult);
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
        Sketcher::GeometryFacade::setConstruction(line, false);
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

            Sketcher::GeometryFacade::setConstruction(gCircle, false);
            return gCircle;
        }
        else {
            auto* gArc = new Part::GeomArcOfCircle();
            Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
            Handle(Geom_TrimmedCurve) tCurve =
                new Geom_TrimmedCurve(hCircle, curve.FirstParameter(), curve.LastParameter());
            gArc->setHandle(tCurve);
            Sketcher::GeometryFacade::setConstruction(gArc, false);
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
            Sketcher::GeometryFacade::setConstruction(gEllipse, false);
            return gEllipse;
        }
        else {
            Handle(Geom_Curve) hEllipse = new Geom_Ellipse(ellipse);
            Handle(Geom_TrimmedCurve) tCurve =
                new Geom_TrimmedCurve(hEllipse, curve.FirstParameter(), curve.LastParameter());
            auto* gArc = new Part::GeomArcOfEllipse();
            gArc->setHandle(tCurve);
            Sketcher::GeometryFacade::setConstruction(gArc, false);
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

        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
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
                int posi, posj;

                if ((firstStartPoint - secondStartPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = 1;
                    posj = 1;
                }
                else if ((firstStartPoint - secondEndPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = 1;
                    posj = 2;
                }
                else if ((firstEndPoint - secondStartPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = 2;
                    posj = 1;
                }
                else if ((firstEndPoint - secondEndPoint).Length() < Precision::Confusion()) {
                    create = true;
                    posi = 2;
                    posj = 2;
                }

                if (create) {
                    bool tangent =
                        needTangent(listOfOffsetGeoIds[i], listOfOffsetGeoIds[j], posi, posj);
                    stream << "conList.append(Sketcher.Constraint('"
                           << (tangent ? "Tangent" : "Coincident");
                    stream << "'," << listOfOffsetGeoIds[i] << "," << posi << ", "
                           << listOfOffsetGeoIds[j] << "," << posj << "))\n";
                }
            }
        }

        stream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
        stream << "del conList\n";
        Gui::Command::doCommand(Gui::Command::Doc, stream.str().c_str());
    }

    bool needTangent(int geoId1, int geoId2, int pos1, int pos2)
    {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        const Part::Geometry* geo1 = Obj->getGeometry(geoId1);
        const Part::Geometry* geo2 = Obj->getGeometry(geoId2);

        if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
            || geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            Base::Vector3d perpendicular1, perpendicular2, p1, p2;
            if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                || geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                const Part::GeomArcOfConic* arcOfCircle =
                    static_cast<const Part::GeomArcOfConic*>(geo1);
                p1 = arcOfCircle->getEndPoint();
                if (pos1 == 1) {
                    p1 = arcOfCircle->getStartPoint();
                }
                perpendicular1.x = -(arcOfCircle->getCenter() - p1).y;
                perpendicular1.y = (arcOfCircle->getCenter() - p1).x;
            }
            else if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo1);
                p1 = line->getEndPoint();
                perpendicular1 = line->getStartPoint() - line->getEndPoint();
                if (pos1 == 1) {
                    p1 = line->getStartPoint();
                    perpendicular1 = line->getEndPoint() - line->getStartPoint();
                }
            }
            else {
                return false;
            }
            // Todo: add cases for arcOfellipse parabolas hyperbolas bspline
            if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                || geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                const Part::GeomArcOfConic* arcOfCircle =
                    static_cast<const Part::GeomArcOfConic*>(geo2);
                p2 = arcOfCircle->getEndPoint();
                if (pos2 == 1) {
                    p2 = arcOfCircle->getStartPoint();
                }
                perpendicular2.x = -(arcOfCircle->getCenter() - p2).y;
                perpendicular2.y = (arcOfCircle->getCenter() - p2).x;
            }
            else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo2);
                p2 = line->getEndPoint();
                perpendicular2 = line->getStartPoint() - line->getEndPoint();
                if (pos2 == 1) {
                    p2 = line->getStartPoint();
                    perpendicular2 = line->getEndPoint() - line->getStartPoint();
                }
            }
            else {
                return false;
            }
            // Todo: add cases for arcOfellipse parabolas hyperbolas bspline

            // if lines are parallel
            if ((perpendicular1 % perpendicular2).Length() < Precision::Intersection()) {
                return true;
            }
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
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        std::stringstream stream;
        stream << "conList = []\n";
        vCCO = generatevCC(listOfOffsetGeoIds);

        int newCurveCounter = 0;
        int prevCurveCounter = 0;
        std::vector<Part::Geometry*> geometriesToAdd;
        for (size_t i = 0; i < vCCO.size(); i++) {
            // Check if curve is closed. Note as we use pipe it should always be closed but in case
            // we enable 'Skin' in the future.
            bool isCurveClosed = false;
            if (vCCO[i].size() > 2) {
                CoincidencePointPos cpp =
                    checkForCoincidence(vCCO[i][0], vCCO[i][vCCO[i].size() - 1]);
                if (cpp.FirstGeoPos != Sketcher::PointPos::none) {
                    isCurveClosed = true;
                }
            }
            else if (vCCO[i].size() == 2) {
                // if only 2 elements, we need to check that they don't close end to end.
                CoincidencePointPos cpp =
                    checkForCoincidence(vCCO[i][0], vCCO[i][vCCO[i].size() - 1]);
                if (cpp.FirstGeoPos != Sketcher::PointPos::none) {
                    if (cpp.SecondCoincidenceFirstGeoPos != Sketcher::PointPos::none) {
                        isCurveClosed = true;
                    }
                }
            }
            bool atLeastOneLine = false;
            bool reRunForFirst = false;
            bool inTangentGroup = false;

            for (size_t j = 0; j < vCCO[i].size(); j++) {

                // Tangent constraint is constraining the offset already. So if there are tangents
                // we should not create the construction lines. Hence the code below.
                bool createLine = true;
                bool forceCreate = false;
                if (!inTangentGroup && (!isCurveClosed || j != 0 || reRunForFirst)) {
                    createLine = true;
                    atLeastOneLine = true;
                }
                else {  // include case of j == 0 and closed curve, because if required the line
                        // will be made after last.
                    createLine = false;
                }

                if (j + 1 < vCCO[i].size()) {
                    CoincidencePointPos ppc = checkForCoincidence(vCCO[i][j],
                                                                  vCCO[i][j + 1],
                                                                  true);  // true is tangentOnly
                    if (ppc.FirstGeoPos != Sketcher::PointPos::none) {
                        inTangentGroup = true;
                    }
                    else {
                        inTangentGroup = false;
                    }
                }
                else if (j == vCCO[i].size() - 1
                         && isCurveClosed) {  // Case of last geoId for closed curves.
                    CoincidencePointPos ppc = checkForCoincidence(vCCO[i][j], vCCO[i][0], true);
                    if (ppc.FirstGeoPos != Sketcher::PointPos::none) {
                        if (!atLeastOneLine) {  // We need at least one line
                            createLine = true;
                            forceCreate = true;
                        }
                    }
                    else {
                        // create line for j = 0. For this we rerun the for at j=0 after this run.
                        // With an escape bool.
                        reRunForFirst = true;
                        inTangentGroup = false;
                    }
                }

                const Part::Geometry* geo = Obj->getGeometry(vCCO[i][j]);
                for (size_t k = 0; k < listOfGeoIds.size(); k++) {
                    // Check if listOfGeoIds[k] is the offsetted curve giving curve i-j.
                    const Part::Geometry* geo2 = Obj->getGeometry(listOfGeoIds[k]);

                    if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()
                        && geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                        const Part::GeomCircle* circle = static_cast<const Part::GeomCircle*>(geo);
                        const Part::GeomCircle* circle2 =
                            static_cast<const Part::GeomCircle*>(geo2);
                        Base::Vector3d p1 = circle->getCenter();
                        Base::Vector3d p2 = circle2->getCenter();
                        if ((p1 - p2).Length() < Precision::Confusion()) {
                            // coincidence of center
                            stream << "conList.append(Sketcher.Constraint('Coincident',"
                                   << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ",3))\n";
                            // Create line between both circles.
                            Part::GeomLineSegment* line = new Part::GeomLineSegment();
                            p1.x = p1.x + circle->getRadius();
                            p2.x = p2.x + circle2->getRadius();
                            line->setPoints(p1, p2);
                            Sketcher::GeometryFacade::setConstruction(line, true);
                            geometriesToAdd.push_back(line);
                            newCurveCounter++;
                            stream << "conList.append(Sketcher.Constraint('Perpendicular',"
                                   << getHighestCurveIndex() + newCurveCounter << ", " << vCCO[i][j]
                                   << "))\n";
                            stream << "conList.append(Sketcher.Constraint('PointOnObject',"
                                   << getHighestCurveIndex() + newCurveCounter << ",1, "
                                   << vCCO[i][j] << "))\n";
                            stream << "conList.append(Sketcher.Constraint('PointOnObject',"
                                   << getHighestCurveIndex() + newCurveCounter << ",2, "
                                   << listOfGeoIds[k] << "))\n";
                            break;
                        }
                    }
                    else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()
                             && geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                        // same as circle but 2 lines
                    }
                    else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                             && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        const Part::GeomLineSegment* lineSeg1 =
                            static_cast<const Part::GeomLineSegment*>(geo);
                        const Part::GeomLineSegment* lineSeg2 =
                            static_cast<const Part::GeomLineSegment*>(geo2);
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
                                if (!forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Parallel',"
                                           << vCCO[i][j] << ", " << listOfGeoIds[k] << "))\n";
                                }

                                // We don't need a construction line if the line has a tangent at
                                // one end. Unless it's the first line that we're making.
                                if (createLine) {
                                    Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                    line->setPoints(p1[0], p1[0] + projectedP);
                                    Sketcher::GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;

                                    stream << "conList.append(Sketcher.Constraint('Perpendicular',"
                                           << getHighestCurveIndex() + newCurveCounter << ", "
                                           << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject',"
                                           << getHighestCurveIndex() + newCurveCounter << ",1, "
                                           << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject',"
                                           << getHighestCurveIndex() + newCurveCounter << ",2, "
                                           << listOfGeoIds[k] << "))\n";
                                }
                                break;
                            }
                        }
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                        // multiple cases because arc join mode creates arcs or circle.
                        const Part::GeomArcOfCircle* arcOfCircle =
                            static_cast<const Part::GeomArcOfCircle*>(geo);
                        Base::Vector3d p1 = arcOfCircle->getCenter();

                        if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arcOfCircle2 =
                                static_cast<const Part::GeomArcOfCircle*>(geo2);
                            Base::Vector3d p2 = arcOfCircle2->getCenter();
                            Base::Vector3d p3 = arcOfCircle2->getStartPoint();
                            Base::Vector3d p4 = arcOfCircle2->getEndPoint();

                            if ((p1 - p2).Length() < Precision::Confusion()) {
                                // coincidence of center. Offset arc is the offset of an arc
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ",3))\n";
                                if (createLine) {
                                    // Create line between both circles.
                                    Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                    p1.x = p1.x + arcOfCircle->getRadius();
                                    p2.x = p2.x + arcOfCircle2->getRadius();
                                    line->setPoints(p1, p2);
                                    Sketcher::GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;
                                    stream << "conList.append(Sketcher.Constraint('Perpendicular',"
                                           << getHighestCurveIndex() + newCurveCounter << ", "
                                           << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject',"
                                           << getHighestCurveIndex() + newCurveCounter << ",1, "
                                           << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject',"
                                           << getHighestCurveIndex() + newCurveCounter << ",2, "
                                           << listOfGeoIds[k] << "))\n";
                                }
                                break;
                            }
                            else if ((p1 - p3).Length() < Precision::Confusion()) {
                                // coincidence of center to startpoint. offset arc is created arc
                                // join
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 1))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius',"
                                           << vCCO[i][j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }
                            else if ((p1 - p4).Length() < Precision::Confusion()) {
                                // coincidence of center to startpoint
                                stream << "conList.append(Sketcher.Constraint('Coincident',"
                                       << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 2))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius',"
                                           << vCCO[i][j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }
                        }
                        else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                                 || geo2->getTypeId() == Part::GeomArcOfConic::getClassTypeId()
                                 || geo2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                            // cases where arc is created by arc join mode.
                            Base::Vector3d p2, p3;

                            if (getFirstSecondPoints(listOfGeoIds[k], p2, p3)) {
                                if (((p1 - p2).Length() < Precision::Confusion())
                                    || ((p1 - p3).Length() < Precision::Confusion())) {
                                    if ((p1 - p2).Length() < Precision::Confusion()) {
                                        // coincidence of center to startpoint
                                        stream << "conList.append(Sketcher.Constraint('Coincident',"
                                               << vCCO[i][j] << ",3, " << listOfGeoIds[k]
                                               << ", 1))\n";
                                    }
                                    else if ((p1 - p3).Length() < Precision::Confusion()) {
                                        // coincidence of center to endpoint
                                        stream << "conList.append(Sketcher.Constraint('Coincident',"
                                               << vCCO[i][j] << ",3, " << listOfGeoIds[k]
                                               << ", 2))\n";
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                             && geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                        // const Part::GeomArcOfEllipse* arcOfEllipse = static_cast<const
                        // Part::GeomArcOfEllipse*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
                             && geo2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                        // const Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<const
                        // Part::GeomArcOfHyperbola*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()
                             && geo2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                        // const Part::GeomArcOfParabola* arcOfParabola = static_cast<const
                        // Part::GeomArcOfParabola*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()
                             && geo2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {}
                }
                if (newCurveCounter != prevCurveCounter) {
                    prevCurveCounter = newCurveCounter;
                    if (newCurveCounter != 1) {
                        stream << "conList.append(Sketcher.Constraint('Equal',"
                               << getHighestCurveIndex() + newCurveCounter << ", "
                               << getHighestCurveIndex() + 1 << "))\n";
                    }
                }

                if (reRunForFirst) {
                    if (j != 0) {
                        j = -1;
                    }  // j will be incremented to 0 after new loop
                    else {
                        break;
                    }
                }
            }
        }
        if (newCurveCounter != 0) {
            stream << "conList.append(Sketcher.Constraint('Distance'," << getHighestCurveIndex() + 1
                   << ", " << offsetLength << "))\n";
        }
        Obj->addGeometry(std::move(geometriesToAdd));

        stream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
        stream << "del conList\n";
        Gui::Command::doCommand(Gui::Command::Doc, stream.str().c_str());
    }

    std::vector<std::vector<int>> generatevCC(std::vector<int>& listOfGeo)
    {
        // This function separates all the selected geometries into separate continuous curves.
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        std::vector<std::vector<int>> vcc;

        for (size_t i = 0; i < listOfGeo.size(); i++) {
            std::vector<int> vecOfGeoIds;
            const Part::Geometry* geo = Obj->getGeometry(listOfGeo[i]);
            if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()
                || geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                vecOfGeoIds.push_back(listOfGeo[i]);
                vcc.push_back(vecOfGeoIds);
            }
            else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                     || geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                     || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                     || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
                     || geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()
                     || geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                bool inserted = 0;
                int insertedIn = -1;
                for (size_t j = 0; j < vcc.size(); j++) {
                    for (size_t k = 0; k < vcc[j].size(); k++) {
                        CoincidencePointPos pointPosOfCoincidence =
                            checkForCoincidence(listOfGeo[i], vcc[j][k]);
                        if (pointPosOfCoincidence.FirstGeoPos != Sketcher::PointPos::none) {
                            if (inserted && insertedIn != int(j)) {
                                // if it's already inserted in another continuous curve then we need
                                // to merge both curves together. There're 2 cases, it could have
                                // been inserted at the end or at the beginning.
                                if (vcc[insertedIn][0] == listOfGeo[i]) {
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
                                    vcc[j].push_back(listOfGeo[i]);
                                }
                                else {
                                    // in this case k should actually be 0.
                                    vcc[j].insert(vcc[j].begin() + k, listOfGeo[i]);
                                }
                                insertedIn = j;
                                inserted = 1;
                            }
                            // printCCeVec();
                            break;
                        }
                    }
                }
                if (!inserted) {
                    vecOfGeoIds.push_back(listOfGeo[i]);
                    vcc.push_back(vecOfGeoIds);
                }
            }
        }
        return vcc;
    }

    void generateSourceWires()
    {
        vCC = generatevCC(listOfGeoIds);

        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        for (size_t i = 0; i < vCC.size(); i++) {
            BRepBuilderAPI_MakeWire mkWire;
            for (size_t j = 0; j < vCC[i].size(); j++) {
                mkWire.Add(TopoDS::Edge(Obj->getGeometry(vCC[i][j])->toShape()));
            }
            sourceWires.push_back(mkWire.Wire());
        }
    }

    void findOffsetLength()
    {
        double newOffsetLength = LONG_MAX;

        BRepBuilderAPI_MakeVertex mkVertex({endpoint.x, endpoint.y, 0.0});
        TopoDS_Vertex vertex = mkVertex.Vertex();
        for (size_t i = 0; i < sourceWires.size(); i++) {
            BRepExtrema_DistShapeShape distTool(sourceWires[i], vertex);
            if (distTool.IsDone()) {
                double distance = distTool.Value();
                if (distance == std::min(distance, newOffsetLength)) {
                    newOffsetLength = distance;

                    gp_Pnt pnt = distTool.PointOnShape1(1);
                    pointOnSourceWire = Base::Vector2d(pnt.X(), pnt.Y());

                    // find direction
                    if (BRep_Tool::IsClosed(sourceWires[i])) {
                        TopoDS_Face aFace = BRepBuilderAPI_MakeFace(sourceWires[i]);
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

        if (newOffsetLength != LONG_MAX) {
            offsetLength = newOffsetLength;
        }
    }

    bool getFirstSecondPoints(int geoId, Base::Vector3d& startPoint, Base::Vector3d& endPoint)
    {
        const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(geoId);

        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const auto* line = static_cast<const Part::GeomLineSegment*>(geo);
            startPoint = line->getStartPoint();
            endPoint = line->getEndPoint();
            return true;
        }
        else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                 || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                 || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
                 || geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            const auto* arcOfConic = static_cast<const Part::GeomArcOfConic*>(geo);
            startPoint = arcOfConic->getStartPoint(true);
            endPoint = arcOfConic->getEndPoint(true);
            return true;
        }
        else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            const auto* bSpline = static_cast<const Part::GeomBSplineCurve*>(geo);
            startPoint = bSpline->getStartPoint();
            endPoint = bSpline->getEndPoint();
            return true;
        }
        return false;
    }

    CoincidencePointPos checkForCoincidence(int GeoId1, int GeoId2, bool tangentOnly = false)
    {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();
        CoincidencePointPos positions;
        positions.FirstGeoPos = Sketcher::PointPos::none;
        positions.SecondGeoPos = Sketcher::PointPos::none;
        positions.SecondCoincidenceFirstGeoPos = Sketcher::PointPos::none;
        positions.SecondCoincidenceSecondGeoPos = Sketcher::PointPos::none;
        bool firstCoincidenceFound = 0;
        for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
             ++it) {
            if ((!tangentOnly && (*it)->Type == Sketcher::Coincident)
                || (*it)->Type == Sketcher::Tangent) {
                if ((*it)->First == GeoId1 && (*it)->FirstPos != Sketcher::PointPos::mid
                    && (*it)->FirstPos != Sketcher::PointPos::none && (*it)->Second == GeoId2
                    && (*it)->SecondPos != Sketcher::PointPos::mid
                    && (*it)->SecondPos != Sketcher::PointPos::none) {
                    if (!firstCoincidenceFound) {
                        positions.FirstGeoPos = (*it)->FirstPos;
                        positions.SecondGeoPos = (*it)->SecondPos;
                        firstCoincidenceFound = 1;
                    }
                    else {
                        positions.SecondCoincidenceFirstGeoPos = (*it)->FirstPos;
                        positions.SecondCoincidenceSecondGeoPos = (*it)->SecondPos;
                    }
                }
                else if ((*it)->First == GeoId2 && (*it)->FirstPos != Sketcher::PointPos::mid
                         && (*it)->FirstPos != Sketcher::PointPos::none && (*it)->Second == GeoId1
                         && (*it)->SecondPos != Sketcher::PointPos::mid
                         && (*it)->SecondPos != Sketcher::PointPos::none) {
                    if (!firstCoincidenceFound) {
                        positions.FirstGeoPos = (*it)->SecondPos;
                        positions.SecondGeoPos = (*it)->FirstPos;
                        firstCoincidenceFound = 1;
                    }
                    else {
                        positions.SecondCoincidenceFirstGeoPos = (*it)->SecondPos;
                        positions.SecondCoincidenceSecondGeoPos = (*it)->FirstPos;
                    }
                }
            }
        }
        return positions;
    }

    // debug only
    void printCCeVec()
    {
        for (size_t j = 0; j < vCC.size(); j++) {
            Base::Console().Warning("curve %d{", j);
            for (size_t k = 0; k < vCC[j].size(); k++) {
                Base::Console().Warning("%d, ", vCC[j][k]);
            }
            Base::Console().Warning("}\n");
        }
    }
};

template<>
auto DrawSketchHandlerOffsetBase::ToolWidgetManager::getState(int labelindex) const
{
    switch (labelindex) {
        case WLabel::First:
            return SelectMode::SeekFirst;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template<>
void DrawSketchHandlerOffsetBase::ToolWidgetManager::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Arc"), QStringLiteral("Intersection")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    toolWidget->setCheckboxLabel(
        WCheckbox::FirstBox,
        QApplication::translate("TaskSketcherTool_c1_offset", "Delete original geometries"));
    toolWidget->setCheckboxLabel(
        WCheckbox::SecondBox,
        QApplication::translate("TaskSketcherTool_c2_offset", "Add offset constraint"));
}

template<>
void DrawSketchHandlerOffsetBase::ToolWidgetManager::beforeFirstMouseMove(
    Base::Vector2d onSketchPos)
{
    onViewParameters[WLabel::First]->activate();
    onViewParameters[WLabel::First]->setLabelType(Gui::SoDatumLabel::DISTANCE);
    onViewParameters[WLabel::First]->setPoints(Base::Vector3d(0., 0., 0.),
                                               Base::Vector3d(onSketchPos.x, onSketchPos.y, 0.));

    onViewParameters[WLabel::First]->startEdit(dHandler->offsetLength, toolWidget);
    onViewParameters[WLabel::First]->setSpinboxInvisibleToMouse(true);
}

template<>
void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptDrawingToLabelChange(int labelindex,
                                                                               double value)
{
    switch (labelindex) {
        case WLabel::First: {
            if (value == 0.) {
                // Do not accept 0.
                onViewParameters[WLabel::First]->isSet = false;
                onViewParameters[WLabel::First]->setColor(SbColor(0.8f, 0.8f, 0.8f));

                Gui::NotifyUserError(
                    dHandler->sketchgui->getSketchObject(),
                    QT_TRANSLATE_NOOP("Notifications", "Invalid Value"),
                    QT_TRANSLATE_NOOP("Notifications", "Offset value can't be 0."));
            }
            else {
                dHandler->offsetLengthSet = true;
                dHandler->offsetLength = value;
            }
        } break;
        default:
            break;
    }
}

template<>
void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex,
                                                                                  bool value)
{
    switch (checkboxindex) {
        case WCheckbox::FirstBox:
            dHandler->deleteOriginal = value;

            // Both options cannot be enabled at the same time.
            if (value && toolWidget->getCheckboxChecked(WCheckbox::SecondBox)) {
                toolWidget->setCheckboxChecked(WCheckbox::SecondBox, false);
            }
            break;

        case WCheckbox::SecondBox:
            dHandler->offsetConstraint = value;

            // Both options cannot be enabled at the same time.
            if (value && toolWidget->getCheckboxChecked(WCheckbox::FirstBox)) {
                toolWidget->setCheckboxChecked(WCheckbox::FirstBox, false);
            }
            break;
    }
}

template<>
void DrawSketchHandlerOffsetBase::ToolWidgetManager::doEnforceWidgetParameters(
    Base::Vector2d& onSketchPos)
{
    // The tool validates after offset length is set. So we don't need to enforce it.
    // Besides it is hard to override onsketchpos such that it is at offsetLength from the curve.
    // As we do not override the pos, we need to use offsetLengthSet to prevent rewrite of
    // offsetLength.
    lastWidgetEnforcedPosition = onSketchPos;
}

template<>
void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptParameters(Base::Vector2d onSketchPos)
{
    Q_UNUSED(onSketchPos)

    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (!onViewParameters[WLabel::First]->isSet) {
                onViewParameters[WLabel::First]->setSpinboxValue(dHandler->offsetLength);
            }

            onViewParameters[WLabel::First]->setPoints(
                Base::Vector3d(dHandler->endpoint.x, dHandler->endpoint.y, 0.),
                Base::Vector3d(dHandler->pointOnSourceWire.x, dHandler->pointOnSourceWire.y, 0.));
        } break;
        default:
            break;
    }
}

template<>
void DrawSketchHandlerOffsetBase::ToolWidgetManager::doChangeDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[WLabel::First]->isSet) {
                handler->updateDataAndDrawToPosition(prevCursorPosition);

                handler->setState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

/*template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::addConstraints() {
    //none
}*/


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerOffset_H
