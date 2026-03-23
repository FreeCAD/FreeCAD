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
  \class SoTexture2 SoTexture2.h Inventor/nodes/SoTexture2.h
  \brief The SoTexture2 class is used to map a 2D texture onto subsequent geometry in the scene graph.

  \ingroup coin_nodes

  Shape nodes within the scope of SoTexture2 nodes in the scene graph
  (i.e. below the same SoSeparator and to the right hand side of the
  SoTexture2) will have the texture applied according to each shape
  type's individual characteristics.  See the documentation of the
  various shape types (SoFaceSet, SoCube, SoSphere, etc.) for
  information about the specifics of how the textures will be applied.

  For a simple usage example, see the class documentation for SoSFImage.

  One common flaw with many programs that have support for exporting
  VRML or Inventor files, is that the same texture file is exported
  several times, but as different nodes. This can cause excessive
  texture memory usage and slow rendering. Below is an example program
  that fixes this by replacing all instances of the same texture with
  a pointer to the first node:

  \code

  #include <Inventor/actions/SoSearchAction.h>
  #include <Inventor/actions/SoWriteAction.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoTexture2.h>
  #include <Inventor/SoDB.h>
  #include <Inventor/SoInput.h>
  #include <Inventor/SoOutput.h>

  int main(int argc, char ** argv)
  {
    if (argc < 2) return -1;
    SoDB::init();

    SoInput in;
    if (!in.openFile(argv[1])) return -1;

    SoSeparator * root = SoDB::readAll(&in);
    if (!root) return -1;
    root->ref();

    SoSearchAction sa;
    sa.setType(SoTexture2::getClassTypeId());
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(TRUE);
    sa.apply(root);
    SoPathList & pl = sa.getPaths();
    SbDict namedict;

    for (int i = 0; i < pl.getLength(); i++) {
      SoFullPath * p = (SoFullPath*) pl[i];
      if (p->getTail()->isOfType(SoTexture2::getClassTypeId())) {
        SoTexture2 * tex = (SoTexture2*) p->getTail();
        if (tex->filename.getValue().getLength()) {
          SbName name = tex->filename.getValue().getString();
          unsigned long key = (unsigned long) ((void*) name.getString());
          void * tmp;
          if (!namedict.find(key, tmp)) {
            // new texture. just insert into list
            (void) namedict.enter(key, tex);
          }
          else if (tmp != (void*) tex) { // replace with node found in dict
            SoGroup * parent = (SoGroup*) p->getNodeFromTail(1);
            int idx = p->getIndexFromTail(0);
            parent->replaceChild(idx, (SoNode*) tmp);
          }
        }
      }
    }
    sa.reset();

    // output fixed scene to stdout
    SoOutput out;
    SoWriteAction wa(&out);
    wa.apply(root);
    root->unref();
  }
  \endcode

  When working with Inventor files, one often wants to embed external
  texture image files into the Inventor files themselves. Here's a
  minimal, standalone example which shows how that can be
  accomplished by calling SoField::touch() on the SoTexture2::image
  fields before scene graph export:

  \code
  #include <Inventor/SoDB.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/nodes/SoTexture2.h>
  #include <Inventor/actions/SoSearchAction.h>
  #include <Inventor/actions/SoWriteAction.h>

  int
  main(void)
  {
    if (argc == 1) {
      fprintf(stderr,"Usage: %s <infile> > outfile.iv\n", argv[0]);
      return -1;
    }
    SoDB::init();
  
    // open the file (don't use stdin) to enable Coin to search for textures
    // relative to the input file.
    SoInput in;
    if (!in.openFile(argv[1])) {
      return -1;
    }
    SoSeparator * root = SoDB::readAll(&in);
    if (!root) { exit(1); }

    root->ref();

    SoSearchAction searchaction;
    searchaction.setType(SoTexture2::getClassTypeId());
    searchaction.setSearchingAll(TRUE);
    searchaction.setInterest(SoSearchAction::ALL);

    searchaction.apply(root);

    const SoPathList & pl = searchaction.getPaths();
    for (int i=0; i < pl.getLength(); i++) {
      SoFullPath * fp = (SoFullPath *)pl[i];
      SoTexture2 * tex = (SoTexture2 *)fp->getTail();
      assert(tex->getTypeId() == SoTexture2::getClassTypeId());
      tex->image.touch();
    }

    SoWriteAction wa;
    wa.apply(root);

    root->unref();

    return 0;
  }
  \endcode

  Run the example by piping the iv-file you want to do texture embedding
  on from stdin, e.g. like this:

  \verbatim
  $ ./test < input.iv
  \endverbatim

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Texture2 {
        filename ""
        image 0 0 0
        wrapS REPEAT
        wrapT REPEAT
        model MODULATE
        blendColor 0 0 0
        enableCompressedTexture FALSE
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoTexture2.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "coindefs.h" // COIN_OBSOLETED()
#include "elements/SoTextureScalePolicyElement.h"
#include "nodes/SoSubNodeP.h"
#include "tidbitsp.h"
#include <Inventor/C/glue/gl.h>
#include <Inventor/SbImage.h>
#include <Inventor/SoInput.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/misc/SoGLBigImage.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/threads/SbMutex.h>

// *************************************************************************

/*!
  \enum SoTexture2::Model

  Texture mapping model, for deciding how to "merge" the texture map
  with the object it is mapped onto.
*/
/*!
  \var SoTexture2::Model SoTexture2::MODULATE

  Texture color is multiplied by the polygon color. The result will
  be Phong shaded (if light model is PHONG).
*/
/*!
  \var SoTexture2::Model SoTexture2::DECAL

  Texture image overwrites polygon shading. Textured pixels will
  not be Phong shaded. Has undefined behaviour for grayscale and
  grayscale-alpha textures.
*/
/*!
  \var SoTexture2::Model SoTexture2::BLEND

  This model is normally used with monochrome textures (i.e. textures
  with one or two components). The first component, the intensity, is
  then used to blend between the shaded color of the polygon and the
  SoTexture2::blendColor.
*/
/*!
  \var SoTexture2::Model SoTexture2::REPLACE

  Texture image overwrites polygon shading. Textured pixels will not
  be Phong shaded. Supports grayscale and grayscale alpha
  textures. This feature requires OpenGL 1.1. MODULATE will be used if
  OpenGL version < 1.1 is detected.

  Please note that using this texture model will make your Inventor
  files incompatible with older versions of Coin and Inventor. You
  need Coin >= 2.2 or TGS Inventor 4.0 to load Inventor files that
  uses the REPLACE texture model.

  \since Coin 2.2
  \since TGS Inventor 4.0
*/ 

/*!
  \enum SoTexture2::Wrap

  Enumeration of wrapping strategies which can be used when the
  texture map doesn't cover the full extent of the geometry.
*/
/*!
  \var SoTexture2::Wrap SoTexture2::REPEAT
  Repeat texture when coordinate is not between 0 and 1.
*/
/*!
  \var SoTexture2::Wrap SoTexture2::CLAMP
  Clamp coordinate between 0 and 1.
*/

/*!
  \var SoSFString SoTexture2::filename

  Texture filename, referring to a file on disk in a supported image
  bitmap format.

  By default contains an empty string, which means the texture will be
  fetched from SoTexture2::image and not from disk. (Specify either
  this field or use SoTexture2::image, not both.)

  FIXME: Section about simage is outdated.

  For reading texture image files from disk, Coin uses the "simage"
  library from Kongsberg Oil & Gas Technologies. This library must be
  installed for it to be possible to load any image files at all.

  The set of image formats you can actually read with the simage
  library depends on how the simage library was built, as simage is
  again dependent on a few other low level file import/export
  libraries (for instance for JPEG, GIF, TIFF and PNG support).  To
  make sure all wanted formats are supported, you should build the
  simage library yourself.

  The set of image formats guaranteed to be supported by simage is
  Targa (.tga), PIC (.pic), SGI RGB (.rgb, .bw) and XWD (.xwd).

  For more information about the simage library, including download
  and build instructions, see the <a href="https://github.com/coin3d/">Coin
  repository</a>.
*/
/*!
  \var SoSFImage SoTexture2::image

  Inline image data. Defaults to contain an empty image.

  See documentation of the SoSFImage class for a very detailed
  description of how the format specification for the image data is
  laid out, and what different image formats for color textures,
  semi-transparent textures, grayscale textures, etc., are
  supported.
*/
/*!
  \var SoSFEnum SoTexture2::wrapS

  Wrapping strategy for the S coordinate when the texture map is
  narrower than the object to map onto.

  Default value is SoTexture2::REPEAT.
*/
/*!
  \var SoSFEnum SoTexture2::wrapT

  Wrapping strategy for the T coordinate when the texture map is
  shorter than the object to map onto.

  Default value is SoTexture2::REPEAT.
*/
/*!
  \var SoSFEnum SoTexture2::model

  Texture mapping model for how the texture map is "merged" with the
  polygon primitives it is applied to. Default value is
  SoTexture2::MODULATE.
*/
/*!
  \var SoSFColor SoTexture2::blendColor

  Blend color. Used when SoTexture2::model is SoTexture2::BLEND.

  Default color value is [0, 0, 0], black, which means no contribution
  to the blending is made.
*/

/*!
  \var SoSFBool SoTexture2::enableCompressedTexture

  Hint to Coin that compressed textures should be used if this
  is supported by the graphics hardware and OpenGL drivers.
  Using compressed textures usually reduces texture memory usage
  for a texture by 4-6 times.

  \since Coin 2.4.2
  \since TGS Inventor 4.0
*/

// *************************************************************************

class SoTexture2P {
public:
  SoGLImage * glimage;
  SoFieldSensor * filenamesensor;
  static SbMutex * mutex;
  int readstatus;
  SbBool glimagevalid;

  static void cleanup(void) {
    delete SoTexture2P::mutex;
    SoTexture2P::mutex = NULL;
  }
};

SbMutex * SoTexture2P::mutex = NULL;

#define PRIVATE(p) ((p)->pimpl)

// *************************************************************************

#ifdef COIN_THREADSAFE
#define LOCK_GLIMAGE(_thisp_) (PRIVATE(_thisp_)->mutex->lock())
#define UNLOCK_GLIMAGE(_thisp_) (PRIVATE(_thisp_)->mutex->unlock())
#else // COIN_THREADSAFE
#define LOCK_GLIMAGE(_thisp_)
#define UNLOCK_GLIMAGE(_thisp_)
#endif // COIN_THREADSAFE

// *************************************************************************

SO_NODE_SOURCE(SoTexture2);

/*!
  Constructor.
*/
SoTexture2::SoTexture2(void)
{
  PRIVATE(this) = new SoTexture2P;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoTexture2);

  SO_NODE_ADD_FIELD(filename, (""));
  SO_NODE_ADD_FIELD(image, (SbVec2s(0, 0), 0, NULL));
  SO_NODE_ADD_FIELD(wrapS, (REPEAT));
  SO_NODE_ADD_FIELD(wrapT, (REPEAT));
  SO_NODE_ADD_FIELD(model, (MODULATE));
  SO_NODE_ADD_FIELD(blendColor, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(enableCompressedTexture, (FALSE));

  SO_NODE_DEFINE_ENUM_VALUE(Wrap, REPEAT);
  SO_NODE_DEFINE_ENUM_VALUE(Wrap, CLAMP);

  SO_NODE_SET_SF_ENUM_TYPE(wrapS, Wrap);
  SO_NODE_SET_SF_ENUM_TYPE(wrapT, Wrap);

  SO_NODE_DEFINE_ENUM_VALUE(Model, MODULATE);
  SO_NODE_DEFINE_ENUM_VALUE(Model, DECAL);
  SO_NODE_DEFINE_ENUM_VALUE(Model, BLEND);
  SO_NODE_DEFINE_ENUM_VALUE(Model, REPLACE);
  SO_NODE_SET_SF_ENUM_TYPE(model, Model);

  PRIVATE(this)->glimage = NULL;
  PRIVATE(this)->glimagevalid = FALSE;
  PRIVATE(this)->readstatus = 1;

  // use field sensor for filename since we will load an image if
  // filename changes. This is a time-consuming task which should
  // not be done in notify().
  PRIVATE(this)->filenamesensor = new SoFieldSensor(filenameSensorCB, this);
  PRIVATE(this)->filenamesensor->setPriority(0);
  PRIVATE(this)->filenamesensor->attach(&this->filename);
}

/*!
  Destructor. Frees up internal resources used to store texture image
  data.
*/
SoTexture2::~SoTexture2()
{
  if (PRIVATE(this)->glimage) PRIVATE(this)->glimage->unref(NULL);
  delete PRIVATE(this)->filenamesensor;
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTexture2::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTexture2, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureImageElement);
  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureEnabledElement);

  SO_ENABLE(SoCallbackAction, SoMultiTextureEnabledElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureImageElement);

  SO_ENABLE(SoRayPickAction, SoMultiTextureEnabledElement);
  SO_ENABLE(SoRayPickAction, SoMultiTextureImageElement);

