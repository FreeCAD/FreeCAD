// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
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

#include <QTimer>
#include <QVBoxLayout>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Command.h>

#include <Mod/Part/App/PolarPatternExtension.h>

#include "PatternParametersWidget.h"
#include "PatternCircularParametersWidget.h"
#include "PatternPathParametersWidget.h"
#include "PatternPointParametersWidget.h"
#include "TaskPatternParameters.h"

using namespace PartGui;

TaskPatternParameters::TaskPatternParameters() = default;

TaskPatternParameters::~TaskPatternParameters() = default;

void TaskPatternParameters::setupPatternParameterUI(
    QWidget* parent,
    QWidget* firstPlaceholder,
    QWidget* secondPlaceholder,
    Gui::View3DInventorViewer* viewer,
    QObject* signalContext,
    int updateViewTimeout
)
{
    auto* pattern = getPatternObject();
    if (!pattern) {
        return;
    }

    auto* linear = dynamic_cast<Part::LinearPatternExtension*>(pattern);
    auto* polar = dynamic_cast<Part::PolarPatternExtension*>(pattern);
    if (!linear && !polar) {
        Base::Console().warning(
            "Pattern task panel property binding failed. Unsupported pattern object.\n"
        );
        return;
    }

    PartGui::PatternType type = linear ? PartGui::PatternType::Linear : PartGui::PatternType::Polar;

    parametersWidget = new PartGui::PatternParametersWidget(type, viewer, parent);
    auto* placeholderLayout = new QVBoxLayout(firstPlaceholder);
    placeholderLayout->setContentsMargins(0, 0, 0, 0);
    placeholderLayout->addWidget(parametersWidget);
    firstPlaceholder->setLayout(placeholderLayout);

    fillDirectionCombo(parametersWidget->dirLinks, Part::LinearPatternDirection::First);
    QObject::connect(
        parametersWidget,
        &PartGui::PatternParametersWidget::requestReferenceSelection,
        signalContext,
        [this]() {
            activeDirectionWidget = parametersWidget;
            onReferenceSelectionRequested();
        }
    );
    QObject::connect(
        parametersWidget,
        &PartGui::PatternParametersWidget::parametersChanged,
        signalContext,
        [this]() { onPatternParametersChanged(); }
    );

    if (linear) {
        parametersWidget2 = new PartGui::PatternParametersWidget(type, viewer, parent);
        auto* placeholderLayout2 = new QVBoxLayout(secondPlaceholder);
        placeholderLayout2->setContentsMargins(0, 0, 0, 0);
        placeholderLayout2->addWidget(parametersWidget2);
        secondPlaceholder->setLayout(placeholderLayout2);

        fillDirectionCombo(parametersWidget2->dirLinks, Part::LinearPatternDirection::Second);
        QObject::connect(
            parametersWidget2,
            &PartGui::PatternParametersWidget::requestReferenceSelection,
            signalContext,
            [this]() {
                activeDirectionWidget = parametersWidget2;
                onReferenceSelectionRequested();
            }
        );
        QObject::connect(
            parametersWidget2,
            &PartGui::PatternParametersWidget::parametersChanged,
            signalContext,
            [this]() { onPatternParametersChanged(); }
        );
        parametersWidget2->setTitle(tr("Direction 2"));
        parametersWidget2->setCheckable(true);
    }

    bindPatternProperties();

    updateViewTimer = new QTimer(signalContext);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(updateViewTimeout);
    QObject::connect(updateViewTimer, &QTimer::timeout, signalContext, [this]() {
        onUpdateViewTimer();
    });
}

void TaskPatternParameters::setupCircularPatternParameterUI(
    QWidget* parent,
    QWidget* placeholder,
    QObject* signalContext,
    int updateViewTimeout,
    App::PropertyLinkSub* axis,
    App::PropertyLength* radialDistance,
    App::PropertyLength* tangentialDistance,
    App::PropertyIntegerConstraint* numberCircles,
    App::PropertyIntegerConstraint* symmetry
)
{
    circularParametersWidget = new PatternCircularParametersWidget(parent);
    auto* layout = new QVBoxLayout(placeholder);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(circularParametersWidget);

    fillDirectionCombo(circularParametersWidget->dirLinks, Part::LinearPatternDirection::First);
    circularParametersWidget
        ->bindProperties(axis, radialDistance, tangentialDistance, numberCircles, symmetry);

    QObject::connect(
        circularParametersWidget,
        &PatternCircularParametersWidget::requestReferenceSelection,
        signalContext,
        [this]() {
            activeDirectionWidget = circularParametersWidget;
            onReferenceSelectionRequested();
        }
    );
    QObject::connect(
        circularParametersWidget,
        &PatternCircularParametersWidget::parametersChanged,
        signalContext,
        [this]() { onPatternParametersChanged(); }
    );

    updateViewTimer = new QTimer(signalContext);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(updateViewTimeout);
    QObject::connect(updateViewTimer, &QTimer::timeout, signalContext, [this]() {
        onUpdateViewTimer();
    });
}

