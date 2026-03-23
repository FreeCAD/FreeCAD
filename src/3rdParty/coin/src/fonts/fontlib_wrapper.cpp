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

/* ********************************************************************** */

#include "coindefs.h"
#include "fonts/fontlib_wrapper.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstddef>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <Inventor/C/tidbits.h>
#include <Inventor/C/base/string.h>
#include <Inventor/C/errors/debugerror.h>

#include "base/dict.h"
#include "base/dynarray.h"
#include "fonts/freetype.h"
#include "fonts/win32.h"
#include "fonts/defaultfonts.h"
#include "threads/threadsutilp.h"
#include "tidbitsp.h"

/* ********************************************************************** */

/*
  All public interface functions are protected with this "file-global"
  lock, to simplify the implementation of the underlying font-import
  modules (as they won't have to be made reentrant in any way).
*/

static void * flw_global_lock = NULL;
static int flw_global_font_index = 0;
static SbBool initialized = FALSE;
static SbBool tried_init_freetype_fontlib = FALSE;
static SbBool tried_init_win32_fontlib = FALSE;
static SbBool fontlib_freetype_available = FALSE;
static SbBool fontlib_win32_available = FALSE;


/* Debug: enable this in case code hangs waiting for a lock.  A hang
   will typically happen for one out of two reasons:

   1) The mutex is locked but not released. To check whether or not
   this is the cause, look for multiple exit points (return
   statements) in a function.

   2) A cc_flw_*() function locks the global mutex, then calls another
   cc_flw_*() function, which when attempting to lock the same
   mutex will simply hang.

   -- mortene.
*/
#if 0

#define FLW_MUTEX_LOCK(m) \
  do { \
    (void)fprintf(stderr, "mutex lock in %s\n", __func__); \
    CC_MUTEX_LOCK(m); \
  } while (0)

#define FLW_MUTEX_UNLOCK(m) \
  do { \
    (void)fprintf(stderr, "mutex unlock in %s\n", __func__); \
    CC_MUTEX_UNLOCK(m); \
  } while (0)

#else

#define FLW_MUTEX_LOCK(m) CC_MUTEX_LOCK(m)
#define FLW_MUTEX_UNLOCK(m) CC_MUTEX_UNLOCK(m)

#endif

/* ********************************************************************** */

struct cc_flw_glyph {
  unsigned int nativeglyphidx;
  unsigned int character;
  SbBool fromdefaultfont;

  /* FIXME: these should have their own "fromdefaultfont" flags, since
     they may both be, even though the glyph was set up in FreeType or
     Win32 API. 20050624 mortene. */
  struct cc_font_bitmap * bitmap;
  struct cc_font_vector_glyph * vector;
};

struct cc_flw_font {
  void * nativefonthandle;
  cc_string * fontname;
  cc_string * requestname;
  cc_dict * glyphdict;
  unsigned int sizey;
  float angle;
  float complexity;
  SbBool defaultfont;
  int fontindex;
  int refcount;
};

static cc_dynarray * fontarray = NULL;

/* ********************************************************************** */

static void
dump_cc_flw_font(const char * srcfunc, struct cc_flw_font * f)
{
  cc_debugerror_postinfo(srcfunc,
                         "nativefonthandle==%p, fontname=='%s', requestname=='%s', "
                         "glyphdict==%p, sizey==%u, angle==%f, "
                         "complexity==%f, defaultfont==%s, fontindex==%d, "
                         "refcount==%d",
                         f->nativefonthandle,
                         cc_string_get_text(f->fontname),
                         cc_string_get_text(f->requestname),
                         f->glyphdict, f->sizey, f->angle,
                         f->complexity,
                         f->defaultfont ? "TRUE" : "FALSE",
                         f->fontindex, f->refcount);
}

static void
dump_cc_flw_glyph(const char * srcfunc, struct cc_flw_glyph * g)
{
  cc_debugerror_postinfo(srcfunc,
                         "nativeglyphidx==%u, bitmap==%p, vector==%p, "
                         "fromdefaultfont==%s, character=='%c'",
                         g->nativeglyphidx, g->bitmap, g->vector,
                         g->fromdefaultfont ? "TRUE" : "FALSE",
                         g->character);
}

/* ********************************************************************** */

static void freetype_cleanup(void) { cc_flwft_exit(); }

