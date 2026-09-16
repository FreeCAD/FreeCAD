// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
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


#ifndef SKETCHERGUI_DrawSketchHandlerBlock_H
#define SKETCHERGUI_DrawSketchHandlerBlock_H

#include <QFileInfo>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

#include <App/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>
#include <Gui/FileDialog.h>

#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/GroupGeometry.h>

#include "DrawSketchController.h"
#include "SketcherBlockWidget.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"
#include "CommandConstraints.h"

#include <vector>
#include <algorithm>

namespace SketcherGui
{

class DrawSketchHandlerBlock;

namespace ConstructionMethods
{

enum class BlockConstructionMethod
{
    Width,
    Height,
    End  // Must be the last one
};

}  // namespace ConstructionMethods

using DSHBlockControllerBase = DrawSketchController<
    DrawSketchHandlerBlock,
    StateMachines::TwoSeekEnd,
    2,
    OnViewParameters<4, 4>,
    ConstructionMethods::BlockConstructionMethod>;

class DSHBlockController: public DSHBlockControllerBase
{
public:
    using ControllerBase = DSHBlockControllerBase;
    explicit DSHBlockController(DrawSketchHandlerBlock* handler)
        : ControllerBase(handler)
    {}
    ~DSHBlockController() override
    {
        for (const auto& connection : connections) {
            QObject::disconnect(connection);
        }
    }
    void adaptParameters(Base::Vector2d position) override;
    void computeNextDrawSketchHandlerMode() override;
    void addConstraints() override;
    void firstKeyShortcut() override
    {
        toolWidget->toggleFixedSize();
    }
    void secondKeyShortcut() override
    {
        toolWidget->toggleFixedOrientation();
    }

protected:
    void doInitControls(QWidget* widget) override;
    void doResetControls() override;

private:
    SketcherBlockWidget* toolWidget = nullptr;
    std::vector<QMetaObject::Connection> connections;
    void configureToolWidget();
    void placementChanged(int method, bool size, bool orientation);
};

using DrawSketchHandlerBlockBase = DrawSketchControllableHandler<DSHBlockController>;


class DrawSketchHandlerBlock: public DrawSketchHandlerBlockBase
{
    friend DSHBlockController;
    friend DSHBlockControllerBase;

public:
    explicit DrawSketchHandlerBlock(ConstructionMethod constrMethod = ConstructionMethod::Width)
        : DrawSketchHandlerBlockBase(constrMethod)
        , length(0.0)
        , handleId(0)
        , fileName("")
    {}
    ~DrawSketchHandlerBlock() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                startPoint = onSketchPos;
                endPoint = fixedSize ? startPoint + nativeLength() * originalDirection() : startPoint;
                CreateAndDrawShapeGeometry();

                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekSecond: {
                toolWidgetManager.drawDirectionAtCursor(onSketchPos, startPoint);

                endPoint = onSketchPos;

                try {
                    CreateAndDrawShapeGeometry();
                }
                catch (const Base::ValueError&) {
                }  // equal points while hovering raise an objection that can be safely ignored

                // A constrained placement endpoint need not coincide with the mouse target.
                sugConstraints[1].clear();
                if (!fixedSize && !fixedOrientation) {
                    seekAndRenderAutoConstraint(sugConstraints[1], onSketchPos, onSketchPos - startPoint);
                }
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            int firstCurve = getHighestCurveIndex() + 1;

            createShape(false);

            openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch block"));

            commandAddShapeGeometryAndConstraints();

            handleId = getHighestCurveIndex() + 1;  // line is not added yet

            std::vector<Sketcher::GeoElementId> elts;
            for (int i = firstCurve; i < handleId; ++i) {
                elts.push_back(Sketcher::GeoElementId(i));
            }
            if (fixedSize) {
                const double angle = (endPoint - startPoint).Angle() - originalDirection().Angle();
                Gui::Command::doCommand(Gui::Command::App, "import SketcherBlock");
                Gui::cmdAppObjectArgs(
                    getSketchObject(),
                    "addGeometry(Part.Point(App.Vector(%.17g,%.17g,0)), True)",
                    startPoint.x,
                    startPoint.y
                );
                std::string elements = "[" + std::to_string(handleId) + ", 1";
                for (int i = firstCurve; i < handleId; ++i) {
                    elements += ", " + std::to_string(i) + ", 0";
                }
                elements += "]";
                Gui::cmdAppObjectArgs(
                    getSketchObject(),
                    "addConstraint(Sketcher.Constraint('Group', %s, '%s', %s, True, %.17g))",
                    elements.c_str(),
                    escapeForPython(fileName).c_str(),
                    constructionMethod() == ConstructionMethod::Height ? "True" : "False",
                    angle
                );
                commitCommand();
                return;
            }
            bool isHeight = constructionMethod() == ConstructionMethod::Height;
            if (!addListConstraint(
                    getSketchObject(),
                    elts,
                    "Group",
                    startPoint,
                    endPoint,
                    isHeight,
                    "",
                    "",
                    fileName
                )) {
                abortCommand();
                return;
            }

            commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to insert block")
            );

