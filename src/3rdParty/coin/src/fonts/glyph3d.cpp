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

#include "fonts/glyph3d.h"

#include <cfloat> /* FLT_MIN */
#include <cstring>
#include <cassert>
#include <cstdio>

#include <Inventor/C/basic.h>
#include <Inventor/C/base/list.h>
#include <Inventor/C/base/string.h>

#include "tidbitsp.h"
#include "base/dict.h"
#include "threads/threadsutilp.h"
#include "fonts/fontlib_wrapper.h"
#include "fonts/glyph.h"
#include "fonts/defaultfonts.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using namespace std;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/* ********************************************************************** */

static SbBool glyph3d_specmatch(const cc_font_specification * spec1, 
                                const cc_font_specification * spec2);
static void glyph3d_calcboundingbox(cc_glyph3d * g);

struct cc_glyph3d {
  struct cc_glyph c; /* "c" for "common" glyph data (2d & 3d). */
  struct cc_font_vector_glyph * vectorglyph;

  float bbox[4];
  float width; /* FIXME: is this one really in use? 20060109 mortene. */
  SbBool didallocvectorglyph;
};

/* ********************************************************************** */

static cc_dict * glyph3d_fonthash = NULL;
static int glyph3d_spaceglyphindices[] = { -1, -1 };
static float glyph3d_spaceglyphvertices[] = { 0, 0 };
static SbBool glyph3d_initialized = FALSE;

/* Mutex lock for the static ang global font hash */
static void * glyph3d_fonthash_lock = NULL;

/* Because the 3D glyphs are normalized when generated, a standard
   fontsize is used for all glyphs. This also prevent Windows from
   quantizing advancement and kerning values for very small fontsizes
   (size<4).
*/
static unsigned int glyph3d_standardfontsize = 50;

/* ********************************************************************** */

/* Set '#if 1' to enable debug info to stderr when tracking mutex locking. */
#if 0
#define GLYPH3D_MUTEX_LOCK(m) \
  do { \
    (void)fprintf(stderr, "glyph3d mutex lock in %s\n", __func__); \
    CC_MUTEX_LOCK(m); \
  } while (0)
#define GLYPH3D_MUTEX_UNLOCK(m) \
  do { \
    (void)fprintf(stderr, "glyph3d mutex unlock in %s\n", __func__); \
    CC_MUTEX_UNLOCK(m); \
  } while (0)
#else
#define GLYPH3D_MUTEX_LOCK(m) CC_MUTEX_LOCK(m)
#define GLYPH3D_MUTEX_UNLOCK(m) CC_MUTEX_UNLOCK(m)
#endif

static void
cc_glyph3d_cleanup(void)
{
  CC_MUTEX_DESTRUCT(glyph3d_fonthash_lock);
  cc_dict_destruct(glyph3d_fonthash);
  glyph3d_fonthash = NULL;
  glyph3d_initialized = FALSE;
}

static void
cc_glyph3d_initialize()
{
  CC_MUTEX_CONSTRUCT(glyph3d_fonthash_lock);
  GLYPH3D_MUTEX_LOCK(glyph3d_fonthash_lock);
  
  if (glyph3d_initialized) {
    GLYPH3D_MUTEX_UNLOCK(glyph3d_fonthash_lock);
    return;
  }
  glyph3d_initialized = TRUE;
  
  glyph3d_fonthash = cc_dict_construct(15, 0.75);

  /* +1, so it happens before the underlying font abstraction layer
     cleans itself up: */
  coin_atexit((coin_atexit_f*) cc_glyph3d_cleanup, CC_ATEXIT_FONT_SUBSYSTEM_HIGHPRIORITY);

  GLYPH3D_MUTEX_UNLOCK(glyph3d_fonthash_lock);  
}

