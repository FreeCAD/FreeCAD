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
# include <Inventor/sensors/SoNodeSensor.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoTransform.h>
#endif

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
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
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

    aspectRatio = obj->XSize.getValue() / obj->YSize.getValue();

    QAction* action = new QAction(tr("Allow points outside the image"), this);
    action->setCheckable(true);
    action->setChecked(false);
    ui->pushButtonScale->addAction(action);

    connectSignals();
}

TaskImage::~TaskImage()
{
    if (scale) {
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
    auto vp = Application::Instance->getViewProvider(feature.get());
    App::Property* prop = vp->getPropertyByName("Transparency");
    if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
        auto Transparency = static_cast<App::PropertyInteger*>(prop);
        ui->spinBoxTransparency->setValue(Transparency->getValue());
        ui->sliderTransparency->setValue(Transparency->getValue());
    }
}

void TaskImage::changeTransparency(int val)
{
    if (feature.expired())
        return;

    auto vp = Application::Instance->getViewProvider(feature.get());
    App::Property* prop = vp->getPropertyByName("Transparency");
    if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
        auto Transparency = static_cast<App::PropertyInteger*>(prop);
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
        auto doc = static_cast<ViewProviderDocumentObject*>(vp)->getDocument();
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
    scale->activate(qAsConst(ui->pushButtonScale)->actions()[0]->isChecked());
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
            Base::Placement placement = feature->Placement.getValue();
            scale = new InteractiveScale(viewer, vp, placement);
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

void TaskImage::restore(const Base::Placement& plm)
{
    if (feature.expired())
        return;

    QSignalBlocker blockW(ui->spinBoxWidth);
    QSignalBlocker blockH(ui->spinBoxHeight);
    ui->spinBoxWidth->setValue(feature->XSize.getValue());
    ui->spinBoxHeight->setValue(feature->YSize.getValue());

    Base::Rotation rot = plm.getRotation();
    Base::Vector3d pos = plm.getPosition();

    double yaw, pitch, roll;
    rot.getYawPitchRoll(yaw, pitch, roll);

    double tol = 1.0e-5;
    bool reverse = false;
    if (fabs(pitch) < tol && (fabs(roll) < tol || fabs(roll - 180.) < tol)) {
        if (fabs(roll - 180.) < tol)
            reverse = true;
        int inv = reverse ? -1 : 1;
        ui->XY_radioButton->setChecked(true);
        ui->spinBoxX->setValue(pos.x);
        ui->spinBoxY->setValue(pos.y * inv);
        ui->spinBoxZ->setValue(pos.z * inv);
        ui->spinBoxRotation->setValue(yaw * inv);
    }
    else if (fabs(roll - 90.) < tol && (fabs(yaw) < tol || fabs(yaw - 180.) < tol)) {
        if (fabs(yaw - 180.) < tol)
            reverse = true;
        int inv = reverse ? -1 : 1;
        ui->XZ_radioButton->setChecked(true);
        ui->spinBoxX->setValue(inv * pos.x);
        ui->spinBoxY->setValue(pos.z);
        ui->spinBoxZ->setValue(inv * pos.y);
        ui->spinBoxRotation->setValue(- pitch);
    }
    else if (fabs(roll - 90.) < tol && (fabs(yaw - 90.) < tol || fabs(yaw + 90.) < tol)) {
        if (fabs(yaw + 90.) < tol)
            reverse = true;
        int inv = reverse ? -1 : 1;
        ui->YZ_radioButton->setChecked(true);
        ui->spinBoxX->setValue(inv * pos.y);
        ui->spinBoxY->setValue(pos.z);
        ui->spinBoxZ->setValue(inv * pos.x);
        ui->spinBoxRotation->setValue(-pitch);
    }

    ui->Reverse_checkBox->setChecked(reverse);

    onPreview();
}

void TaskImage::updatePlacement()
{
    Base::Placement Pos;
    double offsetX = ui->spinBoxX->value().getValue();
    double offsetY = ui->spinBoxY->value().getValue();
    double offsetZ = ui->spinBoxZ->value().getValue();
    double angle = ui->spinBoxRotation->value().getValue();
    bool reverse = ui->Reverse_checkBox->isChecked();

    Base::Rotation rot;
    double dir = reverse ? 180. : 0.;
    int inv = reverse ? -1 : 1;

    if (ui->XY_radioButton->isChecked()) {
        rot.setYawPitchRoll(inv * angle, 0., dir);
        Pos = Base::Placement(Base::Vector3d(offsetX, inv * offsetY, inv * offsetZ), rot);
    }
    else if (ui->XZ_radioButton->isChecked()) {
        rot.setYawPitchRoll(dir, -angle, 90.);
        Pos = Base::Placement(Base::Vector3d(inv * offsetX, inv * offsetZ, offsetY), rot);
    }
    else if (ui->YZ_radioButton->isChecked()) {
        rot.setYawPitchRoll(90. - dir, -angle, 90.);
        Pos = Base::Placement(Base::Vector3d(inv * offsetZ, inv * offsetX, offsetY), rot);
    }

    if (!feature.expired()) {
        feature->Placement.setValue(Pos);
        if(scale)
            scale->setPlacement(Pos);
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


struct NodeData {
    InteractiveScale* scale;
};

InteractiveScale::InteractiveScale(View3DInventorViewer* view, ViewProvider* vp, Base::Placement plc)
    : active(false)
    , allowOutsideImage(false)
    , placement(plc)
    , viewer(view)
    , viewProv(vp)
    , midPoint(SbVec3f(0,0,0))
{
    root = new SoAnnotation;
    root->ref();
    root->renderCaching = SoSeparator::OFF;

    measureLabel = new SoDatumLabel();
    measureLabel->ref();
    measureLabel->string = "";
    measureLabel->textColor = SbColor(1.0f, 0.149f, 0.0f);
    measureLabel->size.setValue(17);
    measureLabel->lineWidth = 2.0;
    measureLabel->useAntialiasing = false;
    measureLabel->param1 = 0.;
    measureLabel->param2 = 0.;

    transform = new SoTransform();
    root->addChild(transform);
    setPlacement(placement);

    Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    distanceBox = new QuantitySpinBox(mdi);
    distanceBox->setUnit(Base::Unit::Length);
    distanceBox->setMinimum(0.0);
    distanceBox->setMaximum(INT_MAX);
    distanceBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    distanceBox->setToolTip(tr("Enter desired distance between the points"));
    distanceBox->setKeyboardTracking(false);
    distanceBox->installEventFilter(this);

    //track camera movements to update spinbox position.
    NodeData* info = new NodeData{ this };
    cameraSensor = new SoNodeSensor([](void* data, SoSensor* sensor) {
        NodeData* info = static_cast<NodeData*>(data);
        info->scale->positionWidget();
    }, info);
    cameraSensor->attach(viewer->getCamera());
}

InteractiveScale::~InteractiveScale()
{
    root->unref();
    measureLabel->unref();
    distanceBox->deleteLater();
    cameraSensor->detach();
}

void InteractiveScale::activate(bool allowOutside)
{
    if (viewer) {
        static_cast<SoSeparator*>(viewer->getSceneGraph())->addChild(root);
        viewer->setEditing(true);
        viewer->addEventCallback(SoLocation2Event::getClassTypeId(), InteractiveScale::getMousePosition, this);
        viewer->addEventCallback(SoButtonEvent::getClassTypeId(), InteractiveScale::soEventFilter, this);
        viewer->setSelectionEnabled(false);
        viewer->getWidget()->setCursor(QCursor(Qt::CrossCursor));
        active = true;
        allowOutsideImage = allowOutside;
    }
}

void InteractiveScale::deactivate()
{
    if (viewer) {
        distanceBox->hide();
        points.clear();
        root->removeChild(measureLabel);
        static_cast<SoSeparator*>(viewer->getSceneGraph())->removeChild(root);
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
    if ((points[0] - points[1]).length() == 0.)
        return 1.0;

    return distanceBox->value().getValue() / (points[0] - points[1]).length();
}

double InteractiveScale::getDistance(const SbVec3f& pt) const
{
    if (points.empty())
        return 0.0;

    return (points[0] - pt).length();
}

void InteractiveScale::findPointOnPlane(SoEventCallback * ecb)
{
    if (allowOutsideImage) {
        findPointOnFocalPlane(ecb);
    }
    else {
        findPointOnImagePlane(ecb);
    }
}

void InteractiveScale::findPointOnImagePlane(SoEventCallback * ecb)
{
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent *>(ecb->getEvent());
    Gui::View3DInventorViewer* view  = static_cast<Gui::View3DInventorViewer*>(ecb->getUserData());
    std::unique_ptr<SoPickedPoint> pp(view->getPointOnRay(mbe->getPosition(), viewProv));
    if (pp.get()) {
        auto pos3d = pp->getPoint();

        collectPoint(pos3d);
    }
}

void InteractiveScale::findPointOnFocalPlane(SoEventCallback * ecb)
{
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent *>(ecb->getEvent());
    Gui::View3DInventorViewer* view  = static_cast<Gui::View3DInventorViewer*>(ecb->getUserData());

    auto pos2d = mbe->getPosition();
    auto pos3d = view->getPointOnFocalPlane(pos2d);

    collectPoint(pos3d);
}

void InteractiveScale::collectPoint(const SbVec3f& pos3d)
{
    if (points.empty()) {
        points.push_back(pos3d);

        measureLabel->setPoints(getCoordsOnImagePlane(pos3d), getCoordsOnImagePlane(pos3d));
        root->addChild(measureLabel);
    }
    else if (points.size() == 1) {
        double distance = getDistance(pos3d);
        if (distance > Base::Precision::Confusion()) {
            points.push_back(pos3d);

            midPoint = points[0] + (points[1] - points[0]) / 2;

            measureLabel->string = "";
            distanceBox->show();
            QSignalBlocker block(distanceBox);
            distanceBox->setValue((points[1] - points[0]).length());
            distanceBox->adjustSize();
            positionWidget();
            distanceBox->selectNumber();
            distanceBox->setFocus();

            Q_EMIT enableApplyBtn();
        }
        else {
            Base::Console().Warning(std::string("Image scale"), "The second point is too close. Retry!\n");
        }
    }
}

void InteractiveScale::positionWidget()
{
    QSize wSize = distanceBox->size();
    QPoint pxCoord = viewer->toQPoint(viewer->getPointOnViewport(midPoint));
    pxCoord.setX(std::max(pxCoord.x() - wSize.width() / 2, 0));
    pxCoord.setY(std::max(pxCoord.y() - wSize.height() / 2, 0));
    distanceBox->move(pxCoord);
}

void InteractiveScale::getMousePosition(void * ud, SoEventCallback * ecb)
{
    InteractiveScale* scale = static_cast<InteractiveScale*>(ud);
    const SoLocation2Event * l2e = static_cast<const SoLocation2Event *>(ecb->getEvent());
    Gui::View3DInventorViewer* view  = static_cast<Gui::View3DInventorViewer*>(ecb->getUserData());

    if (scale->points.size() == 1) {
        ecb->setHandled();
        SbVec3f pos3d;
        if (scale->allowOutsideImage) {
            auto pos2d = l2e->getPosition();
            pos3d = view->getPointOnFocalPlane(pos2d);
        }
        else {
            std::unique_ptr<SoPickedPoint> pp(view->getPointOnRay(l2e->getPosition(), scale->viewProv));
            if (pp.get()) {
                pos3d = pp->getPoint();
            }
            else {
                return;
            }
        }

        Base::Quantity quantity;
        quantity.setValue((pos3d - scale->points[0]).length());
        quantity.setUnit(Base::Unit::Length);

        //Update the displayed distance
        double factor;
        QString unitStr, valueStr;
        valueStr = quantity.getUserString(factor, unitStr);
        scale->measureLabel->string = SbString(valueStr.toUtf8().constData());

        scale->measureLabel->setPoints(scale->getCoordsOnImagePlane(scale->points[0]), scale->getCoordsOnImagePlane(pos3d));
    }
}

void InteractiveScale::soEventFilter(void* ud, SoEventCallback* ecb)
{
    InteractiveScale* scale = static_cast<InteractiveScale*>(ud);

    const SoEvent* soEvent = ecb->getEvent();
    if (soEvent->isOfType(SoKeyboardEvent::getClassTypeId())) {
        /* If user press escape, then we cancel the tool.*/
        const auto kbe = static_cast<const SoKeyboardEvent*>(soEvent);

        if (kbe->getKey() == SoKeyboardEvent::ESCAPE && kbe->getState() == SoButtonEvent::UP) {
            ecb->setHandled();
            ecb->getAction()->setHandled();
            Q_EMIT scale->scaleCanceled();
        }
    }
    else if (soEvent->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        const auto mbe = static_cast<const SoMouseButtonEvent*>(soEvent);

        if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN)
        {
            ecb->setHandled();
            scale->findPointOnPlane(ecb);
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
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        /* If user press enter in the spinbox, then we validate the tool.*/
        if (keyEvent->key() == Qt::Key_Enter && dynamic_cast<QuantitySpinBox*>(object)) {
            Q_EMIT scaleRequired();
        }

        /* If user press escape, then we cancel the tool. Required here as well for when checkbox has focus.*/
        if (keyEvent->key() == Qt::Key_Escape) {
            Q_EMIT scaleCanceled();
        }
    }
    return false;
}

void InteractiveScale::setPlacement(Base::Placement plc)
{
    placement = plc;
    double x, y, z, w;
    placement.getRotation().getValue(x, y, z, w);
    transform->rotation.setValue(x, y, z, w);
    Base::Vector3d pos = placement.getPosition();
    transform->translation.setValue(pos.x, pos.y, pos.z);

    Base::Vector3d RN(0, 0, 1);
    RN = placement.getRotation().multVec(RN);
    measureLabel->norm.setValue(SbVec3f(RN.x, RN.y, RN.z));
}

SbVec3f InteractiveScale::getCoordsOnImagePlane(const SbVec3f& point)
{
    // Plane form
    Base::Vector3d R0(0, 0, 0), RX(1, 0, 0), RY(0, 1, 0);

    // move to position of Sketch
    Base::Rotation tmp(placement.getRotation());
    RX = tmp.multVec(RX);
    RY = tmp.multVec(RY);

    Base::Vector3d S(point[0], point[1], point[2]);
    S.TransformToCoordinateSystem(R0, RX, RY);

    return SbVec3f(S.x, S.y, 0.);
}

// ----------------------------------------------------------------------------

TaskImageDialog::TaskImageDialog(Image::ImagePlane* obj)
{
    widget = new TaskImage(obj);
    Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
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
