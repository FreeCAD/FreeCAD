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

// This is an internal class with utility functions we use when
// playing around with OpenGL.

// *************************************************************************

#include "rendering/SoGL.h"
#include "coindefs.h"

#include <cassert>
#include <cstdio>
#include <cstring>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/C/tidbits.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/bundles/SoVertexAttributeBundle.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoProfileElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoProfile.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/system/gl.h>
#include <Inventor/threads/SbStorage.h>

#include "glue/GLUWrapper.h"
#include "tidbitsp.h"
#include "glue/glp.h"

// *************************************************************************

// Convenience function for access to OpenGL wrapper from an SoState
// pointer.
const cc_glglue *
sogl_glue_instance(const SoState * state)
{
  SoGLRenderAction * action = (SoGLRenderAction *)state->getAction();
  // FIXME: disabled until we figure out why this doesn't work on some
  // Linux systems (gcc 3.2 systems, it seems). pederb, 2003-11-24
#if 0
  assert(action->isOfType(SoGLRenderAction::getClassTypeId()) &&
         "must have state from SoGLRenderAction to get hold of GL wrapper");
  return cc_glglue_instance(action->getCacheContext());
#else // disabled
  if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
    return cc_glglue_instance(action->getCacheContext());
  }
  static int didwarn = 0;
  if (!didwarn) {
    didwarn = 1;
    SoDebugError::postWarning("sogl_glue_instance",
                              "Wrong action type detected. Please report this to <coin-support@coin3d.org>, "
                              "and include information about your system (compiler, Linux version, etc.");
  }
  // just return some cc_glglue instance. It usually doesn't matter
  // that much unless multiple contexts on multiple displays are used.
  return cc_glglue_instance(1);
#endif // workaround version
}


// generate a 3d circle in the x-z plane
static void
sogl_generate_3d_circle(SbVec3f *coords, const int num, const float radius, const float y)
{
  float delta = 2.0f*float(M_PI)/float(num);
  float angle = 0.0f;
  for (int i = 0; i < num; i++) {
    coords[i][0] = -float(sin(angle)) * radius;
    coords[i][1] = y;
    coords[i][2] = -float(cos(angle)) * radius;
    angle += delta;
  }
}

// generate a 2d circle
static void
sogl_generate_2d_circle(SbVec2f *coords, const int num, const float radius)
{
  float delta = 2.0f*float(M_PI)/float(num);
  float angle = 0.0f;
  for (int i = 0; i < num; i++) {
    coords[i][0] = -float(sin(angle)) * radius;
    coords[i][1] = -float(cos(angle)) * radius;
    angle += delta;
  }
}

void
sogl_render_cone(const float radius,
                 const float height,
                 const int numslices,
                 SoMaterialBundle * const material,
                 const unsigned int flagsin,
                 SoState * state)
{
  const SbBool * unitenabled = NULL;
  int maxunit = 0;
  const cc_glglue * glue = NULL;

  int flags = flagsin;

  if (state) {
    unitenabled =
      SoMultiTextureEnabledElement::getEnabledUnits(state, maxunit);
    if (unitenabled) {
      glue = sogl_glue_instance(state);
      flags |= SOGL_NEED_MULTITEXCOORDS;
    }
    else maxunit = -1;
  }

  int i,u;
  // use a limit of 128 to avoid allocating memory each time
  // a cone is drawn
  int slices = numslices;
  if (slices > 128) slices = 128;
  if (slices < 4) slices = 4;

  float h2 = height * 0.5f;

  // put coordinates on the stack
  SbVec3f coords[129];
  SbVec3f normals[130];
  SbVec2f texcoords[129];

  sogl_generate_3d_circle(coords, slices, radius, -h2);
  coords[slices] = coords[0];
  if (flags & (SOGL_NEED_TEXCOORDS|SOGL_NEED_3DTEXCOORDS|SOGL_NEED_MULTITEXCOORDS)) {
    sogl_generate_2d_circle(texcoords, slices, 0.5f);
    texcoords[slices] = texcoords[0];
  }

  if (flags & SOGL_NEED_NORMALS) {
    double a = atan(height/radius);
    sogl_generate_3d_circle(normals, slices, float(sin(a)), float(cos(a)));
    normals[slices] = normals[0];
    normals[slices+1] = normals[1];
  }

  int matnr = 0;

  // FIXME: the texture coordinate generation for cone sides is of
  // sub-par quality. The textures comes out looking "skewed" and
  // "compressed". 20010926 mortene.

  if (flags & SOGL_RENDER_SIDE) {
    glBegin(GL_TRIANGLES);
    i = 0;

    float t = 1.0;
    float delta = 1.0f / slices;

    while (i < slices) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t - delta*0.5f, 1.0f);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3f(0.5f, 1.0f, 0.5f);
      }
      if (flags & SOGL_NEED_NORMALS) {
        SbVec3f n = (normals[i] + normals[i+1])*0.5f;
        glNormal3f(n[0], n[1], n[2]);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        t - delta*0.5f, 1.0f);
          }
        }
      }

      glVertex3f(0.0f, h2, 0.0f);
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t, 0.0f);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3f(texcoords[i][0]+0.5f, 0.0f, texcoords[i][1]+0.5f);
      }
      if (flags & SOGL_NEED_NORMALS) {
        glNormal3fv((const GLfloat*)&normals[i]);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        t, 0.0f);
          }
        }
      }
      glVertex3fv((const GLfloat*)&coords[i]);

      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t - delta, 0.0f);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3f(texcoords[i+1][0]+0.5f, 0.0f, texcoords[i+1][1]+0.5f);
      }
      if (flags & SOGL_NEED_NORMALS) {
        glNormal3fv((const GLfloat*)&normals[i+1]);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        t - delta, 0.0f);
          }
        }
      }
      glVertex3fv((const GLfloat*)&coords[i+1]);

      i++;
      t -= delta;
    }

    matnr++;
    glEnd();
  }

  if (flags & SOGL_RENDER_BOTTOM) {
    if (flags & SOGL_MATERIAL_PER_PART) {
      material->send(matnr, TRUE);
    }

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f);
    for (i = slices-1; i >= 0; i--) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(texcoords[i][0]+0.5f, texcoords[i][1]+0.5f);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3f(texcoords[i][0]+0.5f, 0.0f, texcoords[i][1]+0.5f);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        texcoords[i][0]+0.5f, texcoords[i][1]+0.5f);
          }
        }
      }

      glVertex3fv((const GLfloat*)&coords[i]);
    }
    glEnd();
  }
  if (state && (SoComplexityTypeElement::get(state) ==
                SoComplexityTypeElement::OBJECT_SPACE)) {
    // encourage auto caching for object space
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DO_AUTO_CACHE);
    SoGLCacheContextElement::incNumShapes(state);
  }
  else {
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
  }
}

void
sogl_render_cylinder(const float radius,
                     const float height,
                     const int numslices,
                     SoMaterialBundle * const material,
                     const unsigned int flagsin,
                     SoState * state)
{
  const SbBool * unitenabled = NULL;
  int maxunit = 0;
  const cc_glglue * glue = NULL;

  int flags = flagsin;

  if (state) {
    unitenabled =
      SoMultiTextureEnabledElement::getEnabledUnits(state, maxunit);
    if (unitenabled) {
      glue = sogl_glue_instance(state);
      flags |= SOGL_NEED_MULTITEXCOORDS;
    }
    else maxunit = -1;
  }

  int i, u;
  int slices = numslices;
  if (slices > 128) slices = 128;
  if (slices < 4) slices = 4;

  float h2 = height * 0.5f;

  SbVec3f coords[129];
  SbVec3f normals[130];
  SbVec2f texcoords[129];

  sogl_generate_3d_circle(coords, slices, radius, -h2);
  coords[slices] = coords[0];
  if (flags & SOGL_NEED_3DTEXCOORDS ||
      (flags & SOGL_NEED_TEXCOORDS &&
       flags & (SOGL_RENDER_BOTTOM | SOGL_RENDER_TOP))) {
    sogl_generate_2d_circle(texcoords, slices, 0.5f);
    texcoords[slices] = texcoords[0];
  }

  if (flags & SOGL_NEED_NORMALS) {
    sogl_generate_3d_circle(normals, slices, 1.0f, 0.0f);
    normals[slices] = normals[0];
    normals[slices+1] = normals[1];
  }

  int matnr = 0;

  if (flags & SOGL_RENDER_SIDE) {
    glBegin(GL_QUAD_STRIP);
    i = 0;

    float t = 0.0;
    float inc = 1.0f / slices;

    while (i <= slices) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t, 1.0f);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3f(texcoords[i][0]+0.5f, 1.0f, 1.0f - texcoords[i][1]-0.5f);
      }
      if (flags & SOGL_NEED_NORMALS) {
        glNormal3fv((const GLfloat*)&normals[i]);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        t, 1.0f);
          }
        }
      }

      SbVec3f c = coords[i];
      glVertex3f(c[0], h2, c[2]);
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t, 0.0f);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3f(texcoords[i][0]+0.5f, 0.0f, 1.0f - texcoords[i][1]-0.5f);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        t, 0.0f);
          }
        }
      }
      glVertex3f(c[0], c[1], c[2]);
      i++;
      t += inc;
    }

    matnr++;
    glEnd();
  }

  if ((flags & (SOGL_NEED_TEXCOORDS|SOGL_NEED_3DTEXCOORDS|SOGL_NEED_MULTITEXCOORDS)) &&
      (flags & (SOGL_RENDER_BOTTOM | SOGL_RENDER_TOP))) {
    sogl_generate_2d_circle(texcoords, slices, 0.5f);
    texcoords[slices] = texcoords[0];
  }

  if (flags & SOGL_RENDER_TOP) {
    if (flags & SOGL_MATERIAL_PER_PART) {
      material->send(matnr, TRUE);
    }
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);

    for (i = 0; i < slices; i++) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(texcoords[i][0]+0.5f, 1.0f - texcoords[i][1]-0.5f);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3f(texcoords[i][0]+0.5f, 1.0f, 1.0f - texcoords[i][1]-0.5f);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                       texcoords[i][0]+0.5f, 1.0f - texcoords[i][1]-0.5f);
          }
        }
      }
      const SbVec3f &c = coords[i];
      glVertex3f(c[0], h2, c[2]);
    }
    glEnd();
    matnr++;
  }
  if (flags & SOGL_RENDER_BOTTOM) {
    if (flags & SOGL_MATERIAL_PER_PART) {
      material->send(matnr, TRUE);
    }
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f);

    for (i = slices-1; i >= 0; i--) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(texcoords[i][0]+0.5f, texcoords[i][1]+0.5f);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3f(texcoords[i][0]+0.5f, 0.0f, 1.0f - texcoords[i][1]-0.5f);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        texcoords[i][0]+0.5f, texcoords[i][1]+0.5f);
          }
        }
      }
      glVertex3fv((const GLfloat*)&coords[i]);
    }
    glEnd();
  }
  if (state && (SoComplexityTypeElement::get(state) ==
                SoComplexityTypeElement::OBJECT_SPACE)) {
    // encourage auto caching for object space
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DO_AUTO_CACHE);
    SoGLCacheContextElement::incNumShapes(state);
  }
  else {
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
  }
}

