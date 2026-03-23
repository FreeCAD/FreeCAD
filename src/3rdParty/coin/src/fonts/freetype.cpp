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

/* FIXME: problem reported by Jan Peèiva on coin-dicuss: if there's a
   freetype.dll installed as part of Cygwin, and Coin has been built
   with MSVC (and not Cygwin GCC), trying to use the Cygwin FreeType
   DLL leads to a crash. Should detect this problem and avoid a
   Cygwin-built FreeType DLL (unless Coin was also built with Cygwin).

   Similar code to what is needed to implement this can be found in
   the listWin32ProcessModules() in src/misc/SoDB.cpp.

   20050613 mortene. */

#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <cstdlib>
#include <cassert>

#include "glue/freetype.h"
#include "glue/GLUWrapper.h"

#include "fonts/freetype.h"

/* ************************************************************************* */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
  Implementation note: no part of the code has to be reentrant, as the
  complete interface is protected from multiple threads accessing it
  at the same time by locking in the cc_flw_* functions (which should
  be the only callers).
*/

/* ************************************************************************* */

#if !defined(HAVE_FREETYPE) && !defined(FREETYPE_RUNTIME_LINKING)

/* Dummy versions of all functions. Only cc_flwft_initialize() will be
   used from generic wrapper. */

SbBool cc_flwft_initialize(void) { return FALSE; }
void cc_flwft_exit(void) { }

void * cc_flwft_get_font(const char * fontname, const unsigned int pixelsize) { assert(FALSE); return NULL; }
void cc_flwft_get_font_name(void * font, cc_string * str) { assert(FALSE); }
void cc_flwft_done_font(void * font) { assert(FALSE); }


int cc_flwft_get_num_charmaps(void * font) { assert(FALSE); return 0; }
const char * cc_flwft_get_charmap_name(void * font, int charmap) { assert(FALSE); return NULL; }
void cc_flwft_set_charmap(void * font, int charmap) { assert(FALSE); }
void cc_flwft_set_char_size(void * font, int height) { assert(FALSE); }


void cc_flwft_set_font_rotation(void * font, float angle) { assert(FALSE); }

int cc_flwft_get_glyph(void * font, unsigned int charidx) { assert(FALSE); return 0; }
void cc_flwft_get_vector_advance(void * font, int glyph, float *x, float *y) { assert(FALSE); }
void cc_flwft_get_vector_kerning(void * font, int glyph1, int glyph2, float *x, float *y) { assert(FALSE); }
void cc_flwft_get_bitmap_kerning(void * font, int glyph1, int glyph2, int *x, int *y) { assert(FALSE); }
void cc_flwft_done_glyph(void * font, int glyph) { assert(FALSE); }

struct cc_font_bitmap * cc_flwft_get_bitmap(void * font, unsigned int glyph) { assert(FALSE); return NULL; }
struct cc_font_vector_glyph * cc_flwft_get_vector_glyph(void * font, unsigned int glyph, float complexity) { assert(FALSE); return NULL; }

void cc_flwft_scale_vector_glyph_coords(struct cc_font_vector_glyph * vecglyph, float factor){ assert(FALSE); }


#else /* HAVE_FREETYPE || FREETYPE_RUNTIME_LINKING */

static int flwft_moveToCallback(FT_Vector * to, void * user);
static int flwft_lineToCallback(FT_Vector * to, void * user);
static int flwft_conicToCallback(FT_Vector * control, FT_Vector * to, void * user);
static int flwft_cubicToCallback(FT_Vector * control1, FT_Vector * control2,
                                 FT_Vector * to, void * user);

static void flwft_vertexCallback(GLvoid * vertex);
static void flwft_beginCallback(GLenum which);
static void flwft_endCallback(void);
static void flwft_combineCallback(GLdouble coords[3], GLvoid * data,
                                  GLfloat weight[4], int **dataOut);
static void flwft_errorCallback(GLenum error_code);
static void flwft_addTessVertex(double * vertex);

static void flwft_buildVertexList(struct cc_font_vector_glyph * newglyph);
static void flwft_buildFaceIndexList(struct cc_font_vector_glyph * newglyph);
static void flwft_buildEdgeIndexList(struct cc_font_vector_glyph * newglyph);
static void flwft_cleanupMallocList(void);

static int flwft_calctessellatorsteps(float complexity);

/* ************************************************************************* */

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stddef.h>

#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_TYPES_H
/* According to Coin user Ralf Corsepius, at least SunOS4 needs to
   include sys/types.h before netinet/in.h. There have also been a
   problem report for FreeBSD which seems to indicate that the same
   dependency exists on that platform as well. */
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#include <Inventor/C/tidbits.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/base/string.h>
#include <Inventor/C/base/list.h>

#include "base/dict.h"
#include "base/dynarray.h"
#include "base/namemap.h"
#include "fonts/common.h"

/* ************************************************************************* */

static const int flwft_3dfontsize = 40;

/* ************************************************************************* */

typedef struct flwft_tessellator_t {
  coin_GLUtessellator * tessellator_object;
  int tessellation_steps;
  SbBool contour_open;

  GLenum triangle_mode;
  int triangle_fan_root_index;
  int triangle_indices[3];
  int triangle_index_counter;
  SbBool triangle_strip_flipflop;

  int vertex_counter;
  FT_Vector last_vertex;
  float vertex_scale;
  int edge_start_vertex;

  cc_list * vertexlist;
  cc_list * faceindexlist;
  cc_list * edgeindexlist;
  cc_list * malloclist; /* to free temporary integers/memory */
} flwft_tessellator_t;

static flwft_tessellator_t flwft_tessellator;

/* ************************************************************************* */

/* FIXME: the following snippet duplicated from SoInput.cpp -- should
   collect common code. 20030604 mortene. */

/* This (POSIX-compliant) macro is missing from the Win32 API header
   files for MSVC++ 6.0. */
#ifndef S_ISDIR
/* The _S_IFDIR bitpattern is not in the POSIX standard, but MSVC++
   header files has it. */
 #ifdef _S_IFDIR
 #define S_ISDIR(s) ((s) & _S_IFDIR)
#else /* Ai. */
 #error Can neither find nor make an S_ISDIR macro to test stat structures.
#endif /* !_S_IFDIR */
#endif /* !S_ISDIR */

/* ************************************************************************* */

#ifdef FREETYPE_RUNTIME_LINKING

#ifndef FT_ENC_TAG
#define FT_ENC_TAG(value, a, b, c, d) \
          value = ( ((FT_UInt32)(a) << 24) | \
                    ((FT_UInt32)(b) << 16) | \
                    ((FT_UInt32)(c) <<  8) | \
                     (FT_UInt32)(d) )
#endif /* FT_ENC_TAG */

enum Coin_FT_Encoding {
  FT_ENC_TAG(FT_ENCODING_NONE, 0, 0, 0, 0),

  FT_ENC_TAG(FT_ENCODING_MS_SYMBOL,  's', 'y', 'm', 'b'),
  FT_ENC_TAG(FT_ENCODING_UNICODE,    'u', 'n', 'i', 'c'),

