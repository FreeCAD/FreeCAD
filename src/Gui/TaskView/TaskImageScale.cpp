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
# include <map>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoAnnotation.h>
#endif

#include <Base/Console.h>
#include <Base/Precision.h>
#include <Base/Tools.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Camera.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/TaskView/TaskView.h>

#include "TaskImageScale.h"
#include "ui_TaskImageScale.h"


using namespace Gui;

/* TRANSLATOR Gui::TaskImageScale */

TaskImageScale::TaskImageScale(Image::ImagePlane* obj, QWidget* parent)
  : QWidget(parent)
  , ui(new Ui_TaskImageScale)
  , feature(obj)
  , aspectRatio{1.0}
{
    ui->setupUi(this);
    ui->pushButtonCancel->hide();
    ui->spinBoxWidth->setValue(obj->getXSizeInPixel());
    ui->spinBoxHeight->setValue(obj->getYSizeInPixel());

    aspectRatio = obj->XSize.getValue() / obj->YSize.getValue();

    connect(ui->spinBoxWidth, qOverload<int>(&QSpinBox::valueChanged),
            this, &TaskImageScale::changeWidth);
    connect(ui->spinBoxHeight, qOverload<int>(&QSpinBox::valueChanged),
            this, &TaskImageScale::changeHeight);
    connect(ui->pushButtonScale, &QPushButton::clicked,
            this, &TaskImageScale::onInteractiveScale);
    connect(ui->pushButtonCancel, &QPushButton::clicked,
            this, &TaskImageScale::rejectScale);
}

TaskImageScale::~TaskImageScale()
{
    if (scale) {
        if (scale->isActive()) {
            scale->deactivate();
        }
        scale->deleteLater();
    }
}

void TaskImageScale::changeWidth()
{
    if (!feature.expired()) {
        int value = ui->spinBoxWidth->value();
        feature->setXSizeInPixel(value);

        if (ui->checkBoxRatio->isChecked()) {
            QSignalBlocker block(ui->spinBoxWidth);
            ui->spinBoxHeight->setValue(int(double(value) / aspectRatio));
        }
    }
}

void TaskImageScale::changeHeight()
{
    if (!feature.expired()) {
        int value = ui->spinBoxHeight->value();
        feature->setYSizeInPixel(value);

        if (ui->checkBoxRatio->isChecked()) {
            QSignalBlocker block(ui->spinBoxHeight);
            ui->spinBoxWidth->setValue(int(double(value) * aspectRatio));
        }
    }
}

View3DInventorViewer* TaskImageScale::getViewer() const
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

void TaskImageScale::selectedPoints(size_t num)
{
    if (num == 1) {
        ui->labelInstruction->setText(tr("Select second point"));
    }
    else if (num == 2) {
        ui->labelInstruction->setText(tr("Enter desired distance between the points"));
        ui->pushButtonScale->setEnabled(true);
        ui->quantitySpinBox->setEnabled(true);
        ui->quantitySpinBox->setValue(scale->getDistance());
    }
}

void TaskImageScale::scaleImage(double factor)
{
    if (!feature.expired()) {
        feature->XSize.setValue(feature->XSize.getValue() * factor);
        feature->YSize.setValue(feature->YSize.getValue() * factor);

        QSignalBlocker blockW(ui->spinBoxWidth);
        ui->spinBoxWidth->setValue(feature->getXSizeInPixel());
        QSignalBlocker blockH(ui->spinBoxHeight);
        ui->spinBoxHeight->setValue(feature->getYSizeInPixel());
    }
}

void TaskImageScale::startScale()
{
    scale->activate(ui->checkBoxOutside->isChecked());
    if (ui->checkBoxOutside->isChecked()) {
        ui->labelInstruction->setText(tr("Select two points in the 3d view"));
    }
    else {
        ui->labelInstruction->setText(tr("Select two points on the image"));
    }
    ui->checkBoxOutside->setEnabled(false);
    ui->pushButtonScale->setEnabled(false);
    ui->pushButtonScale->setText(tr("Accept"));
    ui->pushButtonCancel->show();
    ui->quantitySpinBox->setEnabled(false);
}

void TaskImageScale::acceptScale()
{
    scaleImage(ui->quantitySpinBox->value().getValue() / scale->getDistance());
    rejectScale();
}

