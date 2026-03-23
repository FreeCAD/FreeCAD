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

#ifndef COIN_TIDBITS_H
#define COIN_TIDBITS_H

#include <Inventor/C/basic.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus */

/* ********************************************************************** */

enum CoinEndiannessValues {
  COIN_HOST_IS_UNKNOWNENDIAN = -1,
  COIN_HOST_IS_LITTLEENDIAN = 0,
  COIN_HOST_IS_BIGENDIAN = 1
};

COIN_DLL_API int coin_host_get_endianness(void);

COIN_DLL_API int coin_snprintf(char * dst, unsigned int n, const char * fmtstr, ...);
COIN_DLL_API int coin_vsnprintf(char * dst, unsigned int n, const char * fmtstr, va_list args);

COIN_DLL_API const char * coin_getenv(const char * name);
COIN_DLL_API SbBool coin_setenv(const char * name, const char * value, int overwrite);
COIN_DLL_API void coin_unsetenv(const char * name);

COIN_DLL_API int coin_strncasecmp(const char * str1, const char * str2, int len);
  
COIN_DLL_API uint16_t coin_hton_uint16(uint16_t value);
COIN_DLL_API uint16_t coin_ntoh_uint16(uint16_t value);
COIN_DLL_API uint32_t coin_hton_uint32(uint32_t value);
COIN_DLL_API uint32_t coin_ntoh_uint32(uint32_t value);
COIN_DLL_API uint64_t coin_hton_uint64(uint64_t value);
COIN_DLL_API uint64_t coin_ntoh_uint64(uint64_t value);

COIN_DLL_API void coin_hton_float_bytes(float value, char * result); /* expects room for 4 bytes in result*/
COIN_DLL_API float coin_ntoh_float_bytes(const char * value);   /* expects 4 bytes input */

COIN_DLL_API void coin_hton_double_bytes(double value, char * result); /* expects room for 8 bytes in result */
COIN_DLL_API double coin_ntoh_double_bytes(const char * value); /* expects 8 bytes input */

COIN_DLL_API SbBool coin_isascii(const int c);
COIN_DLL_API SbBool coin_isspace(const char c);

COIN_DLL_API SbBool coin_is_power_of_two(uint32_t x);
COIN_DLL_API uint32_t coin_next_power_of_two(uint32_t x);
COIN_DLL_API uint32_t coin_geq_power_of_two(uint32_t x);

COIN_DLL_API void coin_viewvolume_jitter(int numpasses, int curpass, const int * vpsize, float * jitter);

typedef void coin_atexit_f(void);
COIN_DLL_API void cc_coin_atexit(coin_atexit_f * fp);

/* Used internally to clean up static data. Do not use in application code */
COIN_DLL_API void cc_coin_atexit_static_internal(coin_atexit_f * fp);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !COIN_TIDBITS_H */
