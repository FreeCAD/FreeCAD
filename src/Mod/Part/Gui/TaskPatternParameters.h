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

#pragma once

#include <QCoreApplication>

#include <Mod/Part/App/LinearPatternExtension.h>
#include <Mod/Part/PartGlobal.h>

#include <Base/Vector3D.h>

#include <string>
#include <vector>

class QObject;
class QWidget;
class QTimer;
class gp_Ax2;

namespace App
{
class DocumentObject;
class PropertyEnumeration;
class PropertyLength;
class PropertyBool;
class PropertyIntegerConstraint;
class PropertyLinkSub;
}  // namespace App

namespace Gui
{
class ComboLinks;
class View3DInventorViewer;
}  // namespace Gui

namespace PartGui
{

class PatternParametersWidget;
class PatternCircularParametersWidget;
class PatternPathParametersWidget;
class PatternPointParametersWidget;
class PatternReferenceWidget;

/**
 * Reusable implementation for task panels editing Part pattern extensions.
 *
 * The class deliberately does not inherit QObject. It is meant to be used as a
 * secondary base by task panels that already inherit from their workbench task
 * base class.
 */
class PartGuiExport TaskPatternParameters
{
    Q_DECLARE_TR_FUNCTIONS(PartGui::TaskPatternParameters)

public:
    TaskPatternParameters();
    virtual ~TaskPatternParameters();

protected:
    void setupPatternParameterUI(
        QWidget* parent,
        QWidget* firstPlaceholder,
        QWidget* secondPlaceholder,
        Gui::View3DInventorViewer* viewer,
        QObject* signalContext,
        int updateViewTimeout
    );
    void setupCircularPatternParameterUI(
        QWidget* parent,
        QWidget* placeholder,
        QObject* signalContext,
        int updateViewTimeout,
        App::PropertyLinkSub* axis,
        App::PropertyLength* radialDistance,
        App::PropertyLength* tangentialDistance,
        App::PropertyIntegerConstraint* numberCircles,
        App::PropertyIntegerConstraint* symmetry
    );
    void setupPathPatternParameterUI(
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
    );
    void setupPointPatternParameterUI(
        QWidget* parent,
        QWidget* placeholder,
        QObject* signalContext,
        App::PropertyLinkSub* pointObject
    );

    void updatePatternParameterUI();
    void updatePatternSpacingLabels();
    void kickUpdateViewTimer() const;
    bool consumePendingUpdate();

    void applyPatternParameters(App::DocumentObject* pattern) const;

    PatternParametersWidget* getPrimaryParametersWidget() const;
    PatternParametersWidget* getSecondaryParametersWidget() const;
    PatternCircularParametersWidget* getCircularParametersWidget() const;
    PatternPathParametersWidget* getPathParametersWidget() const;
    PatternPointParametersWidget* getPointParametersWidget() const;
    PatternReferenceWidget* getActiveDirectionWidget() const;
    void clearActiveDirectionWidget();

    virtual App::DocumentObject* getPatternObject() const = 0;
    virtual void fillDirectionCombo(Gui::ComboLinks& combo, Part::LinearPatternDirection direction) = 0;
    virtual void onReferenceSelectionRequested() = 0;
    virtual void onPatternParametersChanged() = 0;
    virtual void setupPatternTransaction() = 0;
    virtual void recomputePatternFeature() = 0;

    virtual Base::Vector3d getPatternStartPoint() const;
    virtual Base::Vector3d getLinearPatternFallbackDirection(
        Part::LinearPatternDirection direction
    ) const;
    virtual Base::Vector3d transformLinearPatternDirection(const Base::Vector3d& direction) const;
    virtual Base::Vector3d getLinearPatternLabelPlaneNormal(
        Part::LinearPatternDirection direction
    ) const;
    virtual void transformPolarPatternAxis(gp_Ax2& axis) const;
    virtual std::string buildDirectionReferencePythonString(
        const App::DocumentObject* obj,
        const std::vector<std::string>& subs
    ) const;

private:
    void bindPatternProperties();
    void onUpdateViewTimer();

private:
    PatternParametersWidget* parametersWidget = nullptr;
    PatternParametersWidget* parametersWidget2 = nullptr;
    PatternCircularParametersWidget* circularParametersWidget = nullptr;
    PatternPathParametersWidget* pathParametersWidget = nullptr;
    PatternPointParametersWidget* pointParametersWidget = nullptr;
    PatternReferenceWidget* activeDirectionWidget = nullptr;
    QTimer* updateViewTimer = nullptr;
};

}  // namespace PartGui
