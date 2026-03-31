// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include "Material.h"
#include <Base/Bitmask.h>

#include <algorithm>
#include <deque>
#include <string>
#include <vector>

namespace App
{

enum class Visibility
{
    Default,
    Grayed,
    Invisible
};

using VisibilityFlags = Base::Flags<Visibility>;

enum class ColorBarStyle
{
    FLOW,
    ZERO_BASED
};

}  // namespace App

ENABLE_BITMASK_OPERATORS(App::Visibility)

namespace App
{

/**
 * Abstract base class that calculates the matching RGB color to a given value.
 */
class AppExport ValueFloatToRGB
{
public:
    virtual Base::Color getColor(float fVal) const = 0;

protected:
    ValueFloatToRGB() = default;
    virtual ~ValueFloatToRGB() = default;
};


class AppExport ColorModel
{
public:
    ColorModel() = default;
    explicit ColorModel(std::size_t usCt)
    {
        colors.resize(usCt);
    }
    ColorModel(const ColorModel&) = default;
    virtual ~ColorModel() = default;
    ColorModel& operator=(const ColorModel&) = default;
    std::size_t getCountColors() const
    {
        return colors.size();
    }
    std::vector<Base::Color> colors;
};

class AppExport ColorModelBlueGreenRed: public ColorModel
{
public:
    ColorModelBlueGreenRed()
        : ColorModel(5)
    {
        colors[0] = Base::Color(0, 0, 1);
        colors[1] = Base::Color(0, 1, 1);
        colors[2] = Base::Color(0, 1, 0);
        colors[3] = Base::Color(1, 1, 0);
        colors[4] = Base::Color(1, 0, 0);
    }
};

class AppExport ColorModelBlueCyanGreen: public ColorModel
{
public:
    ColorModelBlueCyanGreen()
        : ColorModel(3)
    {
        colors[0] = Base::Color(0, 0, 1);
        colors[1] = Base::Color(0, 1, 1);
        colors[2] = Base::Color(0, 1, 0);
    }
};

class AppExport ColorModelGreenYellowRed: public ColorModel
{
public:
    ColorModelGreenYellowRed()
        : ColorModel(3)
    {
        colors[0] = Base::Color(0, 1, 0);
        colors[1] = Base::Color(1, 1, 0);
        colors[2] = Base::Color(1, 0, 0);
    }
};

class AppExport ColorModelRedGreenBlue: public ColorModel
{
public:
    ColorModelRedGreenBlue()
        : ColorModel(5)
    {
        colors[0] = Base::Color(1, 0, 0);
        colors[1] = Base::Color(1, 1, 0);
        colors[2] = Base::Color(0, 1, 0);
        colors[3] = Base::Color(0, 1, 1);
        colors[4] = Base::Color(0, 0, 1);
    }
};

class AppExport ColorModelGreenCyanBlue: public ColorModel
{
public:
    ColorModelGreenCyanBlue()
        : ColorModel(3)
    {
        colors[0] = Base::Color(0, 1, 0);
        colors[1] = Base::Color(0, 1, 1);
        colors[2] = Base::Color(0, 0, 1);
    }
};

class AppExport ColorModelRedYellowGreen: public ColorModel
{
public:
    ColorModelRedYellowGreen()
        : ColorModel(3)
    {
        colors[0] = Base::Color(1, 0, 0);
        colors[1] = Base::Color(1, 1, 0);
        colors[2] = Base::Color(0, 1, 0);
    }
};

class AppExport ColorModelBlueWhiteRed: public ColorModel
{
public:
    ColorModelBlueWhiteRed()
        : ColorModel(5)
    {
        colors[0] = Base::Color(0, 0, 1);
        colors[1] = Base::Color(float(85.0 / 255), float(170.0 / 255), 1);
        colors[2] = Base::Color(1, 1, 1);
        colors[3] = Base::Color(1, float(85.0 / 255), 0);
        colors[4] = Base::Color(1, 0, 0);
    }
};

class AppExport ColorModelBlueWhite: public ColorModel
{
public:
    ColorModelBlueWhite()
        : ColorModel(3)
    {
        colors[0] = Base::Color(0, 0, 1);
        colors[1] = Base::Color(float(85.0 / 255), float(170.0 / 255), 1);
        colors[2] = Base::Color(1, 1, 1);
    }
};

class AppExport ColorModelWhiteRed: public ColorModel
{
public:
    ColorModelWhiteRed()
        : ColorModel(3)
    {
        colors[0] = Base::Color(1, 1, 1);
        colors[1] = Base::Color(1, float(85.0 / 255), 0);
        colors[2] = Base::Color(1, 0, 0);
    }
};

class AppExport ColorModelBlackWhite: public ColorModel
{
public:
    ColorModelBlackWhite()
        : ColorModel(2)
    {
        colors[0] = Base::Color(0, 0, 0);
        colors[1] = Base::Color(1, 1, 1);
    }
};

class AppExport ColorModelBlackGray: public ColorModel
{
public:
    ColorModelBlackGray()
        : ColorModel(2)
    {
        colors[0] = Base::Color(0.0f, 0.0f, 0.0f);
        colors[1] = Base::Color(0.5f, 0.5f, 0.5f);
    }
};

class AppExport ColorModelGrayWhite: public ColorModel
{
public:
    ColorModelGrayWhite()
        : ColorModel(2)
    {
        colors[0] = Base::Color(0.5f, 0.5f, 0.5f);
        colors[1] = Base::Color(1.0f, 1.0f, 1.0f);
    }
};

class AppExport ColorModelWhiteBlack: public ColorModel
{
public:
    ColorModelWhiteBlack()
        : ColorModel(2)
    {
        colors[0] = Base::Color(1, 1, 1);
        colors[1] = Base::Color(0, 0, 0);
    }
};

class AppExport ColorModelWhiteGray: public ColorModel
{
public:
    ColorModelWhiteGray()
        : ColorModel(2)
    {
        colors[0] = Base::Color(1.0f, 1.0f, 1.0f);
        colors[1] = Base::Color(0.5f, 0.5f, 0.5f);
    }
};

class AppExport ColorModelGrayBlack: public ColorModel
{
public:
    ColorModelGrayBlack()
        : ColorModel(2)
    {
        colors[0] = Base::Color(0.5f, 0.5f, 0.5f);
        colors[1] = Base::Color(0.0f, 0.0f, 0.0f);
    }
};

struct AppExport ColorModelPack
{
    ColorModel totalModel = ColorModelBlueGreenRed();
    ColorModel topModel = ColorModelGreenYellowRed();
    ColorModel bottomModel = ColorModelBlueCyanGreen();
    std::string description;
    static ColorModelPack createRedGreenBlue();
    static ColorModelPack createBlueGreenRed();
    static ColorModelPack createWhiteBlack();
    static ColorModelPack createBlackWhite();
    static ColorModelPack createRedWhiteBlue();
};

class AppExport ColorField
{
public:
    ColorField();
    ColorField(const ColorField& rclCF) = default;
    ColorField(const ColorModel& rclModel, float fMin, float fMax, std::size_t usCt);
    virtual ~ColorField() = default;

