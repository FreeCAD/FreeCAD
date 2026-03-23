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

#include "SoGLShaderObject.h"
#include "coindefs.h"

#include <cassert>

#include "SoGLARBShaderObject.h"
#include "SoGLCgShaderObject.h"
#include "SoGLSLShaderObject.h"


static uint32_t shaderid = 0;

// *************************************************************************

SoGLShaderObject::SoGLShaderObject(const uint32_t cachecontext)
{
  this->isActiveFlag = TRUE;
  this->shadertype = VERTEX;
  this->paramsdirty = TRUE;
  this->glctx = cc_glglue_instance(cachecontext);
  this->cachecontext = cachecontext;
  this->id = ++shaderid;
}

const cc_glglue *
SoGLShaderObject::GLContext(void) const
{
  return this->glctx;
}

uint32_t
SoGLShaderObject::getCacheContext(void) const
{
  return this->cachecontext;
}

void
SoGLShaderObject::setShaderType(const ShaderType type)
{
  if (this->shadertype != type) {
    this->unload();
    this->shadertype = type;
  }
}

SoGLShaderObject::ShaderType
SoGLShaderObject::getShaderType(void) const
{
  return this->shadertype;
}

void SoGLShaderObject::setIsActive(SbBool flag)
{
  this->isActiveFlag = flag;
}

SbBool
SoGLShaderObject::isActive(void) const
{
  return (!this->isLoaded()) ? FALSE : this->isActiveFlag;
}

void
SoGLShaderObject::setParametersDirty(SbBool flag)
{
  this->paramsdirty = flag;
}

SbBool
SoGLShaderObject::getParametersDirty(void) const
{
  return this->paramsdirty;
}

void
SoGLShaderObject::updateCoinParameter(SoState * COIN_UNUSED_ARG(state), const SbName & COIN_UNUSED_ARG(name), SoShaderParameter * COIN_UNUSED_ARG(param), const int COIN_UNUSED_ARG(val))
{
}

uint32_t 
SoGLShaderObject::getShaderObjectId(void) const
{
  return this->id;
}
