#ifndef STL_STEEL_H
#define STL_STEEL_H

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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ********************************************************************** */

#define  STL_STEEL_MAJOR           (0)
#define  STL_STEEL_MINOR           (5)
#define  STL_STEEL_MICRO           (0)
#define  STL_STEEL_ABI_VERSION     (0)
#define  STL_STEEL_ABI_REVISION    (0)
#define  STL_STEEL_ABI_AGE         (0)

#define  STL_ERROR                (-1)
#define  STL_OK                    (0)
#define  STL_BEGIN                 (1)
#define  STL_INIT_INFO             (2)
#define  STL_FACET                 (3)
#define  STL_EXIT_INFO             (4)
#define  STL_END                   (5)

#define  STL_BINARY             (0x01)
/* binary file color extensions (mutually exclusive): */
#define  STL_COLOR_MATERIALISE  (0x02)
#define  STL_COLOR_TNO_VISICAM  (0x04)

#define  STL_NO_COLOR     (0xffffffff)

typedef  float                stl_real;
typedef  struct stl_facet_s   stl_facet;
typedef  struct stl_reader_s  stl_reader;
typedef  struct stl_writer_s  stl_writer;

int           stl_steel_major(void);
int           stl_steel_minor(void);
int           stl_steel_micro(void);
int           stl_steel_abi_version(void);
int           stl_steel_abi_revision(void);
int           stl_steel_abi_age(void);
int           stl_steel_abi_supported(int version, int revision);

stl_facet *   stl_facet_create_uninitialized(void);
stl_facet *   stl_facet_create(void);
stl_facet *   stl_facet_clone(stl_facet * facet);
void          stl_facet_copy(stl_facet * source, stl_facet * target);
void          stl_facet_destroy(stl_facet * facet);
void          stl_facet_set_normal(stl_facet * facet, stl_real x, stl_real y, stl_real z);
void          stl_facet_get_normal(stl_facet * facet, stl_real * x, stl_real * y, stl_real * z);
void          stl_facet_set_vertex1(stl_facet * facet, stl_real x, stl_real y, stl_real z);
void          stl_facet_get_vertex1(stl_facet * facet, stl_real * x, stl_real * y, stl_real * z);
void          stl_facet_set_vertex2(stl_facet * facet, stl_real x, stl_real y, stl_real z);
void          stl_facet_get_vertex2(stl_facet * facet, stl_real * x, stl_real * y, stl_real * z);
void          stl_facet_set_vertex3(stl_facet * facet, stl_real x, stl_real y, stl_real z);
void          stl_facet_get_vertex3(stl_facet * facet, stl_real * x, stl_real * y, stl_real * z);
void          stl_facet_set_padding(stl_facet * facet, unsigned int padding);
unsigned int  stl_facet_get_padding(stl_facet * facet);
void          stl_facet_set_color(stl_facet * facet, unsigned int rgb);
unsigned int  stl_facet_get_color(stl_facet * facet);

stl_reader *  stl_reader_create(const char * filename);
void          stl_reader_destroy(stl_reader * reader);
unsigned int  stl_reader_flags(stl_reader * reader);
int           stl_reader_peek(stl_reader * reader);
const char *  stl_reader_get_info(stl_reader * reader);
stl_facet *   stl_reader_get_facet(stl_reader * reader);
void          stl_reader_fill_facet(stl_reader * reader, stl_facet * facet);
const char *  stl_reader_get_error(stl_reader * reader);
int           stl_reader_get_line_number(stl_reader * reader);

/* not really implemented yet */
stl_writer *  stl_writer_create(const char * filename, unsigned int flags);
int           stl_writer_destroy(stl_writer * writer);
unsigned int  stl_writer_get_flags(stl_writer * writer);
int           stl_writer_set_info(stl_writer * writer, const char * info);
void          stl_writer_set_facet(stl_writer * writer, stl_facet * facet);
stl_facet *   stl_writer_get_facet(stl_writer * writer);
int           stl_writer_put_facet(stl_writer * writer, stl_facet * facet);
const char *  stl_writer_get_error(stl_writer * writer);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* STL_STEEL_H */
