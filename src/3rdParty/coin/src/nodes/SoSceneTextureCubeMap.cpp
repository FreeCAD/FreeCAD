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

#include <Inventor/nodes/SoSceneTextureCubeMap.h>
#include "coindefs.h"
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/misc/SoGLCubeMapImage.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/SbImage.h>
#include <Inventor/C/glue/gl.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

// *************************************************************************

/*!
  \class SoSceneTextureCubeMap SoSceneTextureCubeMap.h Inventor/nodes/SoSceneTextureCubeMap.h
  \brief Renders a scene into a texture cube map.

  \ingroup coin_nodes
  \since Coin 2.5
*/

// FIXME: more detailed description on how the camera is to be set 20050429 martin
// if scene does not contain a camera, a default camera is inserted into the
// cachedScene (the original scene stays untouched!)
// Default camera = SoPerspectiveCamera {
//   position SbVec3f(0 0 0)
//   rotation SbRotation(SbVec3f(0,1,0), 0.0f)
//   heightAngle (M_PI / 2.0f)
//   nearDistance = 0.1;
//   farDistance = 100;
// }

class SoSceneTextureCubeMapP {
 public:
  SoSceneTextureCubeMapP(SoSceneTextureCubeMap * theAPI);
  ~SoSceneTextureCubeMapP();

  SoSceneTextureCubeMap * api;
  void * glcontext;
  SbVec2s glcontextsize;
  int contextid;

  SoGLRenderAction * glaction;
  SoGLCubeMapImage * glimage;
  SbBool pbuffervalid;
  SbBool glimagevalid;
  SbBool glrectangle;

  SoNode   * cachedScene;  // scene with guaranteed camera
  SoCamera * cachedCamera; // reference to the camera

  void updatePBuffer(SoState * state, const float quality);
  static void prerendercb(void * userdata, SoGLRenderAction * action);

  SoCamera * findCamera(void);
  void destroyCamera(void);
  SoCamera * ensureCamera(void);
  SoNode   * updateCamera(const SoGLCubeMapImage::Target target);
    
#ifdef COIN_THREADSAFE
  SbMutex mutex;
#endif // COIN_THREADSAFE

  SbBool canrendertotexture;
  unsigned char * offscreenbuffer;
  int offscreenbuffersize;
  SbBool hadSceneCamera;
  SbBool hasSceneChanged;

  // FIXME: this will not work on all platforms/compilers
  static SbRotation ROT_NEG_X;
  static SbRotation ROT_POS_X;
  static SbRotation ROT_NEG_Y;
  static SbRotation ROT_POS_Y;
  static SbRotation ROT_NEG_Z;
  static SbRotation ROT_POS_Z;
};


SbRotation SoSceneTextureCubeMapP::ROT_NEG_X = 
  SbRotation(SbVec3f(0,1,0), (float) (M_PI/2.0f)) *
  SbRotation(SbVec3f(1,0,0), (float) M_PI);
SbRotation SoSceneTextureCubeMapP::ROT_POS_X = 
  SbRotation(SbVec3f(0,1,0), (float) (M_PI/-2.0f)) *
  SbRotation(SbVec3f(1,0,0), (float) M_PI);
SbRotation SoSceneTextureCubeMapP::ROT_NEG_Y = 
  SbRotation(SbVec3f(1,0,0), (float) (M_PI / -2.0f));
SbRotation SoSceneTextureCubeMapP::ROT_POS_Y = 
  SbRotation(SbVec3f(1,0,0), (float) (M_PI / 2.0f));
SbRotation SoSceneTextureCubeMapP::ROT_NEG_Z =  
  SbRotation(SbVec3f(0,1,0), 0.0f) *
  SbRotation(SbVec3f(0,0,1), (float) M_PI);
SbRotation SoSceneTextureCubeMapP::ROT_POS_Z = 
  SbRotation(SbVec3f(0,1,0), (float) M_PI) *
  SbRotation(SbVec3f(0,0,1), (float) M_PI);

#define PRIVATE(p) (p->pimpl)

