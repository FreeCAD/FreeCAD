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
  \class SbViewportRegion SbViewportRegion.h Inventor/SbViewportRegion.h
  \brief The SbViewportRegion class is a viewport within a full window.

  \ingroup coin_base

  The SbViewportRegion class contains information to represent a
  subview within a window. It stores information about the origin and
  size of the subview, as well as the size of the underlying "full"
  window.

  Available methods include inquiries and manipulation in both
  normalized coordinates and pixel coordinates.

  Below is a small example showing how the viewport of a viewer class
  can be modified, within a "proper" Coin and window system
  context. Hit 'D' or 'U' to move the viewport region 40 pixels down
  or up, respectively. Click 'Esc' and zoom with left + middle mouse
  buttons, to see how the region is defined, where no 3D geometry will
  be visible outside it. Click 'Esc' again to use 'U' and 'D'.

  \code
  // Copyright (C) by Kongsberg Oil & Gas Technologies. All rights reserved.

  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
  #include <Inventor/nodes/SoEventCallback.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoCone.h>
  #include <Inventor/events/SoKeyboardEvent.h>

  // ************************************************************

  static void
  keypresscbfunc(void * userdata, SoEventCallback * keyboardcb)
  {
    SoQtExaminerViewer * viewer = (SoQtExaminerViewer *)userdata;

    const SoEvent * event = keyboardcb->getEvent();

    int shift = 0;

    if (SO_KEY_PRESS_EVENT(event, U)) {
      shift = 40;
      keyboardcb->setHandled();
    }
    else if (SO_KEY_PRESS_EVENT(event, D)) {
      shift = -40;
      keyboardcb->setHandled();
    }

    if (keyboardcb->isHandled()) {
      SbViewportRegion vpr = viewer->getViewportRegion();
      SbVec2s size = vpr.getViewportSizePixels();
      SbVec2s origin = vpr.getViewportOriginPixels();
      origin[1] -= shift;
      vpr.setViewportPixels(origin, size);
      viewer->setViewportRegion(vpr);
    }
  }

  // ************************************************************

  int
  main(int argc, char ** argv)
  {
    QWidget * window = SoQt::init(argv[0]);

    SoSeparator * root = new SoSeparator;
    root->ref();

    root->addChild(new SoCone);

    SoQtExaminerViewer * viewer = new SoQtExaminerViewer(window);

    SoEventCallback * eventcb = new SoEventCallback;
    eventcb->addEventCallback(SoKeyboardEvent::getClassTypeId(),
                              keypresscbfunc, viewer);
    root->insertChild(eventcb, 0);

    viewer->setViewing(FALSE);
    viewer->setDecoration(FALSE);
    viewer->setSceneGraph(root);
    viewer->show();
    SoQt::show(window);

    SoQt::mainLoop();

    delete viewer;
    root->unref();
    return 0;
  }
  \endcode

  \sa SbViewVolume
*/
// FIXME: should do a simple illustration in the class documentation
// of the "viewport-within-a-window" concept which this class
// represents. 20020103 mortene.

#include <cassert>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/errors/SoDebugError.h>

/*!
  The default SbViewportRegion constructor initializes the viewport to
  fully cover a [100, 100] size window with 72 pixels per inch
  resolution.
*/
SbViewportRegion::SbViewportRegion(void)
  : winsize(100, 100),
    vporigin(0.0f, 0.0f),
    vpsize(1.0f, 1.0f)
{
  this->pixperinch = 72.0f;
}

/*!
  Construct and initialize an SbViewportRegion instance with the given
  pixel value window dimensions. The viewport within this window will
  be set to cover the window completely.
*/
SbViewportRegion::SbViewportRegion(short width, short height)
  : winsize(width, height),
    vporigin(0,0),
    vpsize(1.0f, 1.0f)
{
#if COIN_DEBUG
  if (width<0) {
    SoDebugError::postWarning("SbViewportRegion::SbViewportRegion",
                              "width (%d) should be >=0. Clamped to 0.",
                              width);
    winsize[0]=0;
  }
  if (height<0) {
    SoDebugError::postWarning("SbViewportRegion::SbViewportRegion",
                              "height (%d) should be >=0. Clamped to 0.",
                              height);
    winsize[1]=0;
  }
#endif // COIN_DEBUG

  this->pixperinch = 72.0f;
}

