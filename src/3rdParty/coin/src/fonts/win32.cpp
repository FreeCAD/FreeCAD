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

#include "coindefs.h"
#include "fonts/win32.h"

#include <cstdlib>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <Inventor/C/base/list.h>

#include "glue/GLUWrapper.h"

/* ************************************************************************* */

/*
  Implementation note: no part of the code has to be reentrant, as the
  complete interface is protected from multiple threads accessing it
  at the same time by locking in the cc_flw_* functions (which should
  be the only callers).
*/

/*
  Resources:

  - <URL:http://www.codeproject.com/gdi/> (contains many programming
  examples for font queries, rendering and other handling)
*/

/* ************************************************************************* */

#ifndef HAVE_WIN32_API

/* Dummy versions of all functions. Only cc_flww32_initialize() will be
   used from generic wrapper. */

SbBool cc_flww32_initialize(void) { return FALSE; }
void cc_flww32_exit(void) { }

void * cc_flww32_get_font(const char * COIN_UNUSED_ARG(fontname), int COIN_UNUSED_ARG(sizey), float COIN_UNUSED_ARG(angle), float COIN_UNUSED_ARG(complexity)) { assert(FALSE); return NULL; }
void cc_flww32_get_font_name(void * COIN_UNUSED_ARG(font), cc_string * COIN_UNUSED_ARG(str)) { assert(FALSE); }
void cc_flww32_done_font(void * COIN_UNUSED_ARG(font)) { assert(FALSE); }

int cc_flww32_get_glyph(void * COIN_UNUSED_ARG(font), unsigned int COIN_UNUSED_ARG(charidx)) { assert(FALSE); return 0; }
void cc_flww32_get_vector_advance(void * COIN_UNUSED_ARG(font), int COIN_UNUSED_ARG(glyph), float * COIN_UNUSED_ARG(x), float * COIN_UNUSED_ARG(y)) { assert(FALSE); }
void cc_flww32_get_bitmap_kerning(void * COIN_UNUSED_ARG(font), int COIN_UNUSED_ARG(glyph1), int COIN_UNUSED_ARG(glyph2), int * COIN_UNUSED_ARG(x), int * COIN_UNUSED_ARG(y)) { assert(FALSE); }
void cc_flww32_get_vector_kerning(void * COIN_UNUSED_ARG(font), int COIN_UNUSED_ARG(glyph1), int COIN_UNUSED_ARG(glyph2), float * COIN_UNUSED_ARG(x), float * COIN_UNUSED_ARG(y)) { assert(FALSE); }
void cc_flww32_done_glyph(void * COIN_UNUSED_ARG(font), int COIN_UNUSED_ARG(glyph)) { assert(FALSE); }

struct cc_font_bitmap * cc_flww32_get_bitmap(void * COIN_UNUSED_ARG(font), int COIN_UNUSED_ARG(glyph)) { assert(FALSE); return NULL; }
struct cc_font_vector_glyph * cc_flww32_get_vector_glyph(void * COIN_UNUSED_ARG(font), unsigned int COIN_UNUSED_ARG(glyph), float COIN_UNUSED_ARG(complexity)){ assert(FALSE); return NULL; }


#else /* HAVE_WIN32_API */


static void CALLBACK flww32_vertexCallback(GLvoid * vertex);
static void CALLBACK flww32_beginCallback(GLenum which);
static void CALLBACK flww32_endCallback(void);
static void CALLBACK flww32_combineCallback(GLdouble coords[3], GLvoid * data, GLfloat weight[4], int **dataOut);
static void CALLBACK flww32_errorCallback(GLenum error_code);
static void flww32_addTessVertex(double x, double y);

static void flww32_buildVertexList(struct cc_font_vector_glyph * newglyph, int size);
static void flww32_buildFaceIndexList(struct cc_font_vector_glyph * newglyph);
static void flww32_buildEdgeIndexList(struct cc_font_vector_glyph * newglyph);
static void flww32_cleanupMallocList(void);
static int flww32_calcfontsize(float complexity);


/* ************************************************************************* */

#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstddef>
#include <cctype>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#undef WIN32_LEAN_AND_MEAN

#include <Inventor/C/tidbits.h>
#include <Inventor/C/base/string.h>

#include "base/dict.h"
#include "glue/win32api.h"
#include "fonts/common.h"

#include <Inventor/errors/SoDebugError.h>

/* ************************************************************************* */

typedef struct flww32_tessellator_t {
  coin_GLUtessellator * tessellator_object;
  SbBool contour_open;

  GLenum triangle_mode;
  int triangle_fan_root_index;
  int triangle_indices[3];
  int triangle_index_counter;
  SbBool triangle_strip_flipflop;

  int vertex_counter;
  float vertex_scale;
  int edge_start_vertex;

  cc_list * vertexlist;
  cc_list * faceindexlist;
  cc_list * edgeindexlist;
  cc_list * malloclist; /* to free temporary integers/memory */
} flww32_tessellator_t;

static flww32_tessellator_t flww32_tessellator;
/* A static flag indicating whether we are running an old version of
   windows or not. This is important when tessellating the glyphs due
   to different behaviour between 95/98/Me and newer versions. */
static SbBool flww32_win9598Me = FALSE;

struct cc_flww32_globals_s {
  /* Offscreen device context for connecting to fonts. */
  HDC devctx;

  /* This is a hash of hashes. Unique keys are HFONT instances. This again
     contains hashes for each glyph which contain a hash for its
     pairing glyphs. This again contains the kerning value for that pair. */
  cc_dict * font2kerninghash;
  cc_dict * fontsizehash;
};

static struct cc_flww32_globals_s cc_flww32_globals = {
  NULL, /* devctx */
  NULL,
  NULL
};

/* Callback functions for cleaning up kerninghash table */
void cc_flww32_kerninghash_deleteCB1(uintptr_t key, void * val, void * closure);
void cc_flww32_kerninghash_deleteCB2(uintptr_t key, void * val, void * closure);
void cc_flww32_kerninghash_deleteCB3(uintptr_t key, void * val, void * closure);

/* ************************************************************************* */

/* dumps debug information about one of the system fonts */
static int CALLBACK
font_enum_proc(ENUMLOGFONTEX * logicalfont, NEWTEXTMETRICEX * physicalfont,
               int fonttype, LPARAM userdata)
{
  cc_string str;
  cc_string_construct(&str);

  cc_string_sprintf(&str,
                    "fontenum: fullname=='%s', style=='%s', script=='%s' "
                    "<h, w>==<%d, %d>",
                    logicalfont->elfFullName, logicalfont->elfStyle,
                    logicalfont->elfScript,
                    logicalfont->elfLogFont.lfHeight,
                    logicalfont->elfLogFont.lfWidth);

  cc_string_append_text(&str, " -- fonttype: ");
  switch (fonttype) {
  case DEVICE_FONTTYPE: cc_string_append_text(&str, "DEVICE"); break;
  case RASTER_FONTTYPE: cc_string_append_text(&str, "RASTER"); break;
  case TRUETYPE_FONTTYPE: cc_string_append_text(&str, "TRUETYPE"); break;
  default: cc_string_append_text(&str, "<unknown>"); break;
  }

  SoDebugError::postInfo("font_enum_proc", "%s", cc_string_get_text(&str));

  cc_string_clean(&str);
  return 1; /* non-0 to continue enumeration */
}