#ifdef COIN_THREADSAFE
#define LOCK_GLIMAGE(_thisp_) (PRIVATE(_thisp_)->mutex.lock())
#define UNLOCK_GLIMAGE(_thisp_) (PRIVATE(_thisp_)->mutex.unlock())
#else // COIN_THREADSAFE
#define LOCK_GLIMAGE(_thisp_)
#define UNLOCK_GLIMAGE(_thisp_)
#endif // COIN_THREADSAFE


SO_NODE_SOURCE(SoSceneTextureCubeMap);

static SoGLCubeMapImage::Wrap
translateWrap(const SoSceneTextureCubeMap::Wrap wrap)
{
  if (wrap == SoSceneTextureCubeMap::REPEAT) return SoGLImage::REPEAT;
  return SoGLImage::CLAMP;
}

/*!
  Constructor.
*/
SoSceneTextureCubeMap::SoSceneTextureCubeMap(void)
{
  PRIVATE(this) = new SoSceneTextureCubeMapP(this);

  SO_NODE_CONSTRUCTOR(SoSceneTextureCubeMap);

  SO_NODE_ADD_FIELD(size, (256, 256));
  SO_NODE_ADD_FIELD(scene, (NULL));
  SO_NODE_ADD_FIELD(backgroundColor, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(transparencyFunction, (NONE));

  SO_NODE_ADD_FIELD(wrapS, (REPEAT));
  SO_NODE_ADD_FIELD(wrapT, (REPEAT));
  SO_NODE_ADD_FIELD(wrapR, (REPEAT));
  SO_NODE_ADD_FIELD(model, (MODULATE));
  SO_NODE_ADD_FIELD(blendColor, (0.0f, 0.0f, 0.0f));

  SO_NODE_DEFINE_ENUM_VALUE(Wrap, REPEAT);
  SO_NODE_DEFINE_ENUM_VALUE(Wrap, CLAMP);

  SO_NODE_SET_SF_ENUM_TYPE(wrapS, Wrap);
  SO_NODE_SET_SF_ENUM_TYPE(wrapT, Wrap);
  SO_NODE_SET_SF_ENUM_TYPE(wrapR, Wrap);

  SO_NODE_DEFINE_ENUM_VALUE(Model, MODULATE);
  SO_NODE_DEFINE_ENUM_VALUE(Model, DECAL);
  SO_NODE_DEFINE_ENUM_VALUE(Model, BLEND);
  SO_NODE_DEFINE_ENUM_VALUE(Model, REPLACE);
  SO_NODE_SET_SF_ENUM_TYPE(model, Model);

  SO_NODE_DEFINE_ENUM_VALUE(TransparencyFunction, NONE);
  SO_NODE_DEFINE_ENUM_VALUE(TransparencyFunction, ALPHA_BLEND);
  SO_NODE_DEFINE_ENUM_VALUE(TransparencyFunction, ALPHA_TEST);
  SO_NODE_SET_SF_ENUM_TYPE(transparencyFunction, TransparencyFunction);
}

/*!
  Destructor. Frees up internal resources used to store texture image
  data.
*/
SoSceneTextureCubeMap::~SoSceneTextureCubeMap()
{
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoSceneTextureCubeMap::initClass(void)
{
  SO_NODE_INIT_CLASS(SoSceneTextureCubeMap, SoNode, "Node");

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureImageElement);
  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureEnabledElement);

  SO_ENABLE(SoCallbackAction, SoMultiTextureImageElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureEnabledElement);

  SO_ENABLE(SoRayPickAction, SoMultiTextureImageElement);
  SO_ENABLE(SoRayPickAction, SoMultiTextureEnabledElement);
}

