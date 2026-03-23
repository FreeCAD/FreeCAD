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
  \class SoOffscreenRenderer SoOffscreenRenderer.h Inventor/SoOffscreenRenderer.h
  \brief The SoOffscreenRenderer class is used for rendering scenes to offscreen buffers.

  \ingroup coin_general

  If you want to render to a memory buffer instead of an on-screen
  OpenGL context, use this class. Rendering to a memory buffer can be
  used to generate texture maps on-the-fly, or for saving snapshots of
  the scene to disk files (as pixel bitmaps or as PostScript files for
  sending to a PostScript capable printer).

  Here's a dead simple usage example, just the code directly related
  to the SoOffscreenRenderer:

  \code
  SoOffscreenRenderer myRenderer(vpregion);
  SoNode * root = myViewer->getSceneManager()->getSceneGraph();
  SbBool ok = myRenderer.render(root);
  unsigned char * imgbuffer = myRenderer.getBuffer();
  // [then use image buffer in a texture, or write it to file, or whatever]
  \endcode

  And here a complete standalone example with a moving camera saving multiple
  frames to disk as JPGs:

  \code
  #include <Inventor/SoDB.h>
  #include <Inventor/SoOffscreenRenderer.h>
  #include <Inventor/engines/SoInterpolateVec3f.h>
  #include <Inventor/nodes/SoCube.h>
  #include <Inventor/nodes/SoDirectionalLight.h>
  #include <Inventor/nodes/SoPerspectiveCamera.h>
  #include <Inventor/nodes/SoSeparator.h>

  #include <iostream>

  int main()
  {
    // Init Coin
    SoDB::init();

    // The root node
    SoSeparator * root = new SoSeparator;
    root->ref();

    // It is mandatory to have at least one light for the offscreen renderer
    SoDirectionalLight * light = new SoDirectionalLight;
    root->addChild(light);

    // It is mandatory to have at least one camera for the offscreen renderer
    SoPerspectiveCamera * camera = new SoPerspectiveCamera;
    SbRotation cameraRotation = SbRotation::identity();
    cameraRotation *= SbRotation(SbVec3f(1, 0, 0), -0.4f);
    cameraRotation *= SbRotation(SbVec3f(0, 1, 0), 0.4f);
    camera->orientation = cameraRotation;
    root->addChild(camera);

    // Something to show... A box
    SoCube * cube = new SoCube;
    root->addChild(cube);

    // Set up the two camera positions we want to move the camera between
    SoInterpolateVec3f * interpolate = new SoInterpolateVec3f;
    interpolate->input0 = SbVec3f(2, 2, 9);
    interpolate->input1 = SbVec3f(2, 2, 5);
    camera->position.connectFrom(&interpolate->output);

    // Set up the offscreen renderer
    SbViewportRegion vpRegion(400, 300);
    SoOffscreenRenderer offscreenRenderer(vpRegion);

    // How many frames to render for the video
    int frames = 5;
    std::cout << "Writing " << frames << " frames..." << std::endl;

    for (int i = 0; i < frames; i++) {
      // Update the camera position
      interpolate->alpha = float(i) / (frames - 1);

      // Render the scene
      SbBool ok = offscreenRenderer.render(root);

      // Save the image to disk
      SbString filename = SbString("coinvideo-") + (i + 1) + ".jpg";
      if (ok) {
        offscreenRenderer.writeToFile(filename.getString(), "jpg");
      } else {
        std::cout << "Error saving image: " << filename.getString() << std::endl;
        break;
      }
    }

    std::cout << "Done!" << std::endl;

    root->unref();
    return 0;
  }
  \endcode


  Note that the SoOffscreenRenderer potentially allocates a fairly
  large amount of resources, both OpenGL and general system resources,
  for each instance. You will therefore be well advised to try to
  reuse SoOffscreenRenderer instances, instead of constructing and
  destructing a new instance e.g. for each frame when generating
  pictures for video.

  Offscreen rendering is internally done through either a GLX
  offscreen context (i.e. OpenGL on X11), WGL (i.e. OpenGL on
  Win32), AGL (old-style OpenGL on the Mac OS X) or CGL (new-style Mac OS X).

  If the OpenGL driver supports the pbuffer extension, it is detected
  and used to provide hardware accelerated offscreen rendering.

  The pixel data is fetched from the OpenGL buffer with glReadPixels(),
  with the format and type arguments set to GL_RGBA and
  GL_UNSIGNED_BYTE, respectively. This means that the maximum
  resolution is 32 bits, 8 bits for each of the R/G/B/A components.


  One particular usage of the SoOffscreenRenderer is to make it render
  frames to be used for the construction of movies. The general
  technique for doing this is to iterate over the following actions:

  <ul>
  <li>move camera to correct position for frame</li>
  <li>update the \c realTime global field (see explanation below)</li>
  <li>invoke the SoOffscreenRenderer</li>
  <li>dump rendered scene to file</li>
  </ul>

  ..then you use some external tool or library to construct the movie
  file, for instance in MPEG format, from the set of files dumped to
  disk from the iterative process above.

  The code would go something like the following (pseudo code
  style). First we need to stop the Coin library itself from doing any
  automatic updating of the \c realTime field, so your application
  initialization for Coin should look something like:


  \code
   [...] = SoQt::init([...]); // or SoWin::init() or SoDB::init()
   // ..and then immediately:

   // Control realTime field ourselves, so animations within the scene
   // follows "movie-time" and not "wallclock-time".
   SoDB::enableRealTimeSensor(FALSE);
   SoSceneManager::enableRealTimeUpdate(FALSE);
   SoSFTime * realtime = SoDB::getGlobalField("realTime");
   realtime->setValue(0.0);
  \endcode

  Note that it is important that the \c realTime field is initialized
  to \e your start-time \e before setting up any engines or other
  entities in the system that uses the \c realTime field.

  Then for the rendering loop, something like:

  \code
   for (int i=0; i < NRFRAMES; i++) {
     // [...reposition camera here, if necessary...]

     // render
     offscreenrend->render(root);

     // dump to file
     SbString framefile;
     framefile.sprintf("frame%06d.rgb", i);
     offscreenrend->writeToRGB(framefile.getString());

     // advance "current time" by the frames-per-second value, which
     // is 24 fps in this example
     realtime->setValue(realtime.getValue() + 1/24.0);
   }
  \endcode

  When making movies you need to write your application control code
  to take care of moving the camera along the correct trajectory
  yourself, and to explicitly control the global \c realTime field.
  The latter is so you're able to "step" with appropriate time units
  for each render operation (e.g. if you want a movie that has a 24
  FPS refresh rate, first render with \c realTime=0.0, then add 1/24s
  to the \c realTime field, render again to a new frame, add another
  1/24s to the \c realTime field, render, and so on).

  For further information about how to control the \c realTime field,
  see documentation of SoDB::getGlobalField(),
  SoDB::enableRealTimeSensor(), and
  SoSceneManager::enableRealTimeUpdate().

  If you want to use this class to create snapshots of your current
  viewer's view, but want to control the size of the snapshot, you
  need to modify the camera a bit while rendering to be sure that
  everything you see in the current view is visible in the snapshot.

  Below you'll find some pseudo code that does this. There are
  probably other ways to do this as well.

  \code
  void render_offscreen(const SbVec2s size)
  {
    SbVec2s glsize = this->getGLSize(); // size of your normal viewer
    float glar = float(glsize[0] / float(glsize[1]));
    float ar = float(size[0]) / float(size[1]);
    SoCamera * camera = this->getCamera(); // the camera you're using
    SoCamera::ViewportMapping oldmap = (SoCamera::ViewportMapping)
      camera->viewportMapping.getValue();
    float oldar = camera->aspectRatio.getValue();

    camera->viewportMapping = SoCamera::LEAVE_ALONE;
    camera->aspectRatio = ar;

    float scaleheight = 1.0f;
    if (glar > ar) {
      scaleheight = glar / ar;
      camera->scaleHeight(scaleheight);
    }
    else {
      scaleheight = ar / glar;
      camera->scaleHeight(scaleheight);
    }
    SoOffscreenRenderer * renderer = new SoOffscreenRenderer(size);
    renderer->render(root);

    // ... save image

    // restore camera
    camera->viewportMapping = oldmap;
    camera->aspectRatio = oldar;

    if (scaleheight != 1.0f) {
      camera->scaleHeight(1.0f / scaleheight);
    }
  }
  \endcode

