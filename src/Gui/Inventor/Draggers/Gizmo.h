// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Sayantan Deb <sayantandebin[at]gmail.com>           *
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

#include <initializer_list>
#include <memory>
#include <vector>

#include <Inventor/fields/SoSFBool.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/SbVec3f.h>
#include <QMetaObject>

#include <Base/Placement.h>
#include <Gui/DocumentObserver.h>

#include <FCGlobal.h>

class SoDragger;
class SoCamera;
class SoInteractionKit;

namespace Gui
{
class QuantitySpinBox;
class SoLinearDragger;
class SoLinearDraggerContainer;
class SoRotationDragger;
class SoRotationDraggerContainer;
class View3DInventorViewer;
class ViewProviderDragger;

struct GizmoPlacement
{
    SbVec3f pos;
    SbVec3f dir;
};

class GuiExport Gizmo
{
public:
    virtual ~Gizmo() = default;
    virtual SoInteractionKit* initDragger() = 0;
    virtual void uninitDragger() = 0;

    virtual GizmoPlacement getDraggerPlacement() = 0;
    virtual void setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir) = 0;
    void setDraggerPlacement(const Base::Vector3d& pos, const Base::Vector3d& dir);

    virtual void setGeometryScale(float scale) = 0;
    virtual void orientAlongCamera([[maybe_unused]] SoCamera* camera) {};
    bool isDelayedUpdateEnabled();

    double getMultFactor();
    double getAddFactor();

    bool getVisibility();

protected:
    double multFactor = 1.0f;
    double addFactor = 0.0f;

    QuantitySpinBox* property = nullptr;
    double initialValue;

    bool visible = true;
};

class GuiExport LinearGizmo: public Gizmo
{
public:
    LinearGizmo(QuantitySpinBox* property);
    ~LinearGizmo() override = default;

    SoInteractionKit* initDragger() override;
    void uninitDragger() override;

    void updateColorTheme();

    // Returns the position and rotation of the base of the dragger
    GizmoPlacement getDraggerPlacement() override;
    void setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir) override;
    void reverseDir();
    // Returns the drag distance from the base of the feature
    double getDragLength();
    void setDragLength(double dragLength);
    void setGeometryScale(float scale) override;
    SoLinearDraggerContainer* getDraggerContainer();
    void setProperty(QuantitySpinBox* property);
    void setMultFactor(const double val);
    void setAddFactor(const double val);
    void setVisibility(bool visible);

private:
    SoLinearDragger* dragger = nullptr;
    SoLinearDraggerContainer* draggerContainer = nullptr;
    QMetaObject::Connection quantityChangedConnection;
    QMetaObject::Connection formulaDialogConnection;

    void draggingStarted();
    void draggingFinished();
    void draggingContinued();

    using inherited = Gizmo;
};

class GuiExport RotationGizmo: public Gizmo
{
public:
    RotationGizmo(QuantitySpinBox* property);
    ~RotationGizmo() override;

    SoInteractionKit* initDragger() override;
    void uninitDragger() override;

    void updateColorTheme();

    // Distance between the linear gizmo base and rotation gizmo
    double sepDistance = 0;
    // Controls if the gizmo is automatically rotated around the pointer in the
    // to always face the camera
    bool automaticOrientation = false;

    // Returns the position and rotation of the base of the dragger
    GizmoPlacement getDraggerPlacement() override;
    void setDraggerPlacement(const SbVec3f& pos, const SbVec3f& dir) override;
    void reverseDir();
    // The two gizmos are separated by sepDistance units
    void placeOverLinearGizmo(LinearGizmo* gizmo);
    void placeBelowLinearGizmo(LinearGizmo* gizmo);
    // Returns the rotation angle wrt the normal axis
    double getRotAngle();
    void setRotAngle(double angle);
    void setGeometryScale(float scale) override;
    SoRotationDraggerContainer* getDraggerContainer();
    void orientAlongCamera(SoCamera* camera) override;
    void setProperty(QuantitySpinBox* property);
    void setMultFactor(const double val);
    void setAddFactor(const double val);
    void setVisibility(bool visible);

private:
    SoRotationDragger* dragger = nullptr;
    SoRotationDraggerContainer* draggerContainer = nullptr;
    SoFieldSensor translationSensor;
    LinearGizmo* linearGizmo = nullptr;
    QMetaObject::Connection quantityChangedConnection;
    QMetaObject::Connection formulaDialogConnection;

    void draggingStarted();
    void draggingFinished();
    void draggingContinued();
    static void translationSensorCB(void* data, SoSensor* sensor);

    using inherited = Gizmo;
};

class GuiExport DirectedRotationGizmo: public RotationGizmo
{
public:
    DirectedRotationGizmo(QuantitySpinBox* property);

    SoInteractionKit* initDragger() override;

    void flipArrow();

private:
    using inherited = RotationGizmo;
};

class GuiExport RadialGizmo: public RotationGizmo
{
public:
    RadialGizmo(QuantitySpinBox* property);

    SoInteractionKit* initDragger() override;

    void updateColorTheme();

    void setRadius(float radius);
    void flipArrow();

private:
    using inherited = RotationGizmo;
};

class GuiExport GizmoContainer: public SoBaseKit
{
    SO_KIT_HEADER(GizmoContainer);
    SO_KIT_CATALOG_ENTRY_HEADER(annotation);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(toggleSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(geometry);

public:
    static void initClass();
    GizmoContainer();
    ~GizmoContainer() override;

    SoSFBool visible;

    void initGizmos();
    void uninitGizmos();

    template<typename T = Gizmo>
    T* getGizmo(int index)
    {
        assert(index >= 0 && index < static_cast<int>(gizmos.size()) && "index out of range!");
        return dynamic_cast<T*>(gizmos[index]);
    }
    // This should be called only once after construction
    void addGizmos(std::initializer_list<Gui::Gizmo*> gizmos);
    void attachViewer(Gui::View3DInventorViewer* viewer, Base::Placement& origin);
    void setUpAutoScale(SoCamera* cameraIn);
    void calculateScaleAndOrientation();

    // Checks if the gizmos are enabled in the preferences
    static bool isEnabled();
    static std::unique_ptr<GizmoContainer> create(
        std::initializer_list<Gui::Gizmo*> gizmos,
        ViewProviderDragger* vp
    );

private:
    std::vector<Gizmo*> gizmos;
    SoFieldSensor cameraSensor;
    SoFieldSensor cameraPositionSensor;
    WeakPtrT<ViewProviderDragger> viewProvider;

    void addGizmo(Gizmo* gizmo);

    static void cameraChangeCallback(void* data, SoSensor*);
    static void cameraPositionChangeCallback(void* data, SoSensor*);
};

}  // namespace Gui
