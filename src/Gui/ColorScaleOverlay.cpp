// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Contributors                               *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 **************************************************************************/

#include "ColorScaleOverlay.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>


using namespace Gui;

namespace
{
constexpr float scientificLimit = 10000.0F;

bool isZeroBased(const App::ColorGradient& gradient, float minimum, float maximum)
{
    return minimum < 0.0F && maximum > 0.0F && gradient.getStyle() == App::ColorBarStyle::ZERO_BASED;
}

int displayedGradientTickCount(const App::ColorGradient& gradient, float minimum, float maximum, int precision)
{
    int count = static_cast<int>(gradient.getCountColors());
    const float scale = std::pow(10.0F, static_cast<float>(precision));
    const float range = std::fabs(maximum - minimum) * scale;
    const int maximumFixedLabels = static_cast<int>(std::floor(range + 1.0e-4F)) + 1;

    if (maximumFixedLabels >= 2) {
        count = std::min(count, maximumFixedLabels);
    }

    if (isZeroBased(gradient, minimum, maximum) && count > 2 && count % 2 == 0) {
        --count;
    }

    return count;
}

std::vector<float> gradientTickValues(
    const App::ColorGradient& gradient,
    float minimum,
    float maximum,
    int precision
)
{
    int count = displayedGradientTickCount(gradient, minimum, maximum, precision);
    std::vector<float> values;
    values.reserve(static_cast<std::size_t>(count));

    if (isZeroBased(gradient, minimum, maximum)) {
        if (count % 2 == 0) {
            ++count;
        }
        const int half = count / 2;
        for (int index = 0; index <= half; ++index) {
            const float factor = static_cast<float>(index) / static_cast<float>(half);
            values.push_back((1.0F - factor) * maximum);
        }
        for (int offset = 1; offset <= half; ++offset) {
            const float factor = static_cast<float>(offset) / static_cast<float>(half);
            values.push_back(factor * minimum);
        }
    }
    else {
        for (int index = 0; index < count; ++index) {
            const float factor = static_cast<float>(index) / static_cast<float>(count - 1);
            values.push_back((1.0F - factor) * maximum + factor * minimum);
        }
    }

    return values;
}

std::string formatValue(float value, int precision, bool scientific)
{
    std::ostringstream stream;
    stream << std::setprecision(precision) << std::showpoint << std::showpos;
    stream << (scientific ? std::scientific : std::fixed) << value;
    return stream.str();
}

bool labelsCollide(const std::vector<float>& values, int precision, bool scientific)
{
    std::vector<std::string> labels;
    labels.reserve(values.size());
    for (const float value : values) {
        labels.push_back(formatValue(value, precision, scientific));
    }
    std::sort(labels.begin(), labels.end());
    return std::adjacent_find(labels.begin(), labels.end()) != labels.end();
}

bool useScientificNotation(const std::vector<float>& values, int precision)
{
    float maximumMagnitude {};
    for (const float value : values) {
        maximumMagnitude = std::max(maximumMagnitude, std::fabs(value));
    }

    return maximumMagnitude > scientificLimit
        || (labelsCollide(values, precision, false) && !labelsCollide(values, precision, true));
}

bool useLegendScientificNotation(float minimum, float maximum, int precision)
{
    const float epsilon = std::pow(10.0F, static_cast<float>(-precision));
    return std::min(std::fabs(minimum), std::fabs(maximum)) < epsilon;
}

}  // namespace

ColorScaleOverlay::ColorScaleOverlay()
{
    gradient.setStyle(App::ColorBarStyle::FLOW);
    gradient.setColorModel(0);
}

void ColorScaleOverlay::setMode(ColorScaleMode mode)
{
    if (currentMode == mode) {
        return;
    }
    currentMode = mode;
    changed();
}

ColorScaleMode ColorScaleOverlay::mode() const
{
    return currentMode;
}

void ColorScaleOverlay::setRange(float minimum, float maximum, int precision)
{
    gradient.setRange(minimum, maximum);
    updateLegendRange(minimum, maximum);
    decimalPlaces = precision;
    changed();
}

void ColorScaleOverlay::setOutsideGrayed(bool value)
{
    gradient.setOutsideGrayed(value);
    colorLegend.setOutsideGrayed(value);
    changed();
}

