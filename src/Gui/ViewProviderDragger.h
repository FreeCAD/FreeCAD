/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "ViewProviderDocumentObject.h"
#include <Base/Placement.h>
#include <App/PropertyGeo.h>
#include <Base/Bitmask.h>

class SoDragger;
class SoTransform;

namespace Gui
{

namespace TaskView
{
class TaskDialog;
}

class SoTransformDragger;
class View3DInventorViewer;
class GizmoContainer;

/**
 * The base class for all view providers modifying the placement
 * of a geometric feature.
 * @author Werner Mayer
 */
class GuiExport ViewProviderDragger: public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderDragger);

public:
    /// constructor.
    ViewProviderDragger();

    /// destructor.
    ~ViewProviderDragger() override;

    /// Origin used when object is transformed. It temporarily changes the origin of object.
    /// Dragger is normally placed at the transform origin, unless explicitly overridden via
    /// ViewProviderDragger#setDraggerPlacement() method.
    App::PropertyPlacement TransformOrigin;

    void attach(App::DocumentObject* pcObject) override;

    /// Convenience method to obtain the transform origin
    Base::Placement getTransformOrigin() const
    {
        return TransformOrigin.getValue();
    }
    /// Convenience method to set the transform origin
    void setTransformOrigin(const Base::Placement& placement);
    /// Resets transform origin to the object origin
    void resetTransformOrigin();

    void setGizmoContainer(Gui::GizmoContainer* gizmoContainer);

public:
    /** @name Edit methods */
    //@{
    bool doubleClicked() override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    void updateData(const App::Property*) override;

    ViewProvider* startEditing(int ModNum = 0) override;

    /*! synchronize From FC placement to Coin placement*/
    static void updateTransform(const Base::Placement& from, SoTransform* to);

    enum class DraggerComponent
    {
        None = 0,
        XPos = 1 << 0,
        YPos = 1 << 1,
        ZPos = 1 << 2,
        XRot = 1 << 3,
        YRot = 1 << 4,
        ZRot = 1 << 5,
        All = XPos | YPos | ZPos | XRot | YRot | ZRot
    };
    using DraggerComponents = Base::Flags<DraggerComponent>;
    /// updates placement of object based on dragger position and chosen axes components
    void updatePlacementFromDragger(DraggerComponents components = DraggerComponent::All);
    /// updates transform of object based on dragger position, can be used to preview movement
    void updateTransformFromDragger();

    /// Gets object placement relative to its coordinate system
    Base::Placement getObjectPlacement() const;
    /// Gets current dragger placement, including current dragger movement
    Base::Placement getDraggerPlacement() const;
    /// Gets original dragger placement, without current dragger movement
    Base::Placement getOriginalDraggerPlacement() const;
    /// Sets placement of dragger relative to objects origin
    void setDraggerPlacement(const Base::Placement& placement);

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void setEditViewer(View3DInventorViewer*, int ModNum) override;
    void unsetEditViewer(View3DInventorViewer*) override;
    //@}

    void onChanged(const App::Property* prop) override;

    bool forwardToLink();

    /**
     * Returns a newly create dialog for the part to be placed in the task view
     * Must be reimplemented in subclasses.
     */
    virtual TaskView::TaskDialog* getTransformDialog();

    CoinPtr<SoTransformDragger> transformDragger;
    ViewProvider* forwardedViewProvider = nullptr;

    CoinPtr<SoSwitch> pcPlacement;

private:
    static void dragStartCallback(void* data, SoDragger* d);
    static void dragFinishCallback(void* data, SoDragger* d);
    static void dragMotionCallback(void* data, SoDragger* d);

    void updateDraggerPosition();

    Base::Placement draggerPlacement {};

    // Rotation by orthonormalizing depending on given axes components
    Base::Rotation orthonormalize(
        Base::Vector3d x,
        Base::Vector3d y,
        Base::Vector3d z,
        ViewProviderDragger::DraggerComponents components = DraggerComponent::All
    );

    GizmoContainer* gizmoContainer = nullptr;
};

}  // namespace Gui

ENABLE_BITMASK_OPERATORS(Gui::ViewProviderDragger::DraggerComponent)
