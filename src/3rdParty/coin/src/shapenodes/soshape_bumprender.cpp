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

#include "shapenodes/soshape_bumprender.h"

#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/C/glue/gl.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/elements/SoBumpMapElement.h>
#include <Inventor/elements/SoBumpMapMatrixElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLDisplayList.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoGLImage.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <Inventor/caches/SoPrimitiveVertexCache.h>

// For coin_apply_normalization_cube_map().
#include "glue/glp.h"
#include "rendering/SoGL.h"

// *************************************************************************

// Fragment program for bumpmapping
static const char * bumpspecfpprogram =
"!!ARBfp1.0\n"
"PARAM u0 = program.env[0];\n" // Specular color (3 floats)
"PARAM u1 = program.env[1];\n" // Shininess color (1 float)
"PARAM c0 = {2, 0.5, 0, 0};\n"
"TEMP R0;\n"
"TEMP R1;\n"
" TEX R0.xyz, fragment.texcoord[0], texture[0], 2D;\n"
" ADD R0.xyz, R0, -c0.y;\n"
" MUL R0.xyz, R0, c0.x;\n"
" MOV R1.xyz, fragment.texcoord[2];\n"
" ADD R1.xyz, fragment.texcoord[1], R1;\n"
" DP3 R0.w, R1, R1;\n"
" RSQ R0.w, R0.w;\n"
" MUL R1.xyz, R0.w, R1;\n"
" TEX R1.xyz, R1, texture[1], CUBE;\n"
" ADD R1.xyz, R1, -c0.y;\n"
" MUL R1.xyz, R1, c0.x;\n"
" DP3_SAT R0.x, R0, R1;\n"
" POW R0.x, R0.x, u1.x;\n"
" MUL result.color, u0, R0.x;\n"
"END\n";

// Vertex program for directional lights
static const char * directionallightvpprogram =
"!!ARBvp1.0\n"
"TEMP R0;\n"
"ATTRIB v26 = vertex.texcoord[2];\n"
"ATTRIB v25 = vertex.texcoord[1];\n"
"ATTRIB v24 = vertex.texcoord[0];\n"
"ATTRIB v18 = vertex.normal;\n"
"ATTRIB v16 = vertex.position;\n"
"PARAM c1 = program.env[1];\n"
"PARAM c0 = program.env[0];\n"
"PARAM c6[4] = { state.matrix.texture[0] };\n"
"PARAM c2[4] = { state.matrix.mvp };\n"
" DPH result.position.x, v16.xyzz, c2[0];\n"
" DPH result.position.y, v16.xyzz, c2[1];\n"
" DPH result.position.z, v16.xyzz, c2[2];\n"
" DPH result.position.w, v16.xyzz, c2[3];\n"
" MUL R0.xy, c6[0].xyxx, v24.xyxx;\n"
" ADD result.texcoord[0].x, R0.x, R0.y;\n"
" MUL R0.xy, c6[1].xyxx, v24.xyxx;\n"
" ADD result.texcoord[0].y, R0.x, R0.y;\n"
" DP3 result.texcoord[1].x, v25.xyzx, c0.xyzx;\n"
" DP3 result.texcoord[1].y, v26.xyzx, c0.xyzx;\n"
" DP3 result.texcoord[1].z, v18.xyzx, c0.xyzx;\n"
" ADD R0.yzw, c1.xxyz, -v16.xxyz;\n"
" DP3 R0.x, R0.yzwy, R0.yzwy;\n"
" RSQ R0.x, R0.x;\n"
" MUL R0.xyz, R0.x, R0.yzwy;\n"
" DP3 result.texcoord[2].x, v25.xyzx, R0.xyzx;\n"
" DP3 result.texcoord[2].y, v26.xyzx, R0.xyzx;\n"
" DP3 result.texcoord[2].z, v18.xyzx, R0.xyzx;\n"
"END\n";