/*!
  Construct and initialize an SbViewportRegion instance with the given
  pixel value window dimensions. The viewport within this window will
  be set to cover the window completely.
*/
SbViewportRegion::SbViewportRegion(SbVec2s winsizearg)
  : winsize(winsizearg),
    vporigin(0,0),
    vpsize(1.0f, 1.0f)
{
#if COIN_DEBUG
  if (winsizearg[0]<0) {
    SoDebugError::postWarning("SbViewportRegion::SbViewportRegion",
                              "winsize[0] (%d) should be >=0. Clamped to 0.",
                              winsizearg[0]);
    this->winsize[0]=0;
  }
  if (winsizearg[1]<0) {
    SoDebugError::postWarning("SbViewportRegion::SbViewportRegion",
                              "winsize[1] (%d) should be >=0. Clamped to 0.",
                              winsizearg[1]);
    this->winsize[1]=0;
  }
#endif // COIN_DEBUG

  this->pixperinch = 72.0f;
}

/*!
  Copy constructor.
*/
SbViewportRegion::SbViewportRegion(const SbViewportRegion & vpReg)
{
  *this = vpReg;
}

/*!
  Set the window size in pixels. The viewport rectangle dimensions
  will stay intact.

  \sa getWindowSize()
*/
void
SbViewportRegion::setWindowSize(short width, short height)
{
#if COIN_DEBUG
  if (width<0) {
    SoDebugError::postWarning("SbViewportRegion::setWindowSize",
                              "width (%d) should be >=0. Clamped to 0.",width);
    width=0;
  }
  if (height<0) {
    SoDebugError::postWarning("SbViewportRegion::setWindowSize",
                              "height (%d) should be >=0. Clamped to 0.",
                              height);
    height=0;
  }
#endif // COIN_DEBUG

  this->winsize.setValue(width, height);
}

/*!
  \overload
*/
void
SbViewportRegion::setWindowSize(SbVec2s winsizearg)
{
#if COIN_DEBUG
  if (winsizearg[0]<0) {
    SoDebugError::postWarning("SbViewportRegion::setWindowSize",
                              "winsize[0] (%d) should be >=0. Clamped to 0.",
                              winsizearg[0]);
    winsizearg[0]=0;
  }
  if (winsizearg[1]<0) {
    SoDebugError::postWarning("SbViewportRegion::setWindowSize",
                              "winsize[1] (%d) should be >=0. Clamped to 0.",
                              winsizearg[1]);
    winsizearg[1]=0;
  }
#endif // COIN_DEBUG

  this->setWindowSize(winsizearg[0], winsizearg[1]);
}

/*!
  Set up the origin and size of the viewport region in normalized
  coordinates.

  \sa getViewportOrigin(), getViewportSize(), setViewportPixels().
*/
void
SbViewportRegion::setViewport(float left, float bottom,
                              float width, float height)
{
#if COIN_DEBUG
  if (width<0) {
    SoDebugError::postWarning("SbViewportRegion::setViewport",
                              "width (%d) should be >=0. Clamped to 0.",width);
    width=0;
  }
  if (height<0) {
    SoDebugError::postWarning("SbViewportRegion::setViewport",
                              "height (%d) should be >=0. Clamped to 0.",
                              height);
    height=0;
  }
#endif // COIN_DEBUG

  this->vporigin.setValue(left, bottom);
  this->vpsize.setValue(width, height);
}

/*!
  \overload
*/
void
SbViewportRegion::setViewport(SbVec2f origin, SbVec2f size)
{
#if COIN_DEBUG
  if (size[0]<0) {
    SoDebugError::postWarning("SbViewportRegion::setViewport",
                              "size[0] (%d) should be >=0. Clamped to 0.",
                              size[0]);
    size[0]=0;
  }
  if (size[1]<0) {
    SoDebugError::postWarning("SbViewportRegion::setViewport",
                              "size[1] (%d) should be >=0. Clamped to 0.",
                              size[1]);
    size[1]=0;
  }
#endif // COIN_DEBUG

  this->setViewport(origin[0], origin[1], size[0], size[1]);
}

