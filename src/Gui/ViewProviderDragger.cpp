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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <memory>
#include <string>
#include <QAction>
#include <QMenu>
#include <Inventor/draggers/SoDragger.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoTransform.h>
#endif

#include <App/GeoFeature.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>
#include <Base/Converter.h>
#include "Gui/ViewParams.h"

#include "Application.h"
#include "BitmapFactory.h"
#include "Control.h"
#include "Document.h"
#include "Inventor/Draggers/SoTransformDragger.h"
#include "Inventor/SoFCPlacementIndicatorKit.h"
#include "SoFCUnifiedSelection.h"
#include "TaskTransform.h"
#include "View3DInventorViewer.h"
#include "ViewProviderDragger.h"
#include "Utilities.h"

#include <ViewProviderLink.h>
#include <App/DocumentObjectGroup.h>
#include <Base/Tools.h>
#include <Inventor/So3DAnnotation.h>

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderDragger, Gui::ViewProviderDocumentObject)

ViewProviderDragger::ViewProviderDragger()
{
    ADD_PROPERTY_TYPE(TransformOrigin, ({}), nullptr, App::Prop_Hidden, nullptr);

    pcPlacement = new SoSwitch;
    pcPlacement->whichChild = SO_SWITCH_NONE;
};

ViewProviderDragger::~ViewProviderDragger() = default;

void ViewProviderDragger::updateData(const App::Property* prop)
{
    if (prop->isDerivedFrom<App::PropertyPlacement>()
        && strcmp(prop->getName(), "Placement") == 0) {
        // Note: If R is the rotation, c the rotation center and t the translation
        // vector then Inventor applies the following transformation: R*(x-c)+c+t
        // In FreeCAD a placement only has a rotation and a translation part but
        // no rotation center. This means that the following equation must be ful-
        // filled: R * (x-c) + c + t = R * x + t
        //    <==> R * x + t - R * c + c = R * x + t
        //    <==> (I-R) * c = 0 ==> c = 0
        // This means that the center point must be the origin!
        Base::Placement p = static_cast<const App::PropertyPlacement*>(prop)->getValue();
        updateTransform(p, pcTransform);
    }

    ViewProviderDocumentObject::updateData(prop);
}

void ViewProviderDragger::setTransformOrigin(const Base::Placement& placement)
{
    TransformOrigin.setValue(placement);
}

void ViewProviderDragger::resetTransformOrigin()
{
    setTransformOrigin({});
}

void ViewProviderDragger::onChanged(const App::Property* property)
{
    if (property == &TransformOrigin) {
        updateDraggerPosition();
    }

    ViewProviderDocumentObject::onChanged(property);
}

TaskView::TaskDialog* ViewProviderDragger::getTransformDialog()
{
    return new TaskTransformDialog(this, transformDragger);
}

bool ViewProviderDragger::doubleClicked()
{
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

void ViewProviderDragger::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QIcon iconObject =
        mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap("Std_TransformManip.svg"));
    QAction* act = menu->addAction(iconObject, QObject::tr("Transform"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Transform));
    ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
}

ViewProvider* ViewProviderDragger::startEditing(int mode)
{
    forwardedViewProvider = nullptr;

    auto ret = ViewProviderDocumentObject::startEditing(mode);
    if (!ret) {
        return ret;
    }

    return forwardedViewProvider ? forwardedViewProvider : ret;
}

bool ViewProviderDragger::forwardToLink()
{
    // Trying to detect if the editing request is forwarded by a link object,
    // usually by doubleClicked(). If so, we route the request back. There shall
    // be no risk of infinite recursion, as ViewProviderLink handles
    // ViewProvider::Transform request by itself.
    ViewProviderDocumentObject* vpParent = nullptr;
    std::string subname;

    auto doc = Application::Instance->editDocument();
    if (!doc) {
        return false;
    }

    doc->getInEdit(&vpParent, &subname);
    if (!vpParent) {
        return false;
    }

    if (vpParent == this) {
        return false;
    }

    if (!vpParent->isDerivedFrom<ViewProviderLink>()) {
        return false;
    }

    forwardedViewProvider = vpParent->startEditing(ViewProvider::Transform);

    return forwardedViewProvider != nullptr;
}
App::PropertyPlacement* ViewProviderDragger::getPlacementProperty() const
{
    auto object = getObject();

    if (auto linkExtension = object->getExtensionByType<App::LinkBaseExtension>(true)) {
        if (auto linkPlacementProp = linkExtension->getLinkPlacementProperty()) {
            return linkPlacementProp;
        }

        return linkExtension->getPlacementProperty();
    }

    return getObject()->getPropertyByName<App::PropertyPlacement>("Placement");
}

