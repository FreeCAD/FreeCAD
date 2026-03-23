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

#include "fonts/coretext.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "glue/GLUWrapper.h"

/* ************************************************************************* */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ************************************************************************* */

#ifdef HAVE_CORETEXT

/* ************************************************************************* */

#include <ApplicationServices/ApplicationServices.h>

/* ************************************************************************* */

SbBool
cc_flwct_initialize(void)
{
  return TRUE;
}

void
cc_flwct_exit(void)
{}

void *
cc_flwct_get_font(const char * fontname, unsigned int pixelsize)
{
  return NULL;
}

void
cc_flwct_get_font_name(void * font, cc_string * str)
{}

void
cc_flwct_done_font(void * font)
{
  CFRelease((CTFontRef)font);
}

int
cc_flwct_get_num_charmaps(void * font)
{
  return 0;
}

const char *
cc_flwct_get_charmap_name(void * font, int charmap)
{
  return "UNICODE";
}

void
cc_flwct_set_charmap(void * font, int charmap)
{}

void
cc_flwct_set_char_size(void * font, int height)
{}

void
cc_flwct_set_font_rotation(void * font, float angle)
{}

int
cc_flwct_get_glyph(void * font, unsigned int charidx)
{
  return 0;
}

void
cc_flwct_get_vector_advance(void * font, int glyph, float * x, float * y)
{}

void
cc_flwct_get_bitmap_kerning(void * font, int glyph1, int glyph2, int *x, int *y)
{}


void
cc_flwct_get_vector_kerning(void * font, int glyph1, int glyph2, float *x, float *y)
{}

void
cc_flwct_done_glyph(void * font, int glyph)
{}

struct cc_font_bitmap *
cc_flwct_get_bitmap(void * font, unsigned int glyph)
{
  return NULL;
}

struct cc_font_vector_glyph *
cc_flwct_get_vector_glyph(void * font, unsigned int glyphindex, float complexity)
{
  return NULL;
}

#endif /* HAVE_CORETEXT */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
