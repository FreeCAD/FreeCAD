/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SoVectorizeAction SoVectorizeAction.h Inventor/annex/HardCopy/SoVectorizeAction.h
  \brief The SoVectorizeAction class is the base class for vectorizing Coin scene graphs.

  \ingroup coin_hardcopy

  SoVectorizeAction will traverse the scene graph and convert all
  supported geometry into vectorized data. Subclasses can then use
  this data to produce vector files of different formats.

  Currently supported geometry:

  \li Triangles (polygons will be tessellated)
  \li Line segments
  \li Points (can be drawn as circles or squares)
  \li 2D text
  \li 3D text (will be converted to triangles)
  \li Images (from the SoImage node)

  The geometry will be shaded based on the OpenGL shading model, so
  lights and material will affect the geometry in the same way as in a
  standard Coin viewer. Please note that neither transparency nor
  texture mapping is supported yet.

  \since Coin 2.1
  \since TGS provides HardCopy support as a separate extension for TGS Inventor.
*/

/*!
  \enum SoVectorizeAction::DimensionUnit

  The unit used by the scene graph.
*/

/*!
  \var SoVectorizeAction::DimensionUnit SoVectorizeAction::INCH
*/

/*!
  \var SoVectorizeAction::DimensionUnit SoVectorizeAction::MM
*/

/*!
  \var SoVectorizeAction::DimensionUnit SoVectorizeAction::METER
*/


/*!
  \enum SoVectorizeAction::Orientation

  The drawing orientation.
*/

/*!
  \var SoVectorizeAction::Orientation SoVectorizeAction::PORTRAIT
*/
/*!
  \var SoVectorizeAction::Orientation SoVectorizeAction::LANDSCAPE
*/

/*!
  \enum SoVectorizeAction::PageSize

  The size of the destination page.
*/


/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A0
  841 x 1189 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A1
  594 x 841 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A2
  420 x 594 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A3
  297 x 420 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A4
  210 x 297 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A5
  148 x 210 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A6
  105 x 148 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A7
  74 x 105 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A8
  52 x 74 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A9
  37 x 52 mm.
*/

/*!
  \var SoVectorizeAction::PageSize SoVectorizeAction::A10
  26 x 37 mm.
*/

/*!
  \enum SoVectorizeAction::PointStyle
  Enumerates point rendering styles.
*/

/*!
  \var SoVectorizeAction::PointStyle SoVectorizeAction::CIRCLE

  Render points as circles.
*/

/*!
  \var SoVectorizeAction::PointStyle SoVectorizeAction::SQUARE

  Render points as squares.
*/

/*!
  \fn void SoVectorizeAction::printHeader(void) const
  \COININTERNAL
*/

// *************************************************************************

#include <Inventor/annex/HardCopy/SoVectorizeAction.h>
#include "coindefs.h"

#include <Inventor/SbViewportRegion.h>

#include "hardcopy/VectorizeActionP.h"
#include "actions/SoSubActionP.h"

// *************************************************************************

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->publ)

// *************************************************************************

SO_ACTION_SOURCE(SoVectorizeAction);

// *************************************************************************

/*!
  \copydetails SoAction::initClass(void)
*/
void
SoVectorizeAction::initClass(void)
{
   SO_ACTION_INTERNAL_INIT_CLASS(SoVectorizeAction, SoCallbackAction);
}

/*!
  Default constructor.
*/
SoVectorizeAction::SoVectorizeAction(void)
{
  PRIVATE(this) = new SoVectorizeActionP(this);
  SO_ACTION_CONSTRUCTOR(SoVectorizeAction);
}

/*!
  Destructor.
*/
SoVectorizeAction::~SoVectorizeAction()
{
  delete PRIVATE(this);
}

// *************************************************************************

/*!
  Returns the SoVectorOutput class used by this action. The output
  is written to stdout by default, but you can change this by using
  SoVectorOutput::openFile().
*/
SoVectorOutput *
SoVectorizeAction::getOutput(void) const
{
  if (PRIVATE(this)->output == NULL) {
    PRIVATE(this)->output = new SoVectorOutput;
  }
  return PRIVATE(this)->output;
}

// doc in parent
void
SoVectorizeAction::apply(SoNode * node)
{
  inherited::apply(node);
}

