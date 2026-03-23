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

#include "SoGLARBShaderParameter.h"
#include "SoGLARBShaderObject.h"
#include "coindefs.h"

#include <cstring>

#include <Inventor/errors/SoDebugError.h>

// *************************************************************************

SoGLARBShaderParameter::SoGLARBShaderParameter(void)
{
  this->target = 0;
  this->identifier = 0;
}

SoGLARBShaderParameter::~SoGLARBShaderParameter()
{
}

SoShader::Type
SoGLARBShaderParameter::shaderType(void) const
{
  return SoShader::ARB_SHADER;
}

void
SoGLARBShaderParameter::set1f(const SoGLShaderObject * shader,
                              const float value, const char*, const int idx)
{
  if (this->isValid(shader, idx))
    cc_glglue_glProgramLocalParameter4f(shader->GLContext(),
                                        this->target, this->identifier,
                                        value, value, value, value);
}

void
SoGLARBShaderParameter::set2f(const SoGLShaderObject * shader,
                              const float * value, const char*, const int idx)
{
  if (this->isValid(shader, idx))
    cc_glglue_glProgramLocalParameter4f(shader->GLContext(),
                                        this->target, this->identifier,
                                        value[0], value[1], value[0], value[0]);
}

void
SoGLARBShaderParameter::set3f(const SoGLShaderObject * shader,
                              const float * value, const char*, const int idx)
{
  if (this->isValid(shader, idx))
    cc_glglue_glProgramLocalParameter4f(shader->GLContext(),
                                        this->target, this->identifier,
                                        value[0], value[1], value[2], value[0]);
}

void
SoGLARBShaderParameter::set4f(const SoGLShaderObject * shader,
                              const float * value, const char*, const int idx)
{
  if (this->isValid(shader, idx))
    cc_glglue_glProgramLocalParameter4f(shader->GLContext(),
                                        this->target, this->identifier,
                                        value[0], value[1], value[2], value[3]);
}

void
SoGLARBShaderParameter::set1fv(const SoGLShaderObject*, const int,
                               const float*, const char*, const int)
{
  // FIXME: not implemented yet 20050127 martin
}

void
SoGLARBShaderParameter::set2fv(const SoGLShaderObject*, const int,
                               const float*, const char*, const int)
{
  // FIXME: not implemented yet 20050127 martin
}

void
SoGLARBShaderParameter::set3fv(const SoGLShaderObject*, const int,
                               const float*, const char*, const int)
{
  // FIXME: not implemented yet 20050127 martin
}

void
SoGLARBShaderParameter::set4fv(const SoGLShaderObject*, const int,
                               const float*, const char*, const int)
{
  // FIXME: not implemented yet 20050127 martin
}

void
SoGLARBShaderParameter::setMatrix(const SoGLShaderObject *,
                                  const float *, const char *, const int)
{
  // FIXME not implemented yet -- 20050128 martin
}


void
SoGLARBShaderParameter::setMatrixArray(const SoGLShaderObject *, const int,
                                       const float *, const char *, const int)
{
  // FIXME not implemented yet -- 20050128 martin
}

void
SoGLARBShaderParameter::set1i(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                              const int32_t COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                              const int)
{
  // FIXME not implemented yet -- 20050222 martin
}

void
SoGLARBShaderParameter::set2i(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                              const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                              const int)
{
  // FIXME not implemented yet -- 20050222 martin
}

void
SoGLARBShaderParameter::set3i(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                              const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                              const int)
{
  // FIXME not implemented yet -- 20050222 martin
}

void
SoGLARBShaderParameter::set4i(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                              const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                              const int)
{
  // FIXME not implemented yet -- 20050222 martin
}

void
SoGLARBShaderParameter::set1iv(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                               const int COIN_UNUSED_ARG(num),
                               const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                               const int)
{
  // probably not supported. pederb, 20070530
}

void
SoGLARBShaderParameter::set2iv(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                               const int COIN_UNUSED_ARG(num),
                               const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                               const int)
{
  // probably not supported. pederb, 20070530
}

void
SoGLARBShaderParameter::set3iv(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                               const int COIN_UNUSED_ARG(num),
                               const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                               const int)
{
  // probably not supported. pederb, 20070530
}

void
SoGLARBShaderParameter::set4iv(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                               const int COIN_UNUSED_ARG(num),
                               const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                               const int)
{
  // probably not supported. pederb, 20070530
}

//FIXME: no type checking implemented 20050128 martin
SbBool
SoGLARBShaderParameter::isValid(const SoGLShaderObject * shader, const int idx)
{
  assert(shader);
  assert(shader->shaderType() == SoShader::ARB_SHADER);

  this->target     = ((SoGLARBShaderObject*)shader)->target;
  this->identifier = idx;
  return TRUE;
}
