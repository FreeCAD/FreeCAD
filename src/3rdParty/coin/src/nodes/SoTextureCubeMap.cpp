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
  \class SoTextureCubeMap SoTextureCubeMap.h Inventor/nodes/SoTextureCubeMap.h
  \brief The SoTextureCubeMap class is used to map a cube map onto subsequent shape nodes.

  \ingroup coin_nodes

  Shape nodes within the scope of SoTextureCubeMap nodes in the scene graph
  (i.e. below the same SoSeparator and to the right hand side of the
  SoTextureCubeMap) will have the texture applied according to each shape
  type's individual characteristics.  See the documentation of the
  various shape types (SoFaceSet, SoCube, SoSphere, etc.) for
  information about the specifics of how the textures will be applied.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCubeMap {
        filenames [  ]
        imagePosX 0 0 0
        imageNegX 0 0 0
        imagePosY 0 0 0
        imageNegY 0 0 0
        imagePosZ 0 0 0
        imageNegZ 0 0 0
        wrapS REPEAT
        wrapT REPEAT
        model MODULATE
        blendColor 0 0 0
    }
  \endcode

  \since Coin 3.0
*/

// *************************************************************************

#include <Inventor/nodes/SoTextureCubeMap.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/SoInput.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/misc/SoGLCubeMapImage.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbImage.h>
#include <Inventor/C/glue/gl.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#include "coindefs.h" // COIN_OBSOLETED()
#include "nodes/SoSubNodeP.h"
#include "elements/SoTextureScalePolicyElement.h"

/*!
  \enum SoTextureCubeMap::Model

  Texture mapping model, for deciding how to "merge" the texture map
  with the object it is mapped onto.
*/
/*!
  \var SoTextureCubeMap::Model SoTextureCubeMap::MODULATE

  Texture color is multiplied by the polygon color. The result will
  be Phong shaded (if light model is PHONG).
*/
/*!
  \var SoTextureCubeMap::Model SoTextureCubeMap::DECAL

  Texture image overwrites polygon shading. Textured pixels will
  not be Phong shaded. Has undefined behaviour for grayscale and
  grayscale-alpha textures.
*/
/*!
  \var SoTextureCubeMap::Model SoTextureCubeMap::BLEND

  This model is normally used with monochrome textures (i.e. textures
  with one or two components). The first component, the intensity, is
  then used to blend between the shaded color of the polygon and the
  SoTextureCubeMap::blendColor.
*/
/*!
  \var SoTextureCubeMap::Model SoTextureCubeMap::REPLACE

  Texture image overwrites polygon shading. Textured pixels will not
  be Phong shaded. Supports grayscale and grayscale alpha
  textures. This feature requires OpenGL 1.1. MODULATE will be used if
  OpenGL version < 1.1 is detected.

  Please note that using this texture model will make your Inventor
  files incompatible with older versions of Coin and Inventor. You
  need Coin >= 2.2 or TGS Inventor 4.0 to load Inventor files that
  uses the REPLACE texture model.

*/ 

/*!
  \enum SoTextureCubeMap::Wrap

  Enumeration of wrapping strategies which can be used when the
  texture map doesn't cover the full extent of the geometry.
*/
/*!
  \var SoTextureCubeMap::Wrap SoTextureCubeMap::REPEAT
  Repeat texture when coordinate is not between 0 and 1.
*/
/*!
  \var SoTextureCubeMap::Wrap SoTextureCubeMap::CLAMP
  Clamp coordinate between 0 and 1.
*/


/*!
  \var SoSFString SoTextureCubeMap::filenames

  The filenames in this order: negx, posx, negy, posy, negz, posz.
*/

/*!
  \var SoSFImage SoTextureCubeMap::imagePosX

  The pos-x image.
*/

/*!
  \var SoSFImage SoTextureCubeMap::imageNegX

  The neg-x image.
*/


/*!
  \var SoSFImage SoTextureCubeMap::imagePosY

  The neg-y image.
*/


/*!
  \var SoSFImage SoTextureCubeMap::imageNegY

  The neg-y image.
*/

/*!
  \var SoSFImage SoTextureCubeMap::imagePosZ
  
  The pos-z image.
*/

/*!
  \var SoSFImage SoTextureCubeMap::imageNegZ

  The neg-z image.
*/

/*!
  \var SoSFEnum SoTextureCubeMap::wrapS

  Wrapping strategy for the S coordinate when the texture map is
  narrower than the object to map onto.

  Default value is SoTextureCubeMap::REPEAT.
*/
/*!
  \var SoSFEnum SoTextureCubeMap::wrapT

  Wrapping strategy for the T coordinate when the texture map is
  shorter than the object to map onto.

  Default value is SoTextureCubeMap::REPEAT.
*/
/*!
  \var SoSFEnum SoTextureCubeMap::model

  Texture mapping model for how the texture map is "merged" with the
  polygon primitives it is applied to. Default value is
  SoTextureCubeMap::MODULATE.
*/
/*!
  \var SoSFColor SoTextureCubeMap::blendColor

  Blend color. Used when SoTextureCubeMap::model is SoTextureCubeMap::BLEND.

  Default color value is [0, 0, 0], black, which means no contribution
  to the blending is made.
*/