/*!
  Set up the origin and size of the viewport region in pixel
  coordinates.

  \sa getViewportOriginPixels(), getViewportSizePixels(), setViewport()
*/
void
SbViewportRegion::setViewportPixels(short left, short bottom,
                                    short width, short height)
{
#if COIN_DEBUG
  if (width<0) {
    SoDebugError::postWarning("SbViewportRegion::setViewportPixels",
                              "width (%d) should be >=0. Clamped to 0.",width);
    width=0;
  }
  if (height<0) {
    SoDebugError::postWarning("SbViewportRegion::setViewportPixels",
                              "height (%d) should be >=0. Clamped to 0.",
                              height);
    height=0;
  }
#endif // COIN_DEBUG

  this->vporigin.setValue(static_cast<float>(left)/static_cast<float>(this->winsize[0]),
                          static_cast<float>(bottom)/static_cast<float>(this->winsize[1]));
  this->vpsize.setValue(static_cast<float>(width)/static_cast<float>(this->winsize[0]),
                        static_cast<float>(height)/static_cast<float>(this->winsize[1]));
}

/*!
  \overload
*/
void
SbViewportRegion::setViewportPixels(SbVec2s origin, SbVec2s size)
{
#if COIN_DEBUG
  if (size[0]<0) {
    SoDebugError::postWarning("SbViewportRegion::setViewportPixels",
                              "size[0] (%d) should be >=0. Clamped to 0.",
                              size[0]);
    size[0]=0;
  }
  if (size[1]<0) {
    SoDebugError::postWarning("SbViewportRegion::setViewportPixels",
                              "size[1] (%d) should be >=0. Clamped to 0.",
                              size[1]);
    size[1]=0;
  }
#endif // COIN_DEBUG
  this->setViewportPixels(origin[0], origin[1], size[0], size[1]);
}

/*!
  Returns window dimensions (which are in absolute (i.e. pixel)
  coordinates).

  \sa setWindowSize().
 */
const SbVec2s&
SbViewportRegion::getWindowSize(void) const
{
  return this->winsize;
}

/*!
  Return normalized viewport origin coordinates.

  \sa setViewport(), getViewportOriginPixels().
 */
const SbVec2f &
SbViewportRegion::getViewportOrigin(void) const
{
  return this->vporigin;
}

/*
  Rounds off the given fractional number to the nearest short integer.
*/
static inline short
round2short(float a)
{
  if (a == static_cast<float>(short(a))) return short(a);
  else return (a>0.0f) ? short(a+0.5f) : -short(0.5f-a);
}

/*!
  Return viewport origin coordinates in pixel values.

  \sa setViewportPixels(), getViewportOrigin().
 */
const SbVec2s &
SbViewportRegion::getViewportOriginPixels(void) const
{
  // Cast away constness. Ugly.
  SbViewportRegion * thisp = const_cast<SbViewportRegion *>(this);
  // FIXME: the tmp storage seems totally unnecessary? 20040213 mortene.
  thisp->vporigin_s.setValue(round2short(this->winsize[0] * this->vporigin[0]),
                             round2short(this->winsize[1] * this->vporigin[1]));
  return this->vporigin_s;
}

/*!
  Returns the normalized viewport size.

  \sa setViewport(), getViewportSizePixels().
 */
const SbVec2f &
SbViewportRegion::getViewportSize(void) const
{
  return this->vpsize;
}

/*!
  Returns viewport size in pixel coordinates.

  \sa setViewportPixels(), getViewportSize().
 */
const SbVec2s &
SbViewportRegion::getViewportSizePixels(void) const
{
  // Cast away constness. Ugly.
  SbViewportRegion * thisp = const_cast<SbViewportRegion *>(this);
  // FIXME: the tmp storage seems totally unnecessary? 20040213 mortene.
  thisp->vpsize_s = SbVec2s(round2short(this->winsize[0] * this->vpsize[0]),
                            round2short(this->winsize[1] * this->vpsize[1]));
  return this->vpsize_s;
}

/*!
  Returns the aspect ratio of the viewport region. The aspect ratio is
  calculated as pixelwidth divided on pixelheight.
 */
float
SbViewportRegion::getViewportAspectRatio(void) const
{
  SbVec2s size = this->getViewportSizePixels();
  if (size[1] == 0) return 1.0;
  return float(size[0])/float(size[1]);
}

/*!
  Scale the width of the viewport region.

  The scale factor should not make the viewport larger than the
  window. If this happens, the viewport will be clamped.

  The scaling will be done around the viewport region center point, but
  if this causes the viewport origin to be moved below (0,0), the
  origin coordinates will be clamped.

  \sa scaleHeight().
*/
void
SbViewportRegion::scaleWidth(float ratio)
{
#if COIN_DEBUG
  if (ratio<0.0f) {
    SoDebugError::postWarning("SbViewportRegion::scaleWidth",
                              "ratio (%f) should be >=0.0f. Clamped to 0.0f.",
                              ratio);
    ratio=0.0f;
  }
#endif // COIN_DEBUG

  float oldw = this->vpsize[0];
  this->vpsize[0] *= ratio;
  this->vporigin[0] -= (this->vpsize[0] - oldw) / 2.0f;

  if (this->vpsize[0] > 1.0f) this->vpsize[0] = 1.0f;
  if (this->vporigin[0] < 0.0f) this->vporigin[0] = 0.0f;
}