// Documented in superclass.
void
SoSceneTextureCubeMap::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  if (SoTextureOverrideElement::getImageOverride(state))
    return;

  float quality = SoTextureQualityElement::get(state);

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));
  SoNode * root = this->scene.getValue();

  LOCK_GLIMAGE(this);

  if (root && (!PRIVATE(this)->glimagevalid || !PRIVATE(this)->pbuffervalid)) {
    PRIVATE(this)->updatePBuffer(state, quality);
    
    // don't cache when we change the glimage
    SoCacheElement::setInvalid(TRUE);
    if (state->isCacheOpen()) {
      SoCacheElement::invalidate(state);
    }
  }  
  UNLOCK_GLIMAGE(this);
  
  SoMultiTextureImageElement::Model glmodel = (SoMultiTextureImageElement::Model) 
    this->model.getValue();
  
  if (glmodel == SoMultiTextureImageElement::REPLACE) {
    if (!cc_glglue_glversion_matches_at_least(glue, 1, 1, 0)) {
      static int didwarn = 0;
      if (!didwarn) {
        SoDebugError::postWarning("SoSceneTextureCubeMap::GLRender",
                                  "Unable to use the GL_REPLACE texture model. "
                                  "Your OpenGL version is < 1.1. "
                                  "Using GL_MODULATE instead.");
        didwarn = 1;
      }
      // use MODULATE and not DECAL, since DECAL only works for RGB
      // and RGBA textures
      glmodel = SoMultiTextureImageElement::MODULATE;
    }
  }
  
  int unit = SoTextureUnitElement::get(state);
  int maxunits = cc_glglue_max_texture_units(glue);
  if (unit < maxunits) {
    SoGLMultiTextureImageElement::set(state, this, unit,
                                      PRIVATE(this)->glimage,
                                      glmodel,
                                      this->blendColor.getValue());
    if (quality > 0.0f && PRIVATE(this)->glimagevalid) {
      SoGLMultiTextureEnabledElement::enableCubeMap(state, this, unit);
    }
  }
  else {
    // we already warned in SoTextureUnit. I think it's best to just
    // ignore the texture here so that all texture for non-supported
    // units will be ignored. pederb, 2003-11-04
  }
}

// Documented in superclass.
void
SoSceneTextureCubeMap::doAction(SoAction * COIN_UNUSED_ARG(action))
{
  // not implemented yet
}

// doc from parent
void
SoSceneTextureCubeMap::callback(SoCallbackAction * action)
{
  SoSceneTextureCubeMap::doAction(action);
}

// doc from parent
void
SoSceneTextureCubeMap::rayPick(SoRayPickAction * action)
{
  SoSceneTextureCubeMap::doAction(action);
}

// Documented in superclass. Overridden to detect when fields change.
void
SoSceneTextureCubeMap::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f == &this->scene) {
    PRIVATE(this)->hasSceneChanged = TRUE; // refetch camera and scene
    PRIVATE(this)->pbuffervalid = FALSE; // rerender scene
  }
  else if (f == &this->size) {
    PRIVATE(this)->pbuffervalid = FALSE; // rerender scene
  }
  else if (f == &this->wrapS || f == &this->wrapT || f == &this->wrapR ||
           f == &this->model || f == &this->transparencyFunction) {
    // no need to render scene again, but update the texture object
    PRIVATE(this)->glimagevalid = FALSE;
  }
  inherited::notify(list);
}

/* *********************************************************************** */
/* ***                       private implementation                    *** */
/* *********************************************************************** */

#define PUBLIC(p) (p->api)

SoSceneTextureCubeMapP::SoSceneTextureCubeMapP(SoSceneTextureCubeMap * apiptr)
{
  this->api = apiptr;
  this->glimage = NULL;
  this->glimagevalid = FALSE;
  this->glcontext = NULL;
  this->pbuffervalid = FALSE;
  this->glaction = NULL;
  this->glcontextsize.setValue(-1,-1);
  this->glrectangle = FALSE;
  this->offscreenbuffer = NULL;
  this->offscreenbuffersize = 0;
  this->canrendertotexture = FALSE;
  this->contextid = -1;
  this->cachedScene = NULL;
  this->cachedCamera = NULL;
  this->hadSceneCamera = FALSE;
  this->hasSceneChanged = TRUE;
}