// *************************************************************************

class SoTextureCubeMapP {
public:
#ifdef COIN_THREADSAFE
  SbMutex mutex;
#endif // COIN_THREADSAFE
  SoGLCubeMapImage * glimage;
  SoFieldSensor * filenames_sensor;
  int readstatus;
  SbBool glimagevalid;
};


#define PRIVATE(p) (p->pimpl)

#ifdef COIN_THREADSAFE
#define LOCK_GLIMAGE(_thisp_) (PRIVATE(_thisp_)->mutex.lock())
#define UNLOCK_GLIMAGE(_thisp_) (PRIVATE(_thisp_)->mutex.unlock())
#else // COIN_THREADSAFE
#define LOCK_GLIMAGE(_thisp_)
#define UNLOCK_GLIMAGE(_thisp_)
#endif // COIN_THREADSAFE


SO_NODE_SOURCE(SoTextureCubeMap);

/*!
  Constructor.
*/
SoTextureCubeMap::SoTextureCubeMap(void)
{
  PRIVATE(this) = new SoTextureCubeMapP;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCubeMap);

  SO_NODE_ADD_FIELD(filenames, (""));
  SO_NODE_ADD_FIELD(imagePosX, (SbVec2s(0, 0), 0, NULL));
  SO_NODE_ADD_FIELD(imageNegX, (SbVec2s(0, 0), 0, NULL));
  SO_NODE_ADD_FIELD(imagePosY, (SbVec2s(0, 0), 0, NULL));
  SO_NODE_ADD_FIELD(imageNegY, (SbVec2s(0, 0), 0, NULL));
  SO_NODE_ADD_FIELD(imagePosZ, (SbVec2s(0, 0), 0, NULL));
  SO_NODE_ADD_FIELD(imageNegZ, (SbVec2s(0, 0), 0, NULL));
  SO_NODE_ADD_FIELD(wrapS, (REPEAT));
  SO_NODE_ADD_FIELD(wrapT, (REPEAT));
  SO_NODE_ADD_FIELD(model, (MODULATE));
  SO_NODE_ADD_FIELD(blendColor, (0.0f, 0.0f, 0.0f));

  SO_NODE_DEFINE_ENUM_VALUE(Wrap, REPEAT);
  SO_NODE_DEFINE_ENUM_VALUE(Wrap, CLAMP);

  SO_NODE_SET_SF_ENUM_TYPE(wrapS, Wrap);
  SO_NODE_SET_SF_ENUM_TYPE(wrapT, Wrap);

  SO_NODE_DEFINE_ENUM_VALUE(Model, MODULATE);
  SO_NODE_DEFINE_ENUM_VALUE(Model, DECAL);
  SO_NODE_DEFINE_ENUM_VALUE(Model, BLEND);
  SO_NODE_DEFINE_ENUM_VALUE(Model, REPLACE);
  SO_NODE_SET_SF_ENUM_TYPE(model, Model);

  this->filenames.setNum(0);
  this->filenames.setDefault(0);

  PRIVATE(this)->glimage = NULL;
  PRIVATE(this)->glimagevalid = FALSE;
  PRIVATE(this)->readstatus = 1;

  // use field sensor for filename since we will load an image if
  // filename changes. This is a time-consuming task which should
  // not be done in notify().
  PRIVATE(this)->filenames_sensor = new SoFieldSensor(filenameSensorCB, this);
  PRIVATE(this)->filenames_sensor->setPriority(0);
  PRIVATE(this)->filenames_sensor->attach(&this->filenames);

}

/*!
  Destructor. Frees up internal resources used to store texture image
  data.
*/
SoTextureCubeMap::~SoTextureCubeMap()
{
  if (PRIVATE(this)->glimage) PRIVATE(this)->glimage->unref(NULL);
  delete PRIVATE(this)->filenames_sensor;
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCubeMap::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCubeMap, SO_FROM_COIN_2_4);

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureImageElement);
  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureEnabledElement);

  SO_ENABLE(SoCallbackAction, SoMultiTextureImageElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureEnabledElement);

  SO_ENABLE(SoRayPickAction, SoMultiTextureImageElement);
  SO_ENABLE(SoRayPickAction, SoMultiTextureEnabledElement);
}


