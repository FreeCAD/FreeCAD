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

#ifdef HAVE_CONFIG_H
#include "config.h"
#else /* No config.h? Hmm. Assume the freetype library is available for linking. */
#define FREETYPEGLUE_ASSUME_FREETYPE 1
#endif /* !HAVE_CONFIG_H */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <cmath>
#include <cstddef>

#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#ifdef HAVE_FREETYPE /* In case we're _not_ doing runtime linking. */
#define FREETYPEGLUE_ASSUME_FREETYPE 1
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#endif /* FREETYPEGLUE_ASSUME_FREETYPE */

#include <Inventor/C/basic.h>
#include <Inventor/C/glue/dl.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/errors/debugerror.h>

#include "tidbitsp.h"
#include "glue/freetype.h"
#include "threads/threadsutilp.h"

typedef int (*cc_fcglue_FcGetVersion_t)(void);
typedef FcPattern * (*cc_fcglue_FcNameParse_t)(const unsigned char * name);
typedef int (*cc_fcglue_FcConfigSubstitute_t)(void * config, FcPattern * p, FcMatchKind kind);
typedef void (*cc_fcglue_FcDefaultSubstitute_t)(FcPattern *pattern);
typedef FcPattern * (*cc_fcglue_FcFontMatch_t)(void * config, FcPattern * p, FcResult * result);
typedef FcResult (*cc_fcglue_FcPatternGetString_t)(const FcPattern * p, const char * object, int n, unsigned char ** s);
typedef void (*cc_fcglue_FcPatternDestroy_t)(FcPattern * p);
typedef void (*cc_fcglue_FcPatternPrint_t)(const FcPattern * p);
typedef int (*cc_fcglue_FcPatternAddDouble_t)(FcPattern *p, const char *object, double d);

typedef struct {
  int available;
  cc_fcglue_FcGetVersion_t FcGetVersion;
  cc_fcglue_FcNameParse_t FcNameParse;
  cc_fcglue_FcConfigSubstitute_t FcConfigSubstitute;
  cc_fcglue_FcDefaultSubstitute_t FcDefaultSubstitute;
  cc_fcglue_FcFontMatch_t FcFontMatch;
  cc_fcglue_FcPatternGetString_t FcPatternGetString;
  cc_fcglue_FcPatternDestroy_t FcPatternDestroy;
  cc_fcglue_FcPatternPrint_t FcPatternPrint;
  cc_fcglue_FcPatternAddDouble_t FcPatternAddDouble;
} cc_fcglue_t;

typedef FT_Error (*cc_ftglue_FT_Init_FreeType_t)(FT_Library * library);
typedef void (*cc_ftglue_FT_Library_Version_t)(void * library, int * major, int * minor, int * patch);
typedef FT_Error (*cc_ftglue_FT_Done_FreeType_t)(void * library);
typedef FT_Error (*cc_ftglue_FT_New_Face_t)(void * library, const char * filepathname, long faceindex, FT_Face * face);
typedef FT_Error (*cc_ftglue_FT_Done_Face_t)(void * face);
typedef FT_Error (*cc_ftglue_FT_Select_Charmap_t)(FT_Face face, int encoding);
typedef FT_Error (*cc_ftglue_FT_Set_Char_Size_t)(FT_Face face, long width, long height, unsigned int hres, unsigned int vres);
typedef void (*cc_ftglue_FT_Set_Transform_t)(FT_Face face, FT_Matrix * matrix, FT_Vector * delta);
typedef FT_UInt (*cc_ftglue_FT_Get_Char_Index_t)(FT_Face face, unsigned long charidx);
typedef FT_Error (*cc_ftglue_FT_Load_Glyph_t)(FT_Face face, unsigned int glyph, int32_t loadflags);
typedef FT_Error (*cc_ftglue_FT_Get_Kerning_t)(FT_Face face, unsigned int left, unsigned int right, unsigned int kernmode, FT_Vector * akerning);
typedef FT_Error (*cc_ftglue_FT_Get_Glyph_t)(void * glyphslot, FT_Glyph * glyph);
typedef FT_Error (*cc_ftglue_FT_Glyph_To_Bitmap_t)(FT_Glyph * glyph, int rendermode, FT_Vector * origin, int destroy);
typedef void (*cc_ftglue_FT_Done_Glyph_t)(FT_Glyph glyph);
typedef FT_Error (*cc_ftglue_FT_Outline_Decompose_t)(FT_Outline * outline, const FT_Outline_Funcs * func_interface, void * user);
typedef FT_Error (*cc_ftglue_FT_Render_Glyph_t)(void * glyphslot, int rendermode);

