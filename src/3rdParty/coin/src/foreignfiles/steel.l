%top{
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

/* ********************************************************************** *
 * TODO:
 * - API to get file position data when encountering errors
 * - implement writing
 * - read/write the two color extensions (binary only)
 * - be robust for corrupt files
 * - wtf does "A facet normal coordinate may have a leading minus sign;
 *   a vertex coordinate may not." for stl ascii files mean?  do I need to
 *   take special care because of this?  why are there no proper formal
 *   specs for the stl file formats?
 *   UPDATE: it probably means that all vertices must lie in the positive
 *   octant in the worldspace - negative coordinates in any dimension is
 *   not allowed.  might be sensible, given that STL files are generally
 *   created for use by 3D rinters.
 * - figure out how to support gzipped files (in combination with flex,
 *   memory buffer techniques will probably have to be used)
 * - cr+lf on DOS/unix for ascii files - is this a problem?
 * - remove any error-handling asserts
 * ********************************************************************** */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <Inventor/system/inttypes.h>

#include "steel.h"
}

%{
/* @stl_real@
This is a typedef for the C type float.  It is used so the floating point type
can be changed at a later date.  It is not likely to happen though, as the
STL file format specifies that the floats should be IEEE 32-bit floats.
 */

/* @stl_facet_s@
This is an opaque handle for one facet record in an STL file.  It contains
the position of the three vertices in the triangle face, and the normal
vector.  For binary STL files, there is also two bytes of padding data
that is used for color information in a couple of STL format extensions,
which is also accessible with this data type.
 */

struct stl_facet_s {
  stl_real nx, ny, nz;
  stl_real v1x, v1y, v1z;
  stl_real v2x, v2y, v2z;
  stl_real v3x, v3y, v3z;
  unsigned int color;
};

/* @stl_reader_s@
This is an opaque handle for an STL file that is opened for reading.
Both ascii and binary file access is handled with this type.
 */

struct stl_reader_s {
  char * filename;
  FILE * file;
  char * info;
  stl_facet * facet;
  const char * error;
  unsigned int flags;
  unsigned int linenum;
  int pending;
  int vertex;
  int facets;
  int facets_total;
  int hickups;
};

/* @stl_writer_s@
This is an opaque handle for an STL file that is opened for writing.
 */

struct stl_writer_s {
  char * filename;
  FILE * file;
  char * info;
  const char * error;
  int facets;
  unsigned int flags;
  stl_facet * facet;
  int linenum;
};

/* prototypes for internal functions used in lex part */
static int stl_parse_real_triple(char * text, stl_real * a, stl_real * b, stl_real * c);

#define STL_PUBLIC_FLAGS 0x000000ff
#define STL_NO_PENDING   ((STL_ERROR) - 1)
#define YY_DECL          int stl_scan(stl_reader * reader)
%}

%option outfile="steel.cpp"
%option prefix="stl_yy"
%option case-insensitive
%option never-interactive
%option noyywrap
%option nounput
%option noinput
%option noyy_scan_buffer
%option noyy_scan_bytes
%option noyy_scan_string

DIGITS  [0123456789]+
SIGN    [+-]?
EXP     ([eE]{SIGN}?{DIGITS})?
FLOAT   {SIGN}{DIGITS}("."{DIGITS})?{EXP}
WHITE   [ \t]*
WS      [ \t]{WHITE}
LINE    [^\n]*

%%

{WHITE}solid{WHITE}{LINE}$	{
	  char * ptr = stl_yytext;
	  while ( *ptr == ' ' || *ptr == '\t' ) ptr++;
	  while ( *ptr != ' ' && *ptr != '\t' ) ptr++;
	  while ( *ptr && (*ptr == ' ' || *ptr == '\t') ) ptr++;
	  if ( strlen(ptr) > 0 ) {
	    reader->info = (char *) malloc(strlen(ptr)+1);
	    assert(reader->info);
	    strcpy(reader->info, ptr);
	    reader->pending = STL_INIT_INFO;
	  } else {
	    reader->pending = STL_NO_PENDING;
	  }
	  return STL_BEGIN;
	}