/* ************************************************************************* */

SbBool
cc_flww32_initialize(void)
{
  OSVERSIONINFO osvi;
  UINT previous;

  cc_flww32_globals.devctx = CreateDC("DISPLAY", NULL, NULL, NULL);
  if (cc_flww32_globals.devctx == NULL) {
    cc_win32_print_error("cc_flww32_initialize", "CreateDC()", GetLastError());
    return FALSE;
  }

  if (cc_font_debug()) { /* list all fonts on system */
    LOGFONT logfont; /* logical font information */

    /* Only these are inspected by the EnumFontFamiliesEx()
       function. Set up to list as many fonts as possible. */
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfFaceName[0] = '\0';
    logfont.lfPitchAndFamily = 0;

    (void)EnumFontFamiliesEx(cc_flww32_globals.devctx,
                             (LPLOGFONT)&logfont,
                             (FONTENUMPROC)font_enum_proc,
                             0, /* user data for callback */
                             0); /* reserved, must be zero */
  }

  /* Draw fonts on the baseline. (If we don't do this, we will for
     instance get placement mismatches when rendering text in
     different fonts on the same line.) */
  previous = SetTextAlign(cc_flww32_globals.devctx, TA_BASELINE);
  assert(previous != GDI_ERROR);

  cc_flww32_globals.font2kerninghash = cc_dict_construct(17, 0.75);
  cc_flww32_globals.fontsizehash = cc_dict_construct(17, 0.75f);

  /* Setup temporary glyph-struct used during for tessellation */
  flww32_tessellator.vertexlist = NULL;
  flww32_tessellator.faceindexlist = NULL;
  flww32_tessellator.edgeindexlist = NULL;
  flww32_tessellator.malloclist = NULL;

  /* Are we running Windows 95/98/Me? */
  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  cc_win32()->GetVersionEx(&osvi);

  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    flww32_win9598Me = TRUE;

  return TRUE;
}

void
cc_flww32_exit(void)
{
  BOOL ok;

  /* FIXME: this hash should be empty at this point? 20030930 handegar */
  cc_dict_destruct(cc_flww32_globals.font2kerninghash);
  cc_dict_destruct(cc_flww32_globals.fontsizehash);

  ok = DeleteDC(cc_flww32_globals.devctx);
  if (!ok) {
    cc_win32_print_error("cc_flww32_exit", "DeleteDC()", GetLastError());
  }
}

/* Callbacks for kerninghash delete */
void
cc_flww32_kerninghash_deleteCB1(uintptr_t key, void * val, void * closure)
{
  cc_dict * khash;
  khash = (cc_dict *) val;
  cc_dict_apply(khash, cc_flww32_kerninghash_deleteCB2, NULL);
  cc_dict_destruct(khash);
}
void
cc_flww32_kerninghash_deleteCB2(uintptr_t key, void * val, void * closure)
{
  cc_dict * khash;
  khash = (cc_dict *) val;
  cc_dict_apply(khash, cc_flww32_kerninghash_deleteCB3, NULL);
  cc_dict_destruct(khash);
}
void
cc_flww32_kerninghash_deleteCB3(uintptr_t key, void * val, void * closure)
{
  float * kerning;
  kerning = (float *) val;
  free(kerning);
}

namespace {

  class FontContext {
  public:
    FontContext(HDC dc, HFONT font, const char * identifier);
    ~FontContext();
  protected:
    HFONT font;
    HFONT previous;
    HDC dc;
    bool valid;
    const char * identifier;
  public:
    bool isValid() { return this->valid; }
  };

  FontContext::FontContext(HDC dc, HFONT font, const char * identifier)
  {
    this->valid = false;
    this->dc = dc; 
    this->font = font;
    this->identifier = identifier;
    SetLastError(0);
    this->previous = static_cast<HFONT>(SelectObject(this->dc, font));
    if (this->previous == NULL){
      DWORD lastError = GetLastError();
      SoDebugError::post(this->identifier, 
        "Could not select font. HDC: %p, HFONT: %p", this->dc, this->font);

      /* NOTE: 
      Apparently SelectObject only affects GetLastError if hdc was invalid.
      If that was not the case, the error message returned from GetLastError
      will be the last error that occurred *before* our call to SelectObject().
      wiesener 20090310
      */
      if (lastError){
        cc_win32_print_error(this->identifier, "SelectObject()", lastError);
      }
    }
    else {
      this->valid = true;
    }
  }

  FontContext::~FontContext()
  {
    if (this->valid){
      SetLastError(0);
      HFONT font = static_cast<HFONT>(SelectObject(this->dc, this->previous));
      if (font != this->font){
        DWORD lastError = GetLastError();
        SoDebugError::post(this->identifier, 
          "Could not revert font. HDC: %p, HFONT: %p", this->dc, this->previous);

        /* See note above about GetLastError() */
        if (lastError){
          cc_win32_print_error(this->identifier, "SelectObject()", lastError);
        }
      }
    }
  }

} //end anonymous namespace 

/*
  Wrapper for
  int tolower(int)
*/
static char cc_flww32_chartolower(char c)
{
  return (char)tolower(c);
}

