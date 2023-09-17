/// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/sensors/SoNodeSensor.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoTransform.h>
#endif // _PreComp_

#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "EditableDatumLabel.h"



using namespace Gui;


struct NodeData {
    EditableDatumLabel* label;
};

EditableDatumLabel::EditableDatumLabel(View3DInventorViewer* view, Base::Placement plc, SbColor color, bool autoDistance)
    : isSet(false)
    , autoDistance(autoDistance)
    , autoDistanceReverse(false)
    , viewer(view)
    , spinBox(nullptr)
    , cameraSensor(nullptr)
{
    root = new SoAnnotation;
    root->ref();
    root->renderCaching = SoSeparator::OFF;

    transform = new SoTransform();
    transform->ref();
    root->addChild(transform);

    label = new SoDatumLabel();
    label->ref();
    label->string = " ";
    label->textColor = color;
    label->size.setValue(17);
    label->lineWidth = 2.0;
    label->useAntialiasing = false;
    label->datumtype = SoDatumLabel::DISTANCE;
    label->param1 = 0.;
    label->param2 = 0.;
    if (autoDistance)
        setLabelRecommendedDistance();
    root->addChild(label);

    setPlacement(plc);
}

EditableDatumLabel::~EditableDatumLabel()
{
    deactivate();
    transform->unref();
    root->unref();
    label->unref();
}

void EditableDatumLabel::activate()
{
    if (viewer) {
        static_cast<SoSeparator*>(viewer->getSceneGraph())->addChild(root);
    }

    //track camera movements to update spinbox position.
    NodeData* info = new NodeData{ this };
    cameraSensor = new SoNodeSensor([](void* data, SoSensor* sensor) {
        Q_UNUSED(sensor);
        NodeData* info = static_cast<NodeData*>(data);
        info->label->positionSpinbox();
        if (info->label->autoDistance)
            info->label->setLabelRecommendedDistance();
    }, info);
    cameraSensor->attach(viewer->getCamera());
}

void EditableDatumLabel::deactivate()
{
    stopEdit();

    if (cameraSensor)
        cameraSensor->detach();

    if (viewer) {
        static_cast<SoSeparator*>(viewer->getSceneGraph())->removeChild(root);
    }
}

void EditableDatumLabel::startEdit(double val, QObject* eventFilteringObj)
{
    QWidget* mdi = viewer->parentWidget();

    label->string = " ";

    spinBox = new QuantitySpinBox(mdi);
    spinBox->setUnit(Base::Unit::Length);
    spinBox->setMinimum(-INT_MAX);
    spinBox->setMaximum(INT_MAX);
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setKeyboardTracking(false);
    if(eventFilteringObj)
        spinBox->installEventFilter(eventFilteringObj);

    spinBox->show();
    setSpinboxValue(val);
    //Note: adjustSize apparantly uses the Min/Max values to set the size. So if we don't set them to INT_MAX, the spinbox are much to big.
    spinBox->adjustSize();
    setFocusToSpinbox();

    connect(spinBox, qOverload<double>(&QuantitySpinBox::valueChanged),
        this, [this](double value) {
        this->isSet = true;
        this->valueChanged(value);
    });
}

void EditableDatumLabel::stopEdit()
{
    if (spinBox) {
        spinBox->deleteLater();
        spinBox = nullptr;
    }
}

double EditableDatumLabel::getValue()
{
    return spinBox->rawValue();
}

void EditableDatumLabel::setSpinboxValue(double val)
{
    if (!spinBox) {
        Base::Console().Warning("Spinbox doesn't exist in EditableDatumLabel::setSpinboxValue.");
        return;
    }

    QSignalBlocker block(spinBox);
    spinBox->setValue(val);
    positionSpinbox();

    if (spinBox->hasFocus()) {
        spinBox->selectNumber();
    }
}
void EditableDatumLabel::setFocusToSpinbox()
{
    if (!spinBox) {
        Base::Console().Warning("Spinbox doesn't exist in EditableDatumLabel::setFocusToSpinbox.");
        return;
    }

    spinBox->setFocus();
    spinBox->selectNumber();
}

