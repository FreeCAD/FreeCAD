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

#include "fonts/fontlib_wrapper.h"

#include <cstring>
#include <cassert>
#include <cstdio>

#include <Inventor/C/base/string.h>

#include "base/dict.h"
#include "threads/threadsutilp.h"
#include "tidbitsp.h"
#include "fonts/glyph2d.h"
#include "fonts/glyph.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using namespace std;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

static SbBool glyph2d_specmatch(const cc_font_specification * spec1, const cc_font_specification * spec2);

struct cc_glyph2d {
  struct cc_glyph c; /* "c" for "common" glyph data (2d & 3d). */

  float angle; /* FIXME: is this one really in use? 20060109 mortene. */
  unsigned short width; /* FIXME: is this one really in use? 20060109 mortene. */

  unsigned short height;
  unsigned short bitmapwidth;
  unsigned short bitmapheight;
  short bitmapoffsetx;
  short bitmapoffsety;
  unsigned char * bitmap;
  SbBool mono;
};

static cc_dict * glyph2d_fonthash = NULL;
static SbBool glyph2d_initialized = FALSE;

/*
  Mutex lock for the static ang global font hash
*/
static void * glyph2d_fonthash_lock = NULL;

/* Set '#if 1' to enable debug output to stderr for tracking mutex locking. */
#if 0
#define GLYPH2D_MUTEX_LOCK(m) \
  do { \
    (void)fprintf(stderr, "glyph2d mutex lock in %s\n", __func__); \
    CC_MUTEX_LOCK(m); \
  } while (0)
#define GLYPH2D_MUTEX_UNLOCK(m) \
  do { \
    (void)fprintf(stderr, "glyph2d mutex unlock in %s\n", __func__); \
    CC_MUTEX_UNLOCK(m); \
  } while (0)
#else
#define GLYPH2D_MUTEX_LOCK(m) CC_MUTEX_LOCK(m)
#define GLYPH2D_MUTEX_UNLOCK(m) CC_MUTEX_UNLOCK(m)
#endif

static void
cc_glyph2d_cleanup(void)
{
  CC_MUTEX_DESTRUCT(glyph2d_fonthash_lock);
  cc_dict_destruct(glyph2d_fonthash);
  glyph2d_fonthash = NULL;
  glyph2d_initialized = FALSE;
}

static void
cc_glyph2d_initialize()
{
  CC_MUTEX_CONSTRUCT(glyph2d_fonthash_lock);
  GLYPH2D_MUTEX_LOCK(glyph2d_fonthash_lock);
  
  if (glyph2d_initialized) {
    GLYPH2D_MUTEX_UNLOCK(glyph2d_fonthash_lock);
    return;
  }
  glyph2d_initialized = TRUE;
  
  glyph2d_fonthash = cc_dict_construct(15, 0.75);

  /* +1, so it happens before the underlying font abstraction layer
     cleans itself up: */
  coin_atexit((coin_atexit_f*) cc_glyph2d_cleanup, CC_ATEXIT_FONT_SUBSYSTEM_HIGHPRIORITY);
  
  GLYPH2D_MUTEX_UNLOCK(glyph2d_fonthash_lock);
}

