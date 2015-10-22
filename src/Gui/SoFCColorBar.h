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


#ifndef GUI_SOFCCOLORBAR_H
#define GUI_SOFCCOLORBAR_H

#include <Inventor/SbVec2s.h>
#include <Inventor/nodes/SoSeparator.h>
#include <QTime>
#include <Base/Observer.h>
#include <App/ColorModel.h>
#include <vector>

class SoSwitch;
class SoEventCallback;
class SbVec2s;
class SoHandleEventAction;
class SoGLRenderAction;

namespace Gui {
class SoFCColorGradient;

/**
 * The abstract color bar base class to get most important information on how to convert a scalar to an RGB color. 
 * @author Werner Mayer
 */
class GuiExport SoFCColorBarBase : public SoSeparator, public App::ValueFloatToRGB {
  typedef SoSeparator inherited;

  SO_NODE_ABSTRACT_HEADER(Gui::SoFCColorBarBase);

public:
  static void initClass(void);
  static void finish(void);

  virtual void GLRenderBelowPath ( SoGLRenderAction *action );

  /**
   * Sets the range of the colorbar from the maximum \a fMax to the minimum \a fMin.
   * \a prec indicates the post decimal positions, \a prec should be in between 0 and 6.
   *
   * This method must be implemented in subclasses.
   */
  virtual void setRange( float fMin, float fMax, int prec=3 ) = 0;
  /**
   * Returns the associated color to the value \a fVal.
   *
   * This method must be implemented in subclasses.
   */
  virtual App::Color getColor(float fVal) const = 0;
  /**
   * Returns always true if the color bar is in mode to show colors to arbitrary values of \a fVal,
   * otherwise true is returned if \a fVal is within the specified parameter range, if not false is
   * returned.
   *
   * This method must be implemented in subclasses.
   */
  virtual bool isVisible (float fVal) const = 0;
  /**
   * Sets whether values outside the range should be in gray,
   *
   * This method must be implemented in subclasses.
   */
  virtual void setOutsideGrayed (bool bVal) = 0;
  /** Returns the current minimum of the parameter range. 
   *
   * This method must be implemented in subclasses.
   */
  virtual float getMinValue (void) const = 0;
  /** Returns the current maximum of the parameter range. 
   *
   * This method must be implemented in subclasses.
   */
  virtual float getMaxValue (void) const = 0;
  /**
   * Opems a dialog to customie the current settings of the color bar.
   * Returns true if the settings have been changed, false otherwise.
   *
   * This method must be implemented in subclasses.
   */
  virtual bool customize() = 0;
  /** Returns the name of the color bar. 
   *
   * This method must be implemented in subclasses.
   */
  virtual const char* getColorBarName() const = 0;

protected:
  /**
   * Sets the current viewer size to recalculate the new position.
   *
   * This method must be implemented in subclasses.
   */
  virtual void setViewportSize( const SbVec2s& size ) = 0;

  SoFCColorBarBase (void);
  virtual ~SoFCColorBarBase ();

private:
  SbVec2s _windowSize;
};

// --------------------------------------------------------------------------

/**
 * The color bar class that redirects all calls to its handled color bars. 
 * @author Werner Mayer
 */
class GuiExport SoFCColorBar : public SoFCColorBarBase, public Base::Subject<int> {
  typedef SoFCColorBarBase inherited;

  SO_NODE_HEADER(Gui::SoFCColorBar);

public:
  static void initClass(void);
  static void finish(void);
  SoFCColorBar(void);

  /**
   * Returns the currently active color bar object.
   */
  SoFCColorBarBase* getActiveBar() const;
  /**
   * Handles the mouse button events and checks if the user has clicked on the area of the currently active color bar.
   */
  void handleEvent (SoHandleEventAction *action);
  /**
   * Sets the range of all color bars from the maximum \a fMax to the minimum \a fMin.
   * \a prec indicates the post decimal positions, \a prec should be in between 0 and 6.
   */
  void setRange( float fMin, float fMax, int prec=3 );
  /**
   * Returns the associated color to the value \a fVal of the currently active color bar.
   */
  App::Color getColor(float fVal) const;
  /**
   * Sets whether values outside the range should be in gray,
   */
  void setOutsideGrayed (bool bVal);
  /**
   * Returns the return value of the currently active color bar.
   */
  bool isVisible (float fVal) const;
  /** 
   * Returns the current minimum of the parameter range of the currently active color bar. 
   */
  float getMinValue (void) const;
  /** 
   * Returns the current maximum of the parameter range of the currently active color bar. 
   */
  float getMaxValue (void) const;
  /**
   * Customizes the currently active color bar.
   */
  bool customize();
  /** Returns the name of the color bar. 
   */
  const char* getColorBarName() const { return "Color Bar"; }

protected:
  /**
   * Sets the current viewer size to all color bars to recalculate their new position.
   */
  void setViewportSize( const SbVec2s& size );

  virtual ~SoFCColorBar();

private:
  static void eventCallback(void * userdata, SoEventCallback * node);

private:
  float _fMaxX, _fMinX, _fMaxY, _fMinY;
  QTime _timer;

  SoSwitch* pColorMode; 
  std::vector<SoFCColorBarBase*> _colorBars;
};

} // namespace Gui


#endif // GUI_SOFCCOLORBAR_H