{WHITE}facet{WS}normal{WS}{FLOAT}{WS}{FLOAT}{WS}{FLOAT}{WHITE}$	{
	  reader->hickups = 0; /* reset flex hickup counter */
	  assert(reader->facet != NULL);
	  if ( reader->info != NULL ) {
	    free(reader->info);
	    reader->info = NULL;
	  }
	  stl_parse_real_triple(stl_yytext, &(reader->facet->nx), &(reader->facet->ny), &(reader->facet->nz));
	}

{WHITE}(outer{WS})?loop{WHITE}$	{
	  reader->vertex = 0;
	}

{WHITE}vertex{WS}{FLOAT}{WS}{FLOAT}{WS}{FLOAT}{WHITE}$	{
	  stl_real x = 0.0f, y = 0.0f, z = 0.0f;
	  assert(reader->facet != NULL);
	  stl_parse_real_triple(stl_yytext, &x, &y, &z);
	  switch ( reader->vertex ) {
	  case 0:
	    reader->facet->v1x = x;
	    reader->facet->v1y = y;
	    reader->facet->v1z = z;
	    break;
	  case 1:
	    reader->facet->v2x = x;
	    reader->facet->v2y = y;
	    reader->facet->v2z = z;
	    break;
	  case 2:
	    reader->facet->v3x = x;
	    reader->facet->v3y = y;
	    reader->facet->v3z = z;
	    break;
	  default:
	    reader->error = "vertex data error";
	    return STL_ERROR;
	  }
	  reader->vertex++;
	}

{WHITE}endloop{WHITE}$	{
	}

{WHITE}endfacet{WHITE}$	{
	  reader->pending = STL_NO_PENDING;
	  return STL_FACET;
	}

{WHITE}end{WHITE}solid{WHITE}{LINE}$	{
	  char * ptr = stl_yytext;
	  if ( reader->info != NULL ) {
	    free(reader->info);
	    reader->info = NULL;
	  }
	  /* FIXME: this scanning is now incorrect if not "endsolid" */
	  while ( *ptr && (*ptr == ' ' || *ptr == '\t') ) ptr++;
	  while ( *ptr && (*ptr != ' ' && *ptr != '\t') ) ptr++;
	  while ( *ptr && (*ptr == ' ' || *ptr == '\t') ) ptr++;
	  if ( strlen(ptr) > 0 ) {
	    reader->info = (char *) malloc(strlen(ptr)+1);
	    assert(reader->info);
	    strcpy(reader->info, ptr);
	    reader->pending = STL_END;
	    return STL_EXIT_INFO;
	  }
	  reader->pending = STL_NO_PENDING;
	  return STL_END;
	}

{WS}	{
	}

\r	{
	}

\n	{
	  reader->linenum++;
	}

^#{LINE}$	{
	  /* SIM extension - enable commenting out lines with # */
	}

.	{
	  /* 8k into sphere.stl, flex needs some help to get going again... */
	  if ( reader->hickups < 32 ) {
	    reader->hickups += 1;
	    yymore(); /* where is stl_yymore()? */
	  } else {
	    reader->error = "unknown problem - too many flex hickups";
	    reader->pending = STL_ERROR;
	    return STL_ERROR;
	  }
	}

<<EOF>>	{
	  reader->error = "premature end of file";
	  reader->pending = STL_ERROR;
	  return STL_ERROR;
	}

%%

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

/* ********************************************************************** */
/* internal functions */

#define STL_SCAN_TO_REAL(strptr) \
  do { \
    int roll = TRUE; \
    while ( roll ) { \
      switch ( *strptr ) { \
      case '\0': \
        return FALSE; \
      case '0': case '1': case '2': case '3': case '4': \
      case '5': case '6': case '7': case '8': case '9': \
      case '-': case '+': case '.': \
        roll = FALSE; \
        break; \
      default: \
        strptr++; \
        break; \
      } \
    } \
  } while ( FALSE )

#define STL_SCAN_TO_WHITESPACE(strptr) \
  do { \
    int roll = TRUE; \
    while ( roll ) { \
      switch ( *strptr ) { \
      case '\0': \
        return FALSE; \
      case ' ': case '\t': \
        roll = FALSE; \
        break; \
      default: \
        strptr++; \
        break; \
      } \
    } \
  } while ( FALSE )

