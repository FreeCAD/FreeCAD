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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <cstdlib>
#endif

#include <Base/Exception.h>

#include "ColorModel.h"


using namespace App;


ColorModelPack ColorModelPack::createRedGreenBlue()
{
    ColorModelPack pack{ColorModelBlueGreenRed(),
                        ColorModelGreenYellowRed(),
                        ColorModelBlueCyanGreen(),
                        "Red-Yellow-Green-Cyan-Blue"};
    return pack;
}

ColorModelPack ColorModelPack::createBlueGreenRed()
{
    ColorModelPack pack{ColorModelRedGreenBlue(),
                        ColorModelGreenCyanBlue(),
                        ColorModelRedYellowGreen(),
                        "Blue-Cyan-Green-Yellow-Red"};
    return pack;
}

ColorModelPack ColorModelPack::createWhiteBlack()
{
    ColorModelPack pack{ColorModelBlackWhite(),
                        ColorModelGrayWhite(),
                        ColorModelBlackGray(),
                        "White-Black"};
    return pack;
}

ColorModelPack ColorModelPack::createBlackWhite()
{
    ColorModelPack pack{ColorModelWhiteBlack(),
                        ColorModelGrayBlack(),
                        ColorModelWhiteGray(),
                        "Black-White"};
    return pack;
}

ColorModelPack ColorModelPack::createRedWhiteBlue()
{
    ColorModelPack pack{ColorModelBlueWhiteRed(),
                        ColorModelWhiteRed(),
                        ColorModelBlueWhite(),
                        "Red-White-Blue"};
    return pack;
}

ColorField::ColorField ()
{
    set(ColorModelBlueGreenRed(), -1.0f, 1.0f, 13);
}

ColorField::ColorField (const ColorModel &rclModel, float fMin, float fMax, std::size_t usCt)
{
    set(rclModel, fMin, fMax, usCt);
}

ColorField& ColorField::operator = (const ColorField &rclCF)
{
    colorField = rclCF.colorField;
    return *this;
}

void ColorField::set (const ColorModel &rclModel, float fMin, float fMax, std::size_t usCt)
{
    auto bounds = std::minmax(fMin, fMax);
    if (bounds.second <= bounds.first) {
        throw Base::ValueError("Maximum must be higher than minimum");
    }

    this->fMin = bounds.first;
    this->fMax = bounds.second;
    colorModel = rclModel;
    ctColors = std::max<std::size_t>(usCt, colorModel.getCountColors());
    rebuild();
}

void ColorField::setColorModel (const ColorModel &rclModel)
{
    colorModel = rclModel;
    rebuild();
}

void ColorField::rebuild ()
{
    colorField.resize(ctColors);

    std::size_t usStep = std::min<std::size_t>(ctColors / (colorModel.getCountColors() - 1), ctColors - 1);
    std::size_t usInd1 = 0;
    std::size_t usInd2 = usStep;
    for (std::size_t i = 0; i < (colorModel.getCountColors() - 1); i++) {
        interpolate(colorModel.colors[i], usInd1, colorModel.colors[i+1], usInd2);
        usInd1 = usInd2;
        if ((i + 1) == (colorModel.getCountColors() - 2))
            usInd2 = ctColors - 1;
        else
            usInd2 += usStep;
    }

    fAscent   = float(ctColors) / (fMax - fMin);
    fConstant = -fAscent * fMin;
}

// fuellt das Array von Farbe 1, Index 1 bis Farbe 2, Index 2
void ColorField::interpolate (Color clCol1, std::size_t usInd1, Color clCol2, std::size_t usInd2)
{
    float fStep = 1.0f, fLen = float(usInd2 - usInd1);

    colorField[usInd1] = clCol1;
    colorField[usInd2] = clCol2;

    float fR = (float(clCol2.r) - float(clCol1.r)) / fLen;
    float fG = (float(clCol2.g) - float(clCol1.g)) / fLen;
    float fB = (float(clCol2.b) - float(clCol1.b)) / fLen;

    for (std::size_t i = (usInd1 + 1); i < usInd2; i++) {
        float ucR = clCol1.r + fR * fStep;
        float ucG = clCol1.g + fG * fStep;
        float ucB = clCol1.b + fB * fStep;
        colorField[i] = Color(ucR, ucG, ucB);
        fStep += 1.0f;
    }
}


ColorGradientProfile::ColorGradientProfile() = default;

bool ColorGradientProfile::isEqual(const ColorGradientProfile& cg) const
{
    if (tStyle != cg.tStyle)
        return false;
    if (fMin != cg.fMin)
        return false;
    if (fMax != cg.fMax)
        return false;
    if (!visibility.isEqual(cg.visibility))
        return false;
    if (tColorModel != cg.tColorModel)
        return false;
    return true;
}


