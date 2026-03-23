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
  \class SoBumpMap SoBumpMap.h Inventor/nodes/SoBumpMap.h
  \brief The SoBumpMap class is used to map a bump map onto subsequent shapes.

  \ingroup coin_nodes

  SoBumpMap has support for two types of maps. If the image is a three
  component (RGB) image, it will be treated as a normal map, where the
  red image component equals the X normal component, green equals Y,
  and blue is Z. See
  http://www.paulsprojects.net/tutorials/simplebump/simplebump.html
  for a nice introduction about bump mapping and normal maps.

  If the image is a grayscale image, it will be treated as a height
  map, and automatically converted to a normal map.

  For bump mapping to work with extension nodes for Coin, the
  SoShape::generatePrimitives() method must be correctly implemented
  for the shape. This is needed since tangent space coordinates needs
  to be calculated for each vertex in the shape. All shape nodes which
  are a standard part of Coin meets this criteria.

  Bump mapping in Coin requires OpenGL 1.3, or the following OpenGL
  extensions: GL_ARB_multitexture, GL_ARB_texture_cube_map,
  GL_ARB_texture_env_combine and GL_ARB_texture_env_dot3 (or the
  corresponding EXT extensions). If the runtime system doesn't meet
  these requirements, Coin will post a warning and the bump map will
  simply be ignored.

  GL_ARB_vertex_program and GL_ARB_fragment_program is required to get
  specular lighting on the bumps. If these extensions are not
  available, the bumps will be rendered with diffuse lighting only.

  Bump mapped objects will be rendered with multiple rendering
  passes. One extra pass per light source for diffuse only bumps, and
  two extra passes per light source for diffuse and specular
  bumps. You can turn off specular lighting on the bumps by setting
  specularColor to (0.0, 0.0, 0.0).

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    BumpMap {
        filename ""
        image 0 0 0

        wrapS REPEAT
        wrapT REPEAT
    }
  \endcode

  \since Coin 2.2
*/

#include <Inventor/nodes/SoBumpMap.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/SoInput.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoBumpMapElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbImage.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/misc/SoGLImage.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/engines/SoHeightMapToNormalMap.h>

#include "coindefs.h" // COIN_OBSOLETED()
#include "nodes/SoSubNodeP.h"

/*!
  \enum SoBumpMap::Wrap

  Enumeration of wrapping strategies which can be used when the
  bump map doesn't cover the full extent of the geometry.
*/
/*!
  \var SoBumpMap::Wrap SoBumpMap::REPEAT
  Repeat bump map  when coordinate is not between 0 and 1.
*/
/*!
  \var SoBumpMap::Wrap SoBumpMap::CLAMP
  Clamp coordinate between 0 and 1.
*/

/*!
  \var SoSFString SoBumpMap::filename

  Bump map (or normal map) filename, referring to a file on disk in a
  supported image bitmap format. See SoBumpMap::filename for more
  information.
*/

/*!
  \var SoSFImage SoBumpMap::image

  Inline image data. Defaults to contain an empty bump map.

*/

/*!
  \var SoSFEnum SoBumpMap::wrapS

  Wrapping strategy for the S coordinate when the bump map is
  narrower than the object to map onto.

  Default value is SoBumpMap::REPEAT.
*/
/*!
  \var SoSFEnum SoBumpMap::wrapT

  Wrapping strategy for the T coordinate when the bump map is
  shorter than the object to map onto.

  Default value is SoBumpMap::REPEAT.
*/

// *************************************************************************

class SoBumpMapP {
public:
  SoFieldSensor * filenamesensor;
  SoGLImage * glimage;
  SbBool glimagevalid;
  SbImage convertedheightmap;
  SbBool didconvert;
  int isgrayscale; // -1 = unknown, 0 = no, 1 = yes

  void testGrayscale(const unsigned char * srcptr,
                     const SbVec2s & size,
                     const int nc)
  {
    // check if we have a cached value
    if (this->isgrayscale >= 0) return;

    if (nc < 3) {
      this->isgrayscale = 1;
    }
    else {
      const unsigned char * src = srcptr;
      this->isgrayscale = 1;
      const int n = size[0]*size[1];
      for (int i = 0; i < n; i++) {
        if ((src[0] != src[1]) || (src[0] != src[2])) {
          this->isgrayscale = 0;
          return;
        }
        src += nc;
      }
    }
  }
};

#define PRIVATE(p) (p->pimpl)

SO_NODE_SOURCE(SoBumpMap);

/*!
  Constructor.
*/
SoBumpMap::SoBumpMap(void)
{
  PRIVATE(this) = new SoBumpMapP;
  PRIVATE(this)->glimage = new SoGLImage;
  PRIVATE(this)->glimagevalid = FALSE;
  PRIVATE(this)->didconvert = FALSE;
  PRIVATE(this)->isgrayscale = -1;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoBumpMap);

  SO_NODE_ADD_FIELD(filename, (""));
  SO_NODE_ADD_FIELD(image, (SbVec2s(0, 0), 0, NULL));
  SO_NODE_ADD_FIELD(wrapS, (REPEAT));
  SO_NODE_ADD_FIELD(wrapT, (REPEAT));

  SO_NODE_DEFINE_ENUM_VALUE(Wrap, REPEAT);
  SO_NODE_DEFINE_ENUM_VALUE(Wrap, CLAMP);

  SO_NODE_SET_SF_ENUM_TYPE(wrapS, Wrap);
  SO_NODE_SET_SF_ENUM_TYPE(wrapT, Wrap);

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
SoBumpMap::~SoBumpMap()
{
  PRIVATE(this)->glimage->unref(NULL);
  delete PRIVATE(this)->filenamesensor;
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoBumpMap::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoBumpMap, SO_FROM_COIN_2_2);

  SO_ENABLE(SoGLRenderAction, SoBumpMapElement);
  SO_ENABLE(SoCallbackAction, SoBumpMapElement);
  SO_ENABLE(SoRayPickAction, SoBumpMapElement);
}


