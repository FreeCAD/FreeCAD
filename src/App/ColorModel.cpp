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

#include "ColorModel.h"

using namespace App;


ColorModel::ColorModel (unsigned short usCt)
  : _usColors(usCt), _pclColors(0)
{
    if (usCt > 0)
        _pclColors = new Color[usCt];
}

ColorModel::ColorModel (const ColorModel &rclM)
  : _pclColors(0)
{
    *this = rclM;
}

ColorModel::~ColorModel ()
{
    delete [] _pclColors;
}

ColorModel& ColorModel::operator = (const ColorModel &rclM)
{
    // first check if both objects are identical
    if (this->_pclColors && this->_pclColors == rclM._pclColors)
        return *this;

    delete [] _pclColors;

    _usColors = rclM._usColors;
    if (_usColors == 0)
        return *this; 

    _pclColors = new Color[rclM._usColors];
    for (int i = 0; i < rclM._usColors; i++)
        _pclColors[i] = rclM._pclColors[i]; 

    return *this;
}

ColorField::ColorField (void)
  : _clModel(ColorModelTria())
{
    set(ColorModelTria(), -1.0f, 1.0f, 13);
}

ColorField::ColorField (const ColorModel &rclModel, float fMin, float fMax, unsigned short usCt)
: _clModel(ColorModelTria())
{
    set(rclModel, fMin, fMax, usCt);
}

ColorField::~ColorField ()
{
}    

ColorField::ColorField (const ColorField &rclCF)
  : _clModel(ColorModelTria())
{
    *this = rclCF;
}

ColorField& ColorField::operator = (const ColorField &rclCF)
{
    _aclField = rclCF._aclField;
    return *this;
}

void ColorField::set (const ColorModel &rclModel, float fMin, float fMax, unsigned short usCt)
{
    _clModel = rclModel;
    _fMin = std::min<float>(fMin, fMax);
    _fMax = std::max<float>(_fMin + CCR_EPS, fMax);
    _usCtColors = std::max<unsigned short>(usCt, _clModel._usColors);
    rebuild();
}

void ColorField::setColorModel (const ColorModel &rclModel)
{
    _clModel = rclModel;
    rebuild();
}

void ColorField::rebuild (void)
{
    unsigned short usInd1, usInd2, usStep, i;

    _aclField.resize(_usCtColors);


    usStep = std::min<unsigned short>(_usCtColors / (_clModel._usColors-1), _usCtColors-1);
    usInd1 = 0;
    usInd2 = usStep;
    for (i = 0; i < (_clModel._usColors - 1); i++) {
        interpolate(_clModel._pclColors[i], usInd1, _clModel._pclColors[i+1], usInd2);
        usInd1 = usInd2;
        if ((i + 1) == (_clModel._usColors - 2))
            usInd2 = _usCtColors - 1;
        else
            usInd2 += usStep;
    }

    _fAscent   = float(_usCtColors) / (_fMax - _fMin);
    _fConstant = -_fAscent * _fMin;
}

// fuellt das Array von Farbe 1, Index 1 bis Farbe 2, Index 2
void ColorField::interpolate (Color clCol1, unsigned short usInd1, Color clCol2, unsigned short usInd2)
{
    unsigned short i;
    float ucR, ucG, ucB;
    float fR, fG, fB, fStep = 1.0f, fLen = float(usInd2 - usInd1);

    _aclField[usInd1] = clCol1;
    _aclField[usInd2] = clCol2;

    fR = (float(clCol2.r) - float(clCol1.r)) / fLen;
    fG = (float(clCol2.g) - float(clCol1.g)) / fLen;
    fB = (float(clCol2.b) - float(clCol1.b)) / fLen;

    for (i = (usInd1 + 1); i < usInd2; i++) {
        ucR = clCol1.r + fR * fStep;
        ucG = clCol1.g + fG * fStep;
        ucB = clCol1.b + fB * fStep;
        _aclField[i] = Color(ucR, ucG, ucB);
        fStep += 1.0f;
    }
}


ColorGradient::ColorGradient (void)
:  _tColorModel(TRIA),
   _bOutsideGrayed(false),
   _clTotal(ColorModelTria()),
   _clTop(ColorModelTriaTop()),
   _clBottom(ColorModelTriaBottom())
{
    setColorModel();
    set(-1.0f, 1.0f, 13, ZERO_BASED, false);
}

ColorGradient::ColorGradient (float fMin, float fMax, unsigned short usCtColors, TStyle tS, bool bOG)
:  _tColorModel(TRIA),
   _bOutsideGrayed(false),
   _clTotal(ColorModelTria()),
   _clTop(ColorModelTriaTop()),
   _clBottom(ColorModelTriaBottom())
{
    setColorModel();
    set(fMin, fMax, usCtColors, tS, bOG);
}

