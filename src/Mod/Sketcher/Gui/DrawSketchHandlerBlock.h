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

#include <QMap>
#include <QFileInfo>

#include <App/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>
#include <Gui/FileDialog.h>

#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/GroupGeometry.h>

#include "DrawSketchDefaultWidgetController.h"
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

using DSHBlockController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerBlock,
    /*SelectModeT*/ StateMachines::TwoSeekEnd,
    /*PAutoConstraintSize =*/2,
    /*OnViewParametersT =*/OnViewParameters<4, 4>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0, 0>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<2, 2>,  // NOLINT
    /*WidgetLineEditsT =*/WidgetLineEdits<0, 0>,    // NOLINT
    ConstructionMethods::BlockConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHBlockControllerBase = DSHBlockController::ControllerBase;

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

                seekAndRenderAutoConstraint(sugConstraints[1], onSketchPos, onSketchPos - startPoint);
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
        generateAutoConstraintsOnElement(ac2, handleId, Sketcher::PointPos::end);

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
        return std::make_unique<SketcherToolDefaultWidget>();
    }

    bool isWidgetVisible() const override
    {
        return true;
    };

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_InsertBlock");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Block Parameters"));
    }

    bool canGoToNextMode() override
    {
        if (fileName.empty()
            || (state() == SelectMode::SeekSecond
                && (length < Precision::Confusion() || ShapeGeometry.empty()))) {
            // Prevent validation of null Block.
            return false;
        }
        return true;
    }

    void angleSnappingControl() override
    {
        if (state() == SelectMode::SeekSecond) {
            setAngleSnapping(true, startPoint);
        }

        else {
            setAngleSnapping(false);
        }
    }

private:
    QMap<QString, QString> pathMap;
    Base::Vector2d startPoint, endPoint;
    double length;
    int handleId;

    std::string fileName;
    std::vector<std::unique_ptr<Part::Geometry>> cachedGeometry;

    void loadFile(const QString& path)
    {
        fileName.clear();
        cachedGeometry.clear();
        if (path.isEmpty()) {
            return;
        }
        try {
            cachedGeometry = readBlockGeometry(path.toStdString());
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
        ShapeGeometry = Sketcher::transformGroupGeometry(
            source,
            toVector3d(startPoint),
            toVector3d(endPoint),
            constructionMethod() == ConstructionMethod::Height
        );

        // 3. Set construction mode on the newly created geometry
        if (isConstructionMode() && !onlyeditoutline) {
            for (auto& geo : ShapeGeometry) {
                Sketcher::GeometryFacade::setConstruction(geo.get(), true);
            }
        }
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
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
        case OnViewParameter::Fourth:
            return SelectMode::SeekSecond;
            break;
        default:
            THROWM(Base::ValueError, "Label index without an associated machine state")
    }
}

template<>
void DSHBlockController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {
            QApplication::translate("TaskSketcherTool_Block", "Width"),
            QApplication::translate("TaskSketcherTool_Block", "Height")
        };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        toolWidget->setComboboxLabel(
            WCombobox::SecondCombo,
            QApplication::translate("TaskSketcherTool_Block", "Block")
        );

        // 1. Scan for block files and store the map
        handler->pathMap = findAvailableBlockFiles();

        // 2. Populate combobox with friendly names (the keys of the map)
        QStringList blocksNames = handler->pathMap.keys();
        blocksNames.sort(Qt::CaseInsensitive);
        blocksNames.append(QApplication::translate("TaskSketcherTool_Block", "Choose file…"));
        toolWidget->setComboboxElements(WCombobox::SecondCombo, blocksNames);
        if (!handler->pathMap.isEmpty()) {
            handler->loadFile(handler->pathMap.value(blocksNames.first()));
        }
    }

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

template<>
void DSHBlockController::adaptDrawingToComboboxChange(int comboboxindex, int value)
{
    if (comboboxindex == WCombobox::FirstCombo) {
        handler->setConstructionMethod(static_cast<ConstructionMethod>(value));
    }
    else if (comboboxindex == WCombobox::SecondCombo) {
        const QString name = toolWidget->getComboboxCurrentText(WCombobox::SecondCombo);
        QString path = handler->pathMap.value(name);
        if (path.isEmpty() && value >= 0) {
            path = Gui::FileDialog::getOpenFileName(
                toolWidget,
                QObject::tr("Insert Block"),
                QString::fromStdString(App::Application::getResourceDir() + "Mod/Sketcher/Blocks/"),
                {{QObject::tr("Sketcher block files"), {QStringLiteral("*.txt")}}}
            );
        }
        handler->loadFile(path);
        // The redraw is handled by the controller's finishControlsChanged()
    }
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

            if (thirdParam->isSet) {
                length = thirdParam->getValue();
                if (length < Precision::Confusion()) {
                    unsetOnViewParameter(thirdParam.get());
                    return;
                }

                onSketchPos = handler->startPoint + length * dir.Normalize();
            }

            if (fourthParam->isSet) {
                double angle = Base::toRadians(fourthParam->getValue());
                if (handler->constructionMethod() == ConstructionMethod::Height) {
                    angle += M_PI * 0.5;
                }
                Base::Vector2d dir(cos(angle), sin(angle));
                onSketchPos.ProjectToLine(onSketchPos - handler->startPoint, dir);
                onSketchPos += handler->startPoint;
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

template<>
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

            double range;
            if (handler->constructionMethod() == ConstructionMethod::Height) {
                Base::Vector2d norm(vec.y, -vec.x);
                Base::Vector2d textAlignPoint = handler->startPoint + norm;
                range = (textAlignPoint - handler->startPoint).Angle();
            }
            else {
                range = (handler->endPoint - handler->startPoint).Angle();
            }


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

template<>
void DSHBlockController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->hasFinishedEditing && secondParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::SeekSecond);
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

template<>
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
    auto p3set = onViewParameters[OnViewParameter::Third]->isSet;
    auto p4set = onViewParameters[OnViewParameter::Fourth]->isSet;

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
        if (handler->constructionMethod() == ConstructionMethod::Height) {
            angle += M_PI * 0.5;
        }

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
         {{QObject::tr("%1 pick bottom-left point"), {Gui::InputHint::UserInput::MouseLeft}},
          switchHint}},
        {static_cast<int>(ConstructionMethod::Height),
         1,
         {{QObject::tr("%1 pick top-left point"), {Gui::InputHint::UserInput::MouseLeft}},
          switchHint}},
        {static_cast<int>(ConstructionMethod::Width),
         0,
         {{QObject::tr("%1 pick bottom-left point"), {Gui::InputHint::UserInput::MouseLeft}},
          switchHint}},
        {static_cast<int>(ConstructionMethod::Width),
         1,
         {{QObject::tr("%1 pick bottom-right point"), {Gui::InputHint::UserInput::MouseLeft}},
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
