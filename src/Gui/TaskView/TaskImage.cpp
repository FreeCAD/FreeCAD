// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QDialog>
# include <QPushButton>
# include <QAction>
# include <QKeyEvent>
# include <map>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoButtonEvent.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/events/SoKeyboardEvent.h>
#endif

// clang-format off
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
// clang-format on

#include <Base/Console.h>
#include <Base/Precision.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Camera.h>
#include <Gui/Document.h>
#include <Gui/SoDatumLabel.h>
#include <Gui/EditableDatumLabel.h>

#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/TaskView/TaskView.h>

#include "TaskImage.h"
#include "ui_TaskImage.h"


using namespace Gui;

/* TRANSLATOR Gui::TaskImage */

TaskImage::TaskImage(Image::ImagePlane* obj, QWidget* parent)
  : QWidget(parent)
  , ui(new Ui_TaskImage)
  , feature(obj)
  , aspectRatio(1.0)
{
    ui->setupUi(this);
    ui->groupBoxCalibration->hide();

    initialiseTransparency();

    // NOLINTNEXTLINE
    aspectRatio = obj->XSize.getValue() / obj->YSize.getValue();

    connectSignals();
}

TaskImage::~TaskImage()
{
    if (!feature.expired() && scale) {
        if (scale->isActive()) {
            scale->deactivate();
        }
        scale->deleteLater();
    }
}

void TaskImage::connectSignals()
{
    connect(ui->Reverse_checkBox, &QCheckBox::clicked,
        this, &TaskImage::onPreview);
    connect(ui->XY_radioButton, &QRadioButton::clicked,
        this, &TaskImage::onPreview);
    connect(ui->XZ_radioButton, &QRadioButton::clicked,
        this, &TaskImage::onPreview);
    connect(ui->YZ_radioButton, &QRadioButton::clicked,
        this, &TaskImage::onPreview);
    connect(ui->spinBoxZ, qOverload<double>(&QuantitySpinBox::valueChanged),
        this, &TaskImage::onPreview);
    connect(ui->spinBoxX, qOverload<double>(&QuantitySpinBox::valueChanged),
        this, &TaskImage::onPreview);
    connect(ui->spinBoxY, qOverload<double>(&QuantitySpinBox::valueChanged),
        this, &TaskImage::onPreview);
    connect(ui->spinBoxRotation, qOverload<double>(&QuantitySpinBox::valueChanged),
        this, &TaskImage::onPreview);
    connect(ui->spinBoxTransparency, qOverload<int>(&QSpinBox::valueChanged),
        this, &TaskImage::changeTransparency);
    connect(ui->sliderTransparency, qOverload<int>(&QSlider::valueChanged),
        this, &TaskImage::changeTransparency);

    connect(ui->spinBoxWidth, qOverload<double>(&QuantitySpinBox::valueChanged),
        this, &TaskImage::changeWidth);
    connect(ui->spinBoxHeight, qOverload<double>(&QuantitySpinBox::valueChanged),
        this, &TaskImage::changeHeight);
    connect(ui->pushButtonScale, &QPushButton::clicked,
        this, &TaskImage::onInteractiveScale);
    connect(ui->pushButtonApply, &QPushButton::clicked,
        this, &TaskImage::acceptScale);
    connect(ui->pushButtonCancel, &QPushButton::clicked,
        this, &TaskImage::rejectScale);
}

void TaskImage::initialiseTransparency()
{
    // NOLINTBEGIN
    auto vp = Application::Instance->getViewProvider(feature.get());
    App::Property* prop = vp->getPropertyByName("Transparency");
    if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
        auto Transparency = static_cast<App::PropertyInteger*>(prop);
        ui->spinBoxTransparency->setValue(Transparency->getValue());
        ui->sliderTransparency->setValue(Transparency->getValue());
    }
    // NOLINTEND
}

