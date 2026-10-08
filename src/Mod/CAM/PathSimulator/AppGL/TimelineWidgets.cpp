// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "TimelineWidgets.h"

#include <QPaintEvent>
#include <QPainter>
#include <QSlider>
#include <QStyleOptionSlider>
#include <algorithm>
#include <cmath>
#include <optional>

namespace CAMSimulator
{

namespace
{

// The slider handle's center at the minimum (x0) and maximum (x0 + span), in slider coordinates,
// so marks line up with the handle.
struct SliderTrack
{
    int x0 = 0;
    int span = 0;
};

SliderTrack sliderTrack(const QSlider* slider)
{
    // Set up the option as QSlider does. Without State_Horizontal a style sheet lays it out as a
    // vertical slider and every mark lands on one point.
    QStyleOptionSlider opt;
    opt.initFrom(slider);
    opt.orientation = Qt::Horizontal;
    opt.state |= QStyle::State_Horizontal;
    opt.upsideDown = slider->invertedAppearance() != (slider->layoutDirection() == Qt::RightToLeft);
    opt.minimum = slider->minimum();
    opt.maximum = slider->maximum();
    opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    QStyle* style = slider->style();
    auto handleCenter = [&](int value) {
        opt.sliderPosition = value;
        opt.sliderValue = value;
        return style->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, slider)
            .center()
            .x();
    };
    SliderTrack track;
    track.x0 = handleCenter(slider->minimum());
    track.span = handleCenter(slider->maximum()) - track.x0;
    return track;
}

// sets value and repaints widget, when value changes
template<typename T>
void setAndRepaint(QWidget* widget, T& value, const T& to)
{
    if (to != value) {
        value = to;
        widget->update();
    }
}

// room for each axis's turn in an index, and for all of them together at most this share of
// the slider
constexpr int MinTurnPixels = 10;
constexpr float MaxTurnShare = 0.5f;

// x along the piecewise line through from[i], to[i], both increasing from 0 to 1
float alongLine(const std::vector<float>& from, const std::vector<float>& to, float x)
{
    if (from.size() < 2) {
        return x;
    }
    x = std::clamp(x, 0.f, 1.f);
    const auto after = std::upper_bound(from.begin(), from.end(), x);
    const size_t i = std::clamp<size_t>(after - from.begin(), 1, from.size() - 1);
    const float span = from[i] - from[i - 1];
    const float t = span > 0 ? (x - from[i - 1]) / span : 0.f;
    return to[i - 1] + t * (to[i] - to[i - 1]);
}

}  // namespace

QColor RotaryAxisColor(int axis)
{
    static const QColor colors[] = {
        QColor(255, 159, 26),   // orange
        QColor(62, 197, 255),   // cyan
        QColor(255, 110, 170),  // pink
        QColor(124, 252, 0),    // green
    };
    if (axis < 0) {
        return QColor(255, 213, 79);
    }
    return colors[axis % 4];
}

bool TimelineScale::set(const std::vector<SimTimeSpan>& s, const QSlider* slider)
{
    const int length = sliderTrack(slider).span;
    if (s == spans && length == pixels) {
        return false;
    }
    spans = s;
    pixels = length;
    build();
    return true;
}

bool TimelineScale::fit(const QSlider* slider)
{
    return set(spans, slider);
}

void TimelineScale::build()
{
    times.clear();
    places.clear();
    // the pieces of time between the starts and ends of the index turns
    std::vector<float> bounds {0.f, 1.f};
    for (const SimTimeSpan& s : spans) {
        if (!s.continuous) {
            bounds.push_back(std::clamp(s.start, 0.f, 1.f));
            bounds.push_back(std::clamp(s.end, 0.f, 1.f));
        }
    }
    std::ranges::sort(bounds);
    bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
    if (bounds.size() <= 2 || pixels <= 0) {
        return;
    }
    std::vector<bool> turning(bounds.size() - 1);
    for (size_t i = 0; i + 1 < bounds.size(); i++) {
        const float middle = (bounds[i] + bounds[i + 1]) / 2;
        turning[i] = std::ranges::any_of(spans, [middle](const SimTimeSpan& s) {
            return !s.continuous && s.start <= middle && middle <= s.end;
        });
    }
    // each turn at least its room, less room when there are too many to fit
    float room = (float)MinTurnPixels / (float)pixels;
    for (;;) {
        float extra = 0;
        for (size_t i = 0; i < turning.size(); i++) {
            if (turning[i]) {
                extra += std::max(0.f, room - (bounds[i + 1] - bounds[i]));
            }
        }
        if (extra <= MaxTurnShare || room < 1e-6f) {
            break;
        }
        room /= 2;
    }
    std::vector<float> widths;
    float total = 0;
    for (size_t i = 0; i < turning.size(); i++) {
        const float time = bounds[i + 1] - bounds[i];
        widths.push_back(turning[i] ? std::max(time, room) : time);
        total += widths.back();
    }
    times = bounds;
    places.push_back(0);
    for (float width : widths) {
        places.push_back(places.back() + width / total);
    }
    places.back() = 1;
}

