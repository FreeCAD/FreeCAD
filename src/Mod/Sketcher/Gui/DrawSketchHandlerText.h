// SPDX - License - Identifier: LGPL - 2.1 - or -later
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


#ifndef SKETCHERGUI_DrawSketchHandlerText_H
#define SKETCHERGUI_DrawSketchHandlerText_H

#include <QMap>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"
#include "CommandConstraints.h"

#include <vector>
#include <algorithm>

namespace SketcherGui
{

class DrawSketchHandlerText;

namespace ConstructionMethods
{

enum class TextConstructionMethod
{
    Width,
    Height,
    End  // Must be the last one
};

}  // namespace ConstructionMethods

using DSHTextController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerText,
    /*SelectModeT*/ StateMachines::TwoSeekEnd,
    /*PAutoConstraintSize =*/2,
    /*OnViewParametersT =*/OnViewParameters<4, 4>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0, 0>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<2, 2>,  // NOLINT
    /*WidgetLineEditsT =*/WidgetLineEdits<1, 1>,    // NOLINT
    ConstructionMethods::TextConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHTextControllerBase = DSHTextController::ControllerBase;

using DrawSketchHandlerTextBase = DrawSketchControllableHandler<DSHTextController>;


class DrawSketchHandlerText: public DrawSketchHandlerTextBase
{
    friend DSHTextController;
    friend DSHTextControllerBase;

public:
    explicit DrawSketchHandlerText(ConstructionMethod constrMethod = ConstructionMethod::Width)
        : DrawSketchHandlerTextBase(constrMethod)
        , length(0.0)
        , handleId(0)
        , text("AstoCAD")
        , font("")
        , cachedTextName("")
        , cachedFontName("")
        , cachedBaseShapes({}) {};
    ~DrawSketchHandlerText() override = default;

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
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch Text"));

            // Add the Handle Line
            Gui::cmdAppObjectArgs(
                getSketchObject(),
                "addGeometry(Part.LineSegment(App.Vector(%f, %f,0), App.Vector(%f, %f,0)), True)",
                startPoint.x,
                startPoint.y,
                endPoint.x,
                endPoint.y
            );
            handleId = getHighestCurveIndex();

            std::string escText = escapeForPython(text);
            std::string escFont = escapeForPython(font);
            bool isHeight = constructionMethod() == ConstructionMethod::Height;
            const char* constrBoolStr = isConstructionMode() ? "True" : "False";
            const char* heightBoolStr = isHeight ? "True" : "False";

            // Add the 'Text' Constraint (Empty)
            // We initialize the constraint containing ONLY the handle (element 0).
            // We do not add the text geometry manually to avoid floating-point precision loss
            // associated with Python serialization.
            Gui::cmdAppObjectArgs(
                getSketchObject(),
                "addConstraint(Sketcher.Constraint('Text', [%d, 0], '%s', '%s', %s))",
                handleId,
                escText.c_str(),
                escFont.c_str(),
                heightBoolStr
            );

            // Generate Text Geometry by calling setTextAndFont on the new constraint.
            // This triggers the C++ logic to generate the exact geometry and insert it
            // into the sketch, ensuring closed wires and perfect precision.
            Gui::cmdAppObjectArgs(
                getSketchObject(),
                "setTextAndFont(len(App.ActiveDocument.getObject('%s').Constraints)-1, '%s', '%s', "
                "%s, %s)",
                getSketchObject()->getNameInDocument(),
                escText.c_str(),
                escFont.c_str(),
                heightBoolStr,
                constrBoolStr
            );

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add text")
            );

            Gui::Command::abortCommand();
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
        return "DSH_Text";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Text.svg");
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