void TaskImage::changeTransparency(int val)
{
    if (feature.expired()) {
        return;
    }

    auto vp = Application::Instance->getViewProvider(feature.get());
    App::Property* prop = vp->getPropertyByName("Transparency");
    if (auto Transparency = dynamic_cast<App::PropertyInteger*>(prop)) {
        Transparency->setValue(val);

        QSignalBlocker block(ui->spinBoxTransparency);
        QSignalBlocker blocks(ui->sliderTransparency);
        ui->spinBoxTransparency->setValue(val);
        ui->sliderTransparency->setValue(val);
    }
}

void TaskImage::changeWidth(double val)
{
    if (!feature.expired()) {
        feature->XSize.setValue(val);

        if (ui->checkBoxRatio->isChecked()) {
            QSignalBlocker block(ui->spinBoxWidth);
            ui->spinBoxHeight->setValue(val / aspectRatio);
        }
    }
}

void TaskImage::changeHeight(double val)
{
    if (!feature.expired()) {
        feature->YSize.setValue(val);

        if (ui->checkBoxRatio->isChecked()) {
            QSignalBlocker block(ui->spinBoxHeight);
            ui->spinBoxWidth->setValue(val * aspectRatio);
        }
    }
}

View3DInventorViewer* TaskImage::getViewer() const
{
    if (!feature.expired()) {
        auto vp = Application::Instance->getViewProvider(feature.get());
        auto doc = static_cast<ViewProviderDocumentObject*>(vp)->getDocument(); // NOLINT
        auto view = dynamic_cast<View3DInventor*>(doc->getViewOfViewProvider(vp));
        if (view) {
            return view->getViewer();
        }
    }

    return nullptr;
}

void TaskImage::scaleImage(double factor)
{
    if (!feature.expired()) {
        feature->XSize.setValue(feature->XSize.getValue() * factor);
        feature->YSize.setValue(feature->YSize.getValue() * factor);

        QSignalBlocker blockW(ui->spinBoxWidth);
        ui->spinBoxWidth->setValue(feature->XSize.getValue());
        QSignalBlocker blockH(ui->spinBoxHeight);
        ui->spinBoxHeight->setValue(feature->YSize.getValue());
    }
}

void TaskImage::startScale()
{
    scale->activate();
    ui->pushButtonScale->hide();
    ui->groupBoxCalibration->show();
    ui->pushButtonApply->setEnabled(false);
}

void TaskImage::acceptScale()
{
    scaleImage(scale->getScaleFactor());
    rejectScale();
}

void TaskImage::enableApplyBtn()
{
    ui->pushButtonApply->setEnabled(true);
}

void TaskImage::rejectScale()
{
    scale->deactivate();
    ui->pushButtonScale->show();
    ui->groupBoxCalibration->hide();
}

void TaskImage::onInteractiveScale()
{
    if (!feature.expired() && !scale) {
        View3DInventorViewer* viewer = getViewer();
        if (viewer) {
            auto vp = Application::Instance->getViewProvider(feature.get());
            scale = new InteractiveScale(viewer, vp, feature->globalPlacement());
            connect(scale, &InteractiveScale::scaleRequired,
                this, &TaskImage::acceptScale);
            connect(scale, &InteractiveScale::scaleCanceled,
                this, &TaskImage::rejectScale);
            connect(scale, &InteractiveScale::enableApplyBtn,
                this, &TaskImage::enableApplyBtn);
        }
    }

    startScale();
}

void TaskImage::open()
{
    if (!feature.expired()) {
        App::Document* doc = feature->getDocument();
        doc->openTransaction(QT_TRANSLATE_NOOP("Command", "Edit image"));
        restore(feature->Placement.getValue());
    }
}

void TaskImage::accept()
{
    if (!feature.expired()) {
        App::Document* doc = feature->getDocument();
        doc->commitTransaction();
        doc->recompute();
    }
}

void TaskImage::reject()
{
    if (!feature.expired()) {
        App::Document* doc = feature->getDocument();
        doc->abortTransaction();
        feature->purgeTouched();
    }
}

void TaskImage::onPreview()
{
    updateIcon();
    updatePlacement();
}

