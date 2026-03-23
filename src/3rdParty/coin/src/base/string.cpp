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

#include <Inventor/C/base/string.h>

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>

#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strlen;
using std::strcpy;
using std::strncpy;
using std::strcat;
using std::strcmp;
using std::strncmp;
using std::malloc;
using std::free;
using std::memmove;
using std::atoi;
using std::printf;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/* ********************************************************************** */

/* FIXME:

   - get rid of strlen() invocations
   - use the cc_memalloc interface?

   20020513 mortene.
*/

/*!
  \struct cc_string string.h Inventor/C/base/string.h
  \brief The cc_string type is a C ADT for ASCII string management.

  \ingroup coin_base

  This is a Coin extension.
*/

/* ********************************************************************** */

/*!
  \relates cc_string
*/

void
cc_string_remove_substring(cc_string * me, int start, int end)
{
  const int len = static_cast<int>(strlen(me->pointer));
  if ( end == -1 ) end = len - 1;

  assert(!(start < 0 || start >= len || end < 0 || end >= len || start > end) &&
         "invalid arguments for cc_string_remove_substring()");

  (void) memmove(me->pointer + start, me->pointer + end + 1, len - end);
}

static void
cc_string_grow_buffer(cc_string * me, size_t newsize)
{
  char * newbuf;
  static int debug = -1;

  if (debug == -1) {
    const char * env = coin_getenv("COIN_DEBUG_STRING_GROW");
    debug = (env && (atoi(env) > 0)) ? 1 : 0;
  }

  /* Can not use cc_debugerror_* interface(), as that could cause an
     infinite recursion. */
  if (debug) {
    printf("cc_string_grow_buffer: "
           "me->bufsize==%zu, me->pointer==%p, me->buffer==%p => "
           "newsize==%zu\n",
           me->bufsize, me->pointer, me->buffer, newsize);
  }


  if (newsize <= me->bufsize) { return; }

  /* Should first try the vastly more efficient realloc() (if
     the current memory buffer is not the default static, of course). */
  if (me->pointer != me->buffer) {
    newbuf = static_cast<char *>(realloc(me->pointer, newsize));
    if (debug) { printf("cc_string_grow_buffer: newbuf==%p\n", newbuf); }
    assert(newbuf != NULL);
  } else {
    newbuf = static_cast<char *>(malloc(newsize));
    if (debug) { printf("cc_string_grow_buffer: newbuf==%p\n", newbuf); }
    assert(newbuf != NULL);

    (void) strcpy(newbuf, me->pointer);

    // don't free the default static me->buffer
  }

  me->pointer = newbuf;
  me->bufsize = newsize;
}

static void
cc_string_expand(cc_string * me, size_t additional)
{
  const size_t newsize = strlen(me->pointer) + additional + 1;
  cc_string_grow_buffer(me, newsize);
}

/* ********************************************************************** */

/*!
  \relates cc_string
*/

void
cc_string_construct(cc_string * me)
{
  me->pointer = me->buffer;
  me->bufsize = CC_STRING_MIN_SIZE;
  me->buffer[0] = '\0';
} /* cc_string_construct() */

/*!
  \relates cc_string
*/

cc_string *
cc_string_construct_new(void)
{
  cc_string * me;
  me = static_cast<cc_string *>(malloc(sizeof(cc_string)));
  assert(me != NULL);
  cc_string_construct(me);
  return me;
} /* cc_string_construct_new() */

/*!
  \relates cc_string
*/

cc_string *
cc_string_clone(const cc_string * string)
{
  cc_string * me;
  me = cc_string_construct_new();
  cc_string_set_text(me, string->pointer);
  return me;
} /* cc_string_clone() */

/*!
  \relates cc_string
*/

void
cc_string_clean(cc_string * string_struct)
{
  if ( string_struct->pointer != string_struct->buffer )
    free(string_struct->pointer);
} /* cc_string_clean() */

/*!
  \relates cc_string
*/