    ColorField& operator=(const ColorField& rclCF);

    std::size_t getCountColors() const
    {
        return ctColors;
    }
    void set(const ColorModel& rclModel, float fMin, float fMax, std::size_t usCt);
    void setCountColors(std::size_t usCt)
    {
        set(colorModel, fMin, fMax, usCt);
    }
    void setRange(float fMin, float fMax)
    {
        set(colorModel, fMin, fMax, ctColors);
    }
    void getRange(float& rfMin, float& rfMax)
    {
        rfMin = fMin;
        rfMax = fMax;
    }
    std::size_t getMinColors() const
    {
        return colorModel.getCountColors();
    }
    void setColorModel(const ColorModel& rclModel);
    const ColorModel& getColorModel() const
    {
        return colorModel;
    }
    void setDirect(std::size_t usInd, Base::Color clCol)
    {
        colorField[usInd] = clCol;
    }
    float getMinValue() const
    {
        return fMin;
    }
    float getMaxValue() const
    {
        return fMax;
    }

    Base::Color getColor(std::size_t usIndex) const
    {
        return colorField[usIndex];
    }
    inline Base::Color getColor(float fVal) const;
    inline std::size_t getColorIndex(float fVal) const;

protected:
    ColorModel colorModel;
    float fMin, fMax;
    float fAscent, fConstant;  // Index := _fConstant + _fAscent * WERT
    std::size_t ctColors;
    std::vector<Base::Color> colorField;

