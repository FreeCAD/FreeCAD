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

#include <Inventor/C/XML/attribute.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "utils.h"

// TODO:
// - optimize empty strings to use a static, nonfreeable buffer?

// *************************************************************************

struct cc_xml_attr {
  char * name;
  char * value;
};

// *************************************************************************

/*!
  Creates a new cc_xml_attr object, with no name or value information.
*/

cc_xml_attr *
cc_xml_attr_new(void)
{
  cc_xml_attr * attr = new cc_xml_attr;
  assert(attr);
  attr->name = NULL;
  attr->value = NULL;
  return attr;
}

/*!
  Creates a new cc_xml_attr object, with the given name and value information.
  The \a value argument can be NULL.
*/

cc_xml_attr *
cc_xml_attr_new_from_data(const char * name, const char * value)
{
  cc_xml_attr * attr = cc_xml_attr_new();
  attr->name = cc_xml_strdup(name);
  if (value) { attr->value = cc_xml_strdup(value); }
  return attr;
}

/*!
  Returns a clone of the given attribute.
*/

cc_xml_attr *
cc_xml_attr_clone(const cc_xml_attr * attr)
{
  assert(attr);
  return cc_xml_attr_new_from_data(attr->name, attr->value);
}

/*!
  Deletes the cc_xml_Attr object, including its internals.
*/

void
cc_xml_attr_delete_x(cc_xml_attr * attr)
{
  assert(attr);
  delete [] attr->name;
  delete [] attr->value;
  delete attr;
}

// *************************************************************************

/*!
  Sets the name part of the attribute.  Old name information will be freed.
  The \a name argument can be NULL to clear the setting.
*/

void
cc_xml_attr_set_name_x(cc_xml_attr * attr, const char * name)
{
  delete [] attr->name;
  attr->name = NULL;
  if (name) attr->name = cc_xml_strdup(name);
}

/*!
  Returns the name part of the attribute if one is set, and NULL otherwise.
*/

const char *
cc_xml_attr_get_name(const cc_xml_attr * attr)
{
  return attr->name;
}

/*!
  Sets the value part of the attribute.  Old information will be freed.
  The \a value argument can be NULL to clear the information.
*/

void
cc_xml_attr_set_value_x(cc_xml_attr * attr, const char * value)
{
  delete [] attr->value;
  attr->value = NULL;
  if (value) attr->value = cc_xml_strdup(value);
}

/*!
  Returns the value part of the attribute if one is set, or NULL otherwise.
*/

const char *
cc_xml_attr_get_value(const cc_xml_attr * attr)
{
  return attr->value;
}

// *************************************************************************

// TODO: type-specific accessor methods...

// *************************************************************************

size_t
cc_xml_attr_calculate_size(const cc_xml_attr * attr)
{
  size_t bytes = 0;
  bytes += strlen(attr->name);
  bytes += 2; // ="
  // FIXME: count quotables in string value, and add up quote count
  if (attr->value) bytes += strlen(attr->value);
  bytes += 1; // "
  return bytes;
}

size_t
cc_xml_attr_write_to_buffer(const cc_xml_attr * attr, char * buffer, size_t bufsize)
{
  // We assert on mismatches between calculated memory usage and actual memory
  // usage, since this must be calculated correctly for not getting memory corruption
  // on invalid buffer size allocation before writing.
  const char * const origbufferptr = buffer;
  const size_t assumed = cc_xml_attr_calculate_size(attr);
  assert(assumed < bufsize);
  size_t namelen = strlen(attr->name);
  strcpy(buffer, attr->name);
  buffer += namelen;
  strcpy(buffer, "=\"");
  buffer += 2;
  if (attr->value) {
    size_t valuelen = strlen(attr->value);
    // FIXME: count quotables, and insert quoting in value string if needed
    strcpy(buffer, attr->value);
    buffer += valuelen;
  }
  buffer[0] = '"';
  buffer += 1;
  size_t used = buffer - origbufferptr;
  assert(used == assumed);
  return used;
}

// *************************************************************************
