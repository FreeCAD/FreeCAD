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


#ifndef SKETCHERGUI_DrawSketchHandlerPattern_H
#define SKETCHERGUI_DrawSketchHandlerPattern_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

#include <Mod/Sketcher/App/GeometryFacade.h>

namespace SketcherGui {

    extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

    class DrawSketchHandlerPattern;

    using DrawSketchHandlerPatternBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerPattern,
        StateMachines::ThreeSeekEnd,
        /*PEditCurveSize =*/ 0,
        /*PAutoConstraintSize =*/ 0,
        /*WidgetParametersT =*/WidgetParameters<6>,
        /*WidgetCheckboxesT =*/WidgetCheckboxes<2>,
        /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

    class DrawSketchHandlerPattern : public DrawSketchHandlerPatternBase
    {
        friend DrawSketchHandlerPatternBase;

    public:
        DrawSketchHandlerPattern(std::vector<int> listOfGeoIds)
            : snapMode(SnapMode::Free)
            , listOfGeoIds(listOfGeoIds)
            , closeOutside(true)
            , allConstr(false)
            , radius(1.)
            , angle(0.)
            , maxNumberOfHex(2000) {}

        virtual ~DrawSketchHandlerPattern() = default;

        enum class SnapMode {
            Free,
            Snap
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
                drawPositionAtCursor(onSketchPos);
                if (IsPointInWire(onSketchPos)) {
                    startPoint = onSketchPos;
                    if (snapMode == SnapMode::Snap) {
                        getSnapPoint(startPoint);
                    }
                }
            }
            break;
            case SelectMode::SeekSecond:
            {
                endpoint = onSketchPos;

                angle = GetPointAngle(startPoint, endpoint);
                radius = (endpoint - startPoint).Length();
                thickness = radius / 5;

                if (snapMode == SnapMode::Snap) {
                    angle = round(angle / (M_PI / 36)) * M_PI / 36;
                    endpoint = startPoint + radius * Base::Vector2d(cos(angle), sin(angle));
                }

                SbString text;
                text.sprintf(" (%.1f, %.1f)", radius, angle);
                setPositionText(endpoint, text);

                try {
                    CreateAndDrawShapeGeometry();
                }
                catch (const Base::ValueError&) {}
            }
            break;
            case SelectMode::SeekThird:
            {
                thickness = getDistanceToFirstHex(onSketchPos);

                SbString text;
                text.sprintf(" (%.1f)", thickness);
                setPositionText(endpoint, text);

                try {
                    CreateAndDrawShapeGeometry();
                }
                catch (const Base::ValueError&) {}
            }
            break;
            default:
                break;
            }
        }

        virtual void executeCommands() override {

            try {
                firstCurve = getHighestCurveIndex() + 1;

                createShape(false);

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Honeycomb Pattern"));

                commandAddShapeGeometryAndConstraints();

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Failed to add Honeycomb Pattern: %s\n", e.what());
                Gui::Command::abortCommand();
                THROWM(Base::RuntimeError, "Tool execution aborted\n") // This prevents constraints from being applied on non existing geometry
            }

            thickness = 0.;
        }

        virtual void createAutoConstraints() override {
            //none
        }

        virtual std::string getToolName() const override {
            return "DSH_Pattern";
        }

        virtual QString getCrosshairCursorSVGName() const override {
            return QString::fromLatin1("Sketcher_Pattern");
        }

        virtual void activated() override
        {
            DrawSketchDefaultHandler::activated();

            vCC = generatevCC(listOfGeoIds);
            if (!generateSourceWire()) {
                Base::Console().Warning("All wires open!\n");
                sketchgui->purgeHandler();
            }
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
        int maxNumberOfHex;
        std::vector<int> listOfGeoIds;
        std::vector<int> listOfPatternGeoIds;
        std::vector<std::vector<int>> vCC;
        Base::Vector2d startPoint, endpoint;
        TopoDS_Wire sourceWire;
        std::vector<Base::Vector2d> firstHexPoints;

        bool closeOutside, allConstr;
        double radius, angle, thickness, interCenterDistance;
        int firstCurve;

        virtual void createShape(bool onlyeditoutline) override {
            ShapeGeometry.clear();

            bool outAfterWholeTurn = false;
            double dirAngle = angle + M_PI / 6;
            int i = 0;
            int j = 1;
            int numberOfTurnSinceLastHex = 0;
            bool turn120 = true;
            bool incrementj = false;
            Base::Vector2d hexCenter = startPoint;
            Base::Vector2d dirVec = Base::Vector2d(cos(dirAngle), sin(dirAngle));
            interCenterDistance = thickness + radius * cos(M_PI / 6) * 2;

            int curveCounter = firstCurve;

            int securityCoutner = 0;
            int maxCounter = 500; //TODO: 500 is the preview max number of hex. Make it a pref setting.
            if (!onlyeditoutline) {
                maxCounter = maxNumberOfHex;
                ShapeConstraints.clear();
            }

            while (!outAfterWholeTurn && securityCoutner < maxCounter) {
                securityCoutner++;
                bool hexCreated = false;

                //Create current hexagone
                std::vector<Base::Vector2d> hexPoints;
                for (int p = 0; p < 6; p++) {
                    hexPoints.push_back(hexCenter + radius * Base::Vector2d(cos(angle + p * M_PI / 3), sin(angle + p * M_PI / 3)));
                }

                //save the initial hexagone points for seekThird.
                if (securityCoutner == 1 && state() == SelectMode::SeekSecond)
                    firstHexPoints = hexPoints;

                //if center of the hex is at a distance greater than radius from the wire, then it should be build completely. Else we check each line one by one.
                double distanceWireToCenter = 0.;
                BRepBuilderAPI_MakeVertex mkVertex({ hexCenter.x, hexCenter.y, 0.0 });
                TopoDS_Vertex vertex = mkVertex.Vertex();
                BRepExtrema_DistShapeShape distTool(sourceWire, vertex);
                if (distTool.IsDone()) {
                    distanceWireToCenter = distTool.Value();
                }
                if (distanceWireToCenter > radius && IsPointInWire(hexCenter)) {
                    addLineToShapeGeometry(Base::Vector3d(hexPoints[0].x, hexPoints[0].y, 0.), Base::Vector3d(hexPoints[1].x, hexPoints[1].y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(hexPoints[1].x, hexPoints[1].y, 0.), Base::Vector3d(hexPoints[2].x, hexPoints[2].y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(hexPoints[2].x, hexPoints[2].y, 0.), Base::Vector3d(hexPoints[3].x, hexPoints[3].y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(hexPoints[3].x, hexPoints[3].y, 0.), Base::Vector3d(hexPoints[4].x, hexPoints[4].y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(hexPoints[4].x, hexPoints[4].y, 0.), Base::Vector3d(hexPoints[5].x, hexPoints[5].y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(hexPoints[5].x, hexPoints[5].y, 0.), Base::Vector3d(hexPoints[0].x, hexPoints[0].y, 0.), geometryCreationMode);
                    hexCreated = true;

                    //create constraints
                    if (!onlyeditoutline) {
                        addToShapeConstraints(Sketcher::Coincident, curveCounter    , Sketcher::PointPos::end, curveCounter + 1, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, curveCounter + 1, Sketcher::PointPos::end, curveCounter + 2, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, curveCounter + 2, Sketcher::PointPos::end, curveCounter + 3, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, curveCounter + 3, Sketcher::PointPos::end, curveCounter + 4, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, curveCounter + 4, Sketcher::PointPos::end, curveCounter + 5, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, curveCounter + 5, Sketcher::PointPos::end, curveCounter    , Sketcher::PointPos::start);

                        if (allConstr) {
                            addToShapeConstraints(Sketcher::Equal, curveCounter, Sketcher::PointPos::none, curveCounter + 1);
                            addToShapeConstraints(Sketcher::Equal, curveCounter + 1, Sketcher::PointPos::none, curveCounter + 2);
                            addToShapeConstraints(Sketcher::Equal, curveCounter + 2, Sketcher::PointPos::none, curveCounter + 3);
                            addToShapeConstraints(Sketcher::Equal, curveCounter + 3, Sketcher::PointPos::none, curveCounter + 4);
                            addToShapeConstraints(Sketcher::Equal, curveCounter + 4, Sketcher::PointPos::none, curveCounter + 5);
                            addToShapeConstraints(Sketcher::Equal, curveCounter + 5, Sketcher::PointPos::none, curveCounter);
                            if (curveCounter > firstCurve + 5) {
                                addToShapeConstraints(Sketcher::Equal, curveCounter, Sketcher::PointPos::none, firstCurve);
                            }
                            addToShapeConstraints(Sketcher::Parallel, curveCounter    , Sketcher::PointPos::none, curveCounter + 3);
                        }
                    }
                    curveCounter = curveCounter + 6;
                }
                else {
                    //check for each line of the hex if it is in the closed wire.
                    bool connectTofirst = false;
                    bool connectToNext = false;
                    int firstLineId = Sketcher::GeoEnum::GeoUndef;

                    for (int l = 0; l < 6; l++) {
                        int lplus1 = l + 1;
                        if (lplus1 == 6)
                            lplus1 = 0;
                        bool lInWire = IsPointInWire(hexPoints[l]); 
                        bool lplus1InWire = IsPointInWire(hexPoints[lplus1]);

                        if (lInWire && lplus1InWire) {
                            addLineToShapeGeometry(Base::Vector3d(hexPoints[l].x, hexPoints[l].y, 0.), Base::Vector3d(hexPoints[lplus1].x, hexPoints[lplus1].y, 0.), geometryCreationMode);
                            hexCreated = true;
                            
                            if(!onlyeditoutline){
                                if (l == 0) {
                                    firstLineId = curveCounter;
                                    connectTofirst = true;
                                }

                                if (connectToNext) { //if connectToNext is true at this point, it has been set true by previous line
                                    addToShapeConstraints(Sketcher::Coincident, curveCounter - 1, Sketcher::PointPos::end, curveCounter, Sketcher::PointPos::start);
                                }

                                if (l == 5 && connectTofirst) {
                                    addToShapeConstraints(Sketcher::Coincident, curveCounter, Sketcher::PointPos::end, firstLineId, Sketcher::PointPos::start);
                                }

                                if (allConstr && curveCounter > firstCurve) {
                                    addToShapeConstraints(Sketcher::Equal, curveCounter, Sketcher::PointPos::none, firstCurve);
                                }
                                curveCounter++;
                                connectToNext = true;
                            }
                        }
                        else if (lInWire || lplus1InWire) {
                            TopoDS_Edge aEdge = BRepBuilderAPI_MakeEdge({ hexPoints[l].x, hexPoints[l].y, 0.0 }, { hexPoints[lplus1].x, hexPoints[lplus1].y, 0.0 });

                            BRepExtrema_DistShapeShape distTool(sourceWire, aEdge);
                            distTool.Perform();
                            if (distTool.IsDone() && distTool.NbSolution() > 0) {
                                const gp_Pnt& intersectionP = distTool.PointOnShape1(1);
                                if(lInWire)
                                    addLineToShapeGeometry(Base::Vector3d(hexPoints[l].x, hexPoints[l].y, 0.), Base::Vector3d(intersectionP.X(), intersectionP.Y(), 0.), geometryCreationMode);
                                else
                                    addLineToShapeGeometry(Base::Vector3d(intersectionP.X(), intersectionP.Y(), 0.), Base::Vector3d(hexPoints[lplus1].x, hexPoints[lplus1].y, 0.), geometryCreationMode);
                            }
                            hexCreated = true;

                            if (!onlyeditoutline) {
                                if (l == 0) {
                                    firstLineId = curveCounter;
                                    if (lInWire)
                                        connectTofirst = true;
                                }
                                if (connectToNext) { //if connectToNext is true at this point, it has been set true by previous line
                                    addToShapeConstraints(Sketcher::Coincident, curveCounter - 1, Sketcher::PointPos::end, curveCounter, Sketcher::PointPos::start);
                                }

                                if (l == 5 && connectTofirst) {
                                    addToShapeConstraints(Sketcher::Coincident, curveCounter, Sketcher::PointPos::end, firstLineId, Sketcher::PointPos::start);
                                }
                                if (lplus1InWire)
                                    connectToNext = true;
                                else
                                    connectToNext = false;
                                curveCounter++;
                            }
                        }
                        else {
                            connectToNext = false;
                        }

                    }
                }

                //Find next hexCenter
                hexCenter = hexCenter + interCenterDistance * dirVec;
                i++;
                //we're building the hex by turning around the first hexagon. Below we find the correct direction
                if (i == j) {
                    i = 0;
                    //turn
                    if (turn120) {
                        dirAngle = dirAngle + 2 * M_PI / 3;
                        turn120 = false;
                    }
                    else{
                        dirAngle = dirAngle + M_PI / 3;
                        turn120 = true;
                    }
                    dirVec = Base::Vector2d(cos(dirAngle), sin(dirAngle));

                    if (incrementj) {
                        j++;
                        incrementj = false;
                    }
                    else {
                        incrementj = true;
                    }

                    //We will stop when after a full circle we have not built a single hexagon.
                    if (hexCreated) {
                        numberOfTurnSinceLastHex = 0;
                    }
                    else {
                        numberOfTurnSinceLastHex++;
                    }
                    if (numberOfTurnSinceLastHex > 5)
                        outAfterWholeTurn = true;
                }
            }

            //Now we need to trim the initial wire such that the end result is a closed shape that can be extruded/pocketed directly.
            if (!onlyeditoutline) {

            }
        }

        double getDistanceToFirstHex(Base::Vector2d onSketchPos) {
            double distanceToFirstHex = 1.;

            //We make a wire
            BRepBuilderAPI_MakeWire mkWire;
            for (size_t l = 0; l < firstHexPoints.size(); l++) {
                int lplus1 = l + 1;
                if (lplus1 == 6)
                    lplus1 = 0;

                TopoDS_Edge aEdge = BRepBuilderAPI_MakeEdge({ firstHexPoints[l].x, firstHexPoints[l].y, 0.0 }, { firstHexPoints[lplus1].x, firstHexPoints[lplus1].y, 0.0 });

                mkWire.Add(aEdge);
            }

            //Then we calculate distance to the wire
            BRepBuilderAPI_MakeVertex mkVertex({ onSketchPos.x, onSketchPos.y, 0.0 });
            TopoDS_Vertex vertex = mkVertex.Vertex();
            BRepExtrema_DistShapeShape distTool(mkWire.Wire(), vertex);
            if (distTool.IsDone()) {
                distanceToFirstHex = distTool.Value();
            }
            return distanceToFirstHex;
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

        bool generateSourceWire() {
            Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

            for (size_t i = 0; i < vCC.size(); i++) {
                BRepBuilderAPI_MakeWire mkWire;
                for (size_t j = 0; j < vCC[i].size(); j++) {
                    mkWire.Add(TopoDS::Edge(Obj->getGeometry(vCC[i][j])->toShape()));
                }
                if (BRep_Tool::IsClosed(mkWire.Wire())) {
                    sourceWire = mkWire.Wire();
                    return true;
                }
                else {
                    Base::Console().Warning("Wire %d open! Trying next if any...\n", i);
                }
            }
            return false;
        }

        bool IsPointInWire(Base::Vector2d pointToCheck) {
            if (BRep_Tool::IsClosed(sourceWire)) {
                TopoDS_Face aFace = BRepBuilderAPI_MakeFace(sourceWire);
                BRepClass_FaceClassifier checkPoint(aFace, { pointToCheck.x, pointToCheck.y, 0.0 }, Precision::Confusion());
                if (checkPoint.State() == TopAbs_IN)
                    return true;
            }
            return false;
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
            if (pointGeoId != Sketcher::GeoEnum::GeoUndef && pointGeoId < firstCurve) {
                //don't want to snap to the point of a geometry which is being previewed!
                auto sk = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());
                snapPoint.x = sk->getPoint(pointGeoId, pointPosId).x;
                snapPoint.y = sk->getPoint(pointGeoId, pointPosId).y;
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

    };

    template <> auto DrawSketchHandlerPatternBase::ToolWidgetManager::getState(int parameterindex) const {
        switch (parameterindex) {
        case WParameter::First:
        case WParameter::Second:
            return SelectMode::SeekFirst;
            break;
        case WParameter::Third:
        case WParameter::Fourth:
            return SelectMode::SeekSecond;
            break;
        case WParameter::Fifth:
        case WParameter::Sixth:
            return SelectMode::SeekThird;
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
        }
    }

    template <> void DrawSketchHandlerPatternBase::ToolWidgetManager::configureToolWidget() {
        if (!init) { // Code to be executed only upon initialisation
            QStringList names = { QStringLiteral("Arc"), QStringLiteral("Intersection") };
            toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
        }

        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_pattern", "x of first point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p1_pattern", "y  of first point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p1_pattern", "Radius"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p1_pattern", "Angle to HAxis"));
        toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p1_pattern", "Thickness"));
        toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("TaskSketcherTool_p1_pattern", "Max number of hex"));
        toolWidget->configureParameterUnit(WParameter::Sixth, Base::Unit());
        toolWidget->setParameter(WParameter::Sixth, dHandler->maxNumberOfHex); // unconditionally set

        toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_pattern", "Close the outside"));
        toolWidget->setCheckboxToolTip(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_pattern", "Trim such that the close wire is the outside of hexagones."));
        toolWidget->setCheckboxLabel(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_pattern", "Add all constraints"));
        toolWidget->setCheckboxToolTip(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_pattern", "Fully constrain the pattern. Can be very laggy if you have lot of hexagones."));

        toolWidget->setNoticeVisible(true);
        toolWidget->setNoticeText(QApplication::translate("Pattern_1", "If your shape does not fill completely, increase the 'Max number of hex'. By default it's 500 for preview and 2000 when creating the geometries (So even if it doesn't fill in preview it might fill when creating).\n Be carefull using 'Add all constraints' if you have a lot of hexagones as large numbers of constraints risk lag or crash."));
    }

    template <> void DrawSketchHandlerPatternBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
        switch (parameterindex) {
        case WParameter::First:
            dHandler->startPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->startPoint.y = value;
            break;
        case WParameter::Third:
            dHandler->radius = value;
            break;
        case WParameter::Fourth:
            dHandler->angle = value / 360 * M_PI;
            break;
        case WParameter::Fifth:
            dHandler->thickness = value;
            break;
        case WParameter::Sixth:
            dHandler->maxNumberOfHex = abs(floor(value));
            break;
        }
    }

    template <> void DrawSketchHandlerPatternBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
        switch (checkboxindex) {
        case WCheckbox::FirstBox:
            dHandler->closeOutside = value;
            break;
        case WCheckbox::SecondBox:
            dHandler->allConstr = value;
            break;
        }
        handler->updateDataAndDrawToPosition(prevCursorPosition);
        onHandlerModeChanged(); //re-focus/select spinbox
    }

    template <> void DrawSketchHandlerPatternBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {
        switch (handler->state()) {
        case SelectMode::SeekFirst:
        {
            if (toolWidget->isParameterSet(WParameter::First))
                onSketchPos.x = toolWidget->getParameter(WParameter::First);

            if (toolWidget->isParameterSet(WParameter::Second))
                onSketchPos.y = toolWidget->getParameter(WParameter::Second);
        }
        break;
        case SelectMode::SeekSecond:
        {

            if (toolWidget->isParameterSet(WParameter::Third)) {
                Base::Vector2d v = onSketchPos - dHandler->startPoint;
                onSketchPos = dHandler->startPoint + v * toolWidget->getParameter(WParameter::Third) / v.Length();
            }

            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double radius = (onSketchPos - dHandler->startPoint).Length();
                onSketchPos.x = dHandler->startPoint.x + cos(toolWidget->getParameter(WParameter::Fourth) * M_PI / 180) * radius;
                onSketchPos.y = dHandler->startPoint.y + sin(toolWidget->getParameter(WParameter::Fourth) * M_PI / 180) * radius;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {
                onSketchPos.x = dHandler->endpoint.x + cos(dHandler->angle + M_PI / 6) * dHandler->thickness;
                onSketchPos.y = dHandler->endpoint.y + sin(dHandler->angle + M_PI / 6) * dHandler->thickness;
            }

        }
        break;
        default:
            break;
        }
    }

    template <> void DrawSketchHandlerPatternBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {

        switch (handler->state()) {
        case SelectMode::SeekFirst:
        {
            if (!toolWidget->isParameterSet(WParameter::First))
                toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Second))
                toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
        }
        break;
        case SelectMode::SeekSecond:
        {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, dHandler->radius);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, dHandler->angle * 360 / M_PI);
        }
        break;
        case SelectMode::SeekThird:
        {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, dHandler->thickness);
        }
        break;
        default:
            break;
        }


    }

    template <> void DrawSketchHandlerPatternBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
        switch (handler->state()) {
        case SelectMode::SeekFirst:
        {
            if (toolWidget->isParameterSet(WParameter::First) &&
                toolWidget->isParameterSet(WParameter::Second)) {

                handler->setState(SelectMode::SeekSecond);
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            if (toolWidget->isParameterSet(WParameter::Third) ||
                toolWidget->isParameterSet(WParameter::Fourth)) {

                if (toolWidget->isParameterSet(WParameter::Third) &&
                    toolWidget->isParameterSet(WParameter::Fourth)) {

                    handler->setState(SelectMode::SeekThird);
                }
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {

                handler->setState(SelectMode::End);
            }
        }
        break;
        default:
            break;
        }

    }

    template <> void DrawSketchHandlerPatternBase::ToolWidgetManager::addConstraints() {
        //none
    }


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerPattern_H

