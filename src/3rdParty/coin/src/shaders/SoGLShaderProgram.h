#ifndef COIN_SOGLSHADERPROGRAM_H
#define COIN_SOGLSHADERPROGRAM_H

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

#include <Inventor/SbString.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/lists/SbList.h>

class SoGLShaderObject;
class SoState;
class SbName;

// *************************************************************************

class SoGLShaderProgram
{
public:
  SoGLShaderProgram(void);
  ~SoGLShaderProgram();
  void addShaderObject(SoGLShaderObject * shaderObject);
  void removeShaderObjects(void);
  void enable(SoState * state);
  void disable(SoState * state);
  SbBool isEnabled(void) const;

  void setEnableCallback(SoShaderProgramEnableCB * cb,
                         void * closure);


#if defined(SOURCE_HINT) // FIXME: what's this? 20050120 mortene.
  SbString getSourceHint(void);
#endif

  void updateCoinParameter(SoState * state, const SbName & name, const int value);
  void addProgramParameter(int name, int value);

  void getShaderObjectIds(SbList <uint32_t> & ids) const;
  uint32_t getGLSLShaderProgramHandle(SoState * state) const;
  SbBool glslShaderProgramLinked(void) const;
private:

  class SoGLARBShaderProgram * arbShaderProgram;
  class SoGLCgShaderProgram  * cgShaderProgram;
  class SoGLSLShaderProgram  * glslShaderProgram;

  SbBool isenabled;
  SoShaderProgramEnableCB * enablecb;
  void * enablecbclosure;
  SbList <uint32_t> objectids;
};

#endif /* ! COIN_SOGLSHADERPROGRAM_H */