// Vertex program for point lights
static const char * pointlightvpprogram =
"!!ARBvp1.0\n"
"TEMP R0;\n"
"ATTRIB v26 = vertex.texcoord[2];\n"
"ATTRIB v25 = vertex.texcoord[1];\n"
"ATTRIB v24 = vertex.texcoord[0];\n"
"ATTRIB v18 = vertex.normal;\n"
"ATTRIB v16 = vertex.position;\n"
"PARAM c1 = program.env[1];\n" // Light position
"PARAM c0 = program.env[0];\n" // Eye position
"PARAM c2[4] = { state.matrix.mvp };\n"
"PARAM c6[4] = { state.matrix.texture[0] };\n"
" DPH result.position.x, v16.xyzz, c2[0];\n"
" DPH result.position.y, v16.xyzz, c2[1];\n"
" DPH result.position.z, v16.xyzz, c2[2];\n"
" DPH result.position.w, v16.xyzz, c2[3];\n"
" MUL R0.xy, c6[0].xyxx, v24.xyxx;\n"
" ADD result.texcoord[0].x, R0.x, R0.y;\n"
" MUL R0.xy, c6[1].xyxx, v24.xyxx;\n"
" ADD result.texcoord[0].y, R0.x, R0.y;\n"
" ADD R0.yzw, c0.xxyz, -v16.xxyz;\n"
" DP3 R0.x, R0.yzwy, R0.yzwy;\n"
" RSQ R0.x, R0.x;\n"
" MUL R0.xyz, R0.x, R0.yzwy;\n"
" DP3 result.texcoord[1].x, v25.xyzx, R0.xyzx;\n"
" DP3 result.texcoord[1].y, v26.xyzx, R0.xyzx;\n"
" DP3 result.texcoord[1].z, v18.xyzx, R0.xyzx;\n"
" ADD R0.yzw, c1.xxyz, -v16.xxyz;\n"
" DP3 R0.x, R0.yzwy, R0.yzwy;\n"
" RSQ R0.x, R0.x;\n"
" MUL R0.xyz, R0.x, R0.yzwy;\n"
" DP3 result.texcoord[2].x, v25.xyzx, R0.xyzx;\n"
" DP3 result.texcoord[2].y, v26.xyzx, R0.xyzx;\n"
" DP3 result.texcoord[2].z, v18.xyzx, R0.xyzx;\n"
"END\n";

// vertex program for bumpmapping (calculate tsb coordinates for
// texture unit 1)
static const char * diffusebumpdirlightvpprogram =
"!!ARBvp1.0\n"
"TEMP R0;\n"
"PARAM c5 = { 1, 0, 2, 0 };\n"
"PARAM color = { 1, 1, 1, 1 };\n"
"ATTRIB v19 = vertex.color;\n"
"ATTRIB v25 = vertex.texcoord[1];\n"
"ATTRIB v24 = vertex.texcoord[0];\n"
"ATTRIB v18 = vertex.normal;\n"
"ATTRIB v16 = vertex.position;\n"
"PARAM c0 = program.env[0];\n"
"PARAM c1[4] = { state.matrix.mvp };\n"
"PARAM c6[4] = { state.matrix.texture[0] };\n"
" MUL R0.xy, c6[0].xyxx, v24.xyxx;\n"
" ADD result.texcoord[0].x, R0.x, R0.y;\n"
" MUL R0.xy, c6[1].xyxx, v24.xyxx;\n"
" ADD result.texcoord[0].y, R0.x, R0.y;\n"
" MOV result.color, color;\n"
" DPH result.position.x, v16.xyzz, c1[0];\n"
" DPH result.position.y, v16.xyzz, c1[1];\n"
" DPH result.position.z, v16.xyzz, c1[2];\n"
" DPH result.position.w, v16.xyzz, c1[3];\n"
" DP3 result.texcoord[1].x, v25.xyzx, c0.xyzx;\n"
" DP3 result.texcoord[1].y, v19.xyzx, c0.xyzx;\n"
" DP3 result.texcoord[1].z, v18.xyzx, c0.xyzx;\n"
"END\n";

// vertex program for normal rendering. Needed to get exactly the same
// z-buffer value for each vertex
static const char * normalrenderingvpprogram =
"!!ARBvp1.0\n"
"TEMP R0;\n"
"ATTRIB v19 = vertex.color;\n"
"ATTRIB v16 = vertex.position;\n"
"ATTRIB v25 = vertex.texcoord[1];\n"
"ATTRIB v24 = vertex.texcoord[0];\n"
"PARAM c6[4] = { state.matrix.texture[0] };\n"
"PARAM c7[4] = { state.matrix.texture[1] };\n"
"PARAM c1[4] = { state.matrix.mvp };\n"
" DPH result.position.x, v16.xyzz, c1[0];\n"
" DPH result.position.y, v16.xyzz, c1[1];\n"
" DPH result.position.z, v16.xyzz, c1[2];\n"
" DPH result.position.w, v16.xyzz, c1[3];\n"
" MUL R0.xy, c6[0].xyxx, v24.xyxx;\n"
" ADD result.texcoord[0].x, R0.x, R0.y;\n"
" MUL R0.xy, c6[1].xyxx, v24.xyxx;\n"
" ADD result.texcoord[0].y, R0.x, R0.y;\n"
" MUL R0.xy, c7[0].xyxx, v25.xyxx;\n"
" ADD result.texcoord[1].x, R0.x, R0.y;\n"
" MUL R0.xy, c7[1].xyxx, v25.xyxx;\n"
" ADD result.texcoord[1].y, R0.x, R0.y;\n"
" MOV result.color, v19;\n"
"END\n";

