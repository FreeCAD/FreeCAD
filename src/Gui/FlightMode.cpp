/***************************************************************************
 *   Copyright (c) 2025 Adrian Przekwas <adrian.v.przekwas[at]gmail.com>   *
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
#include <QtGlobal>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QCursor>
#include <QScreen>
#include <QDockWidget>
#include <Inventor/nodes/SoCamera.h>
#endif

#include "FlightMode.h"
#include "ui_FlightMode.h"
#include "Application.h"
#include "Document.h"
#include "Utilities.h"
#include "DockWindowManager.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

#include <Base/Console.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::FlightMode */

FlightMode::FlightMode(QWidget* /*parent*/, Qt::WindowFlags fl)
    : QDialog(nullptr, fl | Qt::WindowStaysOnTopHint)
    , ui(new Ui_FlightMode)
{
    // Create widgets
    ui->setupUi(this);
    setupConnections();
    ui->lockMouseButton->setCheckable(true);
    ui->prefsGroupBox->setVisible(false);
    setAutoFillBackground(true);

    populateKeyBindings();
    // Grab keyboard and mouse
    QCoreApplication::instance()->installEventFilter(this);

    // Get initial cursor position to avoid view rotation
    oldPos = QCursor::pos();

    qreal rRate = QGuiApplication::primaryScreen()->refreshRate();
    // Movement velocity should be independent form refresh rate
    rRateMod = 60.0f / rRate;
    timer = new QTimer(this);
    timer->setInterval(1000.f / rRate);
    connect(timer, &QTimer::timeout, this, &FlightMode::updatePosition);
    timer->start();
    Base::Console().Message(QT_TR_NOOP("Free Flight mode initialised\n"));

    // Some platforms prevent mouse cursor locking/forced movement
    lockable = checkCursorLocking();
    if (!lockable) {
        Base::Console().Warning(QT_TR_NOOP("This platform does not allow force cursor move or "
                                           "lock! Mouse lock button will be disabled. \n"));
    }
    ui->lockMouseButton->setEnabled(lockable);

    connect(qApp, &QApplication::aboutToQuit, this, &FlightMode::saveSettings);
}

/** Destroys the object and frees any allocated resources */
FlightMode::~FlightMode()
{
    timer->stop();
    delete ui;
}

void FlightMode::setupConnections()
{
    connect(ui->lockMouseButton,
            &QPushButton::clicked,
            this,
            &FlightMode::onLockMouseButtonToggled);
    connect(ui->exitButton, &QPushButton::clicked, this, &FlightMode::onExitButtonClicked);
    connect(ui->mLookSensSlider,
            &QSlider::valueChanged,
            this,
            &FlightMode::onMLookSensSliderValueChanged);
    connect(ui->linVelSlider,
            &QSlider::valueChanged,
            this,
            &FlightMode::onLinVelSliderValueChanged);
    connect(ui->angVelSlider,
            &QSlider::valueChanged,
            this,
            &FlightMode::onAngVelSliderValueChanged);
}

void ::FlightMode::setDefaultInfoText()
{
    ui->infoLabel->setText(
        QObject::tr("You are in the Free Flight Mode now. Press Esc to toggle preferences"));
}

void ::FlightMode::setUnlockInfoText()
{
    ui->infoLabel->setText(
        QObject::tr("Mouse cursor is locked. Press Esc to unlock the cursor"));
}

Gui::View3DInventor* FlightMode::activeView() const
{
    Document* doc = Application::Instance->activeDocument();
    if (doc) {
        MDIView* view = doc->getActiveView();
        if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
            return static_cast<Gui::View3DInventor*>(view);
        }
    }
    return nullptr;
}