void
sogl_render_sphere(const float radius,
                   const int numstacks,
                   const int numslices,
                   SoMaterialBundle * const /* material */,
                   const unsigned int flagsin,
                   SoState * state)
{
  const SbBool * unitenabled = NULL;
  int maxunit = 0;
  const cc_glglue * glue = NULL;

  unsigned int flags = flagsin;

  if (state && (flags & SOGL_NEED_TEXCOORDS)) {
    unitenabled =
      SoMultiTextureEnabledElement::getEnabledUnits(state, maxunit);
    if (unitenabled) {
      glue = sogl_glue_instance(state);
      flags |= SOGL_NEED_MULTITEXCOORDS;
    }
    else maxunit = -1;
  }

  int stacks = numstacks;
  int slices = numslices;

  if (stacks < 3) stacks = 3;
  if (slices < 4) slices = 4;

  if (slices > 128) slices = 128;

  // used to cache last stack's data
  SbVec3f coords[129];
  SbVec3f normals[129];
  float S[129];
  SbVec3f texcoords[129];

  int i, j, u;
  float rho;
  float drho;
  float theta;
  float dtheta;
  float tc, ts;
  SbVec3f tmp;

  drho = float(M_PI) / (float) (stacks-1);
  dtheta = 2.0f * float(M_PI) / (float) slices;

  float currs = 0.0f;
  float incs = 1.0f / (float)slices;
  rho = drho;
  theta = 0.0f;
  tc = (float) cos(rho);
  ts = - (float) sin(rho);
  tmp.setValue(0.0f,
               tc,
               ts);
  normals[0] = tmp;
  texcoords[0] = tmp/2 + SbVec3f(0.5f,0.5f,0.5f);
  tmp *= radius;
  coords[0] = tmp;
  S[0] = currs;
  float dT = 1.0f / (float) (stacks-1);
  float T = 1.0f - dT;

  glBegin(GL_TRIANGLES);

  for (j = 1; j <= slices; j++) {
    glNormal3f(0.0f, 1.0f, 0.0f);
    if (flags & SOGL_NEED_TEXCOORDS) {
      glTexCoord2f(currs + 0.5f * incs, 1.0f);
    }
    else if (flags & SOGL_NEED_3DTEXCOORDS) {
      glTexCoord3f(0.5f, 1.0f, 0.5f);
    }
    if (flags & SOGL_NEED_MULTITEXCOORDS) {
      for (u = 1; u <= maxunit; u++) {
        if (unitenabled[u]) {
          cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                      currs + 0.5f * incs, 1.0f);
        }
      }
    }
    glVertex3f(0.0f, radius, 0.0f);

    glNormal3fv((const GLfloat*) &normals[j-1]);
    if (flags & SOGL_NEED_TEXCOORDS) {
      glTexCoord2f(currs, T);
    }
    else if (flags & SOGL_NEED_3DTEXCOORDS) {
      glTexCoord3fv((const GLfloat*) &texcoords[j-1]);
    }
    if (flags & SOGL_NEED_MULTITEXCOORDS) {
      for (u = 1; u <= maxunit; u++) {
        if (unitenabled[u]) {
          cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                      currs, T);
        }
      }
    }

    glVertex3fv((const GLfloat*) &coords[j-1]);

    currs += incs;
    theta += dtheta;
    tmp.setValue(float(sin(theta))*ts,
                 tc,
                 float(cos(theta))*ts);

    normals[j] = tmp;
    glNormal3fv((const GLfloat*)&normals[j]);
    if (flags & SOGL_NEED_TEXCOORDS) {
      S[j] = currs;
      glTexCoord2f(currs, T);
    }
    else if (flags & SOGL_NEED_3DTEXCOORDS) {
      texcoords[j] = tmp/2 + SbVec3f(0.5f,0.5f,0.5f);
      glTexCoord3fv((const GLfloat*) &texcoords[j]);
    }
    if (flags & SOGL_NEED_MULTITEXCOORDS) {
      for (u = 1; u <= maxunit; u++) {
        if (unitenabled[u]) {
          cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                      currs, T);
        }
      }
    }
    tmp *= radius;
    coords[j] = tmp;
    glVertex3fv((const GLfloat*)&coords[j]);
  }
  glEnd(); // GL_TRIANGLES

  rho += drho;

  for (i = 2; i < stacks-1; i++) {
    tc = (float)cos(rho);
    ts = - (float) sin(rho);
    glBegin(GL_QUAD_STRIP);
    theta = 0.0f;
    for (j = 0; j <= slices; j++) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(S[j], T);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3fv((const GLfloat*) &texcoords[j]);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        S[j], T);
          }
        }
      }
      glNormal3fv((const GLfloat*)&normals[j]);
      glVertex3fv((const GLfloat*)&coords[j]);

      tmp.setValue(float(sin(theta))*ts,
                   tc,
                   float(cos(theta))*ts);
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(S[j], T - dT);
      }
      else if (flags & SOGL_NEED_3DTEXCOORDS) {
        texcoords[j] = tmp/2 + SbVec3f(0.5f,0.5f,0.5f);
        glTexCoord3fv((const GLfloat*) &texcoords[j]);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                        S[j], T - dT);
          }
        }
      }
      normals[j] = tmp;
      glNormal3f(tmp[0], tmp[1], tmp[2]);
      tmp *= radius;
      glVertex3f(tmp[0], tmp[1], tmp[2]);
      coords[j] = tmp;
      theta += dtheta;
    }
    glEnd(); // GL_QUAD_STRIP
    rho += drho;
    T -= dT;
  }

  glBegin(GL_TRIANGLES);
  for (j = 0; j < slices; j++) {
    if (flags & SOGL_NEED_TEXCOORDS) {
      glTexCoord2f(S[j], T);
    }
    else if (flags & SOGL_NEED_3DTEXCOORDS) {
      glTexCoord3fv((const GLfloat*) &texcoords[j]);
    }
    if (flags & SOGL_NEED_MULTITEXCOORDS) {
      for (u = 1; u <= maxunit; u++) {
        if (unitenabled[u]) {
          cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                      S[j], T);
        }
      }
    }
    glNormal3fv((const GLfloat*)&normals[j]);
    glVertex3fv((const GLfloat*)&coords[j]);

    if (flags & SOGL_NEED_TEXCOORDS) {
      glTexCoord2f(S[j]+incs*0.5f, 0.0f);
    }
    else if (flags & SOGL_NEED_3DTEXCOORDS) {
      glTexCoord3f(0.5f, 0.0f, 0.5f);
    }
    if (flags & SOGL_NEED_MULTITEXCOORDS) {
      for (u = 1; u <= maxunit; u++) {
        if (unitenabled[u]) {
          cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                      S[j]+incs*0.5f, 0.0f);
        }
      }
    }
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, -radius, 0.0f);

    if (flags & SOGL_NEED_TEXCOORDS) {
      glTexCoord2f(S[j+1], T);
    }
    else if (flags & SOGL_NEED_3DTEXCOORDS) {
      glTexCoord3fv((const GLfloat*) &texcoords[j+1]);
    }
    if (flags & SOGL_NEED_MULTITEXCOORDS) {
      for (u = 1; u <= maxunit; u++) {
        if (unitenabled[u]) {
          cc_glglue_glMultiTexCoord2f(glue, (GLenum) (GL_TEXTURE0 + u),
                                      S[j+1], T);
        }
      }
    }
    glNormal3fv((const GLfloat*)&normals[j+1]);
    glVertex3fv((const GLfloat*)&coords[j+1]);
  }
  glEnd(); // GL_TRIANGLES

  if (state && (SoComplexityTypeElement::get(state) ==
                SoComplexityTypeElement::OBJECT_SPACE)) {
    // encourage auto caching for object space
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DO_AUTO_CACHE);
    SoGLCacheContextElement::incNumShapes(state);
  }
  else {
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
  }
}

//
// the 12 triangles in the cube
//
static int sogl_cube_vindices[] =
{
  0, 1, 3, 2,
  5, 4, 6, 7,
  1, 5, 7, 3,
  4, 0, 2, 6,
  4, 5, 1, 0,
  2, 3, 7, 6
};