void
cc_string_destruct(cc_string * me)
{
  assert(me != NULL);
  cc_string_clean(me);
  free(me);
} /* cc_string_destruct() */

/* ********************************************************************** */

/*!
  \relates cc_string
*/

void
cc_string_set_text(cc_string * me, const char * text)
{
  static char emptystring[] = "";
  size_t size;
  if ( text == NULL ) text = emptystring;

  if ( text >= me->pointer && text < (me->pointer + me->bufsize) ) {
    /* text is within own buffer */
    const ptrdiff_t range = text - me->pointer;
    cc_string_remove_substring(me, 0, static_cast<int>(range));
    return;
  }
  size = strlen(text) + 1;
  if (size > me->bufsize) { cc_string_grow_buffer(me, size); }
  (void) strcpy(me->pointer, text);
} /* cc_string_set_text() */

static
size_t
cc_string_strnlen(const char * text, size_t max)
{
  size_t n = 0;
  while(n < max && text[n] != '\0')
    ++n;
  return n;
}

/*!
  \relates cc_string
*/

void
cc_string_set_subtext(cc_string * me, const char * text, int start, int end)
{
  static const char * emptystring = "";
  int len;
  size_t size;

  if ( text == NULL ) text = emptystring;
  len = (int)cc_string_strnlen(text,end);
  if ( end == -1 ) end = len - 1;

#if COIN_DEBUG
  if (start<0) {
    cc_debugerror_postwarning("cc_string_set_subtext",
                              "start index (%d) should be >= 0. Clamped to 0.",
                              start);
    start=0;
  }
  else if (start>len) {
    cc_debugerror_postwarning("cc_string_set_subtext",
                              "start index (%d) is out of bounds [0, %d>. "
                              "Clamped to %d.", start, len, len-1);
    start=len;
  }
  if (end<0) {
    cc_debugerror_postwarning("cc_string_set_subtext",
                              "end index (%d) should be >= 0. Clamped to 0.",
                              end);
    end=0;
  }
  else if (end>len) {
    cc_debugerror_postwarning("cc_string_set_subtext",
                              "end index (%d) is out of bounds [0, %d>. "
                              "Clamped to %d.", end, len, len-1);
    end=len;
  }
  if (start>end+1) {
    cc_debugerror_postwarning("cc_string_set_subtext",
                              "start index (%d) is greater than end index "
                              "(%d). Empty string created.", start, end);
    start=0;
    end=-1;
  }
#endif /* COIN_DEBUG */

  /* if new substring fits in internal buffer, freeing allocated
     buffer will not happen - should this be changed? */
  size = end - start + 1;
  if ( size >= me->bufsize ) {
    if ( me->pointer != me->buffer )
      free(me->pointer);
    me->pointer = static_cast<char *>(malloc(size + 1));
    me->bufsize = size + 1;
  }
  (void) strncpy(me->pointer, text + start, size);
  me->pointer[size] = '\0';
} /* cc_string_set_subtext() */

/*!
  \relates cc_string
*/

void
cc_string_set_integer(cc_string * me, int integer)
{
  me->pointer[0] = '\0';
  cc_string_append_integer(me, integer);
} /* cc_string_set_integer() */

/*!
  \relates cc_string
*/

void
cc_string_set_string(cc_string * me, const cc_string * string)
{
  cc_string_set_text(me, string->pointer);
} /* cc_string_set_string() */

/* ********************************************************************** */

/*!
  \relates cc_string
*/

void
cc_string_append_string(cc_string * me, const cc_string * string)
{
  cc_string_append_text(me, string->pointer);
} /* cc_string_append_string() */

/*!
  \relates cc_string
*/

void
cc_string_append_text(cc_string * me, const char * text)
{
  if ( text ) {
    cc_string_expand(me, strlen(text));
    (void) strcat(me->pointer, text);
  }
} /* cc_string_append_text() */

/*!
  \relates cc_string
*/