  FT_ENC_TAG(FT_ENCODING_MS_SJIS,    's', 'j', 'i', 's'),
  FT_ENC_TAG(FT_ENCODING_MS_GB2312,  'g', 'b', ' ', ' '),
  FT_ENC_TAG(FT_ENCODING_MS_BIG5,    'b', 'i', 'g', '5'),
  FT_ENC_TAG(FT_ENCODING_MS_WANSUNG, 'w', 'a', 'n', 's'),
  FT_ENC_TAG(FT_ENCODING_MS_JOHAB,   'j', 'o', 'h', 'a'),

  FT_ENC_TAG(FT_ENCODING_ADOBE_STANDARD, 'A', 'D', 'O', 'B'),
  FT_ENC_TAG(FT_ENCODING_ADOBE_EXPERT,   'A', 'D', 'B', 'E'),
  FT_ENC_TAG(FT_ENCODING_ADOBE_CUSTOM,   'A', 'D', 'B', 'C'),
  FT_ENC_TAG(FT_ENCODING_ADOBE_LATIN_1,  'l', 'a', 't', '1'),

  FT_ENC_TAG(FT_ENCODING_LATIN_2, 'l', 'a', 't', '2'),

  FT_ENC_TAG(FT_ENCODING_APPLE_ROMAN, 'a', 'r', 'm', 'n')

};

#ifndef FT_IMAGE_TAG
#define FT_IMAGE_TAG(value, _x1, _x2, _x3, _x4) \
          value = (((unsigned long)_x1 << 24) | \
                   ((unsigned long)_x2 << 16) | \
                   ((unsigned long)_x3 << 8 ) | \
                    (unsigned long)_x4 )
#endif /* FT_IMAGE_TAG */

enum  Coin_FT_Glyph_Format {
  FT_IMAGE_TAG( FT_GLYPH_FORMAT_NONE, 0, 0, 0, 0 ),

  FT_IMAGE_TAG( FT_GLYPH_FORMAT_COMPOSITE, 'c', 'o', 'm', 'p' ),
  FT_IMAGE_TAG( FT_GLYPH_FORMAT_BITMAP,    'b', 'i', 't', 's' ),
  FT_IMAGE_TAG( FT_GLYPH_FORMAT_OUTLINE,   'o', 'u', 't', 'l' ),
  FT_IMAGE_TAG( FT_GLYPH_FORMAT_PLOTTER,   'p', 'l', 'o', 't' )

};

#endif /* FREETYPE_RUNTIME_LINKING */

/* ************************************************************************* */

static FT_Library library;

/* Built-in mappings from font names to font file names.

   First item is a generic font name, then comes a list of possible
   file names (in sorted order of priority), then a NULL pointer, then
   a new generic font name, etc.
*/
static const char * const fontfilenames[] = {
  /* FIXME: different font _styles_ are just jumbled together below,
     and are not really supported. It will take some work to sort out
     that mess. 20030606 mortene. */

  /*
    Names of some TrueType font files on Microsoft Windows installations.

    FIXME: should provide more information about this -- e.g. is this
    a complete set? Ask Preng why he wrote up exactly these
    files. Where do these file names come from anyway? Just from
    peeking in $WINDIR/Fonts/? 20030606 mortene.
  */
  /*
    FIXME: under MSWin, we should use the GetFontFile() Win32 API
    method to find file names from font names, see
    <URL:http://www.codeproject.com/gdi/fontfile.asp> for an example.
    20030901 mortene.
   */

  "Arial", "arial.ttf", NULL,
  "Arial Bold", "arialbd.ttf", NULL,
  "Arial Bold Italic", "arialbi.ttf", NULL,
  "Arial Italic", "ariali.ttf", NULL,
  "Century Gothic", "gothic.ttf", NULL,
  "Century Gothic Bold", "gothicb.ttf", NULL,
  "Century Gothic Bold Italic", "gothicbi.ttf", NULL,
  "Century Gothic Italic", "gothici.ttf", NULL,
  "Courier", "cour.ttf", NULL,
  "Courier Bold", "courbd.ttf", NULL,
  "Courier Bold Italic", "courbi.ttf", NULL,
  "Courier Italic", "couri.ttf", NULL,
  "Simian", "simtoran.ttf", "simtgori.ttf", "simtchimp.ttf", NULL,
  "Times New Roman", "times.ttf", NULL,
  "Times New Roman Bold", "timesbd.ttf", NULL,
  "Times New Roman Bold Italic", "timesbi.ttf", NULL,
  "Times New Roman Italic", "timesi.ttf", NULL,
  "Verdana", "verdana.ttf", NULL,
  "Verdana Bold", "verdanab.ttf", NULL,
  "Verdana Bold Italic", "verdanaz.ttf", NULL,
  "Verdana Italic", "verdanai.ttf", NULL,
  "OpenSymbol", "opens___.ttf", NULL,
  "Small", "smalle.fon", NULL,

  /* These are the TrueType fonts installed from the Debian Linux
     "msttcorefonts" package, as of version 1.1.2. */
  "Andale Mono", "Andale_Mono.ttf", NULL,
  "Arial", "Arial.ttf", NULL,
  "Arial Black", "Arial_Black.ttf", NULL,
  "Arial Bold", "Arial_Bold.ttf", NULL,
  "Arial Bold Italic", "Arial_Bold_Italic.ttf", NULL,
  "Arial Italic", "Arial_Italic.ttf", NULL,
  "Comic Sans MS", "Comic_Sans_MS.ttf", NULL,
  "Comic Sans MS Bold", "Comic_Sans_MS_Bold.ttf", NULL,
  "Courier", "Courier_New.ttf", NULL,
  "Courier New", "Courier_New.ttf", NULL,
  "Courier New Bold", "Courier_New_Bold.ttf", NULL,
  "Courier New Bold Italic", "Courier_New_Bold_Italic.ttf", NULL,
  "Courier New Italic", "Courier_New_Italic.ttf", NULL,
  "Georgia", "Georgia.ttf", NULL,
  "Georgia Bold", "Georgia_Bold.ttf", NULL,
  "Georgia Bold Italic", "Georgia_Bold_Italic.ttf", NULL,
  "Georgia Italic", "Georgia_Italic.ttf", NULL,
  "Impact", "Impact.ttf", NULL,
  "Times", "Times_New_Roman.ttf", NULL,
  "Times New Roman", "Times_New_Roman.ttf", NULL,
  "Times New Roman Bold", "Times_New_Roman_Bold.ttf", NULL,
  "Times New Roman Bold Italic", "Times_New_Roman_Bold_Italic.ttf", NULL,
  "Times New Roman Italic", "Times_New_Roman_Italic.ttf", NULL,
  "Trebuchet MS", "Trebuchet_MS.ttf", NULL,
  "Trebuchet MS Bold", "Trebuchet_MS_Bold.ttf", NULL,
  "Trebuchet MS Bold Italic", "Trebuchet_MS_Bold_Italic.ttf", NULL,
  "Trebuchet MS Italic", "Trebuchet_MS_Italic.ttf", NULL,
  "Verdana", "Verdana.ttf", NULL,
  "Verdana Bold", "Verdana_Bold.ttf", NULL,
  "Verdana Bold Italic", "Verdana_Bold_Italic.ttf", NULL,
  "Verdana Italic", "Verdana_Italic.ttf", NULL,
  "Webdings", "Webdings.ttf", NULL
};

  struct cc_flwft_globals {
    cc_dict * fontname2filename;
    cc_dynarray * fontfiledirs;
  };

  static struct cc_flwft_globals cc_flwft_globals = {
    NULL, NULL
  };

