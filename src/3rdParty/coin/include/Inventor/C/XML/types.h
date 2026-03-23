#ifndef COIN_XML_TYPES_H
#define COIN_XML_TYPES_H

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

/* ********************************************************************** */

#define COIN_XML_CDATA_TYPE "cdata"
#define COIN_XML_ROOT_TYPE  "root"

typedef  struct cc_xml_doc       cc_xml_doc;
typedef  struct cc_xml_elt       cc_xml_elt;
typedef  struct cc_xml_attr      cc_xml_attr;
typedef  struct cc_xml_ent       cc_xml_ent;
typedef  struct cc_xml_path      cc_xml_path;

/* non-abbreviated aliases */
typedef  cc_xml_doc              cc_xml_document;
typedef  cc_xml_elt              cc_xml_element;
typedef  cc_xml_attr             cc_xml_attribute;
typedef  cc_xml_ent              cc_xml_entity;

/* streaming parsing */
enum cc_xml_filter_choice {
  KEEP,
  DISCARD
};

typedef cc_xml_filter_choice cc_xml_filter_cb(void * userdata, cc_xml_doc * doc, cc_xml_elt * elt, int pushing);

/* ********************************************************************** */

#ifdef __cplusplus

#if 0
class SbXmlDocument;
class SbXmlElement;
class SbXmlAttribute;
class SbXmlEntity;
class SbXmlPath;

SbXmlDocument *  COIN_DLL_API cc_xml_wrap(cc_xml_doc * doc, int pass_ownership = FALSE);
SbXmlElement *   COIN_DLL_API cc_xml_wrap(cc_xml_elt * elt, int pass_ownership = FALSE);
SbXmlAttribute * COIN_DLL_API cc_xml_wrap(cc_xml_attr * attr, int pass_ownership = FALSE);
SbXmlEntity *    COIN_DLL_API cc_xml_wrap(cc_xml_ent * ent, int pass_ownership = FALSE);
SbXmlPath *      COIN_DLL_API cc_xml_wrap(cc_xml_path * path, int pass_ownership = FALSE);

cc_xml_doc *     COIN_DLL_API cc_xml_unwrap(SbXmlDocument * doc, int pass_ownership = FALSE);
cc_xml_elt *     COIN_DLL_API cc_xml_unwrap(SbXmlElement * elt, int pass_ownership = FALSE);
cc_xml_attr *    COIN_DLL_API cc_xml_unwrap(SbXmlAttribute * attr, int pass_ownership = FALSE);
cc_xml_ent *     COIN_DLL_API cc_xml_unwrap(SbXmlEntity * ent, int pass_ownership = FALSE);
cc_xml_path *    COIN_DLL_API cc_xml_unwrap(SbXmlPath * path, int pass_ownership = FALSE);
#endif /* 0 */

#endif /* __cplusplus */

/* ********************************************************************** */

#endif /* !COIN_XML_TYPES_H */