void EditableDatumLabel::positionSpinbox()
{
    if (!spinBox) {
        return;
    }

    QSize wSize = spinBox->size();
    QPoint pxCoord = viewer->toQPoint(viewer->getPointOnViewport(getTextCenterPoint()));
    pxCoord.setX(std::max(pxCoord.x() - wSize.width() / 2, 0));
    pxCoord.setY(std::max(pxCoord.y() - wSize.height() / 2, 0));
    spinBox->move(pxCoord);
}

const SbVec3f EditableDatumLabel::getTextCenterPoint()
{
    //Here we need the 3d point and not the 2d point as are the SoLabel points.
    // First we get the 2D point (on the sketch/image plane) of the middle of the text label.
    SbVec3f point2D = label->textOffset;
    // Get the translation and rotation values from the transform
    SbVec3f translation = transform->translation.getValue();
    SbRotation rotation = transform->rotation.getValue();

    // Calculate the inverse transformation
    SbVec3f invTranslation = -translation;
    SbRotation invRotation = rotation.inverse();

    // Transform the 2D coordinates to 3D
    // Plane form
    SbVec3f RX(1, 0, 0), RY(0, 1, 0);

    // move to position of Sketch
    invRotation.multVec(RX, RX);
    invRotation.multVec(RY, RY);
    invRotation.multVec(invTranslation, invTranslation);

    // we use invTranslation as the Base because in setPlacement we set transform->translation using
    // placement.getPosition() to fix the Zoffset. But this applies the X & Y translation too.
    Base::Vector3d pos(invTranslation[0], invTranslation[1], invTranslation[2]);
    Base::Vector3d RXb(RX[0], RX[1], RX[2]);
    Base::Vector3d RYb(RY[0], RY[1], RY[2]);
    Base::Vector3d P2D(point2D[0], point2D[1], point2D[2]);
    P2D.TransformToCoordinateSystem(pos, RXb, RYb);

    return SbVec3f(P2D.x, P2D.y, P2D.z);
}

void EditableDatumLabel::setPlacement(Base::Placement plc)
{
    double x, y, z, w;
    plc.getRotation().getValue(x, y, z, w);
    //Base::Console().Warning("x%f, y%f, z%f, w%f\n", x, y, z, w);
    transform->rotation.setValue(x, y, z, w);
    Base::Vector3d pos = plc.getPosition();
    transform->translation.setValue(pos.x, pos.y, pos.z);

    Base::Vector3d RN(0, 0, 1);
    RN = plc.getRotation().multVec(RN);
    label->norm.setValue(SbVec3f(RN.x, RN.y, RN.z));
}

void EditableDatumLabel::setColor(SbColor color)
{
    label->textColor = color;
}

void EditableDatumLabel::setFocus()
{
    if (spinBox) {
        spinBox->selectNumber();
    }
}

void EditableDatumLabel::setPoints(SbVec3f p1, SbVec3f p2)
{
    label->setPoints(p1, p2);
    //TODO: here the position of the spinbox is not going to be center of p1, p2 because the point given by getTextCenterPoint
    // is not updated yet. it will be only on redraw so it is actually positioning on previous position.

    positionSpinbox();
    if (autoDistance)
        setLabelRecommendedDistance();
}

void EditableDatumLabel::setPoints(Base::Vector3d p1, Base::Vector3d p2)
{
    setPoints(SbVec3f(p1.x, p1.y, p1.z), SbVec3f(p2.x, p2.y, p2.z));
}

void EditableDatumLabel::setLabelType(SoDatumLabel::Type type)
{
    label->datumtype = type;
}

void EditableDatumLabel::setLabelDistance(double distance)
{
    label->param1 = distance;
}

void EditableDatumLabel::setLabelRecommendedDistance()
{
    // Takes the 3d view size, and set the label distance to a % of that, such that the distance does not depend on the zoom level.
    float width = -1.;
    float length = -1.;
    viewer->getDimensions(width, length);

    if (width == -1. || length == -1.)
        return;

    label->param1 = (autoDistanceReverse ? -1 : 1) * (width + length) * 0.03;
}

void EditableDatumLabel::setLabelAutoDistanceReverse(bool val)
{
    autoDistanceReverse = val;
}

#include "moc_EditableDatumLabel.cpp"