/*
  Simply passes the parameters to the GDI function CreateFontW() and returns the result
*/
static HFONT cc_flww32_create_font(const char* fontname, int sizey,
                                   float angle, BOOL bold, BOOL italic)
{
  HFONT font;
  static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);

  if (disable_utf8) {
    font = CreateFont(-sizey,
		      /* Using a negative 'sizey'. Otherwise
			 leads to less details as it seems like
			 the Win32 systems tries to 'quantize'
			 the glyph to match the pixels of the
			 chosen resolution. */
		      0, /* let Win32 choose to get correct aspect ratio */
		      (int) (10 * (angle * 180) / M_PI) , /* escapement */
		      (int) (10 * (angle * 180) / M_PI) , /* orientation */
		      bold ? FW_BOLD : FW_DONTCARE, /* weight */
		      italic, FALSE, FALSE, /* italic, underline, strikeout */
		      /* FIXME: using DEFAULT_CHARSET is probably not
			 what we should do, AFAICT from a quick
			 read-over of the CreateFont() API
			 doc. 20030530 mortene. */
		      DEFAULT_CHARSET,
		      /* FIXME: to also make it possible to use
			 Window's raster fonts, this should rather be
			 OUT_DEFAULT_PRECIS. Then when
			 GetGlyphOutline() fails on a font, we should
			 grab its bitmap by using TextOut() and
			 GetDIBits(). 20030610 mortene.
		      */
		      OUT_TT_ONLY_PRECIS, /* output precision */
		      CLIP_DEFAULT_PRECIS, /* clipping precision */
		      PROOF_QUALITY, /* output quality */
		      DEFAULT_PITCH, /* pitch and family */
		      fontname); /* typeface name */
  } else {
    // convert font name from UTF-8 to UTF-16
    wchar_t fontnameW[256];
    fontnameW[0] = 0;
    ::MultiByteToWideChar(CP_UTF8, 0, fontname, -1, fontnameW, sizeof(fontnameW) / sizeof(wchar_t));

    font = CreateFontW(-sizey,
		       /* Using a negative 'sizey'. Otherwise
			  leads to less details as it seems like
			  the Win32 systems tries to 'quantize'
			  the glyph to match the pixels of the
			  chosen resolution. */
		       0, /* let Win32 choose to get correct aspect ratio */
		       (int) (10 * (angle * 180) / M_PI) , /* escapement */
		       (int) (10 * (angle * 180) / M_PI) , /* orientation */
		       bold ? FW_BOLD : FW_DONTCARE, /* weight */
		       italic, FALSE, FALSE, /* italic, underline, strikeout */
		       /* FIXME: using DEFAULT_CHARSET is probably not
			  what we should do, AFAICT from a quick
			  read-over of the CreateFontW() API
			  doc. 20030530 mortene. */
		       DEFAULT_CHARSET,
		       /* FIXME: to also make it possible to use
			  Window's raster fonts, this should rather be
			  OUT_DEFAULT_PRECIS. Then when
			  GetGlyphOutlineW() fails on a font, we should
			  grab its bitmap by using TextOutW() and
			  GetDIBits(). 20030610 mortene.
		       */
		       OUT_TT_ONLY_PRECIS, /* output precision */
		       CLIP_DEFAULT_PRECIS, /* clipping precision */
		       PROOF_QUALITY, /* output quality */
		       DEFAULT_PITCH, /* pitch and family */
		       fontnameW); /* typeface name */
  }

  if (!font) {
    DWORD lasterr = GetLastError();
    cc_string * str = cc_string_construct_new();
    cc_string_sprintf(str, "CreateFontW(%d, ..., '%s')", sizey, fontname);
    cc_win32_print_error("cc_flww32_get_font", cc_string_get_text(str), lasterr);
    cc_string_destruct(str);
    return NULL;
  }
  return font;
}

/* ************************************************************************* */

/* Allocates and returns a new font id matching the exact fontname.
   Returns NULL on error.
*/
void *
cc_flww32_get_font(const char * fontname, int sizey, float angle, float complexity)
{
  int i;
  DWORD nrkpairs, ret;
  KERNINGPAIR * kpairs;
  cc_dict * khash;
  cc_dict * fontkerninghash;
  float * kerningvalue;
  HFONT wfont, wfont2;

  cc_string * realname;
  cc_string * namelower;
  cc_string * basename;
  const char * tmp;
  const char * bold;
  const char * italic;
  int baselen;

  /* FIXME: glyph2d.c will pass in complexity==-1.0f, so this can be
     used to detect that the font will be used for 2D font rendering
     -- as sizey must be scaled up for fonts used for 3D
     vectorization, or the resolution will be too low. This is a
     rather ugly hack -- fix. 20050706 mortene. */
  if (complexity >= 0.0f) {
    sizey = flww32_calcfontsize(complexity);
  }

  wfont = cc_flww32_create_font(fontname, sizey, angle, FALSE, FALSE);
  if (!wfont) {
    return NULL;
  }

  /*
  Some windows fonts (like "Courier New") have separate entries for the bold
  ("Courier New Bold"), italic ("Courier New Italic") and bold+italic ("Courier New Bold Italic")
  versions, and will work with the previous approach. Others (like "Lucida Console")
  use the same font for all the versions, and will not be found if we specify bold/italic as
  part of the font name, instead we get a default font back from GDI (Arial plain). This is also
  the case if the parameters are specified in the wrong order (i.e. "Courier New Italic Bold").
  (wiesener 20071010)
  */

  realname = cc_string_construct_new();
  cc_flww32_get_font_name(wfont, realname);

  size_t fontnamelen = strlen(fontname);
  if (cc_string_length(realname) != fontnamelen ||
      coin_strncasecmp(cc_string_get_text(realname), fontname, (int)fontnamelen)) {
    /*
    The names are different, we probably got a bogus font (Arial plain)
    Let's try stripping the bold/italic part from the font name and set those as flags instead
    */
    if (cc_font_debug()) {
      SoDebugError::postInfo("cc_flww32_get_font",
        "Tried fetching '%s', got '%s'. Will try stripping bold/italic.",
        fontname, cc_string_get_text(realname));
    }

    namelower = cc_string_construct_new();
    cc_string_set_text(namelower, fontname);
    cc_string_apply(namelower, cc_flww32_chartolower);

    tmp = cc_string_get_text(namelower);

    bold = strstr(tmp, " bold");
    italic = strstr(tmp, " italic");
    /* FIXME: Should we also try to handle fontconfig style font specification,
       i.e. "bold:italic"? This has most likely never worked with the previous
       approach. (wiesener 20071016)
    */
    if ( bold || italic)
    {
      baselen = 0;
      if (!bold || (italic && (bold > italic)))
      {
        baselen = (int) (italic - tmp);
      }
      else
      {
        baselen = (int) (bold - tmp);
      }
      basename = cc_string_construct_new();
      cc_string_set_text(basename, fontname);
      cc_string_remove_substring(basename, baselen,
          cc_string_length(basename) - 1);

      /* try constructing the font again: */
      wfont2 = cc_flww32_create_font(cc_string_get_text(basename), sizey, angle,
                                     bold ? TRUE : FALSE, italic ? TRUE : FALSE);
      if (wfont2) {
        cc_flww32_get_font_name(wfont2, realname);
        if (((int) cc_string_length(realname) == baselen) &&
            !coin_strncasecmp(cc_string_get_text(realname),
                              cc_string_get_text(basename),
                              baselen)) {
          /* The new font was a perfect match. Replace the old one. */
          DeleteObject(wfont);
          wfont = wfont2;
        }
      }
      cc_string_destruct(basename);
    }
    cc_string_destruct(namelower);
  }
  cc_string_destruct(realname);

  /*
     Constructing a multilevel kerninghash for this font
  */

  FontContext fontContext(cc_flww32_globals.devctx, (HFONT)wfont, "cc_flww32_get_font");
  if (!fontContext.isValid()) return NULL;

  nrkpairs = GetKerningPairs(cc_flww32_globals.devctx, 0, NULL);
  if (nrkpairs) {

    kpairs = (KERNINGPAIR *) malloc(nrkpairs * sizeof(KERNINGPAIR));

    ret = GetKerningPairs(cc_flww32_globals.devctx, nrkpairs, kpairs);
    if (ret == 0) {
      cc_win32_print_error("cc_flww32_get_font", "GetKerningPairs()", GetLastError());
      return NULL;
    }

    if (!cc_dict_get(cc_flww32_globals.font2kerninghash, (uintptr_t) wfont, (void **) &fontkerninghash)) {
      fontkerninghash = cc_dict_construct(5, 0.75);
      cc_dict_put(cc_flww32_globals.font2kerninghash, (uintptr_t) wfont, fontkerninghash);
    }

    for (i=0;i<(int) nrkpairs;++i) {
      if (cc_dict_get(fontkerninghash, kpairs[i].wFirst, (void **) &khash)) {

        if (!cc_dict_get(khash, kpairs[i].wSecond, (void **) &khash)) {
          kerningvalue = (float *) malloc(sizeof(float)); /* Ugly... (handegar)*/
          kerningvalue[0] = (float) kpairs[i].iKernAmount;
          cc_dict_put(khash, kpairs[i].wSecond, kerningvalue);
        }

      } else {

        khash = cc_dict_construct(127, 0.75);
        kerningvalue = (float *) malloc(sizeof(float)); /* Ugly... (handegar)*/
        kerningvalue[0] = (float) kpairs[i].iKernAmount;

        /* FIXME: A standalone cc_floathash should have been made so that we dont have to
           allocate memory to store a single float. We could have used the pointer-value to
           store the float, but that might cause problems later when we go from 32 to 64 bits.
           (20030929 handegar) */

        cc_dict_put(khash, (uintptr_t) kpairs[i].wSecond, kerningvalue);
        cc_dict_put(fontkerninghash, (uintptr_t) kpairs[i].wFirst, khash);

      }
    }

    free(kpairs);
  }

  {
    /* MSVC7 on 64-bit Windows wants this extra cast. */
    const uintptr_t tmp = (uintptr_t)sizey;
    (void) cc_dict_put(cc_flww32_globals.fontsizehash, (uintptr_t)wfont, (void *)tmp);
  }

  return (void *)wfont;
}

