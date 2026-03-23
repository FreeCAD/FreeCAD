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
  \class SoMaterial SoMaterial.h Inventor/nodes/SoMaterial.h
  \brief The SoMaterial class is a node type for setting up material values for scene geometry.

  \ingroup coin_nodes

  After traversing an SoMaterial node, subsequent shape nodes with
  geometry in the scene graph will use values from the material "pool"
  of the traversal state set up from nodes of this type.

  For detailed information on the various components, see the OpenGL
  color model, presented in the chapter "Colors and Coloring" (chapter
  2.13 in the OpenGL 1.4 specification).

  Note that values from a material node will \e replace the previous
  values from the traversal state, they will \e not accumulate. That's
  the case even when e.g. material changes are \e implicit in an
  iv-file, as illustrated by the following example:
  
  Also note that support for multiple values in ambientColor,
  emissiveColor, specularColor and shininess was obsoleted in Open
  Inventor 2.1. The reason for this design change was performance
  driven, since it is relatively slow to change the OpenGL material
  properties. Changing the diffuse color value is fast though, so it is
  still possible to have multiple diffuseColor and transparency
  values.

  \verbatim
  #Inventor V2.1 ascii

  Material { ambientColor 1 0 0 }
  Cone { }

  Translation { translation 5 0 0 }

  Material { }
  Sphere { }
  \endverbatim

  (The SoSphere will not "inherit" the SoMaterial::ambientColor from
  the first SoMaterial node, even though it is not explicitly set in
  the second material node. The default value of
  SoMaterial::ambientColor will be used.)

  Note that nodes imported as part of a VRML V1.0 file has a special
  case, where the fields SoMaterial::ambientColor,
  SoMaterial::diffuseColor and SoMaterial::specularColor contains zero
  values, and SoMaterial::emissiveColor contains one or more
  values. The values in SoMaterial::emissiveColor should then be
  treated as precalculated lighting, and the other fields should be
  ignored.

  You can detect this case by checking the values of the material
  elements when the scene graph is traversed using an
  SoCallbackAction. SoDiffuseColorElement, SoAmbientColorElement, and
  SoSpecularColorElement will contain one value with a completely
  black color (0.0f, 0.0f, 0.0f), SoShininessElement will contain one
  value of 0.0f, and SoEmissiveColorElement will contain one or more
  values. It is done like this to make rendering work correctly on
  systems that do not test for this specific case.

  You should only check for this case when you're traversing a VRML
  V1.0 file scene graph, of course. See SoNode::getNodeType() for
  information about how nodes can be tested for whether or not they
  have been imported or otherwise set up as of VRML1 type versus
  Inventor type.

  When the scene graph is rendered using an SoGLRenderAction, the
  elements will be set differently to optimize rendering.  The
  SoDiffuseColorElement will be set to the values in
  SoMaterial::emissiveColor, and the light model will be set to
  SoLightModel::BASE_COLOR.

  The SoMaterial::transparency values will always be treated normally.

  Here is a very simple usage example:

  \verbatim
  #Inventor V2.1 ascii

  Separator {
     Coordinate3 {
        point [ 0 0 0, 1 0 0, 1 1 0 ]
     }

     Material {
        diffuseColor [ 1 0 0, 1 1 0, 0 0 1 ]
     }

     MaterialBinding {
        value PER_VERTEX
     }

     IndexedFaceSet {
        coordIndex [ 0, 1, 2, -1 ]
     }
  }
  \endverbatim

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Material {
        ambientColor 0.2 0.2 0.2
        diffuseColor 0.8 0.8 0.8
        specularColor 0 0 0
        emissiveColor 0 0 0
        shininess 0.2
        transparency 0
    }
  \endcode

  \sa SoMaterialBinding, SoBaseColor, SoPackedColor
*/

// *************************************************************************

// FIXME: should also describe what happens if the number of values in
// the fields are not consistent. 20020119 mortene.

// *************************************************************************

#include <Inventor/nodes/SoMaterial.h>