// *************************************************************************

SbBool bumphack = TRUE;

// *************************************************************************

static void
soshape_bumprender_diffuseprogramdeletion(unsigned long COIN_UNUSED_ARG(key), void * COIN_UNUSED_ARG(value))
{
#if 0 // FIXME: cleanup routines not implemented yet (for no good
      // reason, really). 20050524 mortene.
  diffuse_programidx * pidx = (diffuse_programidx *) value;
  /* FIXME: There are no pointlight program initialized for diffuse
     rendering yet. Enable when implemented. (20040209 handegar) */
  //pidx->glue->glDeleteProgramsARB(1, &pidx->pointlight);
  cc_glglue_glDeletePrograms(pidx->glue, 1, &pidx->dirlight);
  cc_glglue_glDeletePrograms(pidx->glue, 1, &pidx->normalrendering);
#endif // FIXME
}

static void
soshape_bumprender_specularprogramdeletion(unsigned long COIN_UNUSED_ARG(key), void * COIN_UNUSED_ARG(value))
{
#if 0 // FIXME: cleanup routines not implemented yet (for no good
      // reason, really). 20050524 mortene.
  spec_programidx * pidx = (spec_programidx *) value;
  cc_glglue_glDeletePrograms(pidx->glue, 1, &pidx->pointlight);
  cc_glglue_glDeletePrograms(pidx->glue, 1, &pidx->dirlight);
  cc_glglue_glDeletePrograms(pidx->glue, 1, &pidx->fragment);
#endif // FIXME
}

soshape_bumprender::soshape_bumprender(void)
{
  this->diffuseprogramsinitialized = FALSE;
  this->programsinitialized = FALSE;
}

soshape_bumprender::~soshape_bumprender()
{

  // FIXME: Cannot delete programs just yet, as we dont know if the
  // context was valid or not. We must wait for new functionality to be
  // implemented for the context element code. (20040209 handegar)
  //this->diffuseprogramdict.applyToAll(soshape_bumprender_diffuseprogramdeletion);
  //this->specularprogramdict.applyToAll(soshape_bumprender_specularprogramdeletion);
}

// to avoid warnings from SbVec3f::normalize()
inline void NORMALIZE(SbVec3f &v)
{
  float len = v.length();
  if (len) {
    len = 1.0f / len;
    v[0] *= len;
    v[1] *= len;
    v[2] *= len;
  }
}

void
soshape_bumprender::initDiffusePrograms(const cc_glglue * glue, SoState * state)
{
  const int contextid = SoGLCacheContextElement::get(state);
  diffuse_programidx * old;
  if (this->diffuseprogramdict.get(contextid, old)) {
    this->diffusebumpdirlightvertexprogramid = old->dirlight;
    this->normalrenderingvertexprogramid = old->normalrendering;
  }
  else {

    cc_glglue_glGenPrograms(glue, 1, &this->diffusebumpdirlightvertexprogramid);
    cc_glglue_glBindProgram(glue, GL_VERTEX_PROGRAM_ARB, this->diffusebumpdirlightvertexprogramid);
    cc_glglue_glProgramString(glue, GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              (GLsizei)strlen(diffusebumpdirlightvpprogram),
                              diffusebumpdirlightvpprogram);
    GLint errorPos;
    GLenum err = glGetError();

    if (err != GL_NO_ERROR) {
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
      SoDebugError::postWarning("soshape_bumpspecrender::initPrograms",
                                "Error in diffuse dirlight vertex program! (byte pos: %d) '%s'.\n",
                                errorPos, glGetString(GL_PROGRAM_ERROR_STRING_ARB));

    }

    cc_glglue_glGenPrograms(glue, 1, &this->normalrenderingvertexprogramid);
    cc_glglue_glBindProgram(glue, GL_VERTEX_PROGRAM_ARB, this->normalrenderingvertexprogramid);
    cc_glglue_glProgramString(glue, GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              (GLsizei)strlen(normalrenderingvpprogram),
                              normalrenderingvpprogram);
    err = glGetError();

    if (err != GL_NO_ERROR) {
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
      SoDebugError::postWarning("soshape_bumpspecrender::initPrograms",
                                "Error in normal rendering vertex program! (byte pos: %d) '%s'.\n",
                                errorPos, glGetString(GL_PROGRAM_ERROR_STRING_ARB));

    }

    diffuse_programidx * newstruct = new diffuse_programidx;
    newstruct->glue = glue; // Store the cc_glglue for later when class is to be destructed.
    newstruct->dirlight = this->diffusebumpdirlightvertexprogramid;
    newstruct->pointlight = 0; // Pointlight vertex program not implemented yet.
    newstruct->normalrendering = this->normalrenderingvertexprogramid;

    (void) this->diffuseprogramdict.put(contextid, newstruct);

  }

  this->diffuseprogramsinitialized = TRUE;
}

