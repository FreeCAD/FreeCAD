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
# include <limits>
# include <Inventor/sensors/SoNodeSensor.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoSwitch.h>
#endif // _PreComp_

#include <QEvent>
#include <QKeyEvent>

#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "EditableDatumLabel.h"



using namespace Gui;


struct NodeData {
    EditableDatumLabel* label;
};

EditableDatumLabel::EditableDatumLabel(View3DInventorViewer* view,
                                       const Base::Placement& plc,
                                       SbColor color,
                                       bool autoDistance,
                                       bool avoidMouseCursor)
    : isSet(false)
    , hasFinishedEditing(false)
    , autoDistance(autoDistance)
    , autoDistanceReverse(false)
    , avoidMouseCursor(avoidMouseCursor)
    , value(0.0)
    , viewer(view)
    , spinBox(nullptr)
    , cameraSensor(nullptr)
    , function(Function::Positioning)
{
    // NOLINTBEGIN
    root = new SoSwitch;
    root->ref();

    annotation = new SoAnnotation;
    annotation->ref();
    annotation->renderCaching = SoSeparator::OFF;
    root->addChild(annotation);

    transform = new SoTransform();
    transform->ref();
    annotation->addChild(transform);

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
    label->param3 = 0.;
    if (autoDistance) {
        setLabelRecommendedDistance();
    }
    annotation->addChild(label);

    setPlacement(plc);
    // NOLINTEND

    static_cast<SoSeparator*>(viewer->getSceneGraph())->addChild(root); // NOLINT
}

EditableDatumLabel::~EditableDatumLabel()
{
    deactivate();
    transform->unref();
    annotation->unref();
    root->unref();
    label->unref();
}

void EditableDatumLabel::activate()
{
    if (!viewer || isActive()) {
        return;
    }

    root->whichChild = 0;

    //track camera movements to update spinbox position.
    auto info = new NodeData{ this };
    cameraSensor = new SoNodeSensor([](void* data, SoSensor* sensor) {
        Q_UNUSED(sensor);
        auto info = static_cast<NodeData*>(data);
        info->label->positionSpinbox();
        if (info->label->autoDistance) {
            info->label->setLabelRecommendedDistance();
        }
    }, info);
    cameraSensor->attach(viewer->getCamera());
}

void EditableDatumLabel::deactivate()
{
    stopEdit();

    if (cameraSensor) {
        auto data = static_cast<NodeData*>(cameraSensor->getData());
        delete data;
        cameraSensor->detach();
        delete cameraSensor;
        cameraSensor = nullptr;
    }

    root->whichChild = SO_SWITCH_NONE;
}

void EditableDatumLabel::startEdit(double val, QObject* eventFilteringObj, bool visibleToMouse)
{
    if (isInEdit()) {
        return;
    }

    QWidget* mdi = viewer->parentWidget();

    label->string = " ";

    spinBox = new QuantitySpinBox(mdi);
    spinBox->setUnit(Base::Unit::Length);
    spinBox->setMinimum(-std::numeric_limits<int>::max());
    spinBox->setMaximum(std::numeric_limits<int>::max());
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setFocusPolicy(Qt::ClickFocus); // prevent passing focus with tab.
    spinBox->setAutoNormalize(false);
    spinBox->setKeyboardTracking(true);
    spinBox->installEventFilter(this);

    if (eventFilteringObj) {
        spinBox->installEventFilter(eventFilteringObj);
    }

    if (!visibleToMouse) {
        setSpinboxVisibleToMouse(visibleToMouse);
    }

    spinBox->show();
    setSpinboxValue(val);
    //Note: adjustSize apparently uses the Min/Max values to set the size. So if we don't set them to INT_MAX, the spinbox are much too big.
    spinBox->adjustSize();
    setFocusToSpinbox();

    const auto validateAndFinish = [this]() {
        // this event can be fired after spinBox was already disposed
        // in such case we need to skip processing that event
        if (!spinBox) {
            return;
        }

        if (!spinBox->hasValidInput()) {
            // unset parameters in DrawSketchController, this is needed in a case
            // when user removes values we reset state of the OVP
            Q_EMIT this->parameterUnset();
            return;
        }

        value = spinBox->rawValue();

        isSet = true;
        Q_EMIT this->valueChanged(value);
    };

    connect(spinBox, qOverload<double>(&QuantitySpinBox::valueChanged), this, validateAndFinish);
}