static SbBool
using_freetype(void)
{
  if (!tried_init_freetype_fontlib) {
    const char * env;

    tried_init_freetype_fontlib = TRUE;

    env = coin_getenv("COIN_FORCE_FREETYPE_OFF");
    fontlib_freetype_available = (env && (atoi(env) > 0)) ? FALSE : TRUE;
    fontlib_freetype_available = fontlib_freetype_available && cc_flwft_initialize();
    if (cc_font_debug()) {
      cc_debugerror_postinfo("using_freetype",
                             "FreeType library will%s be used",
                             fontlib_freetype_available ? "" : " not");
    }

    if (fontlib_freetype_available) {
      coin_atexit((coin_atexit_f *)freetype_cleanup,
                  /* priority must be lower than for abstraction
                     interface, so don't change this willy-nilly: */
                  CC_ATEXIT_FONT_SUBSYSTEM_LOWPRIORITY);
    }
  }

  return fontlib_freetype_available;
}

static void win32api_cleanup(void) { cc_flww32_exit(); }

static SbBool
using_win32api(void)
{
  if (!tried_init_win32_fontlib) {
    const char * env;

    tried_init_win32_fontlib = TRUE;

    env = coin_getenv("COIN_FORCE_WIN32FONTS_OFF");
    fontlib_win32_available = (env && (atoi(env) > 0)) ? FALSE : TRUE;
    fontlib_win32_available = fontlib_win32_available && cc_flww32_initialize();
    if (cc_font_debug()) {
      cc_debugerror_postinfo("cc_flw_initialize",
                             "Win32 API can%s be used for font support",
                             fontlib_win32_available ? "" : " not");
    }

    /* Allow only one of the availability flags to be set, as it is too
       easy to get bugs in our code in this file if we depend on
       always checking one particular flag before the other.

       We prefer to consistently use the FreeType library over the
       Win32 API if available -- assuming that FreeType has been
       installed for a good reason on the Windows machine in question.
    */

    if (fontlib_win32_available && using_freetype()) {
      if (cc_font_debug()) {
        cc_debugerror_postinfo("using_win32api",
                               "FreeType library will take precedence "
                               "over Win32 API");
      }
      cc_flww32_exit();
      fontlib_win32_available = FALSE;
    }

    if (fontlib_win32_available) {
      coin_atexit((coin_atexit_f *)win32api_cleanup,
                  /* priority must be lower than for abstraction
                     interface, so don't change this willy-nilly: */
                  CC_ATEXIT_FONT_SUBSYSTEM_LOWPRIORITY);
    }
  }

  return fontlib_win32_available;
}

/* ********************************************************************** */

static struct cc_font_bitmap *
get_default_bitmap(unsigned int character, float wantedsize)
{
  struct cc_font_bitmap * bm;

  if (character < 256) { /* FIXME: should this be an assert? 20050622 mortene. */
    const int fontheight = coin_default2dfont_get_height(wantedsize);
    unsigned char * fontdata =
      (unsigned char *) coin_default2dfont_get_data(wantedsize);

    bm = (struct cc_font_bitmap *) malloc(sizeof(struct cc_font_bitmap));
    bm->buffer = fontdata + fontheight * 4 * character;
    bm->bearingX = 0;
    bm->bearingY = coin_default2dfont_get_bearing(wantedsize);
    bm->rows = fontheight;
    bm->width = coin_default2dfont_get_width(wantedsize);
    bm->pitch = 4;
    bm->mono = 1;
    return bm;
  }
  return NULL;
}

static struct cc_flw_glyph *
flw_glyphidx2glyphptr(struct cc_flw_font * fs, unsigned int glyphidx)
{
  void * tmp;
  struct cc_flw_glyph * gs = NULL;

  if (cc_dict_get(fs->glyphdict, glyphidx, &tmp)) {
    gs = (struct cc_flw_glyph *) tmp;
  }

  return gs;
}

static void
fontstruct_rmglyph(struct cc_flw_font * fs, unsigned int glyph)
{
  struct cc_flw_glyph * gs = flw_glyphidx2glyphptr(fs, glyph);
  assert(gs);

  if (gs->bitmap) {
    if (!gs->fromdefaultfont && gs->bitmap->buffer) { free(gs->bitmap->buffer); }
    free(gs->bitmap);
  }

  if (gs->vector && !gs->fromdefaultfont) {
    free(gs->vector->vertices);
    free(gs->vector->faceindices);
    free(gs->vector->edgeindices);
    free(gs->vector);
  }

  if (!cc_dict_remove(fs->glyphdict, glyph)) {
    assert(0 && "glyph to remove not found");
  }

  free(gs);
}

