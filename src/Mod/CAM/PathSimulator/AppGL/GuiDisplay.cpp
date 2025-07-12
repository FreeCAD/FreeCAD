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

#include "GuiDisplay.h"

#include <iostream>

#include "ui_GuiDisplay.h"

using namespace CAMSimulator;

GuiDisplay::GuiDisplay(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_GuiDisplay)
{
    ui->setupUi(this);

    playing = true;
    setPlaying(false);

    speed = 0;
    setSpeed(1);

    // clang-format off
    connect(ui->slowerButton, &QToolButton::clicked, this, &GuiDisplay::onSlowerFasterButtonClicked);
    connect(ui->fasterButton, &QToolButton::clicked, this, &GuiDisplay::onSlowerFasterButtonClicked);
    // clang format on
}

GuiDisplay::~GuiDisplay()
{
    delete ui;
}

void GuiDisplay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    setMask(childrenRegion());
}

void GuiDisplay::setPlaying(bool b)
{
	if (b == playing) {
		return;
	}

    playing = b;

    const auto icon = playing ? ":/gl_simulator/Pause.png" : ":/gl_simulator/Play.png";
    ui->playButton->setIcon(QIcon(QString::fromUtf8(icon)));
}

void GuiDisplay::on_playButton_clicked()
{
    setPlaying(!playing);
    Q_EMIT play(playing);
}

void GuiDisplay::on_singleStepButton_clicked()
{
    setPlaying(false);
    Q_EMIT singleStep();
}

void GuiDisplay::setSpeed(int s)
{
	if (s == speed) {
		return;
	}

	speed = s;
	ui->speedLabel->setText(tr("x%1").arg(speed));
}

static const std::vector<int> speeds = { 1, 2, 5, 10, 25, 50 };

std::vector<int>::const_iterator findNearestSpeed(int speed)
{
	int dist = std::numeric_limits<int>::max();
	auto ret = speeds.cend();

	for(auto it = speeds.cbegin(); it != speeds.cend(); it++)
	{
		const int curdist = std::abs(*it - speed);
		if(curdist < dist)
		{
			dist = curdist;
			ret = it;
		}
	}

	return ret;
}

void GuiDisplay::onSlowerFasterButtonClicked()
{
	auto it = findNearestSpeed(speed);

	const bool slower = sender() == ui->slowerButton;
	const bool faster = !slower;

	if(slower && it != speeds.begin())
		it--;
	else if(faster && it != (speeds.end() -1))
		it++;

	setSpeed(*it);
	Q_EMIT speedChanged(*it);
}

void GuiDisplay::setStage(float f, int total)
{
	ui->stageSlider->setMaximum(total);
	ui->stageSlider->setValue(f * total);
}

void GuiDisplay::on_stageSlider_sliderMoved(int value)
{
	const float f = (float)value / ui->stageSlider->maximum();
	Q_EMIT stageChanged(f);
}

void GuiDisplay::on_viewAllButton_clicked()
{
	Q_EMIT viewAll();
}

void GuiDisplay::setStockVisible(bool b)
{
	stockVisible = b;
}

void GuiDisplay::setBaseVisible(bool b)
{
	baseVisible = b;
}

void GuiDisplay::on_stockModelButton_clicked()
{
	// stock -> base -> both
	//   ^---------------'

	bool sv, bv;

	if (stockVisible == baseVisible) {
		sv = true;
		bv = false;
	}
	else if (!baseVisible) {
		sv = false;
		bv = true;
	}
	else if (!stockVisible) {
		sv = true;
		bv = true;
	}

	if (sv != stockVisible) {
		setStockVisible(sv);
		Q_EMIT stockVisibleChanged(sv);
	}

	if (bv != baseVisible) {
		setBaseVisible(bv);
		Q_EMIT baseVisibleChanged(bv);
	}
}

void GuiDisplay::setPathVisible(bool b)
{
	pathVisible = b;
}

void GuiDisplay::on_pathButton_clicked()
{
	setPathVisible(!pathVisible);
	Q_EMIT pathVisibleChanged(pathVisible);
}

void GuiDisplay::setSsaoEnabled(bool b)
{
	ssaoEnabled = b;
}

void GuiDisplay::on_ssaoButton_clicked()
{
	setSsaoEnabled(!ssaoEnabled);
	Q_EMIT ssaoEnableChanged(ssaoEnabled);
}