/* ************************************************************************* */

SbBool
cc_flwft_initialize(void)
{
  FT_Error error;
  FT_Int major, minor, patch;

  if (!cc_ftglue_available()) {
    return FALSE;
  }
  error = cc_ftglue_FT_Init_FreeType(&library);
  if (error) {
    if (cc_font_debug()) cc_debugerror_post("cc_flwft_initialize", "error %d", error);
    library = NULL;
    return FALSE;
  }

  cc_ftglue_FT_Library_Version(library, &major, &minor, &patch);
  if (cc_font_debug()) {
    cc_debugerror_postinfo("cc_flwft_initialize",
                           "FreeType library version is %d.%d.%d",
                           major, minor, patch);
  }
  if (major < 2 || (major == 2 && minor < 1)) {
    cc_debugerror_post("cc_flwft_initialize",
                       "Version of Freetype 2 library is < 2.1 "
                       "Font rendering is disabled.");
    cc_ftglue_FT_Done_FreeType(library);
    library = NULL;
    return FALSE;
  }
  assert((cc_flwft_globals.fontname2filename == NULL) && "call cc_flwft_initialize only once!");

  /* Set up hash of font name to array of file name mappings. */
  cc_flwft_globals.fontname2filename = cc_dict_construct(50, 0.75);
  {
    unsigned int i = 0;
    while (i < sizeof(fontfilenames) / sizeof(fontfilenames[0])) {
      void * val;
      SbBool found, unused;
      cc_dynarray * array;
      const uintptr_t key = (uintptr_t)cc_namemap_get_address(fontfilenames[i]);

      found = cc_dict_get(cc_flwft_globals.fontname2filename, key, &val);
      if (found) {
        array = (cc_dynarray *)val;
      }
      else {
        array = cc_dynarray_new();
        unused = cc_dict_put(cc_flwft_globals.fontname2filename, key, array);
        assert(unused);
      }

      while (fontfilenames[++i] != NULL) {
        cc_dynarray_append(array, (void *)fontfilenames[i]);
      }

      i++;
    }
  }

  /* Set up where to look for font files. */
  {
    const char * env;
    char * str;
    cc_flwft_globals.fontfiledirs = cc_dynarray_new();

    if ((env = coin_getenv("COIN_FONT_PATH")) != NULL) {
      str = strdup(env);
      assert(str);
      cc_dynarray_append(cc_flwft_globals.fontfiledirs, str);
    }

#ifdef _WIN32
    if ((env = coin_getenv("WINDIR")) != NULL) {
      cc_string fullpath;
      cc_string_construct(&fullpath);

      cc_string_set_text(&fullpath, env);
      cc_string_append_text(&fullpath, "/Fonts");

      str = strdup(cc_string_get_text(&fullpath));
      assert(str);
      cc_dynarray_append(cc_flwft_globals.fontfiledirs, str);

      cc_string_clean(&fullpath);
    }
#endif /* _WIN32 */

    /* Try current working directory as well. */
    str = strdup("./");
    assert(str);
    cc_dynarray_append(cc_flwft_globals.fontfiledirs, str);
  }

  /* Setup temporary glyph-struct used during tessellation */
  flwft_tessellator.vertexlist = NULL;
  flwft_tessellator.faceindexlist = NULL;
  flwft_tessellator.edgeindexlist = NULL;
  flwft_tessellator.malloclist = NULL;

  return TRUE;
}

static void
clean_fontmap_hash(uintptr_t COIN_UNUSED_ARG(key), void * val, void * COIN_UNUSED_ARG(closure))
{
  cc_dynarray * array = (cc_dynarray *)val;
  cc_dynarray_destruct(array);
}

void
cc_flwft_exit(void)
{
  unsigned int i, n = cc_dynarray_length(cc_flwft_globals.fontfiledirs);
  for (i = 0; i < n; i++) {
    /* All string pointers should have been allocated with
       strdup(). */
    free(cc_dynarray_get(cc_flwft_globals.fontfiledirs, i));
  }
  cc_dynarray_destruct(cc_flwft_globals.fontfiledirs);
  cc_flwft_globals.fontfiledirs = NULL;

  cc_dict_apply(cc_flwft_globals.fontname2filename, clean_fontmap_hash, NULL);
  cc_dict_destruct(cc_flwft_globals.fontname2filename);
  cc_flwft_globals.fontname2filename = NULL;

  cc_ftglue_FT_Done_FreeType(library);
  library = NULL;
}