typedef struct {
  int available;
  cc_ftglue_FT_Init_FreeType_t FT_Init_FreeType;
  cc_ftglue_FT_Library_Version_t FT_Library_Version;
  cc_ftglue_FT_Done_FreeType_t FT_Done_FreeType;
  cc_ftglue_FT_New_Face_t FT_New_Face;
  cc_ftglue_FT_Done_Face_t  FT_Done_Face;
  cc_ftglue_FT_Select_Charmap_t FT_Select_Charmap; 
  cc_ftglue_FT_Set_Char_Size_t FT_Set_Char_Size;
  cc_ftglue_FT_Set_Transform_t FT_Set_Transform;
  cc_ftglue_FT_Get_Char_Index_t FT_Get_Char_Index;
  cc_ftglue_FT_Load_Glyph_t FT_Load_Glyph;
  cc_ftglue_FT_Get_Kerning_t FT_Get_Kerning;
  cc_ftglue_FT_Get_Glyph_t FT_Get_Glyph; 
  cc_ftglue_FT_Glyph_To_Bitmap_t FT_Glyph_To_Bitmap; 
  cc_ftglue_FT_Done_Glyph_t FT_Done_Glyph;
  cc_ftglue_FT_Outline_Decompose_t FT_Outline_Decompose;
} cc_ftglue_t;

static cc_fcglue_t * fontconfig_instance = NULL;
static cc_libhandle fontconfig_libhandle = NULL;
static int fontconfig_failed_to_load = 0;

static cc_ftglue_t * freetype_instance = NULL;
static cc_libhandle freetype_libhandle = NULL;
static int freetype_failed_to_load = 0;

/* Cleans up fontconfig at exit. */
static void
fcglue_cleanup(void)
{
#ifdef FONTCONFIG_RUNTIME_LINKING
  if (fontconfig_libhandle) {
    cc_dl_close(fontconfig_libhandle);
    fontconfig_libhandle = NULL;
  }
#endif /* FONTCONFIG_RUNTIME_LINKING */
  assert(fontconfig_instance);
  free(fontconfig_instance);
  fontconfig_instance = NULL;
  fontconfig_failed_to_load = 0;
}

