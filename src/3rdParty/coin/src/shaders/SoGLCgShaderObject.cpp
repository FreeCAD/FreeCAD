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

#include "shaders/SoGLCgShaderObject.h"

#include <cassert>

#include <Inventor/errors/SoDebugError.h>

#include "shaders/SoGLCgShaderParameter.h"
#include "glue/cg.h"

// *************************************************************************

CGcontext SoGLCgShaderObject::cgContext = NULL;
int SoGLCgShaderObject::instanceCount = 0;

SoGLCgShaderObject::SoGLCgShaderObject(const uint32_t cachecontext)
  : SoGLShaderObject(cachecontext)
{
  SoGLCgShaderObject::instanceCount++;
  this->cgProgram = NULL;
}

SoGLCgShaderObject::~SoGLCgShaderObject()
{
  this->unload();
  SoGLCgShaderObject::destroyCgContext();
}

SoShader::Type
SoGLCgShaderObject::shaderType(void) const
{
  return SoShader::CG_SHADER;
}

SoGLShaderParameter *
SoGLCgShaderObject::getNewParameter(void) const
{
  return new SoGLCgShaderParameter();
}

SbBool
SoGLCgShaderObject::isLoaded(void) const
{
  return glue_cgIsProgram(this->cgProgram);
}

void
SoGLCgShaderObject::load(const char* sourceString)
{
  CGerror errorCode = CG_NO_ERROR;
 
  SoGLCgShaderObject::ensureCgContext();

  this->unload();

  this->cgProfile = getProfile();
  //cgGLSetOptimalOptions(this->cgProfile);

  glue_cgSetErrorCallback(NULL);
  glue_cgGetError(); // remove last error from stack

  this->cgProgram =
    glue_cgCreateProgram(SoGLCgShaderObject::cgContext,
                         CG_SOURCE, 
                         sourceString,
                         this->cgProfile,
                         "main", // entry's function name
                         NULL); // argument names
  errorCode = glue_cgGetError();

  if (errorCode == CG_NO_ERROR) {
    glue_cgGLLoadProgram(this->cgProgram);
    errorCode = glue_cgGetError();
  }

  if (errorCode != CG_NO_ERROR) {
    this->unload();
    SoGLCgShaderObject::printError(errorCode, SoGLCgShaderObject::cgContext);
  }

  glue_cgSetErrorCallback(SoGLCgShaderObject::cgErrorCallback);

  return;
}

void
SoGLCgShaderObject::unload(void)
{
  if (glue_cgIsProgram(this->cgProgram)) {
    glue_cgDestroyProgram(this->cgProgram);
    this->cgProgram = NULL;
  }
}


void
SoGLCgShaderObject::enable(void)
{
  if (glue_cgIsProgram(this->cgProgram)) {
    glue_cgGLBindProgram(this->cgProgram);
    glue_cgGLEnableProfile(this->cgProfile);
  }
}

void
SoGLCgShaderObject::disable(void)
{
  if (glue_cgIsProgram(this->cgProgram)) glue_cgGLDisableProfile(this->cgProfile);
}

CGprofile
SoGLCgShaderObject::getProfile(void) const
{
  CGprofile result = this->getShaderType() == VERTEX ? CG_PROFILE_ARBVP1 : CG_PROFILE_ARBFP1;

  if (!glue_cgGLIsProfileSupported(result)) {
    SoDebugError::postWarning("SoGLCgShaderObject::getProfile",
                              "profile '%s' is not supported",
                              glue_cgGetProfileString(result));

    if (this->getShaderType() == VERTEX) {
      result = glue_cgGLGetLatestProfile(CG_GL_VERTEX);
    }
    else {
      assert(this->getShaderType() == FRAGMENT);
      result = glue_cgGLGetLatestProfile(CG_GL_FRAGMENT);
    }
    SoDebugError::postWarning("SoGLCgShaderObject::getProfile",
                              "'%s' will be used instead",
                              glue_cgGetProfileString(result));
  }
  return result;
}

// --- static methods -------------------------------------------------------

void
SoGLCgShaderObject::printError(CGerror error, CGcontext context)
{
#if defined(SOURCE_HINT) && 0
  std::cerr << "+++ " << this->sourceHint.getString() << " +++" << std::endl;
#endif
  SoDebugError::post("SoGLCgShaderObject::printError",
                     "'%s'", glue_cgGetErrorString(error));
  SoDebugError::post("SoGLCgShaderObject::printError",
                     "'%s'", glue_cgGetLastListing(context));
}

void
SoGLCgShaderObject::cgErrorCallback(void)
{
  CGerror lastError = glue_cgGetError();

  if(lastError) printError(lastError, SoGLCgShaderObject::cgContext);
}

void
SoGLCgShaderObject::ensureCgContext(void)
{
  if (!glue_cgIsContext(SoGLCgShaderObject::cgContext)) {
    SoGLCgShaderObject::cgContext = glue_cgCreateContext();
    glue_cgGLSetManageTextureParameters(cgContext, TRUE);
    glue_cgSetErrorCallback(SoGLCgShaderObject::cgErrorCallback);
  }
}

void
SoGLCgShaderObject::destroyCgContext(void)
{
  SoGLCgShaderObject::instanceCount--;

  if (SoGLCgShaderObject::instanceCount > 0) return;
  if (glue_cgIsContext(SoGLCgShaderObject::cgContext)) {
    glue_cgDestroyContext(SoGLCgShaderObject::cgContext);
  }
}
