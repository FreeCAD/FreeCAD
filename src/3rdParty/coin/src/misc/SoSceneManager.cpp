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
  \class SoSceneManager SoSceneManager.h Inventor/SoSceneManager.h
  \brief The SoSceneManager class provides the main interface between the scene graph and the GUI toolkit.

  \ingroup coin_general

  The render area class from the GUI toolkit you are using uses this
  class as the interface against the scene graph. Event handling and
  providing "hooks" to do rendering are the main functions of the
  class.

  A Coin library instance within an application will typically contain
  a single SoSceneManager object. The pointer for this object is
  stored in the GUI render area class.
*/

// *************************************************************************

#include <Inventor/SoSceneManager.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/C/tidbits.h>
#include <Inventor/SoDB.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/misc/SoAudioDevice.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoInfo.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/sensors/SoOneShotSensor.h>
#include <Inventor/system/gl.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/SoEventManager.h>

#include "tidbitsp.h"
#include "misc/SoSceneManagerP.h"

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->publ)


// *************************************************************************

/*!
  \typedef SoSceneManagerRenderCB(void * userdata, SoSceneManager * mgr)

  Render callback function must have this signature.
*/

// *************************************************************************

/*!
  Constructor. Sets up default SoGLRenderAction and
  SoHandleEventAction instances.
 */
SoSceneManager::SoSceneManager(void)
{
  assert(SoDB::isInitialized() && "SoDB::init() has not been invoked");

  PRIVATE(this) = new SoSceneManagerP(this);

  PRIVATE(this)->rendercb = NULL;
  PRIVATE(this)->rendercbdata = NULL;

  PRIVATE(this)->scene = NULL;
  PRIVATE(this)->camera = NULL;

  PRIVATE(this)->rendermanager = new SoRenderManager;
  PRIVATE(this)->eventmanager = new SoEventManager;

  PRIVATE(this)->backgroundcolor.setValue(0.0f, 0.0f, 0.0f);
}

/*!
  Destructor.
 */
SoSceneManager::~SoSceneManager()
{
  this->setSceneGraph(NULL);
  
  if (PRIVATE(this)->camera) PRIVATE(this)->camera->unref();

  delete PRIVATE(this)->rendermanager;
  delete PRIVATE(this)->eventmanager;
  delete PRIVATE(this);
}

/*!
  Render the scene graph.

  If \a clearwindow is \c TRUE, clear the rendering buffer before
  drawing. If \a clearzbuffer is \c TRUE, clear the depth buffer
  values before rendering. Both of these arguments should normally be
  \c TRUE, but they can be set to \c FALSE to optimize for special
  cases (e.g. when doing wireframe rendering one doesn't need a depth
  buffer).
 */
void
SoSceneManager::render(const SbBool clearwindow, const SbBool clearzbuffer)
{
  PRIVATE(this)->rendermanager->render(clearwindow, clearzbuffer);
}

/*!
  Render method used for thread safe rendering.

  Since only one thread can use an SoGLRenderAction, this method
  enables you to supply your own thread-specific SoGLRenderAction to
  be used for rendering the scene.

  If \a initmatrices is \c TRUE, the OpenGL model and projection
  matrices will be initialized to identity matrices before applying
  the action.

  If \a clearwindow is \c TRUE, clear the rendering buffer before
  drawing. If \a clearzbuffer is \c TRUE, clear the depth buffer
  values before rendering.

  \COIN_FUNCTION_EXTENSION
  
  \since Coin 2.0
 */
void
SoSceneManager::render(SoGLRenderAction * action,
                       const SbBool initmatrices,
                       const SbBool clearwindow,
                       const SbBool clearzbuffer)
{
  PRIVATE(this)->rendermanager->render(action, initmatrices, clearwindow, clearzbuffer);
}

/*!
  Process the given event by applying an SoHandleEventAction on the
  scene graph.
 */
SbBool
SoSceneManager::processEvent(const SoEvent * const event)
{
  return PRIVATE(this)->eventmanager->processEvent(event);
}