static float sogl_cube_texcoords[] =
{
  1.0f, 1.0f,
  0.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f
};

static float sogl_cube_3dtexcoords[][3] =
{
  {1.0f, 1.0f, 1.0f},
  {1.0f, 1.0f, 0.0f},
  {1.0f, 0.0f, 1.0f},
  {1.0f, 0.0f, 0.0f},
  {0.0f, 1.0f, 1.0f},
  {0.0f, 1.0f, 0.0f},
  {0.0f, 0.0f, 1.0f},
  {0.0f, 0.0f, 0.0f}
};

static float sogl_cube_normals[] =
{
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, -1.0f,
  -1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, -1.0f, 0.0f
};

static void
sogl_generate_cube_vertices(SbVec3f *varray,
                       const float w,
                       const float h,
                       const float d)
{
  for (int i = 0; i < 8; i++) {
    varray[i].setValue((i&1) ? -w : w,
                       (i&2) ? -h : h,
                       (i&4) ? -d : d);
  }
}


void
sogl_render_cube(const float width,
                 const float height,
                 const float depth,
                 SoMaterialBundle * const material,
                 const unsigned int flagsin,
                 SoState * state)
{
  const SbBool * unitenabled = NULL;
  int maxunit = 0;
  const cc_glglue * glue = NULL;

  int flags = flagsin;

  if (state) {
    unitenabled =
      SoMultiTextureEnabledElement::getEnabledUnits(state, maxunit);
    if (unitenabled) {
      glue = sogl_glue_instance(state);
      flags |= SOGL_NEED_MULTITEXCOORDS;
    }
    else maxunit = -1;
  }


  SbVec3f varray[8];
  sogl_generate_cube_vertices(varray,
                         width * 0.5f,
                         height * 0.5f,
                         depth * 0.5f);
  glBegin(GL_QUADS);
  int *iptr = sogl_cube_vindices;
  int u;

  for (int i = 0; i < 6; i++) { // 6 quads
    if (flags & SOGL_NEED_NORMALS)
      glNormal3fv((const GLfloat*)&sogl_cube_normals[i*3]);
    if (flags & SOGL_MATERIAL_PER_PART)
      material->send(i, TRUE);
    for (int j = 0; j < 4; j++) {
      if (flags & SOGL_NEED_3DTEXCOORDS) {
        glTexCoord3fv(sogl_cube_3dtexcoords[*iptr]);
      }
      else if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2fv(&sogl_cube_texcoords[j<<1]);
      }
      if (flags & SOGL_NEED_MULTITEXCOORDS) {
        for (u = 1; u <= maxunit; u++) {
          if (unitenabled[u]) {
            cc_glglue_glMultiTexCoord2fv(glue, (GLenum) (GL_TEXTURE0 + u),
                                         &sogl_cube_texcoords[j<<1]);
          }
        }
      }
      glVertex3fv((const GLfloat*)&varray[*iptr++]);
    }
  }
  glEnd();

  if (state) {
    // always encourage auto caching for cubes
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DO_AUTO_CACHE);
    SoGLCacheContextElement::incNumShapes(state);
  }
}

// **************************************************************************

#if !defined(NO_LINESET_RENDER)

namespace { namespace SoGL { namespace IndexedLineSet {

  enum AttributeBinding {
    OVERALL = 0,
    PER_SEGMENT = 1,
    PER_SEGMENT_INDEXED = 2,
    PER_LINE = 3,
    PER_LINE_INDEXED = 4,
    PER_VERTEX = 5,
    PER_VERTEX_INDEXED = 6
  };

  template < int NormalBinding,
             int MaterialBinding,
             int TexturingEnabled >
  static void GLRender(const SoGLCoordinateElement * coords,
                       const int32_t *indices,
                       int num_vertexindices,
                       const SbVec3f *normals,
                       const int32_t *normindices,
                       SoMaterialBundle *const materials,
                       const int32_t *matindices,
                       const SoTextureCoordinateBundle * const texcoords,
                       const int32_t *texindices,
                       const int drawAsPoints)
  {
    const SbVec3f * coords3d = NULL;
    const SbVec4f * coords4d = NULL;
    const SbBool is3d = coords->is3D();
    if (is3d) {
      coords3d = coords->getArrayPtr3();
    }
    else {
      coords4d = coords->getArrayPtr4();
    }
    int numcoords = coords->getNum();

    // This is the same code as in SoGLCoordinateElement::send().
    // It is inlined here for speed (~15% speed increase).
#define SEND_VERTEX(_idx_) \
    if (is3d) glVertex3fv((const GLfloat*) (coords3d + _idx_)); \
    else glVertex4fv((const GLfloat*) (coords4d + _idx_));

    // just in case someone forgot
    if (matindices == NULL) matindices = indices;
    if (normindices == NULL) normindices = indices;

    int matnr = 0;
    int texidx = 0;
    int32_t i;
    const int32_t *end = indices + num_vertexindices;

    SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
    const SbVec3f *currnormal = &dummynormal;
    if (normals) currnormal = normals;
    if ((AttributeBinding)NormalBinding == OVERALL) {
      glNormal3fv((const GLfloat*)currnormal);
    }

    if ((AttributeBinding)MaterialBinding == PER_SEGMENT ||
        (AttributeBinding)MaterialBinding == PER_SEGMENT_INDEXED ||
        (AttributeBinding)NormalBinding == PER_SEGMENT ||
        (AttributeBinding)NormalBinding == PER_SEGMENT_INDEXED) {
      int previ;

      if (drawAsPoints)
        glBegin(GL_POINTS);
      else
        glBegin(GL_LINES);

      while (indices < end) {
        previ = *indices++;

        // Variable used for counting errors and make sure not a bunch of
        // errormessages flood the screen.
        static uint32_t current_errors = 0;

        // This test is for robustness upon buggy data sets
        if (previ < 0 || previ >= numcoords) {
          if (current_errors < 1) {
            SoDebugError::postWarning("[indexedlineset]::GLRender", "Erroneous coordinate "
                                      "index: %d (Should be within [0, %d]). Aborting "
                                      "rendering. This message will be shown once, but "
                                      "there might be more errors", previ, numcoords - 1);
          }

          current_errors++;
          glEnd();
          return;
        }

        if ((AttributeBinding)MaterialBinding == PER_LINE ||
            (AttributeBinding)MaterialBinding == PER_VERTEX) {
          materials->send(matnr++, TRUE);
        } else if ((AttributeBinding)MaterialBinding == PER_LINE_INDEXED ||
                   (AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
          materials->send(*matindices++, TRUE);
        }

        if ((AttributeBinding)NormalBinding == PER_LINE ||
            (AttributeBinding)NormalBinding == PER_VERTEX) {
          currnormal = normals++;
          glNormal3fv((const GLfloat*) currnormal);
        } else if ((AttributeBinding)NormalBinding == PER_LINE_INDEXED ||
                   (AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
          currnormal = &normals[*normindices++];
          glNormal3fv((const GLfloat*) currnormal);
        }
        if (TexturingEnabled == TRUE) {
          texcoords->send(texindices ? *texindices++ : texidx++,coords->get3(previ), *currnormal);
        }
        i = (indices < end) ? *indices++ : -1;
        while (i >= 0) {
          // For robustness upon buggy data sets
          if (i >= numcoords) {
            if (current_errors < 1) {
              SoDebugError::postWarning("[indexedlineset]::GLRender", "Erroneous coordinate "
                                        "index: %d (Should be within [0, %d]). Aborting "
                                        "rendering. This message will be shown once, but "
                                        "there might be more errors", i, numcoords - 1);
            }
            current_errors++;
            break;
          }

          if ((AttributeBinding)MaterialBinding == PER_SEGMENT) {
            materials->send(matnr++, TRUE);
          } else if ((AttributeBinding)MaterialBinding == PER_SEGMENT_INDEXED) {
            materials->send(*matindices++, TRUE);
          }

          if ((AttributeBinding)NormalBinding == PER_SEGMENT) {
            currnormal = normals++;
            glNormal3fv((const GLfloat*) currnormal);
          } else if ((AttributeBinding)NormalBinding == PER_SEGMENT_INDEXED) {
            currnormal = &normals[*normindices++];
            glNormal3fv((const GLfloat*)currnormal);
          }
          SEND_VERTEX(previ);

          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            materials->send(matnr++, TRUE);
          } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
            materials->send(*matindices++, TRUE);
          }
          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            currnormal = normals++;
            glNormal3fv((const GLfloat*)currnormal);
          } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
            currnormal = &normals[*normindices++];
            glNormal3fv((const GLfloat*)currnormal);
          }
          if (TexturingEnabled == TRUE) {
            texcoords->send(texindices ? *texindices++ : texidx++, coords->get3(i), *currnormal);
          }
          SEND_VERTEX(i);
          previ = i;
          i = indices < end ? *indices++ : -1;
        }
        if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
          matindices++;
        }
        if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
          normindices++;
        }
        if (TexturingEnabled == TRUE) {
          if (texindices) texindices++;
        }
      }
      glEnd();

    } else { // no per_segment binding code below

      if (drawAsPoints)
        glBegin(GL_POINTS);

      while (indices < end) {
        if (!drawAsPoints)
          glBegin(GL_LINE_STRIP);

        i = *indices++;

        // Variable used for counting errors and make sure not a bunch of
        // errormessages flood the screen.
        static uint32_t current_errors = 0;

        // This test is for robustness upon buggy data sets
        if (i < 0 || i >= numcoords) {
          if (current_errors < 1) {
            SoDebugError::postWarning("[indexedlineset]::GLRender", "Erroneous coordinate "
                                      "index: %d (Should be within [0, %d]). Aborting "
                                      "rendering. This message will be shown once, but "
                                      "there might be more errors", i, numcoords - 1);
          }

          current_errors++;
          glEnd();
          return;
        }

        if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED ||
            (AttributeBinding)MaterialBinding == PER_LINE_INDEXED) {
          materials->send(*matindices++, TRUE);
        } else if ((AttributeBinding)MaterialBinding == PER_VERTEX ||
                   (AttributeBinding)MaterialBinding == PER_LINE) {
          materials->send(matnr++, TRUE);
        }

        if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED ||
            (AttributeBinding)NormalBinding == PER_LINE_INDEXED) {
          currnormal = &normals[*normindices++];
          glNormal3fv((const GLfloat*) currnormal);
        } else if ((AttributeBinding)NormalBinding == PER_VERTEX ||
                   (AttributeBinding)NormalBinding == PER_LINE) {
          currnormal = normals++;
          glNormal3fv((const GLfloat*) currnormal);
        }
        if (TexturingEnabled == TRUE) {
          texcoords->send(texindices ? *texindices++ : texidx++, coords->get3(i), *currnormal);
        }

        SEND_VERTEX(i);
        i = indices < end ? *indices++ : -1;
        while (i >= 0) {
          // For robustness upon buggy data sets
          if (i >= numcoords) {
            if (current_errors < 1) {
              SoDebugError::postWarning("[indexedlineset]::GLRender", "Erroneous coordinate "
                                        "index: %d (Should be within [0, %d]). Aborting "
                                        "rendering. This message will be shown once, but "
                                        "there might be more errors", i, numcoords - 1);
            }
            current_errors++;
            break;
          }

          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            materials->send(matnr++, TRUE);
          } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
            materials->send(*matindices++, TRUE);
          }

          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            currnormal = normals++;
            glNormal3fv((const GLfloat*) currnormal);
          } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
            currnormal = &normals[*normindices++];
            glNormal3fv((const GLfloat*) currnormal);
          }
          if (TexturingEnabled == TRUE) {
            texcoords->send(texindices ? *texindices++ : texidx++, coords->get3(i), *currnormal);
          }

          SEND_VERTEX(i);
          i = indices < end ? *indices++ : -1;
        }
        if (!drawAsPoints)
          glEnd(); // end of line strip

        if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
          matindices++;
        }
        if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
          normindices++;
        }
        if (TexturingEnabled == TRUE) {
          if (texindices) texindices++;
        }
      }
      if (drawAsPoints)
        glEnd();
    }
  }

} } } // namespace