static const char *
find_font_file(const char * fontname, unsigned int pixelsize)
{
  const char * foundfile = NULL;

  /* use fontconfig to locate fonts if available */
  if (cc_fcglue_available()) {
    unsigned char * filename = NULL;
    FcPattern * font_pattern = NULL;
    FcPattern * matched_pattern = NULL;
    FcResult result;

    /* parse the fontname string to create a fontconfig pattern instance */
    if (!(font_pattern = cc_fcglue_FcNameParse((const unsigned char*)fontname))) {
      cc_debugerror_postinfo("find_font_file",
                             "fontname '%s' could not be parsed by fontconfig",
                             fontname);
      return NULL;
    }

    /* add the requested size to the pattern in order to pick up the correct font
       in case of bitmap fonts */
    if (!cc_fcglue_FcPatternAddDouble(font_pattern, FC_PIXEL_SIZE, pixelsize)) {
      cc_debugerror_postinfo("find_font_file",
                             "cc_fcglue_FcPatternAddDouble failed");
      return NULL;
    }

    /* next two steps mandatory for fontconfig's FcFontMatch call
     * otherwise results will not be correct by either the pattern
     * being underspecified or the pattern modification operations
     * given by the user not taken out. */

    /* apply pattern modification operations for those tagged as such */
    if (!cc_fcglue_FcConfigSubstitute(NULL, font_pattern, FcMatchPattern)) {
      cc_debugerror_postinfo("find_font_file",
                             "cc_fcglue_FcConfigSubstitute failed");
      return NULL;
    }

    /* supply default values for underspecified font patterns */
    cc_fcglue_FcDefaultSubstitute(font_pattern);

    /* FIXME: Should we check the result in the next call and rather
     * bail out if our font has not been found or always keep going with
     * the fallback provided by fontconfig? IMHO, everything is better
     * than the builtin font. Possibly a big fat warning that there was
     * no font found with the specified pattern would be helpful for the
     * visually impaired (like me...). 20040410 tamer. */

    /* return the font most close matching the provided pattern */
    if (!(matched_pattern = cc_fcglue_FcFontMatch(NULL, font_pattern, &result))) {
      cc_debugerror_postinfo("cc_fcglue_find_font_file",
                             "cc_fcglue_FcFontMatch failed");
      return NULL;
    }

    /* get the filename entry out of the pattern */
    if (cc_fcglue_FcPatternGetString(matched_pattern, "file", 0, &filename) != FcResultMatch) {
      cc_debugerror_postinfo("find_font_file",
                             "cc_fcglue_FcPatternGetString failed to get the fontfile");
      return NULL;
    }

    if (cc_font_debug()) {
      cc_fcglue_FcPatternPrint(matched_pattern);
    }

    foundfile = cc_namemap_get_address((const char *)filename);

    cc_fcglue_FcPatternDestroy(font_pattern);
    cc_fcglue_FcPatternDestroy(matched_pattern);

    if (cc_font_debug()) {
      cc_debugerror_postinfo("find_font_file",
                             "fontfile matching the pattern: '%s'",
                             foundfile);
    }
  }
  else {
    struct stat buf;
    cc_string str;
    unsigned int i, j, n;
    cc_dynarray * possiblefilenames;
    void * val;
    SbBool found_in_hash;
    const uintptr_t key = (uintptr_t)cc_namemap_get_address(fontname);
    found_in_hash = cc_dict_get(cc_flwft_globals.fontname2filename, key, &val);
    if (!found_in_hash) {
      const char * c = NULL;
      if (cc_font_debug()) {
        cc_debugerror_postinfo("find_font_file",
                               "fontname '%s' not found in name hash",
                               fontname);
      }
      /* If name ends in ".ttf", we interpret it as filename. */
      c = strrchr(fontname, '.');
      if (!(c && strlen(c) == 4 && strstr(c, ".ttf"))) return NULL;
    }

    possiblefilenames = (cc_dynarray *)val;
    n = found_in_hash ? cc_dynarray_length(possiblefilenames) : 1;

    cc_string_construct(&str);
    for (i = 0; i < n; i++) {
      /* FIXME: the following code is generic code for finding a file in
         a list of directories. Should move this to a new C ADT
         "cc_file" (or "cc_dir"?). If done, should also wrap the SoInput
         functions which does the same around it. 20030604 mortene. */
      const unsigned int dirs = cc_dynarray_length(cc_flwft_globals.fontfiledirs);
      for (j = 0; j < dirs; j++) {
        SbBool found = FALSE;

        cc_string_set_text(&str, (const char *)cc_dynarray_get(cc_flwft_globals.fontfiledirs, j));
        cc_string_append_char(&str, '/');
        if (found_in_hash) {
          cc_string_append_text(&str, (const char *)cc_dynarray_get(possiblefilenames, i));
        } else {
          cc_string_append_text(&str, fontname);
        }

        found = (stat(cc_string_get_text(&str), &buf) == 0) && !S_ISDIR(buf.st_mode);
        if (cc_font_debug()) {
          cc_debugerror_postinfo("find_font_file", "'%s' %s",
                                 cc_string_get_text(&str),
                                 found ? "found!" : "NOT found");
        }

        if (found) {
          /* Store permanent in global name hash. */
          foundfile = cc_namemap_get_address(cc_string_get_text(&str));
          goto done;
        }
      }
    }

  done:
    cc_string_clean(&str);
  }

  return foundfile;
}

void *
cc_flwft_get_font(const char * fontname, unsigned int pixelsize)
{
  FT_Face face;
  FT_Error error;
  const char * fontfilename = find_font_file(fontname, pixelsize);
  static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);

  error = cc_ftglue_FT_New_Face(library, fontfilename ? fontfilename : fontname, 0, &face);

  if (error) {
    if (cc_font_debug()) {
      cc_debugerror_postwarning("cc_flwft_get_font",
                                "error %d for fontname '%s' (filename '%s')",
                                error, fontname,
                                fontfilename ? fontfilename : "(null)");
    }
    return NULL;
  }

  if (cc_font_debug()) {
    cc_debugerror_postinfo("cc_flwft_get_font",
                           "FT_New_Face(..., \"%s\" / \"%s\", ...) => "
                           "family \"%s\" and style \"%s\"",
                           fontname, fontfilename ? fontfilename : "(null)",
                           face->family_name, face->style_name);
  }


  cc_flwft_set_charmap(face,
		       disable_utf8 ?
		       FT_ENCODING_ADOBE_LATIN_1 : FT_ENCODING_UNICODE);

  return face;
}

void
cc_flwft_get_font_name(void * font, cc_string * str)
{
  FT_Face face;
  assert(font);
  face = (FT_Face)font;
  cc_string_sprintf(str, "%s %s", face->family_name, face->style_name);
}

void
cc_flwft_done_font(void * font)
{
  FT_Error error;
  FT_Face face;

  assert(font);
  face = (FT_Face)font;
  error = cc_ftglue_FT_Done_Face(face);
  if (error) {
    if (cc_font_debug()) cc_debugerror_postinfo("cc_flwft_done_font", "Error %d\n", error);
  }
}

int
cc_flwft_get_num_charmaps(void * font)
{
  assert(font);
  return ((FT_Face)font)->num_charmaps;
}

const char *
cc_flwft_get_charmap_name(void * font, int charmap)
{
  FT_Face face;
  const char * name = "unknown";
  assert(font);
  face = (FT_Face)font;

  if (charmap < face->num_charmaps) {
    switch (face->charmaps[charmap]->encoding) {
    case FT_ENCODING_UNICODE:
      name = "unicode"; break;
    case FT_ENCODING_MS_SYMBOL:
      name = "symbol"; break;
    case FT_ENCODING_MS_SJIS:
      name = "sjis"; break;
    case FT_ENCODING_MS_GB2312:
      name = "gb2312"; break;
    case FT_ENCODING_MS_BIG5:
      name = "big5"; break;
    case FT_ENCODING_MS_WANSUNG:
      name = "wansung"; break;
    case FT_ENCODING_MS_JOHAB:
      name = "johab"; break;
    case FT_ENCODING_ADOBE_STANDARD:
      name = "adobe_standard"; break;
    case FT_ENCODING_ADOBE_EXPERT:
      name = "adobe_expert"; break;
    case FT_ENCODING_ADOBE_CUSTOM:
      name = "adobe_custom"; break;
    case FT_ENCODING_ADOBE_LATIN_1:
      name = "latin_1"; break;
    case FT_ENCODING_APPLE_ROMAN:
      name = "apple_roman"; break;
    default:
      if (cc_font_debug()) {
        cc_debugerror_postwarning("cc_flwft_get_charmap_name",
                                  "unknown encoding: 0x%x",
                                  face->charmaps[charmap]->encoding);
      }

      break;
    }
  }

  return name;
}

void
cc_flwft_set_charmap(void * font, int charmap)
{
  FT_Error error;
  FT_Face face;
  assert(font);
  face = (FT_Face)font;
  if (charmap < face->num_charmaps) {
    error = cc_ftglue_FT_Select_Charmap(face, face->charmaps[charmap]->encoding);
    if (error) {
      cc_debugerror_post("cc_flwft_set_charmap",
                         "FT_Select_Charmap(.., %d) returned error %d",
                         charmap, error);
    }
  }
}