// NOLINTNEXTLINE
void TaskImage::restoreAngles(const Base::Rotation& rot)
{
    double yaw{};
    double pitch{};
    double roll{};
    rot.getYawPitchRoll(yaw, pitch, roll);

    bool reverse = false;

    const double tol = 1.0e-5;
    const double angle1 = 90.0;
    const double angle2 = 180.0;

    auto isTopOrBottom = [=](bool& reverse) {
        if (fabs(pitch) < tol && (fabs(roll) < tol || fabs(roll - angle2) < tol)) {
            if (fabs(roll - angle2) < tol) {
                reverse = true;
            }
            return true;
        }

        return false;
    };
    auto isFrontOrRear = [=](bool& reverse) {
        if (fabs(roll - angle1) < tol && (fabs(yaw) < tol || fabs(yaw - angle2) < tol)) {
            if (fabs(yaw - angle2) < tol) {
                reverse = true;
            }
            return true;
        }

        return false;
    };
    auto isRightOrLeft = [=](bool& reverse) {
        if (fabs(roll - angle1) < tol && (fabs(yaw - angle1) < tol || fabs(yaw + angle1) < tol)) {
            if (fabs(yaw + angle1) < tol) {
                reverse = true;
            }
            return true;
        }

        return false;
    };

    if (isTopOrBottom(reverse)) {
        int inv = reverse ? -1 : 1;
        ui->XY_radioButton->setChecked(true);
        ui->spinBoxRotation->setValue(yaw * inv);
    }
    else if (isFrontOrRear(reverse)) {
        ui->XZ_radioButton->setChecked(true);
        ui->spinBoxRotation->setValue(-pitch);
    }
    else if (isRightOrLeft(reverse)) {
        ui->YZ_radioButton->setChecked(true);
        ui->spinBoxRotation->setValue(-pitch);
    }
    ui->Reverse_checkBox->setChecked(reverse);
}

void TaskImage::restore(const Base::Placement& plm)
{
    if (feature.expired()) {
        return;
    }

    QSignalBlocker blockW(ui->spinBoxWidth);
    QSignalBlocker blockH(ui->spinBoxHeight);
    ui->spinBoxWidth->setValue(feature->XSize.getValue());
    ui->spinBoxHeight->setValue(feature->YSize.getValue());

    Base::Rotation rot = plm.getRotation(); // NOLINT
    Base::Vector3d pos = plm.getPosition();

    restoreAngles(rot);

    Base::Vector3d R0(0, 0, 0);
    Base::Vector3d RX(1, 0, 0);
    Base::Vector3d RY(0, 1, 0);

    RX = rot.multVec(RX);
    RY = rot.multVec(RY);
    pos.TransformToCoordinateSystem(R0, RX, RY);
    ui->spinBoxX->setValue(pos.x);
    ui->spinBoxY->setValue(pos.y);
    ui->spinBoxZ->setValue(pos.z);

    onPreview();
}

void TaskImage::updatePlacement()
{
    double angle = ui->spinBoxRotation->value().getValue();
    bool reverse = ui->Reverse_checkBox->isChecked();

    // NOLINTBEGIN
    Base::Placement Pos;
    Base::Rotation rot;
    double dir = reverse ? 180. : 0.;
    int inv = reverse ? -1 : 1;

    if (ui->XY_radioButton->isChecked()) {
        rot.setYawPitchRoll(inv * angle, 0., dir);
    }
    else if (ui->XZ_radioButton->isChecked()) {
        rot.setYawPitchRoll(dir, -angle, 90.);
    }
    else if (ui->YZ_radioButton->isChecked()) {
        rot.setYawPitchRoll(90. - dir, -angle, 90.);
    }
    // NOLINTEND

    Base::Vector3d offset = Base::Vector3d(ui->spinBoxX->value().getValue(), ui->spinBoxY->value().getValue(), ui->spinBoxZ->value().getValue());
    offset = rot.multVec(offset);
    Pos = Base::Placement(offset, rot);

    if (!feature.expired()) {
        feature->Placement.setValue(Pos);
        if (scale) {
            scale->setPlacement(feature->globalPlacement());
        }
    }
}

