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

#pragma once

#include "MillMotion.h"
#include <QColor>
#include <QStringList>
#include <QWidget>
#include <vector>

class QSlider;

namespace CAMSimulator
{

// A rotary axis's color for its bars and name labels; axis -1 is the whole table turning at once.
QColor RotaryAxisColor(int axis);

/**
 * Where the program's time shows along the stage slider. A rotary index takes a moment, so each
 * axis's turn in one gets room of its own, at least MinTurnPixels wide, and the rest of the
 * program shares what is left in proportion to its time, like clips laid end to end. Turns
 * while cutting keep their time. Times and places are fractions, 0 to 1.
 */
class TimelineScale
{
public:
    // true when the scale changed
    bool set(const std::vector<SimTimeSpan>& spans, const QSlider* slider);
    // again for the slider's new length
    bool fit(const QSlider* slider);
    // where a time shows along the slider, and the time shown at a place
    float toSlider(float time) const;
    float toTime(float place) const;

private:
    void build();

    std::vector<SimTimeSpan> spans;
    int pixels = 0;
    std::vector<float> times;   // breaks between pieces, from 0 to 1
    std::vector<float> places;  // where each shows, from 0 to 1
};

/**
 * Marks over the stage slider: operation starts and
 * rotary turns in lanes along the slider's middle line, the first axis above it, the second
 * below. Mouse events pass through. Times are fractions of the program time, placed by scale.
 */
class OperationMarkers: public QWidget
{
public:
    OperationMarkers(QSlider* slider, const TimelineScale& scale);

    // the color of the operation and rotary marks
    void setInk(const QColor& c);
    void setStarts(const std::vector<float>& s);
    void setSpans(const std::vector<SimTimeSpan>& s);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QSlider* slider;
    const TimelineScale& scale;
    // color of the operation and rotary marks, set to show on the background
    QColor ink {255, 255, 255, 200};
    std::vector<float> starts;
    std::vector<SimTimeSpan> spans;
};

// The rotary axes' names left of the stage slider, each in its color beside its lane
class AxisNames: public QWidget
{
public:
    explicit AxisNames(QWidget* parent);

    void setNames(const QStringList& names);
    const QStringList& names() const
    {
        return axisNames;
    }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QStringList axisNames;
};

}  // namespace CAMSimulator