static void
fontstruct_rmglyph_apply(uintptr_t key, void * COIN_UNUSED_ARG(val), void * closure)
{
  fontstruct_rmglyph((struct cc_flw_font *)closure, (unsigned int)key);
}

static void
fontstruct_rmfont(int font)
{
  int i, n;
  int arrayindex;
  struct cc_flw_font * fs = NULL;

  n = cc_dynarray_length(fontarray);
  for (i = 0; i < n; i++) {
    fs = (struct cc_flw_font *)cc_dynarray_get(fontarray, i);
    if (fs->fontindex == font) break;
  }
  assert(i < n);
  arrayindex = i;

  if (fs->fontname) cc_string_destruct(fs->fontname);
  if (fs->requestname) cc_string_destruct(fs->requestname);

  cc_dict_apply(fs->glyphdict, fontstruct_rmglyph_apply, fs);
  cc_dict_destruct(fs->glyphdict);
  free(fs);
  cc_dynarray_remove_idx(fontarray, arrayindex);
}

static struct cc_flw_font *
flw_fontidx2fontptr(int fontidx)
{
  struct cc_flw_font * fs = NULL;
  int i, n;

  n = (int) cc_dynarray_length(fontarray);
  for (i = 0; i < n; i++) {
    fs = (struct cc_flw_font *) cc_dynarray_get(fontarray, i);
    if (fs->fontindex == fontidx) break;
  }
  assert(i < n);

  return fs;
}

static void
flw_exit(void)
{
  unsigned int n;

  n = cc_dynarray_length(fontarray);
  while (n--) {
    struct cc_flw_font * fs = (struct cc_flw_font *)cc_dynarray_get(fontarray, n);

#if COIN_DEBUG
    /* don't use cc_debugerror_post_* here since this will be called when Coin is
       exiting => we cannot allocate new resources */
    fprintf(stderr, "flw_exit: font instance resource leak:\n");
    fprintf(stderr, "nativefonthandle==%p, fontname=='%s', requestname=='%s', "
            "glyphdict==%p, sizey==%u, angle==%f, "
            "complexity==%f, defaultfont==%s, fontindex==%d, "
            "refcount==%d",
            fs->nativefonthandle,
            cc_string_get_text(fs->fontname),
            cc_string_get_text(fs->requestname),
            fs->glyphdict, fs->sizey, fs->angle,
            fs->complexity,
            fs->defaultfont ? "TRUE" : "FALSE",
            fs->fontindex, fs->refcount);
#endif /* COIN_DEBUG */

    fontstruct_rmfont(fs->fontindex);
  }

  cc_dynarray_destruct(fontarray);

  fontarray = NULL;
  initialized = FALSE;

  tried_init_freetype_fontlib = tried_init_win32_fontlib = FALSE;
  fontlib_freetype_available = fontlib_win32_available = FALSE;

  CC_MUTEX_DESTRUCT(flw_global_lock);
  flw_global_font_index = 0;
}

static void
flw_initialize(void)
{
  /* CC_MUTEX_CONSTRUCT uses a global mutex to be thread safe */
  CC_MUTEX_CONSTRUCT(flw_global_lock);
  FLW_MUTEX_LOCK(flw_global_lock);

  if (initialized) {
    FLW_MUTEX_UNLOCK(flw_global_lock);
    return;
  }
  initialized = TRUE;

  fontarray = cc_dynarray_new();

  coin_atexit((coin_atexit_f *)flw_exit,
              /* priority must be higher than for atexit cleanup of
                 underlying, native interfaces (so this is done
                 first), so keep this correctly in sync with other
                 coin_atexit() invocations in the font sub-system: */
              CC_ATEXIT_FONT_SUBSYSTEM);

  FLW_MUTEX_UNLOCK(flw_global_lock);
}

/* ********************************************************************** */