void TaskImageScale::rejectScale()
{
    scale->deactivate();
    ui->labelInstruction->clear();
    ui->pushButtonScale->setEnabled(true);
    ui->pushButtonScale->setText(tr("Interactive"));
    ui->pushButtonCancel->hide();
    ui->quantitySpinBox->setEnabled(false);
    ui->checkBoxOutside->setEnabled(true);

    scale->clearPoints();
}

void TaskImageScale::onInteractiveScale()
{
    if (!feature.expired() && !scale) {
        View3DInventorViewer* viewer = getViewer();
        if (viewer) {
            auto vp = Application::Instance->getViewProvider(feature.get());
            scale = new InteractiveScale(viewer, vp);
            connect(scale, &InteractiveScale::selectedPoints,
                    this, &TaskImageScale::selectedPoints);
        }
    }

    if (scale) {
        if (scale->isActive()) {
            acceptScale();
        }
        else {
            startScale();
        }
    }
}

// ----------------------------------------------------------------------------

InteractiveScale::InteractiveScale(View3DInventorViewer* view, ViewProvider* vp)
    : active{false}
    , allowOutsideImage{false}
    , viewer{view}
    , viewProv{vp}
{
    coords = new SoCoordinate3;
    coords->ref();
    root = new SoAnnotation;
    root->ref();

    root->addChild(coords);

    SoBaseColor* color = new SoBaseColor;
    color->rgb.setValue(1.0F, 0.0F, 0.0F);
    root->addChild(color);
    root->addChild(new SoLineSet);
}

InteractiveScale::~InteractiveScale()
{
    coords->unref();
    root->unref();
}

void InteractiveScale::activate(bool allowOutside)
{
    if (viewer) {
        static_cast<SoSeparator*>(viewer->getSceneGraph())->addChild(root);
        viewer->setEditing(true);
        viewer->addEventCallback(SoLocation2Event::getClassTypeId(), InteractiveScale::getMousePosition, this);
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), InteractiveScale::getMouseClick, this);
        viewer->setSelectionEnabled(false);
        viewer->getWidget()->setCursor(QCursor(Qt::CrossCursor));
        active = true;
        allowOutsideImage = allowOutside;
    }
}

void InteractiveScale::deactivate()
{
    if (viewer) {
        static_cast<SoSeparator*>(viewer->getSceneGraph())->removeChild(root);
        viewer->setEditing(false);
        viewer->removeEventCallback(SoLocation2Event::getClassTypeId(), InteractiveScale::getMousePosition, this);
        viewer->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), InteractiveScale::getMouseClick, this);
        viewer->setSelectionEnabled(true);
        viewer->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
        active = false;
    }
}

double InteractiveScale::getDistance() const
{
    if (points.size() < 2)
        return 0.0;

    return (points[0] - points[1]).length();
}

double InteractiveScale::getDistance(const SbVec3f& pt) const
{
    if (points.empty())
        return 0.0;

    return (points[0] - pt).length();
}

void InteractiveScale::clearPoints()
{
    points.clear();
    coords->point.setNum(0);
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
        coords->point.set1Value(0, pos3d);
    }
    else if (points.size() == 1) {
        double distance = getDistance(pos3d);
        if (distance > Base::Precision::Confusion()) {
            points.push_back(pos3d);
            coords->point.set1Value(1, pos3d);
        }
        else {
            Base::Console().Warning(std::string("Image scale"), "The second point is too close. Retry!\n");
        }
    }

    Q_EMIT selectedPoints(points.size());
}

void InteractiveScale::getMouseClick(void * ud, SoEventCallback * ecb)
{
    InteractiveScale* scale = static_cast<InteractiveScale*>(ud);
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent *>(ecb->getEvent());

    if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
        ecb->setHandled();
        scale->findPointOnPlane(ecb);
    }
}

void InteractiveScale::getMousePosition(void * ud, SoEventCallback * ecb)
{
    InteractiveScale* scale = static_cast<InteractiveScale*>(ud);
    const SoLocation2Event * l2e = static_cast<const SoLocation2Event *>(ecb->getEvent());
    Gui::View3DInventorViewer* view  = static_cast<Gui::View3DInventorViewer*>(ecb->getUserData());

    if (scale->points.size() == 1) {
        ecb->setHandled();
        auto pos2d = l2e->getPosition();
        auto pos3d = view->getPointOnFocalPlane(pos2d);
        scale->coords->point.set1Value(1, pos3d);
    }
}

#include "moc_TaskImageScale.cpp"
