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

/* Generated from 2d-font.xpm (by ???). */

/* FIXME: add iso-latin-1 characters (and characters from other
   character sets?). 20010823 mortene. */

#include "fonts/defaultfonts.h"

#if 0

static const unsigned char coin_default2dfont[][12] = {
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* [blank] */
  {  0,  0, 12, 12,  0,  8, 12, 12, 12, 12, 12,  0 }, /* ! */
  {  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20 }, /* " */
  {  0,  0, 18, 18, 18, 63, 18, 18, 63, 18, 18,  0 }, /* # */
  {  0,  8, 28, 42, 10, 10, 12, 24, 40, 42, 28,  8 }, /* $ */
  {  0,  0,  6, 73, 41, 22,  8, 52, 74, 73, 48,  0 }, /* % */
  {  0,  0, 29, 34, 34, 37, 25, 12, 18, 18, 12,  0 }, /* & */
  {  0,  0,  0,  0,  0,  0,  0,  0,  0, 24, 12, 12 }, /* ' */
  {  0,  6,  8,  8, 16, 16, 16, 16, 16,  8,  8,  6 }, /* ( */
  {  0, 48,  8,  8,  4,  4,  4,  4,  4,  8,  8, 48 }, /* ) */
  {  0,  0,  0,  0,  0,  0,  8, 42, 20, 42,  8,  0 }, /* * */
  {  0,  0,  0,  8,  8,  8,127,  8,  8,  8,  0,  0 }, /* + */
  {  0, 24, 12, 12,  0,  0,  0,  0,  0,  0,  0,  0 }, /* , */
  {  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0 }, /* - */
  {  0,  0, 24, 24,  0,  0,  0,  0,  0,  0,  0,  0 }, /* . */
  {  0, 32, 32, 16, 16,  8,  8,  8,  4,  4,  2,  2 }, /* / */
  {  0,  0, 28, 34, 34, 34, 34, 34, 34, 34, 28,  0 }, /* 0 */
  {  0,  0,  8,  8,  8,  8,  8,  8, 40, 24,  8,  0 }, /* 1 */
  {  0,  0, 62, 32, 16,  8,  4,  2,  2, 34, 28,  0 }, /* 2 */
  {  0,  0, 28, 34,  2,  2, 12,  2,  2, 34, 28,  0 }, /* 3 */
  {  0,  0,  4,  4,  4,126, 68, 36, 20, 12,  4,  0 }, /* 4 */
  {  0,  0, 28, 34,  2,  2,  2, 60, 32, 32, 62,  0 }, /* 5 */
  {  0,  0, 28, 34, 34, 34, 60, 32, 32, 34, 28,  0 }, /* 6 */
  {  0,  0, 16, 16, 16,  8,  8,  4,  2,  2, 62,  0 }, /* 7 */
  {  0,  0, 28, 34, 34, 34, 28, 34, 34, 34, 28,  0 }, /* 8 */
  {  0,  0, 28, 34,  2,  2, 30, 34, 34, 34, 28,  0 }, /* 9 */
  {  0,  0, 24, 24,  0,  0,  0, 24, 24,  0,  0,  0 }, /* : */
  {  0, 48, 24, 24,  0,  0,  0, 24, 24,  0,  0,  0 }, /* ; */
  {  0,  0,  0,  2,  4,  8, 16,  8,  4,  2,  0,  0 }, /* < */
  {  0,  0,  0,  0,  0,127,  0,127,  0,  0,  0,  0 }, /* = */
  {  0,  0,  0, 16,  8,  4,  2,  4,  8, 16,  0,  0 }, /* > */
  {  0,  0, 16, 16,  0, 16, 28,  2,  2,  2, 60,  0 }, /* ? */
  {  0,  0, 28, 32, 73, 86, 82, 82, 78, 34, 28,  0 }, /* @ */
  {  0,  0, 33, 33, 33, 63, 18, 18, 18, 12, 12,  0 }, /* A */
  {  0,  0, 60, 34, 34, 34, 60, 34, 34, 34, 60,  0 }, /* B */
  {  0,  0, 14, 16, 32, 32, 32, 32, 32, 18, 14,  0 }, /* C */
  {  0,  0, 56, 36, 34, 34, 34, 34, 34, 36, 56,  0 }, /* D */
  {  0,  0, 62, 32, 32, 32, 60, 32, 32, 32, 62,  0 }, /* E */
  {  0,  0, 16, 16, 16, 16, 30, 16, 16, 16, 30,  0 }, /* F */
  {  0,  0, 14, 18, 34, 34, 32, 32, 32, 18, 14,  0 }, /* G */
  {  0,  0, 34, 34, 34, 34, 62, 34, 34, 34, 34,  0 }, /* H */
  {  0,  0, 62,  8,  8,  8,  8,  8,  8,  8, 62,  0 }, /* I */
  {  0,  0,112,  8,  8,  8,  8,  8,  8,  8, 62,  0 }, /* J */
  {  0,  0, 33, 33, 34, 36, 56, 40, 36, 34, 33,  0 }, /* K */
  {  0,  0, 30, 16, 16, 16, 16, 16, 16, 16, 16,  0 }, /* L */
  {  0,  0, 33, 33, 33, 45, 45, 45, 51, 51, 33,  0 }, /* M */
  {  0,  0, 34, 34, 38, 38, 42, 42, 50, 50, 34,  0 }, /* N */
  {  0,  0, 12, 18, 33, 33, 33, 33, 33, 18, 12,  0 }, /* O */
  {  0,  0, 32, 32, 32, 60, 34, 34, 34, 34, 60,  0 }, /* P */
  {  3,  6, 12, 18, 33, 33, 33, 33, 33, 18, 12,  0 }, /* Q */
  {  0,  0, 34, 34, 34, 36, 60, 34, 34, 34, 60,  0 }, /* R */
  {  0,  0, 60,  2,  2,  6, 28, 48, 32, 32, 30,  0 }, /* S */
  {  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,127,  0 }, /* T */
  {  0,  0, 28, 34, 34, 34, 34, 34, 34, 34, 34,  0 }, /* U */
  {  0,  0, 12, 12, 18, 18, 18, 33, 33, 33, 33,  0 }, /* V */
  {  0,  0, 34, 34, 34, 54, 85, 73, 73, 73, 65,  0 }, /* W */
  {  0,  0, 34, 34, 20, 20,  8, 20, 20, 34, 34,  0 }, /* X */
  {  0,  0,  8,  8,  8,  8, 20, 20, 34, 34, 34,  0 }, /* Y */
  {  0,  0, 62, 32, 16, 16,  8,  4,  4,  2, 62,  0 }, /* Z */
  {  0, 14,  8,  8,  8,  8,  8,  8,  8,  8,  8, 14 }, /* [ */
  {  0,  2,  2,  4,  4,  8,  8,  8, 16, 16, 32, 32 }, /* [backslash] */
  {  0, 56,  8,  8,  8,  8,  8,  8,  8,  8,  8, 56 }, /* ] */
  {  0,  0,  0,  0,  0, 34, 34, 20, 20,  8,  8,  0 }, /* ^ */
  {  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* _ */
  {  0,  0,  0,  0,  0,  0,  0,  0,  0, 24, 24, 12 }, /* ` */
  {  0,  0, 29, 34, 34, 30,  2, 34, 28,  0,  0,  0 }, /* a */
  {  0,  0, 60, 34, 34, 34, 34, 50, 44, 32, 32, 32 }, /* b */
  {  0,  0, 14, 16, 32, 32, 32, 16, 14,  0,  0,  0 }, /* c */
  {  0,  0, 26, 38, 34, 34, 34, 34, 30,  2,  2,  2 }, /* d */
  {  0,  0, 28, 34, 32, 62, 34, 34, 28,  0,  0,  0 }, /* e */
  {  0,  0, 16, 16, 16, 16, 16, 16, 62, 16, 16, 14 }, /* f */
  { 28,  2,  2, 26, 38, 34, 34, 34, 30,  0,  0,  0 }, /* g */
  {  0,  0, 34, 34, 34, 34, 34, 50, 44, 32, 32, 32 }, /* h */
  {  0,  0,  8,  8,  8,  8,  8,  8, 56,  0,  8,  8 }, /* i */
  { 56,  4,  4,  4,  4,  4,  4,  4, 60,  0,  4,  4 }, /* j */
  {  0,  0, 33, 34, 36, 56, 40, 36, 34, 32, 32, 32 }, /* k */
  {  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,  8, 56 }, /* l */
  {  0,  0, 73, 73, 73, 73, 73,109, 82,  0,  0,  0 }, /* m */
  {  0,  0, 34, 34, 34, 34, 34, 50, 44,  0,  0,  0 }, /* n */
  {  0,  0, 28, 34, 34, 34, 34, 34, 28,  0,  0,  0 }, /* o */
  { 32, 32, 60, 34, 34, 34, 34, 50, 44,  0,  0,  0 }, /* p */
  {  2,  2, 26, 38, 34, 34, 34, 34, 30,  0,  0,  0 }, /* q */
  {  0,  0, 16, 16, 16, 16, 16, 24, 22,  0,  0,  0 }, /* r */
  {  0,  0, 60,  2,  2, 28, 32, 32, 30,  0,  0,  0 }, /* s */
  {  0,  0, 14, 16, 16, 16, 16, 16, 62, 16, 16,  0 }, /* t */
  {  0,  0, 26, 38, 34, 34, 34, 34, 34,  0,  0,  0 }, /* u */
  {  0,  0,  8,  8, 20, 20, 34, 34, 34,  0,  0,  0 }, /* v */
  {  0,  0, 34, 34, 34, 85, 73, 73, 65,  0,  0,  0 }, /* w */
  {  0,  0, 34, 34, 20,  8, 20, 34, 34,  0,  0,  0 }, /* x */
  { 48, 16,  8,  8, 20, 20, 34, 34, 34,  0,  0,  0 }, /* y */
  {  0,  0, 62, 32, 16,  8,  4,  2, 62,  0,  0,  0 }, /* z */
  {  0,  6,  8,  8,  8,  4, 24,  4,  8,  8,  8,  6 }, /* { */
  {  0,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8 }, /* | */
  {  0, 48,  8,  8,  8, 16, 12, 16,  8,  8,  8, 48 }, /* } */
  {  0,  0,  0,  0,  0,  0, 78, 57,  0,  0,  0,  0 }, /* ~ */

  /* iso-latin-1 norwegian letters */
  {  0,  0, 59, 76, 72, 62,  9, 73, 54,  0,  0,  0 }, /* ae */
  {  0,  0, 92, 34, 50, 42, 42, 38, 29,  0,  0,  0 }, /* oe */
  {  0,  0, 29, 34, 34, 30,  2, 34, 28,  8, 20,  8 }, /* aa */
  {  0,  0, 79, 72, 72, 72,127, 72, 72, 72, 63,  0 }, /* AE */
  {  0,  0, 44, 18, 41, 41, 41, 37, 37, 18, 13,  0 }, /* OE */
  {  0,  0, 33, 33, 33, 63, 18, 18, 12, 12, 18, 12 }, /* AA */

  /* other characters we've found use for -- these should at least be
     present in iso-latin-1 */
  {  0,  0,  0,  0,  0,  0,  0, 24, 36, 24,  0,  0 }  /* degree-sign (character #176) */
};

/* map from iso-latin1 to font data array index */
static const int coin_default2dfont_isolatin1_mapping[] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   1,   2,   3,   4,   5,   6,   7,
    8,   9,  10,  11,  12,  13,  14,  15,
   16,  17,  18,  19,  20,  21,  22,  23,
   24,  25,  26,  27,  28,  29,  30,  31,
   32,  33,  34,  35,  36,  37,  38,  39,
   40,  41,  42,  43,  44,  45,  46,  47,
   48,  49,  50,  51,  52,  53,  54,  55,
   56,  57,  58,  59,  60,  61,  62,  63,
   64,  65,  66,  67,  68,  69,  70,  71,
   72,  73,  74,  75,  76,  77,  78,  79,
   80,  81,  82,  83,  84,  85,  86,  87,
   88,  89,  90,  91,  92,  93,  94,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
  101,   0,   0,   0,   0,   0,   0,   0, /* 176-183 */
    0,   0,   0,   0,   0,   0,   0,   0, /* 184-191 */
    0,   0,   0,   0,   0, 100,  98,   0, /* 192-199 */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
   99,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,  97,  95,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
   96,   0,   0,   0,   0,   0,   0,   0
};

