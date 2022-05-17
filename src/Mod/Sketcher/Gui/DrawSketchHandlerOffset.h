/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class DrawSketchHandlerOffset;

using DrawSketchHandlerOffsetBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerOffset,
    StateMachines::OneSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*WidgetParametersT =*/WidgetParameters<1>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<2>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1>>;

class DrawSketchHandlerOffset : public DrawSketchHandlerOffsetBase
{
    friend DrawSketchHandlerOffsetBase;

public:
    DrawSketchHandlerOffset(std::vector<int> listOfGeoIds)
        : snapMode(SnapMode::Free)
        , listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , offsetLengthSet(false)
        , offsetConstraint(false)
        , offsetLength(1) {}

    virtual ~DrawSketchHandlerOffset() = default;

    enum class SnapMode {
        Free,
        Snap
    };

    enum class JoinMode {
        Arc,
        Tangent,
        Intersection
    };

    enum class ModeEnums {
        Skin,
        Pipe,
        RectoVerso
    };

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            endpoint = onSketchPos;
            if (snapMode == SnapMode::Snap) {
                getSnapPoint(endpoint);
            }

            if (!offsetLengthSet) {
                findOffsetLength();
            }

            SbString text;
            text.sprintf(" (%.1f)", offsetLength);
            setPositionText(endpoint, text);

