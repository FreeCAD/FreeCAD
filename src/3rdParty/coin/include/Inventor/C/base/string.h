#ifndef CC_STRING_H
#define CC_STRING_H

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

#include <Inventor/C/basic.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ********************************************************************** */

enum cc_string_constants {
  CC_STRING_MIN_SIZE = 128 - sizeof(char *) + sizeof(size_t),
  CC_STRING_RESIZE   = 128
};

struct cc_string {
  char * pointer;
  size_t bufsize;
  char buffer[CC_STRING_MIN_SIZE];
};

typedef  struct cc_string  cc_string;

typedef char (*cc_apply_f)(char);

/* ********************************************************************** */

COIN_DLL_API void cc_string_construct(cc_string * me);
COIN_DLL_API cc_string * cc_string_construct_new(void);
COIN_DLL_API cc_string * cc_string_clone(const cc_string * str);
COIN_DLL_API void cc_string_clean(cc_string * str);
COIN_DLL_API void cc_string_destruct(cc_string * str);

COIN_DLL_API void cc_string_set_string(cc_string * str, const cc_string * str2);
COIN_DLL_API void cc_string_set_text(cc_string * str, const char * text);
COIN_DLL_API void cc_string_set_subtext(cc_string * str, const char * text, int start, int end);
COIN_DLL_API void cc_string_set_integer(cc_string * str, int integer);

COIN_DLL_API void cc_string_append_string(cc_string * str, const cc_string * str2);
COIN_DLL_API void cc_string_append_text(cc_string * str, const char * text);
COIN_DLL_API void cc_string_append_integer(cc_string * str, const int digits);
COIN_DLL_API void cc_string_append_char(cc_string * str, const char c);

COIN_DLL_API unsigned int cc_string_length(const cc_string * str);
COIN_DLL_API int cc_string_is(const cc_string * str);
COIN_DLL_API void cc_string_clear(cc_string * str);
COIN_DLL_API void cc_string_clear_no_free(cc_string * str);
COIN_DLL_API uint32_t cc_string_hash(const cc_string * str);
COIN_DLL_API uint32_t cc_string_hash_text(const char * text);

COIN_DLL_API const char * cc_string_get_text(const cc_string * str);
COIN_DLL_API void cc_string_remove_substring(cc_string * str, int start, int end);

COIN_DLL_API int cc_string_compare(const cc_string * lhs, const cc_string * rhs);
COIN_DLL_API int cc_string_compare_text(const char * lhs, const char * rhs);
COIN_DLL_API int cc_string_compare_subtext(const cc_string * str, const char * text, int offset);

COIN_DLL_API void cc_string_apply(cc_string * str, cc_apply_f function);

COIN_DLL_API void cc_string_sprintf(cc_string * str, const char * formatstr, ...);
COIN_DLL_API void cc_string_vsprintf(cc_string * str, const char * formatstr, va_list args);

COIN_DLL_API size_t cc_string_utf8_decode(const char * src, size_t srclen, uint32_t * value);
COIN_DLL_API size_t cc_string_utf8_encode(char * buffer, size_t buflen, uint32_t value);
COIN_DLL_API uint32_t cc_string_utf8_get_char(const char * str);
COIN_DLL_API const char * cc_string_utf8_next_char(const char * str);
COIN_DLL_API size_t cc_string_utf8_validate_length(const char * str);

COIN_DLL_API void cc_string_set_wtext(cc_string * str, const wchar_t * text);


/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_STRING_H */