// doc in parent
void
SoVectorizeAction::apply(SoPath * path)
{
  inherited::apply(path);
}

// doc in parent
void
SoVectorizeAction::apply(const SoPathList & pathlist, SbBool obeysrules)
{
  inherited::apply(pathlist, obeysrules);
}

/*!

  Begin writing a page. This will write file header information and
  print background (if enabled) and border.

*/
void
SoVectorizeAction::beginPage(const SbVec2f & startpagepos,
                             const SbVec2f & pagesize,
                             DimensionUnit u)
{
  PRIVATE(this)->page.startpos = to_mm(startpagepos, u);
  PRIVATE(this)->page.size = to_mm(pagesize, u);

  PRIVATE(this)->viewport.startpos = to_mm(startpagepos, u);
  PRIVATE(this)->viewport.size = to_mm(pagesize, u);

  // set up a viewport so that the aspect ratio will match the page
  SbVec2f s = this->getRotatedViewportSize();
  float m = SbMax(s[0], s[1]);

  SbVec2s ss;
  ss[0] = (short) ((s[0] / m) * 32767.0f);
  ss[1] = (short) ((s[1] / m) * 32767.0f);
  this->setViewportRegion(SbViewportRegion(ss));

  // print header and page information
  this->printHeader();
  this->beginViewport(startpagepos, pagesize, u);
  if (PRIVATE(this)->background.on) this->printBackground();
}

/*!
  End page. This will write all remaining geometry, and write the file footer.
*/

void
SoVectorizeAction::endPage(void)
{
  this->endViewport();
  this->printFooter();
}

/*!
  Begin writing a viewport inside the current page.

  \sa beginPage(), endViewport()
*/
void
SoVectorizeAction::beginViewport(const SbVec2f & start, const SbVec2f & size,
                                 DimensionUnit u)
{
  if (start[0] < 0.0f || start[1] < 0.0f) {
    PRIVATE(this)->viewport.startpos = PRIVATE(this)->page.startpos;
  }
  else {
    PRIVATE(this)->viewport.startpos = to_mm(start, u);
  }
  if (size[0] <= 0.0f || size[1] <= 0.0f) {
    PRIVATE(this)->viewport.size = PRIVATE(this)->page.size;
  }
  else {
    PRIVATE(this)->viewport.size = to_mm(size, u);
  }
  PRIVATE(this)->reset();

  // this will set up clipping (for PostScript, at least)
  this->printViewport();

  // set up a SbViewportRegion (used by SoCallbackAction) so that the
  // aspect ratio will match the page
  SbVec2f s = this->getRotatedViewportSize();
  float m = SbMax(s[0], s[1]);

  SbVec2s ss;
  ss[0] = (short) ((s[0] / m) * 32767.0f);
  ss[1] = (short) ((s[1] / m) * 32767.0f);
  this->setViewportRegion(SbViewportRegion(ss));
}

/*!
  End writing a viewport. This will flush all vector items.
*/
void
SoVectorizeAction::endViewport(void)
{
  if (PRIVATE(this)->itemlist.getLength()) {
    PRIVATE(this)->outputItems();
    PRIVATE(this)->reset();
  }
}

/*!
  Will calibrate pixel based attributes (font size, line width,
  points size, etc) so that it will match OpenGL rendering done in
  \a vp.

  \sa setPixelSize()
  \sa setNominalWidth()
*/
void
SoVectorizeAction::calibrate(const SbViewportRegion & vp)
{
  SbVec2s vpsize = vp.getViewportSizePixels();
  PRIVATE(this)->pixelimagesize = this->getPageSize()[1] / float(vpsize[1]);
  PRIVATE(this)->nominalwidth = this->getPageSize()[1] / float(vpsize[1]);
}

/*!
  Sets the orientation to \a o.
*/
void
SoVectorizeAction::setOrientation(Orientation o)
{
  PRIVATE(this)->orientation = o;
}

/*!
  Returns the current orientation.

  \sa setOrientation()
*/

SoVectorizeAction::Orientation
SoVectorizeAction::getOrientation(void) const
{
  return PRIVATE(this)->orientation;
}

/*!
  Sets the background color. If \a bg is \e FALSE, the background will
  not be cleared before rendering. If \a bg is \e TRUE, the background
  will be cleared to \a col before in beginPage().
*/
void
SoVectorizeAction::setBackgroundColor(SbBool bg, const SbColor & col)
{
  PRIVATE(this)->background.on = bg;
  PRIVATE(this)->background.color = col;
}