/* Returns with name of given font id in the string. cc_string needs
   to have been constructed properly by caller. */
void
cc_flww32_get_font_name(void * font, cc_string * str)
{
  int size, newsize;
  char * s;

  /* Connect device context to font. */
  FontContext fontContext(cc_flww32_globals.devctx, (HFONT)font, "cc_flww32_get_font_name");
  if (!fontContext.isValid()) {
    cc_string_set_text(str, "<unknown>");
    return;
  }

  size = cc_win32()->GetTextFace(cc_flww32_globals.devctx, 0, NULL);

  /* 'size' will never be 0. Then GetTextFaceW would have asserted. */
  s = (char *)malloc(size);
  assert(s); /* FIXME: handle alloc problem better. 20030530 mortene. */

  newsize = cc_win32()->GetTextFace(cc_flww32_globals.devctx, size, s);

  cc_string_set_text(str, s);

  /* FIXME: this should be handled better. See FIXME about making an
     additional wrapper around GetTextFaceW() to catch string cropping,
     in win32api.c's coin_GetTextFace(). 20031114 mortene. */
  if (newsize == size) {
    /* The returned fontname length is longer than expected. This
       means that the system has cropped the string. Requested font
       will most probably not be found. */
    SoDebugError::postWarning("cc_flww32_get_font_name",
                              "GetTextFace(). The length of the returned fontname is"
                              " >= expected size. Fontname has been cropped.");
  }

  free(s);
}

/* Deallocates the resources connected with the given font id. Assumes
   that the font will no longer be used. */
void
cc_flww32_done_font(void * font)
{
  BOOL ok;
  SbBool found;
  cc_dict * khash;

  found = cc_dict_remove(cc_flww32_globals.fontsizehash, (uintptr_t)font);
  assert(found && "huh?");

  /* Delete kerninghash for this font using apply-callbacks */

  if (cc_dict_get(cc_flww32_globals.font2kerninghash, (uintptr_t)font, (void **) &khash)) {
    cc_dict_remove(cc_flww32_globals.font2kerninghash, (uintptr_t)font);
    cc_dict_apply(khash, cc_flww32_kerninghash_deleteCB2, NULL);
    cc_dict_destruct(khash);
  }

  ok = DeleteObject((HFONT)font);
  assert(ok && "DeleteObject() failed, investigate");
}


/* Returns the glyph index for the given character code. If the
   character code is undefined for this font, returns 0. */
int
cc_flww32_get_glyph(void * font, unsigned int charidx)
{
  /* FIXME: unimplemented. 20030515 mortene. */

  /* FIXME: what does the above FIXME really mean? This function is in
     use, and there is no immediately obvious ill effect.
     20050623 mortene. */

  return charidx;
}

/* Returns, in x and y input arguments, how much to advance cursor
   after rendering glyph. */
void
cc_flww32_get_vector_advance(void * font, int glyph, float * x, float * y)
{
  LOGFONT lfont;
  GLYPHMETRICS gm;
  static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);

  /* NOTE: Do not make this matrix 'static'. It seems like Win95/98/ME
     fails if the idmatrix is static. Newer versions seems to not mind
     though.  handegar. */
  /* FIXME: this should be investigated in more detail -- how the heck
     can it make a difference whether or not it's static? 20031118 mortene. */
  const MAT2 identitymatrix = { { 0, 1 }, { 0, 0 },
                                { 0, 0 }, { 0, 1 } };
  DWORD ret;
  DWORD size = 0;

  /* Connect device context to font. */
  FontContext fontContext(cc_flww32_globals.devctx, (HFONT)font, "cc_flww32_get_vector_advance");
  if (!fontContext.isValid()) {
    return;
  }

  /* The GetGlyphOutlineW function retrieves the outline or bitmap for
     a character in the TrueType font that is selected into the
     specified device context. */
  if (disable_utf8) {
    ret = GetGlyphOutline(cc_flww32_globals.devctx,
			  glyph, /* character to query */
			  GGO_METRICS, /* format of data to return */
			  &gm, /* metrics */
			  0, /* size of buffer for data */
			  NULL, /* buffer for data */
			  &identitymatrix); /* transformation matrix */
  } else {
    ret = GetGlyphOutlineW(cc_flww32_globals.devctx,
			   glyph, /* character to query */
			   GGO_METRICS, /* format of data to return */
			   &gm, /* metrics */
			   0, /* size of buffer for data */
			   NULL, /* buffer for data */
			   &identitymatrix); /* transformation matrix */
  }

  if (ret == GDI_ERROR) {
    cc_string str;
    cc_string_construct(&str);
    cc_string_sprintf(&str,
                      "GetGlyphOutlineW(HDC=%p, 0x%x '%c', GGO_METRICS, "
                      "<metricsstruct>, 0, NULL, <idmatrix>)",
                      cc_flww32_globals.devctx, glyph, (unsigned char)glyph);
    cc_win32_print_error("cc_flww32_get_vector_advance", cc_string_get_text(&str), GetLastError());
    cc_string_clean(&str);
    return;
  }

  ret = GetObject((HFONT) font,sizeof(lfont), (LPVOID) &lfont);
  size = -lfont.lfHeight;
  if (ret == 0) {
    cc_win32_print_error("cc_flww32_get_vector_advance", "GetObject()", GetLastError());
    size = 1;
  }

  *x = (float) gm.gmCellIncX / ((float) size);
  *y = (float) gm.gmCellIncY / ((float) size);
}