#define SOGL_INDEXEDLINESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, texturing, args) \
  SoGL::IndexedLineSet::GLRender<normalbinding, materialbinding, texturing> args

#define SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3(normalbinding, materialbinding, texturing, args) \
  if (texturing) { \
    SOGL_INDEXEDLINESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, TRUE, args); \
  } else { \
    SOGL_INDEXEDLINESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, FALSE, args); \
  }

#define SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2(normalbinding, materialbinding, texturing, args) \
  switch (materialbinding) { \
  case SoGL::IndexedLineSet::OVERALL: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::IndexedLineSet::OVERALL, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_SEGMENT: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::IndexedLineSet::PER_SEGMENT, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_SEGMENT_INDEXED: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::IndexedLineSet::PER_SEGMENT_INDEXED, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_LINE: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::IndexedLineSet::PER_LINE, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_LINE_INDEXED: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::IndexedLineSet::PER_LINE_INDEXED, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_VERTEX: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::IndexedLineSet::PER_VERTEX, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_VERTEX_INDEXED: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::IndexedLineSet::PER_VERTEX_INDEXED, texturing, args); \
    break; \
  default: \
    assert(!"invalid material binding argument"); \
  }

#define SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args) \
  switch (normalbinding) { \
  case SoGL::IndexedLineSet::OVERALL: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2(SoGL::IndexedLineSet::OVERALL, materialbinding, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_SEGMENT: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2(SoGL::IndexedLineSet::PER_SEGMENT, materialbinding, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_SEGMENT_INDEXED: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2(SoGL::IndexedLineSet::PER_SEGMENT_INDEXED, materialbinding, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_LINE: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2(SoGL::IndexedLineSet::PER_LINE, materialbinding, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_LINE_INDEXED: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2(SoGL::IndexedLineSet::PER_LINE_INDEXED, materialbinding, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_VERTEX: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2(SoGL::IndexedLineSet::PER_VERTEX, materialbinding, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_VERTEX_INDEXED: \
    SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2(SoGL::IndexedLineSet::PER_VERTEX_INDEXED, materialbinding, texturing, args); \
    break; \
  default: \
    assert(!"invalid normal binding argument"); \
  }

#define SOGL_INDEXEDLINESET_GLRENDER(normalbinding, materialbinding, texturing, args) \
  SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args)

void
sogl_render_lineset(const SoGLCoordinateElement * const coords,
                    const int32_t *cindices,
                    int numindices,
                    const SbVec3f *normals,
                    const int32_t *nindices,
                    SoMaterialBundle *const mb,
                    const int32_t *mindices,
                    const SoTextureCoordinateBundle * const tb,
                    const int32_t *tindices,
                    int nbind,
                    int mbind,
                    const int texture,
                    const int drawAsPoints)
{

  SOGL_INDEXEDLINESET_GLRENDER(nbind, mbind, texture, (coords,
                                                       cindices,
                                                       numindices,
                                                       normals,
                                                       nindices,
                                                       mb,
                                                       mindices,
                                                       tb,
                                                       tindices,
                                                       drawAsPoints));
}

#undef SOGL_INDEXEDLINESET_GLRENDER_CALL_FUNC
#undef SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG1
#undef SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG2
#undef SOGL_INDEXEDLINESET_GLRENDER_RESOLVE_ARG3
#undef SOGL_INDEXEDLINESET_GLRENDER

#endif // !NO_LINESET_RENDER

// **************************************************************************

#if !defined(NO_FACESET_RENDER)

namespace { namespace SoGL { namespace FaceSet {

  enum AttributeBinding {
    OVERALL = 0,
    PER_FACE = 1,
    PER_FACE_INDEXED = 2,
    PER_VERTEX = 3,
    PER_VERTEX_INDEXED = 4
  };

  template < int NormalBinding,
             int MaterialBinding,
             int VertexAttributeBinding >
  static void GLRender(const SoGLCoordinateElement * const vertexlist,
                     const int32_t *vertexindices,
                     int numindices,
                     const SbVec3f *normals,
                     const int32_t *normalindices,
                     SoMaterialBundle *materials,
                     const int32_t *matindices,
                     const SoTextureCoordinateBundle * const texcoords,
                     const int32_t *texindices,
                       SoVertexAttributeBundle * const attribs,
                       const int dotexture,
                       const int doattribs)
  {

    // just in case someone forgot
    if (matindices == NULL) matindices = vertexindices;
    if (normalindices == NULL) normalindices = vertexindices;

    int texidx = 0;

    const SbVec3f * coords3d = NULL;
    const SbVec4f * coords4d = NULL;
    const SbBool is3d = vertexlist->is3D();
    if (is3d) {
      coords3d = vertexlist->getArrayPtr3();
    }
    else {
      coords4d = vertexlist->getArrayPtr4();
    }

    // This is the same code as in SoGLCoordinateElement::send().
    // It is inlined here for speed (~15% speed increase).
#define SEND_VERTEX(_idx_)                                           \
    if (is3d) glVertex3fv((const GLfloat*) (coords3d + _idx_));             \
    else glVertex4fv((const GLfloat*) (coords4d + _idx_));

    int mode = GL_POLYGON; // ...to save a test
    int newmode;
    const int32_t *viptr = vertexindices;
    const int32_t *vistartptr = vertexindices;
    const int32_t *viendptr = viptr + numindices;
    int32_t v1, v2, v3, v4, v5 = 0; // v5 init unnecessary, but kills a compiler warning.
    int numverts = vertexlist->getNum();

    SbVec3f dummynormal(0,0,1);
    const SbVec3f * currnormal = &dummynormal;
    if ((AttributeBinding)NormalBinding == PER_VERTEX ||
       (AttributeBinding)NormalBinding == PER_FACE ||
       (AttributeBinding)NormalBinding == PER_VERTEX_INDEXED ||
       (AttributeBinding)NormalBinding == PER_FACE_INDEXED ||
       dotexture) {
      if (normals) currnormal = normals;
    }

    int matnr = 0;
    int attribnr = 0;

    if (doattribs && (AttributeBinding)VertexAttributeBinding == OVERALL) {
      attribs->send(0);
    }

    while (viptr + 2 < viendptr) {
      v1 = *viptr++;
      v2 = *viptr++;
      v3 = *viptr++;

      // Variable used for counting errors and make sure not a
      // bunch of errormessages flood the screen.
      static uint32_t current_errors = 0;

      // This test is for robustness upon buggy data sets
      if (v1 < 0 || v2 < 0 || v3 < 0 ||
          v1 >= numverts || v2 >= numverts || v3 >= numverts) {

        if (current_errors < 1) {
          SoDebugError::postWarning("[faceset]::GLRender", "Erroneous polygon detected. "
                                    "Ignoring (offset: %d, [%d %d %d]). Should be within "
                                    " [0, %d] This message will only be shown once, but "
                                    "more errors might be present",
                                    viptr - vistartptr - 3, v1, v2, v3, numverts - 1);
        }
        current_errors++;
        break;
      }
      v4 = viptr < viendptr ? *viptr++ : -1;
      if (v4  < 0) newmode = GL_TRIANGLES;
      // This test for numverts is for robustness upon buggy data sets
      else if (v4 >= numverts) {
        newmode = GL_TRIANGLES;

        if (current_errors < 1) {
          SoDebugError::postWarning("[faceset]::GLRender", "Erroneous polygon detected. "
                                    "(offset: %d, [%d %d %d %d]). Should be within "
                                    " [0, %d] This message will only be shown once, but "
                                    "more errors might be present",
                                    viptr - vistartptr - 4, v1, v2, v3, v4, numverts - 1);
        }
        current_errors++;
      }
      else {
        v5 = viptr < viendptr ? *viptr++ : -1;
        if (v5 < 0) newmode = GL_QUADS;
        // This test for numverts is for robustness upon buggy data sets
        else if (v5 >= numverts) {
          newmode = GL_QUADS;

          if (current_errors < 1) {
            SoDebugError::postWarning("[faceset]::GLRender", "Erroneous polygon detected. "
                                      "(offset: %d, [%d %d %d %d %d]). Should be within "
                                      " [0, %d] This message will only be shown once, but "
                                      "more errors might be present",
                                      viptr - vistartptr - 5, v1, v2, v3, v4, v5, numverts - 1);
          }
          current_errors++;
        }
        else newmode = GL_POLYGON;
      }
      if (newmode != mode) {
        if (mode != GL_POLYGON) glEnd();
        mode = newmode;
        glBegin((GLenum) mode);
      }
      else if (mode == GL_POLYGON) glBegin(GL_POLYGON);

      /* vertex 1 *********************************************************/
      if ((AttributeBinding)MaterialBinding == PER_VERTEX ||
          (AttributeBinding)MaterialBinding == PER_FACE) {
        materials->send(matnr++, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED ||
                 (AttributeBinding)MaterialBinding == PER_FACE_INDEXED) {
        materials->send(*matindices++, TRUE);
      }

      if ((AttributeBinding)NormalBinding == PER_VERTEX ||
          (AttributeBinding)NormalBinding == PER_FACE) {
        currnormal = normals++;
        glNormal3fv((const GLfloat*)currnormal);
      } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED ||
                 (AttributeBinding)NormalBinding == PER_FACE_INDEXED) {
        currnormal = &normals[*normalindices++];
        glNormal3fv((const GLfloat*)currnormal);
      }

      if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX) {
       attribs->send(attribnr++);
      } else if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX_INDEXED) {
        attribs->send(*vertexindices++);
      }

      if (dotexture) {
        texcoords->send(texindices ? *texindices++ : texidx++,
                        vertexlist->get3(v1),
                        *currnormal);
      }

      SEND_VERTEX(v1);

      /* vertex 2 *********************************************************/
      if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
        materials->send(matnr++, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
        materials->send(*matindices++, TRUE);
      }

      // nvidia color-per-face-bug workaround
      if ((AttributeBinding)MaterialBinding == PER_FACE) {
        materials->send(matnr-1, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_FACE_INDEXED) {
        materials->send(matindices[-1], TRUE);
      }

      if ((AttributeBinding)NormalBinding == PER_VERTEX) {
        currnormal = normals++;
        glNormal3fv((const GLfloat*)currnormal);
      } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
        currnormal = &normals[*normalindices++];
        glNormal3fv((const GLfloat*)currnormal);
      }

      if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX) {
       attribs->send(attribnr++);
      } else if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX_INDEXED) {
        attribs->send(*vertexindices++);
      }

      if (dotexture) {
        texcoords->send(texindices ? *texindices++ : texidx++,
                        vertexlist->get3(v2),
                        *currnormal);
      }

      SEND_VERTEX(v2);

      /* vertex 3 *********************************************************/
      if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
        materials->send(matnr++, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
        materials->send(*matindices++, TRUE);
      }

      // nvidia color-per-face-bug workaround
      if ((AttributeBinding)MaterialBinding == PER_FACE) {
        materials->send(matnr-1, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_FACE_INDEXED) {
        materials->send(matindices[-1], TRUE);
      }

      if ((AttributeBinding)NormalBinding == PER_VERTEX) {
        currnormal = normals++;
        glNormal3fv((const GLfloat*)currnormal);
      } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
        currnormal = &normals[*normalindices++];
        glNormal3fv((const GLfloat*)currnormal);
      }

      if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX) {
       attribs->send(attribnr++);
      } else if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX_INDEXED) {
        attribs->send(*vertexindices++);
      }

      if (dotexture) {
        texcoords->send(texindices ? *texindices++ : texidx++,
                        vertexlist->get3(v3),
                        *currnormal);
      }

      SEND_VERTEX(v3);

      if (mode != GL_TRIANGLES) {
        /* vertex 4 (quad or polygon)**************************************/
        if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
          materials->send(matnr++, TRUE);
        } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
          materials->send(*matindices++, TRUE);
        }

        // nvidia color-per-face-bug workaround
        if ((AttributeBinding)MaterialBinding == PER_FACE) {
          materials->send(matnr-1, TRUE);
        } else if ((AttributeBinding)MaterialBinding == PER_FACE_INDEXED) {
          materials->send(matindices[-1], TRUE);
        }

        if ((AttributeBinding)NormalBinding == PER_VERTEX) {
          currnormal = normals++;
          glNormal3fv((const GLfloat*)currnormal);
        } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
          currnormal = &normals[*normalindices++];
          glNormal3fv((const GLfloat*)currnormal);
        }

        if (dotexture) {
          texcoords->send(texindices ? *texindices++ : texidx++,
                          vertexlist->get3(v4),
                          *currnormal);
        }

        if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX) {
          attribs->send(attribnr++);
        } 
        else if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX_INDEXED) {
          attribs->send(*vertexindices++);
        }
        SEND_VERTEX(v4);

        if (mode == GL_POLYGON) {
          /* vertex 5 (polygon) ********************************************/
          if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
            materials->send(matnr++, TRUE);
          } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
            materials->send(*matindices++, TRUE);
          }

          // nvidia color-per-face-bug workaround
          if ((AttributeBinding)MaterialBinding == PER_FACE) {
            materials->send(matnr-1, TRUE);
          } else if ((AttributeBinding)MaterialBinding == PER_FACE_INDEXED) {
            materials->send(matindices[-1], TRUE);
          }

          if ((AttributeBinding)NormalBinding == PER_VERTEX) {
            currnormal = normals++;
            glNormal3fv((const GLfloat*)currnormal);
          } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
            currnormal = &normals[*normalindices++];
            glNormal3fv((const GLfloat*)currnormal);
          }

          if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX) {
            attribs->send(attribnr++);
          } else if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX_INDEXED) {
            attribs->send(*vertexindices++);
          }

          if (dotexture) {
            texcoords->send(texindices ? *texindices++ : texidx++,
                            vertexlist->get3(v5),
                            *currnormal);

          }

          SEND_VERTEX(v5);

          v1 = viptr < viendptr ? *viptr++ : -1;
          while (v1 >= 0) {
            // For robustness upon buggy data sets
            if (v1 >= numverts) {
              if (current_errors < 1) {
                SoDebugError::postWarning("[faceset]::GLRender", "Erroneous polygon detected. "
                                          "(offset: %d, [... %d]). Should be within "
                                          "[0, %d] This message will only be shown once, but "
                                          "more errors might be present",
                                          viptr - vistartptr - 1, v1, numverts - 1);
              }
              current_errors++;
              break;
            }

            /* vertex 6-n (polygon) *****************************************/
            if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
              materials->send(matnr++, TRUE);
            } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
              materials->send(*matindices++, TRUE);
            }

            // nvidia color-per-face-bug workaround
            if ((AttributeBinding)MaterialBinding == PER_FACE) {
              materials->send(matnr-1, TRUE);
            } else if ((AttributeBinding)MaterialBinding == PER_FACE_INDEXED) {
              materials->send(matindices[-1], TRUE);
            }

            if ((AttributeBinding)NormalBinding == PER_VERTEX) {
              currnormal = normals++;
              glNormal3fv((const GLfloat*)currnormal);
            } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
              currnormal = &normals[*normalindices++];
              glNormal3fv((const GLfloat*)currnormal);
            }

            if (dotexture) {
              texcoords->send(texindices ? *texindices++ : texidx++,
                              vertexlist->get3(v1),
                              *currnormal);
            }

            if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX) {
              attribs->send(attribnr++);
            } else if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX_INDEXED) {
              attribs->send(*vertexindices++);
            }
            SEND_VERTEX(v1);

            v1 = viptr < viendptr ? *viptr++ : -1;
          }
          glEnd(); /* draw polygon */
        }
      }

      if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
        matindices++;
      }
      if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
        normalindices++;
      }
      if ((AttributeBinding)VertexAttributeBinding == PER_VERTEX_INDEXED) {
        vertexindices++;
      }

      if (dotexture) {
        if (texindices) texindices++;
      }
    }
    // check if triangle or quad
    if (mode != GL_POLYGON) glEnd();
  }

} } } // namespace

