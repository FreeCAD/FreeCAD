/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#include "Gui/Control.h"
#include "Gui/View3DInventor.h"
#include "Mod/PartDesign/App/FeatureExtrude.h"
#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMenu>
#include <algorithm>
#endif

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/nodes/SoCamera.h>
#include "TaskPadParameters.h"
#include "ViewProviderPad.h"
#include <Gui/Inventor/Draggers/SoTransformDragger.h>
#include <Gui/ViewParams.h>
#include <qnamespace.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Gui/Inventor/SoFCPlacementIndicatorKit.h>
#include <Base/Converter.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/EditableDatumLabel.h>
#include <Gui/Inventor/Draggers/SoLinearDragger.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPad, PartDesignGui::ViewProviderExtrude)

ViewProviderPad::ViewProviderPad()
{
    sPixmap = "PartDesign_Pad.svg";
}

ViewProviderPad::~ViewProviderPad() = default;

void ViewProviderPad::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit pad"));

    PartDesignGui::ViewProviderSketchBased::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters *ViewProviderPad::getEditDialog()
{
    // TODO fix setting values from the history: now it doesn't work neither in
    //      the master and in the migrated branch  (2015-07-26, Fat-Zer)
    return new TaskDlgPadParameters( this );
}

bool ViewProviderPad::setEdit([[maybe_unused]] int ModNum)
{
    assert(!dragger);
    dragger = new Gui::SoTranslationDragger();
    dragger->color.setValue(1, 0, 0);

    dragger->getDragger()->addStartCallback(dragStartCallback, this);
    dragger->getDragger()->addFinishCallback(dragFinishCallback, this);
    dragger->getDragger()->addMotionCallback(dragMotionCallback, this);
    dragger->getDragger()->setLabelVisibility(false);
    Base::Console().message("%d", dragger->getDragger()->isLabelVisible());
    dragger->getDragger()->cylinderRadius = 1.0f;
    dragger->getDragger()->cylinderHeight = 25.0f;

    dialog = new TaskDlgPadParameters(this);
    Gui::Control().showDialog(dialog);

    createOVP(dialog->getPadLength());
    updatePosition(dialog->getPadLength());

    return true;
}

void ViewProviderPad::unsetEdit([[maybe_unused]] int ModNum)
{
    cameraSensor->detach();
    delete cameraSensor;
    cameraSensor = nullptr;

    dragger.reset();
    delete ovp;
}

void ViewProviderPad::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);

    if (dragger && viewer) {
        // dragger->getDragger()->autoScaleResult->setUpAutoScale(viewer->getSoRenderManager()->getCamera());
        updateOVPPosition();

        auto originPlacement = App::GeoFeature::getGlobalPlacement(getObject()) * getObjectPlacement().inverse();
        auto mat = originPlacement.toMatrix();

        viewer->getDocument()->setEditingTransform(mat);
        viewer->setupEditingRoot(dragger.get(), &mat);
    }
}

void ViewProviderPad::updatePosition(double padLength)
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto shape = extrude->getProfileShape();

    Base::Vector3d center;
    shape.getCenterOfGravity(center);

    dragger->translation = Base::convertTo<SbVec3f>(center);
    dragger->getDragger()->translation.setValue({0, static_cast<float>(padLength), 0});

    dragger->setPointerDirection(extrude->getProfileNormal());

    dragger->getDragger()->translationIncrementCount.setValue(0);

    updateOVPPosition();
}

Base::Placement ViewProviderPad::getDraggerPlacement()
{
    return {Base::convertTo<Base::Vector3d>(dragger->getDragger()->translation.getValue()),
            Base::convertTo<Base::Rotation>(dragger->rotation.getValue())};
}


double PartDesignGui::ViewProviderPad::getPadLengthFromDragger()
{
    double padLength = dragger->getDragger()->translationIncrementCount.getValue();
    return std::min(padLength, 0.0); // hack value
}

void ViewProviderPad::createOVP(double padLength) {
    ovp = new Gui::QuantitySpinBox(getViewer());
    ovp->setUnit(Base::Unit::Length);
    ovp->setMinimum(std::numeric_limits<int>::min());
    ovp->setMaximum(std::numeric_limits<int>::max());
    ovp->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ovp->setKeyboardTracking(false);
    ovp->setValue(padLength);
    ovp->show();
    ovp->setReadOnly(true);
    ovp->setVisible(false);

    auto ret = Gui::QuantitySpinBox::connect(
        ovp,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        [this](double value) { this->setDraggerPosFromUI(value); }
    );

    cameraSensor = new SoNodeSensor([](void* data, SoSensor*) {
        auto thisClass = static_cast<ViewProviderPad*>(data);
        thisClass->updateOVPPosition();
    }, this);
    cameraSensor->attach(getViewer()->getCamera());
}

void ViewProviderPad::updateOVPPosition() {
    if (!dragger)
        return;

    auto viewer = getViewer();
    auto placement = getDraggerPlacement();
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto base_pos = placement.getPosition();
    base_pos += extrude->getProfileNormal() * (10.0f + 2.5f) * (dragger->getDragger()->autoScaleResult.getValue());

    QPoint pos = viewer->toQPoint(viewer->getPointOnViewport(Base::convertTo<SbVec3f>(base_pos)));
    QSize ovpSize = ovp->size();
    ovp->move({
        static_cast<int>(pos.x() - ovpSize.width()/2.0),
        static_cast<int>(pos.y() - ovpSize.height())
    });
    // TODO: Currently the bottomleft of OVP is clamped with the tip of the
    // dragger but it should position itself according to the orientation of the
    // dragger.
}

Gui::View3DInventorViewer* ViewProviderPad::getViewer() {
    Gui::Document* doc = getDocument();
    auto view = dynamic_cast<Gui::View3DInventor*>(doc->getViewOfViewProvider(this));
    return view->getViewer();
}

void ViewProviderPad::dragStartCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    Base::Console().message("Started dragging\n");

    auto vp = static_cast<ViewProviderPad*>(data);
    vp->ovp->setDisabled(true);
}

void ViewProviderPad::dragFinishCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    Base::Console().message("Finished dragging\n");

    auto vp = static_cast<ViewProviderPad*>(data);
    vp->ovp->setDisabled(false);
    vp->ovp->setReadOnly(false);
}

void ViewProviderPad::dragMotionCallback(void *data, [[maybe_unused]] SoDragger *d)
{
    auto vp = static_cast<ViewProviderPad*>(data);

    auto padLength = vp->getPadLengthFromDragger();
    vp->dialog->setPadLength(padLength);
    vp->ovp->setValue(padLength);
    // vp->updatePosition(padLength);

    Base::Console().message("Continuing dragging, Pad Length: %lf\n", padLength);
}

void PartDesignGui::ViewProviderPad::setDraggerPosFromUI(double value)
{
    updatePosition(value);
    dialog->setPadLength(value);
    ovp->setValue(value);
}