static const cc_fcglue_t *
fcglue_init(void)
{
  CC_SYNC_BEGIN(fcglue_init);

  if (!fontconfig_instance && !fontconfig_failed_to_load) {
    /* First invocation, do initializations. */
    cc_fcglue_t * fi = (cc_fcglue_t *)malloc(sizeof(cc_fcglue_t));
    (void)coin_atexit((coin_atexit_f *)fcglue_cleanup, CC_ATEXIT_DYNLIBS);

    /* The common case is that fontconfig is either available from the
       linking process or we're successfully going to link it in. */
    fi->available = 1;

#ifdef FONTCONFIG_RUNTIME_LINKING
    {
      int idx;
      /* FIXME: should we get the system shared library name from an
         Autoconf check? 20000930 mortene. */
      const char * possiblelibnames[] = {
        NULL, /* is set below */
        "fontconfig", "libfontconfig", "libfontconfig.so",
        "libfontconfig.dylib",
        NULL
      };
      possiblelibnames[0] = coin_getenv("COIN_FONTCONFIG_LIBNAME");
      idx = possiblelibnames[0] ? 0 : 1;
      while (!fontconfig_libhandle && possiblelibnames[idx]) {
        fontconfig_libhandle = cc_dl_open(possiblelibnames[idx]);
        idx++;
      }

      if (!fontconfig_libhandle) {
        fi->available = 0;
        fontconfig_failed_to_load = 1;
      }
    }
    /* Define FCGLUE_REGISTER_FUNC macro. Casting the type is
       necessary for this file to be compatible with C++ compilers. */
#define FCGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    do { \
      fi->_funcname_ = (_funcsig_)cc_dl_sym(fontconfig_libhandle, SO__QUOTE(_funcname_)); \
      if (fi->_funcname_ == NULL) fi->available = 0; \
    } while (0)

#elif defined(FONTCONFIGGLUE_ASSUME_FONTCONFIG) /* !FONTCONFIG_RUNTIME_LINKING */

    /* Define FCGLUE_REGISTER_FUNC macro. */
#define FCGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    fi->_funcname_ = (_funcsig_)_funcname_

#else /* !FONTCONFIGGLUE_ASSUME_FONTCONFIG */
    fi->available = 0;
    /* Define FCGLUE_REGISTER_FUNC macro. */
#define FCGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    fi->_funcname_ = NULL

#endif /* !FONTCONFIGGLUE_ASSUME_FONTCONFIG */

    FCGLUE_REGISTER_FUNC(cc_fcglue_FcGetVersion_t, FcGetVersion);

    if (fi->available && (!fi->FcGetVersion)) {
      /* something is seriously wrong */
      cc_debugerror_post("fontconfig glue",
                         "Loaded fontconfig DLL ok, but couldn't resolve some symbols.");
      fi->available = 0;
      fontconfig_failed_to_load = 1;
      fontconfig_instance = fi;
    }
    else {
      FCGLUE_REGISTER_FUNC(cc_fcglue_FcNameParse_t, FcNameParse);
      FCGLUE_REGISTER_FUNC(cc_fcglue_FcConfigSubstitute_t, FcConfigSubstitute);
      FCGLUE_REGISTER_FUNC(cc_fcglue_FcDefaultSubstitute_t, FcDefaultSubstitute); 
      FCGLUE_REGISTER_FUNC(cc_fcglue_FcFontMatch_t, FcFontMatch);
      FCGLUE_REGISTER_FUNC(cc_fcglue_FcPatternGetString_t, FcPatternGetString);
      FCGLUE_REGISTER_FUNC(cc_fcglue_FcPatternDestroy_t, FcPatternDestroy);
      FCGLUE_REGISTER_FUNC(cc_fcglue_FcPatternPrint_t, FcPatternPrint);
      FCGLUE_REGISTER_FUNC(cc_fcglue_FcPatternAddDouble_t, FcPatternAddDouble);

      /* Do this late, so we can detect recursive calls to this function. */
      fontconfig_instance = fi;
    }
  }
  CC_SYNC_END(fcglue_init);
  return fontconfig_instance;
}


int
cc_fcglue_FcGetVersion(void)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  return fontconfig_instance->FcGetVersion();
}

FcPattern * 
cc_fcglue_FcNameParse(const unsigned char * name)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  return fontconfig_instance->FcNameParse(name);
}

int
cc_fcglue_FcConfigSubstitute(void * config, FcPattern * pattern, FcMatchKind kind)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  return fontconfig_instance->FcConfigSubstitute(config, pattern, kind);
}

void
cc_fcglue_FcDefaultSubstitute(FcPattern *pattern)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  fontconfig_instance->FcDefaultSubstitute(pattern);
}

FcPattern *
cc_fcglue_FcFontMatch(void * config, FcPattern * pattern, FcResult * result)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  return fontconfig_instance->FcFontMatch(config, pattern, result);
}

FcResult
cc_fcglue_FcPatternGetString(const FcPattern * pattern, const char * object, int n, unsigned char ** s)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  return fontconfig_instance->FcPatternGetString(pattern, object, n, s);
}

void
cc_fcglue_FcPatternDestroy(FcPattern * pattern)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  fontconfig_instance->FcPatternDestroy(pattern);
}

void
cc_fcglue_FcPatternPrint(const FcPattern * pattern)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  fontconfig_instance->FcPatternPrint(pattern);
}