#define SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, vertexattributebinding, args) \
  SoGL::FaceSet::GLRender<normalbinding, materialbinding, vertexattributebinding> args

#define SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, materialbinding, vertexattributebinding, args) \
  switch (vertexattributebinding) { \
  case SoGL::FaceSet::OVERALL: \
    SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, SoGL::FaceSet::OVERALL, args); \
    break; \
  case SoGL::FaceSet::PER_FACE: \
    SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, SoGL::FaceSet::OVERALL, args); \
    break; \
  case SoGL::FaceSet::PER_FACE_INDEXED: \
    SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, SoGL::FaceSet::OVERALL, args); \
    break; \
  case SoGL::FaceSet::PER_VERTEX: \
    SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, SoGL::FaceSet::PER_VERTEX, args); \
    break; \
  case SoGL::FaceSet::PER_VERTEX_INDEXED: \
    SOGL_FACESET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, SoGL::FaceSet::PER_VERTEX_INDEXED, args); \
    break; \
  default: \
    assert(!"invalid vertex attribute binding argument"); \
  }

#define SOGL_FACESET_GLRENDER_RESOLVE_ARG2(normalbinding, materialbinding, vertexattributebinding, args) \
  switch (materialbinding) { \
  case SoGL::FaceSet::OVERALL: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::FaceSet::OVERALL, vertexattributebinding, args); \
    break; \
  case SoGL::FaceSet::PER_FACE: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::FaceSet::PER_FACE, vertexattributebinding, args); \
    break; \
  case SoGL::FaceSet::PER_FACE_INDEXED: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::FaceSet::PER_FACE_INDEXED, vertexattributebinding, args); \
    break; \
  case SoGL::FaceSet::PER_VERTEX: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::FaceSet::PER_VERTEX, vertexattributebinding, args); \
    break; \
  case SoGL::FaceSet::PER_VERTEX_INDEXED: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::FaceSet::PER_VERTEX_INDEXED, vertexattributebinding, args); \
    break; \
  default: \
    assert(!"invalid material binding argument"); \
  }