/*
  Returns internal index of given font specification, for subsequent
  use as input argument to other functions as identification.

  Returns -1 if no font with the given name and size has been made
  yet.
*/
static int
flw_find_font(const char * fontname, const unsigned int sizey,
              const float angle, const float complexity)
{
  unsigned int i, n;

  /* This function is a common entry spot for first access into the
     wrapper (unless client code is doing something wrong, but then
     we'll likely catch that with an assert somewhere), so we call the
     initialization code from here.

     flw_initialize() is re-entrant, so this should be safe in a
     multithreaded environment.
  */
  if (flw_global_lock == NULL) { flw_initialize(); }

  FLW_MUTEX_LOCK(flw_global_lock);

  n = cc_dynarray_length(fontarray);
  for (i = 0; i < n; i++) {
    struct cc_flw_font * fs = (struct cc_flw_font *)cc_dynarray_get(fontarray, i);
    if ((fs->sizey == sizey) &&
        (strcmp(fontname, cc_string_get_text(fs->requestname))==0) &&
        (fs->angle == angle) &&
        (fs->complexity == complexity)) {
      FLW_MUTEX_UNLOCK(flw_global_lock);
      return fs->fontindex;
    }
  }
  FLW_MUTEX_UNLOCK(flw_global_lock);
  return -1;
}


void
cc_flw_ref_font(int fontid)
{
  int i, n;
  struct cc_flw_font * fs;
  FLW_MUTEX_LOCK(flw_global_lock);
  n = (int) cc_dynarray_length(fontarray);
  for (i = 0; i < n; i++) {
    fs = (struct cc_flw_font *)cc_dynarray_get(fontarray, i);
    if (fs->fontindex == fontid) {
      fs->refcount++;
      break;
    }
  }
  assert(i < n);
  FLW_MUTEX_UNLOCK(flw_global_lock);
}

void
cc_flw_unref_font(int fontid)
{
  int i, n;
  struct cc_flw_font * fs;

  FLW_MUTEX_LOCK(flw_global_lock);

  n = (int) cc_dynarray_length(fontarray);
  for (i = 0; i < n; i++) {
    fs = (struct cc_flw_font *)cc_dynarray_get(fontarray, i);
    if (fs->fontindex == fontid) {
      fs->refcount--;
      if (fs->refcount == 0) {
        if (!fs->defaultfont) {
          if (using_win32api()) { cc_flww32_done_font(fs->nativefonthandle); }
          else if (using_freetype()) { cc_flwft_done_font(fs->nativefonthandle); }
        }
        fontstruct_rmfont(fontid);
      }
      break;
    }
  }
  assert(i < n);

  FLW_MUTEX_UNLOCK(flw_global_lock);
}

/*!
  Set up an internal representation of a font from the \a fontname,
  and returns a font id >= 0 to be used in successive calls.

  \a fontname is the name and style \e requested. The actual font
  created will typically differ slightly, depending on what is
  available on the runtime system.  If a font has already been
  created for this \a fontname and size, the function will not create
  a duplicate, but simply return that font.

  If no font could sensibly be made from the given \a fontname, the
  internal, hard-coded default font will be used. One is therefore
  guaranteed that this function will execute and return without
  needing any error checking on behalf of the client code.
*/
int
cc_flw_get_font_id(const char * fontname, unsigned int sizey,
                   float angle, float complexity)
{
  /* FIXME: complexity (and angle, if we're keeping it) needs to be
     clamped to single-digit precision, to make sure we don't set up
     gazillions of fonts, for instance if end-user can change
     SoComplexity::value with a GUI slider component. 20050624 mortene. */

  struct cc_flw_font * fs;
  void * font;
  int idx;

  /* Don't create font if one has already been created for this name
     and size. */
  idx = flw_find_font(fontname, sizey, angle, complexity);

  if (idx != -1) { return idx; }

  font = NULL;

  FLW_MUTEX_LOCK(flw_global_lock);

  /* Avoid having the underlying font import libraries try to find a
     best match for "defaultFont" (and get a positive result for some
     random font). */
  if (strcmp(fontname, "defaultFont") != 0) {
    if (using_win32api()) {
      font = cc_flww32_get_font(fontname, sizey, angle, complexity);
    }
    else if (using_freetype()) {
     font = cc_flwft_get_font(fontname, sizey);
    }
  }

  fs = (struct cc_flw_font *)malloc(sizeof(struct cc_flw_font));
  fs->nativefonthandle = font;
  fs->defaultfont = font ? FALSE : TRUE;
  fs->complexity = complexity;
  fs->glyphdict = cc_dict_construct(256, 0.7f);
  fs->fontindex = flw_global_font_index++;
  fs->refcount = 0;
  fs->requestname = cc_string_construct_new();
  cc_string_set_text(fs->requestname, fontname);
  fs->fontname = cc_string_construct_new();
  fs->sizey = sizey;

  if (font) {
    cc_string realname;
    cc_string_construct(&realname);

    fs->angle = angle;

    if (using_win32api()) {
      cc_flww32_get_font_name(font, &realname);
    }
    else if (using_freetype()) {
      cc_flwft_set_char_size(font, sizey);
      cc_flwft_set_font_rotation(font, angle);
      cc_flwft_get_font_name(font, &realname);
    }
    else {
      assert(FALSE && "incomplete code path");
    }

    cc_string_set_text(fs->fontname, cc_string_get_text(&realname));

    if (cc_font_debug()) {
      cc_debugerror_postinfo("cc_flw_get_font",
                             "'%s', size==%u => realname='%s'",
                             fontname, sizey,
                             cc_string_get_text(&realname));
    }

    cc_string_clean(&realname);
  }
  else {
    /* Using a built-in default font for the given fontname and size,
       trying to match as close as possible to the requested size.
    */
    cc_string_set_text(fs->fontname, "defaultFont");
    fs->angle = 0.0f;
  }

  cc_dynarray_append(fontarray, fs);

  FLW_MUTEX_UNLOCK(flw_global_lock);
  return fs->fontindex;
}