void FlightMode::updatePosition()
{

    Gui::View3DInventor* view = activeView();
    if (view) {
        SoCamera* cam = view->getViewer()->getSoRenderManager()->getCamera();
        if (!cam) {
            return;
        }
        SbRotation rotY = SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), movMod.yaw * angVel * rRateMod);
        SbRotation rotP = SbRotation(SbVec3f(1.0f, 0.0f, 0.0f), movMod.pitch * angVel * rRateMod);
        SbRotation rotR = SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), movMod.roll * angVel * rRateMod);
        SbRotation rot = rotY * rotP * rotR;
        SbVec3f pos = SbVec3f(movMod.sidestep, movMod.altitude, movMod.walk) * linVel * rRateMod;
        moveCamera(cam, pos, rot);
        // we assume that view only rotates if mouse is moving or a button is pressed
        wasRotRead = true;
        movMod.pitch = 0.0f;
        movMod.yaw = 0.0f;
        movMod.roll = 0.0f;
    }
}

void FlightMode::moveCamera(SoCamera* cam, const SbVec3f& pos, const SbRotation& rot)
{
    // Do not update camera if input is zero
    float eps = 1.0E-15;
    if (rot.equals(SbRotation(SbVec3f(0.0f, 0.0f, 0.0f), 0.0f), eps)
        && pos.equals(SbVec3f(0.0f, 0.0f, 0.0f), eps)) {
        return;
    }
    SbRotation crot = cam->orientation.getValue();
    Base::Rotation camRot = Base::convertTo<Base::Rotation>(crot);
    double yc, pc, rc;
    camRot.getYawPitchRoll(yc, pc, rc);

    Base::Rotation inputRot = Base::convertTo<Base::Rotation>(rot);
    double ym, pm, rm;
    inputRot.getYawPitchRoll(ym, pm, rm);

    camRot.setYawPitchRoll(yc + ym, pc + pm, rc + rm);
    cam->orientation.setValue(Base::convertTo<SbRotation>(camRot));

    SbVec3f transl;
    cam->orientation.getValue().multVec(pos.getValue(), transl);
    cam->position = transl + cam->position.getValue();
}

void FlightMode::saveSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/FlightMode");
    hGrp->SetInt("ForwardKey", kb.forward.key);
    hGrp->SetInt("BackwardKey", kb.backward.key);
    hGrp->SetInt("StepLeftKey", kb.stepLeft.key);
    hGrp->SetInt("StepRightKey", kb.stepRight.key);
    hGrp->SetInt("UpKey", kb.up.key);
    hGrp->SetInt("DownKey", kb.down.key);
    hGrp->SetInt("RollCCWKey", kb.rollCCW.key);
    hGrp->SetInt("RollCWKey", kb.rollCW.key);
    hGrp->SetInt("YawLeftKey", kb.yawLeft.key);
    hGrp->SetInt("YawRightKey", kb.yawRight.key);
    hGrp->SetInt("PitchUpKey", kb.pitchUp.key);
    hGrp->SetInt("PitchDownKey", kb.pitchDown.key);

    hGrp->SetInt("AngularVelocity", ui->angVelSlider->value());
    hGrp->SetInt("LinearVelocity", ui->linVelSlider->value());
    hGrp->SetInt("MouseLookSensitivity", ui->mLookSensSlider->value());
}

