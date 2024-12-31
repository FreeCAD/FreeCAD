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

#ifndef GUI_DIALOG_FLIGHTMODE_H
#define GUI_DIALOG_FLIGHTMODE_H

#include <QDialog>
#include <QLineEdit>
#include <FCGlobal.h>

class QTimer;
class QPoint;
class SoCamera;
class SbVec3f;
class SbRotation;

struct MovementModifier
{
    // translation
    float walk = 0.0f;      // backward - forwward
    float sidestep = 0.0f;  // left - right
    float altitude = 0.0f;  // down - up
    // rotation
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
};

struct KeyBind
{
    int key;
    QString text;
    QLineEdit* lineEdit;
    void setKey(int k)
    {
        this->key = k;
        QString s = this->text;
        this->lineEdit->setText(s.append(QKeySequence(key).toString()));
    }
};
struct KeyBindings
{
    KeyBind forward;
    KeyBind backward;
    KeyBind stepLeft;
    KeyBind stepRight;
    KeyBind up;
    KeyBind down;
    KeyBind rollCCW;
    KeyBind rollCW;
    KeyBind yawLeft;
    KeyBind yawRight;
    KeyBind pitchUp;
    KeyBind pitchDown;
};

namespace Gui
{
class View3DInventor;
namespace Dialog
{

/**
 * @author Adrian Przekwas
 */

class Ui_FlightMode;
class GuiExport FlightMode: public QDialog
{
    Q_OBJECT

public:
    explicit FlightMode(QWidget* = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    void addToActiveView();
    ~FlightMode() override;

protected:
    void setupConnections();
    void onLockMouseButtonToggled(bool);
    void onExitButtonClicked();
    void onMLookSensSliderValueChanged(int);
    void onLinVelSliderValueChanged(int);
    void onAngVelSliderValueChanged(int);

private:
    void updatePosition();
    void moveCamera(SoCamera* cam, const SbVec3f& pos, const SbRotation& rot);
    Gui::View3DInventor* activeView() const;
    bool checkCursorLocking();
    void togglePrefs();
    void saveSettings();
    void populateKeyBindings();
    void updateKeyEdit(KeyBind& bind, int key);
    void setDefaultInfoText();
    void setUnlockInfoText();
    void changeEvent(QEvent* e) override;
    bool eventFilter(QObject*, QEvent*) override;

    MovementModifier movMod;
    Ui_FlightMode* ui;
    QTimer* timer;
    QPoint oldPos;
    float linVel = 100.0f;  // mm per step
    float angVel = 0.01f;   // rad per pixel
    float mLookSens = 1.0f;
    float rRateMod = 1.0f;
    bool wasRotRead = false;
    bool lockMouse = false;
    bool lockable;
    QPoint lockPos;
    KeyBindings kb;
};

}  // namespace Dialog
}  // namespace Gui

#endif  // GUI_DIALOG_FLIGHTMODE_H