// Documented in superclass. Overridden to check if texture file (if
// any) can be found and loaded.
SbBool
SoBumpMap::readInstance(SoInput * in, unsigned short flags)
{
  PRIVATE(this)->filenamesensor->detach();
  SbBool readOK = inherited::readInstance(in, flags);
  if (readOK && !filename.isDefault() && filename.getValue() != "") {
    if (!this->loadFilename()) {
      SoReadError::post(in, "Could not read texture file '%s'",
                        filename.getValue().getString());
    }
  }
  PRIVATE(this)->filenamesensor->attach(&this->filename);
  PRIVATE(this)->glimagevalid = FALSE;
  return readOK;
}

static SoGLImage::Wrap
bumpmap_translateWrap(const SoBumpMap::Wrap wrap)
{
  if (wrap == SoBumpMap::REPEAT) return SoGLImage::REPEAT;
  return SoGLImage::CLAMP;
}


// Documented in superclass.
void
SoBumpMap::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  const cc_glglue * glue = cc_glglue_instance(action->getCacheContext());

  if (SoGLDriverDatabase::isSupported(glue, SO_GL_BUMPMAPPING)) {
    int nc;
    SbVec2s size;
    const unsigned char * bytes = this->image.getValue(size, nc);

    if (bytes && size != SbVec2s(0,0)) {
      if (!PRIVATE(this)->glimagevalid) {
        PRIVATE(this)->testGrayscale(bytes, size, nc);
        if (PRIVATE(this)->isgrayscale) {
          if (!PRIVATE(this)->didconvert) {
            SoHeightMapToNormalMap::convert(bytes, size, nc, PRIVATE(this)->convertedheightmap);
            PRIVATE(this)->didconvert = TRUE;
          }
          bytes = PRIVATE(this)->convertedheightmap.getValue(size, nc);
        }

        PRIVATE(this)->glimage->setData(bytes, size, nc,
                                        bumpmap_translateWrap((Wrap)this->wrapS.getValue()),
                                        bumpmap_translateWrap((Wrap)this->wrapT.getValue()),
                                        1.0f); // max quality for bumpmaps
        PRIVATE(this)->glimagevalid = TRUE;
      }
      SoBumpMapElement::set(state, this, PRIVATE(this)->glimage);
      SoShapeStyleElement::setBumpmapEnabled(state, TRUE);
    }
    else {
      SoBumpMapElement::set(state, this, NULL);
      SoShapeStyleElement::setBumpmapEnabled(state, FALSE);
    }
  }
  else {
    static int didwarn = 0;
    if (!didwarn) {
      // FIXME: add link to bumpmapping doc on doc.coin3d.org. pederb, 2003-11-18
      SoDebugError::postWarning("SoBumpMap::GLRender",
                                "Your OpenGL driver does not support the "
                                "required extensions to do bumpmapping.");
      didwarn = 1;
    }
  }
}

// Documented in superclass.
void
SoBumpMap::doAction(SoAction * action)
{
  SoState * state = action->getState();

  int nc;
  SbVec2s size;
  const unsigned char * bytes = this->image.getValue(size, nc);

  if (bytes && size != SbVec2s(0,0)) {
    SoShapeStyleElement::setBumpmapEnabled(state, TRUE);
  }
  else {
    SoShapeStyleElement::setBumpmapEnabled(state, FALSE);
  }
}

// doc from parent
void
SoBumpMap::callback(SoCallbackAction * COIN_UNUSED_ARG(action))
{
  // not supported for SoCallbackAction yet
  // SoBumpMap::doAction(action);
}

// doc from parent
void
SoBumpMap::rayPick(SoRayPickAction * action)
{
  SoBumpMap::doAction(action);
}

// Documented in superclass. Overridden to detect when fields change.
void
SoBumpMap::notify(SoNotList * l)
{
  SoField * f = l->getLastField();
  if (f == &this->image) {
    // write image, not filename
    this->filename.setDefault(TRUE);
    this->image.setDefault(FALSE);
    PRIVATE(this)->didconvert = FALSE;
    PRIVATE(this)->isgrayscale = -1;
  }
  PRIVATE(this)->glimagevalid = FALSE;
  inherited::notify(l);
}

//
// Called from readInstance() or when user changes the
// filename field.
//
SbBool
SoBumpMap::loadFilename(void)
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
      PRIVATE(this)->didconvert = FALSE;
      PRIVATE(this)->isgrayscale = -1;
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
SoBumpMap::filenameSensorCB(void * data, SoSensor *)
{
  SoBumpMap * thisp = (SoBumpMap*) data;

  if (thisp->filename.getValue().getLength() &&
      !thisp->loadFilename()) {
    SoDebugError::postWarning("SoBumpMap::filenameSensorCB",
                              "Image file '%s' could not be read",
                              thisp->filename.getValue().getString());
  }
}

#undef PRIVATE