void TaskImage::updateIcon()
{
    std::string icon;
    bool reverse = ui->Reverse_checkBox->isChecked();
    if (ui->XY_radioButton->isChecked()) {
        icon = reverse ? "view-bottom" : "view-top";
    }
    else if (ui->XZ_radioButton->isChecked()) {
        icon = reverse ? "view-rear" : "view-front";
    }
    else if (ui->YZ_radioButton->isChecked()) {
        icon = reverse ? "view-left" : "view-right";
    }

    ui->previewLabel->setPixmap(
        Gui::BitmapFactory().pixmapFromSvg(icon.c_str(),
            ui->previewLabel->size()));
}

// ----------------------------------------------------------------------------

InteractiveScale::InteractiveScale(View3DInventorViewer* view,
                                   ViewProvider* vp,
                                   const Base::Placement& plc) // NOLINT
    : active(false)
    , placement(plc)
    , viewer(view)
    , viewProv(vp)
    , midPoint(SbVec3f(0,0,0))
{
    measureLabel = new EditableDatumLabel(viewer, placement, SbColor(1.0F, 0.149F, 0.0F)); //NOLINT
}

InteractiveScale::~InteractiveScale()
{
    delete measureLabel;
}

void InteractiveScale::activate()
{
    if (viewer) {
        viewer->setEditing(true);
        viewer->addEventCallback(SoLocation2Event::getClassTypeId(), InteractiveScale::getMousePosition, this);
        viewer->addEventCallback(SoButtonEvent::getClassTypeId(), InteractiveScale::soEventFilter, this);
        viewer->setSelectionEnabled(false);
        viewer->getWidget()->setCursor(QCursor(Qt::CrossCursor));
        active = true;
    }
}

void InteractiveScale::deactivate()
{
    if (viewer) {
        points.clear();
        measureLabel->deactivate();
        viewer->setEditing(false);
        viewer->removeEventCallback(SoLocation2Event::getClassTypeId(), InteractiveScale::getMousePosition, this);
        viewer->removeEventCallback(SoButtonEvent::getClassTypeId(), InteractiveScale::soEventFilter, this);
        viewer->setSelectionEnabled(true);
        viewer->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
        active = false;
    }
}

double InteractiveScale::getScaleFactor() const
{
    if ((points[0] - points[1]).length() == 0.) {
        return 1.0;
    }

    return measureLabel->getValue() / (points[0] - points[1]).length();
}

double InteractiveScale::getDistance(const SbVec3f& pt) const
{
    if (points.empty()) {
        return 0.0;
    }

    return (points[0] - pt).length();
}

void InteractiveScale::setDistance(const SbVec3f& pos3d)
{
    Base::Quantity quantity;
    quantity.setValue(getDistance(pos3d));
    quantity.setUnit(Base::Unit::Length);

    //Update the displayed distance
    double factor{};
    QString unitStr;
    QString valueStr;
    valueStr = quantity.getUserString(factor, unitStr);
    measureLabel->label->string = SbString(valueStr.toUtf8().constData());
    measureLabel->label->setPoints(getCoordsOnImagePlane(points[0]), getCoordsOnImagePlane(pos3d));
}

void InteractiveScale::findPointOnImagePlane(SoEventCallback * ecb)
{
    const SoEvent * mbe = ecb->getEvent();
    auto view  = static_cast<Gui::View3DInventorViewer*>(ecb->getUserData());
    std::unique_ptr<SoPickedPoint> pp(view->getPointOnRay(mbe->getPosition(), viewProv));
    if (pp) {
        auto pos3d = pp->getPoint();

        collectPoint(pos3d);
    }
}

