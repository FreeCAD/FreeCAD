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


#ifndef GUI_DIALOG_DEMOMODE_H
#define GUI_DIALOG_DEMOMODE_H

#include <Inventor/SbLinear.h>
#include <QDialog>

class QTimer;
class SoCamera;
class SbRotation;

namespace Gui {
class View3DInventor;
namespace Dialog {

/**
 * @author Werner Mayer
 */
class Ui_DemoMode;
class GuiExport DemoMode : public QDialog
{
    Q_OBJECT

public:
    DemoMode(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~DemoMode();

    void accept();
    void reject();

protected Q_SLOTS:
    void on_playButton_toggled(bool);
    void on_fullscreen_toggled(bool);
    void on_timerCheck_toggled(bool);
    void on_speedSlider_valueChanged(int);
    void on_angleSlider_valueChanged(int);
    void on_timeout_valueChanged(int);
    void onAutoPlay();

private:
    void reset();
    float getSpeed(int) const;
    void reorientCamera(SoCamera * cam, const SbRotation & rot);
    SbVec3f getDirection(Gui::View3DInventor*) const;
    Gui::View3DInventor* activeView() const;
    void startAnimation(Gui::View3DInventor*);
    void changeEvent(QEvent *e);
    bool eventFilter(QObject *, QEvent *);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);

private:
    int oldvalue;
    SbVec3f viewAxis;
    bool wasHidden;
    QPoint pnt;
    QPoint oldPos;
    Ui_DemoMode* ui;
    QTimer* timer;
    QTimer* showHideTimer;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DEMOMODE_H