void
soshape_bumprender::initPrograms(const cc_glglue * glue, SoState * state)
{
  const int contextid = SoGLCacheContextElement::get(state);
  spec_programidx * old;
  if (this->specularprogramdict.get(contextid, old)) {
    this->fragmentprogramid = old->fragment;
    this->dirlightvertexprogramid = old->dirlight;
    this->pointlightvertexprogramid = old->pointlight;
  }
  else {
    cc_glglue_glGenPrograms(glue, 1, &this->fragmentprogramid); // -- Fragment program
    cc_glglue_glBindProgram(glue, GL_FRAGMENT_PROGRAM_ARB, this->fragmentprogramid);
    cc_glglue_glProgramString(glue, GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              (GLsizei)strlen(bumpspecfpprogram), bumpspecfpprogram);
    // FIXME: Maybe a wrapper for catching fragment program errors
    // should be a part of GLUE... (20031204 handegar)
    GLint errorPos;
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
      SoDebugError::postWarning("soshape_bumpspecrender::initPrograms",
                                "Error in fragment program! (byte pos: %d) '%s'.\n",
                                errorPos, glGetString(GL_PROGRAM_ERROR_STRING_ARB));

    }

    cc_glglue_glGenPrograms(glue, 1, &this->dirlightvertexprogramid); // -- Directional light program
    cc_glglue_glBindProgram(glue, GL_VERTEX_PROGRAM_ARB, this->dirlightvertexprogramid);
    cc_glglue_glProgramString(glue, GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              (GLsizei)strlen(directionallightvpprogram), directionallightvpprogram);

    err = glGetError();
    if (err != GL_NO_ERROR) {
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
      SoDebugError::postWarning("soshape_bumpspecrender::initPrograms",
                                "Error in directional light vertex program! "
                                "(byte pos: %d) '%s'.\n",
                                errorPos, glGetString(GL_PROGRAM_ERROR_STRING_ARB));

    }

    cc_glglue_glGenPrograms(glue, 1, &this->pointlightvertexprogramid); // -- Point light program
    cc_glglue_glBindProgram(glue, GL_VERTEX_PROGRAM_ARB, this->pointlightvertexprogramid);
    cc_glglue_glProgramString(glue, GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              (GLsizei)strlen(pointlightvpprogram), pointlightvpprogram);

    err = glGetError();
    if (err != GL_NO_ERROR) {
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
      SoDebugError::postWarning("soshape_bumpspecrender::initPrograms",
                                "Error in point light vertex program! (byte pos: %d) '%s'.\n",
                                errorPos, glGetString(GL_PROGRAM_ERROR_STRING_ARB));

    }

    spec_programidx * newstruct = new spec_programidx;
    newstruct->glue = glue; // Store the cc_glglue for later when class is to be destructed.
    newstruct->fragment = this->fragmentprogramid;
    newstruct->dirlight = this->dirlightvertexprogramid;
    newstruct->pointlight = this->pointlightvertexprogramid;

    (void) this->specularprogramdict.put(contextid, newstruct);
  }

  this->programsinitialized = TRUE;
}