/* Returns kerning, in x and y input arguments, for a pair of
   glyphs. */
void
cc_flww32_get_bitmap_kerning(void * font, int glyph1, int glyph2, int * x, int * y)
{

  float * kerning = NULL;
  cc_dict * khash;

  if (cc_dict_get(cc_flww32_globals.font2kerninghash, (uintptr_t)font, (void **) &khash)) {
    if (cc_dict_get(khash, (uintptr_t)glyph1, (void **) &khash)) {
      if (cc_dict_get((cc_dict *) khash, (uintptr_t)glyph2, (void **) &kerning)) {
        *x = (int) kerning[0];
        *y = 0;
        return;
      }
    }
  }

  *x = 0;
  *y = 0;
}

/* Returns kerning, in x and y input arguments, for a pair of
   glyphs. */
void
cc_flww32_get_vector_kerning(void * font, int glyph1, int glyph2, float * x, float * y)
{

  float * kerning = NULL;
  cc_dict * khash;
  DWORD ret;
  DWORD size;
  LOGFONT lfont;

  ret = GetObject((HFONT) font,sizeof(lfont), (LPVOID) &lfont);
  size = -lfont.lfHeight;
  if (ret == 0) {
    cc_win32_print_error("cc_flww32_get_vector_kerning", "GetObject()", GetLastError());
    size = 1;
  }

  if (cc_dict_get(cc_flww32_globals.font2kerninghash, (uintptr_t)font, (void **) &khash)) {
    if (cc_dict_get(khash, (uintptr_t)glyph1, (void **) &khash)) {
      if (cc_dict_get((cc_dict *) khash, (uintptr_t)glyph2, (void **) &kerning)) {
        *x = kerning[0] / size;
        *y = 0;
        return;
      }
    }
  }

  *x = 0.0f;
  *y = 0.0f;
}


/* Client should use this to indicate it's done with a glyph, so the
   resources associated with it can be deallocated. */
void
cc_flww32_done_glyph(void * font, int glyph)
{
}

/* Draws a bitmap for the given glyph. */
struct cc_font_bitmap *
cc_flww32_get_bitmap(void * font, int glyph)
{
  struct cc_font_bitmap * bm = NULL;
  GLYPHMETRICS gm;
  static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);

  /* NOTE: Do not make this matrix 'static'. It seems like Win95/98/ME
     fails if the idmatrix is static. Newer versions seems to not mind
     though. */
  /* FIXME: this should be investigated in more detail -- how the heck
     can it make a difference whether or not it's static? 20031118 mortene. */
  const MAT2 identitymatrix = { { 0, 1 }, { 0, 0 },
                                { 0, 0 }, { 0, 1 } };
  DWORD ret;
  DWORD size = 0;
  uint8_t * w32bitmap = NULL;

  /* Connect device context to font. */
  FontContext fontContext(cc_flww32_globals.devctx, (HFONT)font, "cc_flww32_get_bitmap");
  if (!fontContext.isValid()) {
    return NULL;
  }

  /* The GetGlyphOutlineW function retrieves the outline or bitmap for
     a character in the TrueType font that is selected into the
     specified device context. */

  if (disable_utf8) {
    ret = GetGlyphOutline(cc_flww32_globals.devctx,
			  glyph, /* character to query */
			  GGO_GRAY8_BITMAP, /* format of data to return */
			  &gm, /* metrics */
			  0, /* size of buffer for data */
			  NULL, /* buffer for data */
			  &identitymatrix); /* transformation matrix */
  } else {
    ret = GetGlyphOutlineW(cc_flww32_globals.devctx,
			   glyph, /* character to query */
			   GGO_GRAY8_BITMAP, /* format of data to return */
			   &gm, /* metrics */
			   0, /* size of buffer for data */
			   NULL, /* buffer for data */
			   &identitymatrix); /* transformation matrix */
  }

  /* As of now, GetGlyphOutlineW() should have no known reason to
     fail.

     FIXME: We should eventually allow non-TT fonts to be loaded
     as well, by changing the "precision" setting in the call to
     CreateFontW() to also allow raster fonts (see FIXME comment where
     CreateFontW() is called). Then, when GetGlyphOutlineW() fails, use
     TextOut() and GetDIBits() to grab a font glyph's bitmap.
     20030610 mortene.
  */
  if (ret == GDI_ERROR) {
    cc_string str;
    cc_string_construct(&str);
    cc_string_sprintf(&str,
                      "GetGlyphOutlineW(HDC=%p, 0x%x '%c', GGO_GRAY8_BITMAP, "
                      "<metricsstruct>, 0, NULL, <idmatrix>)",
                      cc_flww32_globals.devctx, glyph, (unsigned char)glyph);
    cc_win32_print_error("cc_flww32_get_bitmap", cc_string_get_text(&str), GetLastError());
    cc_string_clean(&str);
    return NULL;
  }

  assert((ret < 1024*1024) && "bogus buffer size");
  size = ret;

  /* "size" can be equal to zero, it is for instance known to happen
     for at least the space char glyph for some charsets. */
  if (size > 0) {
    w32bitmap = (uint8_t *)malloc(ret);
    assert(w32bitmap != NULL); /* FIXME: be robust. 20030530 mortene. */

    if (disable_utf8) {
      ret = GetGlyphOutline(cc_flww32_globals.devctx,
			     glyph, /* character to query */
			     GGO_GRAY8_BITMAP, /* format of data to return */
			     &gm, /* metrics */
			     size, /* size of buffer for data */
			     w32bitmap, /* buffer for data */
			     &identitymatrix); /* transformation matrix */
    } else {
      ret = GetGlyphOutlineW(cc_flww32_globals.devctx,
			     glyph, /* character to query */
			     GGO_GRAY8_BITMAP, /* format of data to return */
			     &gm, /* metrics */
			     size, /* size of buffer for data */
			     w32bitmap, /* buffer for data */
			     &identitymatrix); /* transformation matrix */
    }

    if (ret == GDI_ERROR) {
      cc_string str;
      cc_string_construct(&str);
      cc_string_sprintf(&str,
                        "GetGlyphOutlineW(HDC=%p, 0x%x '%c', GGO_GRAY8_BITMAP, "
                        "<metricsstruct>, %d, <buffer>, <idmatrix>)",
                        cc_flww32_globals.devctx, glyph, (unsigned char)glyph, size);
      cc_win32_print_error("cc_flww32_get_bitmap", cc_string_get_text(&str), GetLastError());
      cc_string_clean(&str);
      free(w32bitmap);
      return NULL;
    }
  }

  bm = (struct cc_font_bitmap *)malloc(sizeof(struct cc_font_bitmap));
  assert(bm);
  bm->bearingX = gm.gmptGlyphOrigin.x;
  bm->bearingY = gm.gmptGlyphOrigin.y;
  bm->advanceX = gm.gmCellIncX;
  bm->advanceY = gm.gmCellIncY;
  bm->rows = gm.gmBlackBoxY;
  bm->width = gm.gmBlackBoxX;
  bm->pitch = bm->width;
  bm->buffer = NULL;
  /* FIXME: mono hardcoded to false. what about bitmapped fonts? any
     chance we could get one? if yes, we need to query and handle this
     case appropriately. 20040929 tamer. */
  bm->mono = 0;
  if (w32bitmap != NULL) { /* Could be NULL for at least space char glyph. */
    unsigned int i, j;
    unsigned char *dst, * src, * next_row;

    /* copy the values one by one from the win32 bitmap and scale them
       up to 256 gray level values. */
    bm->buffer = (unsigned char *)malloc(bm->rows * bm->width);
    dst = bm->buffer;
    src = next_row = (unsigned char *)w32bitmap;
    for (i = 0; i < bm->rows; i++) {
      src = next_row;
      for (j=0; j < bm->width; j++, dst++, src++) {
        /* we get a value ranging from 0-64. adjust it to 256 gray
           level values by multiplying it by 4. handle the 0 and 64
           special cases by either returning 0 or always using the by
           one smaller value in order not to overflow the 64 case. */
        *dst = *src ? (*src - 1) << 2 : 0;
      }
      /* go to the next DWORD aligned row oriented row */
      next_row += ((bm->width + 3) >> 2) << 2;
    }
  }
  
  free(w32bitmap);
  return bm;
}


