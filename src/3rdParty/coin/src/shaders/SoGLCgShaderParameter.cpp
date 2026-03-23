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

#include "SoGLCgShaderParameter.h"
#include "SoGLCgShaderObject.h"
#include "coindefs.h"

#include <cstring>
#include <cstdio>
#include <cassert>

#include <Inventor/errors/SoDebugError.h>

// *************************************************************************

SoGLCgShaderParameter::SoGLCgShaderParameter(void)
{
  this->cgParameter = NULL;
  this->cacheType = CG_UNKNOWN_TYPE;
  this->cacheName = "";
  this->cacheSize = 0;
}

SoGLCgShaderParameter::~SoGLCgShaderParameter()
{
}

SoShader::Type
SoGLCgShaderParameter::shaderType(void) const
{
  return SoShader::CG_SHADER;
}

void
SoGLCgShaderParameter::setState(const SoGLShaderObject * shader,
                                CGGLenum matrix, CGGLenum transform,
                                const char* name)
{
  if (this->isValid(shader, name, CG_FLOAT4x4))
    glue_cgGLSetStateMatrixParameter(this->cgParameter, matrix, transform);
}

void
SoGLCgShaderParameter::set1f(const SoGLShaderObject * shader, const float v,
                             const char* name, const int)
{
  if (this->isValid(shader, name, CG_FLOAT))
    glue_cgGLSetParameter1f(this->cgParameter, v);
}

void
SoGLCgShaderParameter::set2f(const SoGLShaderObject * shader, const float * v,
                             const char* name, const int)
{
  if (this->isValid(shader, name, CG_FLOAT2))
    glue_cgGLSetParameter2f(this->cgParameter, v[0], v[1]);
}

void
SoGLCgShaderParameter::set3f(const SoGLShaderObject * shader, const float * v,
                             const char* name, const int)
{
  if (this->isValid(shader, name, CG_FLOAT3))
    glue_cgGLSetParameter3f(this->cgParameter, v[0], v[1], v[2]);
}

void
SoGLCgShaderParameter::set4f(const SoGLShaderObject * shader, const float * v,
                             const char* name, const int)
{
  if (this->isValid(shader, name, CG_FLOAT4))
    glue_cgGLSetParameter4f(this->cgParameter, v[0], v[1], v[2],v[3]);
}

void
SoGLCgShaderParameter::set1fv(const SoGLShaderObject * shader, const int num,
                              const float* v, const char * name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, CG_ARRAY, &cnt))
    glue_cgGLSetParameterArray1f(this->cgParameter, 0, cnt, v);
}

void
SoGLCgShaderParameter::set2fv(const SoGLShaderObject * shader, const int num,
                              const float *v, const char * name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, CG_ARRAY, &cnt))
    glue_cgGLSetParameterArray2f(this->cgParameter, 0, cnt, v);
}

void
SoGLCgShaderParameter::set3fv(const SoGLShaderObject * shader, const int num,
                              const float *v, const char * name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, CG_ARRAY, &cnt))
    glue_cgGLSetParameterArray3f(this->cgParameter, 0, cnt, v);
}

void
SoGLCgShaderParameter::set4fv(const SoGLShaderObject * shader, const int num,
                              const float *v, const char * name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, CG_ARRAY, &cnt))
    glue_cgGLSetParameterArray4f(this->cgParameter, 0, cnt, v);
}

void
SoGLCgShaderParameter::setMatrix(const SoGLShaderObject * shader,
                                 const float * value, const char * name,
                                 const int)
{
  // FIXME: Support of other matrices, e.g. float3x4? -- 20050128 martin
  if (this->isValid(shader, name, CG_FLOAT4x4)) {
    // FIXME: check, whether column or row mode should be used
    //        (cgGLSetMatrixParameterfc or cgGLSetMatrixParameterfr?)
    //                                                -- 20050128 martin
    glue_cgGLSetMatrixParameterfc(this->cgParameter, value);
  }
}


void
SoGLCgShaderParameter::setMatrixArray(const SoGLShaderObject * shader,
                                      const int num, const float * value,
                                      const char * name, const int)
{
  // *******************************************
  // FIXME -> NOT TESTED YET!!! 20050128 martin
  // *******************************************

  // FIXME: Support of other matrices, e.g. float3x4? -- 20050128 martin
  int cnt = num;
  if (this->isValid(shader, name, CG_ARRAY, &cnt)) {
    // FIXME: check, whether column or row mode should be used
    //       (cgGLSetMatrixParameterArrayfc or cgGLSetMatrixParameterArrayfr?)
    //                                             -- 20050128 martin
    glue_cgGLSetMatrixParameterArrayfc(this->cgParameter, 0, cnt, value);
  }
}

void
SoGLCgShaderParameter::set1i(const SoGLShaderObject * shader,
                             const int32_t value, const char * name, const int)
{
  if (this->isValid(shader, name, CG_INT))
    glue_cgGLSetParameter1f(this->cgParameter, (float)value);
}