/*!
  Returns a unique glyph index for the given character.
*/
unsigned int
cc_flw_get_glyph(int font, unsigned int character)
{
  unsigned int glyph = 0;
  struct cc_flw_font * fs;
  struct cc_flw_glyph * gs;

  FLW_MUTEX_LOCK(flw_global_lock);

  fs = flw_fontidx2fontptr(font);

  /* Check if it has already been set up, and if so, just return. */
  gs = flw_glyphidx2glyphptr(fs, character);

  /* FIXME: should this perhaps rather be an assert()? 20050623 mortene. */
  if (gs == NULL) {

    gs = (struct cc_flw_glyph *)malloc(sizeof(struct cc_flw_glyph));
    gs->character = character;
    gs->bitmap = NULL;
    gs->vector = NULL;
    /* These will be changed below if the glyph is found in a
       non-default font: */
    gs->nativeglyphidx = character;
    gs->fromdefaultfont = TRUE;

    if (!cc_dict_put(fs->glyphdict, character, gs)) {
      assert(0 && "glyph already exists");
    }

    if (!fs->defaultfont) {
      if (using_win32api()) { glyph = cc_flww32_get_glyph(fs->nativefonthandle, character); }
      else if (using_freetype()) { glyph = cc_flwft_get_glyph(fs->nativefonthandle, character); }

      if (glyph > 0) {
        gs->nativeglyphidx = glyph;
        gs->fromdefaultfont = FALSE;
      }
      else {
        /* Create glyph from default font, mark as default. */

        /* FIXME: shouldn't it rather be handled by making an empty
           rectangle glyph of the correct size, like it is at least done
           for X11 (and probably other systems as well)?

           Or perhaps this strategy _is_ better, but then we should at
           least scale the defaultfont glyph to the correct size.
           20030317 mortene. */

        /* Correct fix is to make an empty rectangle. Using a char from
           the default font only gives a blank since most fonts contain
           a superset of the chars in the default font. (The default
           font characters being rendered was due to a bug in the
           character mapping, causing failure to find special chars such
           as 'ØÆÅ'. The bug has since been fixed).  20030327 preng */

        if (cc_font_debug()) {
          cc_debugerror_postwarning("cc_flw_get_glyph",
                                    "no character 0x%x was found in font '%s'",
                                    character, cc_string_get_text(fs->fontname));
        }
      }
    }
  }

  FLW_MUTEX_UNLOCK(flw_global_lock);

  /* Since we're simply using the character value as the hash key into
     the glyph hash for the font: */
  return character;
}