static void
flww32_getVerticesFromPath(HDC hdc)
{

  LPPOINT p_points = NULL;
  LPBYTE p_types = NULL;
  int numpoints, i, lastmoveto;
  uintptr_t tmp;

  if (FlattenPath(hdc) == 0) {
    cc_win32_print_error("flww32_getVerticesFromPath", "Failed when handling TrueType font; "
                         "FlattenPath()", GetLastError());
    /* The system cannot convert splines to vectors. Aborting. */
    return;
  }

  /* determine the number of endpoints in the path*/
  numpoints = GetPath(hdc, NULL, NULL, 0);
  if (numpoints < 0) {
    cc_win32_print_error("flww32_getVerticesFromPath", "Failed when handling TrueType font; "
                         "GetPath()", GetLastError());
    return;
  }

  if (numpoints > 0) {
    /* allocate memory for the point data and for the vertex types  */
    p_points = (POINT *)malloc(numpoints * sizeof(POINT));
    p_types = (BYTE *)malloc(numpoints * sizeof(BYTE));

    /* get the path's description */
    GetPath(hdc, p_points, p_types, numpoints);

    /* go through the endpoints */
    for (i = 0; i < numpoints; i++) {

      /* if this endpoint starts a new contour */
      if (p_types[i] == PT_MOVETO) {

        lastmoveto = i;
        if (flww32_tessellator.contour_open) {
          GLUWrapper()->gluTessEndContour(flww32_tessellator.tessellator_object);
          cc_list_truncate(flww32_tessellator.edgeindexlist,
                           cc_list_get_length(flww32_tessellator.edgeindexlist)-1);
          /* MSVC7 on 64-bit Windows wants this extra cast. */
          tmp = (uintptr_t)flww32_tessellator.edge_start_vertex;
          cc_list_append(flww32_tessellator.edgeindexlist, (void *)tmp);
        }

        GLUWrapper()->gluTessBeginContour(flww32_tessellator.tessellator_object);
        flww32_tessellator.edge_start_vertex = flww32_tessellator.vertex_counter;
        flww32_tessellator.contour_open = TRUE;
        continue;
      }

      /* Close the contour? */
      if (p_types[i] & PT_CLOSEFIGURE) {
        if (flww32_win9598Me) {
          /* If the current OS is Windows95/98/Me, the last vertex
            must be added before closing the figure-path. If the OS is
            a newer version (i.e. XP/2000/NT), adding the last vertex
            will lead to a 'gap' which looks quite ugly when
            extruded. The 'flww32_win9598Me' is a static SbBool
            initialized once in 'flww32_initialize()'. */
          flww32_addTessVertex(p_points[i].x, p_points[i].y);
        }

        flww32_addTessVertex(p_points[lastmoveto].x, p_points[lastmoveto].y);
      }
      else {
        flww32_addTessVertex(p_points[i].x, p_points[i].y);
      }
    }
    free(p_points);
    free(p_types);
  }

}