bool EditableDatumLabel::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {

            if (auto* spinBox = qobject_cast<QAbstractSpinBox*>(watched)) {
                this->hasFinishedEditing = true;
                Q_EMIT this->valueChanged(this->value);
                return false;
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

void EditableDatumLabel::stopEdit()
{
    if (spinBox) {
        // write the spinbox value in the label.
        Base::Quantity quantity = spinBox->value();

        double factor{};
        std::string unitStr;
        std::string valueStr;
        valueStr = quantity.getUserString(factor, unitStr);
        label->string = SbString(valueStr.c_str());

        spinBox->deleteLater();
        spinBox = nullptr;
    }
}

bool EditableDatumLabel::isActive() const
{
    return cameraSensor != nullptr;
}

bool EditableDatumLabel::isInEdit() const
{
    return spinBox != nullptr;
}


double EditableDatumLabel::getValue() const
{
    // We use value rather than spinBox->rawValue() in case edit stopped.
    return value;
}

void EditableDatumLabel::setSpinboxValue(double val, const Base::Unit& unit)
{
    if (!spinBox) {
        Base::Console().developerWarning("EditableDatumLabel::setSpinboxValue", "Spinbox doesn't exist in");
        return;
    }

    QSignalBlocker block(spinBox);
    spinBox->setValue(Base::Quantity(val, unit));
    value = val;
    positionSpinbox();

    if (spinBox->hasFocus()) {
        spinBox->selectNumber();
    }
}

void EditableDatumLabel::setFocusToSpinbox()
{
    if (!spinBox) {
        Base::Console().developerWarning("EditableDatumLabel::setFocusToSpinbox", "Spinbox doesn't exist in");
        return;
    }
    if (!spinBox->hasFocus()) {
        spinBox->setFocus();
        spinBox->selectNumber();
    }
}

void EditableDatumLabel::positionSpinbox()
{
    if (!spinBox) {
        return;
    }

    if (spinBox->hasFocus()) {
        spinBox->raise();
    }

    QSize wSize = spinBox->size();
    QSize vSize = viewer->size();
    QPoint pxCoord = viewer->toQPoint(viewer->getPointOnViewport(getTextCenterPoint()));

    int posX = std::min(std::max(pxCoord.x() - wSize.width() / 2, 0), vSize.width() - wSize.width());
    int posY = std::min(std::max(pxCoord.y() - wSize.height() / 2, 0), vSize.height() - wSize.height());

    if (avoidMouseCursor) {
        QPoint cursorPos = viewer->mapFromGlobal(QCursor::pos());
        int margin = static_cast<int>(wSize.height() * 0.7); // NOLINT
        if ((cursorPos.x() > posX - margin && cursorPos.x() < posX + wSize.width() + margin)
            && (cursorPos.y() > posY - margin && cursorPos.y() < posY + wSize.height() + margin)) {
            posY = cursorPos.y() + ((cursorPos.y() > pxCoord.y()) ? - wSize.height() - margin : margin);
        }
    }

    pxCoord.setX(posX);
    pxCoord.setY(posY);
    spinBox->move(pxCoord);
}

SbVec3f EditableDatumLabel::getTextCenterPoint() const
{
    //Here we need the 3d point and not the 2d point as are the SoLabel points.
    // First we get the 2D point (on the sketch/image plane) of the middle of the text label.
    SbVec3f point2D = label->getLabelTextCenter();
    // Get the translation and rotation values from the transform
    SbVec3f translation = transform->translation.getValue();
    SbRotation rotation = transform->rotation.getValue();

    // Calculate the inverse transformation
    SbVec3f invTranslation = -translation;
    SbRotation invRotation = rotation.inverse();

    // Transform the 2D coordinates to 3D
    // Plane form
    SbVec3f RX(1, 0, 0);
    SbVec3f RY(0, 1, 0);

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

    return {float(P2D.x), float(P2D.y), float(P2D.z)};
}

void EditableDatumLabel::setPlacement(const Base::Placement& plc)
{
    double x{}, y{}, z{}, w{}; // NOLINT
    plc.getRotation().getValue(x, y, z, w);
    transform->rotation.setValue(x, y, z, w); // NOLINT

    Base::Vector3d pos = plc.getPosition();
    transform->translation.setValue(float(pos.x), float(pos.y), float(pos.z));

    Base::Vector3d RN(0, 0, 1);
    RN = plc.getRotation().multVec(RN);
    label->norm.setValue(SbVec3f(float(RN.x), float(RN.y), float(RN.z)));
}

// NOLINTNEXTLINE
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
    if (autoDistance) {
        setLabelRecommendedDistance();
    }
}

void EditableDatumLabel::setPoints(Base::Vector3d p1, Base::Vector3d p2)
{
    setPoints(SbVec3f(float(p1.x), float(p1.y), float(p1.z)),
              SbVec3f(float(p2.x), float(p2.y), float(p2.z)));
}

// NOLINTNEXTLINE
void EditableDatumLabel::setLabelType(SoDatumLabel::Type type, Function funct)
{
    label->datumtype = type;
    function = funct;
}

// NOLINTNEXTLINE
void EditableDatumLabel::setLabelDistance(double val)
{
    label->param1 = float(val);
}

// NOLINTNEXTLINE
void EditableDatumLabel::setLabelStartAngle(double val)
{
    label->param2 = float(val);
}

// NOLINTNEXTLINE
void EditableDatumLabel::setLabelRange(double val)
{
    label->param3 = float(val);
}

void EditableDatumLabel::setLabelRecommendedDistance()
{
    // Takes the 3d view size, and set the label distance to a % of that, such that the distance does not depend on the zoom level.
    float width = -1.;
    float length = -1.;
    viewer->getDimensions(width, length);

    if (width == -1. || length == -1.) {
        return;
    }

    label->param1 = (autoDistanceReverse ? -1.0F : 1.0F) * (width + length) * 0.03F; // NOLINT
}

void EditableDatumLabel::setLabelAutoDistanceReverse(bool val)
{
    autoDistanceReverse = val;
}

void EditableDatumLabel::setSpinboxVisibleToMouse(bool val)
{
    spinBox->setAttribute(Qt::WA_TransparentForMouseEvents, !val);
}

EditableDatumLabel::Function EditableDatumLabel::getFunction()
{
    return function;
}

#include "moc_EditableDatumLabel.cpp" // NOLINT

