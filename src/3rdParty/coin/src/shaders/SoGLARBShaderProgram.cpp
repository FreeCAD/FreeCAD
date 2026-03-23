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

#include "SoGLARBShaderProgram.h"

#include <cstring>
#include <cassert>

#include <Inventor/errors/SoDebugError.h>
#include "SoGLARBShaderObject.h"

// *************************************************************************

// FIXME: no checking is done to see whether "ARB shading" is actually
// supported or not. 20050124 mortene.

// *************************************************************************

SoGLARBShaderProgram::SoGLARBShaderProgram(void)
{
  this->fragmentShader = NULL;
  this->vertexShader = NULL;
}

void
SoGLARBShaderProgram::addShaderObject(SoGLARBShaderObject * shaderObject)
{
  if (shaderObject->getShaderType() == SoGLShaderObject::VERTEX) {
    this->vertexShader = shaderObject;
  }
  else {
    assert(shaderObject->getShaderType() == SoGLShaderObject::FRAGMENT);
    this->fragmentShader = shaderObject;
  }
}

void
SoGLARBShaderProgram::removeShaderObjects(void)
{
  this->vertexShader = NULL;
  this->fragmentShader = NULL;
}

void
SoGLARBShaderProgram::enable(void)
{
  if (this->fragmentShader) this->fragmentShader->enable();
  if (this->vertexShader) this->vertexShader->enable();
}

void
SoGLARBShaderProgram::disable(void)
{
  if (this->fragmentShader) this->fragmentShader->disable();
  if (this->vertexShader) this->vertexShader->disable();
}

#if defined(SOURCE_HINT)
SbString
SoGLARBShaderProgram::getSourceHint(void) const
{
  SbString result;

  if (this->fragmentShader && this->fragmentShader->isActive()) {
    SbString str = this->fragmentShader->sourceHint;
    if (str.getLength() > 0) str += " ";
    result += str;
  }
  if (this->vertexShader && this->vertexShader->isActive()) {
    SbString str = this->vertexShader->sourceHint;
    if (str.getLength() > 0) str += " ";
    result += str;
  }
  return result;
}
#endif