bool ViewProviderDragger::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);

    if (forwardToLink()) {
        return true;
    }

    assert(!transformDragger);

    transformDragger = new SoTransformDragger();
    transformDragger->setAxisColors(Gui::ViewParams::instance()->getAxisXColor(),
                               Gui::ViewParams::instance()->getAxisYColor(),
                               Gui::ViewParams::instance()->getAxisZColor());
    transformDragger->draggerSize.setValue(ViewParams::instance()->getDraggerScale());

    transformDragger->addStartCallback(dragStartCallback, this);
    transformDragger->addFinishCallback(dragFinishCallback, this);
    transformDragger->addMotionCallback(dragMotionCallback, this);

    Gui::Control().showDialog(getTransformDialog());

    updateDraggerPosition();

    return true;
}

void ViewProviderDragger::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);

    transformDragger.reset();

    Gui::Control().closeDialog();
}

void ViewProviderDragger::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);

    if (transformDragger && viewer) {
        transformDragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());

        auto originPlacement = App::GeoFeature::getGlobalPlacement(getObject()) * getObjectPlacement().inverse();
        auto mat = originPlacement.toMatrix();

        viewer->getDocument()->setEditingTransform(mat);
        viewer->setupEditingRoot(transformDragger, &mat);
    }
}

void ViewProviderDragger::unsetEditViewer([[maybe_unused]] Gui::View3DInventorViewer* viewer)
{}

void ViewProviderDragger::dragStartCallback(void* data, [[maybe_unused]] SoDragger* d)
{
    // This is called when a manipulator has done manipulating
    auto vp = static_cast<ViewProviderDragger*>(data);

    vp->draggerPlacement = vp->getDraggerPlacement();
    vp->transformDragger->clearIncrementCounts();
}

void ViewProviderDragger::dragFinishCallback(void* data, [[maybe_unused]] SoDragger* d)
{
    // This is called when a manipulator has done manipulating
    auto vp = static_cast<ViewProviderDragger*>(data);

    vp->draggerPlacement = vp->getDraggerPlacement();
    vp->transformDragger->clearIncrementCounts();

    vp->updatePlacementFromDragger();
}

void ViewProviderDragger::dragMotionCallback(void* data, [[maybe_unused]] SoDragger* d)
{
    auto vp = static_cast<ViewProviderDragger*>(data);

    vp->updateTransformFromDragger();
}

