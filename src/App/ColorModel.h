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
  ValueFloatToRGB () = default;
  virtual ~ValueFloatToRGB () = default;
};


class AppExport ColorModel
{
public:
  ColorModel() = default;
  ColorModel (std::size_t usCt) {
    colors.resize(usCt);
  }
  ColorModel(const ColorModel&) = default;
  virtual ~ColorModel () = default;
  ColorModel& operator = (const ColorModel&) = default;
  std::size_t getCountColors() const {
    return colors.size();
  }
  std::vector<Color>  colors;
};

class AppExport ColorModelBlueGreenRed : public ColorModel
{
public:
  ColorModelBlueGreenRed () : ColorModel(5)
  {
    colors[0] = Color(0, 0, 1);
    colors[1] = Color(0, 1, 1);
    colors[2] = Color(0, 1, 0);
    colors[3] = Color(1, 1, 0);
    colors[4] = Color(1, 0, 0);
  }
};

class AppExport ColorModelBlueCyanGreen : public ColorModel
{
public:
  ColorModelBlueCyanGreen () : ColorModel(3)
  {
    colors[0] = Color(0, 0, 1);
    colors[1] = Color(0, 1, 1);
    colors[2] = Color(0, 1, 0);
  }
};

class AppExport ColorModelGreenYellowRed : public ColorModel
{
public:
  ColorModelGreenYellowRed () : ColorModel(3)
  {
    colors[0] = Color(0, 1, 0);
    colors[1] = Color(1, 1, 0);
    colors[2] = Color(1, 0, 0);
  }
};

class AppExport ColorModelRedGreenBlue : public ColorModel
{
public:
  ColorModelRedGreenBlue () : ColorModel(5)
  {
    colors[0] = Color(1, 0, 0);
    colors[1] = Color(1, 1, 0);
    colors[2] = Color(0, 1, 0);
    colors[3] = Color(0, 1, 1);
    colors[4] = Color(0, 0, 1);
  }
};

class AppExport ColorModelGreenCyanBlue : public ColorModel
{
public:
  ColorModelGreenCyanBlue () : ColorModel(3)
  {
    colors[0] = Color(0, 1, 0);
    colors[1] = Color(0, 1, 1);
    colors[2] = Color(0, 0, 1);
  }
};

class AppExport ColorModelRedYellowGreen : public ColorModel
{
public:
  ColorModelRedYellowGreen () : ColorModel(3)
  {
    colors[0] = Color(1, 0, 0);
    colors[1] = Color(1, 1, 0);
    colors[2] = Color(0, 1, 0);
  }
};

class AppExport ColorModelBlueWhiteRed : public ColorModel
{
public:
  ColorModelBlueWhiteRed () : ColorModel(5)
  {
    colors[0] = Color(0, 0, 1);
    colors[1] = Color(float(85.0 / 255), float(170.0 / 255), 1);
    colors[2] = Color(1, 1, 1);
    colors[3] = Color(1, float(85.0 / 255), 0);
    colors[4] = Color(1, 0, 0);
  }
};

class AppExport ColorModelBlueWhite : public ColorModel
{
public:
  ColorModelBlueWhite () : ColorModel(3)
  {
    colors[0] = Color(0, 0, 1);
    colors[1] = Color(float(85.0 / 255), float(170.0 / 255), 1);
    colors[2] = Color(1, 1, 1);
  }
};

class AppExport ColorModelWhiteRed : public ColorModel
{
public:
  ColorModelWhiteRed () : ColorModel(3)
  {
    colors[0] = Color(1, 1, 1);
    colors[1] = Color(1, float(85.0 / 255), 0);
    colors[2] = Color(0, 1, 0);
  }
};

class AppExport ColorModelBlackWhite : public ColorModel
{
public:
  ColorModelBlackWhite () : ColorModel(2)
  {
    colors[0] = Color(0, 0, 0);
    colors[1] = Color(1, 1, 1);
  }
};

class AppExport ColorModelBlackGray : public ColorModel
{
public:
  ColorModelBlackGray () : ColorModel(2)
  {
    colors[0] = Color(0.0f, 0.0f, 0.0f);
    colors[1] = Color(0.5f, 0.5f, 0.5f);
  }
};

class AppExport ColorModelGrayWhite : public ColorModel
{
public:
  ColorModelGrayWhite () : ColorModel(2)
  {
    colors[0] = Color(0.5f, 0.5f, 0.5f);
    colors[1] = Color(1.0f, 1.0f, 1.0f);
  }
};

class AppExport ColorModelWhiteBlack : public ColorModel
{
public:
  ColorModelWhiteBlack () : ColorModel(2)
  {
    colors[0] = Color(1, 1, 1);
    colors[1] = Color(0, 0, 0);
  }
};

class AppExport ColorModelWhiteGray : public ColorModel
{
public:
  ColorModelWhiteGray () : ColorModel(2)
  {
    colors[0] = Color(1.0f, 1.0f, 1.0f);
    colors[1] = Color(0.5f, 0.5f, 0.5f);
  }
};

