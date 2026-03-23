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

#include "SoGLShaderProgram.h"

#include <cassert>

#include "SoGLShaderObject.h"
#include "SoGLARBShaderProgram.h"
#include "SoGLCgShaderProgram.h"
#include "SoGLSLShaderProgram.h"
#include <Inventor/elements/SoGLCacheContextElement.h>

// *************************************************************************

SoGLShaderProgram::SoGLShaderProgram(void)
{
  this->arbShaderProgram = new SoGLARBShaderProgram;
  this->cgShaderProgram = new SoGLCgShaderProgram;
  this->glslShaderProgram = new SoGLSLShaderProgram;

  this->isenabled = FALSE;
  this->enablecb = NULL;
  this->enablecbclosure = NULL;
}

SoGLShaderProgram::~SoGLShaderProgram()
{
  delete this->arbShaderProgram;
  delete this->cgShaderProgram;
  delete this->glslShaderProgram;
}

void
SoGLShaderProgram::addShaderObject(SoGLShaderObject *shader)
{
  this->objectids.append(shader->getShaderObjectId());
  switch (shader->shaderType()) {
  case SoShader::ARB_SHADER:
    this->arbShaderProgram->addShaderObject((SoGLARBShaderObject*)shader);
    break;
  case SoShader::CG_SHADER:
    this->cgShaderProgram->addShaderObject((SoGLCgShaderObject*)shader);
    break;
  case SoShader::GLSL_SHADER:
    this->glslShaderProgram->addShaderObject((SoGLSLShaderObject*)shader);
    break;
  default:
    assert(FALSE && "shaderType unknown!");
  }
}

void
SoGLShaderProgram::removeShaderObjects(void)
{
  this->arbShaderProgram->removeShaderObjects();
  this->cgShaderProgram->removeShaderObjects();
  this->glslShaderProgram->removeShaderObjects();
  this->glslShaderProgram->removeProgramParameters();
  this->objectids.truncate(0);
}

void
SoGLShaderProgram::enable(SoState * state)
{
  const uint32_t cachecontext = SoGLCacheContextElement::get(state);
  const cc_glglue * glctx = cc_glglue_instance(cachecontext);

  this->arbShaderProgram->enable();
  this->cgShaderProgram->enable();
  this->glslShaderProgram->enable(glctx);

  this->isenabled = TRUE;
  if (this->enablecb) {
    this->enablecb(this->enablecbclosure, state, TRUE);
  }
}

void
SoGLShaderProgram::disable(SoState * state)
{
  const uint32_t cachecontext = SoGLCacheContextElement::get(state);
  const cc_glglue * glctx = cc_glglue_instance(cachecontext);

  this->arbShaderProgram->disable();
  this->cgShaderProgram->disable();
  this->glslShaderProgram->disable(glctx);

  this->isenabled = FALSE;
  if (this->enablecb) {
    this->enablecb(this->enablecbclosure, state, FALSE);
  }
}

SbBool
SoGLShaderProgram::isEnabled(void) const
{
  return this->isenabled;
}

void
SoGLShaderProgram::setEnableCallback(SoShaderProgramEnableCB * cb,
                                     void * closure)
{
  this->enablecb = cb;
  this->enablecbclosure = closure;
}

void
SoGLShaderProgram::updateCoinParameter(SoState * state, const SbName & name, const int value)
{
  if (this->glslShaderProgram) {
    SbBool enabled = this->isenabled;
    if (!enabled) this->enable(state);
    this->glslShaderProgram->updateCoinParameter(state, name, value);
    if (!enabled) this->disable(state);
  }
}

void
SoGLShaderProgram::addProgramParameter(int name, int value)
{
  if (this->glslShaderProgram) {
    this->glslShaderProgram->addProgramParameter(name, value);
  }
}

void
SoGLShaderProgram::getShaderObjectIds(SbList <uint32_t> & ids) const
{
  ids = this->objectids;
}

uint32_t
SoGLShaderProgram::getGLSLShaderProgramHandle(SoState * state) const
{
  const uint32_t cachecontext = SoGLCacheContextElement::get(state);
  const cc_glglue * glctx = cc_glglue_instance(cachecontext);

  return this->glslShaderProgram->getProgramHandle(glctx);
}

SbBool
SoGLShaderProgram::glslShaderProgramLinked(void) const
{
  if (this->glslShaderProgram) {
    return this->glslShaderProgram->neededLinking();
  }
  return FALSE;
}

#if defined(SOURCE_HINT)
SbString
SoGLShaderProgram::getSourceHint(void)
{
  SbString result;

  result += this->arbShaderProgram->getSourceHint();
  result += this->cgShaderProgram->getSourceHint();
  result += this->glslShaderProgram->getSourceHint();
  return result;
}
#endif