    void rebuild();
    void interpolate(Base::Color clCol1, std::size_t usPos1, Base::Color clCol2, std::size_t usPos2);
};

inline Base::Color ColorField::getColor(float fVal) const
{
    // if the value is outside or at the border of the range
    std::size_t ct = colorModel.getCountColors() - 1;
    if (fVal <= fMin) {
        return colorModel.colors[0];
    }
    else if (fVal >= fMax) {
        return colorModel.colors[ct];
    }

    // get the Base::Color field position (with 0 < t < 1)
    float t = (fVal - fMin) / (fMax - fMin);
    Base::Color col(1.0f, 1.0f, 1.0f);  // white as default
    for (std::size_t i = 0; i < ct; i++) {
        float r = (float)(i + 1) / (float)ct;
        if (t < r) {
            // calculate the exact position in the subrange
            float s = t * float(ct) - float(i);
            Base::Color c1 = colorModel.colors[i];
            Base::Color c2 = colorModel.colors[i + 1];
            col.r = (1.0f - s) * c1.r + s * c2.r;
            col.g = (1.0f - s) * c1.g + s * c2.g;
            col.b = (1.0f - s) * c1.b + s * c2.b;
            break;
        }
    }

    return col;
}

inline std::size_t ColorField::getColorIndex(float fVal) const
{
    return std::size_t(
        std::min<int>(std::max<int>(int(fConstant + fAscent * fVal), 0), int(ctColors - 1)));
}

struct AppExport ColorGradientProfile
{
    ColorBarStyle tStyle {ColorBarStyle::FLOW};
    float fMin {};
    float fMax {};
    std::size_t ctColors {};
    std::size_t tColorModel {};
    VisibilityFlags visibility {Visibility::Default};

    ColorGradientProfile();
    ColorGradientProfile(const ColorGradientProfile&) = default;
    ColorGradientProfile& operator=(const ColorGradientProfile&) = default;

    bool isEqual(const ColorGradientProfile&) const;
};

class AppExport ColorGradient
{
public:
    ColorGradient();
    ColorGradient(float fMin,
                  float fMax,
                  std::size_t usCtColors,
                  ColorBarStyle tS,
                  VisibilityFlags fl = Visibility::Default);
    ColorGradient(const ColorGradient&) = default;
    ColorGradient& operator=(const ColorGradient&) = default;
    const ColorGradientProfile& getProfile() const
    {
        return profile;
    }
    void setProfile(const ColorGradientProfile& pro);

    void set(float fMin, float fMax, std::size_t usCt, ColorBarStyle tS, VisibilityFlags fl);
    void setRange(float fMin, float fMax)
    {
        set(fMin, fMax, profile.ctColors, profile.tStyle, profile.visibility);
    }
    void getRange(float& rfMin, float& rfMax) const
    {
        rfMin = profile.fMin;
        rfMax = profile.fMax;
    }
    bool isOutOfRange(float fVal) const
    {
        return ((fVal < profile.fMin) || (fVal > profile.fMax));
    }
    std::size_t getCountColors() const
    {
        return profile.ctColors;
    }
    void setCountColors(std::size_t usCt)
    {
        set(profile.fMin, profile.fMax, usCt, profile.tStyle, profile.visibility);
    }
    void setStyle(ColorBarStyle tS)
    {
        set(profile.fMin, profile.fMax, profile.ctColors, tS, profile.visibility);
    }
    std::size_t getMinColors() const;
    ColorBarStyle getStyle() const
    {
        return profile.tStyle;
    }
    void setOutsideGrayed(bool value)
    {
        profile.visibility.setFlag(Visibility::Grayed, value);
    }
    bool isOutsideGrayed() const
    {
        return profile.visibility.testFlag(Visibility::Grayed);
    }
    void setOutsideInvisible(bool value)
    {
        profile.visibility.setFlag(Visibility::Invisible, value);
    }
    bool isOutsideInvisible() const
    {
        return profile.visibility.testFlag(Visibility::Invisible);
    }
    void setColorModel(std::size_t tModel);
    std::size_t getColorModelType() const
    {
        return profile.tColorModel;
    }
    inline const ColorModel& getColorModel() const;
    std::vector<std::string> getColorModelNames() const;
    float getMinValue() const
    {
        return profile.fMin;
    }
    float getMaxValue() const
    {
        return profile.fMax;
    }

    inline Base::Color getColor(float fVal) const;
    inline std::size_t getColorIndex(float fVal) const;

private:
    inline Base::Color _getColor(float fVal) const;

protected:
    void createStandardPacks();

protected:
    ColorGradientProfile profile;
    ColorField colorField1, colorField2;
    ColorModelPack currentModelPack;
    std::vector<ColorModelPack> modelPacks;

    void rebuild();
    void setColorModel();
};


class AppExport ColorLegend
{
public:
    ColorLegend();
    ColorLegend(const ColorLegend& rclCL) = default;
    virtual ~ColorLegend() = default;

    ColorLegend& operator=(const ColorLegend& rclCL) = default;
    bool operator==(const ColorLegend& rclCL) const;
    bool operator!=(const ColorLegend& rclCL) const
    {
        return !(*this == rclCL);
    }

    void resize(std::size_t ulN);
    std::size_t addMin(const std::string& rclName);
    std::size_t addMax(const std::string& rclName);
    bool remove(std::size_t ulPos);
    void removeFirst();
    void removeLast();