class AppExport ColorModelGrayBlack : public ColorModel
{
public:
  ColorModelGrayBlack () : ColorModel(2)
  {
    colors[0] = Color(0.5f, 0.5f, 0.5f);
    colors[1] = Color(0.0f, 0.0f, 0.0f);
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
  ColorField ();
  ColorField (const ColorField &rclCF);
  ColorField (const ColorModel &rclModel, float fMin, float fMax, std::size_t usCt);
  virtual ~ColorField () = default;

  ColorField& operator = (const ColorField &rclCF);

  std::size_t getCountColors () const { return ctColors; }
  void set (const ColorModel &rclModel, float fMin, float fMax, std::size_t usCt);
  void setCountColors (std::size_t usCt) { set(colorModel, fMin, fMax, usCt); }
  void setRange (float fMin, float fMax) { set(colorModel, fMin, fMax, ctColors); }
  void getRange (float &rfMin, float &rfMax) { rfMin = fMin; rfMax = fMax; }
  std::size_t getMinColors () const { return colorModel.getCountColors(); }
  void setColorModel (const ColorModel &rclModel);
  const ColorModel& getColorModel () const { return colorModel; }
  void setDirect (std::size_t usInd, Color clCol) { colorField[usInd] = clCol; }
  float getMinValue () const { return fMin; }
  float getMaxValue () const { return fMax; }

  Color getColor (std::size_t usIndex) const { return colorField[usIndex]; }
  inline Color  getColor (float fVal) const;
  inline std::size_t getColorIndex (float fVal) const;

protected:
  ColorModel colorModel;
  float fMin, fMax;
  float fAscent, fConstant;  // Index := _fConstant + _fAscent * WERT
  std::size_t ctColors;
  std::vector<Color> colorField;

  void rebuild ();
  void interpolate (Color clCol1, std::size_t usPos1, Color clCol2, std::size_t usPos2);
};

inline Color ColorField::getColor (float fVal) const
{
  // if the value is outside or at the border of the range
  std::size_t ct = colorModel.getCountColors() - 1;
  if ( fVal <= fMin )
    return colorModel.colors[0];
  else if ( fVal >= fMax )
    return colorModel.colors[ct];

  // get the color field position (with 0 < t < 1)
  float t = (fVal - fMin) / (fMax - fMin);
  Color col(1.0f, 1.0f, 1.0f); // white as default
  for ( std::size_t i=0; i<ct; i++ )
  {
    float r = (float)(i+1)/(float)ct;
    if ( t < r )
    {
      // calculate the exact position in the subrange
      float s = t*(float)ct-(float)i;
      Color c1 = colorModel.colors[i];
      Color c2 = colorModel.colors[i+1];
      col.r = (1.0f-s) * c1.r + s * c2.r;
      col.g = (1.0f-s) * c1.g + s * c2.g;
      col.b = (1.0f-s) * c1.b + s * c2.b;
      break;
    }
  }

  return col;
}

inline std::size_t ColorField::getColorIndex (float fVal) const
{
  return (std::size_t)(std::min<int>(std::max<int>(int(fConstant + fAscent * fVal), 0), int(ctColors - 1)));
}


class AppExport ColorGradient
{
public:
  enum TStyle { FLOW, ZERO_BASED };

  ColorGradient ();
  ColorGradient (float fMin, float fMax, std::size_t usCtColors, TStyle tS, bool bOG = false);
  ColorGradient (const ColorGradient &) = default;
  ColorGradient& operator = (const ColorGradient &) = default;

  void set (float fMin, float fMax, std::size_t usCt, TStyle tS, bool bOG);
  void setRange (float fMin, float fMax) { set(fMin, fMax, ctColors, tStyle, outsideGrayed); }
  void getRange (float &rfMin, float &rfMax) const { rfMin = _fMin; rfMax = _fMax; }
  std::size_t getCountColors () const { return ctColors; }
  void setCountColors (std::size_t usCt) { set(_fMin, _fMax, usCt, tStyle, outsideGrayed); }
  void setStyle (TStyle tS) { set(_fMin, _fMax, ctColors, tS, outsideGrayed); }
  std::size_t getMinColors () const;
  TStyle getStyle () const { return tStyle; }
  void setOutsideGrayed (bool bGrayed) { outsideGrayed = bGrayed; }
  bool isOutsideGrayed () const { return outsideGrayed; }
  void setColorModel (std::size_t tModel);
  std::size_t getColorModelType () const { return tColorModel; }
  inline const ColorModel& getColorModel () const;
  std::vector<std::string> getColorModelNames() const;
  float getMinValue () const { return _fMin; }
  float getMaxValue () const { return _fMax; }