#define SOGL_FACESET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, vertexattributebinding, args) \
  switch (normalbinding) { \
  case SoGL::FaceSet::OVERALL: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG2(SoGL::FaceSet::OVERALL, materialbinding, vertexattributebinding, args); \
    break; \
  case SoGL::FaceSet::PER_FACE: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG2(SoGL::FaceSet::PER_FACE, materialbinding, vertexattributebinding, args); \
    break; \
  case SoGL::FaceSet::PER_FACE_INDEXED: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG2(SoGL::FaceSet::PER_FACE_INDEXED, materialbinding, vertexattributebinding, args); \
    break; \
  case SoGL::FaceSet::PER_VERTEX: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG2(SoGL::FaceSet::PER_VERTEX, materialbinding, vertexattributebinding, args); \
    break; \
  case SoGL::FaceSet::PER_VERTEX_INDEXED: \
    SOGL_FACESET_GLRENDER_RESOLVE_ARG2(SoGL::FaceSet::PER_VERTEX_INDEXED, materialbinding, vertexattributebinding, args); \
    break; \
  default: \
    assert(!"invalid normal binding argument"); \
  }

#define SOGL_FACESET_GLRENDER(normalbinding, materialbinding, vertexattributebinding, args) \
  SOGL_FACESET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, vertexattributebinding, args)


void
sogl_render_faceset(const SoGLCoordinateElement * const vertexlist,
                    const int32_t *vertexindices,
                    int num_vertexindices,
                    const SbVec3f *normals,
                    const int32_t *normindices,
                    SoMaterialBundle *const materials,
                    const int32_t *matindices,
                    SoTextureCoordinateBundle * const texcoords,
                    const int32_t *texindices,
                    SoVertexAttributeBundle * const attribs,
                    const int nbind,
                    const int mbind,
                    const int attribbind,
                    const int dotexture,
                    const int doattribs)
{
  SOGL_FACESET_GLRENDER(nbind, mbind, attribbind, (vertexlist,
                                                   vertexindices,
                                                   num_vertexindices,
                                                   normals,
                                                   normindices,
                                                   materials,
                                                   matindices,
                                                   texcoords,
                                                   texindices,
                                                   attribs,
                                                   dotexture,
                                                   doattribs));
}

#undef SOGL_FACESET_GLRENDER
#undef SOGL_FACESET_GLRENDER_RESOLVE_ARG1
#undef SOGL_FACESET_GLRENDER_RESOLVE_ARG2
#undef SOGL_FACESET_GLRENDER_RESOLVE_ARG3
#undef SOGL_FACESET_GLRENDER_RESOLVE_ARG4
#undef SOGL_FACESET_GLRENDER_CALL_FUNC

#endif // !NO_FACESET_RENDER

// **************************************************************************

//typedef void sogl_render_faceset_func(const SoGLCoordinateElement * const coords,
//                                      const int32_t *vertexindices,
//                                      int num_vertexindices,
//                                      const SbVec3f *normals,
//                                      const int32_t *normindices,
//                                      SoMaterialBundle *materials,
//                                      const int32_t *matindices,
//                                      const SoTextureCoordinateBundle * const texcoords,
//                                      const int32_t *texindices);

#if !defined(NO_TRISTRIPSET_RENDER)

namespace { namespace SoGL { namespace TriStripSet {

  enum AttributeBinding {
    OVERALL = 0,
    PER_STRIP = 1,
    PER_STRIP_INDEXED = 2,
    PER_TRIANGLE = 3,
    PER_TRIANGLE_INDEXED = 4,
    PER_VERTEX = 5,
    PER_VERTEX_INDEXED = 6
  };

  template < int NormalBinding,
             int MaterialBinding,
             int TexturingEnabled >
  static void GLRender(const SoGLCoordinateElement * const vertexlist,
                       const int32_t *vertexindices,
                       int numindices,
                       const SbVec3f *normals,
                       const int32_t *normalindices,
                       SoMaterialBundle *materials,
                       const int32_t *matindices,
                       const SoTextureCoordinateBundle * const texcoords,
                       const int32_t *texindices)
  {

    // just in case someone forgot...
    if (matindices == NULL) matindices = vertexindices;
    if (normalindices == NULL) normalindices = vertexindices;

    int texidx = 0;
    const int32_t *viptr = vertexindices;
    const int32_t *vistartptr = vertexindices;
    const int32_t *viendptr = viptr + numindices;
    int32_t v1, v2, v3;
    int numverts = vertexlist->getNum();

    const SbVec3f * coords3d = NULL;
    const SbVec4f * coords4d = NULL;
    const SbBool is3d = vertexlist->is3D();
    if (is3d) {
      coords3d = vertexlist->getArrayPtr3();
    }
    else {
      coords4d = vertexlist->getArrayPtr4();
    }

    // This is the same code as in SoGLCoordinateElement::send().
    // It is inlined here for speed (~15% speed increase).
#define SEND_VERTEX_TRISTRIP(_idx_) \
    if (is3d) glVertex3fv((const GLfloat*) (coords3d + _idx_)); \
    else glVertex4fv((const GLfloat*) (coords4d + _idx_));

    if ((AttributeBinding)NormalBinding == PER_VERTEX ||
        (AttributeBinding)NormalBinding == PER_TRIANGLE ||
        (AttributeBinding)NormalBinding == PER_STRIP ||
        (AttributeBinding)NormalBinding == PER_VERTEX_INDEXED ||
        (AttributeBinding)NormalBinding == PER_TRIANGLE_INDEXED ||
        (AttributeBinding)NormalBinding == PER_STRIP_INDEXED) {
      assert(normals && "Aborting rendering of tristrip; got NULL normals");
    }

    SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
    const SbVec3f *currnormal = &dummynormal;

    if ((AttributeBinding)NormalBinding == PER_VERTEX ||
        (AttributeBinding)NormalBinding == PER_TRIANGLE ||
        (AttributeBinding)NormalBinding == PER_STRIP ||
        (AttributeBinding)NormalBinding == PER_VERTEX_INDEXED ||
        (AttributeBinding)NormalBinding == PER_TRIANGLE_INDEXED ||
        (AttributeBinding)NormalBinding == PER_STRIP_INDEXED ||
        TexturingEnabled == TRUE) {
      if (normals) currnormal = normals;
    }

    int matnr = 0;

    while (viptr + 2 < viendptr) {
      v1 = *viptr++;
      v2 = *viptr++;
      v3 = *viptr++;

      // This should be here to prevent illegal polygons from being rendered
      if (v1 < 0 || v2 < 0 || v3 < 0 ||
          v1 >= numverts || v2 >= numverts || v3 >= numverts) {

        static uint32_t current_errors = 0;
        if (current_errors < 1) {
          SoDebugError::postWarning("[tristrip]::GLRender", "Erroneous polygon detected. "
                                    "Ignoring (offset: %d, [%d %d %d]). Should be within "
                                    " [0, %d] This message will only be shown once, but "
                                    "more errors may be present",
                                    viptr - vistartptr - 3, v1, v2, v3, numverts - 1);
        }

        current_errors++;
        break;
      }

      glBegin(GL_TRIANGLE_STRIP);

      /* vertex 1 *********************************************************/
      if ((AttributeBinding)MaterialBinding == PER_VERTEX ||
          (AttributeBinding)MaterialBinding == PER_STRIP ||
          (AttributeBinding)MaterialBinding == PER_TRIANGLE) {
        materials->send(matnr++, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED ||
                 (AttributeBinding)MaterialBinding == PER_STRIP_INDEXED ||
                 (AttributeBinding)MaterialBinding == PER_TRIANGLE_INDEXED) {
        materials->send(*matindices++, TRUE);
      }
      if ((AttributeBinding)NormalBinding == PER_VERTEX ||
          (AttributeBinding)NormalBinding == PER_STRIP ||
          (AttributeBinding)NormalBinding == PER_TRIANGLE) {
        currnormal = normals++;
        glNormal3fv((const GLfloat*)currnormal);
      } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED ||
                 (AttributeBinding)NormalBinding == PER_TRIANGLE_INDEXED ||
                 (AttributeBinding)NormalBinding == PER_STRIP_INDEXED) {
        currnormal = &normals[*normalindices++];
        glNormal3fv((const GLfloat*)currnormal);
      }
      if (TexturingEnabled == TRUE) {
        texcoords->send(texindices ? *texindices++ : texidx++,
                        vertexlist->get3(v1),
                        *currnormal);
      }
      SEND_VERTEX_TRISTRIP(v1);

      /* vertex 2 *********************************************************/
      if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
        materials->send(matnr++, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
        materials->send(*matindices++, TRUE);
      }

      // needed for nvidia color-per-face-bug workaround
      if ((AttributeBinding)MaterialBinding == PER_TRIANGLE ||
          (AttributeBinding)MaterialBinding == PER_STRIP) {
        materials->send(matnr-1, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_TRIANGLE_INDEXED ||
                 (AttributeBinding)MaterialBinding == PER_STRIP_INDEXED) {
        materials->send(matindices[-1], TRUE);
      }
      // end of nvidia workaround

      if ((AttributeBinding)NormalBinding == PER_VERTEX) {
        currnormal = normals++;
        glNormal3fv((const GLfloat*)currnormal);
      } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
        currnormal = &normals[*normalindices++];
        glNormal3fv((const GLfloat*)currnormal);
      }
      if (TexturingEnabled == TRUE) {
        texcoords->send(texindices ? *texindices++ : texidx++,
                        vertexlist->get3(v2),
                        *currnormal);
      }
      SEND_VERTEX_TRISTRIP(v2);

      /* vertex 3 *********************************************************/
      if ((AttributeBinding)MaterialBinding == PER_VERTEX) {
        materials->send(matnr++, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
        materials->send(*matindices++, TRUE);
      }

      // needed for nvidia color-per-face-bug workaround
      if ((AttributeBinding)MaterialBinding == PER_STRIP ||
          (AttributeBinding)MaterialBinding == PER_TRIANGLE) {
        materials->send(matnr-1, TRUE);
      } else if ((AttributeBinding)MaterialBinding == PER_TRIANGLE_INDEXED ||
                 (AttributeBinding)MaterialBinding == PER_STRIP_INDEXED) {
        materials->send(matindices[-1], TRUE);
      }
      // end of nvidia workaround

      if ((AttributeBinding)NormalBinding == PER_VERTEX) {
        currnormal = normals++;
        glNormal3fv((const GLfloat*)currnormal);
      } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
        currnormal = &normals[*normalindices++];
        glNormal3fv((const GLfloat*)currnormal);
      }
      if (TexturingEnabled == TRUE) {
        texcoords->send(texindices ? *texindices++ : texidx++,
                        vertexlist->get3(v3),
                        *currnormal);
      }
      SEND_VERTEX_TRISTRIP(v3);

      v1 = viptr < viendptr ? *viptr++ : -1;
      while (v1 >= 0) {
        if ((AttributeBinding)MaterialBinding == PER_VERTEX ||
            (AttributeBinding)MaterialBinding == PER_TRIANGLE) {
          materials->send(matnr++, TRUE);
        } else if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED ||
                   (AttributeBinding)MaterialBinding == PER_TRIANGLE_INDEXED) {
          materials->send(*matindices++, TRUE);
        }

        // needed for nvidia color-per-face-bug workaround
        if ((AttributeBinding)MaterialBinding == PER_STRIP) {
          materials->send(matnr-1, TRUE);
        } else if ((AttributeBinding)MaterialBinding == PER_STRIP_INDEXED) {
          materials->send(matindices[-1], TRUE);
        }
        // end of nvidia workaround

        if ((AttributeBinding)NormalBinding == PER_VERTEX ||
            (AttributeBinding)NormalBinding == PER_TRIANGLE) {
          currnormal = normals++;
          glNormal3fv((const GLfloat*)currnormal);
        } else if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED ||
                   (AttributeBinding)NormalBinding == PER_TRIANGLE_INDEXED) {
          currnormal = &normals[*normalindices++];
          glNormal3fv((const GLfloat*)currnormal);
        }
        if (TexturingEnabled == TRUE) {
          texcoords->send(texindices ? *texindices++ : texidx++,
                          vertexlist->get3(v1),
                          *currnormal);
        }

        SEND_VERTEX_TRISTRIP(v1);
        v1 = viptr < viendptr ? *viptr++ : -1;
      }
      glEnd(); // end of tristrip

