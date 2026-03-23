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

#include "SoGLSLShaderParameter.h"
#include "SoGLSLShaderObject.h"

#include <Inventor/errors/SoDebugError.h>
#include <cstdio>

// *************************************************************************

SoGLSLShaderParameter::SoGLSLShaderParameter(void)
{
  this->location  = -1;
  this->cacheType = GL_FLOAT;
  this->cacheName = "";
  this->cacheSize =  0;
  this->isActive = TRUE;
  this->programid = 0;
}

SoGLSLShaderParameter::~SoGLSLShaderParameter()
{
}

SoShader::Type
SoGLSLShaderParameter::shaderType(void) const
{
  return SoShader::GLSL_SHADER;
}

void
SoGLSLShaderParameter::set1f(const SoGLShaderObject * shader,
                             const float value, const char *name, const int)
{
  if (this->isValid(shader, name, GL_FLOAT))
    shader->GLContext()->glUniform1fARB(this->location, value);
}

void
SoGLSLShaderParameter::set2f(const SoGLShaderObject * shader,
                             const float * value, const char *name, const int)
{
  if (this->isValid(shader, name, GL_FLOAT_VEC2_ARB))
    shader->GLContext()->glUniform2fARB(this->location, value[0], value[1]);
}

void
SoGLSLShaderParameter::set3f(const SoGLShaderObject * shader,
                             const float * v, const char *name, const int)
{
  if (this->isValid(shader, name, GL_FLOAT_VEC3_ARB))
    shader->GLContext()->glUniform3fARB(this->location, v[0], v[1], v[2]);
}

void
SoGLSLShaderParameter::set4f(const SoGLShaderObject * shader,
                             const float * v, const char *name, const int)
{
  if (this->isValid(shader, name, GL_FLOAT_VEC4_ARB))
    shader->GLContext()->glUniform4fARB(this->location, v[0], v[1], v[2], v[3]);
}


void
SoGLSLShaderParameter::set1fv(const SoGLShaderObject * shader, const int num,
                              const float *value, const char * name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, GL_FLOAT, &cnt))
    shader->GLContext()->glUniform1fvARB(this->location, cnt, value);
}

void
SoGLSLShaderParameter::set2fv(const SoGLShaderObject * shader, const int num,
                              const float* value, const char* name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, GL_FLOAT_VEC2_ARB, &cnt))
    shader->GLContext()->glUniform2fvARB(this->location, cnt, value);
}

void
SoGLSLShaderParameter::set3fv(const SoGLShaderObject * shader, const int num,
                              const float* value, const char * name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, GL_FLOAT_VEC3_ARB, &cnt))
    shader->GLContext()->glUniform3fvARB(this->location, cnt, value);
}

void
SoGLSLShaderParameter::set4fv(const SoGLShaderObject * shader, const int num,
                              const float* value, const char * name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, GL_FLOAT_VEC4_ARB, &cnt))
    shader->GLContext()->glUniform4fvARB(this->location, cnt, value);
}

void
SoGLSLShaderParameter::setMatrix(const SoGLShaderObject *shader,
                                 const float * value, const char * name,
                                 const int)
{
  if (this->isValid(shader, name, GL_FLOAT_MAT4_ARB))
    shader->GLContext()->glUniformMatrix4fvARB(this->location,1,FALSE,value);
}


void
SoGLSLShaderParameter::setMatrixArray(const SoGLShaderObject *shader,
                                      const int num, const float *value,
                                      const char *name, const int)
{
  int cnt = num;
  if (this->isValid(shader, name, GL_FLOAT_MAT4_ARB, &cnt))
    shader->GLContext()->glUniformMatrix4fvARB(this->location,cnt,FALSE,value);
}


void
SoGLSLShaderParameter::set1i(const SoGLShaderObject * shader,
                             const int32_t value, const char * name, const int)
{
  if (this->isValid(shader, name, GL_INT))
    shader->GLContext()->glUniform1iARB(this->location, value);
}

void
SoGLSLShaderParameter::set2i(const SoGLShaderObject * shader,
                             const int32_t * value, const char * name,
                             const int)
{
  if (this->isValid(shader, name, GL_INT_VEC2_ARB))
    shader->GLContext()->glUniform2iARB(this->location, value[0], value[1]);
}

void
SoGLSLShaderParameter::set3i(const SoGLShaderObject * shader,
                             const int32_t * v, const char * name,
                             const int)
{
  if (this->isValid(shader, name, GL_INT_VEC3_ARB))
    shader->GLContext()->glUniform3iARB(this->location, v[0], v[1], v[2]);
}

void
SoGLSLShaderParameter::set4i(const SoGLShaderObject * shader,
                             const int32_t * v, const char * name,
                             const int)
{
  if (this->isValid(shader, name, GL_INT_VEC4_ARB))
    shader->GLContext()->glUniform4iARB(this->location, v[0], v[1], v[2], v[3]);
}

void
SoGLSLShaderParameter::set1iv(const SoGLShaderObject * shader,
                              const int num,
                              const int32_t * value, const char * name,
                              const int)
{
  if (this->isValid(shader, name, GL_INT))
    shader->GLContext()->glUniform1ivARB(this->location, num, (const GLint*) value);
}