cc_glyph2d * 
cc_glyph2d_ref(uint32_t character, const cc_font_specification * spec, float angle)
{
  void * val;
  cc_glyph2d * glyph;
  int fontidx;
  int glyphidx;
  int i;
  struct cc_font_bitmap * bm;
  cc_font_specification * newspec;
  cc_string * fonttoload;
  cc_list * glyphlist;


  /* because this function is the entry point for glyph2d, the mutex
     is initialized here. */
  if (glyph2d_fonthash_lock == NULL) 
    cc_glyph2d_initialize();
  
  assert(spec);

  GLYPH2D_MUTEX_LOCK(glyph2d_fonthash_lock);

  /* Has the glyph been created before? */
  if (cc_dict_get(glyph2d_fonthash, (uintptr_t)character, &val)) {
    glyphlist = (cc_list *) val;
    for (i = 0; i < cc_list_get_length(glyphlist); ++i) {
      glyph = (cc_glyph2d *) cc_list_get(glyphlist, i);
      if (glyph2d_specmatch(spec, glyph->c.fontspec)) {
        GLYPH2D_MUTEX_UNLOCK(glyph2d_fonthash_lock);
        glyph->c.refcount++;
        return glyph;
      }
    }    
  } 
  else {
    /* No glyphlist for this character is found. Create one and
       add it to the hashtable. */
    glyphlist = cc_list_construct();
    cc_dict_put(glyph2d_fonthash, (uintptr_t)character, glyphlist);
  }

  assert(glyphlist);

  /* build a new glyph struct with bitmap */    
  glyph = (cc_glyph2d *) malloc(sizeof(cc_glyph2d));
  glyph->c.character = character;
  
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
  
  /* FIXME: get rid of angle -- not used. 20050516 mortene. */
  fontidx =
    cc_flw_get_font_id(cc_string_get_text(fonttoload),
                       (unsigned int)(newspec->size), angle,
                       /* FIXME: passing in -1 for complexity below is
                          necessary because the Win32 API code in
                          win32.c cc_flww32_get_font() detects this
                          and uses this as a hack to *not* change the
                          font size. (Changing the font size is
                          necessary for *3D* fonts because otherwise
                          the resolution will be too low for
                          vectorization.)

                          What an ugly mess. Should be cleaned up
                          properly -- the worst offender against any
                          good design taste is the code in
                          win32.c. 20050706 mortene.*/
                       -1.0f);

  cc_string_destruct(fonttoload);
  assert(fontidx >= 0);

  cc_flw_ref_font(fontidx);

  /* Should _always_ be able to get hold of a glyph -- if no glyph is
     available for a specific character, a default empty rectangle
     should be used.  -mortene. */
  glyphidx = cc_flw_get_glyph(fontidx, character);
  assert(glyphidx >= 0);

  glyph->c.glyphidx = glyphidx;
  glyph->c.fontidx = fontidx;
  glyph->angle = angle;

  bm = cc_flw_get_bitmap(fontidx, glyphidx);
  assert(bm);
  glyph->width = bm->width;
  glyph->height = bm->rows;
  glyph->bitmapwidth = bm->mono ? bm->pitch * 8 : bm->pitch;
  glyph->bitmapheight = bm->rows;
  glyph->bitmapoffsetx = bm->bearingX;
  glyph->bitmapoffsety = bm->bearingY;
  glyph->bitmap = bm->buffer;
  glyph->mono = bm->mono;
  glyph->c.refcount = 1;
  
  /* Store newly created glyph in the list for this character */
  cc_list_append(glyphlist, glyph);
  
  GLYPH2D_MUTEX_UNLOCK(glyph2d_fonthash_lock);
  return glyph;
}

void
cc_glyph2d_unref(cc_glyph2d * glyph)
{
  cc_glyph_unref(glyph2d_fonthash, &(glyph->c), NULL);
}

static SbBool 
glyph2d_specmatch(const cc_font_specification * spec1, 
                  const cc_font_specification * spec2)
{
  assert(spec1);
  assert(spec2);

  if ((!cc_string_compare(&spec1->name, &spec2->name)) &&
      (!cc_string_compare(&spec1->style, &spec2->style)) &&
      (int(spec1->size) == int(spec2->size))) {
    /* No need to compare complexity for 2D fonts */
    return TRUE;
  }
  else return FALSE;
  
}

void 
cc_glyph2d_getadvance(const cc_glyph2d * g, int * x, int * y)
{
  cc_flw_get_bitmap_advance(g->c.fontidx, g->c.glyphidx, x, y);
}

void 
cc_glyph2d_getkerning(const cc_glyph2d * left, const cc_glyph2d * right, int * x, int * y)
{
  cc_flw_get_bitmap_kerning(right->c.fontidx, left->c.glyphidx, right->c.glyphidx, x, y);
}

unsigned int 
cc_glyph2d_getwidth(const cc_glyph2d * g)
{
  return (int) g->width;
}

const unsigned char * 
cc_glyph2d_getbitmap(const cc_glyph2d * g, int * size, int * offset)
{
  size[0] = g->bitmapwidth;
  size[1] = g->bitmapheight;
  offset[0] = g->bitmapoffsetx;
  offset[1] = g->bitmapoffsety;

  return g->bitmap;
}

SbBool 
cc_glyph2d_getmono(const cc_glyph2d * g)
{
  return g->mono;
}

#undef GLYPH2D_MUTEX_LOCK
#undef GLYPH2D_MUTEX_UNLOCK
