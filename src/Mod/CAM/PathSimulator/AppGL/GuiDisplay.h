// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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

#ifndef __guidisplay_t__
#define __guidisplay_t__

#include <QWidget>

namespace CAMSimulator
{
class Ui_GuiDisplay;

class GuiDisplay: public QWidget
{
    Q_OBJECT

public:
    explicit GuiDisplay(QWidget* parent = nullptr);
    ~GuiDisplay();

    void setPlaying(bool b);
    void setSpeed(int s);
    void setStage(float f, int total);

    void setStockVisible(bool b);
    void setBaseVisible(bool b);
    void setRotateEnabled(bool b);
    void setPathVisible(bool b);
    void setSsaoEnabled(bool b);

Q_SIGNALS:
    void play(bool b);
    void singleStep();
    void speedChanged(int s);
    void stageChanged(float f);

    void viewAll();
    void stockVisibleChanged(bool b);
    void baseVisibleChanged(bool b);
    void rotateEnableChanged(bool b);
    void pathVisibleChanged(bool b);
    void ssaoEnableChanged(bool b);

protected:
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void on_playButton_clicked();
    void on_singleStepButton_clicked();

    void onSlowerFasterButtonClicked();
    void on_stageSlider_sliderMoved(int value);

    void on_stockModelButton_clicked();

private:
    Ui_GuiDisplay* ui;

    bool playing = true;
    int speed = 1;

    bool stockVisible = true;
    bool baseVisible = false;
};

}  // namespace CAMSimulator

#endif  // __guidisplay_t__