void TaskPatternParameters::setupPathPatternParameterUI(
    QWidget* parent,
    QWidget* placeholder,
    QObject* signalContext,
    int updateViewTimeout,
    App::PropertyLinkSub* path,
    App::PropertyIntegerConstraint* count,
    App::PropertyEnumeration* spacingMode,
    App::PropertyLength* spacing,
    App::PropertyLength* startOffset,
    App::PropertyLength* endOffset,
    App::PropertyBool* reversePath,
    App::PropertyBool* align
)
{
    pathParametersWidget = new PatternPathParametersWidget(parent);
    auto* layout = new QVBoxLayout(placeholder);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(pathParametersWidget);

    pathParametersWidget
        ->bindProperties(path, count, spacingMode, spacing, startOffset, endOffset, reversePath, align);

    QObject::connect(
        pathParametersWidget,
        &PatternPathParametersWidget::requestReferenceSelection,
        signalContext,
        [this]() {
            activeDirectionWidget = pathParametersWidget;
            onReferenceSelectionRequested();
        }
    );
    QObject::connect(
        pathParametersWidget,
        &PatternPathParametersWidget::parametersChanged,
        signalContext,
        [this]() { onPatternParametersChanged(); }
    );

    updateViewTimer = new QTimer(signalContext);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(updateViewTimeout);
    QObject::connect(updateViewTimer, &QTimer::timeout, signalContext, [this]() {
        onUpdateViewTimer();
    });
}

void TaskPatternParameters::setupPointPatternParameterUI(
    QWidget* parent,
    QWidget* placeholder,
    QObject* signalContext,
    App::PropertyLinkSub* pointObject
)
{
    pointParametersWidget = new PatternPointParametersWidget(parent);
    auto* layout = new QVBoxLayout(placeholder);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(pointParametersWidget);
    pointParametersWidget->bindProperty(pointObject);

    QObject::connect(
        pointParametersWidget,
        &PatternPointParametersWidget::requestReferenceSelection,
        signalContext,
        [this]() {
            activeDirectionWidget = pointParametersWidget;
            onReferenceSelectionRequested();
        }
    );
}

void TaskPatternParameters::bindPatternProperties()
{
    auto* pattern = getPatternObject();
    if (!pattern) {
        return;
    }

    if (auto* linear = dynamic_cast<Part::LinearPatternExtension*>(pattern)) {
        parametersWidget->bindProperties(
            &linear->Direction,
            &linear->Reversed,
            &linear->Mode,
            &linear->Length,
            &linear->Offset,
            &linear->Spacings,
            &linear->SpacingPattern,
            &linear->Occurrences,
            pattern
        );
        parametersWidget2->bindProperties(
            &linear->Direction2,
            &linear->Reversed2,
            &linear->Mode2,
            &linear->Length2,
            &linear->Offset2,
            &linear->Spacings2,
            &linear->SpacingPattern2,
            &linear->Occurrences2,
            pattern
        );
        return;
    }

    if (auto* polar = dynamic_cast<Part::PolarPatternExtension*>(pattern)) {
        parametersWidget->bindProperties(
            &polar->Axis,
            &polar->Reversed,
            &polar->Mode,
            &polar->Angle,
            &polar->Offset,
            &polar->Spacings,
            &polar->SpacingPattern,
            &polar->Occurrences,
            pattern
        );
        return;
    }

    Base::Console().warning(
        "Pattern task panel property binding failed. Unsupported pattern object.\n"
    );
}

void TaskPatternParameters::updatePatternParameterUI()
{
    if (parametersWidget) {
        parametersWidget->updateUI();
    }
    if (parametersWidget2) {
        parametersWidget2->updateUI();
    }
    if (circularParametersWidget) {
        circularParametersWidget->updateUI();
    }
    if (pathParametersWidget) {
        pathParametersWidget->updateUI();
    }
    if (pointParametersWidget) {
        pointParametersWidget->updateUI();
    }
}

