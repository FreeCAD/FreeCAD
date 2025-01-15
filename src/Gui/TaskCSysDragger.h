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


#ifndef TASKCSYSDRAGGER_H
#define TASKCSYSDRAGGER_H

#include "TaskView/TaskDialog.h"
#include "TaskView/TaskView.h"
#include "ViewProviderDragger.h"

#include <Base/ServiceProvider.h>
#include <App/Services.h>

class SoDragger;

namespace Attacher
{
    class AttachEngine;
}

namespace Gui
{
class QuantitySpinBox;
class SoFCCSysDragger;
class ViewProviderDragger;
class Ui_TaskCSysDragger;

class TaskTransform : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

    static constexpr double tolerance = 1e-7;

public:
    enum class SelectionMode { None, SelectTransformOrigin, SelectAlignTarget };
    enum class PlacementMode { ObjectOrigin, Centroid, Custom };
    enum class PositionMode { Local, Global };

    struct CoordinateSystem
    {
        std::array<std::string, 3> labels;
        Base::Placement origin;
    };

    Q_ENUM(SelectionMode)
    Q_ENUM(PlacementMode)
    Q_ENUM(PositionMode)

    TaskTransform(Gui::ViewProviderDragger* vp,
                  Gui::SoFCCSysDragger* dragger,
                  QWidget* parent = nullptr,
                  App::SubObjectPlacementProvider* subObjectPlacementProvider =
                      Base::provideService<App::SubObjectPlacementProvider>(),
                  App::CenterOfMassProvider* centerOfMassProvider =
                      Base::provideService<App::CenterOfMassProvider>());
    ~TaskTransform() override;

private:
    void onSelectionChanged(const SelectionChanges& msg) override;

private Q_SLOTS:
    void onPlacementModeChange(int index);

    void onPickTransformOrigin();
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

    void moveObjectToDragger();

    bool isDraggerAlignedToCoordinateSystem() const;

    ViewProviderDragger* vp;

    const App::SubObjectPlacementProvider* subObjectPlacementProvider;
    const App::CenterOfMassProvider *centerOfMassProvider;

    CoinPtr<SoFCCSysDragger> dragger;

    Ui_TaskCSysDragger *ui;

    SelectionMode selectionMode { SelectionMode::None };
    PlacementMode placementMode { PlacementMode::ObjectOrigin };
    PositionMode positionMode { PositionMode::Local };

    std::optional<Base::Placement> customTransformOrigin {};
    Base::Placement referencePlacement {};
    Base::Placement globalOrigin {};
    Base::Rotation referenceRotation {};

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/History/Dragger");
};

class TaskCSysDragger: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskCSysDragger(ViewProviderDragger* vp, SoFCCSysDragger* dragger);
    ~TaskCSysDragger() override = default;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

    void open() override;
    bool accept() override;
    bool reject() override;

private:
    ViewProviderDragger* vp;
    TaskTransform* transform;
};
}  // namespace Gui

#endif  // TASKCSYSDRAGGER_H
