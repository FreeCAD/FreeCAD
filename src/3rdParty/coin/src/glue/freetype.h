#ifndef COIN_GLUE_FREETYPE_H
#define COIN_GLUE_FREETYPE_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0 /* to get proper auto-indentation in emacs */
}
#endif /* emacs indentation */

/* FIXME: Usage of fontconfig is tightly coupled and only makes sense
 * with freetype. therefore decided to define structures together with
 * freetype. Still, should it better be in its own file or rather
 * inside the HAVE_FREETYPE section? 20040411 tamer.
 */

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#else /* HAVE_FONTCONFIG */

/* avoid including <fontconfig/fontconfig.h>. define absolutely
 * necessary structs and types.
 */

#define FC_PIXEL_SIZE "pixelsize" /* Double */

typedef enum _FcMatchKind {
  FcMatchPattern, FcMatchFont
} FcMatchKind;

typedef enum _FcResult {
  FcResultMatch, FcResultNoMatch, FcResultTypeMismatch, FcResultNoId
} FcResult;

typedef struct _FcPattern {
  int num;
  int size;
  void * elts; /* void * instead of FcPatternElt * to minimize declarations! */
  int ref;
} FcPattern;

#endif /* !HAVE_FONTCONFIG */

int cc_fcglue_available(void);

int cc_fcglue_FcGetVersion(void);
FcPattern * cc_fcglue_FcNameParse(const unsigned char * name);
int cc_fcglue_FcConfigSubstitute(void * config, FcPattern * pattern, FcMatchKind kind);
void cc_fcglue_FcDefaultSubstitute(FcPattern *pattern);
FcPattern * cc_fcglue_FcFontMatch(void * config, FcPattern * pattern, FcResult * result);
FcResult cc_fcglue_FcPatternGetString(const FcPattern * pattern, const char * object, int n, unsigned char ** s);
void cc_fcglue_FcPatternDestroy(FcPattern * pattern);
void cc_fcglue_FcPatternPrint(const FcPattern * pattern);
int cc_fcglue_FcPatternAddDouble(FcPattern *pattern, const char *object, double d);

#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#else /* HAVE_FREETYPE */

#include <Inventor/C/basic.h>

/* 
   We need some freetype structs, so define them here for runtime
   linking support (we want to avoid including <freetype.h>).
*/

/* we also need some defines */
#define FT_LOAD_DEFAULT 0x0
#define FT_LOAD_RENDER 0x4
#define FT_LOAD_MONOCHROME 0x1000
#define FT_FACE_FLAG_KERNING (1L <<  6)
#define FT_KERNING_DEFAULT  0
#define FT_RENDER_MODE_NORMAL 0
#define FT_RENDER_MODE_MONO 2
#define ft_kerning_default FT_KERNING_DEFAULT
#define ft_render_mode_mono FT_RENDER_MODE_MONO
#define ft_render_mode_normal FT_RENDER_MODE_NORMAL

#define FT_HAS_KERNING(face) (face->face_flags & FT_FACE_FLAG_KERNING)

/* ...and lots of typedefs */
typedef void * FT_Library;
typedef int FT_Error;
typedef int FT_Int;
typedef long FT_Long;
typedef unsigned long FT_ULong;
typedef char FT_String;
typedef short FT_Short;
typedef unsigned short FT_UShort;
typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_SizeRec_ * FT_Size;
typedef signed long FT_Fixed;
typedef signed long FT_Pos;
typedef int32_t FT_UInt32;
typedef unsigned int FT_UInt;
typedef struct FT_GlyphRec_ * FT_Glyph;
typedef struct FT_CharMapRec_ * FT_CharMap;
typedef struct FT_GlyphSlotRec_ * FT_GlyphSlot;
typedef struct FT_BitmapGlyphRec_ * FT_BitmapGlyph;

typedef struct FT_Matrix_ {
  long xx, xy;
  long yx, yy;
} FT_Matrix;

typedef struct FT_Vector_ {
  long x;
  long y;
} FT_Vector;

typedef struct {
  FT_Short height;
  FT_Short width;
} FT_Bitmap_Size;

typedef void (*FT_Generic_Finalizer)(void*  object);

typedef struct {
  void * data;
  FT_Generic_Finalizer finalizer;
} FT_Generic;

typedef struct {
  FT_Pos  xMin, yMin;
  FT_Pos  xMax, yMax;  
} FT_BBox;

struct FT_CharMapRec_ {
  FT_Face face;
  int encoding;
  FT_UShort platform_id;
  FT_UShort encoding_id;  
};

typedef struct {
  int rows;
  int width;
  int pitch;
  unsigned char * buffer;
  short num_grays;
  char pixel_mode;
  char palette_mode;
  void * palette;
} FT_Bitmap;

struct FT_FaceRec_ {
  FT_Long num_faces;
  FT_Long face_index;
  
  FT_Long face_flags;
  FT_Long style_flags;
  
  FT_Long num_glyphs;
  
  FT_String * family_name;
  FT_String * style_name;
  
  FT_Int num_fixed_sizes;
  FT_Bitmap_Size * available_sizes;
  
  FT_Int num_charmaps;
  FT_CharMap * charmaps;
  
  FT_Generic generic;
  
  FT_BBox bbox;
  
  FT_UShort units_per_EM;
  FT_Short ascender;
  FT_Short descender;
  FT_Short height;
  
  FT_Short max_advance_width;
  FT_Short max_advance_height;
  
  FT_Short underline_position;
  FT_Short underline_thickness;
  
  FT_GlyphSlot glyph;
  
  FT_Size size;
  FT_CharMap charmap;
  