*/

// As first mentioned to me by kyrah, the functionality of this class
// should really have been outside the core Coin library, seeing how
// it makes heavy use of window-system specifics. To be SGI Inventor
// compatible we need it to be part of the Coin API, though.
//
// mortene.

// *************************************************************************

// FIXME: we don't set up and render to RGBA-capable OpenGL-contexts,
// even when the requested format from the app-programmer is
// RGBA.
//
// I think this is what we should do:
//
//        1) first, try to get hold of a p-buffer with destination
//        alpha (p-buffers are faster to render into, as they can take
//        advantage of hardware acceleration)
//
//        2) failing that, try to make WGL/GLX/AGL/CGL set up a buffer
//        with destination alpha
//
//        3) failing that, get hold of either a p-buffer or a straight
//        WGL buffer with only RGB (no destination alpha -- this
//        should never fail), and do post-processing on the rendered
//        scene pixel-by-pixel to convert it into an RGBA texture
//
// 20020604 mortene.
//
// UPDATE 20041111 mortene: TGS Inventor has a new set of classes,
// e.g. "SoGLGraphicConfigTemplate", which makes it possible to set up
// wanted attributes with GL contexts. Audit their interface and
// implement, if well designed.

// *************************************************************************

#include <Inventor/SoOffscreenRenderer.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cassert>
#include <cstring> // memset(), memcpy()
#include <cmath> // for ceil()
#include <climits> // SHRT_MAX

#include <Inventor/C/glue/gl.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoPath.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoContextHandler.h>
#include <Inventor/misc/SoGLBigImage.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/system/gl.h>
#include <Inventor/SbTime.h>

#include "glue/simage_wrapper.h"
#include "tidbitsp.h"
#include "coindefs.h" // COIN_STUB()

// *************************************************************************

#include "CoinOffscreenGLCanvas.h"

#ifdef HAVE_GLX
#include "SoOffscreenGLXData.h"
#endif // HAVE_GLX

#ifdef COIN_MACOS_10
#include "SoOffscreenCGData.h"
#endif // COIN_MACOS_10

#ifdef HAVE_WGL
#include "SoOffscreenWGLData.h"
#endif // HAVE_WGL

// *************************************************************************

/*!
  \enum SoOffscreenRenderer::Components

  Enumerated values for the available image formats.

  \sa setComponents()
*/

// *************************************************************************

class SoOffscreenRendererP {
public:
  SoOffscreenRendererP(SoOffscreenRenderer * masterptr,
                       const SbViewportRegion & vpr,
                       SoGLRenderAction * glrenderaction = NULL)
  {
    this->master = masterptr;
    this->didreadbuffer = TRUE;

    this->backgroundcolor.setValue(0,0,0);
    this->components = SoOffscreenRenderer::RGB;
    this->buffer = NULL;
    this->bufferbytesize = 0;
    this->lastnodewasacamera = FALSE;
	
    if (glrenderaction) {
      this->renderaction = glrenderaction;
    }
    else {
      this->renderaction = new SoGLRenderAction(vpr);
      this->renderaction->setCacheContext(SoGLCacheContextElement::getUniqueCacheContext());
      this->renderaction->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
    }

    this->didallocation = glrenderaction ? FALSE : TRUE;
    this->viewport = vpr;
	this->useDC = false;
  }

  ~SoOffscreenRendererP()
  {
    if (this->didallocation) { delete this->renderaction; }
  }

  static SbBool offscreenContextsNotSupported(void);

  static const char * debugTileOutputPrefix(void);

  static SoGLRenderAction::AbortCode GLRenderAbortCallback(void *userData);
  SbBool renderFromBase(SoBase * base);

  void setCameraViewvolForTile(SoCamera * cam);

  static SbBool writeToRGB(FILE * fp, unsigned int w, unsigned int h,
                           unsigned int nrcomponents, const uint8_t * imgbuf);

  SbViewportRegion viewport;
  SbColor backgroundcolor;
  SoOffscreenRenderer::Components components;
  SoGLRenderAction * renderaction;
  SbBool didallocation;

  void updateDCBitmap();
  SbBool useDC;

  unsigned char * buffer;
  size_t bufferbytesize;

  CoinOffscreenGLCanvas glcanvas;
  int glcanvassize[2];

  int numsubscreens[2];
  // The subscreen size of the current tile. (Less than max if it is a
  // right- or bottom-border tile.)
  unsigned int subsize[2];
  // Keeps track of the current tile to be rendered.
  SbVec2s currenttile;

  SbBool lastnodewasacamera;
  SoCamera * visitedcamera;

  // used for lazy readPixels()
  SbBool didreadbuffer;
private:
  SoOffscreenRenderer * master;
};

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->master)

// *************************************************************************

// Set the environment variable below to get the individual tiles
// written out for debugging purposes. E.g.
//
//   $ export COIN_DEBUG_SOOFFSCREENRENDERER_TILEPREFIX="/tmp/offscreentile_"
//
// Tile X and Y position, plus the ".rgb" suffix, will be added when
// writing.
const char *
SoOffscreenRendererP::debugTileOutputPrefix(void)
{
  return coin_getenv("COIN_DEBUG_SOOFFSCREENRENDERER_TILEPREFIX");
}

// *************************************************************************

/*!
  Constructor. Argument is the \a viewportregion we should use when
  rendering. An internal SoGLRenderAction will be constructed.
*/
SoOffscreenRenderer::SoOffscreenRenderer(const SbViewportRegion & viewportregion)
{
  PRIVATE(this) = new SoOffscreenRendererP(this, viewportregion);
}

/*!
  Constructor. Argument is the \a action we should apply to the
  scene graph when rendering the scene. Information about the
  viewport is extracted from the \a action.
*/
SoOffscreenRenderer::SoOffscreenRenderer(SoGLRenderAction * action)
{
  PRIVATE(this) = new SoOffscreenRendererP(this, action->getViewportRegion(),
                                           action);
}

/*!
  Destructor.
*/
SoOffscreenRenderer::~SoOffscreenRenderer()
{
  delete[] PRIVATE(this)->buffer;
  delete PRIVATE(this);
}

/*!
  Returns the screen pixels per inch resolution of your monitor.
*/
float
SoOffscreenRenderer::getScreenPixelsPerInch(void)
{
  SbVec2f pixmmres(72.0f / 25.4f, 72.0f / 25.4f);
#ifdef HAVE_GLX
  pixmmres = SoOffscreenGLXData::getResolution();
#elif defined(HAVE_WGL)
  pixmmres = SoOffscreenWGLData::getResolution();
#elif defined(COIN_MACOS_10)
  pixmmres = SoOffscreenCGData::getResolution();
#endif // COIN_MACOS_10

  // The API-signature of this method is not what it should be: it
  // assumes the same resolution in the vertical and horizontal
  // directions.
  float pixprmm = (pixmmres[0] + pixmmres[1]) / 2.0f; // find average

  return pixprmm * 25.4f; // an inch is 25.4 mm.
}

