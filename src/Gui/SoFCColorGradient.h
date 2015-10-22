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


#ifndef GUI_SOFCCOLORGRADIENT_H
#define GUI_SOFCCOLORGRADIENT_H

#include <Inventor/nodes/SoSeparator.h>
#include "SoFCColorBar.h"
#include <App/ColorModel.h>
#include <vector>

class SoCoordinate3;
class SoMFString;
class SbVec2s;

namespace Gui {

class GuiExport SoFCColorGradient : public SoFCColorBarBase {
  typedef SoFCColorBarBase inherited;

  SO_NODE_HEADER(Gui::SoFCColorGradient);

public:
  static void initClass(void);
  static void finish(void);
  SoFCColorGradient(void);

  /**
   * Sets the range of the colorbar from the maximum \a fMax to the minimum \a fMin.
   * \a prec indicates the post decimal positions, \a prec should be in between 0 and 6.
   */
  void setRange( float fMin, float fMax, int prec=3 );
  /**
   * Returns the associated color to the value \a fVal.
   */
  App::Color getColor (float fVal) const { return _cColGrad.getColor(fVal); }
  void setOutsideGrayed (bool bVal) { _cColGrad.setOutsideGrayed(bVal); }
  /**
   * Returns always true if the gradient is in mode to show colors to arbitrary values of \a fVal,
   * otherwise true is returned if \a fVal is within the specified parameter range, if not false is
   * returned.
   */
  bool isVisible (float fVal) const;
  /** Returns the current minimum of the parameter range. */
  float getMinValue (void) const { return _cColGrad.getMinValue(); }
  /** Returns the current maximum of the parameter range. */
  float getMaxValue (void) const { return _cColGrad.getMaxValue(); }
  /**
   * Opems a dialog to customie the current settings of the color gradient bar.
   * Returns true if the settings have been changed, false otherwise.
   */
  bool customize();
  /** Returns the name of the color bar. */
  const char* getColorBarName() const { return "Color Gradient"; }

protected:
  /**
   * Sets the current viewer size this color gradient is embedded into to recalculate its new position.
   */
  void setViewportSize( const SbVec2s& size );

  virtual ~SoFCColorGradient();
  /**
   * Sets the color model of the underlying color gradient to \a tModel. \a tModel either can
   * be \c TRIA, \c INVERSE_TRIA, \c GRAY or \c INVERSE_GRAY
   */
  void setColorModel (App::ColorGradient::TColorModel tModel);
  /**
   * Sets the color style of the underlying color gradient to \a tStyle. \a tStyle either can
   * be \c FLOW or \c ZERO_BASED
   */
  void setColorStyle (App::ColorGradient::TStyle tStyle);
  /** Rebuild the gradient bar. */
  void rebuildGradient();
  /** Returns a list of \a count labels within the ranhe [\a fMin, \a fMax].  */
  std::vector<float> getMarkerValues(float fMin, float fMax, int count) const;

private:
  /** Sets the new labels. */
  void setMarkerLabel( const SoMFString& label );

private:
  SoCoordinate3* coords;
  SoSeparator* labels;
  float _fMaxX, _fMinX, _fMaxY, _fMinY;
  bool  _bOutInvisible;
  int _precision;
  App::ColorGradient _cColGrad;
};

} // namespace Gui


#endif // GUI_SOFCCOLORGRADIENT_H

