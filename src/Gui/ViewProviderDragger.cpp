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
#include "SoFCCSysDragger.h"
#include "SoFCUnifiedSelection.h"
#include "TaskCSysDragger.h"
#include "View3DInventorViewer.h"
#include "ViewProviderDragger.h"
#include "Utilities.h"

#include <ViewProviderLink.h>
#include <App/DocumentObjectGroup.h>
#include <Base/Tools.h>

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderDragger, Gui::ViewProviderDocumentObject)

ViewProviderDragger::ViewProviderDragger()
{
    ADD_PROPERTY_TYPE(TransformOrigin, ({}), nullptr, App::Prop_Hidden, nullptr);
};

ViewProviderDragger::~ViewProviderDragger() = default;

void ViewProviderDragger::updateData(const App::Property* prop)
{
    if (prop->isDerivedFrom(App::PropertyPlacement::getClassTypeId())
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
    return new TaskCSysDragger(this, csysDragger);
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

bool ViewProviderDragger::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);

    if (forwardToLink()) {
        return true;
    }

    assert(!csysDragger);

    csysDragger = new SoFCCSysDragger();
    csysDragger->setAxisColors(Gui::ViewParams::instance()->getAxisXColor(),
                               Gui::ViewParams::instance()->getAxisYColor(),
                               Gui::ViewParams::instance()->getAxisZColor());
    csysDragger->draggerSize.setValue(ViewParams::instance()->getDraggerScale());

    csysDragger->addStartCallback(dragStartCallback, this);
    csysDragger->addFinishCallback(dragFinishCallback, this);
    csysDragger->addMotionCallback(dragMotionCallback, this);

    Gui::Control().showDialog(getTransformDialog());

    updateDraggerPosition();

    return true;
}

void ViewProviderDragger::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);

    csysDragger.reset();

    Gui::Control().closeDialog();
}

void ViewProviderDragger::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);

    if (csysDragger && viewer) {
        csysDragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());

        auto originPlacement = App::GeoFeature::getGlobalPlacement(getObject()) * getObjectPlacement().inverse();
        auto mat = originPlacement.toMatrix();

        viewer->getDocument()->setEditingTransform(mat);
        viewer->setupEditingRoot(csysDragger, &mat);
    }
}

void ViewProviderDragger::unsetEditViewer([[maybe_unused]] Gui::View3DInventorViewer* viewer)
{}

void ViewProviderDragger::dragStartCallback(void* data, [[maybe_unused]] SoDragger* d)
{
    // This is called when a manipulator has done manipulating
    auto vp = static_cast<ViewProviderDragger*>(data);

    vp->draggerPlacement = vp->getDraggerPlacement();
    vp->csysDragger->clearIncrementCounts();
}

void ViewProviderDragger::dragFinishCallback(void* data, [[maybe_unused]] SoDragger* d)
{
    // This is called when a manipulator has done manipulating
    auto vp = static_cast<ViewProviderDragger*>(data);

    vp->draggerPlacement = vp->getDraggerPlacement();
    vp->csysDragger->clearIncrementCounts();

    vp->updatePlacementFromDragger();
}

void ViewProviderDragger::dragMotionCallback(void* data, [[maybe_unused]] SoDragger* d)
{
    auto vp = static_cast<ViewProviderDragger*>(data);

    vp->updateTransformFromDragger();
}

void ViewProviderDragger::updatePlacementFromDragger()
{
    const auto placement = getObject()->getPropertyByName<App::PropertyPlacement>("Placement");

    if (!placement) {
        return;
    }

    placement->setValue(getDraggerPlacement() * getTransformOrigin().inverse());
}

void ViewProviderDragger::updateTransformFromDragger()
{
    const auto placement = getDraggerPlacement() * getTransformOrigin().inverse();

    pcTransform->translation.setValue(Base::convertTo<SbVec3f>(placement.getPosition()));
    pcTransform->rotation.setValue(Base::convertTo<SbRotation>(placement.getRotation()));
}

Base::Placement ViewProviderDragger::getObjectPlacement() const
{
    if (auto placement = getObject()->getPropertyByName<App::PropertyPlacement>("Placement")) {
        return placement->getValue();
    }

    return {};
}

Base::Placement ViewProviderDragger::getDraggerPlacement() const
{
    const double translationStep = csysDragger->translationIncrement.getValue();
    const int xSteps = csysDragger->translationIncrementCountX.getValue();
    const int ySteps = csysDragger->translationIncrementCountY.getValue();
    const int zSteps = csysDragger->translationIncrementCountZ.getValue();

    const auto rotation = draggerPlacement.getRotation();
    const auto xBase = rotation.multVec(Base::Vector3d(1, 0, 0));
    const auto yBase = rotation.multVec(Base::Vector3d(0, 1, 0));
    const auto zBase = rotation.multVec(Base::Vector3d(0, 0, 1));

    const auto positionIncrement =
        xBase * (translationStep * xSteps) +
        yBase * (translationStep * ySteps) +
        zBase * (translationStep * zSteps);

    const double rotationStep = csysDragger->rotationIncrement.getValue();
    const int xRotationSteps = csysDragger->rotationIncrementCountX.getValue();
    const int yRotationSteps = csysDragger->rotationIncrementCountY.getValue();
    const int zRotationSteps = csysDragger->rotationIncrementCountZ.getValue();

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
    csysDragger->translation.setValue(Base::convertTo<SbVec3f>(placement.getPosition()));
    csysDragger->rotation.setValue(Base::convertTo<SbRotation>(placement.getRotation()));

    draggerPlacement = placement;
    csysDragger->clearIncrementCounts();
}

void ViewProviderDragger::updateDraggerPosition()
{
    if (!csysDragger) {
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