#include <cstdlib>

#include <Inventor/C/tidbits.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoAmbientColorElement.h>
#include <Inventor/elements/SoDiffuseColorElement.h>
#include <Inventor/elements/SoSpecularColorElement.h>
#include <Inventor/elements/SoEmissiveColorElement.h>
#include <Inventor/elements/SoShininessElement.h>
#include <Inventor/elements/SoTransparencyElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/errors/SoDebugError.h>

#include <Inventor/annex/Profiler/SoProfiler.h>
#include <Inventor/annex/Profiler/elements/SoProfilerElement.h>
#include <Inventor/annex/Profiler/SbProfilingData.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbStorage.h>
#endif // COIN_THREADSAFE

#include "rendering/SoVBO.h"
#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoMFColor SoMaterial::ambientColor

  Ambient material part color values. Will by default contain a single
  color value of [0.2, 0.2, 0.2] (i.e. dark gray).

  The ambient part of the material is not influenced by any
  light sources, and should be thought of conceptually as the constant,
  but small contribution of light to a scene "seeping in" from
  everywhere.

  (Think of the ambient contribution in the context that there's
  always photons fizzing around everywhere -- even in a black room without
  light sources, for instance).

  Only the first value in this field will be used. All other values
  will be ignored.

  \sa SoEnvironment::ambientIntensity
*/
/*!
  \var SoMFColor SoMaterial::diffuseColor

  Diffuse material part color values. This field is by default
  initialized to contain a single color value of [0.8, 0.8, 0.8]
  (light gray).

  The diffuse part is combined with the light emitted from the scene's
  light sources.

  Traditional Open Inventor uses the same override bit for both
  diffuse color and transparency.  To get around this problem if you
  need to override one without the other, set the environment
  variable "COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE".  This is
  a Coin extension, and will not work on other Open Inventor
  implementations.
*/
/*!
  \var SoMFColor SoMaterial::specularColor

  Specular material part color values. Defaults to a single color
  value of [0, 0, 0] (black).

  Only the first value in this field will be used. All other values
  will be ignored.  
*/

/*!
  \var SoMFColor SoMaterial::emissiveColor

  The color of the light "emitted" by the subsequent geometry,
  independent of lighting / shading.

  Defaults to contain a single color value of [0, 0, 0] (black, i.e. no
  contribution).

  Only the first value in this field will be used. All other values will be ignored.
*/

/*!
  \var SoMFFloat SoMaterial::shininess

  Shininess values. Decides how the light from light sources are
  distributed across the geometry surfaces. Valid range is from 0.0
  (which gives a dim appearance), to 1.0 (glossy-looking surfaces).

  Defaults to contain a single value of 0.2.

  Only the first value in this field will be used. All other values
  will be ignored.
*/

/*!
  \var SoMFFloat SoMaterial::transparency

  Transparency values. Valid range is from 0.0 (completely opaque,
  which is the default) to 1.0 (completely transparent,
  i.e. invisible).

  Defaults to contain a single value of 0.0.

  Traditional Open Inventor uses the same override bit for both
  transparency and diffuse color.  To get around this problem if you
  need to override one without the other, set the environment
  variable "COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE".  This is
  a Coin extension, and  will not work on other Open Inventor
  implementations.
*/

// defines for materialtype
#define TYPE_UNKNOWN            0
#define TYPE_NORMAL             1
#define TYPE_VRML1_ONLYEMISSIVE 2 // special case in vrml1

// *************************************************************************

#ifndef DOXYGEN_SKIP_THIS

class SoMaterialP {
public:
  SoMaterialP() :
#ifdef COIN_THREADSAFE
    colorpacker_storage(sizeof(void*), alloc_colorpacker, free_colorpacker),
#endif // COIN_THREADSAFE
    vbo(NULL) { }
  ~SoMaterialP() { delete this->vbo; }

  int materialtype;
  int transparencyflag;

#ifdef COIN_THREADSAFE
  SbStorage colorpacker_storage;
#else // COIN_THREADSAFE
  SoColorPacker single_colorpacker;
#endif // COIN_THREADSAFE