void
cc_flwft_set_char_size(void * font, int height)
{
  FT_Error error;
  FT_Face face;
  assert(font);
  face = (FT_Face)font;
  /* Height size is specified in 1/64th of points.

     tamer: changed resolution values from hardcoded (...,72,72) to
     (...,0,0) where 72dpi is the current default of FreeType. just in
     case it has a different default for other platforms such as win32
     where 96dpi is the modus operandi.
  */

  /* set the size for the face by using default values of FreeType for
   * the resolution */
  error = cc_ftglue_FT_Set_Char_Size(face, 0, (height<<6), 0, 0);
  if (error) {
    cc_debugerror_post("cc_flwft_set_char_size",
                       "FT_Set_Char_Size(.., %d, %d, ..) returned error code %d",
                       height, height, error);
  }
}


void
cc_flwft_set_font_rotation(void * font, float angle)
{
  FT_Matrix matrix;
  FT_Face face;
  assert(font);
  face = (FT_Face)font;
  matrix.xx = (FT_Fixed)(cos(angle)*0x10000);
  matrix.xy = (FT_Fixed)(-sin(angle)*0x10000);
  matrix.yx = (FT_Fixed)(sin(angle)*0x10000);
  matrix.yy = (FT_Fixed)(cos(angle)*0x10000);
  cc_ftglue_FT_Set_Transform(face, &matrix, 0);
}

/*
  Returns the glyph index. If the character code is undefined, returns
  0.
*/
int
cc_flwft_get_glyph(void * font, unsigned int charidx)
{
  FT_Face face = (FT_Face)font;
  assert(face);
  return cc_ftglue_FT_Get_Char_Index(face, charidx);
}

void
cc_flwft_get_vector_advance(void * font, int glyph, float *x, float *y)
{
  FT_Error error;
  FT_Face face;
  float tmp;

  assert(font);
  face = (FT_Face)font;
  error = cc_ftglue_FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);
  assert(error == 0 && "FT_Load_Glyph() unexpected failure, investigate");

  tmp = face->glyph->advance.x * flwft_tessellator.vertex_scale;
  x[0] = (tmp / 64.0f) / flwft_3dfontsize;
  tmp = face->glyph->advance.y * flwft_tessellator.vertex_scale;
  y[0] = (tmp / 64.0f) / flwft_3dfontsize;

}

void
cc_flwft_get_bitmap_kerning(void * font, int glyph1, int glyph2, int *x, int *y)
{
  FT_Error error;
  FT_Vector kerning;
  FT_Face face;

  assert(font);
  face = (FT_Face)font;
  if (FT_HAS_KERNING(face)) {
    error = cc_ftglue_FT_Get_Kerning(face, glyph1, glyph2, ft_kerning_default, &kerning);
    if (error) {
      cc_debugerror_post("cc_flwft_get_bitmap_kerning", "FT_Get_Kerning() => %d", error);
    }

    *x = (int) (kerning.x / 64.0f);
    *y = (int) (kerning.y / 64.0f);
  }
  else {
    *x = 0;
    *y = 0;
  }
}


void
cc_flwft_get_vector_kerning(void * font, int glyph1, int glyph2, float *x, float *y)
{
  FT_Error error;
  FT_Vector kerning;
  FT_Face face;

  assert(font);
  face = (FT_Face)font;
  if (FT_HAS_KERNING(face)) {
    error = cc_ftglue_FT_Get_Kerning(face, glyph1, glyph2, ft_kerning_default, &kerning);
    if (error) {
      cc_debugerror_post("cc_flwft_get_vector_kerning", "FT_Get_Kerning() => %d", error);
    }
    *x = (kerning.x / 64.0f) / flwft_3dfontsize;
    *y = (kerning.y / 64.0f) / flwft_3dfontsize;
  }
  else {
    *x = 0.0;
    *y = 0.0;
  }
}

void
cc_flwft_done_glyph(void * COIN_UNUSED_ARG(font), int COIN_UNUSED_ARG(glyph))
{
}

struct cc_font_bitmap *
cc_flwft_get_bitmap(void * font, unsigned int glyph)
{
  FT_Error error;
  struct cc_font_bitmap * bm;
  FT_Face face;
  FT_Glyph g;
  FT_BitmapGlyph tfbmg;
  FT_Bitmap * tfbm;
  SbBool mono;

  assert(font);

  face = (FT_Face)font;

  /*
    NOTE: Do not use the combination [FT_LOAD_RENDER |
    FT_LOAD_MONOCHROME] for the 'load_flags' parameter to explicitly
    make sure a monochrome bitmap is returned. This works fine on
    Linux (tested on FreeType 2.1.4) but causes problems on MacOS X
    (reported for FreeType 2.1.5). (20031110 handegar)
  */
  error = cc_ftglue_FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);
  if (error) {
    if (cc_font_debug()) cc_debugerror_post("cc_flwft_get_bitmap",
                                            "FT_Load_Glyph() => error %d",
                                            error);
    return NULL;
  }
  error = cc_ftglue_FT_Get_Glyph(face->glyph, &g);
  if (error) {
    if (cc_font_debug()) cc_debugerror_post("cc_flwft_get_bitmap",
                                            "FT_Get_Glyph() => error %d",
                                            error);
    return NULL;
  }

  /* render a glyph only if it is in outline format. this won't be the
     case for any of the bitmap font types such as pcf. the variable
     mono needs to be set before the conversion to a 256 gray level
     bitmap in order to propagate the correct format value up the code
     path through the later bm->mono assignment. */
  mono = (g->format == FT_GLYPH_FORMAT_BITMAP);
  if (!mono) {
    error = cc_ftglue_FT_Glyph_To_Bitmap(&g, ft_render_mode_normal, 0, 1);
    if (error) {
      if (cc_font_debug()) cc_debugerror_post("cc_flwft_get_bitmap",
                                              "FT_Glyph_To_Bitmap() => error %d",
                                              error);
      return NULL;
    }
  }

  tfbmg = (FT_BitmapGlyph)g;
  tfbm = &tfbmg->bitmap;

  bm = (struct cc_font_bitmap *) malloc(sizeof(struct cc_font_bitmap));
  bm->buffer = (unsigned char *) malloc(size_t(tfbm->rows) * size_t(tfbm->pitch));
  bm->bearingX = tfbmg->left;
  bm->bearingY = tfbmg->top;
  bm->advanceX = (int)(face->glyph->advance.x / 64);
  bm->advanceY = (int)(face->glyph->advance.y / 64);
  bm->rows = tfbm->rows;
  bm->width = tfbm->width;
  bm->pitch = tfbm->pitch;
  bm->mono = mono;

  memcpy(bm->buffer, tfbm->buffer, size_t(tfbm->rows) * size_t(tfbm->pitch));
  cc_ftglue_FT_Done_Glyph(g);

  return bm;
}