            abortCommand();
            THROWM(
                Base::RuntimeError,
                QT_TRANSLATE_NOOP(
                    "Notifications",
                    "Tool execution aborted"
                ) "\n"
            )  // This prevents constraints from being
               // applied on non existing geometry
        }
    }

    void generateAutoConstraints() override
    {
        // Generate temporary autoconstraints (but do not actually add them to the sketch)
        if (avoidRedundants) {
            removeRedundantHorizontalVertical(getSketchObject(), sugConstraints[0], sugConstraints[1]);
        }

        auto& ac1 = sugConstraints[0];
        auto& ac2 = sugConstraints[1];

        generateAutoConstraintsOnElement(ac1, handleId, Sketcher::PointPos::start);
        if (!fixedSize) {
            generateAutoConstraintsOnElement(ac2, handleId, Sketcher::PointPos::end);
        }

        // Ensure temporary autoconstraints do not generate a redundancy and that the geometry
        // parameters are accurate This is particularly important for adding widget mandated
        // constraints.
        removeRedundantAutoConstraints();
    }

    void createAutoConstraints() override
    {
        // execute python command to create autoconstraints
        createGeneratedAutoConstraints(true);

        sugConstraints[0].clear();
        sugConstraints[1].clear();
    }

    std::string getToolName() const override
    {
        return "DSH_Block";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Block.svg");
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherBlockWidget>();
    }

    bool isWidgetVisible() const override
    {
        return true;
    };

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_BlockInsert");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Block Parameters"));
    }

    bool canGoToNextMode() override
    {
        if (fileName.empty()
            || ((state() == SelectMode::SeekSecond || fixedSize)
                && (length < Precision::Confusion() || ShapeGeometry.empty()))) {
            // Prevent validation of null Block.
            return false;
        }
        return true;
    }

    void onButtonPressed(Base::Vector2d onSketchPos) override
    {
        updateDataAndDrawToPosition(onSketchPos);
        if (canGoToNextMode()) {
            if (fixedSize && fixedOrientation) {
                setState(SelectMode::End);
            }
            else {
                moveToNextMode();
            }
        }
    }

    void angleSnappingControl() override
    {
        if (state() == SelectMode::SeekSecond && !fixedOrientation) {
            setAngleSnapping(true, startPoint);
        }

        else {
            setAngleSnapping(false);
        }
    }