            //generate the copies
            if (fabs(offsetLength) > Precision::Confusion()) {
                makeOffset(static_cast<int>(joinMode), false, /*MakeGeos*/false);
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {

        if (fabs(offsetLength) > Precision::Confusion()) {
            makeOffset(static_cast<int>(joinMode), false, /*MakeGeos*/true);
        }

        sketchgui->getSketchObject()->solve(true);
        sketchgui->draw(false, false); // Redraw

        sketchgui->purgeHandler();
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Offset";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Offset");
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        firstCurveCreated = getHighestCurveIndex() + 1;

        vCC = generatevCC(listOfGeoIds);
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

    SnapMode snapMode;
    JoinMode joinMode;
    std::vector<int> listOfGeoIds;
    std::vector<int> listOfOffsetGeoIds;
    std::vector<std::vector<int>> vCC;
    std::vector<std::vector<int>> vCCO;
    Base::Vector2d endpoint;
    std::vector<TopoDS_Wire> sourceWires;

    bool deleteOriginal, offsetLengthSet, offsetConstraint;
    double offsetLength;
    int firstCurveCreated;

    void makeOffset(short joinType, bool allowOpenResult, bool onReleaseButton) {
        //make offset shape using BRepOffsetAPI_MakeOffset
        TopoDS_Shape offsetShape;
        Part::BRepOffsetAPI_MakeOffsetFix mkOffset(GeomAbs_JoinType(joinType), allowOpenResult);
        for (TopoDS_Wire& w : sourceWires) {
            mkOffset.AddWire(w);
        }
        try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
            Base::SignalException se;
#endif
            mkOffset.Perform(offsetLength);
        }
        catch (Standard_Failure&) {
            throw;
        }
        catch (...) {
            throw Base::CADKernelError("BRepOffsetAPI_MakeOffset has crashed! (Unknown exception caught)");
        }
        offsetShape = mkOffset.Shape();

        if (offsetShape.IsNull())
            throw Base::CADKernelError("makeOffset2D: result of offsetting is null!");

        //Copying shape to fix strange orientation behavior, OCC7.0.0. See bug #2699
        // http://www.freecadweb.org/tracker/view.php?id=2699
        offsetShape = BRepBuilderAPI_Copy(offsetShape).Shape();


        //turn wires/edges of shape into Geometries.
        std::vector<Part::Geometry*> geometriesToAdd;
        listOfOffsetGeoIds.clear();
        TopExp_Explorer expl(offsetShape, TopAbs_EDGE);
        int geoIdToAdd = firstCurveCreated;
        for (; expl.More(); expl.Next(), geoIdToAdd++) {

            const TopoDS_Edge& edge = TopoDS::Edge(expl.Current());
            BRepAdaptor_Curve curve(edge);
            if (curve.GetType() == GeomAbs_Line) {
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
                Part::GeomLineSegment* line = new Part::GeomLineSegment();
                line->setPoints(p1, p2);
                Sketcher::GeometryFacade::setConstruction(line, false);
                geometriesToAdd.push_back(line);
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            else if (curve.GetType() == GeomAbs_Circle) {
                gp_Circ circle = curve.Circle();
                gp_Pnt cnt = circle.Location();
                gp_Pnt beg = curve.Value(curve.FirstParameter());
                gp_Pnt end = curve.Value(curve.LastParameter());

                if (beg.SquareDistance(end) < Precision::Confusion()) {
                    Part::GeomCircle* gCircle = new Part::GeomCircle();
                    gCircle->setRadius(circle.Radius());
                    gCircle->setCenter(Base::Vector3d(cnt.X(), cnt.Y(), cnt.Z()));

                    Sketcher::GeometryFacade::setConstruction(gCircle, false);
                    geometriesToAdd.push_back(gCircle);
                }
                else {
                    Part::GeomArcOfCircle* gArc = new Part::GeomArcOfCircle();
                    Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
                    Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(hCircle, curve.FirstParameter(),
                        curve.LastParameter());
                    gArc->setHandle(tCurve);
                    Sketcher::GeometryFacade::setConstruction(gArc, false);
                    geometriesToAdd.push_back(gArc);
                }
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            else if (curve.GetType() == GeomAbs_Ellipse) {

                Base::Console().Warning("hello ellipse\n");
                gp_Elips ellipse = curve.Ellipse();
                //gp_Pnt cnt = ellipse.Location();
                gp_Pnt beg = curve.Value(curve.FirstParameter());
                gp_Pnt end = curve.Value(curve.LastParameter());

                if (beg.SquareDistance(end) < Precision::Confusion()) {
                    Part::GeomEllipse* gEllipse = new Part::GeomEllipse();
                    Handle(Geom_Ellipse) hEllipse = new Geom_Ellipse(ellipse);
                    gEllipse->setHandle(hEllipse);
                    Sketcher::GeometryFacade::setConstruction(gEllipse, false);
                    geometriesToAdd.push_back(gEllipse);
                }
                else {
                    Base::Console().Warning("hello arc ellipse\n");
                    Handle(Geom_Curve) hEllipse = new Geom_Ellipse(ellipse);
                    Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(hEllipse, curve.FirstParameter(), curve.LastParameter());
                    Part::GeomArcOfEllipse* gArc = new Part::GeomArcOfEllipse();
                    gArc->setHandle(tCurve);
                    Sketcher::GeometryFacade::setConstruction(gArc, false);
                    geometriesToAdd.push_back(gArc);
                }
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }

        }


        if (!onReleaseButton) {
            //Draw geos
            drawEdit(geometriesToAdd);
        }
        else {
            Base::Console().Warning("hello onrelease \n");
            //Create geos
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Offset"));
            Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
            Obj->addGeometry(std::move(geometriesToAdd));

            //Create constraints
            std::stringstream stream;
            stream << "conList = []\n";
            for (size_t i = 0; i < listOfOffsetGeoIds.size() - 1; i++) {
                Base::Console().Warning("create constraint i : %d\n", i);
                for (size_t j = i + 1; j < listOfOffsetGeoIds.size(); j++) {
                    //There's a bug with created arcs. They end points seems to swap at some point... Here we make coincidences based on lengths. So they must change after.
                    //here we check for coincidence on all geometries. It's far from ideal. We should check only the geometries that were inside a wire next to each other.
                    Base::Vector3d firstStartPoint, firstEndPoint, secondStartPoint, secondEndPoint;
                    if (getFirstSecondPoints(listOfOffsetGeoIds[i], firstStartPoint, firstEndPoint) && getFirstSecondPoints(listOfOffsetGeoIds[j], secondStartPoint, secondEndPoint)) {
                        bool create = false;
                        int posi = 1;
                        int posj = 1;

                        if ((firstStartPoint - secondStartPoint).Length() < Precision::Confusion()) {
                            create = true;
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
                            bool tangent = needTangent(listOfOffsetGeoIds[i], listOfOffsetGeoIds[j], posi, posj);
                            if (tangent) {
                                stream << "conList.append(Sketcher.Constraint('Tangent'," << listOfOffsetGeoIds[i] << "," << posi << ", " << listOfOffsetGeoIds[j] << "," << posj << "))\n";
                            }
                            else {
                                stream << "conList.append(Sketcher.Constraint('Coincident'," << listOfOffsetGeoIds[i] << "," << posi << ", " << listOfOffsetGeoIds[j] << "," << posj << "))\n";
                            }
                        }
                    }
                }
            }

            Base::Console().Warning("after create constrain for \n");
            stream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
            stream << "del conList\n";
            Gui::Command::doCommand(Gui::Command::Doc, stream.str().c_str());
            //We have to doCommand here even if we makeOffsetConstraint later because we'll have to know if there're tangents.

            //Delete original geometries if necessary
            if (deleteOriginal) {

                std::stringstream stream;
                for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
                    stream << listOfGeoIds[j] << ",";
                }
                stream << listOfGeoIds[listOfGeoIds.size() - 1];
                try {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                }
            }
            else {
                Base::Console().Warning("hbefore makeoffsetconstr \n");
                std::stringstream stream2;
                stream2 << "conList = []\n";
                if (offsetConstraint) {
                    vCCO = generatevCC(listOfOffsetGeoIds);
                    makeOffsetConstraint(stream2);
                }
                stream2 << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
                stream2 << "del conList\n";
                Gui::Command::doCommand(Gui::Command::Doc, stream2.str().c_str());
            }

            Gui::Command::commitCommand();
        }
    }

    bool needTangent(int geoId1, int geoId2, int pos1, int pos2) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        const Part::Geometry* geo1 = Obj->getGeometry(geoId1);
        const Part::Geometry* geo2 = Obj->getGeometry(geoId2);

        if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() || geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            Base::Vector3d perpendicular1, perpendicular2, p1, p2;
            if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                || geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                const Part::GeomArcOfConic* arcOfCircle = static_cast<const Part::GeomArcOfConic*>(geo1);
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
            else { return false; }
            //Todo: add cases for arcOfellipse parabolas hyperbolas bspline
            if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                || geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                const Part::GeomArcOfConic* arcOfCircle = static_cast<const Part::GeomArcOfConic*>(geo2);
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
            else { return false; }
            //Todo: add cases for arcOfellipse parabolas hyperbolas bspline

            //if lines are parallel
            if ((perpendicular1 % perpendicular2).Length() < Precision::Intersection()) {
                return true;
            }
        }
        return false;
    }

    void makeOffsetConstraint(std::stringstream& stream) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        int newCurveCounter = 0;
        int prevCurveCounter = 0;
        std::vector<Part::Geometry*> geometriesToAdd;
        for (size_t i = 0; i < vCCO.size(); i++) {
            //Check if curve is closed. Note as we use pipe it should always be closed but in case we enable 'Skin' in the future.
            bool isCurveClosed = false;
            if (vCCO[i].size() > 2) {
                CoincidencePointPos cpp = checkForCoincidence(vCCO[i][0], vCCO[i][vCCO[i].size() - 1]);
                if (cpp.FirstGeoPos != Sketcher::PointPos::none)
                    isCurveClosed = true;
            }
            else if (vCCO[i].size() == 2) {
                //if only 2 elements, we need to check that they don't close end to end.
                CoincidencePointPos cpp = checkForCoincidence(vCCO[i][0], vCCO[i][vCCO[i].size() - 1]);
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

                //Tangent constraint is constraining the offset already. So if there are tangents we should not create the construction lines. Hence the code below.
                bool createLine = true;
                bool forceCreate = false;
                if (!inTangentGroup && (!isCurveClosed || j != 0 || reRunForFirst)) {
                    createLine = true;
                    atLeastOneLine = true;
                }
                else { //include case of j == 0 and closed curve, because if required the line will be made after last.
                    createLine = false;
                }

                if (j + 1 < vCCO[i].size()) {
                    CoincidencePointPos ppc = checkForCoincidence(vCCO[i][j], vCCO[i][j + 1], true);//true is tangentOnly
                    if (ppc.FirstGeoPos != Sketcher::PointPos::none) {
                        inTangentGroup = true;
                    }
                    else {
                        inTangentGroup = false;
                    }
                }
                else if (j == vCCO[i].size() - 1 && isCurveClosed) {//Case of last geoId for closed curves.
                    CoincidencePointPos ppc = checkForCoincidence(vCCO[i][j], vCCO[i][0], true);
                    if (ppc.FirstGeoPos != Sketcher::PointPos::none) {
                        if (!atLeastOneLine) { //We need at least one line
                            createLine = true;
                            forceCreate = true;
                        }
                    }
                    else {
                        //create line for j = 0. For this we rerun the for at j=0 after this run. With an escape bool.
                        reRunForFirst = true;
                        inTangentGroup = false;
                    }
                }

                Base::Console().Warning("i-j : %d-%d / vCCO[i][j] : %d / / createLine %d / reRunForFirst %d\n", i, j, vCCO[i][j], createLine, reRunForFirst);
                const Part::Geometry* geo = Obj->getGeometry(vCCO[i][j]);
                for (size_t k = 0; k < listOfGeoIds.size(); k++) {
                    //Check if listOfGeoIds[k] is the offsetted curve giving curve i-j.
                    const Part::Geometry* geo2 = Obj->getGeometry(listOfGeoIds[k]);

                    if (geo->getTypeId() == Part::GeomCircle::getClassTypeId() && geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                        const Part::GeomCircle* circle = static_cast<const Part::GeomCircle*>(geo);
                        const Part::GeomCircle* circle2 = static_cast<const Part::GeomCircle*>(geo2);
                        Base::Vector3d p1 = circle->getCenter();
                        Base::Vector3d p2 = circle2->getCenter();
                        if ((p1 - p2).Length() < Precision::Confusion()) {
                            //coincidence of center
                            stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ",3))\n";
                            //Create line between both circles.
                            Part::GeomLineSegment* line = new Part::GeomLineSegment();
                            p1.x = p1.x + circle->getRadius();
                            p2.x = p2.x + circle2->getRadius();
                            line->setPoints(p1, p2);
                            Sketcher::GeometryFacade::setConstruction(line, true);
                            geometriesToAdd.push_back(line);
                            newCurveCounter++;
                            stream << "conList.append(Sketcher.Constraint('Perpendicular'," << getHighestCurveIndex() + newCurveCounter << ", " << vCCO[i][j] << "))\n";
                            stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",1, " << vCCO[i][j] << "))\n";
                            stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",2, " << listOfGeoIds[k] << "))\n";
                            break;
                        }
                    }
                    else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId() && geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                        //same as circle but 2 lines
                    }
                    else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId() && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        const Part::GeomLineSegment* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo);
                        const Part::GeomLineSegment* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);
                        Base::Vector3d p1[2], p2[2];
                        p1[0] = lineSeg1->getStartPoint();
                        p1[1] = lineSeg1->getEndPoint();
                        p2[0] = lineSeg2->getStartPoint();
                        p2[1] = lineSeg2->getEndPoint();
                        //if lines are parallel
                        if (((p1[1] - p1[0]) % (p2[1] - p2[0])).Length() < Precision::Intersection()) {
                            //If the lines are space by offsetLength distance
                            Base::Vector3d projectedP;
                            projectedP.ProjectToLine(p1[0] - p2[0], p2[1] - p2[0]);

                            if ((projectedP).Length() - fabs(offsetLength) < Precision::Confusion()) {
                                if (!forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Parallel'," << vCCO[i][j] << ", " << listOfGeoIds[k] << "))\n";
                                }

                                //We don't need a construction line if the line has a tangent at one end. Unless it's the first line that we're making.
                                if (createLine) {
                                    Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                    line->setPoints(p1[0], p1[0] + projectedP);
                                    Sketcher::GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;

                                    stream << "conList.append(Sketcher.Constraint('Perpendicular'," << getHighestCurveIndex() + newCurveCounter << ", " << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",1, " << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",2, " << listOfGeoIds[k] << "))\n";
                                }
                                break;
                            }
                        }
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                        //multiple cases because arc join mode creates arcs or circle.
                        const Part::GeomArcOfCircle* arcOfCircle = static_cast<const Part::GeomArcOfCircle*>(geo);
                        Base::Vector3d p1 = arcOfCircle->getCenter();

                        if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arcOfCircle2 = static_cast<const Part::GeomArcOfCircle*>(geo2);
                            Base::Vector3d p2 = arcOfCircle2->getCenter();
                            Base::Vector3d p3 = arcOfCircle2->getStartPoint();
                            Base::Vector3d p4 = arcOfCircle2->getEndPoint();

                            if ((p1 - p2).Length() < Precision::Confusion()) {
                                //coincidence of center. Offset arc is the offset of an arc
                                stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ",3))\n";
                                if (createLine) {
                                    //Create line between both circles.
                                    Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                    p1.x = p1.x + arcOfCircle->getRadius();
                                    p2.x = p2.x + arcOfCircle2->getRadius();
                                    line->setPoints(p1, p2);
                                    Sketcher::GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;
                                    stream << "conList.append(Sketcher.Constraint('Perpendicular'," << getHighestCurveIndex() + newCurveCounter << ", " << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",1, " << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",2, " << listOfGeoIds[k] << "))\n";
                                }
                                break;
                            }
                            else if ((p1 - p3).Length() < Precision::Confusion()) {
                                //coincidence of center to startpoint. offset arc is created arc join
                                stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 1))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius'," << vCCO[i][j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }
                            else if ((p1 - p4).Length() < Precision::Confusion()) {
                                //coincidence of center to startpoint
                                stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 2))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius'," << vCCO[i][j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }

                        }
                        else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                            || geo2->getTypeId() == Part::GeomArcOfConic::getClassTypeId()
                            || geo2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                            //cases where arc is created by arc join mode.
                            Base::Vector3d p2, p3;

                            if (getFirstSecondPoints(listOfGeoIds[k], p2, p3)) {
                                if (((p1 - p2).Length() < Precision::Confusion()) || ((p1 - p3).Length() < Precision::Confusion())) {
                                    if ((p1 - p2).Length() < Precision::Confusion()) {
                                        //coincidence of center to startpoint
                                        stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 1))\n";
                                    }
                                    else if ((p1 - p3).Length() < Precision::Confusion()) {
                                        //coincidence of center to endpoint
                                        stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 2))\n";
                                    }
                                    break;
                                }
                            }

                        }

                    }
                    else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() && geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                        //const Part::GeomArcOfEllipse* arcOfEllipse = static_cast<const Part::GeomArcOfEllipse*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() && geo2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                        //const Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<const Part::GeomArcOfHyperbola*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() && geo2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                        //const Part::GeomArcOfParabola* arcOfParabola = static_cast<const Part::GeomArcOfParabola*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() && geo2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                    }

                }
                if (newCurveCounter != prevCurveCounter) {
                    prevCurveCounter = newCurveCounter;
                    if (newCurveCounter != 1) {
                        stream << "conList.append(Sketcher.Constraint('Equal'," << getHighestCurveIndex() + newCurveCounter << ", " << getHighestCurveIndex() + 1 << "))\n";
                    }
                }

                if (reRunForFirst) {
                    if (j != 0) {
                        j = -1;
                    }// j will be incremented to 0 after new loop
                    else {
                        break;
                    }
                }
            }
        }
        if (newCurveCounter != 0) {
            stream << "conList.append(Sketcher.Constraint('Distance'," << getHighestCurveIndex() + 1 << ", " << offsetLength << "))\n";
        }
        Obj->addGeometry(std::move(geometriesToAdd));
    }

    std::vector<std::vector<int>> generatevCC(std::vector<int>& listOfGeo) {
        //This function separates all the selected geometries into separate continuous curves.
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
                //Base::Console().Warning("Inserting : %d\n", listOfGeo[i]);
                for (size_t j = 0; j < vcc.size(); j++) {
                    //Base::Console().Warning("curve : %d\n", j);
                    for (size_t k = 0; k < vcc[j].size(); k++) {
                        //Base::Console().Warning("edge : %d ", vcc[j][k]);
                        CoincidencePointPos pointPosOfCoincidence = checkForCoincidence(listOfGeo[i], vcc[j][k]);
                        if (pointPosOfCoincidence.FirstGeoPos != Sketcher::PointPos::none) {
                            if (inserted && insertedIn != int(j)) {
                                //if it's already inserted in another continuous curve then we need to merge both curves together.
                                //There're 2 cases, it could have been inserted at the end or at the beginning.
                                if (vcc[insertedIn][0] == listOfGeo[i]) {
                                    //Two cases. Either the coincident is at the beginning or at the end.
                                    if (k == 0) {
                                        std::reverse(vcc[j].begin(), vcc[j].end());
                                    }
                                    vcc[j].insert(vcc[j].end(), vcc[insertedIn].begin(), vcc[insertedIn].end());
                                    vcc.erase(vcc.begin() + insertedIn);
                                }
                                else {
                                    if (k != 0) { //ie k is  vcc[j].size()-1
                                        std::reverse(vcc[j].begin(), vcc[j].end());
                                    }
                                    vcc[insertedIn].insert(vcc[insertedIn].end(), vcc[j].begin(), vcc[j].end());
                                    vcc.erase(vcc.begin() + j);
                                }
                                j--;
                                //Base::Console().Warning("Removing vector : %d ", j);
                            }
                            else {
                                //we need to get the curves in the correct order.
                                if (k == vcc[j].size() - 1) {
                                    vcc[j].push_back(listOfGeo[i]);
                                    //Base::Console().Warning("inserted at the end in : %d ", j);
                                }
                                else {
                                    //in this case k should actually be 0.
                                    vcc[j].insert(vcc[j].begin() + k, listOfGeo[i]);
                                    //Base::Console().Warning("inserted after %d in : %d ", k, j);
                                }
                                insertedIn = j;
                                inserted = 1;
                            }
                            //Base::Console().Warning("\n");
                            //printCCeVec();
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
        Base::Console().Warning("vcc.size() : %d\n", vcc.size());
        return vcc;
    }

    void generateSourceWires() {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        for (size_t i = 0; i < vCC.size(); i++) {
            BRepBuilderAPI_MakeWire mkWire;
            for (size_t j = 0; j < vCC[i].size(); j++) {
                Base::Console().Warning("j %d\n", j);
                mkWire.Add(TopoDS::Edge(Obj->getGeometry(vCC[i][j])->toShape()));
            }
            if (BRep_Tool::IsClosed(mkWire.Wire())) {
                Base::Console().Warning("wire %d closed\n", i);
            }
            sourceWires.push_back(mkWire.Wire());
        }
    }

    void findOffsetLength() {
        double newOffsetLength = 1000000000000;
        BRepBuilderAPI_MakeVertex mkVertex({ endpoint.x, endpoint.y, 0.0 });
        TopoDS_Vertex vertex = mkVertex.Vertex();
        for (size_t i = 0; i < sourceWires.size(); i++) {
            BRepExtrema_DistShapeShape distTool(sourceWires[i], vertex);
            if (distTool.IsDone()) {
                double distance = distTool.Value();
                if (distance == std::min(distance, newOffsetLength)) {
                    newOffsetLength = distance;

                    //find direction
                    if (BRep_Tool::IsClosed(sourceWires[i])) {
                        TopoDS_Face aFace = BRepBuilderAPI_MakeFace(sourceWires[i]);
                        BRepClass_FaceClassifier checkPoint(aFace, { endpoint.x, endpoint.y, 0.0 }, Precision::Confusion());
                        if (checkPoint.State() == TopAbs_IN)
                            newOffsetLength = -newOffsetLength;
                    }
                }
            }
        }

        if (newOffsetLength != 1000000000000) {
            offsetLength = newOffsetLength;
        }
    }

    bool getSnapPoint(Base::Vector2d& snapPoint) {
        int pointGeoId = Sketcher::GeoEnum::GeoUndef;
        Sketcher::PointPos pointPosId = Sketcher::PointPos::none;
        int VtId = getPreselectPoint();
        int CrsId = getPreselectCross();
        if (CrsId == 0) {
            pointGeoId = Sketcher::GeoEnum::RtPnt;
            pointPosId = Sketcher::PointPos::start;
        }
        else if (VtId >= 0) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, pointGeoId, pointPosId);
        }
        if (pointGeoId != Sketcher::GeoEnum::GeoUndef && pointGeoId < firstCurveCreated) {
            //don't want to snap to the point of a geometry which is being previewed!
            auto sk = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());
            snapPoint.x = sk->getPoint(pointGeoId, pointPosId).x;
            snapPoint.y = sk->getPoint(pointGeoId, pointPosId).y;
            return true;
        }
        return false;
    }

    bool getFirstSecondPoints(int geoId, Base::Vector3d& startPoint, Base::Vector3d& endPoint) {
        const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(geoId);

        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo);
            startPoint = line->getStartPoint();
            endPoint = line->getEndPoint();
            return true;
        }
        else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
            || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
            || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
            || geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            const Part::GeomArcOfConic* arcOfConic = static_cast<const Part::GeomArcOfConic*>(geo);
            startPoint = arcOfConic->getStartPoint();
            endPoint = arcOfConic->getEndPoint();
            return true;
        }
        else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            const Part::GeomBSplineCurve* bSpline = static_cast<const Part::GeomBSplineCurve*>(geo);
            startPoint = bSpline->getStartPoint();
            endPoint = bSpline->getEndPoint();
            return true;
        }
        return false;
    }

    CoincidencePointPos checkForCoincidence(int GeoId1, int GeoId2, bool tangentOnly = false) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        const std::vector< Sketcher::Constraint* >& vals = Obj->Constraints.getValues();
        CoincidencePointPos positions;
        positions.FirstGeoPos = Sketcher::PointPos::none;
        positions.SecondGeoPos = Sketcher::PointPos::none;
        positions.SecondCoincidenceFirstGeoPos = Sketcher::PointPos::none;
        positions.SecondCoincidenceSecondGeoPos = Sketcher::PointPos::none;
        bool firstCoincidenceFound = 0;
        for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin(); it != vals.end(); ++it) {
            if ((!tangentOnly && (*it)->Type == Sketcher::Coincident) || (*it)->Type == Sketcher::Tangent) {
                if ((*it)->First == GeoId1 && (*it)->FirstPos != Sketcher::PointPos::mid && (*it)->FirstPos != Sketcher::PointPos::none
                    && (*it)->Second == GeoId2 && (*it)->SecondPos != Sketcher::PointPos::mid && (*it)->SecondPos != Sketcher::PointPos::none) {
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
                else if ((*it)->First == GeoId2 && (*it)->FirstPos != Sketcher::PointPos::mid && (*it)->FirstPos != Sketcher::PointPos::none
                    && (*it)->Second == GeoId1 && (*it)->SecondPos != Sketcher::PointPos::mid && (*it)->SecondPos != Sketcher::PointPos::none) {
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

    Base::Vector2d vec3dTo2d(Base::Vector3d pointToProcess) {
        Base::Vector2d pointToReturn;
        pointToReturn.x = pointToProcess.x;
        pointToReturn.y = pointToProcess.y;
        return pointToReturn;
    }

    //debug only
    void printCCeVec() {
        for (size_t j = 0; j < vCC.size(); j++) {
            Base::Console().Warning("curve %d{", j);
            for (size_t k = 0; k < vCC[j].size(); k++) {
                Base::Console().Warning("%d, ", vCC[j][k]);
            }
            Base::Console().Warning("}\n");
        }
    }
};

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Arc"), QStringLiteral("Intersection")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_offset", "Offset length"));

    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_offset", "Delete original geometries"));
    toolWidget->setCheckboxLabel(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_offset", "Add offset constraint"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("Offset_1", "Positive offset length is outward, negative inward."));
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->offsetLengthSet = true;
        dHandler->offsetLength = value;
        break;
    }
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    switch (checkboxindex) {
    case WCheckbox::FirstBox:
        dHandler->deleteOriginal = value;
        break;
    case WCheckbox::SecondBox:
        dHandler->offsetConstraint = value;
        break;
    }
    handler->updateDataAndDrawToPosition(prevCursorPosition);
    onHandlerModeChanged(); //re-focus/select spinbox
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptDrawingToComboboxChange(int comboboxindex, int value) {
    if (comboboxindex == WCombobox::FirstCombo) {
        this->setMode(dHandler->joinMode, value);
    }
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {
    //Too hard to override onsketchpos such that it is at offsetLength from the curve. So we use offsetLengthSet to prevent rewrite of offsetLength.

    lastWidgetEnforcedPosition = onSketchPos;
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    Q_UNUSED(onSketchPos)

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, dHandler->offsetLength);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::addConstraints() {
    //none
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerOffset_H