ColorGradient::ColorGradient ()
{
    createStandardPacks();
    setColorModel();
    set(-1.0f, 1.0f, 13, ColorBarStyle::ZERO_BASED, Visibility::Default);
}

ColorGradient::ColorGradient (float fMin, float fMax, std::size_t usCtColors, ColorBarStyle tS, VisibilityFlags flags)
{
    createStandardPacks();
    setColorModel();
    set(fMin, fMax, usCtColors, tS, flags);
}

void ColorGradient::createStandardPacks()
{
    modelPacks.push_back(ColorModelPack::createRedGreenBlue());
    modelPacks.push_back(ColorModelPack::createBlueGreenRed());
    modelPacks.push_back(ColorModelPack::createRedWhiteBlue());
    modelPacks.push_back(ColorModelPack::createWhiteBlack());
    modelPacks.push_back(ColorModelPack::createBlackWhite());
}

std::vector<std::string> ColorGradient::getColorModelNames() const
{
    std::vector<std::string> names;
    names.reserve(modelPacks.size());
    for (const auto& it : modelPacks)
        names.push_back(it.description);
    return names;
}

void ColorGradient::setProfile(const ColorGradientProfile& pro)
{
    profile = pro;
    setColorModel();
    rebuild();
}

void ColorGradient::set (float fMin, float fMax, std::size_t usCt, ColorBarStyle tS, VisibilityFlags flags)
{
    auto bounds = std::minmax(fMin, fMax);
    if (bounds.second <= bounds.first) {
        throw Base::ValueError("Maximum must be higher than minimum");
    }

    profile.fMin = bounds.first;
    profile.fMax = bounds.second;
    profile.ctColors = std::max<std::size_t>(usCt, getMinColors());
    profile.tStyle = tS;
    profile.visibility = flags;
    rebuild();
}

void ColorGradient::rebuild ()
{
    switch (profile.tStyle)
    {
    case ColorBarStyle::FLOW:
        {
            colorField1.set(currentModelPack.totalModel, profile.fMin, profile.fMax, profile.ctColors);
            break;
        }
    case ColorBarStyle::ZERO_BASED:
        {
            if ((profile.fMin < 0.0f) && (profile.fMax > 0.0f))
            {
                colorField1.set(currentModelPack.bottomModel, profile.fMin, 0.0f, profile.ctColors / 2);
                colorField2.set(currentModelPack.topModel, 0.0f, profile.fMax, profile.ctColors / 2);
            }
            else if (profile.fMin >= 0.0f)
                colorField1.set(currentModelPack.topModel, 0.0f, profile.fMax, profile.ctColors);
            else
                colorField1.set(currentModelPack.bottomModel, profile.fMin, 0.0f, profile.ctColors);
            break;
        }
    }
}

std::size_t ColorGradient::getMinColors () const
{
    switch (profile.tStyle)
    {
    case ColorBarStyle::FLOW:
        return colorField1.getMinColors();
    case ColorBarStyle::ZERO_BASED:
        {
            if ((profile.fMin < 0.0f) && (profile.fMax > 0.0f))
                return colorField1.getMinColors() + colorField2.getMinColors();
            else
                return colorField1.getMinColors();
        }
    }
    return 2;
}

void ColorGradient::setColorModel (std::size_t tModel)
{
    profile.tColorModel = tModel;
    setColorModel();
    rebuild();
}

void ColorGradient::setColorModel ()
{
    if (profile.tColorModel < modelPacks.size())
        currentModelPack = modelPacks[profile.tColorModel];

    switch (profile.tStyle)
    {
    case ColorBarStyle::FLOW:
    {
        colorField1.setColorModel(currentModelPack.totalModel);
        colorField2.setColorModel(currentModelPack.bottomModel);
        break;
    }
    case ColorBarStyle::ZERO_BASED:
    {
        colorField1.setColorModel(currentModelPack.topModel);
        colorField2.setColorModel(currentModelPack.bottomModel);
        break;
    }
    }
}

ColorLegend::ColorLegend()
{
    // default  blue, green, red
    colorFields.emplace_back(0, 0, 1);
    colorFields.emplace_back(0, 1, 0);
    colorFields.emplace_back(1, 0, 0);

    names.emplace_back("Min");
    names.emplace_back("Mid");
    names.emplace_back("Max");

    values.push_back(-1.0f);
    values.push_back(-0.333f);
    values.push_back(0.333f);
    values.push_back(1.0f);
}