void ViewProviderDragger::updatePlacementFromDragger(DraggerComponents components)
{
    const auto placement = getPlacementProperty();

    if (!placement) {
        return;
    }

    // Get new target dragger placement
    Base::Placement newDraggerPlacement = getDraggerPlacement();
    Base::Vector3d newDraggerPosition = newDraggerPlacement.getPosition();
    Base::Rotation newDraggerRotation = newDraggerPlacement.getRotation();

    // Get old dragger placement before movement
    const Base::Placement oldObjectPlacement = placement->getValue();
    const Base::Placement oldDraggerPlacement = oldObjectPlacement * getTransformOrigin();
    const Base::Vector3d oldDraggerPosition = oldDraggerPlacement.getPosition();
    const Base::Rotation oldDraggerRotation = oldDraggerPlacement.getRotation();

    // --- Mask translation ---
    const Base::Vector3d deltaPositionGlobal = newDraggerPosition - oldDraggerPosition;
    const Base::Vector3d deltaPositionLocal = oldDraggerRotation.inverse().multVec(deltaPositionGlobal);
    Base::Vector3d maskedDeltaPositionLocal = deltaPositionLocal;

    if (!components.testFlag(DraggerComponent::XPos)) {
        maskedDeltaPositionLocal.x = 0.0;
    }
    if (!components.testFlag(DraggerComponent::YPos)) {
        maskedDeltaPositionLocal.y = 0.0;
    }
    if (!components.testFlag(DraggerComponent::ZPos)) {
        maskedDeltaPositionLocal.z = 0.0;
    }

    const Base::Vector3d maskedDeltaPositionGlobal = oldDraggerRotation.multVec(maskedDeltaPositionLocal);
    Base::Vector3d finalPosition = oldDraggerPosition + maskedDeltaPositionGlobal;

    // --- Mask rotation ---
    Base::Vector3d oldX = oldDraggerRotation.multVec(Base::Vector3d::UnitX);
    Base::Vector3d oldY = oldDraggerRotation.multVec(Base::Vector3d::UnitY);
    Base::Vector3d oldZ = oldDraggerRotation.multVec(Base::Vector3d::UnitZ);

    Base::Vector3d newX = newDraggerRotation.multVec(Base::Vector3d::UnitX);
    Base::Vector3d newY = newDraggerRotation.multVec(Base::Vector3d::UnitY);
    Base::Vector3d newZ = newDraggerRotation.multVec(Base::Vector3d::UnitZ);

    // Choose which axes to align
    Base::Vector3d x = components.testFlag(DraggerComponent::XRot) ? newX : oldX;
    Base::Vector3d y = components.testFlag(DraggerComponent::YRot) ? newY : oldY;
    Base::Vector3d z = components.testFlag(DraggerComponent::ZRot) ? newZ : oldZ;

    Base::Rotation finalRotation = orthonormalize(x, y, z, components);

    // Create new dragger placement, only if components are masked
    Base::Placement finalDraggerPlacement(newDraggerPosition, newDraggerRotation);
    if (!components.testFlag(DraggerComponent::All)){
        finalDraggerPlacement.setPosition(finalPosition);
        finalDraggerPlacement.setRotation(finalRotation);
    }

    placement->setValue((finalDraggerPlacement * getTransformOrigin().inverse()));
    updateDraggerPosition();
}

Base::Rotation Gui::ViewProviderDragger::orthonormalize(Base::Vector3d x,
                                         Base::Vector3d y,
                                         Base::Vector3d z,
                                         ViewProviderDragger::DraggerComponents components)
{
    // Orthonormalize (Gram–Schmidt process) to find perpendicular unit vector depending on masked axes
    if (components.testFlag(Gui::ViewProviderDragger::DraggerComponent::XRot)
        && components.testFlag(Gui::ViewProviderDragger::DraggerComponent::YRot)) {
        x.Normalize();
        y = y - x * (x * y);
        y.Normalize();
        z = x.Cross(y);
        z.Normalize();
    }
    else if (components.testFlag(Gui::ViewProviderDragger::DraggerComponent::XRot) && components.testFlag(Gui::ViewProviderDragger::DraggerComponent::ZRot)) {
        x.Normalize();
        z = z - x * (x * z);
        z.Normalize();
        y = z.Cross(x);
        y.Normalize();
    }
    else if (components.testFlag(Gui::ViewProviderDragger::DraggerComponent::YRot) && components.testFlag(Gui::ViewProviderDragger::DraggerComponent::ZRot)) {
        y.Normalize();
        z = z - y * (y * z);
        z.Normalize();
        x = y.Cross(z);
        x.Normalize();
    }
    else if (components.testFlag(Gui::ViewProviderDragger::DraggerComponent::XRot)) {
        x.Normalize();
        y = y - x * (x * y);
        y.Normalize();
        z = x.Cross(y);
        z.Normalize();
    }
    else if (components.testFlag(Gui::ViewProviderDragger::DraggerComponent::YRot)) {
        y.Normalize();
        x = x - y * (x * y);
        x.Normalize();
        z = x.Cross(y);
        z.Normalize();
    }
    else if (components.testFlag(Gui::ViewProviderDragger::DraggerComponent::ZRot)) {
        z.Normalize();
        x = x - z * (x * z);
        x.Normalize();
        y = z.Cross(x);
        y.Normalize();
    }

    return Base::Rotation::makeRotationByAxes(x, y, z);
}