void
cc_string_append_integer(cc_string * me, const int digits)
{
  cc_string s;
  cc_string_construct(&s);
  (void)cc_string_sprintf(&s, "%d", digits);
  cc_string_append_string(me, &s);
  cc_string_clean(&s);
} /* cc_string_append_integer() */

/*!
  \relates cc_string
*/

void
cc_string_append_char(cc_string * me, const char c)
{
  size_t pos;
  cc_string_expand(me, 1);
  pos = strlen(me->pointer);
  me->pointer[pos] = c;
  me->pointer[pos+1] = '\0';
} /* cc_string_append_char() */

/* ********************************************************************** */

/*!
  \relates cc_string
*/

unsigned int
cc_string_length(const cc_string * me)
{
  /* FIXME: should cache the length of the string. 20020513 mortene. */
  return static_cast<unsigned int>(strlen(me->pointer));
}

/*!
  \relates cc_string
*/

void
cc_string_clear(cc_string * me)
{
  if ( me->pointer != me->buffer ) {
    free(me->pointer);
    me->pointer = me->buffer;
    me->bufsize = CC_STRING_MIN_SIZE;
  }
  me->pointer[0] = '\0';
} /* cc_string_clear() */

/*!
  \relates cc_string
*/

void
cc_string_clear_no_free(cc_string * me)
{
  me->pointer[0] = '\0';
} /* cc_string_clear_no_free() */

/*!
  \relates cc_string
*/

uint32_t
cc_string_hash_text(const char * text)
{
  uint32_t total, shift;
  total = shift = 0;
  while ( *text ) {
    total = total ^ ((*text) << shift);
    shift += 5;
    if ( shift > 24 ) shift -= 24;
    text++;
  }
  return total;
}

/*!
  \relates cc_string
*/

uint32_t
cc_string_hash(const cc_string * me)
{
  return cc_string_hash_text(me->pointer);
} /* cc_string_hash() */

/*!
  \relates cc_string
*/

const char *
cc_string_get_text(const cc_string * me)
{
  return me->pointer;
} /* cc_string_get_text() */

/* ********************************************************************** */

/*!
  \relates cc_string
*/

int
cc_string_is(const cc_string * me)
{
  return (me->pointer[0] != '\0');
} /* cc_string_is() */

/*!
  \relates cc_string
*/

int
cc_string_compare(const cc_string * lhs, const cc_string * rhs)
{
  return cc_string_compare_text(lhs->pointer, rhs->pointer);
} /* cc_string_compare() */

/*!
  \relates cc_string
*/

int
cc_string_compare_text(const char * lhs, const char * rhs)
{
  return strcmp(lhs ? lhs : "", rhs ? rhs : "");
} /* cc_string_compare_text() */

/*!
  \relates cc_string
*/

int
cc_string_compare_subtext(const cc_string * me, const char * text, int offset)
{
  /* FIXME: assert on invalid offset */
  return strncmp(me->pointer + offset, text, strlen(text));
} /* cc_string_compare_prefix() */

/* ********************************************************************** */

/*!
  \relates cc_string
*/

void
cc_string_apply(cc_string * string, cc_apply_f function)
{
  int len, i;
  assert(function != NULL);
  len = cc_string_length(string);
  for ( i = 0; i < len; i++ )
    string->pointer[i] = function(string->pointer[i]);
}

/* ********************************************************************** */

/*!
  \relates cc_string
*/

void
cc_string_sprintf(cc_string * me, const char * formatstr, ...)
{
  va_list args;
  va_start(args, formatstr);
  cc_string_vsprintf(me, formatstr, args);
  va_end(args);
} /* cc_string_sprintf() */

/*!
  \relates cc_string
*/