void TaskPatternParameters::onUpdateViewTimer()
{
    setupPatternTransaction();
    recomputePatternFeature();
    updatePatternSpacingLabels();
    updatePatternParameterUI();
}

void TaskPatternParameters::kickUpdateViewTimer() const
{
    if (updateViewTimer) {
        updateViewTimer->start();
    }
}

bool TaskPatternParameters::consumePendingUpdate()
{
    if (updateViewTimer && updateViewTimer->isActive()) {
        updateViewTimer->stop();
        recomputePatternFeature();
        return true;
    }

    return false;
}

void TaskPatternParameters::applyPatternParameters(App::DocumentObject* pattern) const
{
    if (!pattern) {
        return;
    }

    if (pathParametersWidget) {
        std::vector<std::string> subnames;
        App::DocumentObject* object = nullptr;
        pathParametersWidget->getPath(object, subnames);
        if (object) {
            const std::string path = buildDirectionReferencePythonString(object, subnames);
            FCMD_OBJ_CMD(pattern, "Path = " << path.c_str());
        }
        pathParametersWidget->applyQuantitySpinboxes();
        return;
    }

    if (pointParametersWidget) {
        // PointObject is updated directly when the reference is selected. Unlike the other
        // pattern widgets, this panel has no deferred values to commit. Reassigning the same
        // PropertyLinkSub through a Python command during dialog acceptance can invalidate its
        // linked storage while the bound widget still exists.
        return;
    }

    if (circularParametersWidget) {
        std::vector<std::string> subnames;
        App::DocumentObject* object = nullptr;
        circularParametersWidget->getAxis(object, subnames);
        if (object || subnames.empty()) {
            const std::string direction = buildDirectionReferencePythonString(object, subnames);
            FCMD_OBJ_CMD(pattern, "Axis = " << direction.c_str());
        }
        circularParametersWidget->applyQuantitySpinboxes();
        return;
    }

    if (!parametersWidget) {
        return;
    }

    auto applyWidget = [this, pattern](
                           PartGui::PatternParametersWidget* widget,
                           const char* directionProperty,
                           const char* reversedProperty,
                           const char* modeProperty,
                           const char* spacingPatternProperty
                       ) {
        std::vector<std::string> dirs;
        App::DocumentObject* obj = nullptr;
        widget->getAxis(obj, dirs);
        if (obj || dirs.empty()) {
            std::string direction = buildDirectionReferencePythonString(obj, dirs);
            FCMD_OBJ_CMD(pattern, directionProperty << " = " << direction.c_str());
        }
        FCMD_OBJ_CMD(pattern, reversedProperty << " = " << widget->getReverse());
        FCMD_OBJ_CMD(pattern, modeProperty << " = " << widget->getMode());
        widget->applyQuantitySpinboxes();
        FCMD_OBJ_CMD(pattern, spacingPatternProperty << " = " << widget->getSpacingPatternsAsString());
    };

    if (dynamic_cast<Part::LinearPatternExtension*>(pattern)) {
        applyWidget(parametersWidget, "Direction", "Reversed", "Mode", "SpacingPattern");
        if (parametersWidget2) {
            applyWidget(parametersWidget2, "Direction2", "Reversed2", "Mode2", "SpacingPattern2");
        }
        return;
    }

    if (dynamic_cast<Part::PolarPatternExtension*>(pattern)) {
        applyWidget(parametersWidget, "Axis", "Reversed", "Mode", "SpacingPattern");
    }
}