void InteractiveScale::collectPoint(const SbVec3f& pos3d)
{
    if (points.empty()) {
        points.push_back(pos3d);

        measureLabel->label->setPoints(getCoordsOnImagePlane(pos3d), getCoordsOnImagePlane(pos3d));
        measureLabel->activate();
    }
    else if (points.size() == 1) {
        double distance = getDistance(pos3d);
        if (distance > Base::Precision::Confusion()) {
            points.push_back(pos3d);

            midPoint = (points[0] + points[1]) / 2;

            measureLabel->startEdit(getDistance(points[1]), this);

            Q_EMIT enableApplyBtn();
        }
        else {
            Base::Console().Warning(std::string("Image scale"), "The second point is too close. Retry!\n");
        }
    }
}

void InteractiveScale::getMousePosition(void * ud, SoEventCallback * ecb)
{
    auto scale = static_cast<InteractiveScale*>(ud);
    const SoEvent* l2e = ecb->getEvent();
    auto view  = static_cast<Gui::View3DInventorViewer*>(ecb->getUserData());

    if (scale->points.size() == 1) {
        ecb->setHandled();

        std::unique_ptr<SoPickedPoint> pp(view->getPointOnRay(l2e->getPosition(), scale->viewProv));
        if (pp) {
            SbVec3f pos3d = pp->getPoint();
            scale->setDistance(pos3d);
        }
    }
}

void InteractiveScale::soEventFilter(void* ud, SoEventCallback* ecb)
{
    auto scale = static_cast<InteractiveScale*>(ud);

    const SoEvent* soEvent = ecb->getEvent();
    if (soEvent->isOfType(SoKeyboardEvent::getClassTypeId())) {
        /* If user presses escape, then we cancel the tool.*/
        const auto kbe = static_cast<const SoKeyboardEvent*>(soEvent); // NOLINT

        if (kbe->getKey() == SoKeyboardEvent::ESCAPE && kbe->getState() == SoButtonEvent::UP) {
            ecb->setHandled();
            Q_EMIT scale->scaleCanceled();
        }
    }
    else if (soEvent->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        const auto mbe = static_cast<const SoMouseButtonEvent*>(soEvent); // NOLINT

        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN)
        {
            ecb->setHandled();
            scale->findPointOnImagePlane(ecb);
        }
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::DOWN)
        {
            ecb->setHandled();
            Q_EMIT scale->scaleCanceled();
        }
    }
}

bool InteractiveScale::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyRelease) {
        auto keyEvent = static_cast<QKeyEvent*>(event); // NOLINT

        /* If user press enter in the spinbox, then we validate the tool.*/
        if ((keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
                && dynamic_cast<QuantitySpinBox*>(object)) {
            Q_EMIT scaleRequired();
        }

        /* If user press escape, then we cancel the tool. Required here as well for when checkbox has focus.*/
        if (keyEvent->key() == Qt::Key_Escape) {
            Q_EMIT scaleCanceled();
        }
    }
    return false;
}

void InteractiveScale::setPlacement(const Base::Placement& plc)
{
    placement = plc;
    measureLabel->setPlacement(plc);
}

SbVec3f InteractiveScale::getCoordsOnImagePlane(const SbVec3f& point)
{
    // Plane form
    Base::Vector3d RX(1, 0, 0);
    Base::Vector3d RY(0, 1, 0);

    // move to position of Sketch
    Base::Rotation tmp(placement.getRotation());
    RX = tmp.multVec(RX);
    RY = tmp.multVec(RY);
    Base::Vector3d pos = placement.getPosition();

    // we use pos as the Base because in setPlacement we set transform->translation using
    // placement.getPosition() to fix the Zoffset. But this applies the X & Y translation too.
    Base::Vector3d pnt(point[0], point[1], point[2]);
    pnt.TransformToCoordinateSystem(pos, RX, RY);

    return {float(pnt.x), float(pnt.y), 0.0F};
}

// ----------------------------------------------------------------------------

TaskImageDialog::TaskImageDialog(Image::ImagePlane* obj)
{
    widget = new TaskImage(obj);
    auto taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("image-plane"), widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskImageDialog::open()
{
    widget->open();
}

bool TaskImageDialog::accept()
{
    widget->accept();
    return true;
}

bool TaskImageDialog::reject()
{
    widget->reject();
    return true;
}

#include "moc_TaskImage.cpp"