/*!  
  Sets the camera to be used.
*/
void 
SoSceneManager::setCamera(SoCamera * camera)
{
  if (PRIVATE(this)->camera) {
    PRIVATE(this)->camera->unref();
  }
  PRIVATE(this)->camera = camera;
  if (camera) camera->ref();
  PRIVATE(this)->rendermanager->setCamera(PRIVATE(this)->camera);
  PRIVATE(this)->eventmanager->setCamera(PRIVATE(this)->camera);
}

/*!
  Returns the current camera.
*/
SoCamera * 
SoSceneManager::getCamera(void) const
{
  return PRIVATE(this)->camera;
}

/*!
  Reinitialize after parameters affecting the OpenGL context have
  changed.
*/
void
SoSceneManager::reinitialize(void)
{
  PRIVATE(this)->rendermanager->reinitialize();
}

/*!
  Redraw at first opportunity as system becomes idle.

  Multiple calls to this method before an actual redraw has taken
  place will only result in a single redraw of the scene.
*/
void
SoSceneManager::scheduleRedraw(void)
{
  PRIVATE(this)->rendermanager->scheduleRedraw();
}

/*!
  Returns the \e active flag.
 */
int
SoSceneManager::isActive(void) const
{
  return PRIVATE(this)->rendermanager->isActive();
}

/*!
  Do an immediate redraw by calling the redraw callback function.
 */
void
SoSceneManager::redraw(void)
{
  PRIVATE(this)->rendermanager->redraw();
}

/*!
  Set the node which is top of the scene graph we're managing.  The \a
  sceneroot node reference count will be increased by 1, and any
  previously set scene graph top node will have its reference count
  decreased by 1.

  \sa getSceneGraph()
*/
void
SoSceneManager::setSceneGraph(SoNode * const sceneroot)
{
  // Don't unref() until after we've set up the new root, in case the
  // old root == the new sceneroot. (Just to be that bit more robust.)
  SoNode * oldroot = PRIVATE(this)->scene;
  
  PRIVATE(this)->scene = sceneroot;

  PRIVATE(this)->rendermanager->setSceneGraph(sceneroot);
  PRIVATE(this)->eventmanager->setSceneGraph(sceneroot);

  if (PRIVATE(this)->scene) {
    PRIVATE(this)->scene->ref();
    this->setCamera(PRIVATE(this)->searchForCamera(PRIVATE(this)->scene));
  } else {
    this->setCamera(NULL);
  }
  
  if (oldroot) oldroot->unref();
}

/*!
  Returns pointer to root of scene graph.
 */
SoNode *
SoSceneManager::getSceneGraph(void) const
{
  return PRIVATE(this)->scene;
}
/*!
  Update window size of our SoGLRenderAction's viewport settings.

  Note that this will \e only change the information about window
  dimensions, the actual viewport size and origin (i.e. the rectangle
  which redraws are confined to) will stay the same.

  \sa setViewportRegion()
*/
void
SoSceneManager::setWindowSize(const SbVec2s & newsize)
{
  PRIVATE(this)->rendermanager->setWindowSize(newsize);
}

/*!
  Returns the current render action window size.

  \sa setWindowSize()
*/
const SbVec2s &
SoSceneManager::getWindowSize(void) const
{
  return PRIVATE(this)->rendermanager->getWindowSize();
}

/*!
  Set size of rendering area for the viewport within the current
  window.
*/
void
SoSceneManager::setSize(const SbVec2s & newsize)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoSceneManager::setSize",
                         "(%d, %d)", newsize[0], newsize[1]);
#endif // debug

  PRIVATE(this)->rendermanager->setSize(newsize);
  PRIVATE(this)->eventmanager->setSize(newsize);
}

/*!
  Returns size of render area.
 */
const SbVec2s &
SoSceneManager::getSize(void) const
{
  return PRIVATE(this)->rendermanager->getSize();
}

/*!
  Set \e only the origin of the viewport region within the rendering
  window.

  \sa setViewportRegion(), setWindowSize()
*/
void
SoSceneManager::setOrigin(const SbVec2s & newOrigin)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoSceneManager::setOrigin",
                         "(%d, %d)", newOrigin[0], newOrigin[1]);