SoSceneTextureCubeMapP::~SoSceneTextureCubeMapP()
{
  if (this->glimage) this->glimage->unref(NULL);
  this->destroyCamera();
  if (this->glcontext != NULL) {
    cc_glglue_context_destruct(this->glcontext);
  }
  delete[] this->offscreenbuffer;
  delete this->glaction;
}

void
SoSceneTextureCubeMapP::updatePBuffer(SoState * state, const float quality)
{
  SbVec2s size = PUBLIC(this)->size.getValue();

  assert(PUBLIC(this)->scene.getValue());

  if ((this->glcontext && this->glcontextsize != size) ||
      (size == SbVec2s(0,0))) {
    if (this->glimage) {
      this->glimage->unref(state);
      this->glimage = NULL;
    }
    if (this->glcontext) {
      cc_glglue_context_destruct(this->glcontext);
      this->glcontextsize.setValue(-1,-1);
      this->glcontext = NULL;
    }
    delete this->glaction; 
    this->glaction = NULL;
    this->glimagevalid = FALSE;
  }
  if (size == SbVec2s(0,0)) return;

  // FIXME: temporary until non power of two textures are supported,
  // pederb 2003-12-05
  size[0] = (short) coin_geq_power_of_two(size[0]);
  size[1] = (short) coin_geq_power_of_two(size[1]);

  if (this->glcontext == NULL) {
    this->glcontextsize = size;
    // disabled until an pbuffer extension is available to create a
    // render-to-texture pbuffer that has a non power of two size.
    // pederb, 2003-12-05
    if (1) { // if (!glue->has_ext_texture_rectangle) {
      this->glcontextsize[0] = (short) coin_geq_power_of_two(size[0]);
      this->glcontextsize[1] = (short) coin_geq_power_of_two(size[1]);

      if (this->glcontextsize != size) {
        static int didwarn = 0;
        if (!didwarn) {
          SoDebugError::postWarning("SoSceneTextureCubeMapP::updatePBuffer",
                                    "Requested non power of two size, "
                                    "but your OpenGL driver lacks support "
                                    "for such pbuffer textures.");
          didwarn = 1;
        }
      }
    }
    
    this->glrectangle = FALSE;
    if (!coin_is_power_of_two(this->glcontextsize[0]) ||
        !coin_is_power_of_two(this->glcontextsize[1])) {
      // we only get here if the OpenGL driver can handle non power of
      // two textures/pbuffers.
      this->glrectangle = TRUE;
    }

    // FIXME: make it possible to specify what kind of context you want
    // (RGB or RGBA, I guess). pederb, 2003-11-27
    unsigned int x = this->glcontextsize[0];
    unsigned int y = this->glcontextsize[1];
    
    this->glcontext = cc_glglue_context_create_offscreen(x, y);
    this->canrendertotexture = 
      cc_glglue_context_can_render_to_texture(this->glcontext);

    if (!this->glaction) {
      this->contextid = (int)SoGLCacheContextElement::getUniqueCacheContext();
      this->glaction = 
        new SoGLRenderAction(SbViewportRegion(this->glcontextsize));
      this->glaction->
        addPreRenderCallback(SoSceneTextureCubeMapP::prerendercb, 
                             (void*) PUBLIC(this));
    } 
    else {
      this->glaction->
        setViewportRegion(SbViewportRegion(this->glcontextsize));
    }
    this->glaction->setCacheContext(this->contextid);    
    this->glimagevalid = FALSE;
  }

  if (!this->pbuffervalid) {
    assert(this->glaction != NULL);
    assert(this->glcontext != NULL);
    this->glaction->setTransparencyType((SoGLRenderAction::TransparencyType)
                                        SoShapeStyleElement::getTransparencyType(state));

    cc_glglue_context_make_current(this->glcontext);


    glEnable(GL_DEPTH_TEST);

    if (!this->canrendertotexture) {
      SbVec2s size = this->glcontextsize;
      int cubeSideSize = size[0]*size[1]*4;
      int reqbytes = cubeSideSize*6; // 6 cube sides
      if (reqbytes > this->offscreenbuffersize) {
        delete[] this->offscreenbuffer;
        this->offscreenbuffer = new unsigned char[reqbytes];
        this->offscreenbuffersize = reqbytes;
      }

      unsigned char * cubeSidePtr = this->offscreenbuffer;
                  
      for (int i=0; i<6; i++) {
        this->glaction->apply(this->updateCamera((SoGLCubeMapImage::Target)i));
        glFlush();

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0,0,size[0],size[1],GL_RGBA,GL_UNSIGNED_BYTE,cubeSidePtr);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        cubeSidePtr += cubeSideSize;
      }
    }

    cc_glglue_context_reinstate_previous(this->glcontext);
  }

  if (!this->glimagevalid || (this->glimage == NULL)) {
    // just delete old glimage
    if (this->glimage) {
      this->glimage->unref(state);
      this->glimage = NULL;
    }
    this->glimage = new SoGLCubeMapImage;
    uint32_t flags = this->glimage->getFlags();
    if (this->glrectangle) {
      flags |= SoGLImage::RECTANGLE;
    }
    switch ((SoSceneTextureCubeMap::TransparencyFunction) (PUBLIC(this)->transparencyFunction.getValue())) {
    case SoSceneTextureCubeMap::NONE:
      flags |= SoGLImage::FORCE_TRANSPARENCY_FALSE|SoGLImage::FORCE_ALPHA_TEST_FALSE;
      break;
    case SoSceneTextureCubeMap::ALPHA_TEST:
      flags |= SoGLImage::FORCE_TRANSPARENCY_TRUE|SoGLImage::FORCE_ALPHA_TEST_TRUE;
      break;
    case SoSceneTextureCubeMap::ALPHA_BLEND:
      flags |= SoGLImage::FORCE_TRANSPARENCY_TRUE|SoGLImage::FORCE_ALPHA_TEST_FALSE;
      break;
    default:
      assert(0 && "should not get here");
      break;
    }
    this->glimage->setFlags(flags);

    if (this->canrendertotexture) {
      // FIXME: not implemented yet - 20050427 martin

      // bind texture to pbuffer
      this->glimage->setPBuffer(state, this->glcontext,
                                translateWrap((SoSceneTextureCubeMap::Wrap)PUBLIC(this)->wrapS.getValue()),
                                translateWrap((SoSceneTextureCubeMap::Wrap)PUBLIC(this)->wrapT.getValue()),
                                quality);
    }
  }

  if (!this->canrendertotexture && !this->pbuffervalid) {
    assert(this->glimage);
    assert(this->offscreenbuffer);
    int cubeSideSize = this->glcontextsize[0] * this->glcontextsize[1] * 4;
    unsigned char * cubeSidePtr = this->offscreenbuffer;
        
    // FIXME: what about  wrapS, wrapT, wrapR, and quality? - martin 20050427
    for (int i=0; i<6; i++) {
      this->glimage->setCubeMapImage((SoGLCubeMapImage::Target)i, 
                                     cubeSidePtr, 
                                     this->glcontextsize, 4);
      cubeSidePtr += cubeSideSize;
    }
  }
  this->glimagevalid = TRUE;
  this->pbuffervalid = TRUE;
}