/*!
  Get maximum dimensions (width, height) of the offscreen buffer.

  Note that from Coin version 2 onwards, the returned value will
  always be (\c SHRT_MAX, \c SHRT_MAX), where \c SHRT_MAX on most
  systems is equal to 32767.

  This because the SoOffscreenRenderer can in principle generate
  unlimited size offscreen canvases by tiling together multiple
  renderings of the same scene.
*/
SbVec2s
SoOffscreenRenderer::getMaximumResolution(void)
{
  return SbVec2s(SHRT_MAX, SHRT_MAX);
}

/*!
  Sets the component format of the offscreen buffer.

  If set to \c LUMINANCE, a grayscale image is rendered, \c
  LUMINANCE_TRANSPARENCY gives us a grayscale image with transparency,
  \c RGB will give us a 24-bit image with 8 bits each for the red,
  green and blue component, and \c RGB_TRANSPARENCY yields a 32-bit
  image (\c RGB plus transparency).

  The default format to render to is \c RGB.

  This will invalidate the current buffer, if any. The buffer will not
  contain valid data until another call to
  SoOffscreenRenderer::render() happens.
*/
void
SoOffscreenRenderer::setComponents(const Components components)
{
  PRIVATE(this)->components = components;
}

/*!
  Returns the component format of the offscreen buffer.

  \sa setComponents()
 */
SoOffscreenRenderer::Components
SoOffscreenRenderer::getComponents(void) const
{
  return PRIVATE(this)->components;

}

/*!
  Sets the viewport region.

  This will invalidate the current buffer, if any. The buffer will not
  contain valid data until another call to
  SoOffscreenRenderer::render() happens.
*/
void
SoOffscreenRenderer::setViewportRegion(const SbViewportRegion & region)
{
  PRIVATE(this)->viewport = region;
}

/*!
  Returns the viewport region.
*/
const SbViewportRegion &
SoOffscreenRenderer::getViewportRegion(void) const
{
  return PRIVATE(this)->viewport;
}

/*!
  Sets the background color. The buffer is cleared to this color
  before rendering.
*/
void
SoOffscreenRenderer::setBackgroundColor(const SbColor & color)
{
  PRIVATE(this)->backgroundcolor = color;
}

/*!
  Returns the background color.
*/
const SbColor &
SoOffscreenRenderer::getBackgroundColor(void) const
{
  return PRIVATE(this)->backgroundcolor;
}

/*!
  Sets the render action. Use this if you have special rendering needs.
*/
void
SoOffscreenRenderer::setGLRenderAction(SoGLRenderAction * action)
{
  if (action == PRIVATE(this)->renderaction) { return; }

  if (PRIVATE(this)->didallocation) { delete PRIVATE(this)->renderaction; }
  PRIVATE(this)->renderaction = action;
  PRIVATE(this)->didallocation = FALSE;
}

/*!
  Returns the rendering action currently used.
*/
SoGLRenderAction *
SoOffscreenRenderer::getGLRenderAction(void) const
{
  return PRIVATE(this)->renderaction;
}

// *************************************************************************

static void
pre_render_cb(void * COIN_UNUSED_ARG(userdata), SoGLRenderAction * action)
{
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  action->setRenderingIsRemote(FALSE);
}

// *************************************************************************

// Callback when rendering scene graph to subscreens. Detects when a
// camera has just been traversed, and then invokes the method which
// narrows the camera viewport according to the current tile we're
// rendering to.
//
// FIXME: if possible, it would be better to pick up from the state
// whatever data we're now grabbing directly from the SoCamera nodes.
// It'd be more robust, I believe, as the elements set by SoCamera can
// in principle also be set from other code. 20041006 mortene.
//
// UPDATE 20050711 mortene: on how to fix this properly, see item #121
// in Coin/BUGS.txt.
SoGLRenderAction::AbortCode
SoOffscreenRendererP::GLRenderAbortCallback(void *userData)
{
  SoOffscreenRendererP * thisp = (SoOffscreenRendererP *) userData;
  const SoFullPath * path = (const SoFullPath*) thisp->renderaction->getCurPath();
  SoNode * node = path->getTail();
  assert(node);

  if (thisp->lastnodewasacamera) {
    thisp->setCameraViewvolForTile(thisp->visitedcamera);
    thisp->lastnodewasacamera = FALSE;
  }

  if (node->isOfType(SoCamera::getClassTypeId())) {
    thisp->visitedcamera = (SoCamera *) node;
    thisp->lastnodewasacamera = TRUE;

    // FIXME: this is not really entirely sufficient. If a camera is
    // already within a cached list upon the first invocation of a
    // render pass, we'll never get a callback upon encountering it.
    //
    // This would be a fairly obscure case, though, as the glcache
    // would have to be set up in another context, compatible for
    // sharing GL data with the one set up internally by the
    // SoOffscreenRenderer -- which is very unlikely.
    //
    // 20050512 mortene.
    //
    // UPDATE 20050711 mortene: on how to fix this properly, see item
    // #121 in Coin/BUGS.txt. (The tile number should be in an
    // element, which the SoCamera would query (and thereby also make
    // the cache dependent on)).
    SoCacheElement::invalidate(thisp->renderaction->getState());
  }

  return SoGLRenderAction::CONTINUE;
}