void
SoGLSLShaderParameter::set2iv(const SoGLShaderObject * shader,
                              const int num,
                              const int32_t * value, const char * name,
                              const int)
{
  if (this->isValid(shader, name, GL_INT_VEC2_ARB))
    shader->GLContext()->glUniform2ivARB(this->location, num, (const GLint*)value);
}

void
SoGLSLShaderParameter::set3iv(const SoGLShaderObject * shader,
                              const int num,
                              const int32_t * v, const char * name,
                              const int)
{
  if (this->isValid(shader, name, GL_INT_VEC3_ARB))
    shader->GLContext()->glUniform3ivARB(this->location, num, (const GLint*)v);
}

void
SoGLSLShaderParameter::set4iv(const SoGLShaderObject * shader,
                              const int num,
                              const int32_t * v, const char * name,
                              const int)
{
  if (this->isValid(shader, name, GL_INT_VEC4_ARB))
    shader->GLContext()->glUniform4ivARB(this->location, num, (const GLint*)v);
}

SbBool
SoGLSLShaderParameter::isEqual(GLenum type1, GLenum type2)
{
  if (type1 == type2)
    return TRUE;

  if (type2 == GL_INT) {
    switch (type1) {
    case GL_INT:
    case GL_SAMPLER_1D_ARB:
    case GL_SAMPLER_2D_ARB:
    case GL_SAMPLER_3D_ARB:
    case GL_SAMPLER_CUBE_ARB:
    case GL_SAMPLER_1D_SHADOW_ARB:
    case GL_SAMPLER_2D_SHADOW_ARB:
    case GL_SAMPLER_2D_RECT_ARB:
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:
      return TRUE;
    default:
      return FALSE;
    }
  }
  return FALSE;
}

SbBool
SoGLSLShaderParameter::isValid(const SoGLShaderObject * shader,
                               const char * name, GLenum type,
                               int * num)
{
  assert(shader);
  assert(shader->shaderType() == SoShader::GLSL_SHADER);

  COIN_GLhandle pHandle = ((SoGLSLShaderObject*)shader)->programHandle;
  int32_t pId = ((SoGLSLShaderObject*)shader)->programid;

  // return TRUE if uniform isn't active. We warned the user about
  // this when we found it to be inactive.
  if ((pId == this->programid) && (this->location > -1) && !this->isActive) return TRUE;

  if ((pId == this->programid) && (this->location > -1) &&
      (this->cacheName == name) && this->isEqual(this->cacheType, type)) {
    if (num) { // assume: ARRAY
      if (this->cacheSize < *num) {
        // FIXME: better error handling - 20050128 martin
        SoDebugError::postWarning("SoGLSLShaderParameter::isValid",
                                  "parameter %s[%d] < input[%d]!",
                                  this->cacheName.getString(),
                                  this->cacheSize, *num);
        *num = this->cacheSize;
      }
      return (*num > 0);
    }
    return TRUE;
  }

  const cc_glglue * g = shader->GLContext();

  this->cacheSize = 0;
  this->location = g->glGetUniformLocationARB(pHandle,
                                              (const COIN_GLchar *)name);
  this->programid = pId;

  if (this->location == -1)  {
#if COIN_DEBUG
    SoDebugError::postWarning("SoGLSLShaderParameter::isValid",
                              "parameter '%s' not found in program.",
                              name);
#endif // COIN_DEBUG
    return FALSE;
  }
  GLint activeUniforms = 0;
  g->glGetObjectParameterivARB(pHandle, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &activeUniforms);

  GLint i;
  GLint tmpSize = 0;
  GLenum tmpType;
  GLsizei length;
  COIN_GLchar myName[256];

  this->cacheName = name;
  this->isActive = FALSE; // set uniform to inactive while searching

  // this will only happen once after the variable has been added so
  // it's not a performance issue that we have to search for it here.
  for (i = 0; i < activeUniforms; i++) {
    g->glGetActiveUniformARB(pHandle, i, 128, &length, &tmpSize,
                             &tmpType, myName);
    if (this->cacheName == myName) {
      this->cacheSize = tmpSize;
      this->cacheType = tmpType;
      this->isActive = TRUE;
      break;
    }
  }
  if (!this->isActive) {
    // not critical, but warn user so they can remove the unused parameter
#if COIN_DEBUG
    SoDebugError::postWarning("SoGLSLShaderParameter::isValid",
                              "parameter '%s' not active.",
                              this->cacheName.getString());
#endif // COIN_DEBUG
    // return here since cacheSize and cacheType will not be properly initialized
    return TRUE;
  }

  if (!this->isEqual(this->cacheType, type)) {
    SoDebugError::postWarning("SoGLSLShaderParameter::isValid",
                              "parameter %s [%d] is "
                              "of wrong type [%d]!",
                              this->cacheName.getString(),
                              this->cacheType, type);
    this->cacheType = GL_FLOAT;
    return FALSE;
  }

  if (num) { // assume: ARRAY
    if (this->cacheSize < *num) {
      // FIXME: better error handling - 20050128 martin
      SoDebugError::postWarning("SoGLSLShaderParameter::isValid",
                                "parameter %s[%d] < input[%d]!",
                                this->cacheName.getString(),
                                this->cacheSize, *num);
      *num = this->cacheSize;
    }
    return (*num > 0);
  }
  return TRUE;
}