void
SoSceneTextureCubeMapP::destroyCamera(void)
{
  if (this->cachedCamera) {
    this->cachedCamera->unref();
    this->cachedCamera = NULL;
  }
  if (this->cachedScene) {
    this->cachedScene->unref();
    this->cachedScene = NULL;
  }
}

SoCamera *
SoSceneTextureCubeMapP::findCamera(void)
{
  SoSearchAction sa;

  sa.setType(SoCamera::getClassTypeId());
  sa.setInterest(SoSearchAction::FIRST);
  sa.apply(PUBLIC(this)->scene.getValue());

  SoPath * path = sa.getPath();

  if (path == NULL)
    return NULL;
  else
    return (SoCamera *)path->getTail();
}


SoCamera * 
SoSceneTextureCubeMapP::ensureCamera(void)
{
  if (this->hasSceneChanged == FALSE) return this->cachedCamera;

  this->hasSceneChanged = FALSE;
  SoCamera * camera    = this->findCamera();
  SbBool     hasCamera = (camera != NULL); // does the scene provide a camera?

  if (hasCamera) {
    if (this->cachedCamera != camera) {
      if (this->cachedCamera) this->cachedCamera->unref();
      this->cachedCamera = camera;
      this->cachedCamera->ref();
    }
  }
  else if (this->hadSceneCamera || this->cachedCamera == NULL) {
    // create default camera:
    static int didwarn = 0;
    if (!didwarn) {
      SoDebugError::postWarning("SoSceneTextureCubeMap::ensureCamera",
                                "The scene does not provide a camera. "
                                "A perspective camera at position (0,0,0) "
                                "will be used.");
      didwarn = 1;
    }
    if (this->cachedCamera) this->cachedCamera->unref();
    this->cachedCamera = new SoPerspectiveCamera;
    this->cachedCamera->position = SbVec3f(0, 0, 0);
    this->cachedCamera->nearDistance = 0.1f;
    this->cachedCamera->farDistance = 100;
    ((SoPerspectiveCamera*)this->cachedCamera)->heightAngle =
      (float) (M_PI / 2.0f);
    this->cachedCamera->ref();
  }
  assert(this->cachedCamera);

  SoNode * scene = PUBLIC(this)->scene.getValue();
  if (hasCamera) {
    if (scene != this->cachedScene) {
      if (this->cachedScene) this->cachedScene->unref();
      this->cachedScene = scene;
      this->cachedScene->ref();
    }
  }
  else if (this->cachedScene == NULL || this->hadSceneCamera) {
    if (this->cachedScene) this->cachedScene->unref();
    SoSeparator * root = new SoSeparator();
    root->addChild(this->cachedCamera);
    root->addChild(scene);
    this->cachedScene = (SoNode *)root;
    this->cachedScene->ref();
  }
  else {
    assert(this->cachedScene->isOfType(SoSeparator::getClassTypeId()));
    SoSeparator * root = (SoSeparator*)this->cachedScene;
    assert(root->getNumChildren() == 2);
    if (root->getChild(1) != scene)
      ((SoSeparator *)this->cachedScene)->replaceChild(1, scene);
  }

  this->hadSceneCamera = hasCamera;
  return this->cachedCamera;
}

