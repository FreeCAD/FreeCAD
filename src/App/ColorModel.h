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


#ifndef APP_COLORMODEL_H
#define APP_COLORMODEL_H

#include "Material.h"

#include <algorithm>
#include <deque>
#include <string>
#include <vector>

#define CCR_EPS  1.0e-5f

namespace App
{

/**
 * Abstract base class that calculates the matching RGB color to a given value.
 */
class AppExport ValueFloatToRGB
{
public:
  virtual Color getColor(float fVal) const = 0;

protected:
  ValueFloatToRGB (void) {}
  virtual ~ValueFloatToRGB () {}
};


class AppExport ColorModel
{
public:
  ColorModel (unsigned short usCt);
  ColorModel (const ColorModel &rclM);
  virtual ~ColorModel ();
  ColorModel& operator = (const ColorModel &rclM);
  unsigned short  _usColors;
  Color  *_pclColors;
};

class AppExport ColorModelTria : public ColorModel
{
public:
  ColorModelTria (void) : ColorModel(5)
  {
    _pclColors[0] = Color( 0, 0, 1);
    _pclColors[1] = Color( 0, 1, 1);
    _pclColors[2] = Color( 0, 1, 0);
    _pclColors[3] = Color( 1, 1, 0);
    _pclColors[4] = Color( 1, 0, 0);
  }
};

class AppExport ColorModelTriaBottom : public ColorModel
{
public:
  ColorModelTriaBottom (void) : ColorModel(3)
  {
    _pclColors[0] = Color( 0, 0, 1);
    _pclColors[1] = Color( 0, 1, 1);
    _pclColors[2] = Color( 0, 1, 0);
  }
};

class AppExport ColorModelTriaTop : public ColorModel
{
public:
  ColorModelTriaTop (void) : ColorModel(3)
  {
    _pclColors[0] = Color( 0, 1, 0);
    _pclColors[1] = Color( 1, 1, 0);
    _pclColors[2] = Color( 1, 0, 0);
  }
};

class AppExport ColorModelInverseTria : public ColorModel
{
public:
  ColorModelInverseTria (void) : ColorModel(5)
  {
    _pclColors[0] = Color( 1, 0, 0);
    _pclColors[1] = Color( 1, 1, 0);
    _pclColors[2] = Color( 0, 1, 0);
    _pclColors[3] = Color( 0, 1, 1);
    _pclColors[4] = Color( 0, 0, 1);
  }
};

class AppExport ColorModelInverseTriaTop : public ColorModel
{
public:
  ColorModelInverseTriaTop (void) : ColorModel(3)
  {
    _pclColors[2] = Color( 0, 0, 1);
    _pclColors[1] = Color( 0, 1, 1);
    _pclColors[0] = Color( 0, 1, 0);
  }
};

class AppExport ColorModelInverseTriaBottom : public ColorModel
{
public:
  ColorModelInverseTriaBottom (void) : ColorModel(3)
  {
    _pclColors[2] = Color( 0, 1, 0);
    _pclColors[1] = Color( 1, 1, 0);
    _pclColors[0] = Color( 1, 0, 0);
  }
};

class AppExport ColorModelGray : public ColorModel
{
public:
  ColorModelGray (void) : ColorModel(2)
  {
    _pclColors[0] = Color( 0, 0, 0);
    _pclColors[1] = Color( 1, 1, 1);
  }
};

class AppExport ColorModelGrayBottom : public ColorModel
{
public:
  ColorModelGrayBottom (void) : ColorModel(2)
  {
    _pclColors[0] = Color( 0.0f, 0.0f, 0.0f);
    _pclColors[1] = Color( 0.5f, 0.5f, 0.5f);
  }
};

class AppExport ColorModelGrayTop : public ColorModel
{
public:
  ColorModelGrayTop (void) : ColorModel(2)
  {
    _pclColors[0] = Color( 0.5f, 0.5f, 0.5f);
    _pclColors[1] = Color( 1.0f, 1.0f, 1.0f);
  }
};

class AppExport ColorModelInverseGray : public ColorModel
{
public:
  ColorModelInverseGray (void) : ColorModel(2)
  {
    _pclColors[0] = Color( 1, 1, 1);
    _pclColors[1] = Color( 0, 0, 0);
  }
};

class AppExport ColorModelInverseGrayBottom : public ColorModel
{
public:
  ColorModelInverseGrayBottom (void) : ColorModel(2)
  {
    _pclColors[0] = Color( 1.0f, 1.0f, 1.0f);
    _pclColors[1] = Color( 0.5f, 0.5f, 0.5f);
  }
};

class AppExport ColorModelInverseGrayTop : public ColorModel
{
public:
  ColorModelInverseGrayTop (void) : ColorModel(2)
  {
    _pclColors[0] = Color( 0.5f, 0.5f, 0.5f);
    _pclColors[1] = Color( 0.0f, 0.0f, 0.0f);
  }
};

class AppExport ColorField
{
public:
  ColorField (void);
  ColorField (const ColorField &rclCF);
  ColorField (const ColorModel &rclModel, float fMin, float fMax, unsigned short usCt);
  virtual ~ColorField ();