struct cc_font_vector_glyph *
cc_flwft_get_vector_glyph(void * font, unsigned int glyphindex, float complexity)
{
  struct cc_font_vector_glyph * new_vector_glyph;
  FT_Outline_Funcs outline_funcs;
  FT_Error error;
  FT_Face face;
  FT_OutlineGlyph g;
  FT_Glyph tmp;
  FT_Outline outline;

  if (!GLUWrapper()->available) {
    cc_debugerror_post("cc_flwft_get_vector_glyph",
                       "GLU library could not be loaded.");
    return NULL;
  }

  if ((GLUWrapper()->gluNewTess == NULL) ||
      (GLUWrapper()->gluTessCallback == NULL) ||
      (GLUWrapper()->gluTessBeginPolygon == NULL) ||
      (GLUWrapper()->gluTessEndContour == NULL) ||
      (GLUWrapper()->gluTessEndPolygon == NULL) ||
      (GLUWrapper()->gluDeleteTess == NULL) ||
      (GLUWrapper()->gluTessVertex == NULL) ||
      (GLUWrapper()->gluTessBeginContour == NULL)) {
    cc_debugerror_post("cc_flwft_get_vector_glyph",
                       "Unable to bind required GLU tessellation "
                       "functions for 3D FreeType font support.");
    return NULL;
  }

  face = (FT_Face) font;

  error = cc_ftglue_FT_Set_Char_Size(face, 0, (flwft_3dfontsize<<6), 0, 0);
  if (error != 0) {
    /* FIXME: No message is printed here because returning NULL will
       force glyph3d.c to use the builtin font. This happens whenever
       the system cannot find the requested font. This cannot be
       detected because 'fontlib_wrapper.c:cc_flw_get_font()' will
       always return the builtin font on failure instead of an error.
       A better and more elegant workaround should be
       made... (handegar). */
    return NULL;
  }

  error = cc_ftglue_FT_Load_Glyph(face, glyphindex, FT_LOAD_DEFAULT);
  if (error != 0) {
    if (cc_font_debug()) {
      cc_debugerror_post("cc_flwft_get_vector_glyph",
                         "Error loading glyph (glyphindex==%d). "
                         "(FT_Load_Glyph() error => %d)", glyphindex, error);
    }
    return NULL;
  }

  error = cc_ftglue_FT_Get_Glyph(face->glyph, &tmp);
  if (error != 0) {
    cc_debugerror_post("cc_flwft_get_vector_glyph",
                       "Error fetching glyph. Font is not properly initialized. "
                       "(FT_Get_Glyph() error => %d)", error);
    return NULL;
  }

  /* FIXME: investigate if there is a simple way to gather the outline
     from a bitmapped font? 20040925 tamer. */
  /* in case of a bitmap font fall back to the default font.  Commonly
     it will already fail and return NULL due to not being able to set
     another character size for fixed sized fonts.  Still, rather be
     robust and catch the improbable case where the provided bitmap
     font could match the flwt_3dfontsize. */
  if (tmp->format == FT_GLYPH_FORMAT_BITMAP) {
    if (cc_font_debug()) {
      cc_debugerror_post("cc_flwft_get_vector_glyph",
                         "Glyph is a bitmap. Falling back to the default font!");
    }
    /* cleanup temporary glyph */
    cc_ftglue_FT_Done_Glyph(tmp);
    return NULL;
  }

  g = (FT_OutlineGlyph)tmp;
  outline = g->outline;

  if (flwft_tessellator.vertexlist == NULL)
     flwft_tessellator.vertexlist = cc_list_construct();
  if (flwft_tessellator.faceindexlist == NULL)
     flwft_tessellator.faceindexlist = cc_list_construct();
  if (flwft_tessellator.edgeindexlist == NULL)
     flwft_tessellator.edgeindexlist = cc_list_construct();
  if (flwft_tessellator.malloclist == NULL) {
    flwft_tessellator.malloclist = cc_list_construct();
  }

  /* FreeType callbacks */
  outline_funcs.move_to = (FT_Outline_MoveToFunc) flwft_moveToCallback;
  outline_funcs.line_to = (FT_Outline_LineToFunc) flwft_lineToCallback;
  outline_funcs.conic_to = (FT_Outline_ConicToFunc) flwft_conicToCallback;
  outline_funcs.cubic_to = (FT_Outline_CubicToFunc) flwft_cubicToCallback;
  outline_funcs.shift = 0;
  outline_funcs.delta = 0;


  flwft_tessellator.tessellator_object = GLUWrapper()->gluNewTess(); /* static object pointer */
  flwft_tessellator.contour_open = FALSE;
  flwft_tessellator.vertex_scale = 1.0f;
  flwft_tessellator.tessellation_steps = flwft_calctessellatorsteps(complexity);
  flwft_tessellator.triangle_mode = 0;
  flwft_tessellator.triangle_index_counter = 0;
  flwft_tessellator.triangle_strip_flipflop = FALSE;
  flwft_tessellator.vertex_counter = 0;

  /* gluTessellator callbacks */
  GLUWrapper()->gluTessCallback(flwft_tessellator.tessellator_object, GLU_TESS_VERTEX, (gluTessCallback_cb_t) flwft_vertexCallback);
  GLUWrapper()->gluTessCallback(flwft_tessellator.tessellator_object, GLU_TESS_BEGIN, (gluTessCallback_cb_t) flwft_beginCallback);
  GLUWrapper()->gluTessCallback(flwft_tessellator.tessellator_object, GLU_TESS_END, (gluTessCallback_cb_t) flwft_endCallback);
  GLUWrapper()->gluTessCallback(flwft_tessellator.tessellator_object, GLU_TESS_COMBINE, (gluTessCallback_cb_t) flwft_combineCallback);
  GLUWrapper()->gluTessCallback(flwft_tessellator.tessellator_object, GLU_TESS_ERROR, (gluTessCallback_cb_t) flwft_errorCallback);

  GLUWrapper()->gluTessBeginPolygon(flwft_tessellator.tessellator_object, NULL);
  /* According to the SGI doc for GLU, specifying the triangle normal
     will speed up the tessellation process. */
  GLUWrapper()->gluTessNormal(flwft_tessellator.tessellator_object, 0.0f, 0.0f, -1.0f);
  error = cc_ftglue_FT_Outline_Decompose(&outline, &outline_funcs, NULL);
  if (flwft_tessellator.contour_open) {
    GLUWrapper()->gluTessEndContour(flwft_tessellator.tessellator_object);

    cc_list_truncate(flwft_tessellator.edgeindexlist,
                     cc_list_get_length(flwft_tessellator.edgeindexlist)-1);
    cc_list_append(flwft_tessellator.edgeindexlist, (void *)(intptr_t)flwft_tessellator.edge_start_vertex);
  }

  GLUWrapper()->gluTessEndPolygon(flwft_tessellator.tessellator_object);
  /* FIXME: why not reuse the GLU tessellation object? 20060215 mortene. */
  GLUWrapper()->gluDeleteTess(flwft_tessellator.tessellator_object);

  cc_list_append(flwft_tessellator.faceindexlist, (void *) -1);
  cc_list_append(flwft_tessellator.edgeindexlist, (void *) -1);


  /* Copy the static vector_glyph struct to a newly allocated struct
     returned to the user. This is done due to the fact that the
     tessellation callback solution needs a static working struct. */
  new_vector_glyph = (struct cc_font_vector_glyph *) malloc(sizeof(struct cc_font_vector_glyph));

  flwft_buildVertexList(new_vector_glyph);
  flwft_buildFaceIndexList(new_vector_glyph);
  flwft_buildEdgeIndexList(new_vector_glyph);
  flwft_cleanupMallocList();

  /* clean up glyph in Freetype */
  cc_ftglue_FT_Done_Glyph((FT_Glyph) g);

  return new_vector_glyph;

}

