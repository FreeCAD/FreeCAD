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
#include "Mod/PartDesign/App/FeatureExtrude.h"
#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMenu>
#include <algorithm>
#endif

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoTransform.h>
#include "TaskPadParameters.h"
#include "ViewProviderPad.h"
#include <Gui/SoFCCSysDragger.h>
#include <Gui/ViewParams.h>
#include <qnamespace.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Gui/Inventor/SoFCPlacementIndicatorKit.h>
#include <Base/Converter.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventorViewer.h>

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
    dragger = new Gui::SoFCCSysDragger();
    dragger->setAxisColors(0x00000000, 0x00000000, 0xFF0000FF);
    dragger->draggerSize.setValue(Gui::ViewParams::instance()->getDraggerScale());

    dragger->addStartCallback(dragStartCallback, this);
    dragger->addFinishCallback(dragFinishCallback, this);
    dragger->addMotionCallback(dragMotionCallback, this);

    dialog = new TaskDlgPadParameters(this);
    Gui::Control().showDialog(dialog);

    setDraggerLabel();
    updatePosition(dialog->getPadLength());
    hideUnWantedAxes();

    return true;
}

void ViewProviderPad::unsetEdit([[maybe_unused]] int ModNum)
{
    dragger.reset();
}

void ViewProviderPad::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);

    if (dragger && viewer) {
        dragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());

        auto originPlacement = App::GeoFeature::getGlobalPlacement(getObject()) * getObjectPlacement().inverse();
        auto mat = originPlacement.toMatrix();

        viewer->getDocument()->setEditingTransform(mat);
        viewer->setupEditingRoot(dragger, &mat);
    }
}

void ViewProviderPad::updatePosition(double padLength)
{
    if (!dragger) {
        return;
    }

    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto shape = extrude->getProfileShape();

    Base::Vector3d center;
    shape.getCenterOfGravity(center);

    center += extrude->getProfileNormal() * padLength;
    dragger->translation.setValue(Base::convertTo<SbVec3f>(center));

    auto rotation = Base::Rotation::fromNormalVector(extrude->getProfileNormal());
    dragger->rotation.setValue(Base::convertTo<SbRotation>(rotation));

    dragger->clearIncrementCounts();
}

Base::Placement ViewProviderPad::getDraggerPlacement()
{
    return {Base::convertTo<Base::Vector3d>(dragger->translation.getValue()),
            Base::convertTo<Base::Rotation>(dragger->rotation.getValue())};
}


double PartDesignGui::ViewProviderPad::getPadLengthFromDragger()
{
    auto extrude = getObject<PartDesign::FeatureExtrude>();
    auto shape = extrude->getProfileShape();

    Base::Vector3d center;
    shape.getCenterOfGravity(center);

    auto placement = getDraggerPlacement();

    auto diff = (placement.getPosition() - center);
    if (diff.Dot(extrude->getProfileNormal()) >=0) {
        return diff.Length();
    }

    return 0.001; // hack value
}

void ViewProviderPad::hideUnWantedAxes()
{
    dragger->hideRotationX();
    dragger->hideRotationY();
    dragger->hideRotationZ();
    dragger->hideTranslationX();
    dragger->hideTranslationY();
    dragger->hidePlanarTranslationXY();
    dragger->hidePlanarTranslationYZ();
    dragger->hidePlanarTranslationZX();
}

void ViewProviderPad::dragStartCallback(void *data, SoDragger *d)
{
    auto vp = static_cast<ViewProviderPad*>(data);
    // Can we assume that the padlength won't change once we start dragging?
    vp->padLength = vp->dialog->getPadLength();

    Base::Console().Message("Started dragging\n");
}

void ViewProviderPad::dragFinishCallback(void *data, SoDragger *d)
{
    Base::Console().Message("Finished dragging\n");
}

void ViewProviderPad::dragMotionCallback(void *data, SoDragger *d)
{
    auto vp = static_cast<ViewProviderPad*>(data);

    auto padLength = std::max<double>(vp->getPadLengthFromDragger(), 0.001);
    vp->dialog->setPadLength(padLength);
    // This is hack used due to pad of 0 length giving arbitrary size in the model
    if (std::abs(padLength - 0.001) <= 0.01) {
        vp->updatePosition(padLength);
        vp->setDraggerLabel();
    }

    Base::Console().Message("Continuing dragging, Pad Length: %lf\n", padLength);
}

void PartDesignGui::ViewProviderPad::setDraggerPosFromUI(double value)
{
    updatePosition(value);
    setDraggerLabel();
}

void PartDesignGui::ViewProviderPad::setDraggerLabel()
{
    dragger->zAxisLabel.setValue(fmt::format("Length: {}", dialog->getPadLength()).c_str());
}