  ColorField& operator = (const ColorField &rclCF);

  unsigned short getCountColors (void) const { return _usCtColors; }
  void set (const ColorModel &rclModel, float fMin, float fMax, unsigned short usCt);
  void setCountColors (unsigned short usCt) { set(_clModel, _fMin, _fMax, usCt); }
  void setRange (float fMin, float fMax) { set(_clModel, fMin, fMax, _usCtColors); }
  void getRange (float &rfMin, float &rfMax) { rfMin = _fMin; rfMax = _fMax; }
  unsigned short getMinColors (void) const { return _clModel._usColors; }
  void setColorModel (const ColorModel &rclModel);
  const ColorModel& getColorModel (void) const { return _clModel; }
  void setDirect (unsigned short usInd, Color clCol) { _aclField[usInd] = clCol; }
  float getMinValue (void) const { return _fMin; }
  float getMaxValue (void) const { return _fMax; }

  Color getColor (unsigned short usIndex) const { return _aclField[usIndex]; }
  inline Color  getColor (float fVal) const;
  inline unsigned short getColorIndex (float fVal) const;

protected:
  ColorModel          _clModel;
  float               _fMin, _fMax;
  float               _fAscent, _fConstant;  // Index := _fConstant + _fAscent * WERT
  unsigned short      _usCtColors;
  std::vector<Color>  _aclField;

  void rebuild (void);
  void interpolate (Color clCol1, unsigned short usPos1, Color clCol2, unsigned short usPos2);
};

inline Color ColorField::getColor (float fVal) const
{
#if 1
  // if the value is outside or at the border of the range
  unsigned short ct = _clModel._usColors-1;
  if ( fVal <= _fMin )
    return _clModel._pclColors[0];
  else if ( fVal >= _fMax )
    return _clModel._pclColors[ct];

  // get the color field position (with 0 < t < 1)
  float t = (fVal-_fMin) / (_fMax-_fMin);
  Color col(1.0f, 1.0f, 1.0f); // white as default
  for ( unsigned short i=0; i<ct; i++ )
  {
    float r = (float)(i+1)/(float)ct;
    if ( t < r )
    {
      // calculate the exact position in the subrange
      float s = t*(float)ct-(float)i;
      Color c1 = _clModel._pclColors[i];
      Color c2 = _clModel._pclColors[i+1];
      col.r = (1.0f-s) * c1.r + s * c2.r;
      col.g = (1.0f-s) * c1.g + s * c2.g;
      col.b = (1.0f-s) * c1.b + s * c2.b;
      break;
    }
  }

  return col;
#else
  return _aclField[getColorIndex(fVal)];
#endif
}

inline unsigned short ColorField::getColorIndex (float fVal) const
{
  return (unsigned short)(std::min<int>(std::max<int>(int(_fConstant + _fAscent * fVal), 0), int(_usCtColors-1)));
}


class AppExport ColorGradient
{
public:
  enum TStyle { FLOW, ZERO_BASED };
  enum TColorModel { TRIA, INVERSE_TRIA, GRAY, INVERSE_GRAY };

  ColorGradient (void);
  ColorGradient (float fMin, float fMax, unsigned short usCtColors, TStyle tS, bool bOG = false);
  ColorGradient (const ColorGradient &rclCR);

  ColorGradient& operator = (const ColorGradient &rclCR);

  void set (float fMin, float fMax, unsigned short usCt, TStyle tS, bool bOG);
  void setRange (float fMin, float fMax) { set(fMin, fMax, _usCtColors, _tStyle, _bOutsideGrayed); }
  void getRange (float &rfMin, float &rfMax) const { rfMin = _fMin; rfMax = _fMax; }
  unsigned short getCountColors (void) const { return _usCtColors; }
  void setCountColors (unsigned short usCt) { set(_fMin, _fMax, usCt, _tStyle, _bOutsideGrayed); }
  void setStyle (TStyle tS) { set(_fMin, _fMax, _usCtColors, tS, _bOutsideGrayed); }
  unsigned short getMinColors (void) const;
  TStyle getStyle (void) const { return _tStyle; }
  void setOutsideGrayed (bool bGrayed) { _bOutsideGrayed = bGrayed; }
  bool isOutsideGrayed (void) const { return _bOutsideGrayed; }
  void setColorModel (TColorModel tModel);
  TColorModel getColorModelType (void) const { return _tColorModel; }
  inline const ColorModel& getColorModel (void) const;
  float getMinValue (void) const { return _fMin; }
  float getMaxValue (void) const { return _fMax; }

  inline Color  getColor (float fVal) const;
  inline unsigned short getColorIndex (float fVal) const;

protected:
  ColorField  _clColFld1, _clColFld2;
  TColorModel    _tColorModel;
  TStyle         _tStyle;
  float          _fMin, _fMax;
  unsigned short        _usCtColors;
  bool           _bOutsideGrayed;
  ColorModel       _clTotal, _clTop, _clBottom;