int
stl_parse_real_triple(char * text, stl_real * a, stl_real * b, stl_real * c)
{
  char * real1, * real2, * real3;
  STL_SCAN_TO_REAL(text);
  real1 = text;
  STL_SCAN_TO_WHITESPACE(text);
  STL_SCAN_TO_REAL(text);
  real2 = text;
  STL_SCAN_TO_WHITESPACE(text);
  STL_SCAN_TO_REAL(text);
  real3 = text;
  if ( a ) *a = (stl_real) strtod(real1, NULL);
  if ( b ) *b = (stl_real) strtod(real2, NULL);
  if ( c ) *c = (stl_real) strtod(real3, NULL);
  return TRUE;
}

static
int
stl_host_is_bigendian(void)
{
  static int retval = -1;
  if ( retval == -1 ) {
    union {
      unsigned char bytes[4];
      uint32_t word;
    } data;
    data.word = 0x01;
    if ( data.bytes[3] == 0x01 )
      retval = TRUE;
    else
      retval = FALSE;
  }
  return retval;
}

static
uint32_t
stl_ntohl(uint32_t word)
{
  if ( stl_host_is_bigendian() ) {
    uint32_t swapped =
      ((word & 0x000000ff) << 24) | ((word & 0x0000ff00) <<  8) |
      ((word & 0x00ff0000) >>  8) | ((word & 0xff000000) >> 24);
    return swapped;
  }
  return word;
}

static
void
stl_reader_binary_facet(stl_reader * reader)
{
  int readok = 1; 
  // FIXME: use one 50-byte read operation instead
  union {
    unsigned char bytes[4];
    uint32_t data;
    float real;
  } data;


  assert(reader != NULL);
  assert(reader->file != NULL);
  assert(reader->facet != NULL);
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->nx = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->ny = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->nz = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v1x = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v1y = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v1z = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v2x = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v2y = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v2z = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v3x = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v3y = data.real;
  readok &= fread(&data.bytes, 4, 1, reader->file);
  data.data = stl_ntohl(data.data);
  reader->facet->v3z = data.real;
  readok &= fread(&data.bytes, 2, 1, reader->file);
  /* byteswap? */
  reader->facet->color = data.bytes[0] | (data.bytes[1] << 8);
  /* fprintf(stderr, "  color : 0x%04x\n", reader->facet->color); */
  reader->facets++;
}

static
int
stl_writer_put_binary_facet(stl_writer * writer, stl_facet * facet)
{
  int writeok = 1;
  union {
    unsigned char bytes[4];
    uint32_t data;
    float real;
  } data;
  assert(writer != NULL);
  assert(writer->file != NULL);
  assert(writer->facet != NULL);
  data.real = writer->facet->nx;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->ny;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->nz;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v1x;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v1y;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v1z;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v2x;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v2y;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v2z;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v3x;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v3y;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);
  data.real = writer->facet->v3z;
  data.data = stl_ntohl(data.data);
  writeok &= fwrite(&data.bytes, 4, 1, writer->file);

  data.bytes[0] = writer->facet->color & 0xff;
  data.bytes[1] = (writer->facet->color >> 8) & 0xff;
  /* byteswap? */
  writeok &= fwrite(&data.bytes, 2, 1, writer->file);
  /* fprintf(stderr, "  color : 0x%04x\n", reader->facet->color); */

  return TRUE;
}

/* ********************************************************************** */

/* @STL_STEEL_MAJOR_VERSION@
This define is the major part (#.-.-) of the steel release version number.
It is provided for knowing which version number you link with so it can
be compared to the steel version loaded at runtime.
 */

/* @STL_STEEL_MINOR_VERSION@
This define is the minor part (-.#.-) of the steel release version number.
It is provided for knowing which version number you link with so it can
be compared to the steel version loaded at runtime.
 */

/* @STL_STEEL_MICRO_VERSION@
This define is the micro part (-.-.#) of the steel release version number.
It is provided for knowing which version number you link with so it can
be compared to the steel version loaded at runtime.
 */

