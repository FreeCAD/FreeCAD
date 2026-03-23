#ifndef COIN_SOGLARBSHADERPARAMETER_H
#define COIN_SOGLARBSHADERPARAMETER_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif

// *************************************************************************

#include "SoGLShaderParameter.h"

// *************************************************************************

class SoGLARBShaderParameter : public SoGLShaderParameter
{
 public: // satisfy SoGLShaderParameter protocol interface
  virtual SoShader::Type shaderType(void) const;

  virtual void set1f(const SoGLShaderObject * shader, const float value, const char * name, const int id);
  virtual void set2f(const SoGLShaderObject * shader, const float * value, const char * name, const int id);
  virtual void set3f(const SoGLShaderObject * shader, const float * value, const char * name, const int id);
  virtual void set4f(const SoGLShaderObject * shader, const float * value, const char * name, const int id);

  virtual void set1fv(const SoGLShaderObject * shader, const int num, const float* value, const char* name, const int id);
  virtual void set2fv(const SoGLShaderObject * shader, const int num, const float* value, const char* name, const int id);
  virtual void set3fv(const SoGLShaderObject * shader, const int num, const float* value, const char* name, const int id);
  virtual void set4fv(const SoGLShaderObject * shader, const int num, const float* value, const char* name, const int id);

  virtual void setMatrix(const SoGLShaderObject * shader, const float * value, const char * name, const int id);
  virtual void setMatrixArray(const SoGLShaderObject * shader, const int num, const float * value, const char * name, const int id);

  virtual void set1i(const SoGLShaderObject * shader, const int32_t value, const char * name, const int id);
  virtual void set2i(const SoGLShaderObject * shader, const int32_t * value, const char * name, const int id);
  virtual void set3i(const SoGLShaderObject * shader, const int32_t * value, const char * name, const int id);
  virtual void set4i(const SoGLShaderObject * shader, const int32_t * value, const char * name, const int id);

  virtual void set1iv(const SoGLShaderObject * shader, const int num, const int32_t * value, const char * name, const int id);
  virtual void set2iv(const SoGLShaderObject * shader, const int num, const int32_t * value, const char * name, const int id);
  virtual void set3iv(const SoGLShaderObject * shader, const int num, const int32_t * value, const char * name, const int id);
  virtual void set4iv(const SoGLShaderObject * shader, const int num, const int32_t * value, const char * name, const int id);

public:
  SoGLARBShaderParameter(void);
  virtual ~SoGLARBShaderParameter();

private:
  GLenum target;
  GLuint identifier;

  SbBool isValid(const SoGLShaderObject * shader, const int idx);
};

#endif /* ! COIN_SOGLARBSHADERPARAMETER_H */
