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

#include <Base/Bitmask.h>
#include <Base/ServiceProvider.h>

#include <App/Application.h>
#include <App/Services.h>

#include <QString>

#include <array>
#include <optional>
#include <string>

class SoDragger;
class SoTransform;

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
        SelectCustomCS
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
    enum class ReferencePlacementOption
    {
        None = 0,
        UseSubObjectPlacement = 1 << 0,
        UseSnapPosition = 1 << 1
    };
    using ReferencePlacementOptions = Base::Flags<ReferencePlacementOption>;

    struct CoordinateSystem
    {
        std::array<std::string, 3> labels;
        Base::Placement origin;
    };
    struct ReferencePlacement
    {
        Base::Placement documentPlacement;
        Base::Placement objectPlacement;
        QString label;
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

    void onCoordinateSystemChange(int mode);

    void onPositionChange();
    void onRotationChange(QuantitySpinBox* changed);

private:
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
    std::optional<ReferencePlacement> referencePlacementFromSelection(
        const SelectionChanges& msg,
        ReferencePlacementOptions options
    ) const;
    void setCustomCoordinateSystemFromSelection(const SelectionChanges& msg);

    ViewProviderDragger::DraggerComponents getRelevantComponents();
    void moveObjectToDragger(
        ViewProviderDragger::DraggerComponents components = ViewProviderDragger::DraggerComponent::All
    );

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
    Base::Placement referencePlacement {};
    Base::Placement globalOrigin {};
    Base::Rotation referenceRotation {};

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

ENABLE_BITMASK_OPERATORS(Gui::TaskTransform::ReferencePlacementOption)
