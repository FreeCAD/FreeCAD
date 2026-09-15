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
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <App/ColorModel.h>
#include <Base/Observer.h>


namespace Gui
{

/** The presentation selected for a scalar color scale. */
enum class ColorScaleMode
{
    Gradient,
    Legend
};

/** Text appearance shared by every color-scale presentation. */
struct ColorScaleTextFormat
{
    // NOLINTBEGIN
    int textSize = 13;
    uint32_t textColor = 0xffffffff;
    // NOLINTEND
};

/** One labelled scalar position in a color-scale snapshot. */
struct ColorScaleTick
{
    float value {};
    std::string text;
};

/** One discrete interval in a color-scale snapshot. */
struct ColorScaleInterval
{
    Base::Color color;
    float minimum {};
    float maximum {};
    std::string label;
};

/**
 * Self-contained presentation data for one frame.
 *
 * Coordinates deliberately do not belong here: text measurement and final placement depend on the
 * presentation.
 */
struct ColorScaleSnapshot
{
    ColorScaleMode mode {ColorScaleMode::Gradient};
    ColorScaleTextFormat textFormat;
    float minimum {};
    float maximum {};
    std::vector<Base::Color> gradientStops;
    std::vector<ColorScaleTick> ticks;
    std::vector<ColorScaleInterval> intervals;
    std::uint64_t revision {};
};

/**
 * State and presentation data for a foreground color scale.
 *
 * Presentation code uses snapshot() rather than inheriting state from a scene graph.
 */
class GuiExport ColorScaleOverlay: public Base::Subject<int>, public App::ValueFloatToRGB
{
public:
    ColorScaleOverlay();

    void setMode(ColorScaleMode mode);
    ColorScaleMode mode() const;

    void setRange(float minimum, float maximum, int precision = 3);
    void setOutsideGrayed(bool value);
    bool isVisible(float value) const;

    Base::Color getColor(float value) const override;
    float minimum() const;
    float maximum() const;

    void setTextFormat(const ColorScaleTextFormat& format);
    ColorScaleTextFormat textFormat() const;
    int precision() const;

    void setGradientProfile(const App::ColorGradientProfile& profile, int precision);
    App::ColorGradientProfile gradientProfile() const;
    const App::ColorGradient& gradientData() const;
    void setLegend(const App::ColorLegend& legend, int precision = 3);
    App::ColorLegend legend() const;
    const App::ColorLegend& legendData() const;

    /** Return a copy for the currently selected presentation. */
    ColorScaleSnapshot snapshot() const;
    /** Return a copy for the requested presentation. */
    ColorScaleSnapshot snapshot(ColorScaleMode mode) const;

private:
    void updateLegendRange(float minimum, float maximum);
    void changed();

    App::ColorGradient gradient;
    App::ColorLegend colorLegend;
    ColorScaleMode currentMode {ColorScaleMode::Gradient};
    ColorScaleTextFormat format;
    int decimalPlaces {3};
    std::uint64_t generation {};
};

}  // namespace Gui