  void  rebuild (void);
  void  setColorModel (void);
};


class AppExport ColorLegend
{
public:
  ColorLegend (void);
  ColorLegend (const ColorLegend &rclCL);
  virtual ~ColorLegend () {}

  ColorLegend& operator = (const ColorLegend &rclCL);
  bool operator == (const ColorLegend &rclCL) const;
  bool operator != (const ColorLegend &rclCL) const { return !(*this == rclCL); }

  void resize (unsigned long ulN);
  bool addMin (const std::string &rclName);
  bool addMax (const std::string &rclName);
  bool remove (unsigned long ulPos);
  void removeFirst (void);
  void removeLast (void);

  Color getColor (unsigned long ulPos) const;
  uint32_t  getPackedColor (unsigned long ulPos) const;
  bool setColor (unsigned long ulPos, float ucRed, float ucGreen, float ucBlue);
  bool setColor (unsigned long ulPos, unsigned long ulColor);
  float getValue (unsigned long ulPos) const;
  bool setValue (unsigned long ulPos, float fVal);
  std::string getText (unsigned long ulPos) const;
  bool setText (unsigned long ulPos, const std::string &rclName);
  unsigned long hasNumberOfFields (void) const { return (unsigned long)_aclColorFields.size(); }
  void setOutsideGrayed (bool bOS) { _bOutsideGrayed = bOS; }
  bool isOutsideGrayed (void) const { return _bOutsideGrayed; }
  inline float  getMinValue (void) const;
  inline float  getMaxValue (void) const;

  inline Color getColor (float fVal) const;
  inline unsigned short getColorIndex (float fVal) const;

protected:
  std::deque<Color>  _aclColorFields;
  std::deque<std::string> _aclNames;
  std::deque<float> _aclValues;
  bool _bOutsideGrayed;
};

inline Color ColorLegend::getColor (float fVal) const
{
  std::deque<float>::const_iterator pI;
  for (pI = _aclValues.begin(); pI != _aclValues.end(); ++pI)
  {
    if (fVal < *pI)
      break;
  }

  if (_bOutsideGrayed == true)
  {
    if ((pI == _aclValues.begin()) || (pI == _aclValues.end()))
      return Color(0.5f, 0.5f, 0.5f);
    else
      return _aclColorFields[pI - _aclValues.begin() - 1];
  }

  if (pI == _aclValues.begin())
    return *_aclColorFields.begin();
  else if (pI == _aclValues.end())
    return *(_aclColorFields.end()-1);
  else
    return _aclColorFields[pI - _aclValues.begin() - 1];
}

inline unsigned short ColorLegend::getColorIndex (float fVal) const
{
  std::deque<float>::const_iterator pI;
  for (pI = _aclValues.begin(); pI != _aclValues.end(); ++pI)
  {
    if (fVal < *pI)
      break;
  }

  if (pI == _aclValues.begin())
    return 0;
  else if (pI == _aclValues.end())
    return (unsigned short)(_aclColorFields.size() - 1);
  else
    return pI - _aclValues.begin() - 1;
}

inline float ColorLegend::getMinValue (void) const
{
  return *_aclValues.begin();
}

inline float ColorLegend::getMaxValue (void) const
{
  return *(_aclValues.end()-1);
}

inline Color ColorGradient::getColor (float fVal) const
{
  if (_bOutsideGrayed == true)
  {
    if ((fVal < _fMin) || (fVal > _fMax))
      return Color(0.5f, 0.5f, 0.5f);
  }
  switch (_tStyle)
  {
    case ZERO_BASED:
    {
      if ((_fMin < 0.0f) && (_fMax > 0.0f))
      {
        if (fVal < 0.0f)
          return _clColFld1.getColor(fVal);
        else
          return _clColFld2.getColor(fVal);
      }
      else
        return _clColFld1.getColor(fVal);
    }

    default:
    case FLOW:
    {
      return _clColFld1.getColor(fVal);
    }
  }
}

inline unsigned short ColorGradient::getColorIndex (float fVal) const
{
  switch (_tStyle)
  {
    case ZERO_BASED:
    {
      if ((_fMin < 0.0f) && (_fMax > 0.0f))
      {
        if (fVal < 0.0f)
          return _clColFld1.getColorIndex(fVal);
        else
          return (unsigned short)(_clColFld1.getCountColors() + _clColFld2.getColorIndex(fVal));
      }
      else
        return _clColFld1.getColorIndex(fVal);
    }

    default:
    case FLOW:
    {
      return _clColFld1.getColorIndex(fVal);
    }
  }
}

inline const ColorModel& ColorGradient::getColorModel (void) const
{
  if ( _tStyle == ZERO_BASED )
  {
    if ( _fMax <= 0.0f )
      return _clBottom;
    else if ( _fMin >= 0.0f )
      return _clTop;
    else
      return _clTotal;
  }
  else
  {
    return _clTotal;
  }
}

} // namespace App

#endif // APP_COLORMODEL_H