int
cc_fcglue_FcPatternAddDouble(FcPattern *p, const char *object, double d)
{
  assert(fontconfig_instance && fontconfig_instance->available);
  return fontconfig_instance->FcPatternAddDouble(p, object, d);
}

int
cc_fcglue_available(void)
{
  const char * env;
  
  if (!((env = coin_getenv("COIN_FORCE_FONTCONFIG_OFF")) && (atoi(env) > 0))) {
    fcglue_init();
  }
  return fontconfig_instance && fontconfig_instance->available;
}


/* Cleans up freetype at exit. */
static void
ftglue_cleanup(void)
{
#ifdef FREETYPE_RUNTIME_LINKING
  if (freetype_libhandle) {
    cc_dl_close(freetype_libhandle);
    freetype_libhandle = NULL;
  }
#endif /* FREETYPE_RUNTIME_LINKING */
  assert(freetype_instance);
  free(freetype_instance);
  freetype_instance = NULL;
  freetype_failed_to_load = 0;
}

static const cc_ftglue_t *
ftglue_init(void)
{
  CC_SYNC_BEGIN(ftglue_init);

  if (!freetype_instance && !freetype_failed_to_load) {
    /* First invocation, do initializations. */
    cc_ftglue_t * fi = (cc_ftglue_t *)malloc(sizeof(cc_ftglue_t));
    (void)coin_atexit((coin_atexit_f *)ftglue_cleanup, CC_ATEXIT_DYNLIBS);

    /* The common case is that FreeType is either available from the
       linking process or we're successfully going to link it in. */
    fi->available = 1;

#ifdef FREETYPE_RUNTIME_LINKING
    {
      int idx;
      /* FIXME: should we get the system shared library name from an
         Autoconf check? 20000930 mortene. */
      const char * possiblelibnames[] = {
        NULL, /* is set below */
        "freetype", "libfreetype", "libfreetype.so",
        "libfreetype.dylib",
        NULL
      };
      possiblelibnames[0] = coin_getenv("COIN_FREETYPE2_LIBNAME");
      idx = possiblelibnames[0] ? 0 : 1;

      while (!freetype_libhandle && possiblelibnames[idx]) {
        freetype_libhandle = cc_dl_open(possiblelibnames[idx]);
        idx++;
      }

      if (!freetype_libhandle) {
        fi->available = 0;
        freetype_failed_to_load = 1;
      }
    }
    /* Define FTGLUE_REGISTER_FUNC macro. Casting the type is
       necessary for this file to be compatible with C++ compilers. */
#define FTGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    do { \
      fi->_funcname_ = (_funcsig_)cc_dl_sym(freetype_libhandle, SO__QUOTE(_funcname_)); \
      if (fi->_funcname_ == NULL) fi->available = 0; \
    } while (0)

#elif defined(FREETYPEGLUE_ASSUME_FREETYPE) /* !FREETYPE_RUNTIME_LINKING */

    /* Define FTGLUE_REGISTER_FUNC macro. */
#define FTGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    fi->_funcname_ = (_funcsig_)_funcname_

#else /* !FREETYPEGLUE_ASSUME_FREETYPE */
    fi->available = 0;
    /* Define FTGLUE_REGISTER_FUNC macro. */
#define FTGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    fi->_funcname_ = NULL

#endif /* !FREETYPEGLUE_ASSUME_FREETYPE */

    FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Init_FreeType_t, FT_Init_FreeType);
    FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Library_Version_t, FT_Library_Version);
    FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Done_FreeType_t, FT_Done_FreeType);

    if (fi->available && (!fi->FT_Init_FreeType ||
                          !fi->FT_Library_Version ||
                          !fi->FT_Done_FreeType)) {
      /* something is seriously wrong */
      cc_debugerror_post("freetype glue",
                         "Loaded freetype DLL ok, but couldn't resolve basic symbols.");
      fi->available = 0;
      freetype_failed_to_load = 1;
      freetype_instance = fi;
    }
    else {
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_New_Face_t, FT_New_Face);
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Done_Face_t, FT_Done_Face);
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Select_Charmap_t, FT_Select_Charmap); 
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Set_Char_Size_t, FT_Set_Char_Size);
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Set_Transform_t, FT_Set_Transform);
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Get_Char_Index_t, FT_Get_Char_Index);
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Load_Glyph_t, FT_Load_Glyph);
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Get_Kerning_t, FT_Get_Kerning);
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Get_Glyph_t, FT_Get_Glyph); 
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Glyph_To_Bitmap_t, FT_Glyph_To_Bitmap); 
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Done_Glyph_t, FT_Done_Glyph);
      FTGLUE_REGISTER_FUNC(cc_ftglue_FT_Outline_Decompose_t, FT_Outline_Decompose);

      /* Do this late, so we can detect recursive calls to this function. */
      freetype_instance = fi;
    }
  }
  CC_SYNC_END(ftglue_init);
  return freetype_instance;
}