void FlightMode::populateKeyBindings()
{

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/FlightMode");

    ui->angVelSlider->setValue(hGrp->GetInt("AngularVelocity", 50));
    ui->linVelSlider->setValue(hGrp->GetInt("LinearVelocity", 50));
    ui->mLookSensSlider->setValue(hGrp->GetInt("MouseLookSensitivity", 50));

    kb.forward.text = QObject::tr("Forward: ");
    kb.forward.lineEdit = ui->forwardLineEdit;
    kb.forward.setKey(hGrp->GetInt("ForwardKey", Qt::Key_I));

    kb.backward.text = QObject::tr("Backward: ");
    kb.backward.lineEdit = ui->backwardLineEdit;
    kb.backward.setKey(hGrp->GetInt("BackwardKey", Qt::Key_K));

    kb.stepLeft.text = QObject::tr("Step Left: ");
    kb.stepLeft.lineEdit = ui->stepLeftLineEdit;
    kb.stepLeft.setKey(hGrp->GetInt("StepLeftKey", Qt::Key_J));

    kb.stepRight.text = QObject::tr("Step Right: ");
    kb.stepRight.lineEdit = ui->stepRightLineEdit;
    kb.stepRight.setKey(hGrp->GetInt("StepRightKey", Qt::Key_L));

    kb.up.text = QObject::tr("Up: ");
    kb.up.lineEdit = ui->upLineEdit;
    kb.up.setKey(hGrp->GetInt("UpKey", Qt::Key_Space));

    kb.down.text = QObject::tr("Down: ");
    kb.down.lineEdit = ui->downLineEdit;
    kb.down.setKey(hGrp->GetInt("DownKey", Qt::Key_Control));

    kb.rollCCW.text = QObject::tr("Roll CCW: ");
    kb.rollCCW.lineEdit = ui->rollCCWLineEdit;
    kb.rollCCW.setKey(hGrp->GetInt("RollCCWKey", Qt::Key_U));

    kb.rollCW.text = QObject::tr("Roll CW: ");
    kb.rollCW.lineEdit = ui->rollCWLineEdit;
    kb.rollCW.setKey(hGrp->GetInt("RollCWKey", Qt::Key_O));

    kb.yawLeft.text = QObject::tr("Yaw Left: ");
    kb.yawLeft.lineEdit = ui->yawLeftLineEdit;
    kb.yawLeft.setKey(hGrp->GetInt("YawLeftKey", Qt::Key_Left));

    kb.yawRight.text = QObject::tr("Yaw Right: ");
    kb.yawRight.lineEdit = ui->yawRightLineEdit;
    kb.yawRight.setKey(hGrp->GetInt("YawRightKey", Qt::Key_Right));

    kb.pitchUp.text = QObject::tr("Pitch Up: ");
    kb.pitchUp.lineEdit = ui->pitchUpLineEdit;
    kb.pitchUp.setKey(hGrp->GetInt("PitchUpKey", Qt::Key_Up));

    kb.pitchDown.text = QObject::tr("Pitch Down: ");
    kb.pitchDown.lineEdit = ui->pitchDownLineEdit;
    kb.pitchDown.setKey(hGrp->GetInt("PitchDownKey", Qt::Key_Down));
}

