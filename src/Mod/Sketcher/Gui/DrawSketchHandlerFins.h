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


#ifndef SKETCHERGUI_DrawSketchHandlerFins_H
#define SKETCHERGUI_DrawSketchHandlerFins_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

    extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

    class DrawSketchHandlerFins;

    namespace ConstructionMethods {

        enum class FinsConstructionMethod {
            OneSide,
            TwoSide,
            End // Must be the last one
        };

    }

    using DrawSketchHandlerFinsBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerFins,
        StateMachines::ThreeSeekEnd,
        /*PEditCurveSize =*/ 0,
        /*PAutoConstraintSize =*/ 2,
        /*WidgetParametersT =*/WidgetParameters<7, 7>,
        /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
        /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
        ConstructionMethods::FinsConstructionMethod,
        /*bool PFirstComboboxIsConstructionMethod =*/ true>;

    class DrawSketchHandlerFins : public DrawSketchHandlerFinsBase
    {
        friend DrawSketchHandlerFinsBase; // allow DrawSketchHandlerFinsBase specialisations access DrawSketchHandlerFins private members

    public:

        DrawSketchHandlerFins(ConstructionMethod constrMethod = ConstructionMethod::OneSide) :
            DrawSketchHandlerFinsBase(constrMethod),
            numberOfFins(1) {}

        virtual ~DrawSketchHandlerFins() = default;

    private:
        virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
            switch (state()) {
            case SelectMode::SeekFirst:
            {
                drawPositionAtCursor(onSketchPos);

                firstCorner = onSketchPos;

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            break;
            case SelectMode::SeekSecond:
            {
                drawDirectionAtCursor(onSketchPos, firstCorner);

                thirdCorner = onSketchPos;
                secondCorner = Base::Vector2d(onSketchPos.x, firstCorner.y);
                fourthCorner = Base::Vector2d(firstCorner.x, onSketchPos.y);

                length = (firstCorner - secondCorner).Length();
                width = (firstCorner - fourthCorner).Length();
                finLength = length / 2 / numberOfFins;
                finWidth = width / 2;

                if (constructionMethod() == ConstructionMethod::TwoSide)
                    finWidth = finWidth / 2;

                try {
                    createShape(true);
                    drawEdit(toPointerVector(ShapeGeometry));
                }
                catch (const Base::ValueError&) {} // equal points while hovering raise an objection that can be safely ignored

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.0, 0.0))) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }
            break;
            case SelectMode::SeekThird:
            {
                double minX, minY, maxX, maxY;
                minX = std::min(firstCorner.x, thirdCorner.x);
                maxX = std::max(firstCorner.x, thirdCorner.x);
                minY = std::min(firstCorner.y, thirdCorner.y);
                maxY = std::max(firstCorner.y, thirdCorner.y);

                if (onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0 && onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0) {
                    //do something only if mouse pos is in the rectangle.

                    Base::Vector2d projectedPoint;
                    projectedPoint.ProjectToLine(onSketchPos - firstCorner, secondCorner - firstCorner);
                    projectedPoint = firstCorner + projectedPoint;
                    finLength = (projectedPoint - firstCorner).Length();
                    if (finLength > length / numberOfFins) {
                        int t = static_cast<int>(finLength * numberOfFins / length);

                        if (finLength > length * (numberOfFins - 1) / numberOfFins)
                            finLength = length - finLength;
                        else {
                            if (std::fmod(finLength, length / numberOfFins) < length * t / (numberOfFins - 1) - length * t / (numberOfFins)) {
                                finLength = length - (numberOfFins - 1) * finLength / t;
                            }
                            else {
                                finLength = (length - (numberOfFins - 1) * finLength / t) / (1. - (numberOfFins - 1.) / t);
                            }
                        }
                    }

                    projectedPoint.ProjectToLine(onSketchPos - firstCorner, fourthCorner - firstCorner);
                    projectedPoint = firstCorner + projectedPoint;
                    finWidth = (width - (projectedPoint - firstCorner).Length());

                    if (constructionMethod() == ConstructionMethod::TwoSide) {
                        if(finWidth > width / 2)
                            finWidth = width - finWidth;
                    }

                    SbString text;
                    text.sprintf(" (%.1ffl %.1ffw)", finLength, finWidth);
                    setPositionText(onSketchPos, text);

                    createShape(true);
                    drawEdit(toPointerVector(ShapeGeometry));
                }
                else if (prevNumberOfFins != numberOfFins) {
                    prevNumberOfFins = numberOfFins;
                    finLength = length / 2 / numberOfFins;
                    finWidth = width / 2;

                    if (constructionMethod() == ConstructionMethod::TwoSide)
                        finWidth = finWidth / 2;

                    try {
                        createShape(true);
                        drawEdit(toPointerVector(ShapeGeometry));
                    }
                    catch (const Base::ValueError&) {} // equal points while hovering raise an objection that can be safely ignored
                }
            }
            break;
            default:
                break;
            }
        }

        virtual void executeCommands() override {
            firstCurve = getHighestCurveIndex() + 1;

            try {

                createShape(false);

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch box with fins"));

                auto shapeGeometry = toPointerVector(ShapeGeometry);
                Gui::Command::doCommand(Gui::Command::Doc,
                    Sketcher::PythonConverter::convert(Gui::Command::getObjectCmd(sketchgui->getObject()), shapeGeometry).c_str());

                auto shapeConstraints = toPointerVector(ShapeConstraints);
                Gui::Command::doCommand(Gui::Command::Doc,
                    Sketcher::PythonConverter::convert(Gui::Command::getObjectCmd(sketchgui->getObject()), shapeConstraints).c_str());

                Gui::Command::commitCommand();
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Failed to add box with fins: %s\n", e.what());
                Gui::Command::abortCommand();
            }

            lastCurve = getHighestCurveIndex();

        }

        virtual void generateAutoConstraints() override {

            // add auto constraints at the start of the first side
            if (!sugConstraints[0].empty())
                generateAutoConstraintsOnElement(sugConstraints[0], firstCurve, Sketcher::PointPos::start);

            if (!sugConstraints[1].empty()) {
                if (numberOfFins == 1)
                    generateAutoConstraintsOnElement(sugConstraints[1], constructionPointOneId, Sketcher::PointPos::start);
                else {
                    if (constructionMethod() == ConstructionMethod::OneSide)
                        generateAutoConstraintsOnElement(sugConstraints[1], firstCurve + 1, Sketcher::PointPos::end);
                    else
                        generateAutoConstraintsOnElement(sugConstraints[1], firstCurve + (lastCurve - firstCurve) / 2, Sketcher::PointPos::end);
                }
            }

            // Ensure temporary autoconstraints do not generate a redundancy and that the geometry parameters are accurate
            // This is particularly important for adding widget mandated constraints.
            removeRedundantAutoConstraints();
        }

        virtual void createAutoConstraints() override {
            createGeneratedAutoConstraints(true);

            sugConstraints[0].clear();
            sugConstraints[1].clear();
        }

        virtual std::string getToolName() const override {
            return "DSH_Fins";
        }

        virtual QString getCrosshairCursorSVGName() const override {
            return QString::fromLatin1("Sketcher_CreateFins");
        }


    private:
        Base::Vector2d firstCorner, secondCorner, thirdCorner, fourthCorner;
        double length, width, finLength, finWidth, spaceLength;
        int firstCurve, lastCurve, constructionPointOneId, numberOfFins, prevNumberOfFins;

        virtual void createShape(bool onlygeometry) override {
            Q_UNUSED(onlygeometry);

            ShapeGeometry.clear();

            spaceLength = (length - numberOfFins * finLength) / (numberOfFins - 1);
            Base::Vector2d v1 = (secondCorner - firstCorner) / length;
            Base::Vector2d v2 = (fourthCorner - firstCorner) / width;

            if (constructionMethod() == ConstructionMethod::OneSide) {
                addLineToShapeGeometry(Base::Vector3d(firstCorner.x, firstCorner.y, 0.), Base::Vector3d(secondCorner.x, secondCorner.y, 0.), geometryCreationMode);
                if (numberOfFins == 1) {
                    Base::Vector2d p5 = thirdCorner - v2 * finWidth;
                    Base::Vector2d p6 = firstCorner + v1 * finLength + v2 * (width - finWidth);
                    Base::Vector2d p7 = fourthCorner + v1 * finLength;
                    addLineToShapeGeometry(Base::Vector3d(secondCorner.x, secondCorner.y, 0.), Base::Vector3d(p5.x, p5.y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(p5.x, p5.y, 0.), Base::Vector3d(p6.x, p6.y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(p6.x, p6.y, 0.), Base::Vector3d(p7.x, p7.y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(p7.x, p7.y, 0.), Base::Vector3d(fourthCorner.x, fourthCorner.y, 0.), geometryCreationMode);
                }
                else {
                    addLineToShapeGeometry(Base::Vector3d(secondCorner.x, secondCorner.y, 0.), Base::Vector3d(thirdCorner.x, thirdCorner.y, 0.), geometryCreationMode);
                    Base::Vector2d p5 = thirdCorner - v1 * finLength;
                    addLineToShapeGeometry(Base::Vector3d(thirdCorner.x, thirdCorner.y, 0.), Base::Vector3d(p5.x, p5.y, 0.), geometryCreationMode);
                    for (int i = 1; i < numberOfFins; i++) {
                        Base::Vector2d p6 = p5 - v2 * finWidth;
                        Base::Vector2d p7 = p6 - v1 * spaceLength;
                        Base::Vector2d p8 = p7 + v2 * finWidth;
                        addLineToShapeGeometry(Base::Vector3d(p5.x, p5.y, 0.), Base::Vector3d(p6.x, p6.y, 0.), geometryCreationMode);
                        addLineToShapeGeometry(Base::Vector3d(p6.x, p6.y, 0.), Base::Vector3d(p7.x, p7.y, 0.), geometryCreationMode);
                        addLineToShapeGeometry(Base::Vector3d(p7.x, p7.y, 0.), Base::Vector3d(p8.x, p8.y, 0.), geometryCreationMode);
                        p5 = p8 - v1 * finLength;
                        addLineToShapeGeometry(Base::Vector3d(p8.x, p8.y, 0.), Base::Vector3d(p5.x, p5.y, 0.), geometryCreationMode);
                    }
                }
                addLineToShapeGeometry(Base::Vector3d(fourthCorner.x, fourthCorner.y, 0.), Base::Vector3d(firstCorner.x, firstCorner.y, 0.), geometryCreationMode);

            }
            else {
                Base::Vector2d p5 = firstCorner + v1 * finLength;
                addLineToShapeGeometry(Base::Vector3d(firstCorner.x, firstCorner.y, 0.), Base::Vector3d(p5.x, p5.y, 0.), geometryCreationMode);
                if (numberOfFins == 1) {
                    Base::Vector2d p6 = p5 + v2 * finWidth;
                    Base::Vector2d p7 = secondCorner + v2 * finWidth;
                    Base::Vector2d p8 = thirdCorner - v2 * finWidth;
                    Base::Vector2d p10 = fourthCorner + v1 * finLength;
                    Base::Vector2d p9 = p10 - v2 * finWidth;
                    addLineToShapeGeometry(Base::Vector3d(p5.x, p5.y, 0.), Base::Vector3d(p6.x, p6.y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(p6.x, p6.y, 0.), Base::Vector3d(p7.x, p7.y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(p7.x, p7.y, 0.), Base::Vector3d(p8.x, p8.y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(p8.x, p8.y, 0.), Base::Vector3d(p9.x, p9.y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(p9.x, p9.y, 0.), Base::Vector3d(p10.x, p10.y, 0.), geometryCreationMode);
                    addLineToShapeGeometry(Base::Vector3d(p10.x, p10.y, 0.), Base::Vector3d(fourthCorner.x, fourthCorner.y, 0.), geometryCreationMode);
                }
                else {
                    for (int i = 1; i < numberOfFins; i++) {
                        Base::Vector2d p6 = p5 + v2 * finWidth;
                        Base::Vector2d p7 = p6 + v1 * spaceLength;
                        Base::Vector2d p8 = p7 - v2 * finWidth;
                        addLineToShapeGeometry(Base::Vector3d(p5.x, p5.y, 0.), Base::Vector3d(p6.x, p6.y, 0.), geometryCreationMode);
                        addLineToShapeGeometry(Base::Vector3d(p6.x, p6.y, 0.), Base::Vector3d(p7.x, p7.y, 0.), geometryCreationMode);
                        addLineToShapeGeometry(Base::Vector3d(p7.x, p7.y, 0.), Base::Vector3d(p8.x, p8.y, 0.), geometryCreationMode);
                        p5 = p8 + v1 * finLength;
                        addLineToShapeGeometry(Base::Vector3d(p8.x, p8.y, 0.), Base::Vector3d(p5.x, p5.y, 0.), geometryCreationMode);
                    }
                    addLineToShapeGeometry(Base::Vector3d(secondCorner.x, secondCorner.y, 0.), Base::Vector3d(thirdCorner.x, thirdCorner.y, 0.), geometryCreationMode);

                    p5 = thirdCorner - v1 * finLength;
                    addLineToShapeGeometry(Base::Vector3d(thirdCorner.x, thirdCorner.y, 0.), Base::Vector3d(p5.x, p5.y, 0.), geometryCreationMode);
                    for (int i = 1; i < numberOfFins; i++) {
                        Base::Vector2d p6 = p5 - v2 * finWidth;
                        Base::Vector2d p7 = p6 - v1 * spaceLength;
                        Base::Vector2d p8 = p7 + v2 * finWidth;
                        addLineToShapeGeometry(Base::Vector3d(p5.x, p5.y, 0.), Base::Vector3d(p6.x, p6.y, 0.), geometryCreationMode);
                        addLineToShapeGeometry(Base::Vector3d(p6.x, p6.y, 0.), Base::Vector3d(p7.x, p7.y, 0.), geometryCreationMode);
                        addLineToShapeGeometry(Base::Vector3d(p7.x, p7.y, 0.), Base::Vector3d(p8.x, p8.y, 0.), geometryCreationMode);
                        p5 = p8 - v1 * finLength;
                        addLineToShapeGeometry(Base::Vector3d(p8.x, p8.y, 0.), Base::Vector3d(p5.x, p5.y, 0.), geometryCreationMode);
                    }
                }
                addLineToShapeGeometry(Base::Vector3d(fourthCorner.x, fourthCorner.y, 0.), Base::Vector3d(firstCorner.x, firstCorner.y, 0.), geometryCreationMode);
            }


            if (!onlygeometry) {
                ShapeConstraints.clear();

                int numberOfGeos = ShapeGeometry.size();

                for (int i = getHighestCurveIndex() + 1; i < getHighestCurveIndex() + numberOfGeos; i++) {
                    addToShapeConstraints(Sketcher::Perpendicular, i, Sketcher::PointPos::none, i + 1, Sketcher::PointPos::none);
                    addToShapeConstraints(Sketcher::Coincident, i, Sketcher::PointPos::end, i + 1, Sketcher::PointPos::start);
                }
                addToShapeConstraints(Sketcher::Coincident, getHighestCurveIndex() + 1, Sketcher::PointPos::start, getHighestCurveIndex() + numberOfGeos, Sketcher::PointPos::end);

                addToShapeConstraints(Sketcher::Horizontal, getHighestCurveIndex() + 1);

                if (numberOfFins != 1) {
                    if (constructionMethod() == ConstructionMethod::OneSide) {
                        for (int i = getHighestCurveIndex() + 3; i + 4 < getHighestCurveIndex() + numberOfGeos; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                        }
                        for (int i = getHighestCurveIndex() + 4; i + 4 < getHighestCurveIndex() + numberOfGeos; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 2, Sketcher::PointPos::none);
                        }
                        addToShapeConstraints(Sketcher::Equal, getHighestCurveIndex() + numberOfGeos - 2, Sketcher::PointPos::none, getHighestCurveIndex() + numberOfGeos - 6, Sketcher::PointPos::none);

                        for (int i = getHighestCurveIndex() + 5; i + 4 < getHighestCurveIndex() + numberOfGeos; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                        }
                    }
                    else {
                        for (int i = getHighestCurveIndex() + 1; i + 4 < getHighestCurveIndex() + numberOfGeos / 2; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                        }
                        addToShapeConstraints(Sketcher::Equal, getHighestCurveIndex() + numberOfGeos / 2 - 1, Sketcher::PointPos::none, getHighestCurveIndex() + numberOfGeos / 2 + 1, Sketcher::PointPos::none);
                        for (int i = getHighestCurveIndex() + numberOfGeos / 2 + 1; i + 4 < getHighestCurveIndex() + numberOfGeos; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                        }

                        for (int i = getHighestCurveIndex() + 2; i + 4 < getHighestCurveIndex() + numberOfGeos / 2; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 2, Sketcher::PointPos::none);
                        }
                        addToShapeConstraints(Sketcher::Equal, getHighestCurveIndex() + numberOfGeos / 2 - 2, Sketcher::PointPos::none, getHighestCurveIndex() + numberOfGeos / 2 - 6, Sketcher::PointPos::none);
                        addToShapeConstraints(Sketcher::Equal, getHighestCurveIndex() + numberOfGeos / 2 - 2, Sketcher::PointPos::none, getHighestCurveIndex() + numberOfGeos / 2 + 2, Sketcher::PointPos::none);
                        for (int i = getHighestCurveIndex() + numberOfGeos / 2 + 2; i + 4 < getHighestCurveIndex() + numberOfGeos; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 2, Sketcher::PointPos::none);
                        }
                        addToShapeConstraints(Sketcher::Equal, getHighestCurveIndex() + numberOfGeos - 2, Sketcher::PointPos::none, getHighestCurveIndex() + numberOfGeos - 6, Sketcher::PointPos::none);

                        for (int i = getHighestCurveIndex() + 3; i + 4 < getHighestCurveIndex() + numberOfGeos / 2; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                        }
                        addToShapeConstraints(Sketcher::Equal, getHighestCurveIndex() + numberOfGeos / 2 - 3, Sketcher::PointPos::none, getHighestCurveIndex() + numberOfGeos / 2 + 3, Sketcher::PointPos::none);
                        for (int i = getHighestCurveIndex() + numberOfGeos / 2 + 3; i + 4 < getHighestCurveIndex() + numberOfGeos; i = i + 4) {
                            addToShapeConstraints(Sketcher::Equal, i, Sketcher::PointPos::none, i + 4, Sketcher::PointPos::none);
                        }
                    }
                }
                else {
                    //create a construction point at thirdcorner for autoconstraints
                    addPointToShapeGeometry(Base::Vector3d(thirdCorner.x, thirdCorner.y, 0.), true);
                    constructionPointOneId = getHighestCurveIndex() + ShapeGeometry.size();
                    if (constructionMethod() == ConstructionMethod::OneSide) {
                        addToShapeConstraints(Sketcher::PointOnObject, constructionPointOneId, Sketcher::PointPos::start, getHighestCurveIndex() + 2);
                        addToShapeConstraints(Sketcher::PointOnObject, constructionPointOneId, Sketcher::PointPos::start, getHighestCurveIndex() + 5);
                    }
                    else {
                        addToShapeConstraints(Sketcher::PointOnObject, constructionPointOneId, Sketcher::PointPos::start, getHighestCurveIndex() + 4);
                        addToShapeConstraints(Sketcher::PointOnObject, constructionPointOneId, Sketcher::PointPos::start, getHighestCurveIndex() + 7);
                    }
                }
            }
        }
    };

    template <> auto DrawSketchHandlerFinsBase::ToolWidgetManager::getState(int parameterindex) const {
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
        case WParameter::Seventh:
            return handler->state();
            break;
        default:
            THROWM(Base::ValueError, "Parameter index without an associated machine state")
        }
    }

    template <> void DrawSketchHandlerFinsBase::ToolWidgetManager::configureToolWidget() {
        if (!init) { // Code to be executed only upon initialisation
            QStringList names = { QStringLiteral("Fins on one side"), QStringLiteral("Fins on two sides") };
            toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

            toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_fins", "x of 1st point"));
            toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_fins", "y of 1st point"));
            toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_fins", "Length (X axis)"));
            toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_fins", "Width (Y axis)"));
            toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p5_fins", "Fin length"));
            toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("TaskSketcherTool_p6_fins", "Fin width"));
            toolWidget->setParameterLabel(WParameter::Seventh, QApplication::translate("TaskSketcherTool_p7_fins", "Number of fins"));

            syncConstructionMethodComboboxToHandler(); // in case the DSH was called with a specific construction method
        }


        toolWidget->configureParameterInitialValue(WParameter::Seventh, dHandler->numberOfFins);

        handler->updateCursor();
    }

    template <> void DrawSketchHandlerFinsBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
        switch (parameterindex) {
        case WParameter::First:
            dHandler->firstCorner.x = value;
            dHandler->fourthCorner.x = value;
            break;
        case WParameter::Second:
            dHandler->firstCorner.y = value;
            dHandler->secondCorner.y = value;
            break;
        case WParameter::Third:
            dHandler->length = value;
            dHandler->thirdCorner.x = dHandler->firstCorner.x + dHandler->length;
            dHandler->secondCorner.x = dHandler->thirdCorner.x;
            break;
        case WParameter::Fourth:
            dHandler->width = value;
            dHandler->thirdCorner.y = dHandler->firstCorner.y + dHandler->width;
            dHandler->fourthCorner.y = dHandler->thirdCorner.y;
            break;
        case WParameter::Fifth:
            dHandler->finLength = value;
            break;
        case WParameter::Sixth:
            dHandler->finWidth = value;
            break;
        case WParameter::Seventh:
            dHandler->numberOfFins = std::max(1, static_cast<int>(value));
            break;
        }
        
    }

    template <> void DrawSketchHandlerFinsBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

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
                dHandler->length = toolWidget->getParameter(WParameter::Third);
                onSketchPos.x = dHandler->firstCorner.x + dHandler->length;
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                dHandler->width = toolWidget->getParameter(WParameter::Fourth);
                onSketchPos.y = dHandler->firstCorner.y + dHandler->width;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {
                dHandler->finLength = toolWidget->getParameter(WParameter::Fifth);
                Base::Vector2d projectedPoint;
                projectedPoint.ProjectToLine(onSketchPos - dHandler->firstCorner, dHandler->fourthCorner - dHandler->firstCorner);
                onSketchPos = dHandler->firstCorner + (dHandler->secondCorner - dHandler->firstCorner) * dHandler->finLength + projectedPoint;
            }
            if (toolWidget->isParameterSet(WParameter::Sixth)) {
                dHandler->finWidth = toolWidget->getParameter(WParameter::Sixth);
                Base::Vector2d projectedPoint;
                projectedPoint.ProjectToLine(onSketchPos - dHandler->firstCorner, dHandler->secondCorner - dHandler->firstCorner);
                onSketchPos = dHandler->firstCorner + (dHandler->fourthCorner - dHandler->firstCorner) * dHandler->finWidth + projectedPoint;
            }
        }
        break;
        default:
            break;
        }
    }

    template <> void DrawSketchHandlerFinsBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
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
                toolWidget->updateVisualValue(WParameter::Third, dHandler->length);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, dHandler->width);
        }
        break;
        case SelectMode::SeekThird:
        {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, dHandler->finLength);

            if (!toolWidget->isParameterSet(WParameter::Sixth))
                toolWidget->updateVisualValue(WParameter::Sixth, dHandler->finWidth);
        }
        break;
        default:
            break;
        }
    }

    template <> void DrawSketchHandlerFinsBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
        switch (handler->state()) {
        case SelectMode::SeekFirst:
        {
            if (toolWidget->isParameterSet(WParameter::First) &&
                toolWidget->isParameterSet(WParameter::Second)) {

                handler->setState(SelectMode::SeekSecond);

                handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            if (toolWidget->isParameterSet(WParameter::Third) ||
                toolWidget->isParameterSet(WParameter::Fourth)) {

                doEnforceWidgetParameters(prevCursorPosition);
                handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                if (toolWidget->isParameterSet(WParameter::Third) &&
                    toolWidget->isParameterSet(WParameter::Fourth)) {

                    handler->setState(SelectMode::SeekThird);

                }
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            if (toolWidget->isParameterSet(WParameter::Fifth) ||
                toolWidget->isParameterSet(WParameter::Sixth)) {

                doEnforceWidgetParameters(prevCursorPosition);
                handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                if (toolWidget->isParameterSet(WParameter::Fifth) &&
                    toolWidget->isParameterSet(WParameter::Sixth)) {

                    handler->setState(SelectMode::End);
                    handler->finish();
                }
            }
        }
        break;
        default:
            break;
        }
    }

    template <> void DrawSketchHandlerFinsBase::ToolWidgetManager::addConstraints() {
        int firstCurve = dHandler->firstCurve;

        auto x0 = toolWidget->getParameter(WParameter::First);
        auto y0 = toolWidget->getParameter(WParameter::Second);
        auto length = toolWidget->getParameter(WParameter::Third);
        auto width = toolWidget->getParameter(WParameter::Fourth);
        auto finLength = toolWidget->getParameter(WParameter::Fifth);
        auto finWidth = toolWidget->getParameter(WParameter::Sixth);

        auto x0set = toolWidget->isParameterSet(WParameter::First);
        auto y0set = toolWidget->isParameterSet(WParameter::Second);
        auto lengthSet = toolWidget->isParameterSet(WParameter::Third);
        auto widthSet = toolWidget->isParameterSet(WParameter::Fourth);
        auto finLengthSet = toolWidget->isParameterSet(WParameter::Fifth);
        auto finWidthSet = toolWidget->isParameterSet(WParameter::Sixth);

        using namespace Sketcher;

        auto constraintx0 = [&]() {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::VAxis, x0, handler->sketchgui->getObject());
        };

        auto constrainty0 = [&]() {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::HAxis, y0, handler->sketchgui->getObject());
        };

        auto constraintlength = [&]() {
            int lineId = firstCurve + 1;
            if (handler->constructionMethod() == ConstructionMethod::TwoSide)
                lineId = firstCurve + (dHandler->lastCurve + 1 - firstCurve) / 2;
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f)) ",
                lineId, 1, dHandler->lastCurve, fabs(length));
        };

        auto constraintwidth = [&]() {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                dHandler->lastCurve, fabs(width));
        };

        // NOTE: if AutoConstraints is empty, we can add constraints directly without any diagnose. No diagnose was run.
        if (handler->AutoConstraints.empty()) {
            if (x0set)
                constraintx0();

            if (y0set)
                constrainty0();

            if (lengthSet)
                constraintlength();

            if (widthSet)
                constraintwidth();
        }
        else { // There is a valid diagnose.
            auto firstpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start));

            if (x0set && firstpointinfo.isXDoF()) {
                constraintx0();

                handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition

                firstpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start)); // get updated point position
            }

            if (y0set && firstpointinfo.isYDoF()) {
                constrainty0();

                handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition
            }

            if (lengthSet) {
                auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 1, PointPos::start));
                auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 3, PointPos::end));

                int DoFs = startpointinfo.getDoFs();
                DoFs += endpointinfo.getDoFs();

                if (DoFs > 0) {
                    constraintlength();
                }

                handler->diagnoseWithAutoConstraints();
            }

            if (widthSet) {
                auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start));
                auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 2, PointPos::end));

                int DoFs = startpointinfo.getDoFs();
                DoFs += endpointinfo.getDoFs();

                if (DoFs > 0) {
                    constraintwidth();
                }
            }
        }

        // NOTE: As of today, there are no autoconstraints on fins, therefore, they are necessarily constrainable were applicable. It could be implemented but it's a bit tricky.

        if (finLengthSet) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                dHandler->lastCurve - 1, fabs(finLength));
        }

        if (finWidthSet) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                dHandler->lastCurve - 2, fabs(finWidth));
        }
    }


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerFins_H

