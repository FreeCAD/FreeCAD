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

#include "shaders/SoGLSLShaderProgram.h"

#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/misc/SoContextHandler.h>

#include "shaders/SoGLSLShaderObject.h"
#include <Inventor/errors/SoDebugError.h>
#include "glue/glp.h"

// *************************************************************************

// FIXME: no checking is done to see whether "shader objects" (as for
// GL_ARB_shader_objects) are actually supported or not. 20050124 mortene.

// *************************************************************************

SoGLSLShaderProgram::SoGLSLShaderProgram(void)
  : programHandles(5)
{
  this->isExecutable = FALSE;
  this->neededlinking = TRUE;
  SoContextHandler::addContextDestructionCallback(context_destruction_cb, this);
}

SoGLSLShaderProgram::~SoGLSLShaderProgram()
{
  SoContextHandler::removeContextDestructionCallback(context_destruction_cb, this);
  this->deletePrograms();
}


void
SoGLSLShaderProgram::deleteProgram(const cc_glglue * g)
{
  COIN_GLhandle glhandle = 0;
  if (this->programHandles.get(g->contextid, glhandle)) {
    uintptr_t tmp = (uintptr_t) glhandle;
    SoGLCacheContextElement::scheduleDeleteCallback(g->contextid,
                                                    really_delete_object, (void*) tmp);
    this->programHandles.erase(g->contextid);
  }
}

void
SoGLSLShaderProgram::deletePrograms(void)
{
  SbList <uint32_t> keylist;
  this->programHandles.makeKeyList(keylist);
  for (int i = 0; i < keylist.getLength(); i++) {
    COIN_GLhandle glhandle = 0;
    (void) this->programHandles.get(keylist[i], glhandle);
    uintptr_t tmp = (uintptr_t) glhandle;
    SoGLCacheContextElement::scheduleDeleteCallback(keylist[i],
                                                    really_delete_object, (void*) tmp);
    this->programHandles.erase(keylist[i]);
  }
}


void
SoGLSLShaderProgram::addShaderObject(SoGLSLShaderObject *shaderObject)
{
  if (shaderObject!=NULL) {
    if (this->indexOfShaderObject(shaderObject) < 0) {
      this->shaderObjects.append(shaderObject);
    }
  }
}

void
SoGLSLShaderProgram::removeShaderObjects(void)
{
  this->shaderObjects.truncate(0);
}

void
SoGLSLShaderProgram::enable(const cc_glglue * g)
{
  this->neededlinking = FALSE;
  this->ensureLinking(g);

  if (this->isExecutable) {
    COIN_GLhandle programhandle = this->getProgramHandle(g, TRUE);
    g->glUseProgramObjectARB(programhandle);

    if (SoGLSLShaderObject::didOpenGLErrorOccur("SoGLSLShaderProgram::enable")) {
      SoGLSLShaderObject::printInfoLog(g, programhandle, 0);
    }
  }
}

void
SoGLSLShaderProgram::disable(const cc_glglue * g)
{
  if (this->isExecutable) {
    g->glUseProgramObjectARB(0);
  }
}

#if defined(SOURCE_HINT)
SbString
SoGLSLShaderProgram::getSourceHint(void) const
{
  SbString result;
  for (int i=0; i<this->shaderObjects.size(); i++) {
    SoGLSLShaderObject *shader = this->shaderObjects[i];
    if (shader && shader->isActive()) {
      SbString str = shader->sourceHint;
      if (str.getLength() > 0) str += " ";
      result += str;
    }
  }
  return result;
}
#endif

void
SoGLSLShaderProgram::ensureLinking(const cc_glglue * g)
{
  SbBool shouldlink = FALSE;
  for (int i = 0; i < this->shaderObjects.getLength() && !shouldlink; i++) {
    if (!this->shaderObjects[i]->isAttached()) shouldlink = TRUE;
  }

  if (!shouldlink) return;

  // delete old programs
  this->deleteProgram(g);

  this->isExecutable = FALSE;

  COIN_GLhandle programHandle = this->getProgramHandle(g, TRUE);

  int cnt = this->shaderObjects.getLength();

  if (cnt > 0) {
    int i;
    GLint didLink = 0;

    for (i = 0; i < cnt; i++) {
      this->shaderObjects[i]->attach(programHandle);
    }

    for (i = 0; i < this->programParameters.getLength(); i += 2) {
      g->glProgramParameteriEXT(programHandle,
                                (GLenum) this->programParameters[i],
                                this->programParameters[i+1]);

    }

    g->glLinkProgramARB(programHandle);

    if (SoGLSLShaderObject::didOpenGLErrorOccur("SoGLSLShaderProgram::ensureLinking")) {
      SoGLSLShaderObject::printInfoLog(g, programHandle, 0);
    }
    g->glGetObjectParameterivARB(programHandle,
                                 GL_OBJECT_LINK_STATUS_ARB,&didLink);

    this->isExecutable = didLink;
    this->neededlinking = TRUE;
  }
}

int
SoGLSLShaderProgram::indexOfShaderObject(SoGLSLShaderObject *shaderObject)
{
  if (shaderObject == NULL) return -1;

  int cnt = this->shaderObjects.getLength();
  for (int i=0; i<cnt; i++) {
    if (shaderObject == this->shaderObjects[i]) return i;
  }
  return -1;
}

void
SoGLSLShaderProgram::ensureProgramHandle(const cc_glglue * g)
{
  (void) this->getProgramHandle(g, TRUE);
}

COIN_GLhandle
SoGLSLShaderProgram::getProgramHandle(const cc_glglue * g, const SbBool create)
{
  COIN_GLhandle handle = 0;
  if (!this->programHandles.get(g->contextid, handle) && create) {
    handle = g->glCreateProgramObjectARB();
    this->programHandles.put(g->contextid, handle);
  }
  return handle;
}

SbBool
SoGLSLShaderProgram::neededLinking(void) const
{
  return this->neededlinking;
}

void
SoGLSLShaderProgram::context_destruction_cb(uint32_t cachecontext, void * userdata)
{
  SoGLSLShaderProgram * thisp = (SoGLSLShaderProgram*) userdata;

  COIN_GLhandle glhandle = 0;
  if (thisp->programHandles.get(cachecontext, glhandle)) {
    // just delete immediately. The context is current
    const cc_glglue * glue = cc_glglue_instance(cachecontext);
    glue->glDeleteObjectARB(glhandle);
    thisp->programHandles.erase(cachecontext);
  }
}

void
SoGLSLShaderProgram::really_delete_object(void * closure, uint32_t contextid)
{
  uintptr_t tmp = (uintptr_t) closure;

  COIN_GLhandle glhandle = (COIN_GLhandle) tmp;

  const cc_glglue * glue = cc_glglue_instance(contextid);
  glue->glDeleteObjectARB(glhandle);
}

void
SoGLSLShaderProgram::updateCoinParameter(SoState * state, const SbName & name, const int value)
{
  const int n = this->shaderObjects.getLength();
  for (int i = 0; i < n; i++) {
    this->shaderObjects[i]->updateCoinParameter(state, name, NULL, value);
  }
}

void
SoGLSLShaderProgram::addProgramParameter(int mode, int value)
{
  this->programParameters.append(mode);
  this->programParameters.append(value);
}

void
SoGLSLShaderProgram::removeProgramParameters(void)
{
  this->programParameters.truncate(0);
}