  SoColorPacker * getColorPacker(void) {
#ifdef COIN_THREADSAFE
    SoColorPacker ** cptr = (SoColorPacker**) this->colorpacker_storage.get();
    return * cptr;
#else // COIN_THREADSAFE
    return &this->single_colorpacker;
#endif // COIN_THREADSAFE
  }

  SoVBO * vbo;

#ifdef COIN_THREADSAFE
private:
  static void alloc_colorpacker(void * data) {
    SoColorPacker ** cptr = (SoColorPacker**) data;
    *cptr = new SoColorPacker;
  }
  static void free_colorpacker(void * data) {
    SoColorPacker ** cptr = (SoColorPacker**) data;
    delete *cptr;
  }
#endif // COIN_THREADSAFE
};

#endif // DOXYGEN_SKIP_THIS

#define PRIVATE(obj) ((obj)->pimpl)

SO_NODE_SOURCE(SoMaterial);

/*!
  Constructor.
*/
SoMaterial::SoMaterial(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoMaterial);

  SO_NODE_ADD_FIELD(ambientColor, (0.2f, 0.2f, 0.2f));
  SO_NODE_ADD_FIELD(diffuseColor, (0.8f, 0.8f, 0.8f));
  SO_NODE_ADD_FIELD(specularColor, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(emissiveColor, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(shininess, (0.2f));
  SO_NODE_ADD_FIELD(transparency, (0.0f));

  PRIVATE(this)->materialtype = TYPE_NORMAL;
  PRIVATE(this)->transparencyflag = FALSE; // we know it's not transparent
}

/*!
  Destructor.
*/
SoMaterial::~SoMaterial()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoMaterial::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoMaterial, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGLRenderAction, SoGLLazyElement);
  SO_ENABLE(SoCallbackAction, SoLazyElement);

  SO_ENABLE(SoCallbackAction, SoAmbientColorElement);
  SO_ENABLE(SoCallbackAction, SoDiffuseColorElement);
  SO_ENABLE(SoCallbackAction, SoEmissiveColorElement);
  SO_ENABLE(SoCallbackAction, SoSpecularColorElement);
  SO_ENABLE(SoCallbackAction, SoShininessElement);
  SO_ENABLE(SoCallbackAction, SoTransparencyElement);

  SO_ENABLE(SoGLRenderAction, SoAmbientColorElement);
  SO_ENABLE(SoGLRenderAction, SoDiffuseColorElement);
  SO_ENABLE(SoGLRenderAction, SoEmissiveColorElement);
  SO_ENABLE(SoGLRenderAction, SoSpecularColorElement);
  SO_ENABLE(SoGLRenderAction, SoShininessElement);
  SO_ENABLE(SoGLRenderAction, SoTransparencyElement);
}

// Doc from superclass.
void
SoMaterial::GLRender(SoGLRenderAction * action)
{
  SoMaterial::doAction(action);
}