void
cc_flw_get_bitmap_advance(int font, unsigned int glyph, int * x, int * y)
{
  struct cc_flw_font * fs;
  struct cc_flw_glyph * gs;

  FLW_MUTEX_LOCK(flw_global_lock);

  fs = flw_fontidx2fontptr(font);
  gs = flw_glyphidx2glyphptr(fs, glyph);
  assert(gs);

  /* the rest should be mt-safe, and we need to give up the lock,
     since we're calling into another cc_flw_*() function (which will
     also want to lock the mutex */
  FLW_MUTEX_UNLOCK(flw_global_lock);

  if (gs->fromdefaultfont) {
    /* FIXME: should be simplified, so we could just use the else{}
       block below unconditionally. 20050623 mortene.*/
    *x = coin_default2dfont_get_width((float)fs->sizey);
    *y = coin_default2dfont_get_height((float)fs->sizey);
  }
  else {
    struct cc_font_bitmap * bm = cc_flw_get_bitmap(font, glyph);
    assert(bm);
    *x = bm->advanceX;
    *y = bm->advanceY;
  }
}

void
cc_flw_get_vector_advance(int font, unsigned int glyph, float * x, float * y)
{
  struct cc_flw_font * fs;
  struct cc_flw_glyph * gs;

  FLW_MUTEX_LOCK(flw_global_lock);

  fs = flw_fontidx2fontptr(font);
  gs = flw_glyphidx2glyphptr(fs, glyph);
  assert(gs);

  *x = *y = 0.0f;

  if (gs->fromdefaultfont) {
    *x = coin_default3dfont_get_advance(gs->character);
  }
  else {
    if (using_win32api()) {
      cc_flww32_get_vector_advance(fs->nativefonthandle, gs->nativeglyphidx,
                                   x, y);
    }
    else if (using_freetype()) {
      cc_flwft_get_vector_advance(fs->nativefonthandle, gs->nativeglyphidx,
                                  x, y);
    }
  }

  FLW_MUTEX_UNLOCK(flw_global_lock);
}

void
cc_flw_get_bitmap_kerning(int font, unsigned int glyph1, unsigned int glyph2,
                          int * x, int * y)
{
  struct cc_flw_font * fs;
  struct cc_flw_glyph * gs1, * gs2;

  FLW_MUTEX_LOCK(flw_global_lock);

  *x = *y = 0;

  fs = flw_fontidx2fontptr(font);

  if (!fs->defaultfont) {
    gs1 = flw_glyphidx2glyphptr(fs, glyph1);
    gs2 = flw_glyphidx2glyphptr(fs, glyph2);
    assert(gs1 && gs2);
    if (using_win32api()) {
      cc_flww32_get_bitmap_kerning(fs->nativefonthandle,
                                   gs1->nativeglyphidx,
                                   gs2->nativeglyphidx, x, y);
    }
    else if (using_freetype()) {
      cc_flwft_get_bitmap_kerning(fs->nativefonthandle,
                                  gs1->nativeglyphidx,
                                  gs2->nativeglyphidx, x, y);
    }
  }

  FLW_MUTEX_UNLOCK(flw_global_lock);
}


void
cc_flw_get_vector_kerning(int font, unsigned int glyph1, unsigned int glyph2,
                          float * x, float * y)
{
  struct cc_flw_font * fs;
  struct cc_flw_glyph * gs1, * gs2;

  FLW_MUTEX_LOCK(flw_global_lock);

  fs = flw_fontidx2fontptr(font);
  gs1 = flw_glyphidx2glyphptr(fs, glyph1);
  gs2 = flw_glyphidx2glyphptr(fs, glyph2);
  assert(gs1 && gs2);

  *x = *y = 0.0f;

  if (!fs->defaultfont) {
    if (using_win32api()) {
      cc_flww32_get_vector_kerning(fs->nativefonthandle,
                                   gs1->nativeglyphidx,
                                   gs2->nativeglyphidx, x, y);
    }
    else if (using_freetype()) {
      cc_flwft_get_vector_kerning(fs->nativefonthandle,
                                  gs1->nativeglyphidx,
                                  gs2->nativeglyphidx, x, y);
    }
  }

  FLW_MUTEX_UNLOCK(flw_global_lock);
}