      if ((AttributeBinding)MaterialBinding == PER_VERTEX_INDEXED) {
        matindices++;
      }
      if ((AttributeBinding)NormalBinding == PER_VERTEX_INDEXED) {
        normalindices++;
      }
      if (TexturingEnabled == TRUE) {
        if (texindices) texindices++;
      }
    }
  }

} } } // namespace

#define SOGL_TRISTRIPSET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, texturing, args) \
  SoGL::TriStripSet::GLRender<normalbinding, materialbinding, texturing> args

#define SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, materialbinding, texturing, args) \
  if (texturing) { \
    SOGL_TRISTRIPSET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, TRUE, args); \
  } else { \
    SOGL_TRISTRIPSET_GLRENDER_CALL_FUNC(normalbinding, materialbinding, FALSE, args); \
  }

#define SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(normalbinding, materialbinding, texturing, args) \
  switch (materialbinding) { \
  case SoGL::TriStripSet::OVERALL: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::TriStripSet::OVERALL, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_STRIP: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::TriStripSet::PER_STRIP, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_STRIP_INDEXED: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::TriStripSet::PER_STRIP_INDEXED, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_TRIANGLE: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::TriStripSet::PER_TRIANGLE, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_TRIANGLE_INDEXED: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::TriStripSet::PER_TRIANGLE_INDEXED, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_VERTEX: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::TriStripSet::PER_VERTEX, texturing, args); \
    break; \
  case SoGL::IndexedLineSet::PER_VERTEX_INDEXED: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3(normalbinding, SoGL::TriStripSet::PER_VERTEX_INDEXED, texturing, args); \
    break; \
  default: \
    assert(!"invalid material binding argument"); \
  }