ColorGradient::ColorGradient (const ColorGradient &rclCR)
:  _tColorModel(TRIA),
   _clTotal(ColorModelTria()),
   _clTop(ColorModelTriaTop()),
   _clBottom(ColorModelTriaBottom())
{
    *this = rclCR;
}

ColorGradient& ColorGradient::operator = (const ColorGradient &rclCR)
{
    _tColorModel    = rclCR._tColorModel;
    _clTotal        = rclCR._clTotal;
    _clTop          = rclCR._clTop;
    _clBottom       = rclCR._clBottom;
    _bOutsideGrayed = rclCR._bOutsideGrayed;
    _clColFld1      = rclCR._clColFld1;
    _clColFld2      = rclCR._clColFld2;
    _tStyle         = rclCR._tStyle;
    _fMin           = rclCR._fMin;
    _fMax           = rclCR._fMax;
    _usCtColors     = rclCR._usCtColors;

    return *this;
}

void ColorGradient::set (float fMin, float fMax, unsigned short usCt, TStyle tS, bool bOG)
{
    _fMin = std::min<float>(fMin, fMax);
    _fMax = std::max<float>(_fMin + CCR_EPS, fMax);
    _usCtColors = std::max<unsigned short>(usCt, getMinColors());
    _tStyle = tS;
    _bOutsideGrayed = bOG;
    rebuild();
}

void ColorGradient::rebuild (void)
{
    switch (_tStyle)
    {
        case FLOW:
        {
            _clColFld1.set(_clTotal, _fMin, _fMax, _usCtColors);
            break;
        }
        case ZERO_BASED:
        {
            if ((_fMin < 0.0f) && (_fMax > 0.0f))
            {
                _clColFld1.set(_clBottom, _fMin, 0.0f, _usCtColors / 2);
                _clColFld2.set(_clTop, 0.0f, _fMax, _usCtColors / 2);
            }
            else if (_fMin >= 0.0f)
                _clColFld1.set(_clTop, 0.0f, _fMax, _usCtColors);
            else
                _clColFld1.set(_clBottom, _fMin, 0.0f, _usCtColors);
            break;
        }
    }
}

unsigned short ColorGradient::getMinColors (void) const
{
    switch (_tStyle)
    {
        case FLOW:   
            return _clColFld1.getMinColors();
        case ZERO_BASED: 
        {
            if ((_fMin < 0.0f) && (_fMax > 0.0f))
                return _clColFld1.getMinColors() + _clColFld2.getMinColors();
            else
                return _clColFld1.getMinColors();
        }
    }
    return 2;
}

void ColorGradient::setColorModel (TColorModel tModel)
{
    _tColorModel = tModel;
    setColorModel();
    rebuild();
}

void ColorGradient::setColorModel (void)
{
    switch (_tColorModel)
    {
        case TRIA:
        {
            _clTotal  = ColorModelTria(); 
            _clTop    = ColorModelTriaTop();
            _clBottom = ColorModelTriaBottom();
            break;
        }
        case INVERSE_TRIA:
        {
            _clTotal  = ColorModelInverseTria();
            _clTop    = ColorModelInverseTriaTop();
            _clBottom = ColorModelInverseTriaBottom();
            break;
        }
        case GRAY:
        {
            _clTotal  = ColorModelGray();
            _clTop    = ColorModelGrayTop();
            _clBottom = ColorModelGrayBottom();
            break;
        }
        case INVERSE_GRAY:
        {
            _clTotal  = ColorModelInverseGray();
            _clTop    = ColorModelInverseGrayTop();
            _clBottom = ColorModelInverseGrayBottom();
            break;
        }
    }

    switch (_tStyle)
    {
        case FLOW:   
        {
            _clColFld1.setColorModel(_clTotal);
            _clColFld2.setColorModel(_clBottom);
            break;
        }
        case ZERO_BASED: 
        {
            _clColFld1.setColorModel(_clTop);
            _clColFld2.setColorModel(_clBottom);
            break;
        }
    }
}

ColorLegend::ColorLegend (void)
: _bOutsideGrayed(false)
{
    // default  green, red
    _aclColorFields.push_back(Color(0, 1, 0));
    _aclColorFields.push_back(Color(1, 0, 0));

    _aclNames.push_back("Min");
    _aclNames.push_back("Max");

    _aclValues.push_back(-1.0f);
    _aclValues.push_back(0.0f);
    _aclValues.push_back(1.0f);
}

ColorLegend::ColorLegend (const ColorLegend &rclCL)
{
    *this = rclCL;
}

ColorLegend& ColorLegend::operator = (const ColorLegend &rclCL)
{
    _aclColorFields = rclCL._aclColorFields;
    _aclNames       = rclCL._aclNames;
    _aclValues      = rclCL._aclValues;
    _bOutsideGrayed = rclCL._bOutsideGrayed;

    return *this;
}

