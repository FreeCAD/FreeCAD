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

/*! \file types.h */
#include <Inventor/C/XML/types.h>

#include <Inventor/C/XML/document.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/C/XML/attribute.h>
#include <Inventor/C/XML/entity.h>
#include <Inventor/C/XML/path.h>

/*!
  \var cc_xml_filter_cb

  Called twice for each element while parsing input, if set.  The push
  argument tells you if the element is being pushed (or opened) or popped
  (closed).  The return value is only considered during the pop callback.
*/

/* ********************************************************************** */

BuxDocument *
cc_xml_wrap(cc_xml_doc * doc, int pass_ownership)
{
  return new BuxDocument(doc);
}

cc_xml_doc *
cc_xml_unwrap(BuxDocument * doc, int pass_ownership)
{
  return doc->getHandle();
}

BuxElement *
cc_xml_wrap(cc_xml_elt * elt, int pass_ownership)
{
  return BuxElement(elt);
}

cc_xml_elt *
cc_xml_unwrap(BuxElement * elt, int pass_ownership)
{
  return elt->getHandle();
}

BuxAttribute *
cc_xml_wrap(cc_xml_attr * attr, int pass_ownership)
{
  return BuxAttribute(attr);
}

cc_xml_attr *
cc_xml_unwrap(BuxAttribute * attr, int pass_ownership)
{
  return attr->getHandle();
}

BuxEntity *
cc_xml_wrap(cc_xml_ent * ent, int pass_ownership)
{
  return BuxEntity(ent);
}

cc_xml_ent *
cc_xml_unwrap(BuxEntity * ent, int pass_ownership)
{
  return ent->getHandle();
}

BuxPath *
cc_xml_wrap(cc_xml_path * path, int pass_ownership)
{
  return BuxPath(path);
}

cc_xml_path *
cc_xml_unwrap(BuxPath * path, int pass_ownership)
{
  return path->getHandle();
}

/* ********************************************************************** */