    Base::Color getColor(std::size_t ulPos) const;
    uint32_t getPackedColor(std::size_t ulPos) const;
    bool setColor(std::size_t ulPos, float ucRed, float ucGreen, float ucBlue);
    bool setColor(std::size_t ulPos, unsigned long ulColor);
    float getValue(std::size_t ulPos) const;
    bool setValue(std::size_t ulPos, float fVal);
    std::string getText(std::size_t ulPos) const;
    bool setText(std::size_t ulPos, const std::string& rclName);
    std::size_t hasNumberOfFields() const
    {
        return colorFields.size();
    }
    void setOutsideGrayed(bool bOS)
    {
        outsideGrayed = bOS;
    }
    bool isOutsideGrayed() const
    {
        return outsideGrayed;
    }
    inline float getMinValue() const;
    inline float getMaxValue() const;

    inline Base::Color getColor(float fVal) const;
    inline std::size_t getColorIndex(float fVal) const;

protected:
    std::deque<Base::Color> colorFields;
    std::deque<std::string> names;
    std::deque<float> values;
    bool outsideGrayed {false};
};

inline Base::Color ColorLegend::getColor(float fVal) const
{
    std::deque<float>::const_iterator pI;
    for (pI = values.begin(); pI != values.end(); ++pI) {
        if (fVal < *pI) {
            break;
        }
    }

    if (outsideGrayed) {
        if ((pI == values.begin()) || (pI == values.end())) {
            return Base::Color(0.5f, 0.5f, 0.5f);
        }
        else {
            return colorFields[pI - values.begin() - 1];
        }
    }

    if (pI == values.begin()) {
        return *colorFields.begin();
    }
    else if (pI == values.end()) {
        return *(colorFields.end() - 1);
    }
    else {
        return colorFields[pI - values.begin() - 1];
    }
}

inline std::size_t ColorLegend::getColorIndex(float fVal) const
{
    std::deque<float>::const_iterator pI;
    for (pI = values.begin(); pI != values.end(); ++pI) {
        if (fVal < *pI) {
            break;
        }
    }

    if (pI == values.begin()) {
        return 0;
    }
    else if (pI == values.end()) {
        return (std::size_t)(colorFields.size() - 1);
    }
    else {
        return pI - values.begin() - 1;
    }
}

inline float ColorLegend::getMinValue() const
{
    return values.front();
}

inline float ColorLegend::getMaxValue() const
{
    return values.back();
}

inline Base::Color ColorGradient::getColor(float fVal) const
{
    Base::Color Color = _getColor(fVal);
    if (isOutsideInvisible()) {
        if (isOutOfRange(fVal)) {
            Color.a = 0.2F;
        }
    }

    return Color;
}

inline Base::Color ColorGradient::_getColor(float fVal) const
{
    if (isOutsideGrayed()) {
        if (isOutOfRange(fVal)) {
            return Base::Color(0.5f, 0.5f, 0.5f);
        }
    }

    switch (profile.tStyle) {
        case ColorBarStyle::ZERO_BASED: {
            if ((profile.fMin < 0.0f) && (profile.fMax > 0.0f)) {
                if (fVal < 0.0f) {
                    return colorField1.getColor(fVal);
                }
                else {
                    return colorField2.getColor(fVal);
                }
            }
            else {
                return colorField1.getColor(fVal);
            }
        }

        default:
        case ColorBarStyle::FLOW: {
            return colorField1.getColor(fVal);
        }
    }
}

inline std::size_t ColorGradient::getColorIndex(float fVal) const
{
    switch (profile.tStyle) {
        case ColorBarStyle::ZERO_BASED: {
            if ((profile.fMin < 0.0f) && (profile.fMax > 0.0f)) {
                if (fVal < 0.0f) {
                    return colorField1.getColorIndex(fVal);
                }
                else {
                    return std::size_t(colorField1.getCountColors()
                                       + colorField2.getColorIndex(fVal));
                }
            }
            else {
                return colorField1.getColorIndex(fVal);
            }
        }

        default:
        case ColorBarStyle::FLOW: {
            return colorField1.getColorIndex(fVal);
        }
    }
}

inline const ColorModel& ColorGradient::getColorModel() const
{
    if (profile.tStyle == ColorBarStyle::ZERO_BASED) {
        if (profile.fMax <= 0.0f) {
            return currentModelPack.bottomModel;
        }
        else if (profile.fMin >= 0.0f) {
            return currentModelPack.topModel;
        }
        else {
            return currentModelPack.totalModel;
        }
    }

    return currentModelPack.totalModel;
}

}  // namespace App