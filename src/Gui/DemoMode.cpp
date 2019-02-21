/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <cmath>
# include <float.h>
# include <climits>
# include <QCursor>
# include <QTimer>
# include <Inventor/nodes/SoCamera.h>
#endif

#include "DemoMode.h"
#include "ui_DemoMode.h"

#include "Application.h"
#include "Document.h"
#include "MainWindow.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include <Base/Tools.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DemoMode */

DemoMode::DemoMode(QWidget* /*parent*/, Qt::WindowFlags fl)
  : QDialog(0, fl|Qt::WindowStaysOnTopHint), viewAxis(0,0,-1), ui(new Ui_DemoMode)
{
    // create widgets
    ui->setupUi(this);
    timer = new QTimer(this);
    timer->setInterval(1000 * ui->timeout->value());
    connect(timer, SIGNAL(timeout()), this, SLOT(onAutoPlay()));
    oldvalue = ui->angleSlider->value();

    wasHidden = false;
    showHideTimer = new QTimer(this);
    showHideTimer->setInterval(5000);
    connect(showHideTimer, SIGNAL(timeout()), this, SLOT(hide()));
}

/** Destroys the object and frees any allocated resources */
DemoMode::~DemoMode()
{
    delete ui;
}

void DemoMode::reset()
{
    on_fullscreen_toggled(false);
    on_stopButton_clicked();
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    hGrp->Notify("UseAutoRotation");
}

void DemoMode::accept()
{
    reset();
    QDialog::accept();
}

void DemoMode::reject()
{
    reset();
    QDialog::reject();
}

bool DemoMode::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        if (ui->fullscreen->isChecked()) {
            QPoint point = QCursor::pos() - oldPos;
            if (point.manhattanLength() > 5) {
                show();
                showHideTimer->start();
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void DemoMode::showEvent(QShowEvent *)
{
    if (this->wasHidden)
        this->move(this->pnt);
    this->wasHidden = false;
}

void DemoMode::hideEvent(QHideEvent *)
{
    this->pnt = this->pos();
    this->wasHidden = true;
    this->oldPos = QCursor::pos();
    showHideTimer->stop();
}

Gui::View3DInventor* DemoMode::activeView() const
{
    Document* doc = Application::Instance->activeDocument();
    if (doc) {
        MDIView* view = doc->getActiveView();
        if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
            return static_cast<Gui::View3DInventor*>(view);
        }
    }

    return 0;
}

float DemoMode::getSpeed(int v) const
{
    float speed = ((float)v)/50.0f; // let 2.0 be the maximum speed
    return speed;
}

SbVec3f DemoMode::getDirection(Gui::View3DInventor* view) const
{
    SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
    if (!cam) return this->viewAxis;
    SbRotation rot = cam->orientation.getValue();
    SbRotation inv = rot.inverse();
    SbVec3f vec(this->viewAxis);
    inv.multVec(vec, vec);
    if (vec.length() < FLT_EPSILON)
        vec = this->viewAxis;
    vec.normalize();
    return vec;
}

void DemoMode::on_angleSlider_valueChanged(int v)
{
    Gui::View3DInventor* view = activeView();
    if (view) {
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        if (!cam) return;
        float angle = Base::toRadians<float>(/*90-v*/v-this->oldvalue);
        SbRotation rot(SbVec3f(-1,0,0),angle);
        reorientCamera(cam ,rot);
        this->oldvalue = v;
        if (view->getViewer()->isAnimating()) {
            startAnimation(view);
        }
    }
}

void DemoMode::reorientCamera(SoCamera * cam, const SbRotation & rot)
{
    // Find global coordinates of focal point.
    SbVec3f direction;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    SbVec3f focalpoint = cam->position.getValue() +
                         cam->focalDistance.getValue() * direction;

    // Set new orientation value by accumulating the new rotation.
    cam->orientation = rot * cam->orientation.getValue();

    // Reposition camera so we are still pointing at the same old focal point.
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    cam->position = focalpoint - cam->focalDistance.getValue() * direction;
}

void DemoMode::on_speedSlider_valueChanged(int v)
{
    Q_UNUSED(v); 
    Gui::View3DInventor* view = activeView();
    if (view && view->getViewer()->isAnimating()) {
        startAnimation(view);
    }
}

void DemoMode::on_playButton_clicked()
{
    Gui::View3DInventor* view = activeView();
    if (view) {
        if (!view->getViewer()->isAnimating()) {
            SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
            if (cam) {
                SbRotation rot = cam->orientation.getValue();
                SbVec3f vec(0,-1,0);
                rot.multVec(vec, this->viewAxis);
            }
        }

        startAnimation(view);
    }
}

void DemoMode::on_stopButton_clicked()
{
    Gui::View3DInventor* view = activeView();
    if (view)
        view->getViewer()->stopAnimating();
}

void DemoMode::on_fullscreen_toggled(bool on)
{
    Gui::View3DInventor* view = activeView();
    if (view) {
        view->setCurrentViewMode(on ? MDIView::/*TopLevel*/FullScreen : MDIView::Child);
        this->activateWindow();
    }
    if (on) {
        qApp->installEventFilter(this);
        showHideTimer->start();
    }
    else {
        qApp->removeEventFilter(this);
        showHideTimer->stop();
    }
}

void DemoMode::on_timeout_valueChanged(int v)
{
    timer->setInterval(v*1000);
}

void DemoMode::onAutoPlay()
{
    Gui::View3DInventor* view = activeView();
    if (view && !view->getViewer()->isAnimating()) {
        startAnimation(view);
    }
}

void DemoMode::startAnimation(Gui::View3DInventor* view)
{
    if (!view->getViewer()->isAnimationEnabled())
        view->getViewer()->setAnimationEnabled(true);
    view->getViewer()->startAnimating(getDirection(view), 
        getSpeed(ui->speedSlider->value()));
}

void DemoMode::on_timerCheck_toggled(bool on)
{
    if (on)
        timer->start();
    else
        timer->stop();
}

void DemoMode::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QDialog::changeEvent(e);
}

#include "moc_DemoMode.cpp"
