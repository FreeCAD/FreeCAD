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
  \class SoGLVertexAttributeElement Inventor/elements/SoGLVertexAttributeElement.h
  \brief The SoGLVertexAttributeElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoGLVertexAttributeElement.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoMFVec2f.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFVec4f.h>
#include <Inventor/fields/SoMFShort.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/errors/SoDebugError.h>
#include "elements/SoVertexAttributeData.h"
#include "shaders/SoGLShaderProgram.h"
#include "misc/SbHash.h"
#include "rendering/SoVBO.h"
#include "glue/glp.h"
#include "rendering/SoGL.h"

SO_ELEMENT_SOURCE(SoGLVertexAttributeElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLVertexAttributeElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLVertexAttributeElement, inherited);
}

/*!
  Internal Coin method.
*/
void
SoGLVertexAttributeElement::init(SoState * state)
{
  inherited::init(state);
}

/*!
  Destructor.
*/
SoGLVertexAttributeElement::~SoGLVertexAttributeElement()
{

}

/*!
  Returns a read-only element instance.
*/
const SoGLVertexAttributeElement *
SoGLVertexAttributeElement::getInstance(SoState * const state)
{
  return (const SoGLVertexAttributeElement *)
    (getConstElement(state, classStackIndex));
}

typedef const char * Key;
typedef SoVertexAttributeData * Type;

//
// send attributes to gl
//
static void send_attribs(const Key & COIN_UNUSED_ARG(key),
                         const Type & attribdata,
                         void * closure)
{
  SoVertexAttributeData * data = (SoVertexAttributeData *) attribdata;
  assert(data);

  // only send attributes that were found in the shader object
  if (data->index < 0) {
    return;
  }

  const int dataindex = *((const int *) closure);
  const cc_glglue * glue = sogl_glue_instance(data->state);

#if COIN_DEBUG
  if (dataindex >= data->data->getNum()) {
    static SbBool first = TRUE;
    if (first) {
      SoDebugError::post("SoGLVertexAttributeElement::send",
                         "attribute index out of bounds.");
      first = FALSE;
    }
  }
#endif

  if (data->type == SoMFFloat::getClassTypeId()) {
    SoMFFloat * mfield = static_cast<SoMFFloat *>(data->data);
    const float * attribs = mfield->getValues(0);
    glue->glVertexAttrib1fARB(data->index, attribs[dataindex]);

  } else if (data->type == SoMFVec2f::getClassTypeId()) {

    SoMFVec2f * mfield = static_cast<SoMFVec2f *>(data->data);
    const SbVec2f * attribs = mfield->getValues(0);
    glue->glVertexAttrib2fvARB(data->index, attribs[dataindex].getValue());

  } else if (data->type == SoMFVec3f::getClassTypeId()) {

    SoMFVec3f * mfield = static_cast<SoMFVec3f *>(data->data);
    const SbVec3f * attribs = mfield->getValues(0);
    glue->glVertexAttrib3fvARB(data->index, attribs[dataindex].getValue());

  } else if (data->type == SoMFVec4f::getClassTypeId()) {

    SoMFVec4f * mfield = static_cast<SoMFVec4f *>(data->data);
    const SbVec4f * attribs = mfield->getValues(0);
    glue->glVertexAttrib4fvARB(data->index, attribs[dataindex].getValue());

  } else if (data->type == SoMFShort::getClassTypeId()) {

    SoMFShort * mfield = static_cast<SoMFShort *>(data->data);
    const short * attribs = mfield->getValues(0);
    glue->glVertexAttrib1sARB(data->index, attribs[dataindex]);

  } else {
    assert(0 && "unknown attribute type");
  }
}

//
// get the index of each specified attribute.
//
static void query_attribs(const Key & key,
                          const Type & attribdata,
                          void * COIN_UNUSED_ARG(closure))
{
  SoVertexAttributeData * data = (SoVertexAttributeData *) attribdata;
  const cc_glglue * glue = sogl_glue_instance(data->state);

  SoGLShaderProgram * shaderprogram =
    static_cast<SoGLShaderProgram *>(SoGLShaderProgramElement::get(data->state));

  if (shaderprogram && shaderprogram->glslShaderProgramLinked()) {
    uint32_t shaderobj = shaderprogram->getGLSLShaderProgramHandle(data->state);

    data->index = glue->glGetAttribLocationARB((COIN_GLhandle) shaderobj,
                                               (COIN_GLchar *) key);
#if COIN_DEBUG
    if (data->index < 0) {
      SoDebugError::postWarning("SoGLVertexAttributeElement::addElt",
                                "vertex attribute '%s' not used in vertex shader", key);
    }
#endif // COIN_DEBUG
  }
}

//
// enable vertex array rendering, with or without vbo
//
static void enable_vbo(const Key & COIN_UNUSED_ARG(key),
                       const Type & attribdata,
                       void * closure)
{
  SoVertexAttributeData * data = (SoVertexAttributeData *) attribdata;

  // only enable vertex array rendering for attributes that were
  // actually used in the shader object
  if (data->index < 0) { return; }

  SoGLRenderAction * action = static_cast<SoGLRenderAction *>(closure);
  const cc_glglue * glue = sogl_glue_instance(action->getState());

  const SoCoordinateElement * coords =
    SoCoordinateElement::getInstance(action->getState());
  assert(coords->getNum() == attribdata->data->getNum());

  const void * dataptr = NULL;

  if (data->vbo) {
    data->vbo->bindBuffer(action->getCacheContext());
  } else {
    cc_glglue_glBindBuffer(glue, GL_ARRAY_BUFFER, 0);
    dataptr = attribdata->dataptr;
  }

  glue->glVertexAttribPointerARB(data->index,
                                 attribdata->num,
                                 attribdata->gltype,
                                 GL_FALSE, 0, dataptr);

  glue->glEnableVertexAttribArrayARB(data->index);
}

//
// disable vertex array rendering, with or without vbo
//
static void disable_vbo(const Key & COIN_UNUSED_ARG(key),
                        const Type & attribdata,
                        void * closure)
{
  SoVertexAttributeData * data = (SoVertexAttributeData *) attribdata;
  // only disable vertex array rendering for attributes that were
  // actually used in the shader object
  if (data->index < 0) { return; }

  SoGLRenderAction * action = static_cast<SoGLRenderAction *>(closure);
  const cc_glglue * glue = sogl_glue_instance(action->getState());

  glue->glDisableVertexAttribArrayARB(data->index);
}

//! FIXME: write doc.
void
SoGLVertexAttributeElement::send(const int index) const
{
  this->applyToAttributes(send_attribs, (void *) &index);
}

//! FIXME: write doc.
void
SoGLVertexAttributeElement::addElt(SoVertexAttributeData * attribdata)
{
  inherited::addElt(attribdata);
  this->applyToAttributes(query_attribs, NULL);
}

//! FIXME: write doc.
void
SoGLVertexAttributeElement::enableVBO(SoGLRenderAction * action) const
{
  this->applyToAttributes(enable_vbo, action);
}

//! FIXME: write doc.
void
SoGLVertexAttributeElement::disableVBO(SoGLRenderAction * action) const
{
  this->applyToAttributes(disable_vbo, action);
}