// Collects common code from the two render() functions.
SbBool
SoOffscreenRendererP::renderFromBase(SoBase * base)
{
  if (SoOffscreenRendererP::offscreenContextsNotSupported()) {
    static SbBool first = TRUE;
    if (first) {
      SoDebugError::post("SoOffscreenRenderer::renderFromBase",
                         "SoOffscreenRenderer not compiled against any "
                         "window-system binding, it is defunct for this build.");
      first = FALSE;
    }
    return FALSE;
  }

  const SbVec2s fullsize = this->viewport.getViewportSizePixels();
  this->glcanvas.setWantedSize(fullsize);

  // check if no possible canvas size was found
  if (this->glcanvas.getActualSize() == SbVec2s(0, 0)) { return FALSE; }

  const uint32_t newcontext = this->glcanvas.activateGLContext();
  if (newcontext == 0) {
    SoDebugError::postWarning("SoOffscreenRenderer::renderFromBase",
                              "Could not set up an offscreen OpenGL context.");
    return FALSE;
  }

  const SbVec2s glsize = this->glcanvas.getActualSize();

  // We need to know the actual GL viewport size for tiled rendering,
  // in calculations when narrowing the camera view volume -- so we
  // store away this value for the "found a camera"-callback.
  //
  // FIXME: seems unnecessary now, should be able to just query
  // glcanvas.getActualSize() XXX
  this->glcanvassize[0] = glsize[0];
  this->glcanvassize[1] = glsize[1];

  if (CoinOffscreenGLCanvas::debug()) {
    SoDebugError::postInfo("SoOffscreenRendererP::renderFromBase",
                           "fullsize==<%d, %d>, glsize==<%d, %d>",
                           fullsize[0], fullsize[1], glsize[0], glsize[1]);
  }

  // oldcontext is used to restore the previous context id, in case
  // the render action is not allocated by us.
  const uint32_t oldcontext = this->renderaction->getCacheContext();
  this->renderaction->setCacheContext(newcontext);

  if (CoinOffscreenGLCanvas::debug()) {
    GLint colbits[4];
    glGetIntegerv(GL_RED_BITS, &colbits[0]);
    glGetIntegerv(GL_GREEN_BITS, &colbits[1]);
    glGetIntegerv(GL_BLUE_BITS, &colbits[2]);
    glGetIntegerv(GL_ALPHA_BITS, &colbits[3]);
    SoDebugError::postInfo("SoOffscreenRenderer::renderFromBase",
                           "GL context GL_[RED|GREEN|BLUE|ALPHA]_BITS=="
                           "[%d, %d, %d, %d]",
                           colbits[0], colbits[1], colbits[2], colbits[3]);
  }

  glEnable(GL_DEPTH_TEST);
  glClearColor(this->backgroundcolor[0],
               this->backgroundcolor[1],
               this->backgroundcolor[2],
               0.0f);

  // Make this large to get best possible quality on any "big-image"
  // textures (from using SoTextureScalePolicy).
  //
  // FIXME: this doesn't seem to be working, according to a report by
  // Colin Dunlop. See bug item #108. 20050509 mortene.
  //
  // UPDATE 20050711 mortene: the bug report referred to above may not
  // be correct. We should anyway fix this in a more appropriate
  // manner, for instance by setting up a new element with a boolean
  // value to indicate whether or not stuff should be rendered in
  // maximum quality. That would be generally useful for having better
  // control from the offscreenrenderer.
  const int bigimagechangelimit = SoGLBigImage::setChangeLimit(INT_MAX);

  // Deallocate old and allocate new target buffer, if necessary.
  //
  // If we need more space:
  const size_t bufsize =
    size_t(fullsize[0]) * size_t(fullsize[1]) * size_t(PUBLIC(this)->getComponents());
  SbBool alloc = (bufsize > this->bufferbytesize);
  // or if old buffer was much larger, free up the memory by fitting
  // to smaller size:
  alloc = alloc || (bufsize <= (this->bufferbytesize / 8));

  if (alloc) {
    delete[] this->buffer;
    this->buffer = new unsigned char[bufsize];
    this->bufferbytesize = bufsize;
  }

  if (SoOffscreenRendererP::debugTileOutputPrefix()) {
    (void)memset(this->buffer, 0x00, bufsize);
  }

  // needed to clear viewport after glViewport() is called from
  // SoGLRenderAction
  this->renderaction->addPreRenderCallback(pre_render_cb, NULL);

  // For debugging purposes, it has been made possible to use an
  // envvar to *force* tiled rendering even when it can be done in a
  // single chunk.
  //
  // (Note: don't use this envvar when using SoExtSelection nodes, for
  // the reason noted below.)
  static int forcetiled = -1;
  if (forcetiled == -1) {
    const char * env = coin_getenv("COIN_FORCE_TILED_OFFSCREENRENDERING");
    forcetiled = (env && (atoi(env) > 0)) ? 1 : 0;
    if (forcetiled) {
      SoDebugError::postInfo("SoOffscreenRendererP::renderFromBase",
                             "Forcing tiled rendering.");
    }
  }

  // FIXME: tiled rendering should be decided on the exact same
  // criteria as is used in SoExtSelection to decide which size to use
  // for its offscreen-buffer, as that node fails in VISIBLE_SHAPE
  // mode with tiled rendering. This is a weakness with SoExtSelection
  // which should be improved upon, if possible (i.e. fix
  // SoExtSelection, rather than adding some kind of "semi-private"
  // API to let SoExtSelection find out whether or not tiled rendering
  // is used). 20041028 mortene.
  const SbBool tiledrendering =
    forcetiled || (fullsize[0] > glsize[0]) || (fullsize[1] > glsize[1]);

  // Shall we use subscreen rendering or regular one-screen renderer?
  if (tiledrendering) {
    // we need to copy from GL to system memory if we're doing tiled rendering
    this->didreadbuffer = TRUE;

    for (int i=0; i < 2; i++) {
      this->numsubscreens[i] = (fullsize[i] + (glsize[i] - 1)) / glsize[i];
    }

    // We have to grab cameras using this callback during rendering
    this->visitedcamera = NULL;
    this->renderaction->setAbortCallback(SoOffscreenRendererP::GLRenderAbortCallback, this);

    // Render entire scene graph for each subscreen.
    for (int y=0; y < this->numsubscreens[1]; y++) {
      for (int x=0; x < this->numsubscreens[0]; x++) {
        this->currenttile = SbVec2s(x, y);

        // Find current "active" tilesize.
        this->subsize[0] = glsize[0];
        this->subsize[1] = glsize[1];
        if (x == (this->numsubscreens[0] - 1)) {
          this->subsize[0] = fullsize[0] % glsize[0];
          if (this->subsize[0] == 0) { this->subsize[0] = glsize[0]; }
        }
        if (y == (this->numsubscreens[1] - 1)) {
          this->subsize[1] = fullsize[1] % glsize[1];
          if (this->subsize[1] == 0) { this->subsize[1] = glsize[1]; }
        }

        SbViewportRegion subviewport = SbViewportRegion(SbVec2s(this->subsize[0], this->subsize[1]));
        this->renderaction->setViewportRegion(subviewport);

        if (base->isOfType(SoNode::getClassTypeId()))
          this->renderaction->apply((SoNode *)base);
        else if (base->isOfType(SoPath::getClassTypeId()))
          this->renderaction->apply((SoPath *)base);
        else {
          assert(FALSE && "Cannot apply to anything else than an SoNode or an SoPath");
        }

        const unsigned int nrcomp = PUBLIC(this)->getComponents();

        const int MAINBUF_OFFSET =
          (glsize[1] * y * fullsize[0] + glsize[0] * x) * nrcomp;

        const SbVec2s vpsize = subviewport.getViewportSizePixels();
        this->glcanvas.readPixels(this->buffer + MAINBUF_OFFSET,
                                  vpsize, fullsize[0], nrcomp);

        // Debug option to dump the (full) buffer after each
        // iteration.
        if (SoOffscreenRendererP::debugTileOutputPrefix()) {
          SbString s;
          s.sprintf("%s_%03d_%03d.rgb",
                    SoOffscreenRendererP::debugTileOutputPrefix(), x, y);

          FILE * f = fopen(s.getString(), "wb");
		  if (f) {
            SbBool w = SoOffscreenRendererP::writeToRGB(f, fullsize[0], fullsize[1],
                                                        nrcomp, this->buffer);
            assert(w);
            const int r = fclose(f);
            assert(r == 0);
		  }

          // This is sometimes useful to enable during debugging to
          // see the exact order and position of the tiles. Not
          // enabled by default because it makes the final buffer
          // completely blank.
#if 0 // debug
          (void)memset(this->buffer, 0x00, bufsize);
#endif // debug
        }
      }
    }

    this->renderaction->setAbortCallback(NULL, this);

    if (!this->visitedcamera) {
      SoDebugError::postWarning("SoOffscreenRenderer::renderFromBase",
                                "No camera node found in scene graph while rendering offscreen image. "
                                "The result will most likely be incorrect.");
    }

  }
  // Regular, non-tiled rendering.
  else {
    // do lazy buffer read (GL context is read in getBuffer())
    this->didreadbuffer = FALSE;
	
	SbViewportRegion region;

	region.setViewportPixels(0,0,fullsize[0],fullsize[1]);

    this->renderaction->setViewportRegion(region);

    SbTime t = SbTime::getTimeOfDay(); // for profiling

    if (base->isOfType(SoNode::getClassTypeId()))
      this->renderaction->apply((SoNode *)base);
    else if (base->isOfType(SoPath::getClassTypeId()))
      this->renderaction->apply((SoPath *)base);
    else  {
      assert(FALSE && "Cannot apply to anything else than an SoNode or an SoPath");
    }

    if (CoinOffscreenGLCanvas::debug()) {
      SoDebugError::postInfo("SoOffscreenRendererP::renderFromBase",
                             "*TIMING* SoGLRenderAction::apply() took %f msecs",
                             (SbTime::getTimeOfDay() - t).getValue() * 1000);
      t = SbTime::getTimeOfDay();
    }

    if (CoinOffscreenGLCanvas::debug()) {
      SoDebugError::postInfo("SoOffscreenRendererP::renderFromBase",
                             "*TIMING* glcanvas.readPixels() took %f msecs",
                             (SbTime::getTimeOfDay() - t).getValue() * 1000);
    }
  }

  this->renderaction->removePreRenderCallback(pre_render_cb, NULL);

  // Restore old value.
  (void)SoGLBigImage::setChangeLimit(bigimagechangelimit);

  this->glcanvas.deactivateGLContext();
  this->renderaction->setCacheContext(oldcontext); // restore old

  if(this->useDC)
	this->updateDCBitmap();

  return TRUE;
}