  inline Color  getColor (float fVal) const;
  inline std::size_t getColorIndex (float fVal) const;

protected:
  void createStandardPacks();

protected:
  ColorField     colorField1, colorField2;
  TStyle         tStyle;
  float          _fMin, _fMax;
  std::size_t    ctColors;
  bool           outsideGrayed;
  std::size_t    tColorModel;
  ColorModelPack currentModelPack;
  std::vector<ColorModelPack> modelPacks;

  void  rebuild ();
  void  setColorModel ();
};


class AppExport ColorLegend
{
public:
  ColorLegend ();
  ColorLegend (const ColorLegend &rclCL);
  virtual ~ColorLegend () {}

  ColorLegend& operator = (const ColorLegend &rclCL);
  bool operator == (const ColorLegend &rclCL) const;
  bool operator != (const ColorLegend &rclCL) const { return !(*this == rclCL); }

  void resize (std::size_t ulN);
  std::size_t addMin (const std::string &rclName);
  std::size_t addMax (const std::string &rclName);
  bool remove (std::size_t ulPos);
  void removeFirst ();
  void removeLast ();

  Color getColor (std::size_t ulPos) const;
  uint32_t  getPackedColor (std::size_t ulPos) const;
  bool setColor (std::size_t ulPos, float ucRed, float ucGreen, float ucBlue);
  bool setColor (std::size_t ulPos, unsigned long ulColor);
  float getValue (std::size_t ulPos) const;
  bool setValue (std::size_t ulPos, float fVal);
  std::string getText (std::size_t ulPos) const;
  bool setText (std::size_t ulPos, const std::string &rclName);
  std::size_t hasNumberOfFields () const { return colorFields.size(); }
  void setOutsideGrayed (bool bOS) { outsideGrayed = bOS; }
  bool isOutsideGrayed () const { return outsideGrayed; }
  inline float  getMinValue () const;
  inline float  getMaxValue () const;

  inline Color getColor (float fVal) const;
  inline std::size_t getColorIndex (float fVal) const;

protected:
  std::deque<Color> colorFields;
  std::deque<std::string> names;
  std::deque<float> values;
  bool outsideGrayed;
};

inline Color ColorLegend::getColor (float fVal) const
{
  std::deque<float>::const_iterator pI;
  for (pI = values.begin(); pI != values.end(); ++pI)
  {
    if (fVal < *pI)
      break;
  }

  if (outsideGrayed == true)
  {
    if ((pI == values.begin()) || (pI == values.end()))
      return Color(0.5f, 0.5f, 0.5f);
    else
      return colorFields[pI - values.begin() - 1];
  }

  if (pI == values.begin())
    return *colorFields.begin();
  else if (pI == values.end())
    return *(colorFields.end()-1);
  else
    return colorFields[pI - values.begin() - 1];
}

inline std::size_t ColorLegend::getColorIndex (float fVal) const
{
  std::deque<float>::const_iterator pI;
  for (pI = values.begin(); pI != values.end(); ++pI)
  {
    if (fVal < *pI)
      break;
  }

  if (pI == values.begin())
    return 0;
  else if (pI == values.end())
    return (std::size_t)(colorFields.size() - 1);
  else
    return pI - values.begin() - 1;
}

inline float ColorLegend::getMinValue () const
{
  return values.front();
}

inline float ColorLegend::getMaxValue () const
{
  return values.back();
}

inline Color ColorGradient::getColor (float fVal) const
{
  if (outsideGrayed == true)
  {
    if ((fVal < _fMin) || (fVal > _fMax))
      return Color(0.5f, 0.5f, 0.5f);
  }
  switch (tStyle)
  {
    case ZERO_BASED:
    {
      if ((_fMin < 0.0f) && (_fMax > 0.0f))
      {
        if (fVal < 0.0f)
          return colorField1.getColor(fVal);
        else
          return colorField2.getColor(fVal);
      }
      else
        return colorField1.getColor(fVal);
    }

    default:
    case FLOW:
    {
      return colorField1.getColor(fVal);
    }
  }
}

inline std::size_t ColorGradient::getColorIndex (float fVal) const
{
  switch (tStyle)
  {
    case ZERO_BASED:
    {
      if ((_fMin < 0.0f) && (_fMax > 0.0f))
      {
        if (fVal < 0.0f)
          return colorField1.getColorIndex(fVal);
        else
          return (std::size_t)(colorField1.getCountColors() + colorField2.getColorIndex(fVal));
      }
      else
        return colorField1.getColorIndex(fVal);
    }

    default:
    case FLOW:
    {
      return colorField1.getColorIndex(fVal);
    }
  }
}

inline const ColorModel& ColorGradient::getColorModel () const
{
  if ( tStyle == ZERO_BASED )
  {
    if ( _fMax <= 0.0f )
      return currentModelPack.bottomModel;
    else if ( _fMin >= 0.0f )
      return currentModelPack.topModel;
    else
      return currentModelPack.totalModel;
  }
  else
  {
    return currentModelPack.totalModel;
  }
}

} // namespace App

#endif // APP_COLORMODEL_H