void
cc_flw_done_glyph(int fontidx, unsigned int glyphidx)
{
  struct cc_flw_font * fs;
  struct cc_flw_glyph * gs;

  FLW_MUTEX_LOCK(flw_global_lock);

  fs = flw_fontidx2fontptr(fontidx);
  assert(fs);

  if (cc_font_debug()) { dump_cc_flw_font("cc_flw_done_glyph", fs); }

  gs = flw_glyphidx2glyphptr(fs, glyphidx);
  assert(gs);

  if (cc_font_debug()) { dump_cc_flw_glyph("cc_flw_done_glyph", gs); }

  if (!gs->fromdefaultfont) {
    if (using_win32api()) {
      cc_flww32_done_glyph(fs->nativefonthandle, gs->nativeglyphidx);
    }
    else if (using_freetype()) {
      cc_flwft_done_glyph(fs->nativefonthandle, gs->nativeglyphidx);
    }
  }

  fontstruct_rmglyph(fs, glyphidx);

  FLW_MUTEX_UNLOCK(flw_global_lock);
}

struct cc_font_bitmap *
cc_flw_get_bitmap(int font, unsigned int glyph)
{
  unsigned char * buf;
  struct cc_flw_font * fs;
  struct cc_flw_glyph * gs;
  struct cc_font_bitmap * bm = NULL;
  unsigned int i;

  FLW_MUTEX_LOCK(flw_global_lock);

  fs = flw_fontidx2fontptr(font);
  gs = flw_glyphidx2glyphptr(fs, glyph);
  assert(gs);

  if (gs->bitmap == NULL) {

    if (!gs->fromdefaultfont) {
      if (using_win32api()) {
        bm = cc_flww32_get_bitmap(fs->nativefonthandle, gs->nativeglyphidx);
      }
      else if (using_freetype()) {
        bm = cc_flwft_get_bitmap(fs->nativefonthandle, gs->nativeglyphidx);
      }
    }

    if (!bm) {
      /* glyph handle == char value in default font. &255 to avoid
         index out of range. */
      bm = get_default_bitmap(gs->nativeglyphidx & 0xff, (float)fs->sizey);
      gs->fromdefaultfont = TRUE;
    }
    else if (bm->buffer) {
      buf = (unsigned char *)malloc(size_t(bm->pitch) * size_t(bm->rows));
      /* Copy & reverse buffer to OpenGL "up" direction. */
      for (i = 0; i < bm->rows; i++) {
        (void)memcpy(buf + i*bm->pitch,
                     bm->buffer + (bm->rows-i-1) * bm->pitch,
                     bm->pitch);
      }
      free(bm->buffer);
      bm->buffer = buf;
    }

    gs->bitmap = bm;
  }

  FLW_MUTEX_UNLOCK(flw_global_lock);

  return gs->bitmap;
}

struct cc_font_vector_glyph *
cc_flw_get_vector_glyph(int font, unsigned int glyph)
{
  struct cc_flw_font * fs;
  struct cc_flw_glyph * gs;

  FLW_MUTEX_LOCK(flw_global_lock);

  fs = flw_fontidx2fontptr(font);
  gs = flw_glyphidx2glyphptr(fs, glyph);
  assert(gs);

  if (gs->vector == NULL && !gs->fromdefaultfont) {
    struct cc_font_vector_glyph * vector_glyph = NULL;

    if (using_freetype()) {
      vector_glyph = cc_flwft_get_vector_glyph(fs->nativefonthandle,
                                               gs->nativeglyphidx,
                                               fs->complexity);
      if (!vector_glyph) { gs->fromdefaultfont = TRUE; }
    }
    else if (using_win32api()) {
      vector_glyph = cc_flww32_get_vector_glyph(fs->nativefonthandle,
                                                gs->nativeglyphidx,
                                                fs->complexity);
      if (!vector_glyph) { gs->fromdefaultfont = TRUE; }
    }

    gs->vector = vector_glyph;
  }

  FLW_MUTEX_UNLOCK(flw_global_lock);

  return gs->vector;
}

const float *
cc_flw_get_vector_glyph_coords(struct cc_font_vector_glyph * vecglyph)
{
  assert(vecglyph);
  assert(vecglyph->vertices);

  return vecglyph->vertices;
}

const int *
cc_flw_get_vector_glyph_faceidx(struct cc_font_vector_glyph * vecglyph)
{
  assert(vecglyph);
  assert(vecglyph->faceindices);

  return vecglyph->faceindices;
}

const int *
cc_flw_get_vector_glyph_edgeidx(struct cc_font_vector_glyph * vecglyph)
{
  assert(vecglyph);
  assert(vecglyph->edgeindices);

  return vecglyph->edgeindices;
}

#undef FLW_MUTEX_LOCK
#undef FLW_MUTEX_UNLOCK