/*!
  Render the scene graph rooted at \a scene into our internal pixel
  buffer.


  Important note: make sure you pass in a \a scene node pointer which
  has both a camera and at least one light source below it -- otherwise
  you are likely to end up with just a blank or black image buffer.

  This mistake is easily made if you use an SoOffscreenRenderer on a
  scene graph from one of the standard viewer components, as you will
  often just leave the addition of a camera and a headlight
  light source to the viewer to set up. This camera and light source are
  then part of the viewer's private "super-graph" outside of the scope
  of the scene graph passed in by the application programmer. To make
  sure the complete scene graph (including the viewer's "private parts"
  (*snicker*)) are passed to this method, you can get the scene graph
  root from the viewer's internal SoSceneManager instance instead of
  from the viewer's own getSceneGraph() method, like this:

  \code
  SoOffscreenRenderer * myRenderer = new SoOffscreenRenderer(vpregion);
  SoNode * root = myViewer->getSceneManager()->getSceneGraph();
  SbBool ok = myRenderer->render(root);
  // [then use image buffer in a texture, or write it to file, or whatever]
  \endcode

  If you do this and still get a blank buffer, another common problem
  is to have a camera which is not actually pointing at the scene
  geometry you want a snapshot of. If you suspect that could be the
  cause of problems on your end, take a look at SoCamera::pointAt()
  and SoCamera::viewAll() to see how you can make a camera node
  guaranteed to be directed at the scene geometry.

  Yet another common mistake when setting up the camera is to specify
  values for the SoCamera::nearDistance and SoCamera::farDistance
  fields which doesn't not enclose the full scene. This will result in
  either just the background color, or that parts at the front or the
  back of the scene will not be visible in the rendering.

  \sa writeToRGB()
*/
SbBool
SoOffscreenRenderer::render(SoNode * scene)
{
  return PRIVATE(this)->renderFromBase(scene);
}

/*!
  Render the \a scene path into our internal memory buffer.
*/
SbBool
SoOffscreenRenderer::render(SoPath * scene)
{
  return PRIVATE(this)->renderFromBase(scene);
}

// *************************************************************************

/*!
  Returns the offscreen memory buffer.
*/
unsigned char *
SoOffscreenRenderer::getBuffer(void) const
{
  if (!PRIVATE(this)->didreadbuffer) {
    const SbVec2s dims = this->getViewportRegion().getViewportSizePixels();
    //fprintf(stderr,"reading pixels: %d %d\n", dims[0], dims[1]);

    PRIVATE(this)->glcanvas.activateGLContext();
    PRIVATE(this)->glcanvas.readPixels(PRIVATE(this)->buffer, dims, dims[0],
                                       (unsigned int) this->getComponents());
    PRIVATE(this)->glcanvas.deactivateGLContext();
    PRIVATE(this)->didreadbuffer = TRUE;
  }
  return PRIVATE(this)->buffer;
}

/*!
  Win32 only:

  returns a direct handle to the internal DC of the offscreen
  context.

  Useful for efficient access to the raw image under certain special
  circumstances. getBuffer() might be too slow, for instance due to
  pixel format conversion (Windows DCs are usually BGRA, while the
  32-bit buffers returned from getBuffer() are RGBA).

  Notes:

  The return value is a reference to a HDC. The HDC typedef has been
  unwound to a native C++ type for multiplatform compatibility
  reasons.

  Returned reference will contain a NULL value on other platforms.

  Important limitation: if the current dimensions of the
  SoOffscreenRenderer instance are larger than what can be rendered
  with a single offscreen buffer, tiling will be used by the
  SoOffscreenRenderer, and the returned HDC will contain only part of
  the full rendered image.

  \sa getBuffer()
  \since Coin 3.1
*/
const void * const &
SoOffscreenRenderer::getDC(void) const
{
  if(!PRIVATE(this)->useDC)
  {
	PRIVATE(this)->useDC = true;
	PRIVATE(this)->updateDCBitmap();
  }
  
  return PRIVATE(this)->glcanvas.getHDC();
}

void SoOffscreenRendererP::updateDCBitmap()
{
  this->glcanvas.updateDCBitmap();
}
// *************************************************************************

//
// avoid endian problems (little endian sucks, right? :)
//
static size_t
write_short(FILE * fp, unsigned short val)
{
  unsigned char tmp[2];
  tmp[0] = (unsigned char)(val >> 8);
  tmp[1] = (unsigned char)(val & 0xff);
  return fwrite(&tmp, 2, 1, fp);
}

SbBool
SoOffscreenRendererP::writeToRGB(FILE * fp, unsigned int w, unsigned int h,
                                 unsigned int nrcomponents,
                                 const uint8_t * imgbuf)
{
  // FIXME: add code to rle rows, pederb 2000-01-10

  (void)write_short(fp, 0x01da); // imagic
  (void)write_short(fp, 0x0001); // raw (no rle yet)

  if (nrcomponents == 1)
    (void)write_short(fp, 0x0002); // 2 dimensions (heightmap)
  else
    (void)write_short(fp, 0x0003); // 3 dimensions

  (void)write_short(fp, (unsigned short) w);
  (void)write_short(fp, (unsigned short) h);
  (void)write_short(fp, (unsigned short) nrcomponents);

  const size_t BUFSIZE = 500;
  unsigned char buf[BUFSIZE];
  (void)memset(buf, 0, BUFSIZE);
  buf[7] = 255; // set maximum pixel value to 255
  strcpy((char *)buf+8, "https://github.com/coin3d/");
  const size_t wrote = fwrite(buf, 1, BUFSIZE, fp);
  assert(wrote == BUFSIZE);

  unsigned char * tmpbuf = new unsigned char[w];

  SbBool writeok = TRUE;
  for (unsigned int c = 0; c < nrcomponents; c++) {
    for (unsigned int y = 0; y < h; y++) {
      for (unsigned int x = 0; x < w; x++) {
        tmpbuf[x] = imgbuf[(x + y * w) * nrcomponents + c];
      }
      writeok = writeok && (fwrite(tmpbuf, 1, w, fp) == w);
    }
  }

  if (!writeok) {
    SoDebugError::postWarning("SoOffscreenRendererP::writeToRGB",
                              "error when writing RGB file");
  }

  delete [] tmpbuf;
  return writeok;
}


/*!
  Writes the buffer in SGI RGB format by appending it to the already
  open file. Returns \c FALSE if writing fails.

  Important note: do \e not use this method when the Coin library has
  been compiled as an Microsoft Windows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for Microsoft Windows DLLs.
*/
SbBool
SoOffscreenRenderer::writeToRGB(FILE * fp) const
{
  if (SoOffscreenRendererP::offscreenContextsNotSupported()) { return FALSE; }

  SbVec2s size = PRIVATE(this)->viewport.getViewportSizePixels();

  return SoOffscreenRendererP::writeToRGB(fp, size[0], size[1],
                                          this->getComponents(),
                                          this->getBuffer());
}

