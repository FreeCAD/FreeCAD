/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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


#pragma once

#include "TaskView/TaskDialog.h"
#include "TaskView/TaskView.h"
#include "ViewProviderDragger.h"

#include <Inventor/nodes/SoSeparator.h>

#include <Base/ServiceProvider.h>

#include <App/Application.h>
#include <App/Services.h>

#include <array>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class SoDragger;
class SoTransform;
class QString;

namespace Gui
{
class QuantitySpinBox;
class SoTransformDragger;
class ViewProviderDragger;
class Ui_TaskTransformDialog;

class TaskTransform: public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

    static constexpr double tolerance = 1e-7;

public:
    enum class SelectionMode
    {
        None,
        SelectTransformOrigin,
        SelectAlignTarget,
        SelectCustomCS,
        SelectCumulativeSnapReference,
        SelectCumulativeSnapTarget
    };
    enum class PlacementMode
    {
        ObjectOrigin,
        Centroid,
        Custom
    };
    enum class PositionMode
    {
        Local,
        Global,
        Custom
    };

    struct CoordinateSystem
    {
        std::array<std::string, 3> labels;
        Base::Placement origin;
    };

    Q_ENUM(SelectionMode)
    Q_ENUM(PlacementMode)
    Q_ENUM(PositionMode)

    TaskTransform(
        Gui::ViewProviderDragger* vp,
        Gui::SoTransformDragger* dragger,
        QWidget* parent = nullptr,
        App::SubObjectPlacementProvider* subObjectPlacementProvider
        = Base::provideService<App::SubObjectPlacementProvider>(),
        App::CenterOfMassProvider* centerOfMassProvider
        = Base::provideService<App::CenterOfMassProvider>()
    );
    ~TaskTransform() override;

private:
    void onSelectionChanged(const SelectionChanges& msg) override;

private Q_SLOTS:
    void onPlacementModeChange(int index);

    void onPickTransformOrigin();
    void onPickCoordinateSystemReference();
    void onTransformOriginReset();
    void onAlignRotationChanged();

    void onAlignToOtherObject();
    void onFlip();
    void onCumulativeSnap();
    void onUndoCumulativeSnap();
    void onClearCumulativeSnap();
    void onInvertCumulativeSnapU();
    void onInvertCumulativeSnapV();

    void onCoordinateSystemChange(int mode);

    void onPositionChange();
    void onRotationChange(QuantitySpinBox* changed);

private:
    struct CumulativeSnapStep;

    static inline bool firstDrag = true;
    static void dragStartCallback(void* data, SoDragger* d);
    static void dragMotionCallback(void* data, SoDragger* d);

    void setSelectionMode(SelectionMode mode);
    SelectionMode getSelectionMode() const;

    CoordinateSystem globalCoordinateSystem() const;
    CoordinateSystem localCoordinateSystem() const;
    CoordinateSystem customCoordinateSystem() const;
    CoordinateSystem currentCoordinateSystem() const;

    Base::Rotation::EulerSequence eulerSequence() const;

    void setupGui();

    void loadPreferences();
    void savePreferences();

    void loadPositionModeItems() const;
    void loadPlacementModeItems() const;

    void updatePositionAndRotationUi() const;
    void updateDraggerLabels() const;
    void updateInputLabels() const;
    void updateIncrements() const;
    void updateTransformOrigin();
    void updateSpinBoxesReadOnlyStatus() const;

    void resetReferencePlacement();
    void resetReferenceRotation();
    void setCustomCoordinateSystemFromSelection(const SelectionChanges& msg);