void
soshape_bumprender::renderBumpSpecular(SoState * state,
                                       const SoPrimitiveVertexCache * cache,
                                       SoLight * light, const SbMatrix & toobjectspace)
{

  //
  // A check for fragment- and vertex-program support has already
  // been done in SoShape::shouldGLRender().
  //
  const int n = cache->getNumTriangleIndices();
  if (n == 0) return;

  const cc_glglue * glue = sogl_glue_instance(state);
  const SbColor spec = SoLazyElement::getSpecular(state);
  float shininess = SoLazyElement::getShininess(state);

  if (!this->programsinitialized)
    this->initPrograms(glue, state);

  this->initLight(light, toobjectspace);

  const SbMatrix & oldtexture0matrix = SoMultiTextureMatrixElement::get(state, 0);
  const SbMatrix & oldtexture1matrix = SoMultiTextureMatrixElement::get(state, 1);
  const SbMatrix & oldtexture2matrix = SoMultiTextureMatrixElement::get(state, 2);
  const SbMatrix & bumpmapmatrix = SoBumpMapMatrixElement::get(state);

  int lastenabled;
  const SbBool * enabled = 
    SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled); 

  state->push();
  SoMultiTextureEnabledElement::disableAll(state);
  SoGLMultiTextureEnabledElement::set(state, NULL, 0, TRUE); // enable GL_TEXTURE_2D
  
  SoGLImage * bumpimage = SoBumpMapElement::get(state);
  assert(bumpimage);
  // set up textures
  cc_glglue_glActiveTexture(glue, GL_TEXTURE0);

  if (bumpmapmatrix != oldtexture0matrix) {
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(bumpmapmatrix[0]);
    glMatrixMode(GL_MODELVIEW);
  }
  
  bumpimage->getGLDisplayList(state)->call(state);

  // FRAGMENT: Setting up spec. colour and shininess for the fragment program
  glEnable(GL_FRAGMENT_PROGRAM_ARB);
  cc_glglue_glBindProgram(glue, GL_FRAGMENT_PROGRAM_ARB, fragmentprogramid);
  cc_glglue_glProgramEnvParameter4f(glue, GL_FRAGMENT_PROGRAM_ARB, 0,
                                    spec[0], spec[1], spec[2], 1.0f);

  cc_glglue_glProgramEnvParameter4f(glue, GL_FRAGMENT_PROGRAM_ARB, 1,
                                    shininess * 64, 0.0f, 0.0f, 1.0f);

  const SbViewVolume & vv = SoViewVolumeElement::get(state);
  //const SbMatrix & vm = SoViewingMatrixElement::get(state);

  SbVec3f eyepos = vv.getProjectionPoint();
  SoModelMatrixElement::get(state).inverse().multVecMatrix(eyepos, eyepos);

  // VERTEX: Setting up lightprograms
  glEnable(GL_VERTEX_PROGRAM_ARB);
  if (!this->ispointlight) {
    cc_glglue_glBindProgram(glue, GL_VERTEX_PROGRAM_ARB, dirlightvertexprogramid);
  }
  else {
    cc_glglue_glBindProgram(glue, GL_VERTEX_PROGRAM_ARB, pointlightvertexprogramid);
  }

  cc_glglue_glProgramEnvParameter4f(glue, GL_VERTEX_PROGRAM_ARB, 0,
                                    this->lightvec[0],
                                    this->lightvec[1],
                                    this->lightvec[2], 1);

  cc_glglue_glProgramEnvParameter4f(glue, GL_VERTEX_PROGRAM_ARB, 1,
                                    eyepos[0],
                                    eyepos[1],
                                    eyepos[2], 1);

  if (oldtexture2matrix != SbMatrix::identity()) {
    cc_glglue_glActiveTexture(glue, GL_TEXTURE2);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity(); // load identity texture matrix
    glMatrixMode(GL_MODELVIEW);
  }

  cc_glglue_glActiveTexture(glue, GL_TEXTURE1);
  if (oldtexture1matrix != SbMatrix::identity()) {
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity(); // load identity texture matrix
    glMatrixMode(GL_MODELVIEW);
  }
  coin_apply_normalization_cube_map(glue);
  glEnable(GL_TEXTURE_CUBE_MAP);

  cc_glglue_glActiveTexture(glue, GL_TEXTURE0);

  //const SbVec3f * cmptr = this->cubemaplist.getArrayPtr();
  const SbVec3f * tptr = this->tangentlist.getArrayPtr();
  
  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0,
                            (GLvoid*) cache->getVertexArray());
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);

  cc_glglue_glTexCoordPointer(glue, 2, GL_FLOAT, 0,
                              (GLvoid*) cache->getBumpCoordArray());
  cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

  cc_glglue_glNormalPointer(glue, GL_FLOAT, 0,
                           (GLvoid*) cache->getNormalArray());
  cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);

  cc_glglue_glClientActiveTexture(glue, GL_TEXTURE1);
  cc_glglue_glTexCoordPointer(glue, 3, GL_FLOAT, 6*sizeof(float), (GLvoid*) tptr);
  cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

  cc_glglue_glClientActiveTexture(glue, GL_TEXTURE2);
  cc_glglue_glTexCoordPointer(glue, 3, GL_FLOAT, 6*sizeof(float), (GLvoid*) (tptr + 1));
  cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

  cc_glglue_glDrawElements(glue, GL_TRIANGLES, n, GL_UNSIGNED_INT,
                           (const GLvoid*) cache->getTriangleIndices());

  cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
  cc_glglue_glClientActiveTexture(glue, GL_TEXTURE1);
  cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
  cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0);
  cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
  cc_glglue_glDisableClientState(glue, GL_VERTEX_ARRAY);
  cc_glglue_glDisableClientState(glue, GL_NORMAL_ARRAY);

  glDisable(GL_FRAGMENT_PROGRAM_ARB);
  glDisable(GL_VERTEX_PROGRAM_ARB);
  glDisable(GL_TEXTURE_CUBE_MAP); // unit 1

  if (lastenabled >= 1 && enabled[1]) {
    // restore blend mode for texture unit 1
    SoGLMultiTextureImageElement::restore(state, 1);
  }

  if (oldtexture2matrix != SbMatrix::identity()) {
    cc_glglue_glActiveTexture(glue, GL_TEXTURE2);
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(oldtexture2matrix[0]);
    glMatrixMode(GL_MODELVIEW);
  }

  cc_glglue_glActiveTexture(glue, GL_TEXTURE1);
  glDisable(GL_TEXTURE_CUBE_MAP);
  if (oldtexture1matrix != SbMatrix::identity()) {
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(oldtexture1matrix[0]);
    glMatrixMode(GL_MODELVIEW);
  }

  cc_glglue_glActiveTexture(glue, GL_TEXTURE0);

  if (bumpmapmatrix != oldtexture0matrix) {
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(oldtexture0matrix[0]);
    glMatrixMode(GL_MODELVIEW);
  }

  state->pop();
}


