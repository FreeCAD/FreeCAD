#ifndef COIN_XMLDOCUMENT_H
#define COIN_XMLDOCUMENT_H

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

#include <Inventor/C/XML/types.h>

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* basic construction */
COIN_DLL_API cc_xml_doc * cc_xml_doc_new(void);
COIN_DLL_API void cc_xml_doc_delete_x(cc_xml_doc * doc);

/* parser configuration */
COIN_DLL_API void cc_xml_doc_set_filter_cb_x(cc_xml_doc * doc, cc_xml_filter_cb * cb, void * userdata);
COIN_DLL_API void cc_xml_doc_get_filter_cb(const cc_xml_doc * doc, cc_xml_filter_cb *& cb, void *& userdata);

/* document io */
COIN_DLL_API SbBool cc_xml_doc_read_file_x(cc_xml_doc * doc, const char * path);
COIN_DLL_API SbBool cc_xml_doc_read_buffer_x(cc_xml_doc * doc, const char * buffer, size_t buflen);

COIN_DLL_API SbBool cc_xml_doc_parse_buffer_partial_x(cc_xml_doc * doc, const char * buffer, size_t buflen);
COIN_DLL_API SbBool cc_xml_doc_parse_buffer_partial_done_x(cc_xml_doc * doc, const char * buffer, size_t buflen);

COIN_DLL_API SbBool cc_xml_doc_write_to_buffer(const cc_xml_doc * doc, char *& buffer, size_t & bytes);
COIN_DLL_API SbBool cc_xml_doc_write_to_file(const cc_xml_doc * doc, const char * path);

COIN_DLL_API cc_xml_path * cc_xml_doc_diff(const cc_xml_doc * doc, const cc_xml_doc * other);

/* document attributes */
COIN_DLL_API void cc_xml_doc_set_filename_x(cc_xml_doc * doc, const char * path);
COIN_DLL_API const char * cc_xml_doc_get_filename(const cc_xml_doc * doc);

/* misc... */

COIN_DLL_API cc_xml_elt *   cc_xml_doc_get_root(const cc_xml_doc * doc);
COIN_DLL_API void           cc_xml_doc_set_current_x(cc_xml_doc * doc, cc_xml_elt * elt);
COIN_DLL_API cc_xml_elt *   cc_xml_doc_get_current(const cc_xml_doc * doc);
COIN_DLL_API void           cc_xml_doc_strip_whitespace_x(cc_xml_doc * doc);

COIN_DLL_API void           cc_xml_doc_set_root_x(cc_xml_doc * doc, cc_xml_elt * root);

COIN_DLL_API const cc_xml_elt * cc_xml_doc_find_element(const cc_xml_doc * doc, cc_xml_path * path);
COIN_DLL_API const cc_xml_elt * cc_xml_doc_find_next_element(const cc_xml_doc * doc, cc_xml_elt * elt, cc_xml_path * path);
COIN_DLL_API cc_xml_elt *       cc_xml_doc_create_element_x(cc_xml_doc * doc, cc_xml_path * path);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !COIN_XML_DOCUMENT_H */