/*!
  Scale the height of the viewport region.

  The scale factor should not make the viewport larger than the
  window. If this happens, the viewport will be clamped.

  The scaling will be done around the viewport region center point, but
  if this causes the viewport origin to be moved below (0,0), the
  origin coordinates will be clamped.

  \sa scaleWidth().  */
void
SbViewportRegion::scaleHeight(float ratio)
{
#if COIN_DEBUG
  if (ratio<0.0f) {
    SoDebugError::postWarning("SbViewportRegion::scaleheight",
                              "ratio (%f) should be >=0.0f. Clamped to 0.0f.",
                              ratio);
    ratio=0.0f;
  }
#endif // COIN_DEBUG

  float oldh = this->vpsize[1];
  this->vpsize[1] *= ratio;
  this->vporigin[1] -= (this->vpsize[1] - oldh) / 2.0f;

  if(this->vpsize[1] > 1.0f) this->vpsize[1] = 1.0f;
  if(this->vporigin[1] < 0.0f) this->vporigin[1] = 0.0f;
}

/*!
  Set pixels per inch. Default value is 72.

  \sa getPixelsPerInch().
 */
void
SbViewportRegion::setPixelsPerInch(float ppi)
{
#if COIN_DEBUG
  if (ppi<0.0f) {
    SoDebugError::postWarning("SbViewportRegion::setPixelsPerInch",
                              "ppi value (%f) should be >=0.0f. "
                              "Clamped to 0.0f.",ppi);
    ppi=0.0f;
  }
#endif // COIN_DEBUG

  this->pixperinch = ppi;
}

/*!
  Get pixels per inch.

  \sa setPixelsPerInch().
 */
float
SbViewportRegion::getPixelsPerInch(void) const
{
  return this->pixperinch;
}

/*!
  Get pixels per point. A \e point is defined as something you can put
  72 of per inch...

  \sa setPixelsPerInch(), getPixelsPerInch().
 */
float
SbViewportRegion::getPixelsPerPoint(void) const
{
  return this->pixperinch / 72.0f;
}

/*!
  \relates SbViewportRegion
  Compares two SbViewportRegion instances for equality.
*/
int
operator==(const SbViewportRegion & reg1, const SbViewportRegion & reg2)
{
  return
    reg1.winsize == reg2.winsize &&
    reg1.getViewportOriginPixels() == reg2.getViewportOriginPixels() &&
    reg1.getViewportSizePixels() == reg2.getViewportSizePixels() &&
    reg1.pixperinch == reg2.pixperinch;
}

/*!
  \relates SbViewportRegion
  Compares two SbViewportRegion instances for inequality.

  \since Coin 2.4
*/
int
operator!=(const SbViewportRegion & reg1, const SbViewportRegion & reg2)
{
  return !(reg1 == reg2);
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbViewportRegion::print(FILE * fp) const
{
#if COIN_DEBUG
  (void)fprintf( fp, "  winsize:     " );
  this->getWindowSize().print(fp);
  (void)fprintf( fp, "\n" );
  (void)fprintf( fp, "  vporigin:    " );
  this->getViewportOrigin().print(fp);
  (void)fprintf( fp, "\n" );
  (void)fprintf( fp, "  vporiginpix: " );
  this->getViewportOriginPixels().print(fp);
  (void)fprintf( fp, "\n" );
  (void)fprintf( fp, "  vpsize:      " );
  this->getViewportSize().print(fp);
  (void)fprintf( fp, "\n" );
  (void)fprintf( fp, "  vpsizepix:   " );
  this->getViewportSizePixels().print(fp);
  (void)fprintf( fp, "\n" );
  (void)fprintf( fp, "  aspectratio: %f\n", this->getViewportAspectRatio() );
  (void)fprintf( fp, "  ppi:         %f\n", this->getPixelsPerInch() );
  (void)fprintf( fp, "  ppp:         %f\n", this->getPixelsPerPoint() );
#endif // COIN_DEBUG
}
