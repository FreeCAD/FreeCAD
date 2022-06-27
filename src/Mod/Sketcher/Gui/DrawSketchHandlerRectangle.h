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


#ifndef SKETCHERGUI_DrawSketchHandlerRectangle_H
#define SKETCHERGUI_DrawSketchHandlerRectangle_H


#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class DrawSketchHandlerRectangle;

namespace ConstructionMethods {

enum class RectangleConstructionMethod {
    Diagonal,
    CenterAndCorner,
    End // Must be the last one
};

}

using DrawSketchHandlerRectangleBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerRectangle,
                                                                        StateMachines::FourSeekEnd,
                                                                        /*PEditCurveSize =*/ 5,
                                                                        /*PAutoConstraintSize =*/ 2,
                                                                        /*WidgetParametersT =*/WidgetParameters<6, 6>,
                                                                        /*WidgetCheckboxesT =*/WidgetCheckboxes<2, 2>,
                                                                        /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
                                                                        ConstructionMethods::RectangleConstructionMethod,
                                                                        /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerRectangle: public DrawSketchHandlerRectangleBase
{
    friend DrawSketchHandlerRectangleBase; // allow DrawSketchHandlerRectangleBase specialisations access DrawSketchHandlerRectangle private members

public:

    DrawSketchHandlerRectangle(ConstructionMethod constrMethod = ConstructionMethod::Diagonal, bool roundcorners = false, bool frame = false) :
        DrawSketchHandlerRectangleBase(constrMethod),
        roundCorners(roundcorners),
        makeFrame(frame),
        thickness(0.) {}

    virtual ~DrawSketchHandlerRectangle() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch(state()) {
            case SelectMode::SeekFirst:
            {
                drawPositionAtCursor(onSketchPos);

                if(constructionMethod() == ConstructionMethod::Diagonal)
                    firstCorner = onSketchPos;
                else //(constructionMethod == ConstructionMethod::CenterAndCorner)
                    center = onSketchPos;

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f,0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            break;
            case SelectMode::SeekSecond:
            {
                if(constructionMethod() == ConstructionMethod::Diagonal) {
                    drawDirectionAtCursor(onSketchPos, firstCorner);

                    thirdCorner = onSketchPos;
                    secondCorner = Base::Vector2d(onSketchPos.x ,firstCorner.y);
                    fourthCorner = Base::Vector2d(firstCorner.x,onSketchPos.y);

                }
                else { //if (constructionMethod == ConstructionMethod::CenterAndCorner)
                    drawDirectionAtCursor(onSketchPos, center);

                    firstCorner = center - (onSketchPos - center);
                    secondCorner = Base::Vector2d(onSketchPos.x, firstCorner.y);
                    thirdCorner = onSketchPos;
                    fourthCorner = Base::Vector2d(firstCorner.x, onSketchPos.y);
                }

                if (roundCorners) {
                    if (fabs(length) > fabs(width)) {
                        radius = fabs(width) / 6;
                    }
                    else {
                        radius = fabs(length) / 6;
                    }
                }
                else {
                    radius = 0.;
                }

                try {
                    CreateAndDrawShapeGeometry();
                }
                catch(const Base::ValueError &) {} // equal points while hovering raise an objection that can be safely ignored

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.0,0.0))) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }
            break;
            case SelectMode::SeekThird:
            {
                double dx, dy, minX, minY, maxX, maxY;
                minX = std::min(firstCorner.x, thirdCorner.x);
                maxX = std::max(firstCorner.x, thirdCorner.x);
                minY = std::min(firstCorner.y, thirdCorner.y);
                maxY = std::max(firstCorner.y, thirdCorner.y);

                if (roundCorners) {
                    if (onSketchPos.x < minX || onSketchPos.y < minY || onSketchPos.x > maxX || onSketchPos.y > maxY) {
                        radius = 0.;
                    }
                    else {
                        dx = onSketchPos.x - minX;
                        dy = onSketchPos.y - minY;
                        if (dx < abs(length / 2)) {
                            dx = (onSketchPos.x - minX);
                        }
                        else {
                            dx = -(onSketchPos.x - maxX);
                        }
                        dy = onSketchPos.y - minY;
                        if (dy < abs(width / 2)) {
                            dy = (onSketchPos.y - minY);
                        }
                        else {
                            dy = -(onSketchPos.y - maxY);
                        }
                        radius = std::min((dx + dy + sqrt(2 * dx * dy)), std::min(abs(length / 2), abs(width / 2)) * 0.99);
                    }
                    SbString text;
                    text.sprintf(" (%.1f radius)", radius);
                    setPositionText(onSketchPos, text);
                }
                else {//This is the case of frame of normal rectangle.

                    dx = std::min(abs(onSketchPos.x - minX), abs(onSketchPos.x - maxX));
                    dy = std::min(abs(onSketchPos.y - minY), abs(onSketchPos.y - maxY));
                    if (onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0 && !(onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0))
                        thickness = dy;
                    else if (onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0 && !(onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0))
                        thickness = dx;
                    else if (onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0 && onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0)
                        thickness = -std::min(dx, dy);
                    else
                        thickness = std::max(dx, dy);

                    firstCornerFrame.x = firstCorner.x == minX ? minX - thickness : maxX + thickness;
                    firstCornerFrame.y = firstCorner.y == minY ? minY - thickness : maxY + thickness;
                    thirdCornerFrame.x = thirdCorner.x == minX ? minX - thickness : maxX + thickness;
                    thirdCornerFrame.y = thirdCorner.y == minY ? minY - thickness : maxY + thickness;
                    secondCornerFrame = Base::Vector2d(thirdCornerFrame.x, firstCornerFrame.y);
                    fourthCornerFrame = Base::Vector2d(firstCornerFrame.x, thirdCornerFrame.y);

                    SbString text;
                    text.sprintf(" (%.1fT)", thickness);
                    setPositionText(onSketchPos, text);
                }

                CreateAndDrawShapeGeometry();
            }
            break;
            case SelectMode::SeekFourth:
            {
                //This is the case of frame of round corner rectangle.
                double dx, dy, minX, minY, maxX, maxY;
                minX = std::min(firstCorner.x, thirdCorner.x);
                maxX = std::max(firstCorner.x, thirdCorner.x);
                minY = std::min(firstCorner.y, thirdCorner.y);
                maxY = std::max(firstCorner.y, thirdCorner.y);

                dx = std::min(abs(onSketchPos.x - minX), abs(onSketchPos.x - maxX));
                dy = std::min(abs(onSketchPos.y - minY), abs(onSketchPos.y - maxY));
                if (onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0 && !(onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0))
                    thickness = dy;
                else if (onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0 && !(onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0))
                    thickness = dx;
                else if (onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0 && onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0)
                    thickness = -std::min(dx, dy);
                else
                    thickness = std::max(dx, dy);

                firstCornerFrame.x = firstCorner.x == minX ? minX - thickness : maxX + thickness;
                firstCornerFrame.y = firstCorner.y == minY ? minY - thickness : maxY + thickness;
                thirdCornerFrame.x = thirdCorner.x == minX ? minX - thickness : maxX + thickness;
                thirdCornerFrame.y = thirdCorner.y == minY ? minY - thickness : maxY + thickness;
                secondCornerFrame = Base::Vector2d(thirdCornerFrame.x, firstCornerFrame.y);
                fourthCornerFrame = Base::Vector2d(firstCornerFrame.x, thirdCornerFrame.y);

                SbString text;
                text.sprintf(" (%.1fT)", thickness);
                setPositionText(onSketchPos, text);

                CreateAndDrawShapeGeometry();
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

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch box"));

            commandAddShapeGeometryAndConstraints();

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add box: %s\n", e.what());
            Gui::Command::abortCommand();
            THROWM(Base::RuntimeError, "Tool execution aborted\n") // This prevents constraints from being applied on non existing geometry
        }

        thickness = 0.;
    }

    virtual void generateAutoConstraints() override {

        if(constructionMethod() == ConstructionMethod::Diagonal) {
            // add auto constraints at the start of the first side
            if (radius > Precision::Confusion()) {
                if (!sugConstraints[0].empty())
                    generateAutoConstraintsOnElement(sugConstraints[0], constructionPointOneId, Sketcher::PointPos::start);

                if (!sugConstraints[1].empty())
                    generateAutoConstraintsOnElement(sugConstraints[1], constructionPointTwoId, Sketcher::PointPos::start);
            }
            else {
                if (!sugConstraints[0].empty())
                    generateAutoConstraintsOnElement(sugConstraints[0], firstCurve, Sketcher::PointPos::start);

                if (!sugConstraints[1].empty())
                    generateAutoConstraintsOnElement(sugConstraints[1], firstCurve + 1, Sketcher::PointPos::end);
            }
        }
        else if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
            // add auto constraints at center
            if (!sugConstraints[0].empty())
                generateAutoConstraintsOnElement(sugConstraints[0], centerPointId, Sketcher::PointPos::start);

            // add auto constraints for the line segment end
            if (!sugConstraints[1].empty()) {
                if (radius > Precision::Confusion())
                    generateAutoConstraintsOnElement(sugConstraints[1], constructionPointOneId, Sketcher::PointPos::start);
                else
                    generateAutoConstraintsOnElement(sugConstraints[1], firstCurve + 1, Sketcher::PointPos::end);
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
        return "DSH_Rectangle";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if(!roundCorners && !makeFrame) {
            if(constructionMethod() == ConstructionMethod::CenterAndCorner)
                return QString::fromLatin1("Sketcher_Pointer_Create_Box_Center");
            else
                return QString::fromLatin1("Sketcher_Pointer_Create_Box");
        }
        else if (roundCorners && !makeFrame) {
            if(constructionMethod() == ConstructionMethod::CenterAndCorner)
                return QString::fromLatin1("Sketcher_Pointer_Oblong_Center");
            else
                return QString::fromLatin1("Sketcher_Pointer_Oblong");
        }
        else if (!roundCorners && makeFrame) {
            if(constructionMethod() == ConstructionMethod::CenterAndCorner)
                return QString::fromLatin1("Sketcher_Pointer_Create_Frame_Center");
            else
                return QString::fromLatin1("Sketcher_Pointer_Create_Frame");
        }
        else { // both roundCorners and makeFrame
            if(constructionMethod() == ConstructionMethod::CenterAndCorner)
                return QString::fromLatin1("Sketcher_Pointer_Oblong_Frame_Center");
            else
                return QString::fromLatin1("Sketcher_Pointer_Oblong_Frame");
        }
    }

    //reimplement because if not radius then it's 2 steps
    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {
        this->updateDataAndDrawToPosition(onSketchPos);

        if (state() == SelectMode::SeekSecond && !roundCorners && !makeFrame) {
            setState(SelectMode::End);
        }
        else if ((state() == SelectMode::SeekThird && roundCorners && !makeFrame) || (state() == SelectMode::SeekThird && !roundCorners && makeFrame)) {
            setState(SelectMode::End);
        }
        else {
            this->moveToNextMode();
        }
    }

private:
    Base::Vector2d center, firstCorner, secondCorner, thirdCorner, fourthCorner, firstCornerFrame, secondCornerFrame, thirdCornerFrame, fourthCornerFrame;
    Base::Vector3d arc1Center, arc2Center, arc3Center, arc4Center;
    bool roundCorners, makeFrame;
    double radius, length, width, thickness, radiusFrame;
    int signX, signY, firstCurve, constructionPointOneId, constructionPointTwoId, centerPointId;

    virtual void createShape(bool onlyeditoutline) override {

        ShapeGeometry.clear();

        length = thirdCorner.x - firstCorner.x;
        width = thirdCorner.y - firstCorner.y;
        signX = Base::sgn(length);
        signY = Base::sgn(width);
        double start = 0;
        double end = M_PI / 2;

        addLineToShapeGeometry(Base::Vector3d(firstCorner.x + signX * radius, firstCorner.y, 0.), Base::Vector3d(secondCorner.x - signX * radius, secondCorner.y, 0.), geometryCreationMode);
        addLineToShapeGeometry(Base::Vector3d(secondCorner.x, secondCorner.y + signY * radius, 0.), Base::Vector3d(thirdCorner.x, thirdCorner.y - signY * radius, 0.), geometryCreationMode);
        addLineToShapeGeometry(Base::Vector3d(thirdCorner.x - signX * radius, thirdCorner.y, 0.), Base::Vector3d(fourthCorner.x + signX * radius, fourthCorner.y, 0.), geometryCreationMode);
        addLineToShapeGeometry(Base::Vector3d(fourthCorner.x, fourthCorner.y - signY * radius, 0.), Base::Vector3d(firstCorner.x, firstCorner.y + signY * radius, 0.), geometryCreationMode);

        if (roundCorners && radius > Precision::Confusion()) {
            if (signX > 0 && signY > 0) {
                start = -M_PI;
                end = -M_PI / 2;
            }
            else if (signX > 0 && signY < 0) {
                start = M_PI / 2;
                end = M_PI;
            }
            else if (signX < 0 && signY > 0) {
                start = -M_PI / 2;
                end = 0;
            }

            //center points required later for special case of round corner frame with radiusFrame = 0.
            arc1Center = Base::Vector3d(firstCorner.x + signX * radius, firstCorner.y + signY * radius, 0.);
            arc2Center = Base::Vector3d(secondCorner.x - signX * radius, secondCorner.y + signY * radius, 0.);
            arc3Center = Base::Vector3d(thirdCorner.x - signX * radius, thirdCorner.y - signY * radius, 0.);
            arc4Center = Base::Vector3d(fourthCorner.x + signX * radius, fourthCorner.y - signY * radius, 0.);

            addArcToShapeGeometry(arc1Center, start                                            , end                                                      , radius, geometryCreationMode);
            addArcToShapeGeometry(arc2Center, (signX * signY > 0) ? end - 2 * M_PI : end - M_PI, (signX * signY > 0) ? end - 1.5 * M_PI : end - 0.5 * M_PI, radius, geometryCreationMode);
            addArcToShapeGeometry(arc3Center, end - 1.5 * M_PI                                 , end - M_PI                                               , radius, geometryCreationMode);
            addArcToShapeGeometry(arc4Center, (signX * signY > 0) ? end - M_PI : end - 2 * M_PI, (signX * signY > 0) ? end - 0.5 * M_PI : end - 1.5 * M_PI, radius, geometryCreationMode);
       }

        if (makeFrame && state() != SelectMode::SeekSecond && fabs(thickness) > Precision::Confusion()) {
            if (radius < Precision::Confusion()) {
                radiusFrame = 0.;
            }
            else {
                radiusFrame = radius + thickness;
                if (radiusFrame < 0.) {
                    radiusFrame = 0.;
                }
            }

            addLineToShapeGeometry(Base::Vector3d(firstCornerFrame.x + signX * radiusFrame, firstCornerFrame.y, 0.), Base::Vector3d(secondCornerFrame.x - signX * radiusFrame, secondCornerFrame.y, 0.), geometryCreationMode);
            addLineToShapeGeometry(Base::Vector3d(secondCornerFrame.x, secondCornerFrame.y + signY * radiusFrame, 0.), Base::Vector3d(thirdCornerFrame.x, thirdCornerFrame.y - signY * radiusFrame, 0.), geometryCreationMode);
            addLineToShapeGeometry(Base::Vector3d(thirdCornerFrame.x - signX * radiusFrame, thirdCornerFrame.y, 0.), Base::Vector3d(fourthCornerFrame.x + signX * radiusFrame, fourthCornerFrame.y, 0.), geometryCreationMode);
            addLineToShapeGeometry(Base::Vector3d(fourthCornerFrame.x, fourthCornerFrame.y - signY * radiusFrame, 0.), Base::Vector3d(firstCornerFrame.x, firstCornerFrame.y + signY * radiusFrame, 0.), geometryCreationMode);

            if (roundCorners && radiusFrame > Precision::Confusion()) {
                double start = 0;
                double end = M_PI / 2;
                if (signX > 0 && signY > 0) {
                    start = -M_PI;
                    end = -M_PI / 2;
                }
                else if (signX > 0 && signY < 0) {
                    start = M_PI / 2;
                    end = M_PI;
                }
                else if (signX < 0 && signY > 0) {
                    start = -M_PI / 2;
                    end = 0;
                }

                addArcToShapeGeometry(Base::Vector3d(firstCornerFrame.x + signX * radiusFrame , firstCornerFrame.y + signY * radiusFrame , 0.), start                                            , end                                                      , radiusFrame, geometryCreationMode);
                addArcToShapeGeometry(Base::Vector3d(secondCornerFrame.x - signX * radiusFrame, secondCornerFrame.y + signY * radiusFrame, 0.), (signX * signY > 0) ? end - 2 * M_PI : end - M_PI, (signX * signY > 0) ? end - 1.5 * M_PI : end - 0.5 * M_PI, radiusFrame, geometryCreationMode);
                addArcToShapeGeometry(Base::Vector3d(thirdCornerFrame.x - signX * radiusFrame , thirdCornerFrame.y - signY * radiusFrame , 0.), end - 1.5 * M_PI                                 , end - M_PI                                               , radiusFrame, geometryCreationMode);
                addArcToShapeGeometry(Base::Vector3d(fourthCornerFrame.x + signX * radiusFrame, fourthCornerFrame.y - signY * radiusFrame, 0.), (signX * signY > 0) ? end - M_PI : end - 2 * M_PI, (signX * signY > 0) ? end - 0.5 * M_PI : end - 1.5 * M_PI, radiusFrame, geometryCreationMode);
            }
        }

        if (!onlyeditoutline) {
            ShapeConstraints.clear();

            if (radius > Precision::Confusion()) {

                Sketcher::PointPos a = signX * signY > 0. ? Sketcher::PointPos::end : Sketcher::PointPos::start;
                Sketcher::PointPos b = signX * signY > 0. ? Sketcher::PointPos::start : Sketcher::PointPos::end;

                addToShapeConstraints(Sketcher::Tangent, firstCurve    , Sketcher::PointPos::start, firstCurve + 4, a);
                addToShapeConstraints(Sketcher::Tangent, firstCurve    , Sketcher::PointPos::end  , firstCurve + 5, b);
                addToShapeConstraints(Sketcher::Tangent, firstCurve + 1, Sketcher::PointPos::start, firstCurve + 5, a);
                addToShapeConstraints(Sketcher::Tangent, firstCurve + 1, Sketcher::PointPos::end  , firstCurve + 6, b);
                addToShapeConstraints(Sketcher::Tangent, firstCurve + 2, Sketcher::PointPos::start, firstCurve + 6, a);
                addToShapeConstraints(Sketcher::Tangent, firstCurve + 2, Sketcher::PointPos::end  , firstCurve + 7, b);
                addToShapeConstraints(Sketcher::Tangent, firstCurve + 3, Sketcher::PointPos::start, firstCurve + 7, a);
                addToShapeConstraints(Sketcher::Tangent, firstCurve + 3, Sketcher::PointPos::end  , firstCurve + 4, b);

                addToShapeConstraints(Sketcher::Horizontal, firstCurve);
                addToShapeConstraints(Sketcher::Horizontal, firstCurve + 2);
                addToShapeConstraints(Sketcher::Vertical  , firstCurve + 1);
                addToShapeConstraints(Sketcher::Vertical  , firstCurve + 3);
                addToShapeConstraints(Sketcher::Equal, firstCurve + 4, Sketcher::PointPos::none, firstCurve + 5);
                addToShapeConstraints(Sketcher::Equal, firstCurve + 5, Sketcher::PointPos::none, firstCurve + 6);
                addToShapeConstraints(Sketcher::Equal, firstCurve + 6, Sketcher::PointPos::none, firstCurve + 7);

                if (fabs(thickness) > Precision::Confusion()) {
                    if (radiusFrame < Precision::Confusion()) { //case inner rectangle is normal rectangle

                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 8, Sketcher::PointPos::end, firstCurve + 9, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 9, Sketcher::PointPos::end, firstCurve + 10, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 10, Sketcher::PointPos::end, firstCurve + 11, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 11, Sketcher::PointPos::end, firstCurve + 8, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Horizontal, firstCurve + 8);
                        addToShapeConstraints(Sketcher::Horizontal, firstCurve + 10);
                        addToShapeConstraints(Sketcher::Vertical, firstCurve + 9);
                        addToShapeConstraints(Sketcher::Vertical, firstCurve + 11);

                        //add construction lines +12, +13, +14, +15
                        addLineToShapeGeometry(Base::Vector3d(firstCorner.x + signX * radius, firstCorner.y + signY * radius, 0.), Base::Vector3d(firstCornerFrame.x, firstCornerFrame.y, 0.), true);
                        addLineToShapeGeometry(Base::Vector3d(secondCorner.x - signX * radius, secondCorner.y + signY * radius, 0.), Base::Vector3d(secondCornerFrame.x, secondCornerFrame.y, 0.), true);
                        addLineToShapeGeometry(Base::Vector3d(thirdCorner.x - signX * radius, thirdCorner.y - signY * radius, 0.), Base::Vector3d(thirdCornerFrame.x, thirdCornerFrame.y, 0.), true);
                        addLineToShapeGeometry(Base::Vector3d(fourthCorner.x + signX * radius, fourthCorner.y - signY * radius, 0.), Base::Vector3d(fourthCornerFrame.x, fourthCornerFrame.y, 0.), true);

                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 12, Sketcher::PointPos::start, firstCurve + 4 , Sketcher::PointPos::mid);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 12, Sketcher::PointPos::end  , firstCurve + 8 , Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 13, Sketcher::PointPos::start, firstCurve + 5 , Sketcher::PointPos::mid);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 13, Sketcher::PointPos::end  , firstCurve + 9 , Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 14, Sketcher::PointPos::start, firstCurve + 6 , Sketcher::PointPos::mid);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 14, Sketcher::PointPos::end  , firstCurve + 10, Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 15, Sketcher::PointPos::start, firstCurve + 7 , Sketcher::PointPos::mid);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 15, Sketcher::PointPos::end  , firstCurve + 11, Sketcher::PointPos::start);

                        addToShapeConstraints(Sketcher::Perpendicular, firstCurve + 12, Sketcher::PointPos::none, firstCurve + 13);
                        addToShapeConstraints(Sketcher::Perpendicular, firstCurve + 13, Sketcher::PointPos::none, firstCurve + 14);
                        addToShapeConstraints(Sketcher::Perpendicular, firstCurve + 14, Sketcher::PointPos::none, firstCurve + 15);

                    }
                    else { //case inner rectangle is rounded rectangle
                        addToShapeConstraints(Sketcher::Tangent, firstCurve + 8, Sketcher::PointPos::start, firstCurve + 12, a);
                        addToShapeConstraints(Sketcher::Tangent, firstCurve + 8, Sketcher::PointPos::end  , firstCurve + 13, b);
                        addToShapeConstraints(Sketcher::Tangent, firstCurve + 9, Sketcher::PointPos::start, firstCurve + 13, a);
                        addToShapeConstraints(Sketcher::Tangent, firstCurve + 9, Sketcher::PointPos::end  , firstCurve + 14, b);
                        addToShapeConstraints(Sketcher::Tangent, firstCurve + 10, Sketcher::PointPos::start, firstCurve + 14, a);
                        addToShapeConstraints(Sketcher::Tangent, firstCurve + 10, Sketcher::PointPos::end  , firstCurve + 15, b);
                        addToShapeConstraints(Sketcher::Tangent, firstCurve + 11, Sketcher::PointPos::start, firstCurve + 15, a);
                        addToShapeConstraints(Sketcher::Tangent, firstCurve + 11, Sketcher::PointPos::end  , firstCurve + 12, b);

                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 4, Sketcher::PointPos::mid, firstCurve + 12, Sketcher::PointPos::mid);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 5, Sketcher::PointPos::mid, firstCurve + 13, Sketcher::PointPos::mid);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 6, Sketcher::PointPos::mid, firstCurve + 14, Sketcher::PointPos::mid);
                        addToShapeConstraints(Sketcher::Coincident, firstCurve + 7, Sketcher::PointPos::mid, firstCurve + 15, Sketcher::PointPos::mid);
                        addToShapeConstraints(Sketcher::Horizontal, firstCurve + 8);
                        addToShapeConstraints(Sketcher::Horizontal, firstCurve + 10);
                        addToShapeConstraints(Sketcher::Vertical, firstCurve + 9);
                    }
                }


                if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                    if (fabs(thickness) > Precision::Confusion()) {
                        constructionPointOneId = firstCurve + 16;
                        centerPointId = firstCurve + 17;
                    }
                    else {
                        constructionPointOneId = firstCurve + 8;
                        centerPointId = firstCurve + 9;
                    }

                    addPointToShapeGeometry(Base::Vector3d(thirdCorner.x, thirdCorner.y, 0.), true);
                    addPointToShapeGeometry(Base::Vector3d(center.x, center.y, 0.), true);
                    addToShapeConstraints(Sketcher::Symmetric, firstCurve + 2, Sketcher::PointPos::start, firstCurve, Sketcher::PointPos::start, centerPointId, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::PointOnObject, constructionPointOneId, Sketcher::PointPos::start, firstCurve + 1);
                    addToShapeConstraints(Sketcher::PointOnObject, constructionPointOneId, Sketcher::PointPos::start, firstCurve + 2);
                }
                else {
                    if (fabs(thickness) > Precision::Confusion()) {
                        constructionPointOneId = firstCurve + 16;
                        constructionPointTwoId = firstCurve + 17;
                    }
                    else {
                        constructionPointOneId = firstCurve + 8;
                        constructionPointTwoId = firstCurve + 9;
                    }

                    addPointToShapeGeometry(Base::Vector3d(firstCorner.x, firstCorner.y, 0.), true);
                    addPointToShapeGeometry(Base::Vector3d(thirdCorner.x, thirdCorner.y, 0.), true);
                    addToShapeConstraints(Sketcher::PointOnObject, constructionPointOneId, Sketcher::PointPos::start, firstCurve);
                    addToShapeConstraints(Sketcher::PointOnObject, constructionPointOneId, Sketcher::PointPos::start, firstCurve + 3);
                    addToShapeConstraints(Sketcher::PointOnObject, constructionPointTwoId, Sketcher::PointPos::start, firstCurve + 1);
                    addToShapeConstraints(Sketcher::PointOnObject, constructionPointTwoId, Sketcher::PointPos::start, firstCurve + 2);
                }

            }
            else { //cases of normal rectangles and normal frames
                addToShapeConstraints(Sketcher::Coincident, firstCurve, Sketcher::PointPos::end, firstCurve + 1, Sketcher::PointPos::start);
                addToShapeConstraints(Sketcher::Coincident, firstCurve + 1, Sketcher::PointPos::end, firstCurve + 2, Sketcher::PointPos::start);
                addToShapeConstraints(Sketcher::Coincident, firstCurve + 2, Sketcher::PointPos::end, firstCurve + 3, Sketcher::PointPos::start);
                addToShapeConstraints(Sketcher::Coincident, firstCurve + 3, Sketcher::PointPos::end, firstCurve, Sketcher::PointPos::start);
                addToShapeConstraints(Sketcher::Horizontal, firstCurve);
                addToShapeConstraints(Sketcher::Horizontal, firstCurve + 2);
                addToShapeConstraints(Sketcher::Vertical  , firstCurve + 1);
                addToShapeConstraints(Sketcher::Vertical  , firstCurve + 3);

                if (fabs(thickness) > Precision::Confusion()) {
                    addToShapeConstraints(Sketcher::Coincident, firstCurve + 4, Sketcher::PointPos::end, firstCurve + 5, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve + 5, Sketcher::PointPos::end, firstCurve + 6, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve + 6, Sketcher::PointPos::end, firstCurve + 7, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve + 7, Sketcher::PointPos::end, firstCurve + 4, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Horizontal, firstCurve + 4);
                    addToShapeConstraints(Sketcher::Horizontal, firstCurve + 6);
                    addToShapeConstraints(Sketcher::Vertical, firstCurve + 5);
                    addToShapeConstraints(Sketcher::Vertical, firstCurve + 7);

                    //add construction lines
                    addLineToShapeGeometry(Base::Vector3d(firstCorner.x , firstCorner.y , 0.), Base::Vector3d(firstCornerFrame.x , firstCornerFrame.y , 0.), true);
                    addLineToShapeGeometry(Base::Vector3d(secondCorner.x, secondCorner.y, 0.), Base::Vector3d(secondCornerFrame.x, secondCornerFrame.y, 0.), true);
                    addLineToShapeGeometry(Base::Vector3d(thirdCorner.x , thirdCorner.y , 0.), Base::Vector3d(thirdCornerFrame.x , thirdCornerFrame.y , 0.), true);
                    addLineToShapeGeometry(Base::Vector3d(fourthCorner.x, fourthCorner.y, 0.), Base::Vector3d(fourthCornerFrame.x, fourthCornerFrame.y, 0.), true);

                    addToShapeConstraints(Sketcher::Coincident, firstCurve + 8, Sketcher::PointPos::start, firstCurve    , Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve + 8, Sketcher::PointPos::end, firstCurve + 4, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve + 9, Sketcher::PointPos::start, firstCurve + 1, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve + 9, Sketcher::PointPos::end, firstCurve + 5, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve +10, Sketcher::PointPos::start, firstCurve + 2, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve +10, Sketcher::PointPos::end, firstCurve + 6, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve +11, Sketcher::PointPos::start, firstCurve + 3, Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident, firstCurve +11, Sketcher::PointPos::end, firstCurve + 7, Sketcher::PointPos::start);

                    addToShapeConstraints(Sketcher::Perpendicular, firstCurve + 8, Sketcher::PointPos::none, firstCurve + 9);
                    addToShapeConstraints(Sketcher::Perpendicular, firstCurve + 9, Sketcher::PointPos::none, firstCurve +10);
                    addToShapeConstraints(Sketcher::Perpendicular, firstCurve +10, Sketcher::PointPos::none, firstCurve +11);
                }

                if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                    if (fabs(thickness) > Precision::Confusion())
                        centerPointId = firstCurve + 12;
                    else
                        centerPointId = firstCurve + 4;

                    addPointToShapeGeometry(Base::Vector3d(center.x, center.y, 0.), true);
                    addToShapeConstraints(Sketcher::Symmetric, firstCurve + 2, Sketcher::PointPos::start, firstCurve, Sketcher::PointPos::start, centerPointId, Sketcher::PointPos::start);
                }
            }
        }
    }

};