#endif // 0

#include "builtin2dfonts.icc"

#if 0

const int * 
coin_default2dfont_get_isolatin1_mapping(void)
{
  return coin_default2dfont_isolatin1_mapping;
}

const unsigned char * 
coin_default2dfont_get_data(void)
{
  return (const unsigned char *) coin_default2dfont;
}

int 
coin_default2dfont_get_size(void)
{
  return 12;
}

#endif // 0

int
coin_default2dfont_get_height(float size)
{
  if ( size < 14.0f ) { return COIN_FONT_13_HEIGHT; }
  else if ( size < 18.0f ) { return COIN_FONT_17_HEIGHT; }
  else if ( size < 26.0f ) { return COIN_FONT_25_HEIGHT; }
  else { return COIN_FONT_33_HEIGHT; }
}

int
coin_default2dfont_get_width(float size)
{
  if ( size < 14.0f ) { return COIN_FONT_13_WIDTH; }
  else if ( size < 18.0f ) { return COIN_FONT_17_WIDTH; }
  else if ( size < 26.0f ) { return COIN_FONT_25_WIDTH; }
  else { return COIN_FONT_33_WIDTH; }
}

int
coin_default2dfont_get_bearing(float size)
{
  if ( size < 14.0f ) { return COIN_FONT_13_BEARING; }
  else if ( size < 18.0f ) { return COIN_FONT_17_BEARING; }
  else if ( size < 26.0f ) { return COIN_FONT_25_BEARING; }
  else { return COIN_FONT_33_BEARING; }
}

const unsigned char *
coin_default2dfont_get_data(float size)
{
  if ( size < 14.0f ) { return (const unsigned char *) font_data_13; }
  else if ( size < 18.0f ) { return (const unsigned char *) font_data_17; }
  else if ( size < 26.0f ) { return (const unsigned char *) font_data_25; }
  else { return (const unsigned char *) font_data_33; }
}
