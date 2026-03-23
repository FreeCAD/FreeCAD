#ifndef COIN_GLUE_ZLIB_H
#define COIN_GLUE_ZLIB_H

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

#include <Inventor/C/basic.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0 /* to get proper auto-indentation in emacs */
}
#endif /* emacs indentation */

int cc_zlibglue_available(void);
int cc_zlibglue_deflateInit2(void * stream,
                             int level,
                             int method,
                             int windowbits,
                             int memlevel,
                             int strategy);

int cc_zlibglue_inflateInit2(void * stream,
                             int windowbits);

int cc_zlibglue_deflateEnd(void * stream);
int cc_zlibglue_inflateEnd(void * stream);
int cc_zlibglue_inflate(void * stream, int flush);
int cc_zlibglue_inflateReset(void * stream);
int cc_zlibglue_deflateParams(void * stream, int level, int strategy);
int cc_zlibglue_deflate(void * stream, int flush);

void * cc_zlibglue_gzopen(const char * path, const char * mode);
void * cc_zlibglue_gzdopen(int fd, const char * mode);
int cc_zlibglue_gzsetparams(void * fp, int level, int strategy);
int cc_zlibglue_gzread(void * fp, void * buf, unsigned int len);
int cc_zlibglue_gzwrite(void * fp, const void * buf, unsigned int len);
off_t cc_zlibglue_gzseek(void * fp, off_t offset, int whence);
int cc_zlibglue_gzrewind(void * fp);
off_t cc_zlibglue_gztell(void * fp);
int cc_zlibglue_gzeof(void * fp);
int cc_zlibglue_gzclose(void * fp);
int cc_zlibglue_crc32(unsigned long crc, const char * buf, unsigned int len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !COIN_GLUE_ZLIB_H */