static void
flwft_addTessVertex(double * vertex)
{

  int * counter;
  float * point;
  point = (float*) malloc(sizeof(float)*2);
  point[0] = flwft_tessellator.vertex_scale * ((float) vertex[0]) / 64.0f;
  point[1] = flwft_tessellator.vertex_scale * ((float) vertex[1]) / 64.0f;
  cc_list_append(flwft_tessellator.vertexlist, point);

  cc_list_append(flwft_tessellator.edgeindexlist, (void *)(intptr_t)flwft_tessellator.vertex_counter);

  counter = (int *) malloc(sizeof(int));
  cc_list_append(flwft_tessellator.malloclist, counter); /* to avoid mem leaks */
  counter[0] = flwft_tessellator.vertex_counter++;
  GLUWrapper()->gluTessVertex(flwft_tessellator.tessellator_object, vertex, counter);


  cc_list_append(flwft_tessellator.edgeindexlist, (void *)(intptr_t)flwft_tessellator.vertex_counter);

}

/* "move to" in this context means that a previous contour should be
   terminated, as the virtual "pencil" is moved to start a new
   contour. (so "move to" means "lift pencil, jump to and start new
   contour".) */
static int
flwft_moveToCallback(FT_Vector * to, void * COIN_UNUSED_ARG(user))
{

  if (flwft_tessellator.contour_open) {
    GLUWrapper()->gluTessEndContour(flwft_tessellator.tessellator_object);
    cc_list_truncate(flwft_tessellator.edgeindexlist,
                     cc_list_get_length(flwft_tessellator.edgeindexlist)-1);
    cc_list_append(flwft_tessellator.edgeindexlist, (void *)(intptr_t)flwft_tessellator.edge_start_vertex);
  }

  flwft_tessellator.last_vertex.x = to->x;
  flwft_tessellator.last_vertex.y = to->y;

  GLUWrapper()->gluTessBeginContour(flwft_tessellator.tessellator_object);
  flwft_tessellator.edge_start_vertex = flwft_tessellator.vertex_counter;

  flwft_tessellator.contour_open = TRUE;
  return 0;
}

/* "line to" in this context means that the current contour is
   continued with a line to the given point. */
static int
flwft_lineToCallback(FT_Vector * to, void * COIN_UNUSED_ARG(user))
{

  double vertex[3];

  flwft_tessellator.last_vertex.x = to->x;
  flwft_tessellator.last_vertex.y = to->y;

  vertex[0] = to->x;
  vertex[1] = to->y;
  vertex[2] = 0;

  flwft_addTessVertex(vertex);

  return 0;
}

static int
flwft_conicToCallback(FT_Vector * control, FT_Vector * to, void * COIN_UNUSED_ARG(user))
{

  int i;
  double vertex[3];
  double b[2], c[2], d[2], f[2], df[2], df2[2];
  double spline_delta, spline_delta2;

  spline_delta = 1. / (double)flwft_tessellator.tessellation_steps;
  spline_delta2 = spline_delta * spline_delta;

  b[0] = flwft_tessellator.last_vertex.x - (2*control->x) + to->x;
  b[1] = flwft_tessellator.last_vertex.y - (2*control->y) + to->y;

  c[0] = 2*(-flwft_tessellator.last_vertex.x + control->x);
  c[1] = 2*(-flwft_tessellator.last_vertex.y + control->y);

  d[0] = (flwft_tessellator.last_vertex.x);
  d[1] = (flwft_tessellator.last_vertex.y);

  f[0] = d[0];
  f[1] = d[1];

  df[0] = c[0] * spline_delta + b[0] * spline_delta2;
  df[1] = c[1] * spline_delta + b[1] * spline_delta2;
  df2[0] = 2 * b[0] * spline_delta2;
  df2[1] = 2 * b[1] * spline_delta2;

  for (i=0;i<flwft_tessellator.tessellation_steps - 1;i++) {

    f[0] += df[0];
    f[1] += df[1];

    vertex[0] = f[0];
    vertex[1] = f[1];
    vertex[2] = 0;
    flwft_addTessVertex(vertex);

    df[0] += df2[0];
    df[1] += df2[1];

  }

  vertex[0] = to->x;
  vertex[1] = to->y;
  vertex[2] = 0;
  flwft_addTessVertex(vertex);

  flwft_tessellator.last_vertex.x = to->x;
  flwft_tessellator.last_vertex.y = to->y;

  return 0;
}

static int
flwft_cubicToCallback(FT_Vector * control1, FT_Vector * control2, FT_Vector * to, void * COIN_UNUSED_ARG(user))
{

  /*
    FIXME: Cubic splines are not tested due to the fact that I
    haven't managed to find any fonts which contains other spline
    types than conic splines. (20030905 handegar)
  */

  int i;
  double vertex[3];
  double a[2], b[2], c[2], d[2], f[2], df[2], df2[2], df3[2];
  double spline_delta, spline_delta2, spline_delta3;

  spline_delta = 1. / (double)flwft_tessellator.tessellation_steps;
  spline_delta2 = spline_delta * spline_delta;
  spline_delta3 = spline_delta2 * spline_delta;


  a[0] = -flwft_tessellator.last_vertex.x + 3*control1->x - 3*control2->x + to->x;
  a[1] = -flwft_tessellator.last_vertex.y + 3*control1->y - 3*control2->y + to->y;

  b[0] = 3 * flwft_tessellator.last_vertex.x - 6*control1->x + 3*control2->x;
  b[1] = 3 * flwft_tessellator.last_vertex.y - 6*control1->y + 3*control2->y;

  c[0] = -3 * flwft_tessellator.last_vertex.x + 3 * control1->x;
  c[1] = -3 * flwft_tessellator.last_vertex.y + 3 * control1->y;

  d[0] = flwft_tessellator.last_vertex.x;
  d[1] = flwft_tessellator.last_vertex.y;

  f[0] = d[0];
  f[1] = d[1];

  df[0] = c[0] * spline_delta + b[0]*spline_delta2 + a[0]*spline_delta3;
  df[1] = c[1] * spline_delta + b[1]*spline_delta2 + a[1]*spline_delta3;

  df2[0] = 2 * b[0] * spline_delta2 + 6 * a[0] * spline_delta3;
  df2[1] = 2 * b[1] * spline_delta2 + 6 * a[1] * spline_delta3;

  df3[0] = 6 * a[0] * spline_delta3;
  df3[1] = 6 * a[1] * spline_delta3;


  for (i=0; i < flwft_tessellator.tessellation_steps-1; i++) {

    f[0] += df[0];
    f[1] += df[1];

    vertex[0] = f[0];
    vertex[1] = f[1];
    vertex[2] = 0;

    flwft_addTessVertex(vertex);

    df[0] += df2[0];
    df[1] += df2[1];
    df2[0] += df3[0];
    df2[1] += df3[1];
  }

  vertex[0] = to->x;
  vertex[1] = to->y;
  vertex[2] = 0;
  flwft_addTessVertex(vertex);

  flwft_tessellator.last_vertex.x = to->x;
  flwft_tessellator.last_vertex.y = to->y;

  return 0;
}