void TaskPatternParameters::updatePatternSpacingLabels()
{
    auto* pattern = getPatternObject();
    if (!pattern) {
        return;
    }

    if (auto* linearPattern = dynamic_cast<Part::LinearPatternExtension*>(pattern)) {
        const Base::Vector3d startPoint = getPatternStartPoint();

        auto updateLinearLabels = [this, linearPattern, startPoint](
                                      PartGui::PatternParametersWidget* widget,
                                      Part::LinearPatternDirection dir,
                                      const Base::Vector3d& fallbackDirection
                                  ) {
            if (!widget) {
                return;
            }

            Base::Vector3d direction = fallbackDirection;
            try {
                gp_Vec offset = linearPattern->calculateOffsetVector(dir);
                if (offset.Magnitude() > Precision::Confusion()) {
                    offset.Normalize();
                    direction.x = offset.X();
                    direction.y = offset.Y();
                    direction.z = offset.Z();
                }
            }
            catch (const Base::Exception& e) {
                Base::Console().warning(
                    "Could not update linear pattern spacing labels: %s\n",
                    e.what()
                );
            }

            direction = transformLinearPatternDirection(direction);
            widget->updateSpacingLabels(
                startPoint,
                direction,
                getLinearPatternLabelPlaneNormal(dir)
            );
        };

        updateLinearLabels(
            parametersWidget,
            Part::LinearPatternDirection::First,
            getLinearPatternFallbackDirection(Part::LinearPatternDirection::First)
        );
        updateLinearLabels(
            parametersWidget2,
            Part::LinearPatternDirection::Second,
            getLinearPatternFallbackDirection(Part::LinearPatternDirection::Second)
        );
        return;
    }

    if (auto* polarPattern = dynamic_cast<Part::PolarPatternExtension*>(pattern)) {
        try {
            gp_Ax2 axisDef = polarPattern->getRotation();
            transformPolarPatternAxis(axisDef);

            gp_Pnt centerGp = axisDef.Location();
            gp_Dir axisGp = axisDef.Direction();
            Base::Vector3d center(centerGp.X(), centerGp.Y(), centerGp.Z());
            Base::Vector3d axis(axisGp.X(), axisGp.Y(), axisGp.Z());

            Base::Rotation labelRot(Base::Vector3d(0.0, 0.0, 1.0), axis);
            Base::Vector3d xDir;
            labelRot.multVec(Base::Vector3d(1.0, 0.0, 0.0), xDir);

            const Base::Vector3d startPoint = getPatternStartPoint();
            Base::Vector3d startRadiusVec = startPoint - center;
            double radius = startRadiusVec.Length();
            if (radius < Precision::Confusion()) {
                radius = 1.0;
            }

            double initialAngleRad = 0.0;
            Base::Vector3d projectedRadiusVec = startRadiusVec - (startRadiusVec.Dot(axis)) * axis;
            if (projectedRadiusVec.Length() > 1e-6) {
                initialAngleRad = xDir.GetAngleOriented(projectedRadiusVec, axis);
            }

            if (parametersWidget) {
                parametersWidget->updateSpacingLabels(center, axis, radius, initialAngleRad);
            }
        }
        catch (const Base::Exception& e) {
            Base::Console().warning("Could not update polar pattern spacing labels: %s\n", e.what());
        }
    }
}

PatternParametersWidget* TaskPatternParameters::getPrimaryParametersWidget() const
{
    return parametersWidget;
}

PatternParametersWidget* TaskPatternParameters::getSecondaryParametersWidget() const
{
    return parametersWidget2;
}

PatternCircularParametersWidget* TaskPatternParameters::getCircularParametersWidget() const
{
    return circularParametersWidget;
}

PatternPathParametersWidget* TaskPatternParameters::getPathParametersWidget() const
{
    return pathParametersWidget;
}

PatternPointParametersWidget* TaskPatternParameters::getPointParametersWidget() const
{
    return pointParametersWidget;
}

PatternReferenceWidget* TaskPatternParameters::getActiveDirectionWidget() const
{
    return activeDirectionWidget;
}

void TaskPatternParameters::clearActiveDirectionWidget()
{
    activeDirectionWidget = nullptr;
}

Base::Vector3d TaskPatternParameters::getPatternStartPoint() const
{
    return Base::Vector3d(0.0, 0.0, 0.0);
}

Base::Vector3d TaskPatternParameters::getLinearPatternFallbackDirection(
    Part::LinearPatternDirection direction
) const
{
    if (direction == Part::LinearPatternDirection::Second) {
        return Base::Vector3d(0.0, 1.0, 0.0);
    }

    return Base::Vector3d(1.0, 0.0, 0.0);
}

Base::Vector3d TaskPatternParameters::transformLinearPatternDirection(
    const Base::Vector3d& direction
) const
{
    return direction;
}

Base::Vector3d TaskPatternParameters::getLinearPatternLabelPlaneNormal(
    Part::LinearPatternDirection
) const
{
    return Base::Vector3d();
}

void TaskPatternParameters::transformPolarPatternAxis(gp_Ax2&) const
{}

std::string TaskPatternParameters::buildDirectionReferencePythonString(
    const App::DocumentObject* obj,
    const std::vector<std::string>& subs
) const
{
    if (!obj) {
        return "None";
    }

    if (subs.empty()) {
        return Gui::Command::getObjectCmd(obj);
    }

    const std::string sub = subs.front();
    return Gui::Command::getObjectCmd(obj, "(", ", ['") + sub + "'])";
}