struct cc_font_vector_glyph *
cc_flww32_get_vector_glyph(void * font, unsigned int glyph, float complexity)
{
  HDC memdc;
  HBITMAP membmp;
  HDC screendc;
  TCHAR glyph_str[1];
  WCHAR glyph_strw[1];
  struct cc_font_vector_glyph * new_vector_glyph;
  void * tmp;
  unsigned int size;
  uintptr_t cast_aid;
  UINT previous;
  static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);


  if (!GLUWrapper()->available) {
    SoDebugError::post("cc_flww32_get_vector_glyph",
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
    SoDebugError::post("cc_flww32_get_vector_glyph",
                       "Unable to bind required GLU tessellation "
                       "functions for 3D Win32 TrueType font support.");
    return NULL;
  }


  /* FIXME: we're being unnecessary robust for much of the code below
     calling into Win32 API functions. For most or all of the calls we
     should just wrap the Win32 API calls in the glue/C/win32api.h
     interface, catch errors, inform about the problem and assert()
     there -- to simplify the client code below. 20031118 mortene. */

  /*
     If NULL is returned due to an error, glyph3d.c will load the
     default font instead.
  */

  /* FIXME: don't do the DC- and bitmap-initialization for each and
     every glyph -- once should be enough. 20050706 mortene. */

  memdc = CreateCompatibleDC(NULL);
  if (memdc == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling CreateCompatibleDC(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  /* About this: see comment on SetTextAlign() call in
     cc_flww32_initialize(). */
  previous = SetTextAlign(memdc, TA_BASELINE);
  assert(previous != GDI_ERROR);

  screendc = GetDC(NULL);
  if (screendc == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling GetDC(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  membmp = CreateCompatibleBitmap(screendc, 300, 300);
  if (membmp == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling CreateCompatibleBitmap(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  if (SelectObject(memdc, membmp) == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling SelectObject(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  if (SelectObject(memdc, font) == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling SelectObject(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  if (SetBkMode(memdc, TRANSPARENT) == 0) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling SetBkMode()", GetLastError());
    /* Not a critical error, continuing. Glyphs might look a bit weird
       though. */
  }
  if (BeginPath(memdc) == 0) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling BeginPath(). Cannot vectorize font.", GetLastError());
    return NULL;
  }
  if (disable_utf8) {
    glyph_str[0] = glyph;
    if (TextOut(memdc, 0, 0, glyph_str, 1) == 0) {
      cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling TextOut(). Cannot vectorize font.", GetLastError());
      return NULL;
    }
  } else {
    glyph_strw[0] = glyph;
    if (TextOutW(memdc, 0, 0, glyph_strw, 1) == 0) {
      cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling TextOutW(). Cannot vectorize font.", GetLastError());
      return NULL;
    }
  }
  if (EndPath(memdc) == 0) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling EndPath(). Cannot vectorize font.", GetLastError());
    return NULL;
  }

  /* FIXME: very inefficient to do initialization of the GLU
     tessellator object, and the cc_list instances, for each and every
     glyph to vectorize -- do this only once. 20050706 mortene. */

  if (flww32_tessellator.vertexlist == NULL)
    flww32_tessellator.vertexlist = cc_list_construct();
  if (flww32_tessellator.faceindexlist == NULL)
    flww32_tessellator.faceindexlist = cc_list_construct();
  if (flww32_tessellator.edgeindexlist == NULL)
    flww32_tessellator.edgeindexlist = cc_list_construct();
  if (flww32_tessellator.malloclist == NULL)
    flww32_tessellator.malloclist = cc_list_construct();

  flww32_tessellator.tessellator_object = GLUWrapper()->gluNewTess();
  flww32_tessellator.contour_open = FALSE;
  flww32_tessellator.vertex_scale = 1.0f;
  flww32_tessellator.triangle_mode = 0;
  flww32_tessellator.triangle_index_counter = 0;
  flww32_tessellator.triangle_strip_flipflop = FALSE;
  flww32_tessellator.vertex_counter = 0;


  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_VERTEX, (gluTessCallback_cb_t)flww32_vertexCallback);
  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_BEGIN, (gluTessCallback_cb_t)flww32_beginCallback);
  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_END, (gluTessCallback_cb_t)flww32_endCallback);
  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_COMBINE, (gluTessCallback_cb_t)flww32_combineCallback);
  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_ERROR, (gluTessCallback_cb_t)flww32_errorCallback);

  GLUWrapper()->gluTessBeginPolygon(flww32_tessellator.tessellator_object, NULL);

  flww32_getVerticesFromPath(memdc);

  if (flww32_tessellator.contour_open) {
    GLUWrapper()->gluTessEndContour(flww32_tessellator.tessellator_object);
    cc_list_truncate(flww32_tessellator.edgeindexlist,
                     cc_list_get_length(flww32_tessellator.edgeindexlist)-1);
    /* MSVC7 on 64-bit Windows wants this extra cast. */
    cast_aid = (uintptr_t)flww32_tessellator.edge_start_vertex;
    cc_list_append(flww32_tessellator.edgeindexlist, (void *)cast_aid);
  }

  GLUWrapper()->gluTessEndPolygon(flww32_tessellator.tessellator_object);
  GLUWrapper()->gluDeleteTess(flww32_tessellator.tessellator_object);

  cc_list_append(flww32_tessellator.faceindexlist, (void *) -1);
  cc_list_append(flww32_tessellator.edgeindexlist, (void *) -1);

  /* Copy the static vector_glyph struct to a newly allocated struct
     returned to the user. This is done due to the fact that the
     tessellation callback solution needs a static working struct. */
  new_vector_glyph = (struct cc_font_vector_glyph *) malloc(sizeof(struct cc_font_vector_glyph));

  size = 200;
  if (cc_dict_get(cc_flww32_globals.fontsizehash, (uintptr_t)font, &tmp)) {
    /* MSVC7 on 64-bit Windows wants this extra cast. */
    cast_aid = (uintptr_t)tmp;
    size = (int)cast_aid;
  }

  flww32_buildVertexList(new_vector_glyph, size);
  flww32_buildFaceIndexList(new_vector_glyph);
  flww32_buildEdgeIndexList(new_vector_glyph);
  flww32_cleanupMallocList();

  /* Remove allocated DCs */
  if (!DeleteObject(membmp))
    cc_win32_print_error("cc_flww32_get_vector_glyph","DeleteObject(). Error deleting bitmap device context.", GetLastError());
  if (!DeleteObject(screendc))
    cc_win32_print_error("cc_flww32_get_vector_glyph","DeleteObject(). Error deleting screen device context.", GetLastError());
  if (!DeleteObject(memdc))
    cc_win32_print_error("cc_flww32_get_vector_glyph","DeleteObject(). Error deleting memory device context.", GetLastError());

  return new_vector_glyph;
}


static void
flww32_addTessVertex(double x, double y)
{
  uintptr_t cast_aid;
  int * counter = (int *)malloc(sizeof(int));
  float * point = (float *)malloc(sizeof(float)*2);

  point[0] = flww32_tessellator.vertex_scale * (float)x;
  point[1] = flww32_tessellator.vertex_scale * (float)y;
  cc_list_append(flww32_tessellator.vertexlist, point);

  /* MSVC7 on 64-bit Windows wants this extra cast. */
  cast_aid = (uintptr_t)flww32_tessellator.vertex_counter;
  cc_list_append(flww32_tessellator.edgeindexlist, (void *)cast_aid);

  counter[0] = flww32_tessellator.vertex_counter++;
  cc_list_append(flww32_tessellator.malloclist, counter);
  {
    GLdouble v[3] = { x, y, 0 };
    GLUWrapper()->gluTessVertex(flww32_tessellator.tessellator_object, v, counter);
  }

  /* MSVC7 on 64-bit Windows wants this extra cast. */
  cast_aid = (uintptr_t)flww32_tessellator.vertex_counter;
  cc_list_append(flww32_tessellator.edgeindexlist, (void *)cast_aid);
}

static void CALLBACK
flww32_vertexCallback(GLvoid * data)
{
  uintptr_t cast_aid;
  unsigned int i;
  int index;
  index = ((int *) data)[0];

  if ((flww32_tessellator.triangle_fan_root_index == -1) &&
      (flww32_tessellator.triangle_index_counter == 0)) {
    flww32_tessellator.triangle_fan_root_index = index;
  }

  if (flww32_tessellator.triangle_mode == GL_TRIANGLE_FAN) {
    if (flww32_tessellator.triangle_index_counter == 0) {
      flww32_tessellator.triangle_indices[0] = flww32_tessellator.triangle_fan_root_index;
      flww32_tessellator.triangle_indices[1] = index;
      ++flww32_tessellator.triangle_index_counter;
    }
    else flww32_tessellator.triangle_indices[flww32_tessellator.triangle_index_counter++] = index;
  }
  else {
    flww32_tessellator.triangle_indices[flww32_tessellator.triangle_index_counter++] = index;
  }

  assert(flww32_tessellator.triangle_index_counter < 4);

  if (flww32_tessellator.triangle_index_counter == 3) {


    if (flww32_tessellator.triangle_mode == GL_TRIANGLE_STRIP) {
      if (flww32_tessellator.triangle_strip_flipflop) {
        index = flww32_tessellator.triangle_indices[1];
        flww32_tessellator.triangle_indices[1] = flww32_tessellator.triangle_indices[2];
        flww32_tessellator.triangle_indices[2] = index;
      }
    }

    for (i=0; i < 3; i++) {
      /* MSVC7 on 64-bit Windows wants this extra cast. */
      cast_aid = (uintptr_t)flww32_tessellator.triangle_indices[i];
      cc_list_append(flww32_tessellator.faceindexlist, (void *)cast_aid);
    }

    if (flww32_tessellator.triangle_mode == GL_TRIANGLE_FAN) {
      flww32_tessellator.triangle_indices[1] = flww32_tessellator.triangle_indices[2];
      flww32_tessellator.triangle_index_counter = 2;
    }

    else if (flww32_tessellator.triangle_mode == GL_TRIANGLE_STRIP) {

      if (flww32_tessellator.triangle_strip_flipflop) {
        index = flww32_tessellator.triangle_indices[1];
        flww32_tessellator.triangle_indices[1] = flww32_tessellator.triangle_indices[2];
        flww32_tessellator.triangle_indices[2] = index;
      }

      flww32_tessellator.triangle_indices[0] = flww32_tessellator.triangle_indices[1];
      flww32_tessellator.triangle_indices[1] = flww32_tessellator.triangle_indices[2];
      flww32_tessellator.triangle_index_counter = 2;
      flww32_tessellator.triangle_strip_flipflop = !flww32_tessellator.triangle_strip_flipflop;

    } else flww32_tessellator.triangle_index_counter = 0;

  }

}

static void CALLBACK
flww32_beginCallback(GLenum which)
{
  flww32_tessellator.triangle_mode = which;
  if (which == GL_TRIANGLE_FAN)
    flww32_tessellator.triangle_fan_root_index = -1;
  else
    flww32_tessellator.triangle_fan_root_index = 0;
  flww32_tessellator.triangle_index_counter = 0;
  flww32_tessellator.triangle_strip_flipflop = FALSE;

}

static void CALLBACK
flww32_endCallback(void)
{
}

static void CALLBACK
flww32_combineCallback(GLdouble coords[3], GLvoid * vertex_data, GLfloat weight[4], int **dataOut)
{

  int * ret = (int *)malloc(sizeof(int));
  float * point = (float *)malloc(sizeof(float)*2);

  point[0] = flww32_tessellator.vertex_scale * ((float) coords[0]);
  point[1] = flww32_tessellator.vertex_scale * ((float) coords[1]);

  cc_list_append(flww32_tessellator.vertexlist, point);
  cc_list_append(flww32_tessellator.malloclist, ret);

  ret[0] = flww32_tessellator.vertex_counter++;

  *dataOut = ret;

}

static void CALLBACK
flww32_errorCallback(GLenum error_code)
{
  SoDebugError::post("flww32_errorCallback","Error when tesselating glyph (GLU errorcode: %d):, investigate.", error_code);
}

static void
flww32_buildVertexList(struct cc_font_vector_glyph * newglyph, int size)
{

  int numcoords,i;
  float * coord;

  assert(flww32_tessellator.vertexlist && "Error fetching vector glyph coordinates\n");
  numcoords = cc_list_get_length(flww32_tessellator.vertexlist);

  newglyph->vertices = (float *) malloc(sizeof(float)*numcoords*2);

  for (i=0;i<numcoords;++i) {
    coord = (float *) cc_list_get(flww32_tessellator.vertexlist,i);

    /* Must flip and translate glyph due to the W32 coord system
       which has a Y-axis pointing downwards */
    newglyph->vertices[i*2 + 0] = coord[0] / size;
    newglyph->vertices[i*2 + 1] = (-coord[1]) / size;

    free(coord);
  }

  cc_list_destruct(flww32_tessellator.vertexlist);
  flww32_tessellator.vertexlist = NULL;
}

static void
flww32_cleanupMallocList(void)
{
  int i, n;
  if (flww32_tessellator.malloclist) {
    n = cc_list_get_length(flww32_tessellator.malloclist);
    for (i = 0; i < n; i++) {
      free(cc_list_get(flww32_tessellator.malloclist, i));
    }
    cc_list_destruct(flww32_tessellator.malloclist);
    flww32_tessellator.malloclist = NULL;
  }
}

static void
flww32_buildEdgeIndexList(struct cc_font_vector_glyph * newglyph)
{
  int i,len;

  assert(flww32_tessellator.edgeindexlist);

  len = cc_list_get_length(flww32_tessellator.edgeindexlist);
  newglyph->edgeindices = (int *) malloc(sizeof(int)*len);

  for (i=0;i<len;++i) {
    /* MSVC7 on 64-bit Windows wants this extra cast. */
    const uintptr_t cast_aid = (uintptr_t)
      cc_list_get(flww32_tessellator.edgeindexlist, i);
    newglyph->edgeindices[i] = (int)cast_aid;
  }

  cc_list_destruct(flww32_tessellator.edgeindexlist);
  flww32_tessellator.edgeindexlist = NULL;
}

static void
flww32_buildFaceIndexList(struct cc_font_vector_glyph * newglyph)
{
  int len,i;

  assert(flww32_tessellator.faceindexlist);

  len = cc_list_get_length(flww32_tessellator.faceindexlist);
  newglyph->faceindices = (int *) malloc(sizeof(int)*len);

  for (i=0;i<len;++i) {
    /* MSVC7 on 64-bit Windows wants this extra cast. */
    const uintptr_t cast_aid = (uintptr_t)
      cc_list_get(flww32_tessellator.faceindexlist, i);
    newglyph->faceindices[i] = (int)cast_aid;
  }

  cc_list_destruct(flww32_tessellator.faceindexlist);
  flww32_tessellator.faceindexlist = NULL;

}

static int
flww32_calcfontsize(float complexity)
{
  const int sizes[] = {
    10, 25, 50, 100, 500, 1000, 2500, 4000, 6000, 8000, 10000
  };
  const unsigned int lastidx = sizeof(sizes) / sizeof(sizes[0]) - 1;
  unsigned int index = (unsigned int)(lastidx * complexity);
  if (index > lastidx) index = lastidx;

  return sizes[index];
}

#endif /* HAVE_WIN32_API */