    bool isWidgetVisible() const override
    {
        return true;  // Text tool must show the line edit to make sense
    };

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_CreateText");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Text parameters"));
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekSecond && length < Precision::Confusion()) {
            // Prevent validation of null Text.
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
    QMap<QString, QString> fontPathMap;
    Base::Vector2d startPoint, endPoint;
    double length;
    int handleId;

    std::string text;
    std::string font;
    std::string cachedTextName;
    std::string cachedFontName;
    std::vector<TopoDS_Shape> cachedBaseShapes;

    void createShape(bool onlyeditoutline) override
    {
        ShapeGeometry.clear();

        Base::Vector2d vecL = endPoint - startPoint;
        length = vecL.Length();
        if (length < Precision::Confusion()) {
            return;
        }

        // 1. Check if the cache is valid. If the user selected a new file,
        // or if the cache is empty, we need to re-load from the SVG.
        if (cachedTextName != text || cachedFontName != font || cachedBaseShapes.empty()) {
            if (!font.empty()) {
                cachedTextName = text;
                cachedFontName = font;
                // This is the one-time slow operation to get the template shapes.
                cachedBaseShapes = Part::makeTextWires(text, font);
            }
            else {
                cachedBaseShapes.clear();
            }
        }

        // 2. Call the generic helper to transform and create the final geometry.
        transformAndConvertToGeometry(
            ShapeGeometry,
            cachedBaseShapes,
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
        return lookupTextHints(static_cast<int>(constructionMethod()), static_cast<int>(state()));
    }

    struct HintEntry
    {
        int constructionMethod;
        int state;
        std::list<Gui::InputHint> hints;
    };

    using HintTable = std::vector<HintEntry>;

    static Gui::InputHint switchModeHint();
    static HintTable getTextHintTable();
    static std::list<Gui::InputHint> lookupTextHints(int method, int state);
};

template<>
auto DSHTextControllerBase::getState(int labelindex) const
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
void DSHTextController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {
            QApplication::translate("TaskSketcherTool_c1_text", "Width"),
            QApplication::translate("TaskSketcherTool_c1_text", "Height")
        };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        toolWidget->setLineEditLabel(
            WLineEdit::FirstEdit,
            QApplication::translate("TaskSketcherTool_Text", "Text")
        );
        toolWidget->setLineEditText(WLineEdit::FirstEdit, QString::fromStdString(handler->text));

        toolWidget->setComboboxLabel(
            WCombobox::SecondCombo,
            QApplication::translate("TaskSketcherTool_Text", "Font")
        );

        // 1. Scan for font files and store the map
        handler->fontPathMap = findAvailableFontFiles();

        // 2. Populate combobox with friendly names (the keys of the map)
        QStringList fontNames = handler->fontPathMap.keys();
        fontNames.sort(Qt::CaseInsensitive);
        toolWidget->setComboboxElements(WCombobox::SecondCombo, fontNames);

        // 3. Set a sensible default font
        QString defaultFontName;
        if (fontNames.contains(QString::fromUtf8("osifont-lgpl3fe"), Qt::CaseInsensitive)) {
            defaultFontName = QString::fromUtf8("osifont-lgpl3fe");
        }
        else if (fontNames.contains(QString::fromUtf8("DejaVu Sans"), Qt::CaseInsensitive)) {
            defaultFontName = QString::fromUtf8("DejaVu Sans");
        }
        else if (fontNames.contains(QString::fromUtf8("Arial"), Qt::CaseInsensitive)) {
            defaultFontName = QString::fromUtf8("Arial");
        }
        else if (!fontNames.isEmpty()) {
            defaultFontName = fontNames.first();
        }

        if (!defaultFontName.isEmpty()) {
            // Find the actual case-sensitive key
            for (const auto& key : fontNames) {
                if (key.compare(defaultFontName, Qt::CaseInsensitive) == 0) {
                    handler->font = handler->fontPathMap.value(key).toStdString();
                    toolWidget->setComboboxCurrentText(WCombobox::SecondCombo, key);
                    break;
                }
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

    toolWidget->setLineEditText(
        SketcherToolDefaultWidget::LineEdit::FirstEdit,
        QString::fromStdString(handler->text)
    );
}

template<>
void DSHTextController::adaptDrawingToLineEditTextChange(int lineeditindex, const QString& value)
{
    if (lineeditindex == WLineEdit::FirstEdit) {
        handler->text = value.toStdString();
        // The redraw is handled by the controller's finishControlsChanged()
    }
}

template<>
void DSHTextController::adaptDrawingToComboboxChange(int comboboxindex, int value)
{
    if (comboboxindex == WCombobox::FirstCombo) {
        handler->setConstructionMethod(static_cast<ConstructionMethod>(value));
    }
    else if (comboboxindex == WCombobox::SecondCombo) {
        // Get the selected friendly name
        QString fontName = toolWidget->getComboboxCurrentText(WCombobox::SecondCombo);
        // Look up the full path in our map and update the handler
        if (handler->fontPathMap.contains(fontName)) {
            handler->font = handler->fontPathMap.value(fontName).toStdString();
        }
        // The redraw is handled by the controller's finishControlsChanged()
    }
}

template<>
void DSHTextControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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
void DSHTextController::adaptParameters(Base::Vector2d onSketchPos)
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
            else if (vec.Length() > Precision::Confusion()) {
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
void DSHTextController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->isSet && secondParam->isSet) {
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
void DSHTextController::addConstraints()
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

Gui::InputHint DrawSketchHandlerText::switchModeHint()
{
    return {QObject::tr("%1 switch mode"), {Gui::InputHint::UserInput::KeyM}};
}

DrawSketchHandlerText::HintTable DrawSketchHandlerText::getTextHintTable()
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

std::list<Gui::InputHint> DrawSketchHandlerText::lookupTextHints(int method, int state)
{
    const auto TextHintTable = getTextHintTable();

    auto it = std::find_if(
        TextHintTable.begin(),
        TextHintTable.end(),
        [method, state](const HintEntry& entry) {
            return entry.constructionMethod == method && entry.state == state;
        }
    );

    return (it != TextHintTable.end()) ? it->hints : std::list<Gui::InputHint> {};
}

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerText_H