    ViewProviderDragger::DraggerComponents getRelevantComponents();
    void moveObjectToDragger(
        ViewProviderDragger::DraggerComponents components = ViewProviderDragger::DraggerComponent::All
    );
    App::SubObjectPlacementProvider::SnapGeometryType snapGeometryType(
        const SelectionChanges& msg
    ) const;
    bool isCumulativeSnapMovingObjectSelection(
        const SelectionChanges& msg,
        const App::DocumentObject* object,
        const App::DocumentObject* originalObject
    ) const;
    bool isCompatibleCumulativeSnapTarget(
        App::SubObjectPlacementProvider::SnapGeometryType referenceType,
        App::SubObjectPlacementProvider::SnapGeometryType targetType
    ) const;
    std::optional<Base::Placement> solveCumulativeSnapObjectPlacement(
        const Base::Placement& candidate,
        const Base::Placement& currentReferenceLocalPlacement,
        const Base::Placement& currentReferenceTargetPlacement,
        App::SubObjectPlacementProvider::SnapGeometryType currentReferenceType,
        App::SubObjectPlacementProvider::SnapGeometryType currentTargetType,
        bool currentTargetDirectionFixed = false,
        std::size_t historySize = std::numeric_limits<std::size_t>::max()
    ) const;
    void startCumulativeSnap();
    void stopCumulativeSnap();
    void appendCumulativeSnapStep(
        const QString& referenceLabel,
        const QString& targetLabel,
        const Base::Placement& referenceLocalPlacement,
        const Base::Placement& targetPlacement,
        App::SubObjectPlacementProvider::SnapGeometryType referenceType,
        App::SubObjectPlacementProvider::SnapGeometryType targetType
    );
    void restoreCumulativeSnapPlacement(const Base::Placement& placement);
    bool isCumulativeSnapStepInvertible(const CumulativeSnapStep& step) const;
    std::optional<std::size_t> cumulativeSnapInvertTargetIndex() const;
    bool canInvertCumulativeSnapDirection() const;
    void invertCumulativeSnapDirection(const Base::Vector3d& localAxis);
    bool updateCumulativeSnapHistoryPlacements();
    void updateCumulativeSnapUi() const;

    bool isDraggerAlignedToCoordinateSystem() const;

    void showCoordinateSystemIndicator();
    void hideCoordinateSystemIndicator();
    void updateCoordinateSystemIndicator();

    ViewProviderDragger* vp;

    const App::SubObjectPlacementProvider* subObjectPlacementProvider;
    const App::CenterOfMassProvider* centerOfMassProvider;

    CoinPtr<SoTransformDragger> dragger;
    CoinPtr<SoSeparator> csIndicatorRoot;
    CoinPtr<SoTransform> csIndicatorTransform;

    Ui_TaskTransformDialog* ui;

    SelectionMode selectionMode {SelectionMode::None};
    PlacementMode placementMode {PlacementMode::ObjectOrigin};
    PositionMode positionMode {PositionMode::Local};

    std::optional<Base::Placement> customTransformOrigin {};
    std::optional<Base::Placement> customCoordinateSystemPlacement {};
    std::optional<Base::Placement> cumulativeSnapStartPlacement {};
    Base::Placement referencePlacement {};
    Base::Placement globalOrigin {};
    Base::Rotation referenceRotation {};
    bool cumulativeSnapActive {false};
    struct CumulativeSnapReference
    {
        std::string label;
        Base::Placement localPlacement;
        App::SubObjectPlacementProvider::SnapGeometryType type;
    };
    struct CumulativeSnapStep
    {
        std::string label;
        Base::Placement objectPlacement;
        Base::Placement referenceLocalPlacement;
        Base::Placement targetPlacement;
        App::SubObjectPlacementProvider::SnapGeometryType referenceType;
        App::SubObjectPlacementProvider::SnapGeometryType targetType;
        bool targetDirectionFixed {false};
    };
    std::optional<CumulativeSnapReference> currentCumulativeSnapReference {};
    std::vector<CumulativeSnapStep> cumulativeSnapHistory;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/History/Dragger"
    );
};

class TaskTransformDialog: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskTransformDialog(ViewProviderDragger* vp, SoTransformDragger* dragger);
    ~TaskTransformDialog() override = default;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

    void open() override;
    bool accept() override;
    bool reject() override;
    void onUndo() override;
    void onRedo() override;

private:
    void openCommand();
    void updateDraggerPlacement();

private:
    ViewProviderDragger* vp;
    TaskTransform* transform;
};
}  // namespace Gui
