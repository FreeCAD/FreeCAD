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


#ifndef GUI_SOFCCOLORLEGEND_H
#define GUI_SOFCCOLORLEGEND_H

#include <Inventor/nodes/SoSeparator.h>
#include "SoFCColorBar.h"
#include <App/ColorModel.h>

class SoCoordinate3;
class SoMFString;
class SbVec2s;

namespace Gui {

class GuiExport SoFCColorLegend : public SoFCColorBarBase {
  typedef SoFCColorBarBase inherited;

  SO_NODE_HEADER(Gui::SoFCColorLegend);

public:
  static void initClass(void);
  static void finish(void);
  SoFCColorLegend(void);

  void setMarkerLabel( const SoMFString& label );

  /**
   * Sets the range of the colorbar from the maximum \a fMax to the minimum \a fMin.
   * \a prec indicates the post decimal positions, \a prec should be in between 0 and 6.
   */
  void setRange( float fMin, float fMax, int prec=3 );
  /**
   * Sets the color model of the underlying color ramp to \a tModel. \a tModel either can
   * be \c TRIA, \c INVERSE_TRIA or \c GRAY
   */
  void setColorModel (App::ColorGradient::TColorModel tModel);

  unsigned short getColorIndex (float fVal) const { return _cColRamp.getColorIndex(fVal);  }
  App::Color getColor (float fVal) const { return _cColRamp.getColor(fVal); }
  void setOutsideGrayed (bool bVal) { _cColRamp.setOutsideGrayed(bVal); }
  bool isVisible (float fVal) const { return false; }
  float getMinValue (void) const { return _cColRamp.getMinValue(); }
  float getMaxValue (void) const { return _cColRamp.getMaxValue(); }
  unsigned long countColors (void) const { return _cColRamp.getCountColors(); }

  bool customize() { return false; }
  const char* getColorBarName() const { return "Color Legend"; }

//  virtual void handleEvent(SoHandleEventAction * action);
//  virtual void GLRenderBelowPath(SoGLRenderAction * action);
//  virtual void GLRenderInPath(SoGLRenderAction * action);

protected:
  void setViewportSize( const SbVec2s& size );
  virtual ~SoFCColorLegend();
//  virtual void redrawHighlighted(SoAction * act, SbBool  flag);

  SoCoordinate3* coords;
  SoSeparator* labels;

private:
  float _fPosX, _fPosY;
  App::ColorGradient _cColRamp;
};

} // namespace Gui


#endif // GUI_SOFCCOLORLEGEND_H

