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
  \class SoPackedColor SoPackedColor.h Inventor/nodes/SoPackedColor.h
  \brief The SoPackedColor class is a node for setting diffuse and transparency material values.

  \ingroup coin_nodes

  This node provides a convenient way of setting diffuse colors and
  transparency values with packed 32-bit RGBA vectors.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    PackedColor {
        orderedRGBA 0xccccccff
    }
  \endcode

  \sa SoBaseColor
*/

// *************************************************************************

// FIXME: the name of the "orderedRGBA" field was actually "rgba" in
// the version 2.0 Inventor file format, and the semantics was also
// different (reversed order). This means that Inventor 2.0 files with
// PackedColor nodes will not import properly at the moment.  20020508 mortene.

// *************************************************************************

#include <Inventor/nodes/SoPackedColor.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/C/tidbits.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"


// *************************************************************************

/*!
  \var SoMFUInt32 SoPackedColor::orderedRGBA

  Set of packed 32-bit RGBA vectors.

  The most significant 24 bits specify 8 bits each for the red,
  green and blue components.

  The least significant 8 bits specify the transparency value, where
  0x00 means completely transparent, and 0xff completely opaque.
*/

// *************************************************************************

class SoPackedColorP {
 public:
  SoPackedColorP() : vbo(NULL) { }
  ~SoPackedColorP() { delete this->vbo; }
  SbBool transparent;
  SbBool checktransparent;
  SoVBO * vbo;
};

#define PRIVATE(obj) obj->pimpl

SO_NODE_SOURCE(SoPackedColor);

/*!
  Constructor.
*/
SoPackedColor::SoPackedColor()
{
  PRIVATE(this) = new SoPackedColorP;
  SO_NODE_INTERNAL_CONSTRUCTOR(SoPackedColor);

  SO_NODE_ADD_FIELD(orderedRGBA, (0xccccccff));

  PRIVATE(this)->checktransparent = FALSE;
  PRIVATE(this)->transparent = FALSE;
}

/*!
  Destructor.
*/
SoPackedColor::~SoPackedColor()
{
  delete PRIVATE(this);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoPackedColor::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPackedColor, SO_FROM_INVENTOR_2_1);

  SO_ENABLE(SoCallbackAction, SoLazyElement);
  SO_ENABLE(SoGLRenderAction, SoGLLazyElement);
}

// Doc from superclass.
void
SoPackedColor::GLRender(SoGLRenderAction * action)
{
  SoPackedColor::doAction(action);
}

// Doc from superclass.
void
SoPackedColor::doAction(SoAction * action)
{
  (void) this->isTransparent(); // update cached value for transparent

  SoState * state = action->getState();
  const int num = this->orderedRGBA.getNum();
  if (!this->orderedRGBA.isIgnored() &&
      num > 0 &&
      !SoOverrideElement::getDiffuseColorOverride(state)) {
    SoLazyElement::setPacked(state, this,
                             num,
                             this->orderedRGBA.getValues(0),
                             PRIVATE(this)->transparent);

    if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
      SoBase::staticDataLock();
      SbBool setvbo = FALSE;
      if (SoGLVBOElement::shouldCreateVBO(state, num)) {
        SbBool dirty = FALSE;
        setvbo = TRUE;
        if (PRIVATE(this)->vbo == NULL) {
          PRIVATE(this)->vbo = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
          dirty = TRUE;
        }
        else if (PRIVATE(this)->vbo->getBufferDataId() != this->getNodeId()) {
          dirty = TRUE;
        }
        if (dirty) {
          if (coin_host_get_endianness() == COIN_HOST_IS_BIGENDIAN) {
            PRIVATE(this)->vbo->setBufferData(this->orderedRGBA.getValues(0),
                                              num*sizeof(uint32_t),
                                              this->getNodeId());
          }
          else {
            // sigh. Need to swap bytes on little endian systems.
            const uint32_t * src = this->orderedRGBA.getValues(0);
            uint32_t * dst = (uint32_t*) 
              PRIVATE(this)->vbo->allocBufferData(num*sizeof(uint32_t), 
                                                  this->getNodeId());  
            for (int i = 0; i < num; i++) {
              uint32_t tmp = src[i];
              dst[i] = 
                (tmp << 24) |
                ((tmp & 0xff00) << 8) |
                ((tmp & 0xff0000) >> 8) |
                (tmp >> 24);
            }
          }
        }
      }
      else if (PRIVATE(this)->vbo) {
        PRIVATE(this)->vbo->setBufferData(NULL, 0, 0);
      }
      SoBase::staticDataUnlock();
      if (setvbo) {
        SoGLVBOElement::setColorVBO(state, PRIVATE(this)->vbo);
      }    
    }
    if (this->isOverride()) {
      SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
    }
  }
}

// Doc from superclass.
void
SoPackedColor::callback(SoCallbackAction * action)
{
  SoPackedColor::doAction(action);
}

/*!
  Returns \c TRUE if there is at least one RGBA vector in the set
  which is not completely opaque.
 */
SbBool
SoPackedColor::isTransparent(void)
{
  if (PRIVATE(this)->checktransparent) {
    PRIVATE(this)->checktransparent = FALSE;
    PRIVATE(this)->transparent = FALSE;
    int n = this->orderedRGBA.getNum();
    for (int i = 0; i < n; i++) {
      if ((this->orderedRGBA[i] & 0xff) != 0xff) {
        PRIVATE(this)->transparent = TRUE;
        break;
      }
    }
  }
  return PRIVATE(this)->transparent;
}

// Documented in superclass.
void
SoPackedColor::notify(SoNotList *list)
{
  // Overridden to check for transparency when orderedRGBA changes.

  SoField *f = list->getLastField();
  if (f == &this->orderedRGBA) {
    PRIVATE(this)->checktransparent = TRUE;
  }
  inherited::notify(list);
}

#undef PRIVATE