/*!
  Returns if the background will be cleared or not. When this function
  returns \e TRUE, \a col will be set to the background color.
*/
SbBool
SoVectorizeAction::getBackgroundColor(SbColor & col) const
{
  col = PRIVATE(this)->background.color;
  return PRIVATE(this)->background.on;
}

/*!
  Sets how to convert pixel based attributes (line width and point
  size) to vector sizes. By default 1 pixel equals 0.35 mm.

  \sa calibrate()
*/
void
SoVectorizeAction::setNominalWidth(float w, DimensionUnit u)
{
  PRIVATE(this)->nominalwidth = to_mm(w, u);
}

float
SoVectorizeAction::getNominalWidth(DimensionUnit u) const
{
  return from_mm(PRIVATE(this)->nominalwidth, u);
}

/*!
  Sets how the images and 2D fonts are converted. By default 1
  pixel equals 0.35 mm.


  \sa calibrate()
*/
void
SoVectorizeAction::setPixelImageSize(float w, DimensionUnit u)
{
  PRIVATE(this)->pixelimagesize = to_mm(w, u);
}

/*!
  Returns the pixel image size.

  \sa setPixelImageSize()
*/
float
SoVectorizeAction::getPixelImageSize(DimensionUnit u) const
{
  return from_mm(PRIVATE(this)->pixelimagesize, u);
}

/*!
  Sets the points rendering style. Default style is CIRCLE.
*/
void
SoVectorizeAction::setPointStyle(const PointStyle & style)
{
  PRIVATE(this)->pointstyle = style;
}

/*!
  Returns the points rendering style.
*/
SoVectorizeAction::PointStyle
SoVectorizeAction::getPointStyle(void) const
{
  return PRIVATE(this)->pointstyle;
}


/*!
  \COININTERNAL

  Should be overridden by subclasses to print file footer data.
 */
void
SoVectorizeAction::printFooter(void) const
{
}

/*!
  \COININTERNAL

  Should be overridden by subclasses to print background data.
*/
void
SoVectorizeAction::printBackground(void) const
{
}

/*!
  \COININTERNAL

  Should be overridden by subclasses to set up the current page viewport.
*/
void
SoVectorizeAction::printViewport(void) const
{

}

/*!
  \COININTERNAL

  Should be overridden by subclasses to print an item.
*/
void
SoVectorizeAction::printItem(const SoVectorizeItem * COIN_UNUSED_ARG(item)) const
{
}

/*!
  Convenience method for subclasses. Will return the viewport startpos,
  taking the orientation into account
*/
SbVec2f
SoVectorizeAction::getRotatedViewportStartpos(void) const
{
  SbVec2f p = PRIVATE(this)->viewport.startpos;

  if (this->getOrientation() == LANDSCAPE) SbSwap(p[0], p[1]);
  return p;
}


/*!
  Convenience method for subclasses. Will return the viewport size,
  taking the orientation into account
*/
SbVec2f
SoVectorizeAction::getRotatedViewportSize(void) const
{
  SbVec2f p = PRIVATE(this)->viewport.size;

  if (this->getOrientation() == LANDSCAPE) SbSwap(p[0], p[1]);
  return p;
}

/*!
  Should be used by subclasses to set the SoVectorOutput
  instance that should be used.
*/
void
SoVectorizeAction::setOutput(SoVectorOutput * output)
{
  if (PRIVATE(this)->output) {
    delete PRIVATE(this)->output;
  }
  PRIVATE(this)->output = output;
}

/*!
  Converts pixels to normalized units.
 */
float
SoVectorizeAction::pixelsToUnits(const int pixels)
{
  float mm = this->getPixelImageSize() * pixels;
  float ps = this->getPageSize()[1];
  return mm / ps;
}

/*!
  Returns the current page startpos.
*/
const SbVec2f &
SoVectorizeAction::getPageStartpos(void) const
{
  return PRIVATE(this)->page.startpos;
}

/*!
  Returns the current page size.
*/
const SbVec2f &
SoVectorizeAction::getPageSize(void) const
{
  return PRIVATE(this)->page.size;
}