/* @STL_STEEL_ABI_VERSION@
This define is the ABI version of the steel library you link with.
It is provided for knowing which steel ABI you link with so it can be
compared to the ABI of the steel library loaded at runtime.
*/

/* @STL_STEEL_ABI_REVISION@
This define is the revision of the steel ABI version you link with.
It is provided for knowing which steel ABI you link with so it can be
compared to the ABI of the steel library loaded at runtime.
*/

/* @STL_STEEL_ABI_AGE@
This define is the age of the steel ABI you link with.
It is provided for knowing which steel ABI you link with so it can be
compared to the ABI of the steel library loaded at runtime.
*/

/* ********************************************************************** */

/* @stl_steel_major@
This function returns the major part (#.-.-) of the steel release version
number.
 */

int
stl_steel_major(void)
{
  return STL_STEEL_MAJOR;
}

/* @stl_steel_minor@
This function returns the minor part (-.#.-) of the steel release version
number.
 */

int
stl_steel_minor(void)
{
  return STL_STEEL_MINOR;
}

/* @stl_steel_micro@
This function returns the micro part (-.-.#) of the steel release version
number.
 */

int
stl_steel_micro(void)
{
  return STL_STEEL_MICRO;
}

/* @stl_steel_abi_version@
This function returns the version of the steel library ABI.  It is the same
as the libtool "current" number.
 */

int
stl_steel_abi_version(void)
{
  return STL_STEEL_ABI_VERSION;
}

/* @stl_steel_abi_revision@
This function returns the revision of the current steel library ABI version.
It is the same as the libtool "revision" number.
 */

int
stl_steel_abi_revision(void)
{
  return STL_STEEL_ABI_REVISION;
}

/* @stl_steel_abi_age@
This function returns the age of the current steel library ABI.
It is essentially the same as the libtool "age" number.
 */

int
stl_steel_abi_age(void)
{
  return STL_STEEL_ABI_AGE;
}

/* @stl_steel_abi_supported@
This function returns TRUE if the requested ABI version is supported and
FALSE otherwise.
 */

int
stl_steel_abi_supported(int version, int revision)
{
  if ( (version < STL_STEEL_ABI_VERSION) &&
       (version >= (STL_STEEL_ABI_VERSION - STL_STEEL_ABI_AGE)) )
    return TRUE;
  if ( (version == STL_STEEL_ABI_VERSION) &&
       (revision <= STL_STEEL_ABI_REVISION) )
    return TRUE;
  return FALSE;
}

/* ********************************************************************** */

/* @stl_facet_create_uninitialized@
 */

stl_facet *
stl_facet_create_uninitialized(void)
{
  stl_facet * facet;
  facet = (stl_facet *) malloc(sizeof(stl_facet));
  assert(facet);
  return facet;
}

/* @stl_facet_create@
 */

stl_facet *
stl_facet_create(void)
{
  stl_facet * facet;
  facet = stl_facet_create_uninitialized();
  facet->nx = facet->ny = facet->nz = 0.0f;
  facet->v1x = facet->v1y = facet->v1z = 0.0f;
  facet->v2x = facet->v2y = facet->v2z = 0.0f;
  facet->v3x = facet->v3y = facet->v3z = 0.0f;
  facet->color = STL_NO_COLOR;
  return facet;
} /* stl_facet_create() */

/* @stl_facet_clone@
 */

stl_facet *
stl_facet_clone(stl_facet * facet)
{
  stl_facet * clone;
  assert(facet != NULL);
  clone = stl_facet_create_uninitialized();
  stl_facet_copy(facet, clone);
  return clone;
} /* stl_facet_clone() */

/* @stl_facet_destroy@
 */

void
stl_facet_destroy(stl_facet * facet)
{
  assert(facet != NULL);
  free(facet);
} /* stl_facet_destroy() */

/* @stl_facet_copy@
 */

