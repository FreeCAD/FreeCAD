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
#include "CAMSettings.h"
#include "TimelineWidgets.h"

#include "ui_GuiDisplay.h"
#include <QFontMetrics>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <Base/Quantity.h>
#include <Base/Unit.h>
#include <cmath>
#include <limits>
#include <string>

// include this last as the defines can mess up other includes
#include "OpenGlWrapper.h"

namespace CAMSimulator
{

GuiDisplay::GuiDisplay(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_GuiDisplay)
{
    ui->setupUi(this);

    // Give each readout the width of its longest text, so the row does not shift as values change.
    const QFontMetrics metrics(ui->timeLabel->font());
    const int padding = 16;
    ui->timeLabel->setMinimumWidth(
        metrics.horizontalAdvance(QStringLiteral("00:00:00 / 00:00:00")) + padding
    );
    ui->feedLabel->setMinimumWidth(
        metrics.horizontalAdvance(tr("Rapid %1").arg(QStringLiteral("00000.0 mm/min"))) + padding
    );
    ui->fpsLabel->setMinimumWidth(
        metrics.horizontalAdvance(tr("%1 fps").arg(QStringLiteral("000"))) + padding
    );
    // The readouts keep fixed widths; the operation name, which can be long, takes the remaining
    // space.
    ui->speedLabel->setMinimumWidth(metrics.horizontalAdvance(QStringLiteral("x50")));
    for (QLabel* label : {ui->speedLabel, ui->timeLabel, ui->feedLabel, ui->fpsLabel}) {
        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    }
    ui->opLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    opMarkers = new OperationMarkers(ui->stageSlider, timeScale);
    ui->stageSlider->installEventFilter(this);

    // the rotary axes' names left of the slider, beside their lanes
    const int sliderRow = ui->verticalLayout->indexOf(ui->stageSlider);
    ui->verticalLayout->removeWidget(ui->stageSlider);
    auto row = new QHBoxLayout();
    row->setSpacing(0);
    axisNames = new AxisNames(this);
    row->addWidget(axisNames);
    row->addWidget(ui->stageSlider);
    ui->verticalLayout->insertLayout(sliderRow, row);

    // the frame rate goes in the top left corner, away from the control bar
    ui->horizontalLayout->removeWidget(ui->fpsLabel);
    ui->verticalLayout->insertWidget(0, ui->fpsLabel, 0, Qt::AlignLeft | Qt::AlignTop);

    playing = true;
    setPlaying(false);

    speed = 0;
    setSpeed(1);

    connect(ui->slowerButton, &QToolButton::clicked, this, &GuiDisplay::onSlowerFasterButtonClicked);
    connect(ui->fasterButton, &QToolButton::clicked, this, &GuiDisplay::onSlowerFasterButtonClicked);
    connect(ui->viewAllButton, &QToolButton::clicked, this, &GuiDisplay::viewAll);
    connect(ui->rotateButton, &QToolButton::toggled, this, &GuiDisplay::rotateEnableChanged);
    connect(ui->pathButton, &QToolButton::toggled, this, &GuiDisplay::pathVisibleChanged);
    connect(ui->ssaoButton, &QToolButton::toggled, this, &GuiDisplay::ssaoEnableChanged);
    connect(ui->incrementalButton, &QToolButton::toggled, this, &GuiDisplay::incrementalEnableChanged);
    connect(ui->tablePoseButton, &QToolButton::toggled, this, &GuiDisplay::tablePoseEnableChanged);
}

GuiDisplay::~GuiDisplay()
{
    delete ui;
}

void GuiDisplay::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(event->rect(), Qt::transparent);
}