/*!
  Returns the bps tree used to store triangle and line vertices.
*/
const SbBSPTree &
SoVectorizeAction::getBSPTree(void) const
{
  return PRIVATE(this)->bsp;
}

void
SoVectorizeAction::beginStandardPage(const PageSize & pagesize, const float border)
{
  static int a_table[] = {
    // A0 - A10
    841, 1189,
    594, 841,
    420, 594,
    297, 420,
    210, 297,
    148, 210,
    105, 148,
    74, 105,
    52, 74,
    37, 52,
    26, 37,
    // B0 - B10 (might add later)
  };

  int idx = (int) pagesize;
  assert(idx >=0 && idx <= 10);

  float xdim = (float) a_table[idx*2];
  float ydim = (float) a_table[idx*2+1];

  this->beginPage(SbVec2f(border, border), SbVec2f(xdim-2.0f*border, ydim-2.0f*border), MM);
}

/*!
  Sets the drawing dimensions. You can use this and setStartPosition() instead
  of using beginViewport(). Provided for TGS OIV compatibility.

  \sa beginViewport()
*/
void
SoVectorizeAction::setDrawingDimensions(const SbVec2f & d, DimensionUnit u)
{
  PRIVATE(this)->viewport.size = to_mm(d, u);
}

/*!
  Returns the current drawing dimensions.
*/
SbVec2f
SoVectorizeAction::getDrawingDimensions(DimensionUnit u) const
{
  return from_mm(PRIVATE(this)->viewport.size, u);
}

/*!
  Sets the drawing staring position. You can use this and setDrawingDimensions() instead
  of using beginViewport(). Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setStartPosition(const SbVec2f & p, DimensionUnit u)
{
  PRIVATE(this)->viewport.startpos = to_mm(p, u);

}

/*!
  Returns the current drawing starting position.
*/
SbVec2f
SoVectorizeAction::getStartPosition(DimensionUnit u) const
{
  return from_mm(PRIVATE(this)->viewport.startpos, u);
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setColorTranslationMethod(ColorTranslationMethod COIN_UNUSED_ARG(method))
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
SoVectorizeAction::ColorTranslationMethod
SoVectorizeAction::getColorTranslationMethod(void) const
{
  return AS_IS;
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setLineEndStyle(EndLineStyle COIN_UNUSED_ARG(style))
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
SoVectorizeAction::EndLineStyle
SoVectorizeAction::getLineEndStyle(void) const
{
  return BUTT_END;
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setLineJoinsStyle(JoinLineStyle COIN_UNUSED_ARG(style))
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
SoVectorizeAction::JoinLineStyle
SoVectorizeAction::getLineJoinsStyle(void) const
{
  return NO_JOIN;
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setHLHSRMode(HLHSRMode COIN_UNUSED_ARG(mode))
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
SoVectorizeAction::HLHSRMode
SoVectorizeAction::getHLHSRMode(void) const
{
  return HLHSR_PAINTER;
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setBorder(float COIN_UNUSED_ARG(width))
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setBorder(float COIN_UNUSED_ARG(width), SbColor COIN_UNUSED_ARG(color))
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setMiterLimit(float COIN_UNUSED_ARG(limit))
{
}

float
SoVectorizeAction::getMiterLimit(void) const
{
  return 0.0f;
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setPenDescription(int COIN_UNUSED_ARG(num_pens),
                                     const SbColor * COIN_UNUSED_ARG(colors),
                                     const float * COIN_UNUSED_ARG(widths),
                                     DimensionUnit COIN_UNUSED_ARG(u))
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::getPenDescription(SbColor * COIN_UNUSED_ARG(colors),
                                     float * COIN_UNUSED_ARG(widths),
                                     DimensionUnit COIN_UNUSED_ARG(u)) const
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
int
SoVectorizeAction::getPenNum(void) const
{
  return 0;
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::setColorPriority(SbBool COIN_UNUSED_ARG(priority))
{
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
SbBool
SoVectorizeAction::getColorPriority(void) const
{
  return FALSE;
}

/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
void
SoVectorizeAction::enableLighting(SbBool COIN_UNUSED_ARG(flag))
{
}
/*!
  Not implemented yet. Provided for TGS OIV compatibility.
*/
SbBool
SoVectorizeAction::isLightingEnabled(void) const
{
  return TRUE;
}


#undef PRIVATE
#undef PUBLIC

// *************************************************************************