void
soshape_bumprender::renderBump(SoState * state,
                               const SoPrimitiveVertexCache * cache,
                               SoLight * light, const SbMatrix & toobjectspace)
{
  const int n = cache->getNumTriangleIndices();
  if (n == 0) return;

  this->initLight(light, toobjectspace);

  const cc_glglue * glue = sogl_glue_instance(state);
  const SbMatrix & oldtexture0matrix = SoMultiTextureMatrixElement::get(state, 0);
  const SbMatrix & oldtexture1matrix = SoMultiTextureMatrixElement::get(state, 1);
  const SbMatrix & bumpmapmatrix = SoBumpMapMatrixElement::get(state);
  
  int lastenabled;
  const SbBool * enabled = 
    SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled); 

  state->push();
  // only use vertex program if two texture units (or less) are used
  // (only two units supported in the vertex program)
  SbBool use_vertex_program = lastenabled <= 1 && SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_VERTEX_PROGRAM);
  use_vertex_program = FALSE; // FIXME: disabled until vertex program
                              // for point lights is implemented
  if (use_vertex_program) {
    if (!this->diffuseprogramsinitialized) {
      this->initDiffusePrograms(glue, state);
    }
  }
  else {
    // need to calculate tsb coordinates manually
    this->calcTSBCoords(cache, light);
  }
  SoMultiTextureEnabledElement::disableAll(state);
  SoMultiTextureEnabledElement::set(state, NULL, 0, TRUE);

  SoGLImage * bumpimage = SoBumpMapElement::get(state);
  assert(bumpimage);

  // set up textures
  cc_glglue_glActiveTexture(glue, GL_TEXTURE0);

  if (bumpmapmatrix != oldtexture0matrix) {
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(bumpmapmatrix[0]);
    glMatrixMode(GL_MODELVIEW);
  }

  bumpimage->getGLDisplayList(state)->call(state);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);

  cc_glglue_glActiveTexture(glue, GL_TEXTURE1);

  if (oldtexture1matrix != SbMatrix::identity()) {
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity(); // load identity texture matrix
    glMatrixMode(GL_MODELVIEW);
  }
  coin_apply_normalization_cube_map(glue);
  glEnable(GL_TEXTURE_CUBE_MAP);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);

  const SbVec3f * cmptr = this->cubemaplist.getArrayPtr();
  const SbVec3f * tsptr = this->tangentlist.getArrayPtr();

  if (!SoGLDriverDatabase::isSupported(glue, SO_GL_VBO_IN_DISPLAYLIST)) {
    SoCacheElement::invalidate(state);
    SoGLCacheContextElement::shouldAutoCache(state, 
                                             SoGLCacheContextElement::DONT_AUTO_CACHE);
  }

  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0,
                            (GLvoid*) cache->getVertexArray());
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);
  cc_glglue_glTexCoordPointer(glue, 2, GL_FLOAT, 0,
                              (GLvoid*) cache->getBumpCoordArray());
  cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

  cc_glglue_glClientActiveTexture(glue, GL_TEXTURE1);
  if (use_vertex_program) {
    cc_glglue_glColorPointer(glue, 3, GL_FLOAT, 6*sizeof(float),
                             (GLvoid*) (tsptr + 1));
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
    cc_glglue_glTexCoordPointer(glue, 3, GL_FLOAT, 6*sizeof(float),
                                (GLvoid*) tsptr);
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0,
                              (GLvoid*) cache->getNormalArray());
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }
  else {
    cc_glglue_glTexCoordPointer(glue, 3, GL_FLOAT, 0,
                                (GLvoid*) cmptr);
  }
  cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

  if (use_vertex_program) {
    glEnable(GL_VERTEX_PROGRAM_ARB);
    if (!this->ispointlight) {
      cc_glglue_glBindProgram(glue, GL_VERTEX_PROGRAM_ARB, diffusebumpdirlightvertexprogramid);
    }
    else {
      assert(0);
    }
    cc_glglue_glProgramEnvParameter4f(glue, GL_VERTEX_PROGRAM_ARB, 0,
                                      this->lightvec[0],
                                      this->lightvec[1],
                                      this->lightvec[2], 1);
  }

  cc_glglue_glDrawElements(glue, GL_TRIANGLES, n, GL_UNSIGNED_INT,
                           (const GLvoid*) cache->getTriangleIndices());

  if (use_vertex_program) {
    cc_glglue_glDisableClientState(glue, GL_NORMAL_ARRAY);
    cc_glglue_glDisableClientState(glue, GL_COLOR_ARRAY);
    glDisable(GL_VERTEX_PROGRAM_ARB);
  }
  cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
  cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0);
  cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
  cc_glglue_glDisableClientState(glue, GL_VERTEX_ARRAY);

  glDisable(GL_TEXTURE_CUBE_MAP); // unit 1

  if (lastenabled >= 1 && enabled[1]) {
    // restore blend mode for texture unit 1
    SoGLMultiTextureImageElement::restore(state, 1);
  }

  if (oldtexture1matrix != SbMatrix::identity()) {
    cc_glglue_glActiveTexture(glue, GL_TEXTURE1);
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(oldtexture1matrix[0]);
    glMatrixMode(GL_MODELVIEW);
  }

  cc_glglue_glActiveTexture(glue, GL_TEXTURE0);

  if (bumpmapmatrix != oldtexture0matrix) {
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(oldtexture0matrix[0]);
    glMatrixMode(GL_MODELVIEW);
  }
  state->pop();
}