bool ColorLegend::operator == (const ColorLegend &rclCL) const
{
  return (_aclColorFields.size() == rclCL._aclColorFields.size())                                     &&
         (_aclNames.size() == rclCL._aclNames.size())                                                 &&
         (_aclValues.size() == rclCL._aclValues.size())                                               &&
          std::equal(_aclColorFields.begin(), _aclColorFields.end(), rclCL._aclColorFields.begin())   &&
          std::equal(_aclNames.begin(), _aclNames.end(), rclCL._aclNames.begin())                     &&
          std::equal(_aclValues.begin(), _aclValues.end(), rclCL._aclValues.begin())                  &&
          _bOutsideGrayed == rclCL._bOutsideGrayed;
}

float ColorLegend::getValue (unsigned long ulPos) const
{
    if (ulPos < _aclValues.size())
        return _aclValues[ulPos];
    else
        return 0.0f;
}

bool ColorLegend::setValue (unsigned long ulPos, float fVal)
{
    if (ulPos < _aclValues.size())
    {
        _aclValues[ulPos] = fVal;
        return true;
    }
    else
        return false;
}

Color ColorLegend::getColor (unsigned long ulPos) const
{
    if (ulPos < _aclColorFields.size())
        return _aclColorFields[ulPos];
    else
        return Color();
}

// color as: 0x00rrggbb
uint32_t ColorLegend::getPackedColor (unsigned long ulPos) const
{
    Color clRGB = getColor(ulPos);
    return clRGB.getPackedValue();
}

std::string ColorLegend::getText (unsigned long ulPos) const
{
    if (ulPos < _aclNames.size())
        return _aclNames[ulPos];
    else
        return "";
}

bool ColorLegend::addMin (const std::string &rclName)
{
    _aclNames.push_front(rclName);
    _aclValues.push_front(*_aclValues.begin() - 1.0f);

    Color clNewRGB;
    clNewRGB.r = ((float)rand() / (float)RAND_MAX);
    clNewRGB.g = ((float)rand() / (float)RAND_MAX);
    clNewRGB.b = ((float)rand() / (float)RAND_MAX);

    _aclColorFields.push_front(clNewRGB);

    return true;
}

bool ColorLegend::addMax (const std::string &rclName)
{
    _aclNames.push_back(rclName);
    _aclValues.push_back(*(_aclValues.end()-1) + 1.0f);

    Color clNewRGB;
    clNewRGB.r = ((float)rand() / (float)RAND_MAX);
    clNewRGB.g = ((float)rand() / (float)RAND_MAX);
    clNewRGB.b = ((float)rand() / (float)RAND_MAX);

    _aclColorFields.push_back(clNewRGB);

    return true;
}

bool ColorLegend::remove (unsigned long ulPos)
{
    if (ulPos < _aclColorFields.size())
    {
        _aclColorFields.erase(_aclColorFields.begin() + ulPos);
        _aclNames.erase(_aclNames.begin() + ulPos);
        _aclValues.erase(_aclValues.begin() + ulPos);

        return true;
    }

    return false;
}

void ColorLegend::removeFirst (void)
{
    if (_aclColorFields.size() > 0)
    {
        _aclColorFields.erase(_aclColorFields.begin());
        _aclNames.erase(_aclNames.begin());
        _aclValues.erase(_aclValues.begin()); 
    }
}

void ColorLegend::removeLast (void)
{
    if (_aclColorFields.size() > 0)
    {
        _aclColorFields.erase(_aclColorFields.end()-1);
        _aclNames.erase(_aclNames.end()-1);
        _aclValues.erase(_aclValues.end()-1); 
    }
}

void ColorLegend::resize (unsigned long ulCt)
{
    if ((ulCt < 2) || (ulCt == _aclColorFields.size()))
        return;

    if (ulCt > _aclColorFields.size())
    {
        int k = ulCt - _aclColorFields.size();
        for (int i = 0; i < k; i++)
            addMin("new");
    }
    else
    {
        int k = _aclColorFields.size() - ulCt;
        for (int i = 0; i < k; i++)
            removeLast();
    }
}

bool ColorLegend::setColor (unsigned long ulPos, float ucRed, float ucGreen, float ucBlue)
{
    if (ulPos < _aclNames.size())
    {
        _aclColorFields[ulPos] = Color(ucRed, ucGreen, ucBlue);
        return true;
    }
    else
        return false;
}

// color as 0x00rrggbb
bool ColorLegend::setColor (unsigned long ulPos, unsigned long ulColor)
{
    unsigned char ucRed   = (unsigned char)((ulColor & 0x00ff0000) >> 16);
    unsigned char ucGreen = (unsigned char)((ulColor & 0x0000ff00) >> 8);
    unsigned char ucBlue  = (unsigned char)(ulColor & 0x000000ff);
    return setColor(ulPos, ucRed, ucGreen, ucBlue);
}

bool ColorLegend::setText (unsigned long ulPos, const std::string &rclName)
{
    if (ulPos < _aclNames.size())
    {
        _aclNames[ulPos] = rclName;
        return true;
    }
    else
        return false;
}