void
cc_string_vsprintf(cc_string * me, const char * formatstr, va_list args)
{
  int length;
  SbBool expand;

  do {
    length = coin_vsnprintf(me->pointer, (unsigned int)me->bufsize, formatstr, args);
    expand = (length == -1);
    if ( expand ) {
      /* Note: On Microsoft Windows, using Microsoft's CRT, _vsnprintf(),
         called by coin_vsnprintf(), doesn't add a terminating '0' at
         the end of the buffer if the number of characters to write is
         equal to or larger than the buffer size (second parameter to
         _vsnprintf). This is documented in MSDN's entry for
         _vsnprintf().

         To make sure me->buffer never contains a string that is not
         '0'-terminated, we clear the buffer below before we grow it
         and retry coin_vsnprintf().

         20070927 thammer.  */
      cc_string_clear_no_free(me);
      /* increase linearly in 1Kb intervals */
      cc_string_grow_buffer(me, me->bufsize + 1024);
    }
  } while ( expand );
} /* cc_string_vsprintf() */

/* ********************************************************************** */

size_t
cc_string_utf8_decode(const char * src, size_t srclen, uint32_t * value)
{
  const unsigned char * s = reinterpret_cast<const unsigned char *>(src);

  if ((s[0] & 0x80) == 0x00) {                    // Check s[0] == 0xxxxxxx
    *value = s[0];
    return 1;
  }
  if ((srclen < 2) || ((s[1] & 0xC0) != 0x80)) {  // Check s[1] != 10xxxxxx
    return 0;
  }
  // Accumulate the trailer byte values in value16, and combine it with the
  // relevant bits from s[0], once we've determined the sequence length.
  uint32_t value16 = (s[1] & 0x3F);
  if ((s[0] & 0xE0) == 0xC0) {                    // Check s[0] == 110xxxxx
    *value = ((s[0] & 0x1F) << 6) | value16;
    return 2;
  }
  if ((srclen < 3) || ((s[2] & 0xC0) != 0x80)) {  // Check s[2] != 10xxxxxx
    return 0;
  }
  value16 = (value16 << 6) | (s[2] & 0x3F);
  if ((s[0] & 0xF0) == 0xE0) {                    // Check s[0] == 1110xxxx
    *value = ((s[0] & 0x0F) << 12) | value16;
    return 3;
  }
  if ((srclen < 4) || ((s[3] & 0xC0) != 0x80)) {  // Check s[3] != 10xxxxxx
    return 0;
  }
  value16 = (value16 << 6) | (s[3] & 0x3F);
  if ((s[0] & 0xF8) == 0xF0) {                    // Check s[0] == 11110xxx
    *value = ((s[0] & 0x07) << 18) | value16;
    return 4;
  }
  return 0;
}

size_t
cc_string_utf8_encode(char * buffer, size_t buflen, uint32_t value)
{
  if ((value <= 0x7F) && (buflen >= 1)) {
    buffer[0] = static_cast<unsigned char>(value);
    return 1;
  }
  if ((value <= 0x7FF) && (buflen >= 2)) {
    buffer[0] = 0xC0 | static_cast<unsigned char>(value >> 6);
    buffer[1] = 0x80 | static_cast<unsigned char>(value & 0x3F);
    return 2;
  }
  if ((value <= 0xFFFF) && (buflen >= 3)) {
    buffer[0] = 0xE0 | static_cast<unsigned char>(value >> 12);
    buffer[1] = 0x80 | static_cast<unsigned char>((value >> 6) & 0x3F);
    buffer[2] = 0x80 | static_cast<unsigned char>(value & 0x3F);
    return 3;
  }
  if ((value <= 0x1FFFFF) && (buflen >= 4)) {
    buffer[0] = 0xF0 | static_cast<unsigned char>(value >> 18);
    buffer[1] = 0x80 | static_cast<unsigned char>((value >> 12) & 0x3F);
    buffer[2] = 0x80 | static_cast<unsigned char>((value >> 6) & 0x3F);
    buffer[3] = 0x80 | static_cast<unsigned char>(value & 0x3F);
    return 4;
  }
  return 0;
}