void
stl_facet_copy(stl_facet * source, stl_facet * target)
{
  assert(source != NULL && target != NULL);
  target->nx = source->nx;
  target->ny = source->ny;
  target->nz = source->nz;
  target->v1x = source->v1x;
  target->v1y = source->v1y;
  target->v1z = source->v1z;
  target->v2x = source->v2x;
  target->v2y = source->v2y;
  target->v2z = source->v2z;
  target->v3x = source->v3x;
  target->v3y = source->v3y;
  target->v3z = source->v3z;
  target->color = source->color;
} /* stl_facet_copy() */

/* @stl_facet_set_normal@
 */

void
stl_facet_set_normal(stl_facet * facet, stl_real x, stl_real y, stl_real z)
{
  assert(facet != NULL);
  facet->nx = x;
  facet->ny = y;
  facet->nz = z;
} /* stl_facet_set_normal() */

/* @stl_facet_get_normal@
 */

void
stl_facet_get_normal(stl_facet * facet, stl_real * x, stl_real * y, stl_real * z)
{
  assert(facet != NULL);
  if ( x ) *x = facet->nx;
  if ( y ) *y = facet->ny;
  if ( z ) *z = facet->nz;
} /* stl_facet_get_normal() */

/* @stl_facet_set_vertex1@
 */

void
stl_facet_set_vertex1(stl_facet * facet, stl_real x, stl_real y, stl_real z)
{
  assert(facet != NULL);
  facet->v1x = x;
  facet->v1y = y;
  facet->v1z = z;
} /* stl_facet_set_vertex1() */

/* @stl_facet_get_vertex1@
 */

void
stl_facet_get_vertex1(stl_facet * facet, stl_real * x, stl_real * y, stl_real * z)
{
  assert(facet != NULL);
  if ( x ) *x = facet->v1x;
  if ( y ) *y = facet->v1y;
  if ( z ) *z = facet->v1z;
} /* stl_facet_get_vertex1() */

/* @stl_facet_set_vertex2@
 */

void
stl_facet_set_vertex2(stl_facet * facet, stl_real x, stl_real y, stl_real z)
{
  assert(facet != NULL);
  facet->v2x = x;
  facet->v2y = y;
  facet->v2z = z;
} /* stl_facet_set_vertex2() */

/* @stl_facet_get_vertex2@
 */

void
stl_facet_get_vertex2(stl_facet * facet, stl_real * x, stl_real * y, stl_real * z)
{
  assert(facet != NULL);
  if ( x ) *x = facet->v2x;
  if ( y ) *y = facet->v2y;
  if ( z ) *z = facet->v2z;
} /* stl_facet_get_vertex2() */

/* @stl_facet_set_vertex3@
 */

void
stl_facet_set_vertex3(stl_facet * facet, stl_real x, stl_real y, stl_real z)
{
  assert(facet != NULL);
  facet->v3x = x;
  facet->v3y = y;
  facet->v3z = z;
} /* stl_facet_set_vertex3() */

/* @stl_facet_get_vertex3@
 */

void
stl_facet_get_vertex3(stl_facet * facet, stl_real * x, stl_real * y, stl_real * z)
{
  assert(facet != NULL);
  if ( x ) *x = facet->v3x;
  if ( y ) *y = facet->v3y;
  if ( z ) *z = facet->v3z;
} /* stl_facet_get_vertex3() */

/* @stl_facet_get_padding@
 */

void
stl_facet_set_padding(stl_facet * facet, unsigned int padding)
{
  assert(facet != NULL);
} /* stl_facet_set_padding() */

/* @stl_facet_get_padding@
 */

unsigned int
stl_facet_get_padding(stl_facet * facet)
{
  assert(facet != NULL);
  return 0;
} /* stl_facet_get_padding() */

/* @stl_facet_set_color@
 */

void
stl_facet_set_color(stl_facet * facet, unsigned int rgb)
{
  assert(facet != NULL);
  facet->color = rgb;
} /* stl_facet_set_color() */

/* @stl_facet_get_color@
This function returns the color of the facet, if one has been set.
 */

unsigned int
stl_facet_get_color(stl_facet * facet)
{
  assert(facet != NULL);
  return facet->color;
} /* stl_facet_get_color() */

/* ********************************************************************** */