bool ColorLegend::operator == (const ColorLegend &rclCL) const
{
  return (colorFields.size() == rclCL.colorFields.size())                                 &&
         (names.size() == rclCL.names.size())                                             &&
         (values.size() == rclCL.values.size())                                           &&
          std::equal(colorFields.begin(), colorFields.end(), rclCL.colorFields.begin())   &&
          std::equal(names.begin(), names.end(), rclCL.names.begin())                     &&
          std::equal(values.begin(), values.end(), rclCL.values.begin())                  &&
          outsideGrayed == rclCL.outsideGrayed;
}

float ColorLegend::getValue (std::size_t ulPos) const
{
    if (ulPos < values.size())
        return values[ulPos];
    else
        return 0.0f;
}

bool ColorLegend::setValue (std::size_t ulPos, float fVal)
{
    if (ulPos < values.size())
    {
        values[ulPos] = fVal;
        return true;
    }
    else
        return false;
}

Color ColorLegend::getColor (std::size_t ulPos) const
{
    if (ulPos < colorFields.size())
        return colorFields[ulPos];
    else
        return Color();
}

// color as: 0x00rrggbb
uint32_t ColorLegend::getPackedColor (std::size_t ulPos) const
{
    Color clRGB = getColor(ulPos);
    return clRGB.getPackedValue();
}

std::string ColorLegend::getText (std::size_t ulPos) const
{
    if (ulPos < names.size())
        return names[ulPos];
    else
        return "";
}

std::size_t ColorLegend::addMin (const std::string &rclName)
{
    names.push_front(rclName);
    values.push_front(values.front() - 1.0f);

    Color clNewRGB;
    clNewRGB.r = ((float)rand() / (float)RAND_MAX);
    clNewRGB.g = ((float)rand() / (float)RAND_MAX);
    clNewRGB.b = ((float)rand() / (float)RAND_MAX);

    colorFields.push_front(clNewRGB);

    return colorFields.size() - 1;
}

std::size_t ColorLegend::addMax (const std::string &rclName)
{
    names.push_back(rclName);
    values.push_back(values.back() + 1.0f);

    Color clNewRGB;
    clNewRGB.r = ((float)rand() / (float)RAND_MAX);
    clNewRGB.g = ((float)rand() / (float)RAND_MAX);
    clNewRGB.b = ((float)rand() / (float)RAND_MAX);

    colorFields.push_back(clNewRGB);

    return colorFields.size() - 1;
}

bool ColorLegend::remove (std::size_t ulPos)
{
    if (ulPos < colorFields.size()) {
        colorFields.erase(colorFields.begin() + ulPos);
        names.erase(names.begin() + ulPos);
        values.erase(values.begin() + ulPos);

        return true;
    }

    return false;
}

void ColorLegend::removeFirst ()
{
    if (!colorFields.empty()) {
        colorFields.erase(colorFields.begin());
        names.erase(names.begin());
        values.erase(values.begin());
    }
}

void ColorLegend::removeLast ()
{
    if (!colorFields.empty()) {
        colorFields.erase(colorFields.end()-1);
        names.erase(names.end()-1);
        values.erase(values.end()-1);
    }
}

void ColorLegend::resize (std::size_t ulCt)
{
    if ((ulCt < 2) || (ulCt == colorFields.size()))
        return;

    if (ulCt > colorFields.size()) {
        int k = ulCt - colorFields.size();
        for (int i = 0; i < k; i++)
            addMin("new");
    }
    else {
        int k = colorFields.size() - ulCt;
        for (int i = 0; i < k; i++)
            removeLast();
    }
}

bool ColorLegend::setColor (std::size_t ulPos, float ucRed, float ucGreen, float ucBlue)
{
    if (ulPos < names.size()) {
        colorFields[ulPos] = Color(ucRed, ucGreen, ucBlue);
        return true;
    }

    return false;
}

// color as 0x00rrggbb
bool ColorLegend::setColor (std::size_t ulPos, unsigned long ulColor)
{
    unsigned char ucRed   = (unsigned char)((ulColor & 0x00ff0000) >> 16);
    unsigned char ucGreen = (unsigned char)((ulColor & 0x0000ff00) >> 8);
    unsigned char ucBlue  = (unsigned char)(ulColor & 0x000000ff);
    return setColor(ulPos, ucRed, ucGreen, ucBlue);
}

bool ColorLegend::setText (std::size_t ulPos, const std::string &rclName)
{
    if (ulPos < names.size()) {
        names[ulPos] = rclName;
        return true;
    }

    return false;
}