// Doc from superclass.
void
SoMaterial::doAction(SoAction * action)
{
  SbBool istransparent = FALSE;

  SoState * state = action->getState();

  if (SoProfiler::isEnabled()) {
    // register the SoColorPacker memory usage
    if (state->isElementEnabled(SoProfilerElement::getClassStackIndex())) {
      const SoColorPacker * packer = PRIVATE(this)->getColorPacker();
      if (packer) {
        SoProfilerElement * profilerelt = SoProfilerElement::get(state);
        assert(profilerelt);
        SbProfilingData & data = profilerelt->getProfilingData();
        int entry = data.getIndex(action->getCurPath(), TRUE);
        assert(entry != -1);
        size_t mem = data.getNodeFootprint(entry, SbProfilingData::MEMORY_SIZE);
        data.setNodeFootprint(entry, SbProfilingData::MEMORY_SIZE,
                              mem + packer->getSize() * sizeof(uint32_t));
      }
    }
  }

  uint32_t bitmask = 0;
  uint32_t flags = SoOverrideElement::getFlags(state);
#define TEST_OVERRIDE(bit) ((SoOverrideElement::bit & flags) != 0)

  if (!this->ambientColor.isIgnored() && this->ambientColor.getNum() &&
      !TEST_OVERRIDE(AMBIENT_COLOR)) {
    bitmask |= SoLazyElement::AMBIENT_MASK;
    if (this->isOverride()) {
      SoOverrideElement::setAmbientColorOverride(state, this, TRUE);
    }
  }
  if (!this->diffuseColor.isIgnored() && this->diffuseColor.getNum() &&
      !TEST_OVERRIDE(DIFFUSE_COLOR)) {
    // Note: the override flag bit values for diffuseColor and
    // transparency are equal (done like that to match SGI/TGS
    // Inventor behavior), so overriding one will also override the
    // other.
    bitmask |= SoLazyElement::DIFFUSE_MASK;
    if (this->isOverride()) {
      SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
    }
  }
  if (!this->emissiveColor.isIgnored() && this->emissiveColor.getNum() &&
      !TEST_OVERRIDE(EMISSIVE_COLOR)) {
    bitmask |= SoLazyElement::EMISSIVE_MASK;
    if (this->isOverride()) {
      SoOverrideElement::setEmissiveColorOverride(state, this, TRUE);
    }
  }
  if (!this->specularColor.isIgnored() && this->specularColor.getNum() &&
      !TEST_OVERRIDE(SPECULAR_COLOR)) {
    bitmask |= SoLazyElement::SPECULAR_MASK;
    if (this->isOverride()) {
      SoOverrideElement::setSpecularColorOverride(state, this, TRUE);
    }
  }
  if (!this->shininess.isIgnored() && this->shininess.getNum() &&
      !TEST_OVERRIDE(SHININESS)) {
    bitmask |= SoLazyElement::SHININESS_MASK;
    if (this->isOverride()) {
      SoOverrideElement::setShininessOverride(state, this, TRUE);
    }
  }
  if (!this->transparency.isIgnored() && this->transparency.getNum() &&
      !TEST_OVERRIDE(TRANSPARENCY)) {
    // Note: the override flag bit values for diffuseColor and
    // transparency are equal (done like that to match SGI/TGS
    // Inventor behavior), so overriding one will also override the
    // other.
    bitmask |= SoLazyElement::TRANSPARENCY_MASK;
    if (this->isOverride()) {
      SoOverrideElement::setTransparencyOverride(state, this, TRUE);
    }
    // if we don't know if material is transparent, run through all
    // values and test
    if (PRIVATE(this)->transparencyflag < 0) {
      int i, n = this->transparency.getNum();
      const float * p = this->transparency.getValues(0);
      for (i = 0; i < n; i++) {
        if (p[i] > 0.0f) {
          istransparent = TRUE;
          break;
        }
      }
      // we now know whether material is transparent or not
      PRIVATE(this)->transparencyflag = (int) istransparent;
    }
    istransparent = (SbBool) PRIVATE(this)->transparencyflag;
  }
#undef TEST_OVERRIDE

  if (bitmask) {
    SbColor dummycolor(0.8f, 0.8f, 0.0f);
    float dummyval = 0.2f;
    const SbColor * diffuseptr = this->diffuseColor.getValues(0);
    int numdiffuse = this->diffuseColor.getNum();

    if (this->getMaterialType() == TYPE_VRML1_ONLYEMISSIVE) {
      bitmask |= SoLazyElement::DIFFUSE_MASK;
      bitmask &= ~SoLazyElement::EMISSIVE_MASK;
      diffuseptr = this->emissiveColor.getValues(0);
      numdiffuse = this->emissiveColor.getNum();
      // if only emissive color, turn off lighting and render as diffuse.
      // this is much faster
      SoLightModelElement::set(state, this, SoLightModelElement::BASE_COLOR);
    }
    else if (this->getNodeType() == SoNode::VRML1) {
      SoLightModelElement::set(state, this, SoLightModelElement::PHONG);
    }

#if COIN_DEBUG
    if (bitmask & SoLazyElement::SHININESS_MASK) {
      static int didwarn = 0;
      if (!didwarn && (this->shininess[0] < 0.0f || this->shininess[0] > 1.0f)) {
        SoDebugError::postWarning("SoMaterial::GLRender",
                                  "Shininess out of range [0-1]. "
                                  "The shininess value will be clamped."
                                  "This warning will be printed only once, but there might be more errors. "
                                  "You should check and fix your code and/or Inventor exporter.");

        didwarn = 1;
      }
    }
#endif // COIN_DEBUG

    const int numtransp = this->transparency.getNum();
    SoLazyElement::setMaterials(state, this, bitmask,
                                PRIVATE(this)->getColorPacker(),
                                diffuseptr, numdiffuse,
                                this->transparency.getValues(0), numtransp,
                                bitmask & SoLazyElement::AMBIENT_MASK ?
                                this->ambientColor[0] : dummycolor,
                                bitmask & SoLazyElement::EMISSIVE_MASK ?
                                this->emissiveColor[0] : dummycolor,
                                bitmask & SoLazyElement::SPECULAR_MASK ?
                                this->specularColor[0] : dummycolor,
                                bitmask & SoLazyElement::SHININESS_MASK ?
                                SbClamp(this->shininess[0], 0.0f, 1.0f) : dummyval,
                                istransparent);
    if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
      SoBase::staticDataLock();
      SbBool setvbo = FALSE;
      if (SoGLVBOElement::shouldCreateVBO(state, numdiffuse)) {
        setvbo = TRUE;
        if (PRIVATE(this)->vbo == NULL) {
          PRIVATE(this)->vbo = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
        }
      }
      else if (PRIVATE(this)->vbo) {
        PRIVATE(this)->vbo->setBufferData(NULL, 0, 0);
      }
      // don't fill in any data in the VBO. Data will be filled in
      // using the ColorPacker right before the VBO is used
      SoBase::staticDataUnlock();
      if (setvbo) {
        SoGLVBOElement::setColorVBO(state, PRIVATE(this)->vbo);
      }
    }
  }
}

