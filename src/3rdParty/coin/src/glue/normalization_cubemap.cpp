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

#include <cstdlib>
#include <cmath>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/C/tidbits.h>

#include "glue/glp.h"
#include "tidbitsp.h"

// *************************************************************************

#define COIN_DEFAULT_CUBEMAP_SIZE 64

// *************************************************************************

/*
  Based on code from "Simple Bump Mapping Tutorial"
  http://www.paulsprojects.net
*/

/*
  Convert a [-1,1] vector to a [0,1] vector.
*/
static void coin_pack_to_01(float * v)
{
  float len = (float) sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

  /* normalize */
  if (len != 0.0f) {
    len = 1.0f / len;
    v[0] *= len;
    v[1] *= len;
    v[2] *= len;
  }
  v[0] = 0.5f * v[0] + 0.5f;
  v[1] = 0.5f * v[1] + 0.5f;
  v[2] = 0.5f * v[2] + 0.5f;
}

static void
coin_create_normalization_cube_map(const int size)
{
  float offset = 0.5f;
  float halfsize = size * 0.5f;
  float tmpvec[3];
  unsigned char * ptr;
  unsigned char * data = (unsigned char*) malloc(size_t(size)*size_t(size)*3);

  /* positive x */
  ptr = data;
  for(int j = 0; j < size; j++) {
    for(int i = 0; i < size; i++) {
      tmpvec[0] = halfsize;
      tmpvec[1] = -(((float)j)+offset-halfsize);
      tmpvec[2] = -(((float)i)+offset-halfsize);

      coin_pack_to_01(tmpvec);

      ptr[0]=(unsigned char)(tmpvec[0]*255);
      ptr[1]=(unsigned char)(tmpvec[1]*255);
      ptr[2]=(unsigned char)(tmpvec[2]*255);

      ptr += 3;
    }
  }
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X,
               0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

  /* negative x */
  ptr = data;

  for(int j = 0; j < size; j++) {
    for(int i = 0; i < size; i++) {
      tmpvec[0] = -halfsize;
      tmpvec[1] = -(((float)j)+offset-halfsize);
      tmpvec[2] = (((float)i)+offset-halfsize);

      coin_pack_to_01(tmpvec);

      ptr[0]=(unsigned char)(tmpvec[0]*255);
      ptr[1]=(unsigned char)(tmpvec[1]*255);
      ptr[2]=(unsigned char)(tmpvec[2]*255);

      ptr += 3;
    }
  }
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
               0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

  /* positive y */
  ptr=data;

  for(int j = 0; j < size; j++) {
      for(int i = 0; i < size; i++) {
        tmpvec[0] = ((float)i)+offset-halfsize;
        tmpvec[1] = halfsize;
        tmpvec[2] = ((float)j)+offset-halfsize;

        coin_pack_to_01(tmpvec);

        ptr[0]=(unsigned char)(tmpvec[0]*255);
        ptr[1]=(unsigned char)(tmpvec[1]*255);
        ptr[2]=(unsigned char)(tmpvec[2]*255);

        ptr += 3;
      }
  }
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
               0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

  /* negative y */
  ptr = data;

  for(int j = 0; j < size; j++) {
    for(int i = 0; i < size; i++) {
      tmpvec[0] = ((float)i)+offset-halfsize;
      tmpvec[1] = -halfsize;
      tmpvec[2] = -(((float)j)+offset-halfsize);

      coin_pack_to_01(tmpvec);

      ptr[0]=(unsigned char)(tmpvec[0]*255);
      ptr[1]=(unsigned char)(tmpvec[1]*255);
      ptr[2]=(unsigned char)(tmpvec[2]*255);

      ptr += 3;
    }
  }
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
               0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

  /* positive z */
  ptr = data;

  for(int j = 0; j < size; j++) {
    for(int i = 0; i < size; i++) {
      tmpvec[0] = ((float)i)+offset-halfsize;
      tmpvec[1] = -(((float)j)+offset-halfsize);
      tmpvec[2] = halfsize;

      coin_pack_to_01(tmpvec);

      ptr[0]=(unsigned char)(tmpvec[0]*255);
      ptr[1]=(unsigned char)(tmpvec[1]*255);
      ptr[2]=(unsigned char)(tmpvec[2]*255);

      ptr += 3;
    }
  }
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
               0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

  /* negative z */
  ptr=data;

  for(int j = 0; j < size; j++) {
    for(int i = 0; i < size; i++) {
      tmpvec[0] = -(((float)i)+offset-halfsize);
      tmpvec[1] = -(((float)j)+offset-halfsize);
      tmpvec[2] = -halfsize;

      coin_pack_to_01(tmpvec);

      ptr[0]=(unsigned char)(tmpvec[0]*255);
      ptr[1]=(unsigned char)(tmpvec[1]*255);
      ptr[2]=(unsigned char)(tmpvec[2]*255);

      ptr += 3;
    }
  }
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
               0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

  free(data);
}

void
coin_apply_normalization_cube_map(const cc_glglue * glue)
{
  if (glue->normalizationcubemap == 0) {
    GLuint * dst = (GLuint*) &glue->normalizationcubemap;
    static int CUBEMAP_SIZE = -1;
    if (CUBEMAP_SIZE < 0) {
      const char * env = coin_getenv("COIN_NORMALIZATION_CUBEMAP_SIZE");
      CUBEMAP_SIZE = COIN_DEFAULT_CUBEMAP_SIZE;
      if (env) {
        int size = atoi(env);
        if (size >= 2 && coin_is_power_of_two((uint32_t) size)) {
          CUBEMAP_SIZE = size;
        }
      }
    }
    cc_glglue_glGenTextures(glue, 1, dst);
    cc_glglue_glBindTexture(glue, GL_TEXTURE_CUBE_MAP, *dst);
    coin_create_normalization_cube_map(CUBEMAP_SIZE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }
  cc_glglue_glBindTexture(glue, GL_TEXTURE_CUBE_MAP,
                          glue->normalizationcubemap);
}

#undef COIN_DEFAULT_CUBEMAP_SIZE