template <> auto DrawSketchHandlerRectangleBase::ToolWidgetManager::getState(int parameterindex) const {
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
        return SelectMode::SeekThird;
        break;
    case WParameter::Sixth:
        return SelectMode::SeekFourth;
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Diagonal corners"), QStringLiteral("Center and corner")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        syncConstructionMethodComboboxToHandler(); // in case the DSH was called with a specific construction method
    }

    if(dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal){
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (X axis)"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (Y axis)"));
    }
    else { //if (constructionMethod == ConstructionMethod::CenterAndCorner)
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_rectangle", "x of center point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_rectangle", "y of center point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (X axis)"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (Y axis)"));
    }

    toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p5_rectangle", "Corner radius"));
    toolWidget->setParameterEnabled(WParameter::Fifth, dHandler->roundCorners);

    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_rectangle", "Rounded corners"));
    toolWidget->setCheckboxToolTip(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_rectangle", "Create a rectangle with rounded corners."));

    syncCheckboxToHandler(WCheckbox::FirstBox, dHandler->roundCorners);

    toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("TaskSketcherTool_p6_rectangle", "Thickness"));
    toolWidget->setParameterEnabled(WParameter::Sixth, dHandler->makeFrame);

    toolWidget->setCheckboxLabel(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_rectangle", "Frame"));
    toolWidget->setCheckboxToolTip(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_rectangle", "Create two rectangles, one in the other with a constant thickness."));

    syncCheckboxToHandler(WCheckbox::SecondBox, dHandler->makeFrame);

    handler->updateCursor();
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if(dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal){
        switch(parameterindex) {
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
                dHandler->radius = value;
                break;
            case WParameter::Sixth:
                dHandler->thickness = value;
                break;
        }
    }
    else { //if (constructionMethod == ConstructionMethod::CenterAndCorner)
        switch(parameterindex) {
            case WParameter::First:
                dHandler->center.x = value;
                dHandler->firstCorner.x = value - dHandler->length / 2;
                dHandler->secondCorner.x = value + dHandler->length / 2;
                dHandler->thirdCorner.x = value + dHandler->length / 2;
                dHandler->fourthCorner.x = value - dHandler->length / 2;
                break;
            case WParameter::Second:
                dHandler->center.y = value;
                dHandler->firstCorner.y = value - dHandler->width / 2;
                dHandler->secondCorner.y = value - dHandler->width / 2;
                dHandler->thirdCorner.y = value + dHandler->width / 2;
                dHandler->fourthCorner.y = value + dHandler->width / 2;
                break;
            case WParameter::Third:
                dHandler->length = value;
                dHandler->firstCorner.x = dHandler->center.x + dHandler->length/2;
                dHandler->thirdCorner.x = dHandler->center.x - dHandler->length/2;
                dHandler->secondCorner.x = dHandler->thirdCorner.x;
                dHandler->fourthCorner.x = dHandler->firstCorner.x;
                break;
            case WParameter::Fourth:
                dHandler->width = value;
                dHandler->firstCorner.y = dHandler->center.y + dHandler->width / 2;
                dHandler->thirdCorner.y = dHandler->center.y - dHandler->width / 2;
                dHandler->secondCorner.y = dHandler->firstCorner.y;
                dHandler->fourthCorner.y = dHandler->thirdCorner.y;
                break;
            case WParameter::Fifth:
                dHandler->radius = value;
                break;
            case WParameter::Sixth:
                dHandler->thickness = value;
                break;
        }
    }
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    Q_UNUSED(checkboxindex);

    switch (checkboxindex) {
    case WCheckbox::FirstBox:
        dHandler->roundCorners = value;
        toolWidget->setParameterEnabled(WParameter::Fifth, value);
        break;
    case WCheckbox::SecondBox:
        dHandler->makeFrame = value;
        toolWidget->setParameterEnabled(WParameter::Sixth, value);
        break;
    }

    handler->updateCursor();
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

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
        if (dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal) {
            if (toolWidget->isParameterSet(WParameter::Third)) {
                double length = toolWidget->getParameter(WParameter::Third);
                onSketchPos.x = dHandler->firstCorner.x + length;
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double width = toolWidget->getParameter(WParameter::Fourth);
                onSketchPos.y = dHandler->firstCorner.y + width;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third)) {
                double length = toolWidget->getParameter(WParameter::Third);
                onSketchPos.x = dHandler->center.x + length/2;
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double width = toolWidget->getParameter(WParameter::Fourth);
                onSketchPos.y = dHandler->center.y + width / 2;
            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->roundCorners) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {
                double radius = toolWidget->getParameter(WParameter::Fifth);
                if (dHandler->firstCorner.x - dHandler->thirdCorner.x > 0.)
                    onSketchPos.x = dHandler->firstCorner.x - radius;
                else
                    onSketchPos.x = dHandler->firstCorner.x + radius;
                onSketchPos.y = dHandler->firstCorner.y;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Sixth)) {
                double thickness = toolWidget->getParameter(WParameter::Sixth);
                if (dHandler->firstCorner.x - dHandler->thirdCorner.x > 0.)
                    onSketchPos.x = dHandler->firstCorner.x + thickness;
                else
                    onSketchPos.x = dHandler->firstCorner.x - thickness;
                onSketchPos.y = dHandler->firstCorner.y;
            }
        }
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (toolWidget->isParameterSet(WParameter::Sixth)) {
            double thickness = toolWidget->getParameter(WParameter::Sixth);
            if (dHandler->firstCorner.x - dHandler->thirdCorner.x > 0.)
                onSketchPos.x = dHandler->firstCorner.x + thickness;
            else
                onSketchPos.x = dHandler->firstCorner.x - thickness;
            onSketchPos.y = dHandler->firstCorner.y;
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {

    // If checkboxes need synchronisation (they were changed by the DSH, e.g. by using 'M' to switch construction method), synchronise them and return.
    if(syncCheckboxToHandler(WCheckbox::FirstBox, dHandler->roundCorners))
        return;

    if(syncCheckboxToHandler(WCheckbox::SecondBox, dHandler->makeFrame))
        return;


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
        if (dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x - dHandler->firstCorner.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y - dHandler->firstCorner.y);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, fabs(onSketchPos.x - dHandler->center.x)*2);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, fabs(onSketchPos.y - dHandler->center.y)*2);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->roundCorners) {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, dHandler->radius);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Sixth))
                toolWidget->updateVisualValue(WParameter::Sixth, dHandler->thickness);
        }
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (!toolWidget->isParameterSet(WParameter::Sixth))
            toolWidget->updateVisualValue(WParameter::Sixth, dHandler->thickness);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
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

                if (dHandler->roundCorners || dHandler->makeFrame)
                    handler->setState(SelectMode::SeekThird);
                else
                    handler->setState(SelectMode::End);
            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->roundCorners && toolWidget->isParameterSet(WParameter::Fifth)) {

            if (dHandler->makeFrame)
                handler->setState(SelectMode::SeekFourth);
            else
                handler->setState(SelectMode::End);
        }
        else if (dHandler->makeFrame && toolWidget->isParameterSet(WParameter::Sixth)) {

            handler->setState(SelectMode::End);
        }
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (toolWidget->isParameterSet(WParameter::Sixth)) {
            handler->setState(SelectMode::End);
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::addConstraints() {
    int firstCurve = dHandler->firstCurve;

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);
    auto length = toolWidget->getParameter(WParameter::Third);
    auto width = toolWidget->getParameter(WParameter::Fourth);
    auto radius = toolWidget->getParameter(WParameter::Fifth);
    auto thickness = toolWidget->getParameter(WParameter::Sixth);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);
    auto lengthSet = toolWidget->isParameterSet(WParameter::Third);
    auto widthSet = toolWidget->isParameterSet(WParameter::Fourth);
    auto radiusSet = toolWidget->isParameterSet(WParameter::Fifth);
    auto thicknessSet = toolWidget->isParameterSet(WParameter::Sixth);

    using namespace Sketcher;

    int firstPointId = firstCurve;
    if (handler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal) {
        if (dHandler->radius > Precision::Confusion())
            firstPointId = dHandler->constructionPointOneId;
    }
    else {
        firstPointId = dHandler->centerPointId;
    }

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstPointId,PointPos::start), GeoElementId::VAxis, x0, handler->sketchgui->getObject());
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstPointId,PointPos::start), GeoElementId::HAxis, y0, handler->sketchgui->getObject());
    };

    auto constraintlength = [&]() {
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                firstCurve + 1, 1, firstCurve + 3, 2, fabs(length));
    };

    auto constraintwidth = [&]() {
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                        firstCurve, 1, firstCurve + 2, 2, fabs(width));
    };

    // NOTE: if AutoConstraints is empty, we can add constraints directly without any diagnose. No diagnose was run.
    if(handler->AutoConstraints.empty()) {
        if(x0set)
            constraintx0();

        if(y0set)
            constrainty0();

        if (lengthSet)
            constraintlength();

        if (widthSet)
            constraintwidth();
    }
    else { // There is a valid diagnose.
        auto firstpointinfo = handler->getPointInfo(GeoElementId(firstPointId, PointPos::start));

        if(x0set && firstpointinfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition

            firstpointinfo = handler->getPointInfo(GeoElementId(firstPointId, PointPos::start)); // get updated point position
        }

        if(y0set && firstpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition
        }

        if (lengthSet) {
            auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 1, PointPos::start));
            auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 3, PointPos::end));

            int DoFs = startpointinfo.getDoFs();
            DoFs += endpointinfo.getDoFs();

            if(DoFs > 0) {
                constraintlength();
            }

            handler->diagnoseWithAutoConstraints();
        }

        if (widthSet) {
            auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve , PointPos::start));
            auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve + 2, PointPos::end));

            int DoFs = startpointinfo.getDoFs();
            DoFs += endpointinfo.getDoFs();

            if(DoFs > 0) {
                constraintwidth();
            }
        }
    }

    // NOTE: As of today, there are no autoconstraints on the radius or on the frame thickness, therefore, they are necessarily constrainable were applicable.

    if (radiusSet && radius > Precision::Confusion())
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
            firstCurve + 5, radius);

    if (thicknessSet) {
        if(dHandler->firstCorner.y - dHandler->firstCornerFrame.y < 0)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
                firstCurve, 1, firstCurve + (dHandler->roundCorners == true ? 8 : 4), 1, fabs(thickness));
        else
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
                firstCurve + (dHandler->roundCorners == true ? 8 : 4), 1, firstCurve, 1, fabs(thickness));
    }
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerRectangle_H