FT_Error 
cc_ftglue_FT_Init_FreeType(FT_Library * library)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Init_FreeType(library);
}

void 
cc_ftglue_FT_Library_Version(void * library, int * major, int * minor, int * patch)
{
  assert(freetype_instance && freetype_instance->available);
  freetype_instance->FT_Library_Version(library, major, minor, patch);
}

void 
cc_ftglue_FT_Done_FreeType(void * library)
{
  FT_Error err;
  assert(freetype_instance && freetype_instance->available);
  err = freetype_instance->FT_Done_FreeType(library);
  assert(err == 0 && "something bad happened at FreeType exit");
}

FT_Error 
cc_ftglue_FT_New_Face(void * library, const char * filepathname, long faceindex, FT_Face * face)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_New_Face(library, filepathname, faceindex, face);
}

FT_Error 
cc_ftglue_FT_Done_Face(void * face)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Done_Face(face);
}

FT_Error 
cc_ftglue_FT_Select_Charmap(FT_Face face, int encoding)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Select_Charmap(face, encoding);
}

FT_Error 
cc_ftglue_FT_Set_Char_Size(FT_Face face, long width, long height, unsigned int hres, unsigned int vres)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Set_Char_Size(face, width, height, hres, vres);
}

void 
cc_ftglue_FT_Set_Transform(FT_Face face, FT_Matrix * matrix, FT_Vector * delta)
{
  assert(freetype_instance && freetype_instance->available);
  freetype_instance->FT_Set_Transform(face, matrix, delta);
}

FT_UInt
cc_ftglue_FT_Get_Char_Index(FT_Face face, unsigned long charidx)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Get_Char_Index(face, charidx);
}

FT_Error 
cc_ftglue_FT_Load_Glyph(FT_Face face, unsigned int glyph, int32_t loadflags)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Load_Glyph(face, glyph, loadflags);
}

FT_Error 
cc_ftglue_FT_Get_Kerning(FT_Face face, unsigned int left, unsigned int right, unsigned int kernmode, FT_Vector * akerning)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Get_Kerning(face, left, right, kernmode, akerning);
}

FT_Error 
cc_ftglue_FT_Get_Glyph(void * glyphslot, FT_Glyph * glyph)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Get_Glyph(glyphslot, glyph);
}

FT_Error 
cc_ftglue_FT_Glyph_To_Bitmap(FT_Glyph * glyph, int rendermode, FT_Vector * origin, int destroy)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Glyph_To_Bitmap(glyph, rendermode, origin, destroy);
}

void 
cc_ftglue_FT_Done_Glyph(FT_Glyph glyph)
{
  assert(freetype_instance && freetype_instance->available);
  freetype_instance->FT_Done_Glyph(glyph);
}

int
cc_ftglue_available(void)
{
  ftglue_init();
  return freetype_instance && freetype_instance->available;
}

FT_Error 
cc_ftglue_FT_Outline_Decompose(FT_Outline * outline, const FT_Outline_Funcs * func_interface, void * user)
{
  assert(freetype_instance && freetype_instance->available);
  return freetype_instance->FT_Outline_Decompose(outline, func_interface, user);
}