static void
flwft_vertexCallback(GLvoid * data)
{

  int index = ((int *) data)[0];

  if ((flwft_tessellator.triangle_fan_root_index == -1) &&
     (flwft_tessellator.triangle_index_counter == 0)) {
    flwft_tessellator.triangle_fan_root_index = index;
  }

  if (flwft_tessellator.triangle_mode == GL_TRIANGLE_FAN) {
    if (flwft_tessellator.triangle_index_counter == 0) {
      flwft_tessellator.triangle_indices[0] = flwft_tessellator.triangle_fan_root_index;
      flwft_tessellator.triangle_indices[1] = index;
      ++flwft_tessellator.triangle_index_counter;
    }
    else flwft_tessellator.triangle_indices[flwft_tessellator.triangle_index_counter++] = index;
  }
  else {
    flwft_tessellator.triangle_indices[flwft_tessellator.triangle_index_counter++] = index;
  }

  assert(flwft_tessellator.triangle_index_counter < 4);

  if (flwft_tessellator.triangle_index_counter == 3) {


    if (flwft_tessellator.triangle_mode == GL_TRIANGLE_STRIP) {
      if (flwft_tessellator.triangle_strip_flipflop) {
        index = flwft_tessellator.triangle_indices[1];
        flwft_tessellator.triangle_indices[1] = flwft_tessellator.triangle_indices[2];
        flwft_tessellator.triangle_indices[2] = index;
      }
    }

    /* FIXME: Using pointers as storage for INTs it not nice. This
       might cause a lot of problems when compiling on a 64 bits
       platform. A special cc_list for integers would be a good
       solution (Reported by mortene). (20030914 handegar) */
    cc_list_append(flwft_tessellator.faceindexlist,
                   (void *)(intptr_t)flwft_tessellator.triangle_indices[0]);
    cc_list_append(flwft_tessellator.faceindexlist,
                   (void *)(intptr_t)flwft_tessellator.triangle_indices[1]);
    cc_list_append(flwft_tessellator.faceindexlist,
                   (void *)(intptr_t)flwft_tessellator.triangle_indices[2]);

   if (flwft_tessellator.triangle_mode == GL_TRIANGLE_FAN) {
      flwft_tessellator.triangle_indices[1] = flwft_tessellator.triangle_indices[2];
      flwft_tessellator.triangle_index_counter = 2;
    }

    else if (flwft_tessellator.triangle_mode == GL_TRIANGLE_STRIP) {

      if (flwft_tessellator.triangle_strip_flipflop) {
        index = flwft_tessellator.triangle_indices[1];
        flwft_tessellator.triangle_indices[1] = flwft_tessellator.triangle_indices[2];
        flwft_tessellator.triangle_indices[2] = index;
      }

      flwft_tessellator.triangle_indices[0] = flwft_tessellator.triangle_indices[1];
      flwft_tessellator.triangle_indices[1] = flwft_tessellator.triangle_indices[2];
      flwft_tessellator.triangle_index_counter = 2;
      flwft_tessellator.triangle_strip_flipflop = !flwft_tessellator.triangle_strip_flipflop;

    } else flwft_tessellator.triangle_index_counter = 0;

  }

}

static void
flwft_beginCallback(GLenum which)
{
  flwft_tessellator.triangle_mode = which;  
  flwft_tessellator.triangle_fan_root_index = (which==GL_TRIANGLE_FAN) ? -1 : 0;
  flwft_tessellator.triangle_index_counter = 0;
  flwft_tessellator.triangle_strip_flipflop = FALSE;
}

static void
flwft_endCallback(void)
{}

static void
flwft_combineCallback(GLdouble coords[3], GLvoid * COIN_UNUSED_ARG(vertex_data), GLfloat /*weight*/[4], int **dataOut)
{
  /* FIXME: I believe this callback may not actually be necessary.
     20060227 mortene. */

  int * ret;
  float * point;

  point = (float*) malloc(sizeof(float)*2);
  point[0] = flwft_tessellator.vertex_scale * ((float) coords[0]) / 64.0f;
  point[1] = flwft_tessellator.vertex_scale * ((float) coords[1]) / 64.0f;

  cc_list_append(flwft_tessellator.vertexlist, point);

  ret = (int*) malloc(sizeof(int));
  cc_list_append(flwft_tessellator.malloclist, ret); /* to avoid mem leaks */
  ret[0] = flwft_tessellator.vertex_counter++;

  *dataOut = ret;
}

static void
flwft_errorCallback(GLenum errcode)
{
  const GLubyte * str = GLUWrapper()->gluGetString(errcode);
  cc_debugerror_post("flwft_errorCallback",
                     "Got following error from GLU when tessellating FreeType glyph: '%s'",
                     str);
}

static void
flwft_cleanupMallocList(void)
{
  int i, n;
  if (flwft_tessellator.malloclist) {
    n = cc_list_get_length(flwft_tessellator.malloclist);
    for (i = 0; i < n; i++) {
      free(cc_list_get(flwft_tessellator.malloclist, i));
    }
    cc_list_destruct(flwft_tessellator.malloclist);
    flwft_tessellator.malloclist = NULL;
  }
}

static void
flwft_buildVertexList(struct cc_font_vector_glyph * newglyph)
{
  int numcoords,i;
  float * coord;

  assert(flwft_tessellator.vertexlist && "Error fetching vector glyph coordinates");
  numcoords = cc_list_get_length(flwft_tessellator.vertexlist);

  newglyph->vertices = (float *) malloc(sizeof(float)*numcoords*2);

  for (i=0;i<numcoords;++i) {
    coord = (float *) cc_list_get(flwft_tessellator.vertexlist,i);
    newglyph->vertices[i*2 + 0] = coord[0] / flwft_3dfontsize;
    newglyph->vertices[i*2 + 1] = coord[1] / flwft_3dfontsize;
    free(coord);
  }

  cc_list_destruct(flwft_tessellator.vertexlist);
  flwft_tessellator.vertexlist = NULL;
}

static void
flwft_buildEdgeIndexList(struct cc_font_vector_glyph * newglyph)
{
  int i,len;

  assert(flwft_tessellator.edgeindexlist);

  len = cc_list_get_length(flwft_tessellator.edgeindexlist);
  newglyph->edgeindices = (int *) malloc(sizeof(int)*len);

  for (i=0;i<len;++i)
    newglyph->edgeindices[i] = (int) ((intptr_t) cc_list_get(flwft_tessellator.edgeindexlist, i));

  cc_list_destruct(flwft_tessellator.edgeindexlist);
  flwft_tessellator.edgeindexlist = NULL;
}

static void
flwft_buildFaceIndexList(struct cc_font_vector_glyph * newglyph)
{
  int len,i;

  assert(flwft_tessellator.faceindexlist);

  len = cc_list_get_length(flwft_tessellator.faceindexlist);
  newglyph->faceindices = (int *) malloc(sizeof(int)*len);

  for (i=0;i<len;++i)
    newglyph->faceindices[i] = (int) ((intptr_t) cc_list_get(flwft_tessellator.faceindexlist, i));

  cc_list_destruct(flwft_tessellator.faceindexlist);
  flwft_tessellator.faceindexlist = NULL;
}

static int
flwft_calctessellatorsteps(float complexity)
{
  /* Default is 5 steps. Minimum is zero step, maximum is 10. */
  return (int)  (16 * complexity);
}

#endif /* HAVE_FREETYPE */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
