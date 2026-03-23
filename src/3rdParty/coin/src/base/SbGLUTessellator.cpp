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

#include "SbGLUTessellator.h"

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/system/gl.h>

// *************************************************************************

SbBool
SbGLUTessellator::available(void)
{
  return GLUWrapper()->available &&
    GLUWrapper()->gluNewTess &&
    GLUWrapper()->gluTessBeginPolygon &&
    GLUWrapper()->gluTessBeginContour &&
    GLUWrapper()->gluTessEndContour &&
    GLUWrapper()->gluTessEndPolygon &&
    GLUWrapper()->gluDeleteTess;
}

// *************************************************************************

SbGLUTessellator::SbGLUTessellator(void (* cb)(void *, void *, void *, void *),
                                   void * userdata)
{
  assert(cb && "tessellation without callback is meaningless");
  this->callback = cb;
  this->cbdata = userdata;

  // allocated later on demand, so there is no resource allocation
  // when just putting an SbGLUTessellator on the current stack frame:
  this->tessobj = NULL;
}

SbGLUTessellator::~SbGLUTessellator()
{
  if (this->tessobj) { GLUWrapper()->gluDeleteTess(this->tessobj); }
}

// *************************************************************************

void APIENTRY
SbGLUTessellator::cb_begin(GLenum mode, void * x)
{
  SbGLUTessellator * t = static_cast<SbGLUTessellator *>(x);

  t->triangletessmode = mode;
  t->vertexidx = 0;
  t->stripflipflop = FALSE;
}

void APIENTRY
SbGLUTessellator::cb_vertex(void * vertex_data, void * x)
{
  SbGLUTessellator * t = static_cast<SbGLUTessellator *>(x);

  switch (t->triangletessmode) {
  case GL_TRIANGLE_FAN:
    if (t->vertexidx == 0) { t->vertexdata[0] = vertex_data; }
    else if (t->vertexidx == 1) { t->vertexdata[1] = vertex_data; }
    else {
      t->callback(t->vertexdata[0], t->vertexdata[1], vertex_data, t->cbdata);
      t->vertexdata[1] = vertex_data;
    }
    break;

  case GL_TRIANGLE_STRIP:
    if (t->vertexidx == 0) { t->vertexdata[0] = vertex_data; }
    else if (t->vertexidx == 1) { t->vertexdata[1] = vertex_data; }
    else {
      t->callback(t->vertexdata[t->stripflipflop ? 1 : 0],
                  t->vertexdata[t->stripflipflop ? 0 : 1],
                  vertex_data,
                  t->cbdata);

      t->vertexdata[0] = t->vertexdata[1];
      t->vertexdata[1] = vertex_data;
      t->stripflipflop = t->stripflipflop ? FALSE : TRUE;
    }
    break;

  case GL_TRIANGLES:
    if (t->vertexidx % 3 == 0) { t->vertexdata[0] = vertex_data; }
    else if (t->vertexidx % 3 == 1) { t->vertexdata[1] = vertex_data; }
    else if (t->vertexidx % 3 == 2) {
      t->callback(t->vertexdata[0], t->vertexdata[1], vertex_data, t->cbdata);
    }
    break;

  default:
    assert(FALSE);
    break;
  }

  t->vertexidx++;
}

void APIENTRY
SbGLUTessellator::cb_error(GLenum err, void *)
{
  // These would be user errors on our side, so catch them:
  assert(err != GLU_TESS_MISSING_BEGIN_POLYGON);
  assert(err != GLU_TESS_MISSING_END_POLYGON);
  assert(err != GLU_TESS_MISSING_BEGIN_CONTOUR);
  assert(err != GLU_TESS_MISSING_END_CONTOUR);
  
  // We will get this error if there are polygons with intersecting
  // edges (a "bow-tie" polygon, for instance), but this may be hard
  // to avoid for the app programmer, so we have made it possible to
  // silence this error messages by an envvar (according to the GLU
  // docs, the tessellator will be ok, it just ignores those polygons
  // and generates no output):
  if (err == GLU_TESS_NEED_COMBINE_CALLBACK) {
    static int v = -1;
    if (v == -1) {
      const char * env = coin_getenv("COIN_GLU_SILENCE_TESS_COMBINE_WARNING");
      v = env && (atoi(env) > 0);
    }
    // requested to be silenced
    if (v) { return; }
  }

  SoDebugError::post("SbGLUTessellator::cb_error",
                     "GLU library tessellation error: '%s'",
                     GLUWrapper()->gluErrorString(err));
}

// *************************************************************************

void
SbGLUTessellator::beginPolygon(const SbVec3f & normal)
{
  if (!this->tessobj) {
    coin_GLUtessellator * t = this->tessobj = GLUWrapper()->gluNewTess();

    const gluTessCallback_t f = GLUWrapper()->gluTessCallback;
    (*f)(t, GLU_TESS_BEGIN_DATA, reinterpret_cast<gluTessCallback_cb_t>(SbGLUTessellator::cb_begin));
    (*f)(t, GLU_TESS_VERTEX_DATA, reinterpret_cast<gluTessCallback_cb_t>(SbGLUTessellator::cb_vertex));
    (*f)(t, GLU_TESS_ERROR_DATA, reinterpret_cast<gluTessCallback_cb_t>(SbGLUTessellator::cb_error));
  }

  GLUWrapper()->gluTessBeginPolygon(this->tessobj, this);
  if (normal != SbVec3f(0, 0, 0)) {
    GLUWrapper()->gluTessNormal(this->tessobj, normal[0], normal[1], normal[2]);
  }

  GLUWrapper()->gluTessBeginContour(this->tessobj);
}

void
SbGLUTessellator::addVertex(const SbVec3f & v, void * data)
{
  struct v c = { { v[0], v[1], v[2] } };
  this->coords.append(c); // needs to be stored until gluTessEndPolygon()

  const int l = this->coords.getLength();
  GLUWrapper()->gluTessVertex(this->tessobj, this->coords[l - 1].c, data);
}

void
SbGLUTessellator::endPolygon(void)
{
  GLUWrapper()->gluTessEndContour(this->tessobj);
  GLUWrapper()->gluTessEndPolygon(this->tessobj);

  this->coords.truncate(0);
}

// *************************************************************************

// Whether or not the SbGLUTessellator should be preferred over our
// own Coin SbTesselator class, for tessellating faceset polygons.
SbBool
SbGLUTessellator::preferred(void)
{
  static int v = -1;
  if (v == -1) {
    const char * e = coin_getenv("COIN_PREFER_GLU_TESSELLATOR");
    v = (e && (atoi(e) > 0)) ? 1 : 0;

    if (v && !SbGLUTessellator::available()) {
      SoDebugError::postWarning("SbGLUTessellator::preferred",
                                "Preference setting "
                                "COIN_PREFER_GLU_TESSELLATOR indicates that "
                                "GLU tessellation is wanted, but GLU library "
                                "detected to not have this capability.");
      v = 0;
    }
  }
  return v ? TRUE : FALSE;
}

// *************************************************************************