uint32_t
cc_string_utf8_get_char(const char * str)
{
  static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);
  uint32_t value = 0;
  size_t declen = 0;

  if (disable_utf8) {
    value = static_cast<uint8_t>(*str);
  } else {
    declen = cc_string_utf8_decode(str, strlen(str), &value);
    if (!declen) {
      cc_debugerror_postinfo("cc_string_utf8_get_char",
			     "UTF-8 decoding of string \"%s\" failed.\n\n"
			     "To disable UTF-8 support and fall back to pre"
			     "Coin 4.0 behavior, set the\nenvironment variable "
			     "COIN_DISABLE_UTF8=1 and re-run the application.\n", str);
    }
  }
  return value;
}

const char *
cc_string_utf8_next_char(const char * str)
{
  static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);
  uint32_t value = 0;
  size_t declen = 0;

  if (disable_utf8) {
    declen = 1;
  } else {
    declen = cc_string_utf8_decode(str, strlen(str), &value);
    if (!declen) {
      cc_debugerror_postinfo("cc_string_utf8_get_char",
			     "UTF-8 decoding of string \"%s\" failed.\n\n"
			     "To disable UTF-8 support and fall back to pre"
			     "Coin 4.0 behavior, set the\nenvironment variable "
			     "COIN_DISABLE_UTF8=1 and re-run the application.\n", str);
    }
  }
  return str+declen;
}

size_t
cc_string_utf8_validate_length(const char * str)
{
  static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);
  const char * s = str;
  size_t declen = 0;
  size_t srclen = strlen(str);
  size_t utf8len = 0;
  uint32_t value = 0;

  if (disable_utf8) {
    utf8len = srclen;
  } else {
    while (srclen) {
      if (!(declen = cc_string_utf8_decode(s, srclen, &value))) {
	cc_debugerror_postinfo("cc_string_utf8_get_char",
			       "UTF-8 decoding of string \"%s\" failed.\n\n"
			       "To disable UTF-8 support and fall back to pre"
			       "Coin 4.0 behavior, set the\nenvironment variable "
			       "COIN_DISABLE_UTF8=1 and re-run the application.\n", str);
	return 0;
      }
      srclen -= declen;
      s += declen;
      ++utf8len;
    }
  }

  return utf8len;
}


#if defined HAVE_WINDOWS_H
#include <windows.h> // for WideCharToMultiByte
#endif

void cc_string_set_wtext(cc_string * me, const wchar_t * text)
{
  if ( text == NULL ) {
    // empty string
    cc_string_set_text(me, NULL);
  } else {
    static const int disable_utf8 = (coin_getenv("COIN_DISABLE_UTF8") != NULL);
    if (disable_utf8) {
      // convert using current locale instead of UTF-8
      cc_string_sprintf(me, "%ls", text);
    } else {
      #if defined HAVE_WINDOWS_H
      // use WideCharToMultiByte for Windows systems (UTF-16 encoding for wchar_t)
      int newsize = ::WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
      cc_string_grow_buffer(me, newsize);
      ::WideCharToMultiByte(CP_UTF8, 0, text, -1, me->pointer, newsize, NULL, NULL);
      #else
      // other systems use UTF-32
      if (sizeof(wchar_t) == 4) {
        const wchar_t * readptr = text;
        size_t writepos = 0;
        uint32_t value = 0;
        do {
          if ((me->bufsize - writepos) < 4) {
            // enlarge in bigger chunks for performance reasons
            cc_string_grow_buffer(me, me->bufsize + CC_STRING_RESIZE);
          }
          value = *readptr++;
          writepos += cc_string_utf8_encode(me->pointer + writepos, 
                                            me->bufsize - writepos, value);
        } while (value);
      } else {
        cc_debugerror_postinfo("cc_string_set_wtext",
                   "UTF-8 encoding of string \"%ls\" failed "
                   "(unsupported wchar_t size).\n\n"
                   "To disable UTF-8 support and fall back to pre"
                   "Coin 4.0 behavior, set the\nenvironment variable "
                   "COIN_DISABLE_UTF8=1 and re-run the application.\n", text);
      }
      #endif
    }
  }
} /* cc_string_set_wtext() */

/* ********************************************************************** */