// Doc from superclass.
void
SoMaterial::callback(SoCallbackAction * action)
{
  SoMaterial::doAction(action);
}

void
SoMaterial::notify(SoNotList *list)
{
  SoField * f = list->getLastField();
  if (f) PRIVATE(this)->materialtype = TYPE_UNKNOWN;
  if (f == &this->transparency) {
    PRIVATE(this)->transparencyflag = -1; // unknown
  }
  inherited::notify(list);
}

//
// to test for special vrml1 case. It's not used right now,
// but it might be enabled again later. pederb, 2002-09-11
//
int
SoMaterial::getMaterialType(void)
{
  if (this->getNodeType() != SoNode::VRML1) return TYPE_NORMAL;
  else {
    if (PRIVATE(this)->materialtype == TYPE_UNKNOWN) {
      if (!this->diffuseColor.isIgnored() && this->diffuseColor.getNum() == 0 &&
          !this->ambientColor.isIgnored() && this->ambientColor.getNum() == 0 &&
          !this->specularColor.isIgnored() && this->specularColor.getNum() == 0 &&
          !this->emissiveColor.isIgnored() && this->emissiveColor.getNum()) {
        PRIVATE(this)->materialtype = TYPE_VRML1_ONLYEMISSIVE;
      }
      else if (this->emissiveColor.getNum() > this->diffuseColor.getNum()) {
        PRIVATE(this)->materialtype = TYPE_VRML1_ONLYEMISSIVE;
      }
      else {
        PRIVATE(this)->materialtype = TYPE_NORMAL;
      }
    }
    return PRIVATE(this)->materialtype;
  }
}

#undef PRIVATE
#undef TYPE_UNKNOWN
#undef TYPE_NORMAL
#undef TYPE_VRML1_ONLYEMISSIVE