  /* private begin.
   *
   * FT_Driver         driver;
   * FT_Memory         memory;
   * FT_Stream         stream;
   *
   * FT_ListRec        sizes_list;
   * 
   * FT_Generic        autohint;
   * void*             extensions;
   * 
   * FT_Face_Internal  internal;
   * 
   * private end */ 
};

typedef struct FT_GlyphRec_ {
  FT_Library library;
  void * clazz; /* const FT_Glyph_Class * */
  int format;
  FT_Vector advance;
} FT_GlyphRec;

typedef struct {
  short n_contours;
  short n_points;
  FT_Vector * points;
  char * tags;
  short * contours;
  int flags;
} FT_Outline;

typedef struct {
  FT_Pos width;
  FT_Pos height;
  FT_Pos horiBearingX;
  FT_Pos horiBearingY;
  FT_Pos horiAdvance;
  FT_Pos vertBearingX;
  FT_Pos vertBearingY;
  FT_Pos vertAdvance;
} FT_Glyph_Metrics;

struct FT_GlyphSlotRec_ {
  FT_Library library;
  FT_Face face;
  FT_GlyphSlot next;
  FT_UInt flags;
  FT_Generic generic;
  
  FT_Glyph_Metrics metrics;
  FT_Fixed linearHoriAdvance;
  FT_Fixed linearVertAdvance;
  FT_Vector advance;
  
  int format; /* FT_Glyph_Format (enum) */
  
  FT_Bitmap bitmap;
  FT_Int bitmap_left;
  FT_Int bitmap_top;
  
  FT_Outline outline;
  
  FT_UInt num_subglyphs;
  void * subglyphs; /* FT_SubGlyph (struct ptr) */
  
  void * control_data;
  long control_len;
  
  void * other;
  void * internal;
};

typedef struct {
  FT_UShort x_ppem;
  FT_UShort y_ppem;

  FT_Fixed x_scale;
  FT_Fixed y_scale;
  
  FT_Pos ascender;
  FT_Pos descender;
  FT_Pos height;
  FT_Pos max_advance;
} FT_Size_Metrics;

struct FT_Size_ {
  FT_Face face;
  FT_Generic generic;
  FT_Size_Metrics metrics;
  void * internal;
};

struct FT_BitmapGlyphRec_ {
  FT_GlyphRec  root;
  FT_Int left;
  FT_Int top;
  FT_Bitmap bitmap;
};

typedef int (*FT_Outline_MoveToFunc) (FT_Vector * to, void * user);
typedef int (*FT_Outline_LineToFunc) (FT_Vector * to, void * user);
typedef int (*FT_Outline_ConicToFunc) (FT_Vector * control, FT_Vector * to, void * user);
typedef int (*FT_Outline_CubicToFunc) (FT_Vector * control1, FT_Vector * control2, FT_Vector * to, void * user);
#define FT_Outline_MoveTo_Func FT_Outline_MoveToFunc
#define FT_Outline_LineTo_Func FT_Outline_LineToFunc
#define FT_Outline_ConicTo_Func FT_Outline_ConicToFunc
#define FT_Outline_CubicTo_Func FT_Outline_CubicToFunc

typedef struct  FT_Outline_Funcs_ {
  FT_Outline_MoveToFunc move_to;
  FT_Outline_LineToFunc line_to;
  FT_Outline_ConicToFunc conic_to;
  FT_Outline_CubicToFunc cubic_to;  
  int shift;
  FT_Pos delta;  
} FT_Outline_Funcs;

typedef struct  FT_OutlineGlyphRec_
{
  FT_GlyphRec root;
  FT_Outline outline;
  
} FT_OutlineGlyphRec;


typedef struct FT_OutlineGlyphRec_* FT_OutlineGlyph;

#endif /* !HAVE_FREETYPE */

int cc_ftglue_available(void);

/* FIXME: some of the typedef'ed function signatures below does not
   match the correct function def, as given in
   /usr/include/freetype2/freetype/freetype.h. 20050711 mortene. */

FT_Error cc_ftglue_FT_Init_FreeType(FT_Library * library);
void cc_ftglue_FT_Library_Version(void * library, int * major, int * minor, int * patch);
void cc_ftglue_FT_Done_FreeType(void * library);

FT_Error cc_ftglue_FT_New_Face(void * library, const char * filepathname, long faceindex, FT_Face * face);
FT_Error cc_ftglue_FT_Done_Face(void * face);
FT_Error cc_ftglue_FT_Select_Charmap(FT_Face face, int encoding);
FT_Error cc_ftglue_FT_Set_Char_Size(FT_Face face, long width, long height, unsigned int hres, unsigned int vres);
void cc_ftglue_FT_Set_Transform(FT_Face face, FT_Matrix * matrix, FT_Vector * delta);
FT_UInt cc_ftglue_FT_Get_Char_Index(FT_Face face, unsigned long charidx);
FT_Error cc_ftglue_FT_Load_Glyph(FT_Face face, unsigned int glyph, int32_t loadflags);
FT_Error cc_ftglue_FT_Get_Kerning(FT_Face face, unsigned int left, unsigned int right, unsigned int kernmode, FT_Vector * akerning);
FT_Error cc_ftglue_FT_Get_Glyph(void * glyphslot, FT_Glyph * glyph);
FT_Error cc_ftglue_FT_Glyph_To_Bitmap(FT_Glyph * glyph, int rendermode, FT_Vector * origin, int destroy);
void cc_ftglue_FT_Done_Glyph(FT_Glyph glyph);
FT_Error cc_ftglue_FT_Outline_Decompose(FT_Outline * outline, const FT_Outline_Funcs * func_interface, void * user);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !COIN_GLUE_FREETYPE_H */