void
SoGLCgShaderParameter::set2i(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                             const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                             const int)
{
  // FIXME not implemented yet -- 20050222 martin
}

void
SoGLCgShaderParameter::set3i(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                             const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                             const int)
{
  // FIXME not implemented yet -- 20050222 martin
}

void
SoGLCgShaderParameter::set4i(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                             const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                             const int)
{
  // FIXME not implemented yet -- 20050222 martin
}

void
SoGLCgShaderParameter::set1iv(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                              const int COIN_UNUSED_ARG(num),
                              const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                              const int)
{
  // probably not supported. pederb, 20070530
}

void
SoGLCgShaderParameter::set2iv(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                              const int COIN_UNUSED_ARG(num),
                              const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                              const int)
{
  // probably not supported. pederb, 20070530
}

void
SoGLCgShaderParameter::set3iv(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                              const int COIN_UNUSED_ARG(num),
                              const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                              const int)
{
  // probably not supported. pederb, 20070530
}

void
SoGLCgShaderParameter::set4iv(const SoGLShaderObject * COIN_UNUSED_ARG(shader),
                              const int COIN_UNUSED_ARG(num),
                              const int32_t * COIN_UNUSED_ARG(value), const char * COIN_UNUSED_ARG(name),
                              const int)
{
  // probably not supported. pederb, 20070530
}

SbBool
SoGLCgShaderParameter::isEqual(CGtype type1, CGtype type2)
{
  if (type1 == type2)
    return TRUE;
  else if (type1 == CG_FLOAT && type2 == CG_FLOAT1)
    return TRUE;
  else if (type1 == CG_FLOAT1 && type2 == CG_FLOAT)
    return TRUE;
  else if (type1 == CG_INT && type2 == CG_INT1)
    return TRUE;
  else if (type1 == CG_INT1 && type2 == CG_INT)
    return TRUE;

  if (type2 == CG_INT) {
    switch (type1) {
    case CG_SAMPLER1D:
    case CG_SAMPLER2D:
    case CG_SAMPLER3D:
    case CG_SAMPLERRECT:
    case CG_SAMPLERCUBE: return TRUE;
    default: return FALSE;
    }
  }
  return FALSE;
}

SbBool
SoGLCgShaderParameter::isValid(const SoGLShaderObject * shader,
                               const char* name, CGtype type, int * num)
{
  assert(shader->shaderType() == SoShader::CG_SHADER);

  if (!name || name[0] == '\0') return FALSE;

  // FIXME: how to handle a new compiled shader object? -- martin
  if (this->isEqual(this->cacheType, type) && (this->cacheName == name) &&
      glue_cgIsParameter(this->cgParameter)) {
    if (num) { // assume: this->cacheType == CG_ARRAY
      if (this->cacheSize < *num) {
        // FIXME: better error handling - 20050128 martin
        SoDebugError::postWarning("SoGLCgShaderParameter::isValid",
                                  "parameter %s[%d] < input[%d]!",
                                  this->cacheName.getString(),
                                  this->cacheSize, *num);
        *num = this->cacheSize;
      }
      return (*num > 0);
    }
    return TRUE;
  }

  CGprogram * cgProg = &((SoGLCgShaderObject*)shader)->cgProgram;

  this->cacheType   = CG_UNKNOWN_TYPE;
  this->cgParameter = glue_cgGetNamedParameter(*cgProg, name);
  this->cacheName   = name;
  this->cacheSize   = 0;

  if (!glue_cgIsParameter(this->cgParameter)) return FALSE;

  this->cacheType = glue_cgGetParameterType(this->cgParameter);

  if (!this->isEqual(this->cacheType, type)) {
    SoDebugError::postWarning("SoGLCgShaderParameter::isValid",
                              "In main(): parameter %s [%s] is "
                              "of wrong type [%s]!",
                              this->cacheName.getString(),
                              glue_cgGetTypeString(this->cacheType),
                              glue_cgGetTypeString(type));
    this->cacheType = CG_UNKNOWN_TYPE;
    return FALSE;
  }

  // handle array
  if (this->cacheType == CG_ARRAY) {
    int dim = glue_cgGetArrayDimension(this->cgParameter);

    if (dim != 1) {
      SoDebugError::postWarning("SoGLCgShaderParameter::isValid",
                                "In main(): Dimension of %s is %d "
                                "(only dim=1 is supported)",
                                this->cacheName.getString(), dim);
      return FALSE;
    }
    // FIXME: What about checking the array sub type? - 20050128 martin

    this->cacheSize = glue_cgGetArraySize(this->cgParameter, 0);
    if (num) {
      if (this->cacheSize < *num) {
        // FIXME: better error handling - 20050128 martin
        SoDebugError::postWarning("SoGLCgShaderParameter::isValid",
                                  "parameter %s[%d] < input[%d]!",
                                  this->cacheName.getString(),
                                  this->cacheSize, *num);
        *num = this->cacheSize;
      }
      return (*num > 0);
    }
    else
      return FALSE;
  }
  return glue_cgIsParameter(this->cgParameter);
}