#endif // debug

  PRIVATE(this)->rendermanager->setOrigin(newOrigin);
  PRIVATE(this)->eventmanager->setOrigin(newOrigin);
}

/*!
  Returns origin of rendering area viewport.

  \sa setOrigin()
*/
const SbVec2s &
SoSceneManager::getOrigin(void) const
{
  return PRIVATE(this)->rendermanager->getOrigin();
}

/*!
  Update our SoGLRenderAction's viewport settings.

  This will change \e both the information about window dimensions and
  the actual viewport size and origin.

  \sa setWindowSize()
*/
void
SoSceneManager::setViewportRegion(const SbViewportRegion & newregion)
{
#if COIN_DEBUG && 0 // debug
  const SbVec2s & ws = newregion.getWindowSize();
  const SbVec2s & vpop = newregion.getViewportOriginPixels();
  const SbVec2s & vpsp = newregion.getViewportSizePixels();
  SoDebugError::postInfo("SoSceneManager::setViewportRegion",
                         "windowsize=(%d, %d) "
                         "viewportorigin=(%d, %d) "
                         "viewportsize=(%d, %d) ",
                         ws[0], ws[1],
                         vpop[0], vpop[1],
                         vpsp[0], vpsp[1]);
#endif // debug

  PRIVATE(this)->rendermanager->setViewportRegion(newregion);
  PRIVATE(this)->eventmanager->setViewportRegion(newregion);
}

/*!
  Returns current viewport region used by the render action and the
  event handling.

  \sa setViewportRegion()
*/
const SbViewportRegion &
SoSceneManager::getViewportRegion(void) const
{
  return PRIVATE(this)->rendermanager->getViewportRegion();
}

/*!
  Sets color of rendering canvas.
 */
void
SoSceneManager::setBackgroundColor(const SbColor & color)
{
  PRIVATE(this)->rendermanager->setBackgroundColor(SbColor4f(color, 0.0));
}

/*!
  Returns color used for clearing the rendering area before rendering
  the scene.
 */
const SbColor &
SoSceneManager::getBackgroundColor(void) const
{
  SbColor4f bgcolor = PRIVATE(this)->rendermanager->getBackgroundColor();
  PRIVATE(this)->backgroundcolor = SbColor(bgcolor[0], bgcolor[1], bgcolor[2]);
  return PRIVATE(this)->backgroundcolor;
}

/*!
  Set index of background color in the color lookup table if rendering
  in color index mode.

  Note: color index mode is not supported yet in Coin.
 */
void
SoSceneManager::setBackgroundIndex(const int index)
{
  PRIVATE(this)->rendermanager->setBackgroundIndex(index);
}

/*!
  Returns index of colormap for background filling.

  \sa setBackgroundIndex()
 */
int
SoSceneManager::getBackgroundIndex(void) const
{
  return PRIVATE(this)->rendermanager->getBackgroundIndex();
}

/*!
  Turn RGB truecolor mode on or off. If you turn truecolor mode off,
  colorindex mode will be used instead.
*/
void
SoSceneManager::setRGBMode(const SbBool yes)
{
  PRIVATE(this)->rendermanager->setRGBMode(yes);
}

/*!
  Returns the "truecolor or colorindex" mode flag.
 */
SbBool
SoSceneManager::isRGBMode(void) const
{
  return PRIVATE(this)->rendermanager->isRGBMode();
}

/*!
  Activate rendering and event handling. Default is \c off.
 */
void
SoSceneManager::activate(void)
{
  PRIVATE(this)->rendermanager->activate();
}

/*!
  Deactivate rendering and event handling.
 */
void
SoSceneManager::deactivate(void)
{
  PRIVATE(this)->rendermanager->deactivate();
}

/*!
  Set the callback function \a f to invoke when rendering the
  scene. \a userdata will be passed as the first argument of the
  function.
 */
void
SoSceneManager::setRenderCallback(SoSceneManagerRenderCB * f,
                                  void * const userdata)
{
  PRIVATE(this)->rendercb = f;
  PRIVATE(this)->rendercbdata = userdata;
  PRIVATE(this)->rendermanager->setRenderCallback(SoSceneManagerP::renderCB, PRIVATE(this));
}