bool FlightMode::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        auto key = (keyEvent->key());
        if (ui->forwardLineEdit->hasFocus()) {
            updateKeyEdit(kb.forward, key);
            return true;
        }
        if (ui->backwardLineEdit->hasFocus()) {
            updateKeyEdit(kb.backward, key);
            return true;
        }
        if (ui->stepLeftLineEdit->hasFocus()) {
            updateKeyEdit(kb.stepLeft, key);
            return true;
        }
        if (ui->stepRightLineEdit->hasFocus()) {
            updateKeyEdit(kb.stepRight, key);
            return true;
        }
        if (ui->upLineEdit->hasFocus()) {
            updateKeyEdit(kb.up, key);
            return true;
        }
        if (ui->downLineEdit->hasFocus()) {
            updateKeyEdit(kb.down, key);
            return true;
        }
        if (ui->rollCCWLineEdit->hasFocus()) {
            updateKeyEdit(kb.rollCCW, key);
            return true;
        }
        if (ui->rollCWLineEdit->hasFocus()) {
            updateKeyEdit(kb.rollCW, key);
            return true;
        }
        if (ui->yawLeftLineEdit->hasFocus()) {
            updateKeyEdit(kb.yawLeft, key);
            return true;
        }
        if (ui->yawRightLineEdit->hasFocus()) {
            updateKeyEdit(kb.yawRight, key);
            return true;
        }
        if (ui->pitchUpLineEdit->hasFocus()) {
            updateKeyEdit(kb.pitchUp, key);
            return true;
        }
        if (ui->pitchDownLineEdit->hasFocus()) {
            updateKeyEdit(kb.pitchDown, key);
            return true;
        }
        if (key == kb.forward.key) {
            movMod.walk = -1.0f;
            return true;
        }
        else if (key == kb.backward.key) {
            movMod.walk = 1.0f;
            return true;
        }
        else if (key == kb.stepLeft.key) {
            movMod.sidestep = -1.0f;
            return true;
        }
        else if (key == kb.stepRight.key) {
            movMod.sidestep = 1.0f;
            return true;
        }
        else if (key == kb.up.key) {
            movMod.altitude = 1.0f;
            return true;
        }
        else if (key == kb.down.key) {
            movMod.altitude = -1.0f;
            return true;
        }
        else if (key == kb.rollCCW.key) {
            movMod.roll = -1.0f;
            return true;
        }
        else if (key == kb.rollCW.key) {
            movMod.roll = 1.0f;
            return true;
        }
        else if (key == kb.yawLeft.key) {
            movMod.yaw = 1.0f;
            return true;
        }
        else if (key == kb.yawRight.key) {
            movMod.yaw = -1.0f;
            return true;
        }
        else if (key == kb.pitchUp.key) {
            movMod.pitch = 1.0f;
            return true;
        }
        else if (key == kb.pitchDown.key) {
            movMod.pitch = -1.0f;
            return true;
        }
        else if (key == Qt::Key_Escape) {
            if (lockMouse) {
                setDefaultInfoText();
                ui->lockMouseButton->setChecked(false);
                lockMouse = false;
                QApplication::restoreOverrideCursor();
            }
            else {
                togglePrefs();
            }
            return true;
        }
    }
    else if (event->type() == QEvent::KeyRelease) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        auto key = keyEvent->key();
        if (key == kb.forward.key || key == kb.backward.key) {
            movMod.walk = 0.0f;
            return true;
        }
        else if (key == kb.stepLeft.key || key == kb.stepRight.key) {
            movMod.sidestep = 0.0f;
            return true;
        }
        else if (key == kb.up.key || key == kb.down.key) {
            movMod.altitude = 0.0f;
            return true;
        }
        else if (key == kb.rollCCW.key || key == kb.rollCW.key) {
            movMod.roll = 0.0f;
            return true;
        }
        else if (key == kb.yawLeft.key || key == kb.yawRight.key) {
            movMod.yaw = 0.0f;
            return true;
        }
        else if (key == kb.pitchUp.key || key == kb.pitchDown.key) {
            movMod.pitch = 0.0f;
            return true;
        }
    }
    else if (event->type() == QEvent::MouseMove && wasRotRead) {
        wasRotRead = false;
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QPoint pos = mouseEvent->globalPosition().toPoint();
#else
        QPoint pos = mouseEvent->globalPos();
#endif
        if (lockMouse) {
            movMod.pitch = -(pos.ry() - oldPos.ry()) * mLookSens;
            movMod.yaw = -(pos.rx() - oldPos.rx()) * mLookSens;
            QCursor::setPos(lockPos);
        }
        oldPos = QCursor::pos();
    }
    return QDialog::eventFilter(obj, event);
}

void FlightMode::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

void FlightMode::updateKeyEdit(KeyBind& bind, int key)
{
    bind.setKey(key);
    bind.lineEdit->clearFocus();
}


bool FlightMode::checkCursorLocking()
{
    QPoint pos = QCursor::pos();
    QPoint initialPos = pos;
    if (pos.rx() > 100) {
        pos.setX(5);
    }
    else {
        pos.setX(200);
    }
    QCursor::setPos(pos);
    pos = QCursor::pos();
    bool moved = initialPos != pos;
    QCursor::setPos(initialPos);
    return moved;
}

void FlightMode::togglePrefs()
{
    ui->prefsGroupBox->setVisible(!ui->prefsGroupBox->isVisible());
}

void FlightMode::onLockMouseButtonToggled(bool pressed)
{
    lockMouse = pressed;
    if (pressed) {
        QApplication::setOverrideCursor(Qt::BlankCursor);
        lockPos = QCursor::pos();
        setUnlockInfoText();
    }
    else {
        setDefaultInfoText();
        QApplication::restoreOverrideCursor();
    }
}
void FlightMode::onExitButtonClicked()
{
    saveSettings();
    reject();
}

void FlightMode::onMLookSensSliderValueChanged(int v)
{
    mLookSens = (float)v / 50.0f;
}

void FlightMode::onLinVelSliderValueChanged(int v)
{
    linVel = (float)v * 2.0f;
}

void FlightMode::onAngVelSliderValueChanged(int v)
{
    angVel = (float)v / 5000.0f;
}

#include "moc_FlightMode.cpp"