private:
    Base::Vector2d startPoint, endPoint;
    double length;
    int handleId;
    bool fixedSize = false;
    bool fixedOrientation = false;
    double sourceWidth = 0.0;
    double sourceHeight = 0.0;
    Base::Vector3d sourceHandle;

    bool hasSourceHandle() const
    {
        return sourceHandle.Length() > Precision::Confusion();
    }

    double nativeLength() const
    {
        if (hasSourceHandle()) {
            return sourceHandle.Length();
        }
        return constructionMethod() == ConstructionMethod::Height ? sourceHeight : sourceWidth;
    }

    Base::Vector2d originalDirection() const
    {
        if (hasSourceHandle()) {
            return Base::Vector2d(sourceHandle.x, sourceHandle.y).Normalize();
        }
        return constructionMethod() == ConstructionMethod::Height ? Base::Vector2d(0.0, 1.0)
                                                                  : Base::Vector2d(1.0, 0.0);
    }

    std::string fileName;
    std::vector<std::unique_ptr<Part::Geometry>> cachedGeometry;

    void loadFile(const QString& path)
    {
        fileName.clear();
        cachedGeometry.clear();
        sourceWidth = sourceHeight = 0.0;
        sourceHandle = Base::Vector3d();
        fixedSize = false;
        if (path.isEmpty()) {
            return;
        }
        try {
            cachedGeometry = readBlockGeometry(path.toStdString(), &fixedSize, &sourceHandle);
            Bnd_Box bounds;
            for (const auto& geo : cachedGeometry) {
                BRepBndLib::AddOptimal(geo->toShape(), bounds, false, false);
            }
            if (bounds.IsVoid() || bounds.IsOpen()) {
                throw Base::ValueError("The source file contains no finite geometry");
            }
            double xmin, ymin, zmin, xmax, ymax, zmax;
            bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            sourceWidth = xmax - xmin;
            sourceHeight = ymax - ymin;
            fileName = QFileInfo(path).absoluteFilePath().toStdString();
        }
        catch (const Base::Exception& error) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Cannot read block"),
                error.what()
            );
        }
    }

    void createShape(bool onlyeditoutline) override
    {
        ShapeGeometry.clear();

        Base::Vector2d vecL = endPoint - startPoint;
        length = vecL.Length();
        if (length < Precision::Confusion()) {
            return;
        }

        if (fileName.empty()) {
            return;
        }
        std::vector<Part::Geometry*> source;
        for (const auto& geo : cachedGeometry) {
            source.push_back(geo.get());
        }
        if (fixedSize) {
            const double angle = vecL.Angle() - originalDirection().Angle();
            ShapeGeometry
                = Sketcher::transformFixedGroupGeometry(source, toVector3d(startPoint), angle);
        }
        else {
            ShapeGeometry = Sketcher::transformGroupGeometry(
                source,
                toVector3d(startPoint),
                toVector3d(endPoint),
                constructionMethod() == ConstructionMethod::Height,
                true,
                sourceHandle
            );
        }
        // 3. Set construction mode on the newly created geometry
        if (isConstructionMode() && !onlyeditoutline) {
            for (auto& geo : ShapeGeometry) {
                Sketcher::GeometryFacade::setConstruction(geo.get(), true);
            }
        }
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
        if (hasSourceHandle()) {
            if (state() == SelectMode::SeekFirst) {
                return {{QObject::tr("%1 place block origin"), {Gui::InputHint::UserInput::MouseLeft}}};
            }
            return {
                {fixedSize ? QObject::tr("%1 set orientation")
                           : QObject::tr("%1 place the end of the block handle"),
                 {Gui::InputHint::UserInput::MouseLeft}}
            };
        }
        if (state() == SelectMode::SeekFirst && fixedSize && !fixedOrientation) {
            return {
                {QObject::tr("%1 place block origin"), {Gui::InputHint::UserInput::MouseLeft}},
                switchModeHint()
            };
        }
        if (state() == SelectMode::SeekFirst && fixedSize && fixedOrientation) {
            return {
                {QObject::tr("%1 place block"), {Gui::InputHint::UserInput::MouseLeft}},
                switchModeHint()
            };
        }
        if (state() == SelectMode::SeekSecond && (fixedSize || fixedOrientation)) {
            return {
                {fixedSize ? QObject::tr("%1 set orientation") : QObject::tr("%1 set size"),
                 {Gui::InputHint::UserInput::MouseLeft}},
                switchModeHint()
            };
        }
        return lookupBlockHints(static_cast<int>(constructionMethod()), static_cast<int>(state()));
    }

    struct HintEntry
    {
        int constructionMethod;
        int state;
        std::list<Gui::InputHint> hints;
    };

    using HintTable = std::vector<HintEntry>;

    static Gui::InputHint switchModeHint();
    static HintTable getBlockHintTable();
    static std::list<Gui::InputHint> lookupBlockHints(int method, int state);
};

template<>
auto DSHBlockControllerBase::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
        case OnViewParameter::Second:
            return SelectMode::SeekFirst;
            break;
        case OnViewParameter::Third:
            return handler->fixedSize ? SelectMode::End : SelectMode::SeekSecond;
        case OnViewParameter::Fourth:
            return handler->fixedOrientation ? SelectMode::End : SelectMode::SeekSecond;
            break;
        default:
            THROWM(Base::ValueError, "Label index without an associated machine state")
    }
}

void DSHBlockController::doInitControls(QWidget* widget)
{
    toolWidget = static_cast<SketcherBlockWidget*>(widget);
    connections.push_back(
        QObject::connect(
            toolWidget,
            &SketcherBlockWidget::fileSelected,
            toolWidget,
            [this](const QString& path) {
                handler->loadFile(path);
                unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                configureToolWidget();
                handler->updateHint();
                onHandlerModeChanged();
                finishControlsChanged();
            }
        )
    );
    connections.push_back(
        QObject::connect(
            toolWidget,
            &SketcherBlockWidget::placementChanged,
            toolWidget,
            [this](int method, bool size, bool orientation) {
                placementChanged(method, size, orientation);
            }
        )
    );
    handler->loadFile(toolWidget->selectedFile());
}