cc_glyph3d *
cc_glyph3d_ref(uint32_t character, const cc_font_specification * spec)
{
  cc_glyph3d * glyph;
  int glyphidx;
  int fontidx;
  void * val;
  cc_font_specification * newspec;
  cc_string * fonttoload;
  cc_list * glyphlist = NULL;

  /* because this function is the entry point for glyph3d, the mutex
     is initialized here. */
  if (glyph3d_fonthash_lock == NULL) 
    cc_glyph3d_initialize();
  
  assert(spec);

  GLYPH3D_MUTEX_LOCK(glyph3d_fonthash_lock);

  /* Has the glyph been created before? */
  if (cc_dict_get(glyph3d_fonthash, (uintptr_t)character, &val)) {
    int i;
    glyphlist = (cc_list *) val;
    for (i=0;i<cc_list_get_length(glyphlist);++i) {
      glyph = (cc_glyph3d *) cc_list_get(glyphlist, i);
      if (glyph3d_specmatch(spec, glyph->c.fontspec)) {
        GLYPH3D_MUTEX_UNLOCK(glyph3d_fonthash_lock);
        glyph->c.refcount++;
        return glyph;
      }
    }    
  } else {
    /* No glyphlist for this character is found. Create one and
       add it to the hashtable. */
    glyphlist = cc_list_construct();
    cc_dict_put(glyph3d_fonthash, (uintptr_t)character, glyphlist);
  }

  assert(glyphlist);

  /* build a new glyph struct */
  glyph = (cc_glyph3d *) malloc(sizeof(cc_glyph3d));

  glyph->c.character = character;
  glyph->c.refcount = 1;


  newspec = (cc_font_specification *) malloc(sizeof(cc_font_specification));
  assert(newspec);
  cc_fontspec_copy(spec, newspec);

  glyph->c.fontspec = newspec;

  /* FIXME: fonttoload variable should be allocated on the
     stack. 20030921 mortene. */
  fonttoload = cc_string_construct_new();
  cc_string_set_text(fonttoload, cc_string_get_text(&spec->name));
  if (cc_string_length(&spec->style) > 0) {
    cc_string_append_text(fonttoload, " ");
    cc_string_append_string(fonttoload, &spec->style);
  }
  
  fontidx = cc_flw_get_font_id(cc_string_get_text(fonttoload), 
                               glyph3d_standardfontsize,
                               0.0f,
                               newspec->complexity);

  cc_string_destruct(fonttoload);
  assert(fontidx >= 0);

  cc_flw_ref_font(fontidx);

  glyphidx = cc_flw_get_glyph(fontidx, character);

  /* Should _always_ be able to get hold of a glyph -- if no glyph is
     available for a specific character, a default empty rectangle
     should be used.  -mortene. */
  assert(glyphidx >= 0);

  glyph->c.glyphidx = glyphidx;
  glyph->c.fontidx = fontidx;
  glyph->didallocvectorglyph = FALSE;

  glyph->vectorglyph = cc_flw_get_vector_glyph(fontidx, glyphidx);

  /* Setup builtin default font if no character was found */
  /* FIXME: this should be moved to fontlib_wrapper.c. 20050623 mortene. */
  if (glyph->vectorglyph == NULL) {
    glyph->vectorglyph = (struct cc_font_vector_glyph *) malloc(sizeof(struct cc_font_vector_glyph));
    glyph->didallocvectorglyph = TRUE;

    if (character <= 32 || character >= 127) {

      /* FIXME: Characters other than space, should be replaced with
         squares. (20030910 handegar) */

      /* treat all these characters as spaces*/
      glyph->vectorglyph->vertices = (float *) glyph3d_spaceglyphvertices;
      glyph->vectorglyph->faceindices = (int *) glyph3d_spaceglyphindices;
      glyph->vectorglyph->edgeindices = (int *) glyph3d_spaceglyphindices;
    }
    else {
      glyph->vectorglyph->vertices = (float *) coin_default3dfont_get_coords()[character-33];
      glyph->vectorglyph->faceindices = (int *) coin_default3dfont_get_faceidx()[character-33];
      glyph->vectorglyph->edgeindices = (int *) coin_default3dfont_get_edgeidx()[character-33];
    }
  }
  
  glyph3d_calcboundingbox(glyph);
  glyph->width = glyph->bbox[2] - glyph->bbox[0];

  /* Store newly created glyph in the list for this character */
  cc_list_append(glyphlist, glyph);

  GLYPH3D_MUTEX_UNLOCK(glyph3d_fonthash_lock);
  return glyph;
}

static void
finalize_glyph3d(cc_glyph * g)
{
  cc_glyph3d * g3d = (cc_glyph3d *)g;
  if (g3d->didallocvectorglyph) { free(g3d->vectorglyph); }
}

void 
cc_glyph3d_unref(cc_glyph3d * glyph)
{
  cc_glyph_unref(glyph3d_fonthash, &(glyph->c), finalize_glyph3d);
}

const float *
cc_glyph3d_getcoords(const cc_glyph3d * g)
{
  const float * ptr = cc_flw_get_vector_glyph_coords(g->vectorglyph);
  if (ptr == NULL) {    
    assert(g->vectorglyph->vertices && "Default vertices have not been initialized as expected!");
    return g->vectorglyph->vertices;
  }
  return ptr;  
}

const int *
cc_glyph3d_getfaceindices(const cc_glyph3d * g)
{
  const int * ptr = cc_flw_get_vector_glyph_faceidx(g->vectorglyph);
  if (ptr == NULL) {      
    assert(g->vectorglyph->faceindices && "Default face indices have not been initialized as expected!");
    return g->vectorglyph->faceindices; 
  }
  return ptr;
}