void
soshape_bumprender::renderNormal(SoState * state, const SoPrimitiveVertexCache * cache)
{
  const cc_glglue * glue = sogl_glue_instance(state);
  int lastenabled = -1;
  //const SbBool * enabled = SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);

  // only use vertex program if two texture units (or less) are used
  // (only two units supported in the vertex program)
  SbBool use_vertex_program = lastenabled <= 1 && SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_VERTEX_PROGRAM);
  use_vertex_program = FALSE; // FIXME: disabled until vertex program
                              // for point lights is implemented
  if (use_vertex_program) {
    if (!this->diffuseprogramsinitialized) {
      this->initDiffusePrograms(glue, state);
    }
    glEnable(GL_VERTEX_PROGRAM_ARB);
    cc_glglue_glBindProgram(glue, GL_VERTEX_PROGRAM_ARB, normalrenderingvertexprogramid);
  }

  int arrays =
    SoPrimitiveVertexCache::TEXCOORD|
    SoPrimitiveVertexCache::COLOR;
  cache->renderTriangles(state, arrays);

  if (use_vertex_program) {
    glDisable(GL_VERTEX_PROGRAM_ARB);
  }
}

void
soshape_bumprender::calcTangentSpace(const SoPrimitiveVertexCache * cache)
{
  int i;
  const int numi = cache->getNumTriangleIndices();
  if (numi == 0) return;


  const int numv = cache->getNumVertices();
  const GLint * idxptr = cache->getTriangleIndices();
  const SbVec3f * vertices = cache->getVertexArray();
  //const SbVec3f * normals = cache->getNormalArray();
  const SbVec2f * bumpcoords = cache->getBumpCoordArray();

  this->tangentlist.truncate(0);
  this->tangentlist.ensureCapacity(numv * 2);

  for (i = 0; i < numv; i++) {
    this->tangentlist.append(SbVec3f(0.0f, 0.0f, 0.0f));
    this->tangentlist.append(SbVec3f(0.0f, 0.0f, 0.0f));
  }

  SbVec3f sTangent;
  SbVec3f tTangent;

  int idx[3];

  for (i = 0; i < numi; i += 3) {
    idx[0] = idxptr[i];
    idx[1] = idxptr[i+1];
    idx[2] = idxptr[i+2];

    SbVec3f side0 = vertices[idx[1]] - vertices[idx[0]];
    SbVec3f side1 = vertices[idx[2]] - vertices[idx[0]];

    float deltaT0 = bumpcoords[idx[1]][1] - bumpcoords[idx[0]][1];
    float deltaT1 = bumpcoords[idx[2]][1] - bumpcoords[idx[0]][1];
    sTangent = deltaT1 * side0 - deltaT0 * side1;
    NORMALIZE(sTangent);

    float deltaS0 = bumpcoords[idx[1]][0] - bumpcoords[idx[0]][0];
    float deltaS1 = bumpcoords[idx[2]][0] - bumpcoords[idx[0]][0];
    tTangent = deltaS1 * side0 - deltaS0 * side1;
    NORMALIZE(tTangent);

    for (int j = 0; j < 3; j++) {
      this->tangentlist[idx[j]*2] += sTangent;
      this->tangentlist[idx[j]*2+1] += tTangent;
    }
  }
  for (i = 0; i < numv; i++) {
    NORMALIZE(this->tangentlist[i*2]);
    NORMALIZE(this->tangentlist[i*2+1]);
  }
}