/*!
  Returns \c TRUE if the SoSceneManager automatically redraws the
  scene upon detecting changes in the scene graph.

  The automatic redraw is turned on and off by setting either a valid
  callback function with setRenderCallback(), or by passing \c NULL.
 */
SbBool
SoSceneManager::isAutoRedraw(void) const
{
  return PRIVATE(this)->rendermanager->isAutoRedraw();
}

/*!
  Set up the redraw \a priority for the SoOneShotSensor used to
  trigger redraws. By setting this lower than for your own sensors,
  you can make sure some code is always run before redraw happens.

  \sa SoDelayQueueSensor
 */
void
SoSceneManager::setRedrawPriority(const uint32_t priority)
{
  PRIVATE(this)->rendermanager->setRedrawPriority(priority);
}

/*!
  Returns value of priority on the redraw sensor.
 */
uint32_t
SoSceneManager::getRedrawPriority(void) const
{
  return PRIVATE(this)->rendermanager->getRedrawPriority();
}

/*!
  Turn antialiased rendering on or off.

  See documentation for SoGLRenderAction::setSmoothing() and
  SoGLRenderAction::setNumPasses().
 */
void
SoSceneManager::setAntialiasing(const SbBool smoothing, const int numpasses)
{
  PRIVATE(this)->rendermanager->setAntialiasing(smoothing, numpasses);
}

/*!
  Returns rendering pass information.

  \sa setAntialiasing()
 */
void
SoSceneManager::getAntialiasing(SbBool & smoothing, int & numpasses) const
{
  PRIVATE(this)->rendermanager->getAntialiasing(smoothing, numpasses);
}

/*!
  Set the \a action to use for rendering. Overrides the default action
  made in the constructor.
 */
void
SoSceneManager::setGLRenderAction(SoGLRenderAction * const action)
{
  PRIVATE(this)->rendermanager->setGLRenderAction(action);
}

/*!
  Returns pointer to render action.
 */
SoGLRenderAction *
SoSceneManager::getGLRenderAction(void) const
{
  return PRIVATE(this)->rendermanager->getGLRenderAction();
}

/*!
  Set the \a action to use for rendering audio. Overrides the default action
  made in the constructor.
 */
void
SoSceneManager::setAudioRenderAction(SoAudioRenderAction * const action)
{
  PRIVATE(this)->rendermanager->setAudioRenderAction(action);
}

/*!
  Returns pointer to audio render action.
 */
SoAudioRenderAction *
SoSceneManager::getAudioRenderAction(void) const
{
  return PRIVATE(this)->rendermanager->getAudioRenderAction();
}

/*!
  Set the \a action to use for event handling. Overrides the default
  action made in the constructor.
 */
void
SoSceneManager::setHandleEventAction(SoHandleEventAction * hea)
{
  PRIVATE(this)->eventmanager->setHandleEventAction(hea);
}

/*!
  Returns pointer to event handler action.
 */
SoHandleEventAction *
SoSceneManager::getHandleEventAction(void) const
{
//   return PRIVATE(this)->handleeventaction;
  return PRIVATE(this)->eventmanager->getHandleEventAction();
}

/*!
  Returns the default priority of the redraw sensor.

  \sa SoDelayQueueSensor, setRedrawPriority()
 */
uint32_t
SoSceneManager::getDefaultRedrawPriority(void)
{
  return SoRenderManager::getDefaultRedrawPriority();
}

/*!
  Set whether or not for SoSceneManager instances to "touch" the
  global \c realTime field after a redraw. If this is not done,
  redrawing when animating the scene will only happen as fast as the
  \c realTime interval goes (which defaults to 12 times a second).

  \sa SoDB::setRealTimeInterval()
 */
void
SoSceneManager::enableRealTimeUpdate(const SbBool flag)
{
  SoRenderManager::enableRealTimeUpdate(flag);
}

/*!
  Returns whether or not we automatically notifies everything
  connected to the \c realTime field after a redraw.
 */
SbBool
SoSceneManager::isRealTimeUpdateEnabled(void)
{
  return SoRenderManager::isRealTimeUpdateEnabled();
}

#undef PRIVATE
#undef PUBLIC