/* @stl_reader_create@
 */

stl_reader *
stl_reader_create(const char * filename)
{
  stl_reader * reader;
  int id;
  long length;
  unsigned char bytes[4];
  assert(filename != NULL);
  reader = (stl_reader *) malloc(sizeof(stl_reader));
  assert(reader);
  reader->filename = NULL;
  reader->file = NULL;
  reader->info = NULL;
  reader->facet = NULL;
  reader->error = NULL;
  reader->flags = 0;
  reader->linenum = 0;
  reader->pending = STL_NO_PENDING;
  reader->vertex = 0;
  reader->facets = 0;
  reader->facets_total = 0;
  reader->hickups = 0;
  reader->file = fopen(filename, "rb");
  if ( reader->file == NULL ) {
    free(reader);
    return NULL;
  }
  reader->filename = (char *) malloc(strlen(filename)+1);
  assert(reader->filename);
  strcpy(reader->filename, filename);
  reader->facet = stl_facet_create();

  /* check if file is binary stl file first */
  do {
    int readok = 1;
    /* FIXME: scan header for "COLOR=" for the "Materialise" color extension */
    reader->linenum = 0;
    readok &= !fseek(reader->file, 0, SEEK_END);
    length = ftell(reader->file);
    readok &= !fseek(reader->file, 80, SEEK_SET);
    readok &= fread(bytes, 4, 1, reader->file);
    reader->facets_total =
      (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
    if ( (84 + (reader->facets_total * 50)) != length ) {
      break; /* not a binary stl file */
    }
    reader->flags |= STL_BINARY;
    readok &= !fseek(reader->file, 0, SEEK_SET);
    reader->info = static_cast<char *>(malloc(81));
    assert(reader->info);
    readok &= fread(reader->info, 80, 1, reader->file);
    reader->info[80] = '\0';
    readok &= !fseek(reader->file, 84, SEEK_SET); /* position of first facet */
    reader->pending = STL_INIT_INFO;
    return reader;
  } while ( FALSE );

  /* now try ascii stl */
  do {
    int readok = 1;
    reader->linenum = 1;
    reader->file = freopen(reader->filename, "r", reader->file);
    assert(reader->file);
    id = stl_reader_peek(reader);
    if ( id == STL_ERROR ) {
      break; /* not an ascii stl file */
    }
    readok &= !fseek(reader->file, 0, SEEK_SET);
    stl_yyrestart(reader->file);
    reader->pending = STL_NO_PENDING;
    return reader;
  } while ( FALSE );

  /* the file is not an stl file */
  (void)fclose(reader->file);
  free(reader->filename);
  reader->filename = NULL;
  stl_facet_destroy(reader->facet);
  reader->facet = NULL;
  free(reader);
  /* could return a reader with pending STL_ERROR and error message instead? */
  return NULL;
} /* stl_reader_create() */

/* @stl_reader_destroy@
 */

void
stl_reader_destroy(stl_reader * reader)
{
  assert(reader != NULL);
  if ( reader->filename ) {
    free(reader->filename);
    reader->filename = NULL;
  }
  if ( reader->info ) {
    free(reader->info);
    reader->info = NULL;
  }
  if ( reader->file ) {
    fclose(reader->file);
    reader->file = NULL;
  }
  if ( reader->facet ) {
    stl_facet_destroy(reader->facet);
    reader->facet = NULL;
  }
  free(reader);
} /* stl_reader_destroy() */

/* @stl_reader_flags@
 */

unsigned int
stl_reader_flags(stl_reader * reader)
{
  assert(reader != NULL);
  return reader->flags;
} /* stl_reader_flags() */

/* @stl_reader_peek@
 */

int
stl_reader_peek(stl_reader * reader)
{
  int peekval;
  assert(reader != NULL);
  if ( reader->pending != STL_NO_PENDING ) {
    peekval = reader->pending;
    if ( reader->pending == STL_END ) {
      reader->pending = STL_ERROR;
    }
    if ( reader->pending == STL_BEGIN ) {
      if ( reader->info != NULL ) {
        reader->pending = STL_INIT_INFO;
      } else {
        reader->pending = STL_NO_PENDING;
      }
    } else if ( reader->pending != STL_ERROR ) {
      reader->pending = STL_NO_PENDING;
    }
    return peekval;
  }
  if ( !(reader->flags & STL_BINARY) ) {
    stl_yyin = reader->file;
    peekval = stl_scan(reader);
  } else {
    if ( reader->facets == reader->facets_total ) {
      return STL_END;
    }
    stl_reader_binary_facet(reader);
    return STL_FACET;
  }
  if ( reader->error ) {
    return STL_ERROR;
  }
  return peekval;
} /* stl_reader_peek() */

/* @stl_reader_get_info@
 */

const char *
stl_reader_get_info(stl_reader * reader)
{
  assert(reader != NULL);
  return reader->info;
} /* stl_reader_get_info() */

/* @stl_reader_get_facet@
 */

stl_facet *
stl_reader_get_facet(stl_reader * reader)
{
  assert(reader != NULL);
  assert(reader->facet != NULL);
  return stl_facet_clone(reader->facet);
} /* stl_reader_get_facet() */

/* @stl_reader_fill_facet@
 */

void
stl_reader_fill_facet(stl_reader * reader, stl_facet * facet)
{
  assert(reader != NULL);
  assert(reader->facet != NULL);
  facet->nx = reader->facet->nx;
  facet->ny = reader->facet->ny;
  facet->nz = reader->facet->nz;
  facet->v1x = reader->facet->v1x;
  facet->v1y = reader->facet->v1y;
  facet->v1z = reader->facet->v1z;
  facet->v2x = reader->facet->v2x;
  facet->v2y = reader->facet->v2y;
  facet->v2z = reader->facet->v2z;
  facet->v3x = reader->facet->v3x;
  facet->v3y = reader->facet->v3y;
  facet->v3z = reader->facet->v3z;
  facet->color = reader->facet->color;
} /* stl_reader_fill_facet() */

/* @stl_reader_get_error@
 */

const char *
stl_reader_get_error(stl_reader * reader)
{
  assert(reader != NULL);
  return reader->error;
} /* stl_reader_get_error() */

/* @stl_reader_get_line_number@
 */

int
stl_reader_get_line_number(stl_reader * reader)
{
  assert(reader != NULL);
  return reader->linenum;
}

/* ********************************************************************** */

/* @stl_writer_create@
 */

stl_writer *
stl_writer_create(const char * filename, unsigned int flags)
{
  stl_writer * writer;
  assert(filename != NULL);
  writer = (stl_writer *) malloc(sizeof(stl_writer));
  assert(writer);
  writer->filename = (char *) malloc(strlen(filename)+1);
  assert(writer->filename);
  strcpy(writer->filename, filename);
  writer->flags = (flags & STL_PUBLIC_FLAGS);
  if ( writer->flags & STL_BINARY ) {
    writer->file = fopen(writer->filename, "wb");
    assert(writer->file);
    writer->linenum = 0;
  } else {
    writer->file = fopen(writer->filename, "w");
    assert(writer->file);
    writer->linenum = 1;
  }
  assert(writer->file);
  writer->facets = 0;
  writer->error = NULL;
  writer->facet = NULL;
  writer->info = NULL;
  return writer;
} /* stl_writer_create() */

/* @stl_writer_destroy@
 */

int
stl_writer_destroy(stl_writer * writer)
{
  assert(writer != NULL);
  assert(writer->file != NULL);
  if ( writer->flags & STL_BINARY ) {
    int writeok = 1;
    unsigned char bytes[4];
    bytes[3] = (writer->facets >> 24) & 0xff;
    bytes[2] = (writer->facets >> 16) & 0xff;
    bytes[1] = (writer->facets >> 8) & 0xff;
    bytes[0] = writer->facets & 0xff;
    writeok &= !fflush(writer->file);
    writeok &= !fseek(writer->file, 80, SEEK_SET);
    writeok &= fwrite(bytes, 4, 1, writer->file);
  } else {
    fprintf(writer->file, "endsolid\n");
    writer->linenum++;
  }
  fclose(writer->file);
  writer->file = NULL;
  if ( writer->info != NULL ) {
    free(writer->info);
    writer->info = NULL;
  }
  if ( writer->facet != NULL ) {
    stl_facet_destroy(writer->facet);
    writer->facet = NULL;
  }
  free(writer);
  return STL_OK;
} /* stl_writer_destroy() */

/* @stl_writer_flags@
 */

unsigned int
stl_writer_get_flags(stl_writer * writer)
{
  assert(writer != NULL);
  return writer->flags & STL_PUBLIC_FLAGS;
} /* stl_writer_flags() */

/* @stl_writer_put_info@
 */

int
stl_writer_set_info(stl_writer * writer, const char * info)
{
  assert(writer != NULL);
  assert(writer->file != NULL);
  if ( writer->info != NULL ) {
    free(writer->info);
    writer->info = NULL;
  }
  if ( info == NULL ) {
    return STL_OK;
  }
  if ( writer->facets != 0 ) {
    writer->error =
      "programming error - info must be set before writing facets";
    return STL_ERROR;
  }
  if ( strlen(info) > 80 ) {
    writer->error = "too long info string";
    return STL_ERROR;
  }
  writer->info = (char *) malloc(strlen(info)+1);
  assert(writer->info);
  strcpy(writer->info, info);
  return STL_OK;
} /* stl_writer_put_info() */

/* @stl_writer_set_facet@
 */

void
stl_writer_set_facet(stl_writer * writer, stl_facet * facet)
{
  assert(writer);
  writer->facet = facet;
}

/* @stl_writer_get_facet@
 */

stl_facet *
stl_writer_get_facet(stl_writer * writer)
{
  assert(writer);
  return writer->facet;
}

/* @stl_writer_put_facet@
 */

int
stl_writer_put_facet(stl_writer * writer, stl_facet * facet)
{
  assert(writer != NULL);
  assert(writer->file != NULL);
  assert(facet);

  if ( writer->facets == 0 ) {
    /* write info */
    if ( writer->flags & STL_BINARY ) {
      /* FIXME: take color extension into account when constructing header */
      char header[84];
      memset(header, 0, 84);
      if ( writer->info ) {
        if ( strlen(writer->info) < 80 ) {
          strcpy(header, writer->info);
        }
      }
      if ( fwrite(header, 84, 1, writer->file) != 1 ) {
        writer->error = "writing header failed";
        return STL_ERROR;
      }
    } else {
      if ( writer->info ) {
        fprintf(writer->file, "solid %s\n", writer->info);
        free(writer->info);
        writer->info = NULL;
      } else {
        fprintf(writer->file, "solid\n");
      }
    }
  }

  if ( writer->flags & STL_BINARY ) {
    stl_writer_put_binary_facet(writer, facet);
  } else {
    float x, y, z;
    stl_facet_get_normal(facet, &x, &y, &z);
    fprintf(writer->file, "  facet normal %g %g %g\n", x, y, z);
    writer->linenum++;
    fprintf(writer->file, "    outer loop\n");
    writer->linenum++;
    stl_facet_get_vertex1(facet, &x, &y, &z);
    fprintf(writer->file, "      vertex %g %g %g\n", x, y, z);
    writer->linenum++;
    stl_facet_get_vertex2(facet, &x, &y, &z);
    fprintf(writer->file, "      vertex %g %g %g\n", x, y, z);
    writer->linenum++;
    stl_facet_get_vertex3(facet, &x, &y, &z);
    fprintf(writer->file, "      vertex %g %g %g\n", x, y, z);
    writer->linenum++;
    fprintf(writer->file, "    endloop\n");
    writer->linenum++;
    fprintf(writer->file, "  endfacet\n");
    writer->linenum++;
  }
  // fflush(writer->file);
  writer->facets++;
  return STL_OK;
} /* stl_writer_put_facet() */

/* @stl_writer_get_error@
This function returns the last error message for the writer object.
 */

const char *
stl_writer_get_error(stl_writer * writer)
{
  assert(writer != NULL);
  return writer->error;
} /* stl_writer_get_error() */

/* ********************************************************************** */