float TimelineScale::toSlider(float time) const
{
    return alongLine(times, places, time);
}

float TimelineScale::toTime(float place) const
{
    return alongLine(places, times, place);
}

OperationMarkers::OperationMarkers(QSlider* slider, const TimelineScale& scale)
    : QWidget(slider)
    , slider(slider)
    , scale(scale)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    // a theme's style sheet may give every widget a background, which would hide the slider
    setStyleSheet(QStringLiteral("background: transparent;"));
    setGeometry(slider->rect());
}

void OperationMarkers::setInk(const QColor& c)
{
    setAndRepaint(this, ink, c);
}

void OperationMarkers::setStarts(const std::vector<float>& s)
{
    setAndRepaint(this, starts, s);
}

void OperationMarkers::setSpans(const std::vector<SimTimeSpan>& s)
{
    setAndRepaint(this, spans, s);
}

void OperationMarkers::paintEvent(QPaintEvent* /*event*/)
{
    if (starts.empty() && spans.empty()) {
        return;
    }
    const SliderTrack track = sliderTrack(slider);
    // where a time shows
    const auto at = [&](float time) {
        return track.x0 + (int)std::lround(scale.toSlider(time) * (float)track.span);
    };

    QPainter painter(this);
    // every mark this wide, to be seen easily
    const int markWidth = 4;
    // the rotary turns, in lanes on the middle line: the first axis above it, the second below,
    // the whole table both
    const int middle = height() / 2;
    const int lineHalf = 3;
    const int lane = 3;
    // the line sits a little below the widget's middle, so the lower lane drops this much more
    // to leave it the same light gap the upper lane has
    const int lowerDrop = 2;
    for (const SimTimeSpan& s : spans) {
        const int left = at(s.start);
        const int width = std::max(2, at(s.end) - left);
        const QColor color = RotaryAxisColor(s.axis);
        if (s.axis < 0 || s.axis % 2 == 0) {
            painter.fillRect(left, middle - lineHalf - lane, width, lane, color);
        }
        if (s.axis < 0 || s.axis % 2 == 1) {
            painter.fillRect(left, middle + lineHalf + lowerDrop, width, lane, color);
        }
    }
    if (starts.empty()) {
        return;
    }
    painter.setPen(QPen(ink, markWidth));
    // operation starts and the end of the last operation: one mark at each spot
    const int near = 4;
    std::vector<int> marks;
    for (float f : starts) {
        marks.push_back(at(f));
    }
    marks.push_back(track.x0 + track.span);
    std::ranges::sort(marks);
    std::optional<int> last;
    for (int x : marks) {
        if (!last || x - *last > near) {
            painter.drawLine(x, 1, x, height() - 2);
            last = x;
        }
    }
}

AxisNames::AxisNames(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet(QStringLiteral("background: transparent;"));
    setFixedWidth(14);
    // room for a letter above the slider's middle line and one below
    setMinimumHeight(22);
}

void AxisNames::setNames(const QStringList& n)
{
    setAndRepaint(this, axisNames, n);
}

void AxisNames::paintEvent(QPaintEvent* /*event*/)
{
    if (axisNames.isEmpty()) {
        return;
    }
    // beside the lanes: the first axis in the half above the middle line, the second below
    const int middle = height() / 2;
    QPainter painter(this);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(std::clamp(middle, 6, 10));
    painter.setFont(font);
    for (int k = 0; k < axisNames.size(); k++) {
        const QRect row = k % 2 == 0 ? QRect(0, 0, width() - 1, middle)
                                     : QRect(0, middle, width() - 1, height() - middle);
        painter.setPen(RotaryAxisColor(k));
        painter.drawText(row, Qt::AlignRight | Qt::AlignVCenter, axisNames[k]);
    }
}

}  // namespace CAMSimulator