#ifdef COIN_THREADSAFE
  SoTexture2P::mutex = new SbMutex;
#endif // COIN_THREADSAFE

  coin_atexit(SoTexture2P::cleanup, CC_ATEXIT_NORMAL);
}


// Documented in superclass. Overridden to check if texture file (if
// any) can be found and loaded.
SbBool
SoTexture2::readInstance(SoInput * in, unsigned short flags)
{
  PRIVATE(this)->filenamesensor->detach();
  SbBool readOK = inherited::readInstance(in, flags);
  this->setReadStatus((int) readOK);
  if (readOK && !filename.isDefault() && filename.getValue() != "") {
    if (!this->loadFilename()) {
      SoReadError::post(in, "Could not read texture file '%s'",
                        filename.getValue().getString());
      this->setReadStatus(FALSE);
    }
  }
  PRIVATE(this)->filenamesensor->attach(&this->filename);
  return readOK;
}

static SoGLImage::Wrap
translateWrap(const SoTexture2::Wrap wrap)
{
  if (wrap == SoTexture2::REPEAT) return SoGLImage::REPEAT;
  return SoGLImage::CLAMP;
}

// Documented in superclass.
void
SoTexture2::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);
  
  if ((unit == 0) && SoTextureOverrideElement::getImageOverride(state))
    return;

  float quality = SoTextureQualityElement::get(state);

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));
  SoTextureScalePolicyElement::Policy scalepolicy =
    SoTextureScalePolicyElement::get(state);
  SbBool needbig = (scalepolicy == SoTextureScalePolicyElement::FRACTURE);
  SoType glimagetype = PRIVATE(this)->glimage ? PRIVATE(this)->glimage->getTypeId() : SoType::badType();
    
  LOCK_GLIMAGE(this);
  
  if (!PRIVATE(this)->glimagevalid || 
      (needbig && glimagetype != SoGLBigImage::getClassTypeId()) ||
      (!needbig && glimagetype != SoGLImage::getClassTypeId())) {
    int nc;
    SbVec2s size;
    const unsigned char * bytes =
      this->image.getValue(size, nc);
    
    if (needbig &&
        (glimagetype != SoGLBigImage::getClassTypeId())) {
      if (PRIVATE(this)->glimage) PRIVATE(this)->glimage->unref(state);
      PRIVATE(this)->glimage = new SoGLBigImage();
    }
    else if (!needbig &&
             (glimagetype != SoGLImage::getClassTypeId())) {
      if (PRIVATE(this)->glimage) PRIVATE(this)->glimage->unref(state);
      PRIVATE(this)->glimage = new SoGLImage();
    }
    
    if (this->enableCompressedTexture.getValue()) {
      PRIVATE(this)->glimage->setFlags(PRIVATE(this)->glimage->getFlags()|
                                       SoGLImage::COMPRESSED);
    }

    if (scalepolicy == SoTextureScalePolicyElement::SCALE_DOWN) {
      PRIVATE(this)->glimage->setFlags(PRIVATE(this)->glimage->getFlags()|SoGLImage::SCALE_DOWN);
    }

    if (bytes && size != SbVec2s(0,0)) {
      PRIVATE(this)->glimage->setData(bytes, size, nc,
                             translateWrap((Wrap)this->wrapS.getValue()),
                             translateWrap((Wrap)this->wrapT.getValue()),
                             quality);
      PRIVATE(this)->glimagevalid = TRUE;
      // don't cache while creating a texture object
      SoCacheElement::setInvalid(TRUE);
      if (state->isCacheOpen()) {
        SoCacheElement::invalidate(state);
      }
    }
  }

  if (PRIVATE(this)->glimage && PRIVATE(this)->glimage->getTypeId() == SoGLBigImage::getClassTypeId()) {
    SoCacheElement::invalidate(state);
  }

  UNLOCK_GLIMAGE(this);
  
  SoMultiTextureImageElement::Model glmodel = (SoMultiTextureImageElement::Model) 
    this->model.getValue();
  
  if (glmodel == SoMultiTextureImageElement::REPLACE) {
    if (!cc_glglue_glversion_matches_at_least(glue, 1, 1, 0)) {
      static int didwarn = 0;
      if (!didwarn) {
        SoDebugError::postWarning("SoTexture2::GLRender",
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
  
  int maxunits = cc_glglue_max_texture_units(glue);
  if (unit < maxunits) {
    SoGLMultiTextureImageElement::set(state, this, unit,
                                      PRIVATE(this)->glimagevalid ? PRIVATE(this)->glimage : NULL,
                                      glmodel,
                                      this->blendColor.getValue());
    
    SoGLMultiTextureEnabledElement::set(state, this, unit,
                                        PRIVATE(this)->glimagevalid &&
                                        quality > 0.0f);
  }
  else {
    // we already warned in SoTextureUnit. I think it's best to just
    // ignore the texture here so that all texture for non-supported
    // units will be ignored. pederb, 2003-11-04
  }
}

// Documented in superclass.
void
SoTexture2::doAction(SoAction * action)
{
  SoState * state = action->getState();

  int unit = SoTextureUnitElement::get(state);
  if ((unit == 0) && SoTextureOverrideElement::getImageOverride(state))
    return;

  int nc;
  SbVec2s size;
  const unsigned char * bytes = this->image.getValue(size, nc);
  
  // if a filename has been set, but the file has not been loaded, supply
  // a dummy texture image to make sure texture coordinates are generated.
  if ((size == SbVec2s(0,0)) &&
      this->image.isDefault() && 
      this->filename.getValue().getLength()) {
    static const unsigned char dummytex[] = {0xff,0xff,0xff,0xff};
    bytes = dummytex;
    size = SbVec2s(2,2);
    nc = 1;
  }

  if (size != SbVec2s(0,0)) {
    SoMultiTextureImageElement::set(state, this, unit,
                                    size, nc, bytes,
                                    (SoMultiTextureImageElement::Wrap)this->wrapT.getValue(),
                                    (SoMultiTextureImageElement::Wrap)this->wrapS.getValue(),
                                    (SoMultiTextureImageElement::Model) model.getValue(),
                                    this->blendColor.getValue());
    SoMultiTextureEnabledElement::set(state, this, unit, TRUE); 
  }
  else {
    SoMultiTextureImageElement::setDefault(state, this, unit);
    SoMultiTextureEnabledElement::set(state, this, unit, FALSE);
  }
}

// doc from parent
void
SoTexture2::callback(SoCallbackAction * action)
{
  SoTexture2::doAction(action);
}

// doc from parent
void
SoTexture2::rayPick(SoRayPickAction * action)
{
  SoTexture2::doAction(action);
}

/*!
  Not implemented in Coin; should probably not have been public in the
  original SGI Open Inventor API.  We'll consider to implement it if
  requested.
*/
SbBool
SoTexture2::readImage(const SbString & COIN_UNUSED_ARG(fname), int & COIN_UNUSED_ARG(w), int & COIN_UNUSED_ARG(h), int & COIN_UNUSED_ARG(nc),
                      unsigned char *& COIN_UNUSED_ARG(bytes))
{
  COIN_OBSOLETED();
  return FALSE;
}

/*!
  Returns read status. 1 for success, 0 for failure.
*/
int
SoTexture2::getReadStatus(void)
{
  return PRIVATE(this)->readstatus;
}

/*!
  Sets read status.
  \sa getReadStatus()
 */
void
SoTexture2::setReadStatus(int s)
{
  PRIVATE(this)->readstatus = s;
}

// Documented in superclass. Overridden to detect when fields change.
void
SoTexture2::notify(SoNotList * l)
{
  SoField * f = l->getLastField();
  if (f == &this->image) {
    PRIVATE(this)->glimagevalid = FALSE;

    // write image, not filename
    this->filename.setDefault(TRUE);
    this->image.setDefault(FALSE);
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
SoTexture2::loadFilename(void)
{
  SbBool retval = FALSE;
  if (this->filename.getValue().getLength()) {
    SbImage tmpimage;
    const SbStringList & sl = SoInput::getDirectories();
    if (tmpimage.readFile(this->filename.getValue(),
                          sl.getArrayPtr(), sl.getLength())) {
      int nc;
      SbVec2s size;
      unsigned char * bytes = tmpimage.getValue(size, nc);
      // disable notification on image while setting data from filename
      // as a notify will cause a filename.setDefault(TRUE).
      SbBool oldnotify = this->image.enableNotify(FALSE);
      this->image.setValue(size, nc, bytes);
      this->image.enableNotify(oldnotify);
      PRIVATE(this)->glimagevalid = FALSE; // recreate GL image in next GLRender()
      retval = TRUE;
    }
  }
  this->image.setDefault(TRUE); // write filename, not image
  return retval;
}

//
// called when filename changes
//
void
SoTexture2::filenameSensorCB(void * data, SoSensor *)
{
  SoTexture2 * thisp = (SoTexture2*) data;

  thisp->setReadStatus(1);
  if (thisp->filename.getValue().getLength() &&
      !thisp->loadFilename()) {
    SoDebugError::postWarning("SoTexture2::filenameSensorCB",
                              "Image file '%s' could not be read",
                              thisp->filename.getValue().getString());
    thisp->setReadStatus(0);
  }
  else if (thisp->filename.getValue() == "") {
    // setting filename to "" should reset the node to its initial state
    thisp->setReadStatus(0);
    thisp->image.setValue(SbVec2s(0,0), 0, NULL);
    thisp->image.setDefault(TRUE);
    thisp->filename.setDefault(TRUE);
  }
}

#undef LOCK_GLIMAGE
#undef UNLOCK_GLIMAGE
#undef PRIVATE