bool ColorScaleOverlay::isVisible(float value) const
{
    return currentMode == ColorScaleMode::Gradient
        && (!gradient.isOutsideInvisible() || !gradient.isOutOfRange(value));
}

Base::Color ColorScaleOverlay::getColor(float value) const
{
    return currentMode == ColorScaleMode::Gradient ? gradient.getColor(value)
                                                   : colorLegend.getColor(value);
}

float ColorScaleOverlay::minimum() const
{
    return currentMode == ColorScaleMode::Gradient ? gradient.getMinValue()
                                                   : colorLegend.getMinValue();
}

float ColorScaleOverlay::maximum() const
{
    return currentMode == ColorScaleMode::Gradient ? gradient.getMaxValue()
                                                   : colorLegend.getMaxValue();
}

void ColorScaleOverlay::setTextFormat(const ColorScaleTextFormat& textFormat)
{
    format = textFormat;
    changed();
}

ColorScaleTextFormat ColorScaleOverlay::textFormat() const
{
    return format;
}

int ColorScaleOverlay::precision() const
{
    return decimalPlaces;
}

void ColorScaleOverlay::setGradientProfile(const App::ColorGradientProfile& profile, int precision)
{
    gradient.setProfile(profile);
    updateLegendRange(profile.fMin, profile.fMax);
    decimalPlaces = precision;
    changed();
}

App::ColorGradientProfile ColorScaleOverlay::gradientProfile() const
{
    return gradient.getProfile();
}

const App::ColorGradient& ColorScaleOverlay::gradientData() const
{
    return gradient;
}

void ColorScaleOverlay::setLegend(const App::ColorLegend& legend, int precision)
{
    colorLegend = legend;
    decimalPlaces = precision;
    changed();
}

App::ColorLegend ColorScaleOverlay::legend() const
{
    return colorLegend;
}

const App::ColorLegend& ColorScaleOverlay::legendData() const
{
    return colorLegend;
}

ColorScaleSnapshot ColorScaleOverlay::snapshot() const
{
    return snapshot(currentMode);
}

ColorScaleSnapshot ColorScaleOverlay::snapshot(ColorScaleMode mode) const
{
    ColorScaleSnapshot result;
    result.mode = mode;
    result.textFormat = format;
    result.minimum = mode == ColorScaleMode::Gradient ? gradient.getMinValue()
                                                      : colorLegend.getMinValue();
    result.maximum = mode == ColorScaleMode::Gradient ? gradient.getMaxValue()
                                                      : colorLegend.getMaxValue();
    result.revision = generation;

    if (mode == ColorScaleMode::Gradient) {
        result.gradientStops = gradient.getColorModel().colors;
        const auto values = gradientTickValues(gradient, result.minimum, result.maximum, decimalPlaces);
        const bool scientific = useScientificNotation(values, decimalPlaces);
        result.ticks.reserve(values.size());
        for (const float value : values) {
            result.ticks.push_back({value, formatValue(value, decimalPlaces, scientific)});
        }
        return result;
    }

    const std::size_t count = colorLegend.hasNumberOfFields();
    result.intervals.reserve(count);
    result.ticks.reserve(count + 1);
    const bool scientific = useLegendScientificNotation(result.minimum, result.maximum, decimalPlaces);

    for (std::size_t index = count; index-- > 0;) {
        result.intervals.push_back(
            {colorLegend.getColor(index),
             colorLegend.getValue(index),
             colorLegend.getValue(index + 1),
             colorLegend.getText(index)}
        );
    }
    for (std::size_t index = count + 1; index-- > 0;) {
        const float value = colorLegend.getValue(index);
        result.ticks.push_back({value, formatValue(value, decimalPlaces, scientific)});
    }
    return result;
}

void ColorScaleOverlay::updateLegendRange(float minimum, float maximum)
{
    const std::size_t count = colorLegend.hasNumberOfFields();
    for (std::size_t index = 0; index <= count; ++index) {
        const float factor = static_cast<float>(index) / static_cast<float>(count);
        colorLegend.setValue(index, (1.0F - factor) * minimum + factor * maximum);
    }
}

void ColorScaleOverlay::changed()
{
    ++generation;
    Notify(0);
}