void DSHBlockController::doResetControls()
{
    ControllerBase::doResetControls();
    configureToolWidget();
}

void DSHBlockController::configureToolWidget()
{
    toolWidget->setPlacementOptions(
        static_cast<int>(handler->constructionMethod()),
        handler->fixedSize,
        handler->fixedOrientation,
        handler->hasSourceHandle()
    );
    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

    onViewParameters[OnViewParameter::Third]->setLabelType(
        Gui::SoDatumLabel::DISTANCE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
}

void DSHBlockController::placementChanged(int method, bool size, bool orientation)
{
    if (handler->fixedSize != size) {
        handler->fixedSize = size;
        unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
    }
    if (handler->fixedOrientation != orientation) {
        handler->fixedOrientation = orientation;
        unsetOnViewParameter(onViewParameters[OnViewParameter::Fourth].get());
    }
    if (static_cast<int>(handler->constructionMethod()) != method) {
        handler->setConstructionMethod(static_cast<ConstructionMethod>(method));
    }
    handler->sugConstraints[1].clear();
    handler->angleSnappingControl();
    handler->updateHint();
    onHandlerModeChanged();
    finishControlsChanged();
}

template<>
void DSHBlockControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->isSet) {
                onSketchPos.x = firstParam->getValue();
            }

            if (secondParam->isSet) {
                onSketchPos.y = secondParam->getValue();
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            Base::Vector2d dir = onSketchPos - handler->startPoint;
            if (dir.Length() < Precision::Confusion()) {
                dir.x = 1.0;  // if direction null, default to (1,0)
            }
            double length = dir.Length();

            if (thirdParam->isSet && !handler->fixedSize) {
                length = thirdParam->getValue();
                if (length < Precision::Confusion()) {
                    unsetOnViewParameter(thirdParam.get());
                    return;
                }

                onSketchPos = handler->startPoint + length * dir.Normalize();
            }

            if (fourthParam->isSet && !handler->fixedOrientation) {
                double angle = Base::toRadians(fourthParam->getValue());
                angle += handler->originalDirection().Angle();
                Base::Vector2d dir(cos(angle), sin(angle));
                if (handler->fixedSize) {
                    onSketchPos = handler->startPoint + handler->nativeLength() * dir;
                }
                else {
                    onSketchPos.ProjectToLine(onSketchPos - handler->startPoint, dir);
                    onSketchPos += handler->startPoint;
                }
            }

            if (handler->fixedSize || handler->fixedOrientation) {
                dir = onSketchPos - handler->startPoint;
                length = handler->fixedSize ? handler->nativeLength() : dir.Length();
                if (handler->fixedOrientation || dir.Length() < Precision::Confusion()) {
                    dir = handler->originalDirection();
                }
                onSketchPos = handler->startPoint + length * dir.Normalize();
            }

            if (thirdParam->isSet && fourthParam->isSet
                && (onSketchPos - handler->startPoint).Length() < Precision::Confusion()) {
                unsetOnViewParameter(thirdParam.get());
                unsetOnViewParameter(fourthParam.get());
            }
        } break;
        default:
            break;
    }
}

void DSHBlockController::adaptParameters(Base::Vector2d onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (!firstParam->isSet) {
                setOnViewParameterValue(OnViewParameter::First, onSketchPos.x);
            }

            if (!secondParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Second, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            firstParam->setLabelAutoDistanceReverse(!sameSign);
            secondParam->setLabelAutoDistanceReverse(sameSign);
            firstParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
            secondParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            Base::Vector3d start = toVector3d(handler->startPoint);
            Base::Vector3d end = toVector3d(handler->endPoint);
            Base::Vector3d vec = end - start;

            if (!thirdParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, vec.Length());
            }

            const double angle = (handler->endPoint - handler->startPoint).Angle()
                - handler->originalDirection().Angle();
            const double range = std::atan2(std::sin(angle), std::cos(angle));


            if (!fourthParam->isSet) {
                setOnViewParameterValue(
                    OnViewParameter::Fourth,
                    Base::toDegrees(range),
                    Base::Unit::Angle
                );
            }
            else if (fourthParam->hasFinishedEditing && vec.Length() > Precision::Confusion()) {
                double ovpRange = Base::toRadians(fourthParam->getValue());
                if (fabs(range - ovpRange) > Precision::Confusion()) {
                    setOnViewParameterValue(
                        OnViewParameter::Fourth,
                        Base::toDegrees(range),
                        Base::Unit::Angle
                    );
                }
            }

            thirdParam->setPoints(start, end);
            fourthParam->setPoints(start, Base::Vector3d());
            fourthParam->setLabelRange(range);
        } break;
        default:
            break;
    }
}

