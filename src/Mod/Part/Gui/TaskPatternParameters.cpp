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
#include <Gui/Application.h>
#include <Gui/Macro.h>

#include <Mod/Part/App/PolarPatternExtension.h>

#include "PatternParametersWidget.h"
#include "PatternCircularParametersWidget.h"
#include "PatternPathParametersWidget.h"
#include "PatternPointParametersWidget.h"
#include "TaskPatternParameters.h"

using namespace PartGui;

namespace
{
std::string patternReferenceCommand(
    const App::DocumentObject* object,
    const std::vector<std::string>& subnames
)
{
    if (!object) {
        return "None";
    }
    std::string command = "(" + Gui::Command::getObjectCmd(object) + ", [";
    for (const auto& subname : subnames) {
        command += Base::Tools::quoted(Base::Tools::escapeEncodeString(subname)) + ", ";
    }
    return command + "])";
}
}  // namespace

TaskPatternParameters::TaskPatternParameters() = default;

TaskPatternParameters::~TaskPatternParameters() = default;

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
            const std::string path = patternReferenceCommand(object, subnames);
            FCMD_OBJ_CMD(pattern, "Path = " << path.c_str());
        }
        pathParametersWidget->applyQuantitySpinboxes();
        return;
    }

    if (pointParametersWidget) {
        App::DocumentObject* object = nullptr;
        std::vector<std::string> subnames;
        pointParametersWidget->getPointObject(object, subnames);
        // Selection already applied the property. Record a copied reference without assigning it again.
        const std::string command = Gui::Command::getObjectCmd(pattern)
            + ".PointObject = " + patternReferenceCommand(object, subnames);
        Gui::Application::Instance->macroManager()->addLine(Gui::MacroManager::App, command.c_str());
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
        return Base::Vector3d::UnitY;
    }

    return Base::Vector3d::UnitX;
}

Base::Vector3d TaskPatternParameters::transformLinearPatternDirection(
    const Base::Vector3d& direction
) const
{
    return direction;
}

Base::Vector3d TaskPatternParameters::getLinearPatternLabelPlaneNormal(Part::LinearPatternDirection) const
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