#define SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args) \
  switch (normalbinding) { \
  case SoGL::TriStripSet::OVERALL: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(SoGL::TriStripSet::OVERALL, materialbinding, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_STRIP: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(SoGL::TriStripSet::PER_STRIP, materialbinding, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_STRIP_INDEXED: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(SoGL::TriStripSet::PER_STRIP_INDEXED, materialbinding, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_TRIANGLE: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(SoGL::TriStripSet::PER_TRIANGLE, materialbinding, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_TRIANGLE_INDEXED: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(SoGL::TriStripSet::PER_TRIANGLE_INDEXED, materialbinding, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_VERTEX: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(SoGL::TriStripSet::PER_VERTEX, materialbinding, texturing, args); \
    break; \
  case SoGL::TriStripSet::PER_VERTEX_INDEXED: \
    SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2(SoGL::TriStripSet::PER_VERTEX_INDEXED, materialbinding, texturing, args); \
    break; \
  default: \
    assert(!"invalid normal binding argument"); \
  }

#define SOGL_TRISTRIPSET_GLRENDER(normalbinding, materialbinding, texturing, args) \
  SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG1(normalbinding, materialbinding, texturing, args)

void
sogl_render_tristrip(const SoGLCoordinateElement * const vertexlist,
                     const int32_t *vertexindices,
                     int num_vertexindices,
                     const SbVec3f *normals,
                     const int32_t *normindices,
                     SoMaterialBundle *const materials,
                     const int32_t *matindices,
                     const SoTextureCoordinateBundle * const texcoords,
                     const int32_t *texindices,
                     const int nbind,
                     const int mbind,
                     const int texture)
{
  SOGL_TRISTRIPSET_GLRENDER(nbind, mbind, texture, (vertexlist,
                                                    vertexindices,
                                                    num_vertexindices,
                                                    normals,
                                                    normindices,
                                                    materials,
                                                    matindices,
                                                    texcoords,
                                                    texindices));
}

#undef SOGL_TRISTRIPSET_GLRENDER_CALL_FUNC
#undef SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG1
#undef SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG2
#undef SOGL_TRISTRIPSET_GLRENDER_RESOLVE_ARG3
#undef SOGL_TRISTRIPSET_GLRENDER

#endif // !NO_TRISTRIPSET_RENDER


// PointSet rendering
// here we include the 8 variations directly...

static void
sogl_render_pointset_m0n0t0(const SoGLCoordinateElement * coords,
                            const SbVec3f * COIN_UNUSED_ARG(normals),
                            SoMaterialBundle * COIN_UNUSED_ARG(mb),
                            const SoTextureCoordinateBundle * COIN_UNUSED_ARG(tb),
                            int32_t numpts,
                            int32_t idx)
{
  int i;
  const int unroll = numpts >> 2;
  const int rest = numpts & 3;

  // manually unroll this common loop

  glBegin(GL_POINTS);
  for (i = 0; i < unroll; i++) {
    coords->send(idx++);
    coords->send(idx++);
    coords->send(idx++);
    coords->send(idx++);
  }
  for (i = 0; i < rest; i++) {
    coords->send(idx++);
  }
  glEnd();
}

static void
sogl_render_pointset_m0n0t1(const SoGLCoordinateElement * coords,
                            const SbVec3f * COIN_UNUSED_ARG(normals),
                            SoMaterialBundle * COIN_UNUSED_ARG(mb),
                            const SoTextureCoordinateBundle * tb,
                            int32_t numpts,
                            int32_t idx)
{
  int texnr = 0;
  const SbVec3f currnormal(0.0f,0.0f,1.0f);

  glBegin(GL_POINTS);
  for (int i = 0; i < numpts; i++) {
    tb->send(texnr++, coords->get3(idx), currnormal);
    coords->send(idx++);
  }
  glEnd();
}

static void
sogl_render_pointset_m0n1t0(const SoGLCoordinateElement * coords,
                            const SbVec3f * normals,
                            SoMaterialBundle * COIN_UNUSED_ARG(mb),
                            const SoTextureCoordinateBundle * COIN_UNUSED_ARG(tb),
                            int32_t numpts,
                            int32_t idx)
{
  glBegin(GL_POINTS);
  for (int i = 0; i < numpts; i++) {
    glNormal3fv((const GLfloat*)normals++);
    coords->send(idx++);
  }
  glEnd();
}

static void
sogl_render_pointset_m0n1t1(const SoGLCoordinateElement * coords,
                            const SbVec3f * normals,
                            SoMaterialBundle * COIN_UNUSED_ARG(mb),
                            const SoTextureCoordinateBundle * tb,
                            int32_t numpts,
                            int32_t idx)
{
  int texnr = 0;
  const SbVec3f currnormal(0.0f,0.0f,1.0f);

  glBegin(GL_POINTS);
  for (int i = 0; i < numpts; i++) {
    glNormal3fv((const GLfloat*)normals++);
    tb->send(texnr++, coords->get3(idx), currnormal);
    coords->send(idx++);
  }
  glEnd();
}

static void
sogl_render_pointset_m1n0t0(const SoGLCoordinateElement * coords,
                            const SbVec3f * COIN_UNUSED_ARG(normals),
                            SoMaterialBundle * mb,
                            const SoTextureCoordinateBundle * COIN_UNUSED_ARG(tb),
                            int32_t numpts,
                            int32_t idx)
{
  int i;
  int matnr = 0;
  const int unroll = numpts >> 2;
  const int rest = numpts & 3;

  // manually unroll this common loop

  glBegin(GL_POINTS);
  for (i = 0; i < unroll; i++) {
    mb->send(matnr++, TRUE);
    coords->send(idx++);
    mb->send(matnr++, TRUE);
    coords->send(idx++);
    mb->send(matnr++, TRUE);
    coords->send(idx++);
    mb->send(matnr++, TRUE);
    coords->send(idx++);
  }
  for (i = 0; i < rest; i++) {
    mb->send(matnr++, TRUE);
    coords->send(idx++);
  }
  glEnd();
}

static void
sogl_render_pointset_m1n0t1(const SoGLCoordinateElement * coords,
                            const SbVec3f * COIN_UNUSED_ARG(normals),
                            SoMaterialBundle * mb,
                            const SoTextureCoordinateBundle * tb,
                            int32_t numpts,
                            int32_t idx)
{
  int matnr = 0;
  int texnr = 0;
  const SbVec3f currnormal(0.0f,0.0f,1.0f);

  glBegin(GL_POINTS);
  for (int i = 0; i < numpts; i++) {
    mb->send(matnr++, TRUE);
    tb->send(texnr++, coords->get3(idx), currnormal);
    coords->send(idx++);
  }
  glEnd();
}

static void
sogl_render_pointset_m1n1t0(const SoGLCoordinateElement * coords,
                            const SbVec3f * normals,
                            SoMaterialBundle * mb,
                            const SoTextureCoordinateBundle * COIN_UNUSED_ARG(tb),
                            int32_t numpts,
                            int32_t idx)
{
  int matnr = 0;

  glBegin(GL_POINTS);
  for (int i = 0; i < numpts; i++) {
    mb->send(matnr++, TRUE);
    glNormal3fv((const GLfloat*)normals++);
    coords->send(idx++);
  }
  glEnd();
}

static void
sogl_render_pointset_m1n1t1(const SoGLCoordinateElement * coords,
                            const SbVec3f * normals,
                            SoMaterialBundle * mb,
                            const SoTextureCoordinateBundle * tb,
                            int32_t numpts,
                            int32_t idx)
{
  int texnr = 0;
  int matnr = 0;

  glBegin(GL_POINTS);
  for (int i = 0; i < numpts; i++) {
    mb->send(matnr++, TRUE);
    tb->send(texnr++, coords->get3(idx), *normals);
    glNormal3fv((const GLfloat*)normals++);
    coords->send(idx++);
  }
  glEnd();
}

// ---

typedef void sogl_render_pointset_func(const SoGLCoordinateElement * coords,
                                       const SbVec3f * normals,
                                       SoMaterialBundle * mb,
                                       const SoTextureCoordinateBundle * tb,
                                       int32_t numpts,
                                       int32_t idx);

static sogl_render_pointset_func * sogl_render_pointset_funcs[8];

void
sogl_render_pointset(const SoGLCoordinateElement * coords,
                     const SbVec3f * normals,
                     SoMaterialBundle * mb,
                     const SoTextureCoordinateBundle * tb,
                     int32_t numpts,
                     int32_t idx)
{
  static int first = 1;
  if (first) {
    sogl_render_pointset_funcs[0] = sogl_render_pointset_m0n0t0;
    sogl_render_pointset_funcs[1] = sogl_render_pointset_m0n0t1;
    sogl_render_pointset_funcs[2] = sogl_render_pointset_m0n1t0;
    sogl_render_pointset_funcs[3] = sogl_render_pointset_m0n1t1;
    sogl_render_pointset_funcs[4] = sogl_render_pointset_m1n0t0;
    sogl_render_pointset_funcs[5] = sogl_render_pointset_m1n0t1;
    sogl_render_pointset_funcs[6] = sogl_render_pointset_m1n1t0;
    sogl_render_pointset_funcs[7] = sogl_render_pointset_m1n1t1;
    first = 0;
  }

  int mat = mb ? 1 : 0;
  int norm = normals ? 1 : 0;
  int tex = tb ? 1 : 0;

  sogl_render_pointset_funcs[ (mat << 2) | (norm << 1) | tex ]
    ( coords,
      normals,
      mb,
      tb,
      numpts,
      idx);
}

// Used by library code to decide whether or not to add extra
// debugging checks for glGetError().
SbBool
sogl_glerror_debugging(void)
{
  static int COIN_GLERROR_DEBUGGING = -1;
  if (COIN_GLERROR_DEBUGGING == -1) {
    const char * str = coin_getenv("COIN_GLERROR_DEBUGGING");
    COIN_GLERROR_DEBUGGING = str ? atoi(str) : 0;
  }
  return (COIN_GLERROR_DEBUGGING == 0) ? FALSE : TRUE;
}

static int SOGL_AUTOCACHE_REMOTE_MIN = 500000;
static int SOGL_AUTOCACHE_REMOTE_MAX = 5000000;
static int SOGL_AUTOCACHE_LOCAL_MIN = 100000;
static int SOGL_AUTOCACHE_LOCAL_MAX = 1000000;
static int SOGL_AUTOCACHE_VBO_LIMIT = 65536;

/*!
  Called by each shape during rendering. Will enable/disable auto caching
  based on the number of primitives.
*/
void
sogl_autocache_update(SoState * state, const int numprimitives, SbBool didusevbo)
{
  static SbBool didtestenv = FALSE;
  if (!didtestenv) {
    const char * env;
    env = coin_getenv("COIN_AUTOCACHE_REMOTE_MIN");
    if (env) {
      SOGL_AUTOCACHE_REMOTE_MIN = atoi(env);
    }
    env = coin_getenv("COIN_AUTOCACHE_REMOTE_MAX");
    if (env) {
      SOGL_AUTOCACHE_REMOTE_MAX = atoi(env);
    }
    env = coin_getenv("COIN_AUTOCACHE_LOCAL_MIN");
    if (env) {
      SOGL_AUTOCACHE_LOCAL_MIN = atoi(env);
    }
    env = coin_getenv("COIN_AUTOCACHE_LOCAL_MAX");
    if (env) {
      SOGL_AUTOCACHE_LOCAL_MAX = atoi(env);
    }
    env = coin_getenv("COIN_AUTOCACHE_VBO_LIMIT");
    if (env) {
      SOGL_AUTOCACHE_VBO_LIMIT = atoi(env);
    }
    didtestenv = TRUE;
  }

  int minval = SOGL_AUTOCACHE_LOCAL_MIN;
  int maxval = SOGL_AUTOCACHE_LOCAL_MAX;
  if (SoGLCacheContextElement::getIsRemoteRendering(state)) {
    minval = SOGL_AUTOCACHE_REMOTE_MIN;
    maxval = SOGL_AUTOCACHE_REMOTE_MAX;
  }
  if (numprimitives <= minval) {
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DO_AUTO_CACHE);
  }
  else if (numprimitives >= maxval) {
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
  }
  SoGLCacheContextElement::incNumShapes(state);

  if (didusevbo) {
    // avoid creating caches when rendering large VBOs
    if (numprimitives > SOGL_AUTOCACHE_VBO_LIMIT) {
      SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
    }
  }
}

// **************************************************************************

static SoOffscreenRenderer * offscreenrenderer = NULL;
static SoCallback * offscreencallback = NULL;

static void offscreenrenderer_cleanup(void)
{
  offscreencallback->unref();
  delete offscreenrenderer;
  offscreenrenderer = NULL;
  offscreencallback = NULL;
}

// This is really obsoleted now that we have
// cc_glglue_context_create_offscreen() et al in the OpenGL
// wrapper. So don't use this function any more from within Coin.
//
// FIXME: must be kept around due to ABI & API compatibility reasons
// for now, but should consider taking it out for the next major Coin
// release.
//
// 20030519 mortene.
//
// SoGL API is private so this function can be taken out any time
// 20090426 pederb

void
sogl_offscreencontext_callback(void (*cb)(void *, SoAction*),
                               void * closure)
{
  if (offscreenrenderer == NULL) {
    offscreenrenderer = new SoOffscreenRenderer(SbViewportRegion(32, 32));
    offscreencallback = new SoCallback;
    offscreencallback->ref();
    coin_atexit((coin_atexit_f*) offscreenrenderer_cleanup, CC_ATEXIT_NORMAL);
  }
  offscreencallback->setCallback(cb, closure);
  offscreenrenderer->render(offscreencallback);
}