void DSHBlockController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->hasFinishedEditing && secondParam->hasFinishedEditing) {
                handler->setNextState(
                    handler->fixedSize && handler->fixedOrientation ? SelectMode::End
                                                                    : SelectMode::SeekSecond
                );
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if ((!handler->fixedSize || !handler->fixedOrientation)
                && (handler->fixedSize || thirdParam->hasFinishedEditing)
                && (handler->fixedOrientation || fourthParam->hasFinishedEditing)) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

void DSHBlockController::addConstraints()
{
    App::DocumentObject* obj = handler->sketchgui->getObject();

    int firstCurve = handler->handleId;

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();
    auto p3 = onViewParameters[OnViewParameter::Third]->getValue();
    auto p4 = onViewParameters[OnViewParameter::Fourth]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
    auto p3set = !handler->fixedSize && onViewParameters[OnViewParameter::Third]->isSet;
    auto p4set = !handler->fixedSize && !handler->fixedOrientation
        && onViewParameters[OnViewParameter::Fourth]->isSet;

    using namespace Sketcher;

    auto constraintToOrigin = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::RtPnt, x0, obj);
    };

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::VAxis, x0, obj);
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::HAxis, y0, obj);
    };

    auto constraintp3length = [&]() {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
            firstCurve,
            fabs(p3)
        );
    };

    auto constraintp4angle = [&]() {
        double angle = Base::toRadians(p4);
        angle += handler->originalDirection().Angle();

        ConstraintLineByAngle(firstCurve, angle, obj);
    };

    if (handler->AutoConstraints.empty()) {  // No valid diagnosis. Every constraint can be added.

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            constraintToOrigin();
        }
        else {
            if (x0set) {
                constraintx0();
            }

            if (y0set) {
                constrainty0();
            }
        }

        if (p3set) {
            constraintp3length();
        }

        if (p4set) {
            constraintp4angle();
        }
    }
    else {  // Valid diagnosis. Must check which constraints may be added.
        auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start));

        if (x0set && startpointinfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition

            startpointinfo = handler->getPointInfo(
                GeoElementId(firstCurve, PointPos::start)
            );  // get updated point position
        }

        if (y0set && startpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition

            startpointinfo = handler->getPointInfo(
                GeoElementId(firstCurve, PointPos::start)
            );  // get updated point position
        }

        if (handler->fixedSize) {
            return;
        }
        auto endpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::end));

        int DoFs = startpointinfo.getDoFs();
        DoFs += endpointinfo.getDoFs();

        if (p3set && DoFs > 0) {
            constraintp3length();
            DoFs--;
        }

        if (p4set && DoFs > 0) {
            constraintp4angle();
        }
    }
}

Gui::InputHint DrawSketchHandlerBlock::switchModeHint()
{
    return {QObject::tr("%1 switch mode"), {Gui::InputHint::UserInput::KeyM}};
}

DrawSketchHandlerBlock::HintTable DrawSketchHandlerBlock::getBlockHintTable()
{
    const auto switchHint = switchModeHint();
    return {
        // Structure: {constructionMethod, state, {hints...}}
        {static_cast<int>(ConstructionMethod::Height),
         0,
         {{QObject::tr("%1 place block origin"), {Gui::InputHint::UserInput::MouseLeft}}, switchHint}},
        {static_cast<int>(ConstructionMethod::Height),
         1,
         {{QObject::tr("%1 set height and orientation"), {Gui::InputHint::UserInput::MouseLeft}},
          switchHint}},
        {static_cast<int>(ConstructionMethod::Width),
         0,
         {{QObject::tr("%1 place block origin"), {Gui::InputHint::UserInput::MouseLeft}}, switchHint}},
        {static_cast<int>(ConstructionMethod::Width),
         1,
         {{QObject::tr("%1 set width and orientation"), {Gui::InputHint::UserInput::MouseLeft}},
          switchHint}}
    };
}

std::list<Gui::InputHint> DrawSketchHandlerBlock::lookupBlockHints(int method, int state)
{
    const auto BlockHintTable = getBlockHintTable();

    auto it = std::find_if(
        BlockHintTable.begin(),
        BlockHintTable.end(),
        [method, state](const HintEntry& entry) {
            return entry.constructionMethod == method && entry.state == state;
        }
    );

    return (it != BlockHintTable.end()) ? it->hints : std::list<Gui::InputHint> {};
}

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerBlock_H