SoNode *
SoSceneTextureCubeMapP::updateCamera(const SoGLCubeMapImage::Target target)
{
  SoCamera * camera = this->ensureCamera();

  switch (target) {
  default:
  case SoGLCubeMapImage::NEGATIVE_X:
    camera->orientation.setValue(ROT_NEG_X);
    break;
  case SoGLCubeMapImage::POSITIVE_X:
    camera->orientation.setValue(ROT_POS_X);
    break;

  case SoGLCubeMapImage::NEGATIVE_Y:
    camera->orientation.setValue(ROT_NEG_Y);
    break;
  case SoGLCubeMapImage::POSITIVE_Y:
    camera->orientation.setValue(ROT_POS_Y);
    break;

  case SoGLCubeMapImage::NEGATIVE_Z:
    camera->orientation.setValue(ROT_NEG_Z);
    break;
  case SoGLCubeMapImage::POSITIVE_Z:
    camera->orientation.setValue(ROT_POS_Z);
    break;
  }

  return this->cachedScene;
}

void
SoSceneTextureCubeMapP::prerendercb(void * userdata, SoGLRenderAction * COIN_UNUSED_ARG(action))
{
  SoSceneTextureCubeMap * thisp = (SoSceneTextureCubeMap*) userdata;
  SbColor col = thisp->backgroundColor.getValue();
  glClearColor(col[0], col[1], col[2], 1.0f);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
}

#undef LOCK_GLIMAGE
#undef UNLOCK_GLIMAGE
#undef PRIVATE
#undef PUBLIC