void ViewProviderDragger::updateTransformFromDragger()
{
    const auto placement = getDraggerPlacement() * getTransformOrigin().inverse();

    pcTransform->translation.setValue(Base::convertTo<SbVec3f>(placement.getPosition()));
    pcTransform->rotation.setValue(Base::convertTo<SbRotation>(placement.getRotation()));
}

Base::Placement ViewProviderDragger::getObjectPlacement() const
{
    if (auto placement = getPlacementProperty()) {
        return placement->getValue();
    }

    return {};
}

Base::Placement ViewProviderDragger::getDraggerPlacement() const
{
    const double translationStep = transformDragger->translationIncrement.getValue();
    const int xSteps = transformDragger->translationIncrementCountX.getValue();
    const int ySteps = transformDragger->translationIncrementCountY.getValue();
    const int zSteps = transformDragger->translationIncrementCountZ.getValue();

    const auto rotation = draggerPlacement.getRotation();
    const auto xBase = rotation.multVec(Base::Vector3d(1, 0, 0));
    const auto yBase = rotation.multVec(Base::Vector3d(0, 1, 0));
    const auto zBase = rotation.multVec(Base::Vector3d(0, 0, 1));

    const auto positionIncrement =
        xBase * (translationStep * xSteps) +
        yBase * (translationStep * ySteps) +
        zBase * (translationStep * zSteps);

    const double rotationStep = transformDragger->rotationIncrement.getValue();
    const int xRotationSteps = transformDragger->rotationIncrementCountX.getValue();
    const int yRotationSteps = transformDragger->rotationIncrementCountY.getValue();
    const int zRotationSteps = transformDragger->rotationIncrementCountZ.getValue();

    auto newRotation = rotation;
    newRotation = newRotation * Base::Rotation(Base::Vector3d(1, 0, 0), xRotationSteps * rotationStep);
    newRotation = newRotation * Base::Rotation(Base::Vector3d(0, 1, 0), yRotationSteps * rotationStep);
    newRotation = newRotation * Base::Rotation(Base::Vector3d(0, 0, 1), zRotationSteps * rotationStep);

    return Base::Placement(
        draggerPlacement.getPosition() + positionIncrement,
        newRotation
    );
}

Base::Placement ViewProviderDragger::getOriginalDraggerPlacement() const
{
    return draggerPlacement;
}

void ViewProviderDragger::setDraggerPlacement(const Base::Placement& placement)
{
    transformDragger->translation.setValue(Base::convertTo<SbVec3f>(placement.getPosition()));
    transformDragger->rotation.setValue(Base::convertTo<SbRotation>(placement.getRotation()));

    draggerPlacement = placement;
    transformDragger->clearIncrementCounts();
}

void ViewProviderDragger::attach(App::DocumentObject* pcObject)
{
    ViewProviderDocumentObject::attach(pcObject);

    getAnnotation()->addChild(pcPlacement);

    auto* pcAxisCrossKit = new Gui::SoFCPlacementIndicatorKit();

    auto* pcAnnotation = new So3DAnnotation();
    pcAnnotation->addChild(pcAxisCrossKit);

    pcPlacement->addChild(pcAnnotation);
}

void ViewProviderDragger::updateDraggerPosition()
{
    if (!transformDragger) {
        return;
    }

    auto placement = getObjectPlacement() * getTransformOrigin();

    setDraggerPlacement(placement);
}

void ViewProviderDragger::updateTransform(const Base::Placement& from, SoTransform* to)
{
    to->rotation.setValue(Base::convertTo<SbRotation>(from.getRotation()));
    to->translation.setValue(Base::convertTo<SbVec3f>(from.getPosition()));
    to->center.setValue(0.0f, 0.0f, 0.0f);
    to->scaleFactor.setValue(1.0f, 1.0f, 1.0f);
}