/*!
  Opens a file with the given name and writes the offscreen buffer in
  SGI RGB format to the new file. If the file already exists, it will
  be overwritten (if permitted by the filesystem).

  Returns \c TRUE if all went ok, otherwise \c FALSE.
*/
SbBool
SoOffscreenRenderer::writeToRGB(const char * filename) const
{
  FILE * rgbfp = fopen(filename, "wb");
  if (!rgbfp) {
    SoDebugError::postWarning("SoOffscreenRenderer::writeToRGB",
                              "couldn't open file '%s'", filename);
    return FALSE;
  }
  SbBool result = this->writeToRGB(rgbfp);
  (void)fclose(rgbfp);
  return result;
}

/*!
  Writes the buffer in PostScript format by appending it to the
  already open file. Returns \c FALSE if writing fails.

  Important note: do \e not use this method when the Coin library has
  been compiled as an Microsoft Windows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for Microsoft Windows DLLs.
*/
SbBool
SoOffscreenRenderer::writeToPostScript(FILE * fp) const
{
  // just choose a page size of 8.5 x 11 inches (A4)
  return this->writeToPostScript(fp, SbVec2f(8.5f, 11.0f));
}

/*!
  Opens a file with the given name and writes the offscreen buffer in
  PostScript format to the new file. If the file already exists, it
  will be overwritten (if permitted by the file system).

  Returns \c TRUE if all went OK, otherwise \c FALSE.
*/
SbBool
SoOffscreenRenderer::writeToPostScript(const char * filename) const
{
  FILE * psfp = fopen(filename, "wb");
  if (!psfp) {
    SoDebugError::postWarning("SoOffscreenRenderer::writeToPostScript",
                              "couldn't open file '%s'", filename);
    return FALSE;
  }
  SbBool result = this->writeToPostScript(psfp);
  (void)fclose(psfp);
  return result;
}