bool GuiDisplay::eventFilter(QObject* watched, QEvent* event)
{
    // keep the markers the same size as the slider they cover
    if (watched == ui->stageSlider && event->type() == QEvent::Resize) {
        opMarkers->setGeometry(ui->stageSlider->rect());
        // the turns' room is in pixels
        if (timeScale.fit(ui->stageSlider)) {
            showStage();
        }
    }
    return QWidget::eventFilter(watched, event);
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

void GuiDisplay::on_nextOpButton_clicked()
{
    Q_EMIT nextMark();
}

void GuiDisplay::on_prevOpButton_clicked()
{
    Q_EMIT previousMark();
}

void GuiDisplay::setIndexSpans(const std::vector<SimTimeSpan>& spans, const QStringList& names)
{
    opMarkers->setSpans(spans);
    if (timeScale.set(spans, ui->stageSlider)) {
        showStage();
    }
    if (names != axisNames->names()) {
        axisNames->setNames(names);
        // the slider's tooltip is the legend: each axis name in its color
        QString tip = tr("When the rotary axes turn:");
        for (int k = 0; k < names.size(); k++) {
            tip += QStringLiteral(" <b style='color:%1'>%2</b>")
                       .arg(RotaryAxisColor(k).name(), names[k]);
        }
        ui->stageSlider->setToolTip(names.isEmpty() ? QString() : tip);
    }
}

void GuiDisplay::setIndexAngles(const QStringList& axisNames, const std::vector<float>& angles)
{
    // shown in place of the feed during an index turn, each axis name in its color
    QString text = tr("Index");
    for (size_t k = 0; k < angles.size() && (int)k < axisNames.size(); k++) {
        text += QStringLiteral(" &nbsp;<b style='color:%1'>%2</b> %3°")
                    .arg(RotaryAxisColor((int)k).name(), axisNames[(int)k])
                    .arg(angles[k], 0, 'f', 1);
    }
    setFeedLabel(text, false);
}

void GuiDisplay::setOperationStarts(const std::vector<float>& starts)
{
    opMarkers->setStarts(starts);
}

namespace
{

// set a label's text or style only when it changes, to avoid relayouts
void setLabelText(QLabel* label, const QString& text)
{
    if (label->text() != text) {
        label->setText(text);
    }
}

void setLabelStyle(QLabel* label, const QString& style)
{
    if (label->styleSheet() != style) {
        label->setStyleSheet(style);
    }
}

// style for readouts spaced from their left neighbor
QString paddedLabelStyle(const QString& color)
{
    return QStringLiteral("color:%1; padding-left:12px").arg(color);
}

QString formatTime(float seconds)
{
    const int s = (int)std::lround(seconds);
    if (s >= 3600) {
        return QStringLiteral("%1:%2:%3")
            .arg(s / 3600)
            .arg((s / 60) % 60, 2, 10, QLatin1Char('0'))
            .arg(s % 60, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
}

}  // namespace

void GuiDisplay::setOperation(const QString& name)
{
    setLabelText(ui->opLabel, name);
}

void GuiDisplay::setSpeed(int s)
{
    if (s == speed) {
        return;
    }

    speed = s;
    ui->speedLabel->setText(tr("x%1").arg(speed));
}

static const std::vector<int> speeds = {1, 2, 5, 10, 15, 25, 50};

std::vector<int>::const_iterator findNearestSpeed(int speed)
{
    int dist = std::numeric_limits<int>::max();
    auto ret = speeds.cend();

    for (auto it = speeds.cbegin(); it != speeds.cend(); it++) {
        const int curdist = std::abs(*it - speed);
        if (curdist < dist) {
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

    if (slower && it != speeds.begin()) {
        it--;
    }
    else if (faster && it != (speeds.end() - 1)) {
        it++;
    }

    setSpeed(*it);
    Q_EMIT speedChanged(*it);
}

void GuiDisplay::setStage(float f, int total)
{
    stage = f;
    ui->stageSlider->setMaximum(total);
    showStage();
}

void GuiDisplay::showStage()
{
    const int total = ui->stageSlider->maximum();
    ui->stageSlider->setValue((int)std::lround(timeScale.toSlider(stage) * (float)total));
}

void GuiDisplay::setTime(float seconds, float totalSeconds)
{
    setLabelText(
        ui->timeLabel,
        QStringLiteral("%1 / %2").arg(formatTime(seconds), formatTime(totalSeconds))
    );
}

void GuiDisplay::setFeedLabel(const QString& text, bool rapid)
{
    setLabelText(ui->feedLabel, text);
    // rapid moves are shown in red
    feedRapid = rapid;
    setLabelStyle(ui->feedLabel, paddedLabelStyle((rapid ? rapidInk : ink).name()));
}

void GuiDisplay::setBackgroundColor(const QColor& background)
{
    // perceived brightness (Rec. 601 luma); above lightLuma the readouts use dark text
    const double luma = 0.299 * background.redF() + 0.587 * background.greenF()
        + 0.114 * background.blueF();
    const double lightLuma = 0.55;
    const bool light = luma > lightLuma;
    ink = light ? QColor(25, 25, 25) : QColor(Qt::white);
    rapidInk = light ? QColor(200, 30, 30) : QColor(255, 80, 80);
    restyle();
}

void GuiDisplay::restyle()
{
    const QString color = ink.name();
    setLabelStyle(ui->speedLabel, QStringLiteral("color:%1").arg(color));
    const QString padded = paddedLabelStyle(color);
    for (QLabel* label : {ui->timeLabel, ui->fpsLabel, ui->opLabel}) {
        setLabelStyle(label, padded);
    }
    setFeedLabel(ui->feedLabel->text(), feedRapid);
    QColor mark = ink;
    mark.setAlpha(200);
    opMarkers->setInk(mark);
}

void GuiDisplay::setFeed(float feed, bool rapid)
{
    if (feed <= 0) {
        setFeedLabel(rapid ? tr("Rapid") : QString(), rapid);
        return;
    }
    // feed is in mm/s; shown in the user's unit schema, to one decimal
    double factor = 1;
    std::string unit;
    Base::Quantity(feed, Base::Unit::Velocity).getUserString(factor, unit);
    const QString rate = QStringLiteral("%1 %2")
                             .arg(feed / (factor != 0 ? factor : 1.0), 0, 'f', 1)
                             .arg(QString::fromStdString(unit));
    setFeedLabel(rapid ? tr("Rapid %1").arg(rate) : tr("F %1").arg(rate), rapid);
}

void GuiDisplay::on_stageSlider_sliderMoved(int value)
{
    const float f = timeScale.toTime((float)value / (float)ui->stageSlider->maximum());
    Q_EMIT stageChanged(f);
}

void GuiDisplay::setStockVisible(bool b)
{
    stockVisible = b;

    QSignalBlocker blocker(ui->stockModelButton);
    ui->stockModelButton->setChecked(stockVisible && baseVisible);
}

void GuiDisplay::setBaseVisible(bool b)
{
    baseVisible = b;

    QSignalBlocker blocker(ui->stockModelButton);
    ui->stockModelButton->setChecked(stockVisible && baseVisible);
}

void GuiDisplay::on_stockModelButton_clicked()
{
    // stock -> base -> both
    //   ^---------------'

    bool sv = false;
    bool bv = false;

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

void GuiDisplay::setRotateEnabled(bool b)
{
    ui->rotateButton->setChecked(b);
}

void GuiDisplay::setPathVisible(bool b)
{
    QSignalBlocker blocker(ui->pathButton);
    ui->pathButton->setChecked(b);
}

void GuiDisplay::setSsaoEnabled(bool b)
{
    QSignalBlocker blocker(ui->ssaoButton);
    ui->ssaoButton->setChecked(b);
}

void GuiDisplay::setIncrementalEnabled(bool b)
{
    QSignalBlocker blocker(ui->incrementalButton);
    ui->incrementalButton->setChecked(b);
}

void GuiDisplay::setFps(float fps, bool shown)
{
    if (ui->fpsLabel->isHidden() == shown) {
        ui->fpsLabel->setVisible(shown);
        // only what the mask covers shows over the view
        layout()->activate();
        setMask(childrenRegion());
    }
    setLabelText(ui->fpsLabel, fps > 0 ? tr("%1 fps").arg(fps, 0, 'f', 0) : QString());
}

void GuiDisplay::setTablePoseEnabled(bool b)
{
    QSignalBlocker blocker(ui->tablePoseButton);
    ui->tablePoseButton->setChecked(b);
}

}  // namespace CAMSimulator