const int *
cc_glyph3d_getedgeindices(const cc_glyph3d * g)
{
  const int * ptr = cc_flw_get_vector_glyph_edgeidx(g->vectorglyph);
  if (ptr == NULL) {    
    assert(g->vectorglyph->edgeindices && "Default edge indices have not been initialized as expected!");
    return g->vectorglyph->edgeindices; 
  }
  return ptr;
}

const int *
cc_glyph3d_getnextcwedge(const cc_glyph3d * g, int edgeidx)
{

  int findidx;
  const int * ptr;
  const int * edgeptr;
  int idx = edgeidx * 2;

  edgeptr = cc_glyph3d_getedgeindices(g);

  /* test for common case */
  if (edgeidx > 0) {
    if (edgeptr[idx] == edgeptr[idx-1])
      return &edgeptr[idx-2];
  }
  /* do a linear search */
  findidx = edgeptr[idx];
  ptr = edgeptr;
  while (*ptr >= 0) {
    if (ptr[1] == findidx) return ptr;
    ptr += 2;
  }

  return NULL;
}

const int *
cc_glyph3d_getnextccwedge(const cc_glyph3d * g, int edgeidx)
{
  int findidx;
  const int * ptr;
  const int * edgeptr;  
  int idx = edgeidx * 2;
  
  edgeptr = cc_glyph3d_getedgeindices(g);

  /* test for common case */
  if (edgeptr[idx+1] == edgeptr[idx+2])
    return &edgeptr[idx+2];

  /* do a linear search */
  findidx = edgeptr[idx+1];
  ptr = edgeptr;
  while (*ptr >= 0) {
    if (*ptr == findidx) 
      return ptr;
    ptr += 2;
  }

  return NULL;

}


float
cc_glyph3d_getwidth(const cc_glyph3d * g)
{
  assert(g && "Parameter cc_glyph3d == NULL!");
  return g->width;
}

static void
glyph3d_calcboundingbox(cc_glyph3d * g)
{
  float * coordptr;
  int * edgeptr;

  g->bbox[0] = 0.0f;
  g->bbox[1] = 0.0f;
  g->bbox[2] = 0.0f;
  g->bbox[3] = 0.0f;

  coordptr = (float *) cc_glyph3d_getcoords(g);
  edgeptr = (int *) cc_glyph3d_getedgeindices(g);

  while ((*edgeptr >= 0) && (*edgeptr != -1)) {

    g->bbox[0] = cc_min(coordptr[(*edgeptr)*2], g->bbox[0]);
    g->bbox[1] = cc_min(coordptr[(*edgeptr)*2 + 1], g->bbox[1]);
    g->bbox[2] = cc_max(coordptr[(*edgeptr)*2], g->bbox[2]);
    g->bbox[3] = cc_max(coordptr[(*edgeptr)*2 + 1], g->bbox[3]);

    edgeptr++;
  }

}

const float *
cc_glyph3d_getboundingbox(const cc_glyph3d * g)
{
  return g->bbox;
}

void
cc_glyph3d_getadvance(const cc_glyph3d * g, float * x, float * y)
{
  cc_flw_get_vector_advance(g->c.fontidx, g->c.glyphidx, x, y);
}

void
cc_glyph3d_getkerning(const cc_glyph3d * left, const cc_glyph3d * right,
                      float * x, float * y)
{
  cc_flw_get_vector_kerning(right->c.fontidx, left->c.glyphidx, right->c.glyphidx, x, y);
}

static SbBool
glyph3d_specmatch(const cc_font_specification * spec1,
                  const cc_font_specification * spec2)
{
  float c1, c2;
  int temp1,temp2;
  
  assert(spec1);
  assert(spec2);
  
  /* Reducing precision of the complexity variable. This is done to
     prevent the user from flooding the memory with generated glyphs
     which might be more or less identical */
  c1 = spec1->complexity;
  c2 = spec2->complexity;
  
  /* Clamp values to [0...1] */
  if (c1 > 1.0f) c1 = 1.0f;
  if (c1 < 0.0f) c1 = 0.0f;
  if (c2 > 1.0f) c2 = 1.0f;
  if (c2 < 0.0f) c2 = 0.0f;
  
  temp1 = (int) (c1 * 10.0f);
  temp2 = (int) (c2 * 10.0f);
  
  if ((!cc_string_compare(&spec1->name, &spec2->name)) &&
      (!cc_string_compare(&spec1->style, &spec2->style)) &&
      (temp1 == temp2)) {
    /* No need to compare size for 3D fonts */
    return TRUE;
  }

  return FALSE;
}

#undef GLYPH3D_MUTEX_LOCK
#undef GLYPH3D_MUTEX_UNLOCK
