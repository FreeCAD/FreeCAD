#ifndef COIN_SOGLSLSHADERPROGRAM_H
#define COIN_SOGLSLSHADERPROGRAM_H

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

#include <Inventor/lists/SbList.h>

#include "misc/SbHash.h"
#include "glue/glp.h"

class SoGLSLShaderObject;
class SoState;
class SbName;

// *************************************************************************

class SoGLSLShaderProgram
{
public:
  void addShaderObject(SoGLSLShaderObject *shaderObject);
  void removeShaderObjects(void);
  void enable(const cc_glglue * g);
  void disable(const cc_glglue * g);
  void postShouldLink(void);

  void updateCoinParameter(SoState * state, const SbName & name, const int value);
  void addProgramParameter(int mode, int value);
  void removeProgramParameters(void);

#if defined(SOURCE_HINT)
  SbString getSourceHint(void) const;
#endif

public:
  SoGLSLShaderProgram(void);
  ~SoGLSLShaderProgram();

  COIN_GLhandle getProgramHandle(const cc_glglue * g, const SbBool create = FALSE);
  SbBool neededLinking(void) const;

protected:
  SbList <int> programParameters;
  SbList <SoGLSLShaderObject *> shaderObjects;
  SbHash<uint32_t, COIN_GLhandle> programHandles;

  SbBool isExecutable;
  SbBool neededlinking;

  int indexOfShaderObject(SoGLSLShaderObject * shaderObject);
  void ensureLinking(const cc_glglue * g);
  void ensureProgramHandle(const cc_glglue * g);

private:
  void deletePrograms(void);
  void deleteProgram(const cc_glglue * g);

  static void context_destruction_cb(uint32_t cachecontext, void * userdata);
  static void really_delete_object(void * closure, uint32_t contextid);

};

#endif /* ! COIN_SOGLSLSHADERPROGRAM_H */