void
soshape_bumprender::calcTSBCoords(const SoPrimitiveVertexCache * cache, SoLight * COIN_UNUSED_ARG(light))
{
  SbVec3f thelightvec;
  SbVec3f tlightvec;

  const int numv = cache->getNumVertices();
  const SbVec3f * vertices = cache->getVertexArray();
  const SbVec3f * normals = cache->getNormalArray();

  this->cubemaplist.truncate(0);
  for (int i = 0; i < numv; i++) {
    SbVec3f sTangent = this->tangentlist[i*2];
    SbVec3f tTangent = this->tangentlist[i*2+1];
    thelightvec = this->getLightVec(vertices[i]);
    tlightvec = thelightvec;
#if 0 // FIXME: I don't think it's necessary to do this test. pederb, 2003-11-20
    SbVec3f tcross = tTangent.cross(sTangent);
    if (tcross.dot(normals[i]) < 0.0f) {
      tlightvec = -tlightvec;      
    }
#endif // disabled, probably not necessary
    this->cubemaplist.append(SbVec3f(sTangent.dot(tlightvec),
                                     tTangent.dot(tlightvec),
                                     normals[i].dot(thelightvec)));

  }
}

void
soshape_bumprender::initLight(SoLight * light, const SbMatrix & m)
{
  if (light->isOfType(SoPointLight::getClassTypeId())) {
    SoPointLight * pl = (SoPointLight*) light;
    this->lightvec = pl->location.getValue();
    m.multVecMatrix(this->lightvec, this->lightvec);
    this->ispointlight = TRUE;
  }
  else if (light->isOfType(SoDirectionalLight::getClassTypeId())) {
    SoDirectionalLight * dir = (SoDirectionalLight*)light;
    m.multDirMatrix(-(dir->direction.getValue()), this->lightvec);
    this->ispointlight = FALSE;
    NORMALIZE(this->lightvec);
  }
  else if (light->isOfType(SoSpotLight::getClassTypeId())) {
    SoSpotLight * pl = (SoSpotLight*) light;
    this->lightvec = pl->location.getValue();
    m.multVecMatrix(this->lightvec, this->lightvec);
    this->ispointlight = TRUE;

  }
  else {
    this->lightvec = SbVec3f(0.0f, 0.0f, 1.0f);
    this->ispointlight = FALSE;

  }
}


SbVec3f
soshape_bumprender::getLightVec(const SbVec3f & v) const
{
  if (this->ispointlight) {
    SbVec3f tmp = lightvec - v;
    NORMALIZE(tmp);
    return tmp;
  }
  else return this->lightvec;
}