/*!
  Writes the buffer to a file in PostScript format, with \a printsize
  dimensions.

  Important note: do \e not use this method when the Coin library has
  been compiled as an Microsoft Windows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for Microsoft Windows DLLs.
*/
SbBool
SoOffscreenRenderer::writeToPostScript(FILE * fp,
                                       const SbVec2f & printsize) const
{
  if (SoOffscreenRendererP::offscreenContextsNotSupported()) { return FALSE;}

  const SbVec2s size = PRIVATE(this)->viewport.getViewportSizePixels();
  const int nc = this->getComponents();
  const float defaultdpi = 72.0f; // we scale against this value
  const float dpi = this->getScreenPixelsPerInch();
  const SbVec2s pixelsize((short)(printsize[0]*defaultdpi),
                          (short)(printsize[1]*defaultdpi));

  const unsigned char * src = this->getBuffer();
  const int chan = nc <= 2 ? 1 : 3;
  const SbVec2s scaledsize((short) ceil(size[0]*defaultdpi/dpi),
                           (short) ceil(size[1]*defaultdpi/dpi));

  cc_string storedlocale;
  SbBool changed = coin_locale_set_portable(&storedlocale);

  fprintf(fp, "%%!PS-Adobe-2.0 EPSF-1.2\n");
  fprintf(fp, "%%%%BoundingBox: 0 %d %d %d\n",
          pixelsize[1]-scaledsize[1],
          scaledsize[0],
          pixelsize[1]);
  fprintf(fp, "%%%%Creator: Coin <https://github.com/coin3d/>\n");
  fprintf(fp, "%%%%EndComments\n");

  fprintf(fp, "\n");
  fprintf(fp, "/origstate save def\n");
  fprintf(fp, "\n");
  fprintf(fp, "%% workaround for bug in some PS interpreters\n");
  fprintf(fp, "%% which doesn't skip the ASCII85 EOD marker.\n");
  fprintf(fp, "/~ {currentfile read pop pop} def\n\n");
  fprintf(fp, "/image_wd %d def\n", size[0]);
  fprintf(fp, "/image_ht %d def\n", size[1]);
  fprintf(fp, "/pos_wd %d def\n", size[0]);
  fprintf(fp, "/pos_ht %d def\n", size[1]);
  fprintf(fp, "/image_dpi %g def\n", dpi);
  fprintf(fp, "/image_scale %g image_dpi div def\n", defaultdpi);
  fprintf(fp, "/image_chan %d def\n", chan);
  fprintf(fp, "/xpos_offset 0 image_scale mul def\n");
  fprintf(fp, "/ypos_offset 0 image_scale mul def\n");
  fprintf(fp, "/pix_buf_size %d def\n\n", size[0]*chan);
  fprintf(fp, "/page_ht %g %g mul def\n", printsize[1], defaultdpi);
  fprintf(fp, "/page_wd %g %g mul def\n", printsize[0], defaultdpi);
  fprintf(fp, "/image_xpos 0 def\n");
  fprintf(fp, "/image_ypos page_ht pos_ht image_scale mul sub def\n");
  fprintf(fp, "image_xpos xpos_offset add image_ypos ypos_offset add translate\n");
  fprintf(fp, "\n");
  fprintf(fp, "/pix pix_buf_size string def\n");
  fprintf(fp, "image_wd image_scale mul image_ht image_scale mul scale\n");
  fprintf(fp, "\n");
  fprintf(fp, "image_wd image_ht 8\n");
  fprintf(fp, "[image_wd 0 0 image_ht 0 0]\n");
  fprintf(fp, "currentfile\n");
  fprintf(fp, "/ASCII85Decode filter\n");
  // fprintf(fp, "/RunLengthDecode filter\n"); // FIXME: add later. 2003???? pederb.
  if (chan == 3) fprintf(fp, "false 3\ncolorimage\n");
  else fprintf(fp,"image\n");

  const int rowlen = 72;
  int num = size[0] * size[1];
  unsigned char tuple[4];
  unsigned char linebuf[rowlen+5];
  int tuplecnt = 0;
  int linecnt = 0;
  int cnt = 0;
  while (cnt < num) {
    switch (nc) {
    default: // avoid warning
    case 1:
      coin_output_ascii85(fp, src[cnt], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      break;
    case 2:
      coin_output_ascii85(fp, src[cnt*2], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      break;
    case 3:
      coin_output_ascii85(fp, src[cnt*3], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      coin_output_ascii85(fp, src[cnt*3+1], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      coin_output_ascii85(fp, src[cnt*3+2], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      break;
    case 4:
      coin_output_ascii85(fp, src[cnt*4], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      coin_output_ascii85(fp, src[cnt*4+1], tuple, linebuf, &tuplecnt, &linecnt,rowlen, FALSE);
      coin_output_ascii85(fp, src[cnt*4+2], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      break;
    }
    cnt++;
  }

  // flush data in ascii85 encoder
  coin_flush_ascii85(fp, tuple, linebuf, &tuplecnt, &linecnt, rowlen);

  fprintf(fp, "~>\n\n"); // ASCII85 EOD marker
  fprintf(fp, "origstate restore\n");
  fprintf(fp, "\n");
  fprintf(fp, "%%%%Trailer\n");
  fprintf(fp, "\n");
  fprintf(fp, "%%%%EOF\n");

  if (changed) { coin_locale_reset(&storedlocale); }

  return (SbBool) (ferror(fp) == 0);
}

/*!
  Opens a file with the given name and writes the offscreen buffer in
  PostScript format with \a printsize dimensions to the new file. If
  the file already exists, it will be overwritten (if permitted by the
  file system).

  Returns \c TRUE if all went ok, otherwise \c FALSE.
*/
SbBool
SoOffscreenRenderer::writeToPostScript(const char * filename,
                                       const SbVec2f & printsize) const
{
  FILE * psfp = fopen(filename, "wb");
  if (!psfp) {
    SoDebugError::postWarning("SoOffscreenRenderer::writeToPostScript",
                              "couldn't open file '%s'", filename);
    return FALSE;
  }
  SbBool result = this->writeToPostScript(psfp, printsize);
  (void)fclose(psfp);
  return result;
}

// FIXME: the file format support checking could have been done
// better, for instance by using MIME types. Consider fixing the API
// for later major releases. 20020206 mortene.
//
// UPDATE 20050711 mortene: it seems like TGS has extended their API
// in an even worse way; by adding separate writeToJPEG(),
// writeToPNG(), etc. functions.

/*!
  Returns \c TRUE if the buffer can be saved as a file of type \a
  filetypeextension, using SoOffscreenRenderer::writeToFile().  This
  function needs simage v1.1 or newer.

  Examples of possibly supported extensions are: "jpg", "png", "tiff",
  "gif", "bmp", etc. The extension match is not case sensitive.

  Which formats are \e actually supported depends on the capabilities
  of Coin's support library for handling import and export of
  pixel data files: the simage library. If the simage library is not
  installed on your system, no extension output formats will be
  supported.

  Also, note that it is possible to build and install a simage library
  that lacks support for most or all of the file formats it is \e
  capable of supporting. This is so because the simage library depends
  on other, external 3rd party libraries -- in the same manner as Coin
  depends on the simage library for added file format support.

  The two built-in formats that are supported through the
  SoOffscreenRenderer::writeToRGB() and
  SoOffscreenRenderer::writeToPostScript() methods (for SGI RGB format
  and for Adobe PostScript files, respectively) are \e not considered
  by this method, as those two formats are guaranteed to \e always be
  supported through those functions.

  So if you want to be guaranteed to be able to export a screenshot in
  your wanted format, you will have to use either one of the above
  mentioned method for writing SGI RGB or Adobe PostScript directly,
  or make sure the Coin library has been built and is running on top
  of a version of the simage library (that you have preferably built
  yourself) with the file format(s) you want support for.


  This method is an extension versus the original SGI Open Inventor
  API.

  \sa  getNumWriteFiletypes(), getWriteFiletypeInfo(), writeToFile()
*/
SbBool
SoOffscreenRenderer::isWriteSupported(const SbName & filetypeextension) const
{
  if (!simage_wrapper()->versionMatchesAtLeast(1,1,0)) {

    if (CoinOffscreenGLCanvas::debug()) {
      if (!simage_wrapper()->available) {
        SoDebugError::postInfo("SoOffscreenRenderer::isWriteSupported",
                               "simage library not available.");
      } else {
        SoDebugError::postInfo("SoOffscreenRenderer::isWriteSupported",
                               "You need simage v1.1 for this functionality.");
      }
    }
    return FALSE;
  }
  int ret = simage_wrapper()->simage_check_save_supported(filetypeextension.getString());
  return ret ? TRUE : FALSE;
}

/*!
  Returns the number of available exporters. Detailed information
  about the exporters can then be found using getWriteFiletypeInfo().

  See SoOffscreenRenderer::isWriteSupported() for information about
  which file formats you can expect to be present.

  Note that the two built-in export formats, SGI RGB and Adobe
  PostScript, are not counted.

  This method is an extension versus the original SGI Open Inventor
  API.

  \sa getWriteFiletypeInfo()
*/
int
SoOffscreenRenderer::getNumWriteFiletypes(void) const
{
  if (!simage_wrapper()->versionMatchesAtLeast(1,1,0)) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoOffscreenRenderer::getNumWriteFiletypes",
                           "You need simage v1.1 for this functionality.");
#endif // COIN_DEBUG
    return 0;
  }
  return simage_wrapper()->simage_get_num_savers();
}

/*!
  Returns information about an image exporter. \a extlist is a list
  of filename extensions for a file format. E.g. for JPEG it is legal
  to use both jpg and jpeg. \a extlist will contain const char * pointers
  (you need to cast the void * pointers to const char * before using
  them).

  \a fullname is the full name of the image format. \a description is
  an optional string with more information about the file format.

  See SoOffscreenRenderer::isWriteSupported() for information about
  which file formats you can expect to be present.

  This method is an extension versus the original SGI Open Inventor
  API.

  Here is a standalone, complete code example that shows how you can
  check exactly which output formats are supported:

  \code
  #include <Inventor/SoDB.h>
  #include <Inventor/SoOffscreenRenderer.h>

  int
  main(int argc, char **argv)
  {
    SoDB::init();
    SoOffscreenRenderer * r = new SoOffscreenRenderer(*(new SbViewportRegion));
    int num = r->getNumWriteFiletypes();

    if (num == 0) {
      (void)fprintf(stdout,
                    "No image formats supported by the "
                    "SoOffscreenRenderer except SGI RGB and PostScript.\n");
    }
    else {
      for (int i=0; i < num; i++) {
        SbPList extlist;
        SbString fullname, description;
        r->getWriteFiletypeInfo(i, extlist, fullname, description);
        (void)fprintf(stdout, "%s: %s (extension%s: ",
                      fullname.getString(), description.getString(),
                      extlist.getLength() > 1 ? "s" : "");
        for (int j=0; j < extlist.getLength(); j++) {
          (void)fprintf(stdout, "%s%s", j>0 ? ", " : "", (const char*) extlist[j]);
        }
        (void)fprintf(stdout, ")\n");
      }
    }

    delete r;
    return 0;
  }
  \endcode

  \sa getNumWriteFiletypes(), writeToFile()

  \since Coin 2.3
*/
void
SoOffscreenRenderer::getWriteFiletypeInfo(const int idx,
                                          SbPList & extlist,
                                          SbString & fullname,
                                          SbString & description)
{
  if (!simage_wrapper()->versionMatchesAtLeast(1,1,0)) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoOffscreenRenderer::getNumWriteFiletypes",
                           "You need simage v1.1 for this functionality.");
#endif // COIN_DEBUG
    return;
  }
  extlist.truncate(0);
  assert(idx >= 0 && idx < this->getNumWriteFiletypes());
  void * saver = simage_wrapper()->simage_get_saver_handle(idx);
  SbString allext(simage_wrapper()->simage_get_saver_extensions(saver));
  const char * start = allext.getString();
  const char * curr = start;
  const char * end = strchr(curr, ',');
  while (end) {
    const ptrdiff_t offset_start = curr - start;
    const ptrdiff_t offset_end = end - start - 1;
    SbString ext = allext.getSubString((int)offset_start, (int)offset_end);
    SbName extname(ext.getString());
    extlist.append((void*)extname.getString());
    curr = end+1;
    end = strchr(curr, ',');
  }
  const ptrdiff_t offset = curr - start;
  SbString ext = allext.getSubString((int)offset);
  SbName extname(ext.getString());
  extlist.append((void*)extname.getString());
  const char * fullname_s = simage_wrapper()->simage_get_saver_fullname(saver);
  const char * desc_s = simage_wrapper()->simage_get_saver_description(saver);
  fullname = fullname_s ? SbString(fullname_s) : SbString("");
  description = desc_s ? SbString(desc_s) : SbString("");
}

/*!
  Saves the buffer to \a filename, in the file type specified by \a
  filetypeextensions.

  Note that you must still specify the \e full \a filename for the
  first argument, i.e. the second argument will not automatically be
  attached to the filename -- it is only used to decide the file type.

  This method is an extension versus the original SGI Open Inventor
  API.

  \sa isWriteSupported()
*/
SbBool
SoOffscreenRenderer::writeToFile(const SbString & filename, const SbName & filetypeextension) const
{
  if (!simage_wrapper()->versionMatchesAtLeast(1,1,0)) {
    //FIXME: Shouldn't use BOOST_CURRENT_FUNCTION here, the
    //HAVE_CPP_COMPILER_FUNCTION_NAME_VAR should be massaged correctly
    //to fit here. BFG 20090917
    if (!simage_wrapper()->available) {
      SoDebugError::post(__func__,
                             "simage library not available.");
    }
    else {
      int major, minor, micro;
      simage_wrapper()->simage_version(&major,&minor,&micro);
      SoDebugError::post(__func__,
                         "simage version is older than 1.1.0, available version is %d.%d.%d", major,minor,micro);
    }
    return FALSE;
  }
  if (SoOffscreenRendererP::offscreenContextsNotSupported()) {
    SoDebugError::post(__func__,
                       "Offscreen contexts not supported.");
    return FALSE;
  }

  SbVec2s size = PRIVATE(this)->viewport.getViewportSizePixels();
  int comp = (int) this->getComponents();
  unsigned char * bytes = this->getBuffer();
  int ret = simage_wrapper()->simage_save_image(filename.getString(),
                                                bytes,
                                                int(size[0]), int(size[1]), comp,
                                                filetypeextension.getString());
  return ret ? TRUE : FALSE;
}

// *************************************************************************

/*!
  Control whether or not SoOffscreenRenderer can use the "pbuffer"
  feature of OpenGL to render the scenes with hardware acceleration.

  This is a dummy function in Coin, provided for API compatibility
  reasons, as it is really superfluous:

  Coin has internal heuristics to figure out if pbuffers are available
  and can be allocated and used for the SoOffscreenRenderer.  The
  SoOffscreenRenderer will also automatically fall back on "soft"
  buffers if it cannot use pbuffers (or any other hardware
  accelerated rendering technique).

  \since Coin 3.1
*/
void
SoOffscreenRenderer::setPbufferEnable(SbBool COIN_UNUSED_ARG(enable))
{
  // FIXME: change the semantics of this function from just ignoring
  // the input argument, to using it for shutting off pbuffers if
  // FALSE?
  //
  // not sure there's really any good reason to do that, however.
  //
  // mortene.
}

/*!
  See SoOffscreenRenderer::setPbufferEnable().

  \since Coin 3.1
*/
SbBool
SoOffscreenRenderer::getPbufferEnable(void) const
{
  // FIXME: should perhaps return a flag indicating whether or not the
  // system can use pbuffers. this depends on the GL context, however,
  // so the design of this Mercury Inventor API function is inherently
  // flawed.
  //
  // hardly any GL driver these days does *not* provide pbuffers,
  // though, so this is unlikely to be an important issue.
  //
  // mortene.

  return TRUE;
}

// *************************************************************************

// FIXME: this should really be done by SoCamera, on the basis of data
// from an "SoTileRenderingElement". See BUGS.txt, item #121. 20050712 mortene.
void
SoOffscreenRendererP::setCameraViewvolForTile(SoCamera * cam)
{
  SoState * state = (PUBLIC(this)->getGLRenderAction())->getState();

  // A small trick to change the aspect ratio without changing the
  // scene graph camera.
  SbViewVolume vv;
  const float aspectratio = this->viewport.getViewportAspectRatio();
  const SbVec2s vporigin = this->viewport.getViewportOriginPixels();

  switch(cam->viewportMapping.getValue()) {
  case SoCamera::CROP_VIEWPORT_FILL_FRAME:
  case SoCamera::CROP_VIEWPORT_LINE_FRAME:
  case SoCamera::CROP_VIEWPORT_NO_FRAME:
    vv = cam->getViewVolume(0.0f);

    { // FIXME: should really fix this bug, not just warn that it is
      // there. See item #191 in Coin/BUGS.txt for more information.
      // 20050714 mortene.
      static SbBool first = TRUE;
      if (first) {
        SbString s;
        cam->viewportMapping.get(s);
        SoDebugError::postWarning("SoOffscreenRendererP::setCameraViewvolForTile",
                                  "The SoOffscreenRenderer does not yet work "
                                  "properly with the SoCamera::viewportMapping "
                                  "field set to '%s'", s.getString());
        first = FALSE;
      }
    }
    break;
  case SoCamera::ADJUST_CAMERA:
    vv = cam->getViewVolume(aspectratio);
    if (aspectratio < 1.0f) vv.scale(1.0f / aspectratio);
    break;
  case SoCamera::LEAVE_ALONE:
    vv = cam->getViewVolume(0.0f);
    break;
  default:
    assert(0 && "unknown viewport mapping");
    break;
  }

  const int LEFTINTPOS = (this->currenttile[0] * this->glcanvassize[0]) - vporigin[0];
  const int RIGHTINTPOS = LEFTINTPOS + this->subsize[0];
  const int TOPINTPOS = (this->currenttile[1] * this->glcanvassize[1]) - vporigin[1];
  const int BOTTOMINTPOS = TOPINTPOS + this->subsize[1];

  const SbVec2s fullsize = this->viewport.getViewportSizePixels();
  const float left = float(LEFTINTPOS) / float(fullsize[0]);
  const float right = float(RIGHTINTPOS) / float(fullsize[0]);
  // Swap top / bottom, to flip the coordinate system for the Y-axis
  // the way we want it.
  const float top = float(BOTTOMINTPOS) / float(fullsize[1]);
  const float bottom = float(TOPINTPOS) / float(fullsize[1]);

  if (CoinOffscreenGLCanvas::debug()) {
    SoDebugError::postInfo("SoOffscreenRendererP::setCameraViewvolForTile",
                           "narrowing for tile <%d, %d>: <%f, %f> - <%f, %f>",
                           this->currenttile[0], this->currenttile[1],
                           left, bottom, right, top);
  }

  // Reshape view volume
  vv = vv.narrow(left, bottom, right, top);

  SbMatrix proj, affine;
  vv.getMatrices(affine, proj);

  // Support antialiasing if renderpasses > 1
  if (renderaction->getNumPasses() > 1) {
    SbVec3f jittervec;
    SbMatrix m;
    coin_viewvolume_jitter(renderaction->getNumPasses(), renderaction->getCurPass(),
                           this->glcanvassize, (float *)jittervec.getValue());
    m.setTranslate(jittervec);
    proj.multRight(m);
  }

  SoCullElement::setViewVolume(state, vv);
  SoViewVolumeElement::set(state, cam, vv);
  SoProjectionMatrixElement::set(state, cam, proj);
  SoViewingMatrixElement::set(state, cam, affine);
}

// *************************************************************************

SbBool
SoOffscreenRendererP::offscreenContextsNotSupported(void)
{
  // Returning FALSE means that offscreen rendering seems to be
  // generally supported on the system.
  //
  // (It is however important to be robust and handle cases where it
  // still fails, as this can happen due to e.g. lack of resources or
  // other causes that may change during runtime.)

#ifdef HAVE_GLX
  return FALSE;
#elif defined(HAVE_EGL)
  return FALSE;
#elif defined(HAVE_WGL)
  return FALSE;
#elif defined(COIN_MACOS_10)
  return FALSE;
#endif

  // No win-system GL binding was found, so we're sure that offscreen
  // rendering can *not* be done.
  return TRUE;
}

// *************************************************************************

#undef PRIVATE
#undef PUBLIC