// Documented in superclass. Overridden to check if texture file (if
// any) can be found and loaded.
SbBool
SoTextureCubeMap::readInstance(SoInput * in, unsigned short flags)
{
  PRIVATE(this)->filenames_sensor->detach();

  SbBool readOK = inherited::readInstance(in, flags);
  this->setReadStatus((int) readOK);
  if (readOK) {
    for (int i = 0; i < this->filenames.getNum(); i++) {
      const SbString & fn = this->filenames[i];
      SoSFImage * img;
      switch (i) {
      default:
      case 0: img = &this->imageNegX; break;
      case 1: img = &this->imagePosX; break;
      case 2: img = &this->imageNegY; break;
      case 3: img = &this->imagePosY; break;
      case 4: img = &this->imageNegZ; break;
      case 5: img = &this->imagePosZ; break;
      }
      // only load if filename is set last (no image data is saved to
      // the image field)
      if (img->isDefault() && fn.getLength()) {
        if (!this->loadFilename(fn, img)) {
          SoReadError::post(in, "Could not read texture file '%s'",
                            fn.getString());
          this->setReadStatus(FALSE);
        }
      }
    }
  }
  PRIVATE(this)->filenames_sensor->attach(&this->filenames);
  return readOK;
}

// Documented in superclass.
void
SoTextureCubeMap::GLRender(SoGLRenderAction * action)
{
  // FIXME: consider sharing textures among contexts, pederb
  SoState * state = action->getState();

  if (SoTextureOverrideElement::getImageOverride(state))
    return;

  float quality = SoTextureQualityElement::get(state);

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));

  LOCK_GLIMAGE(this);

  if (!PRIVATE(this)->glimagevalid) {    
    if (PRIVATE(this)->glimage) PRIVATE(this)->glimage->unref(state);
    PRIVATE(this)->glimage = new SoGLCubeMapImage();
    
    for (int i = 0; i < 6; i++) {
      SoSFImage * img = this->getImageField(i);

      SbVec2s size;
      int nc;
      const unsigned char * bytes = img->getValue(size, nc);
      
      if (bytes && size != SbVec2s(0,0)) {
        PRIVATE(this)->glimage->setCubeMapImage((SoGLCubeMapImage::Target)i, bytes, size, nc);
        // don't cache while creating a texture object
        SoCacheElement::setInvalid(TRUE);
        if (state->isCacheOpen()) {
          SoCacheElement::invalidate(state);
        }
      }
    }
    if (state->isCacheOpen()) {
      SoCacheElement::invalidate(state);
    }
    PRIVATE(this)->glimagevalid = TRUE;
  }
  
  UNLOCK_GLIMAGE(this);
  
  SoMultiTextureImageElement::Model glmodel = (SoMultiTextureImageElement::Model) 
    this->model.getValue();
  
  if (glmodel == SoMultiTextureImageElement::REPLACE) {
    if (!cc_glglue_glversion_matches_at_least(glue, 1, 1, 0)) {
      static int didwarn = 0;
      if (!didwarn) {
        SoDebugError::postWarning("SoTextureCubeMap::GLRender",
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
                                      PRIVATE(this)->glimagevalid ? PRIVATE(this)->glimage : NULL,
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
SoTextureCubeMap::doAction(SoAction * COIN_UNUSED_ARG(action))
{
#if 0 // not implemented yet
  SoState * state = action->getState();

  if (SoTextureOverrideElement::getImageOverride(state))
    return;

  int nc;
  SbVec2s size;
  const unsigned char * bytes = this->image.getValue(size, nc);

  if (size != SbVec2s(0,0)) {
    SoTextureImageElement::set(state, this,
                               size, nc, bytes,
                               (int)this->wrapT.getValue(),
                               (int)this->wrapS.getValue(),
                               (SoTextureImageElement::Model) model.getValue(),
                               this->blendColor.getValue());
    SoTextureEnabledElement::set(state, this, TRUE);
  }
  // if a filename has been set, but the file has not been loaded, supply
  // a dummy texture image to make sure texture coordinates are generated.
  else if (this->image.isDefault() && this->filename.getValue().getLength()) {
    static const unsigned char dummytex[] = {0xff,0xff,0xff,0xff};
    SoTextureImageElement::set(state, this,
                               SbVec2s(2,2), 1, dummytex,
                               (int)this->wrapT.getValue(),
                               (int)this->wrapS.getValue(),
                               (SoTextureImageElement::Model) model.getValue(),
                               this->blendColor.getValue());
    SoTextureEnabledElement::set(state, this, TRUE);
  }
  else {
    SoTextureImageElement::setDefault(state, this);
    SoTextureEnabledElement::set(state, this, FALSE);
  }
  if (this->isOverride()) {
    SoTextureOverrideElement::setImageOverride(state, TRUE);
  }
#endif // not implemented
}

// doc from parent
void
SoTextureCubeMap::callback(SoCallbackAction * action)
{
  SoTextureCubeMap::doAction(action);
}

// doc from parent
void
SoTextureCubeMap::rayPick(SoRayPickAction * action)
{
  SoTextureCubeMap::doAction(action);
}

/*!
  Not implemented in Coin; should probably not have been public in the
  original SGI Open Inventor API.  We'll consider to implement it if
  requested.
*/
SbBool
SoTextureCubeMap::readImage(const SbString & COIN_UNUSED_ARG(fname), int & COIN_UNUSED_ARG(w), int & COIN_UNUSED_ARG(h), int & COIN_UNUSED_ARG(nc),
                      unsigned char *& COIN_UNUSED_ARG(bytes))
{
  COIN_OBSOLETED();
  return FALSE;
}

/*!
  Returns read status. 1 for success, 0 for failure.
*/
int
SoTextureCubeMap::getReadStatus(void)
{
  return PRIVATE(this)->readstatus;
}

/*!
  Sets read status.
  \sa getReadStatus()
 */
void
SoTextureCubeMap::setReadStatus(int s)
{
  PRIVATE(this)->readstatus = s;
}

// Documented in superclass. Overridden to detect when fields change.
void
SoTextureCubeMap::notify(SoNotList * l)
{
  SoField * f = l->getLastField();
  if (f == &this->imagePosX) {
    PRIVATE(this)->glimagevalid = FALSE;    
    this->imagePosX.setDefault(FALSE);
  }
  else if (f == &this->imageNegX) {
    PRIVATE(this)->glimagevalid = FALSE;
    this->imageNegX.setDefault(FALSE);
  }
  
  else if (f == &this->imagePosY) {
    PRIVATE(this)->glimagevalid = FALSE;
    this->imagePosY.setDefault(FALSE);
  }
  else if (f == &this->imageNegY) {
    PRIVATE(this)->glimagevalid = FALSE;    
    this->imageNegY.setDefault(FALSE);
  }
  
  else if (f == &this->imagePosZ) {
    PRIVATE(this)->glimagevalid = FALSE;
    this->imagePosZ.setDefault(FALSE);
  }
  else if (f == &this->imageNegZ) {
    PRIVATE(this)->glimagevalid = FALSE;
    this->imageNegZ.setDefault(FALSE);
  }

  else if (f == &this->wrapS || f == &this->wrapT) {
    PRIVATE(this)->glimagevalid = FALSE;
  }
  inherited::notify(l);
}

//
// Called from readInstance() or when user changes the
// filename field.
//
SbBool 
SoTextureCubeMap::loadFilename(const SbString & filename, SoSFImage * image)
{
  SbBool retval = FALSE;
  if (filename.getLength()) {
    SbImage tmpimage;
    const SbStringList & sl = SoInput::getDirectories();
    if (tmpimage.readFile(filename,
                          sl.getArrayPtr(), sl.getLength())) {
      int nc;
      SbVec2s size;
      unsigned char * bytes = tmpimage.getValue(size, nc);
      // disable notification on image while setting data from filename
      // as a notify will cause a filename.setDefault(TRUE).
      SbBool oldnotify = image->enableNotify(FALSE);
      image->setValue(size, nc, bytes);
      image->enableNotify(oldnotify);
      PRIVATE(this)->glimagevalid = FALSE; // recreate GL image in next GLRender()
      retval = TRUE;
    }
  }
  image->setDefault(TRUE); // write filename, not image
  return retval;
}

//
// called when filename changes
//
void
SoTextureCubeMap::filenameSensorCB(void * data, SoSensor * COIN_UNUSED_ARG(s))
{
  SoTextureCubeMap * thisp = (SoTextureCubeMap*) data;

  thisp->setReadStatus(1);


  for (int i = 0; i < thisp->filenames.getNum(); i++) {
    const SbString & fn = thisp->filenames[i];
    SoSFImage * img = thisp->getImageField(i);
    
    if (fn.getLength() &&
        !thisp->loadFilename(fn, img)) {
      SoDebugError::postWarning("SoTextureCubeMap::filenameSensorCB",
                                "Image file '%s' could not be read",
                                fn.getString());
      thisp->setReadStatus(0);
    }
  }
}

SoSFImage * 
SoTextureCubeMap::getImageField(const int idx)
{
  SoSFImage * img = NULL;
  switch (idx) {
  default:
  case 0: img = &this->imageNegX; break;
  case 1: img = &this->imagePosX; break;
  case 2: img = &this->imageNegY; break;
  case 3: img = &this->imagePosY; break;
  case 4: img = &this->imageNegZ; break;
  case 5: img = &this->imagePosZ; break;
  }
  return img;
}


#undef LOCK_GLIMAGE
#undef UNLOCK_GLIMAGE

#undef PRIVATE
