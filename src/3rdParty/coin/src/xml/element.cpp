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

#include <Inventor/C/XML/element.h>
#include "elementp.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <Inventor/C/base/string.h>
#include <Inventor/lists/SbList.h>

#include <Inventor/C/XML/document.h>
#include <Inventor/C/XML/attribute.h>
#include <Inventor/C/XML/path.h>
#include "attributep.h"
#include "utils.h"

// *************************************************************************

struct cc_xml_elt {
  char * type;
  char * data;
  char * cdata;
  cc_xml_elt * parent;
  SbList<cc_xml_attr *> attributes;
  SbList<cc_xml_elt *> children;
};

// *************************************************************************

/*!
  Creates a new element with no type, no attributes, and no child elements.
*/

cc_xml_elt *
cc_xml_elt_new(void)
{
  cc_xml_elt * elt = new cc_xml_elt;
  assert(elt);
  elt->type = NULL;
  elt->data = NULL;
  elt->cdata = NULL;
  elt->parent = NULL;
  return elt;
}

/*!
  Creates a new element with the given type and attributes.
  The \a attrs argument can be NULL.
*/

cc_xml_elt *
cc_xml_elt_new_from_data(const char * type, cc_xml_attr ** attrs)
{
  cc_xml_elt * elt = cc_xml_elt_new();
  cc_xml_elt_set_type_x(elt, type);
  if (attrs) { cc_xml_elt_set_attributes_x(elt, attrs); }
  return elt;
}

/*!
  Returns a clone of the element, including child elements and attributes.
  Only the parent connection is dropped.
*/

cc_xml_elt *
cc_xml_elt_clone(const cc_xml_elt * elt)
{
  cc_xml_elt * clone = cc_xml_elt_new();
  if (elt->type) {
    cc_xml_elt_set_type_x(clone, elt->type);
  }
  if (elt->data) {
    clone->data = cc_xml_strdup(elt->data);
  }
  if (elt->cdata) {
    clone->cdata = cc_xml_strdup(elt->cdata);
  }
  int i = 0;
  for (i = 0; i < elt->attributes.getLength(); ++i) {
    cc_xml_elt_set_attribute_x(clone, cc_xml_attr_clone(elt->attributes[i]));
  }
  for (i = 0; i < elt->children.getLength(); ++i) {
    cc_xml_elt_add_child_x(clone, cc_xml_elt_clone(elt->children[i]));
  }
  return clone;
}

/*!
  Frees the given \a elt element, including its attributes and children.
*/

void
cc_xml_elt_delete_x(cc_xml_elt * elt)
{
  assert(elt);
  delete [] elt->type;
  delete [] elt->data;
  delete [] elt->cdata;
  if (elt->attributes.getLength() > 0) {
    const int num = elt->attributes.getLength();
    for (int i = 0; i < num; ++i) {
      cc_xml_attr_delete_x(elt->attributes[i]);
    }
  }
  if (elt->children.getLength() > 0) {
    const int num = elt->children.getLength();
    for (int i = 0; i < num; ++i) {
      cc_xml_elt_delete_x(elt->children[i]);
    }
  }
  delete elt;
}

// *************************************************************************

/*!
  Sets the element type identifier.  The old value will be freed, if any.
  The \a type argument can be NULL to just clear the old value.
*/

void
cc_xml_elt_set_type_x(cc_xml_elt * elt, const char * type)
{
  assert(elt);
  delete [] elt->type;
  elt->type = NULL;
  if (type) elt->type = cc_xml_strdup(type);
}

/*!
  Returns the element type identifier if one is set, and NULL otherwise.
*/

const char *
cc_xml_elt_get_type(const cc_xml_elt * elt)
{
  return elt->type;
}

// FIXME: document behaviour

void
cc_xml_elt_set_cdata_x(cc_xml_elt * elt, const char * cdata)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( cc_xml_elt_get_num_children(elt) == 0 ) {
      // cdata child not existing yet
      cc_xml_elt * child = cc_xml_elt_new();
      cc_xml_elt_set_type_x(child, COIN_XML_CDATA_TYPE);
      cc_xml_elt_add_child_x(elt, child);
      elt = child;
    } else if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      // we really want to manipulate the cdata child, not this element
      elt = elt->children[0];
    } else {
      // FIXME: warn that this doesn't make sense  20021119 larsa
      // but we can run through anyways
    }
  }
  delete [] elt->cdata;
  elt->cdata = NULL;
  if ( cdata ) elt->cdata = cc_xml_strdup(cdata);
  delete [] elt->data;
  elt->data = NULL;
  // Update data to whitespace-stripped cdata
  if( cdata) {
    const char * startptr = elt->cdata;
    const char * endptr = startptr + (strlen(startptr) - 1);
    while ( *startptr == '\t' || *startptr == '\r' || *startptr == '\n' || *startptr == ' ' ) {
      startptr++;
    }
    // FIXME: avoid whitespace-only cdata from parser, then uncomment assert's.
    // assert(startptr <= endptr && "just whitespace from the front");
    while ( endptr > startptr && (*endptr == '\t' || *endptr == '\r' || *endptr == '\n' || *endptr == ' ') ) {
      endptr--;
    }
    //  assert(endptr > startptr && "makes no sense at all");
    endptr++;
    if( endptr > startptr) {
      elt->data = cc_xml_strndup(startptr, endptr - startptr);
    }
  }
}

const char *
cc_xml_elt_get_cdata(const cc_xml_elt * elt)
{
  assert(elt);
  if (strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0) {
    if ((cc_xml_elt_get_num_children(elt) == 1) &&
        (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0)) {
      return elt->children[0]->cdata;
    }
  }
  return elt->cdata;
}

const char *
cc_xml_elt_get_data(const cc_xml_elt * elt)
{
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      return elt->children[0]->data;
    }
  }
  return elt->data;
}

// *************************************************************************

void
cc_xml_elt_remove_all_attributes_x(cc_xml_elt * elt)
{
  assert(elt);
  if (elt->attributes.getLength()) {
    // FIXME: implement proper action
  }
}

void
cc_xml_elt_set_attribute_x(cc_xml_elt * elt, cc_xml_attr * attr)
{
  cc_xml_attr * shadowed = cc_xml_elt_get_attribute(elt, cc_xml_attr_get_name(attr));
  if (shadowed) {
    cc_xml_attr_set_value_x(shadowed, cc_xml_attr_get_value(attr));
    // FIXME: free attr, or replace shadowed by attr and free shadowed?
  } else {
    elt->attributes.append(attr);
  }
}

void
cc_xml_elt_set_attributes_x(cc_xml_elt * elt, cc_xml_attr ** attrs)
{
  for (int c = 0; attrs[c] != NULL; ++c) {
    cc_xml_elt_set_attribute_x(elt, attrs[c]);
  }
}

cc_xml_attr *
cc_xml_elt_get_attribute(const cc_xml_elt * elt, const char * attrname)
{
  assert(elt);
  const int num = elt->attributes.getLength();
  for (int c = 0; c < num; ++c) {
    if (strcmp(attrname, cc_xml_attr_get_name(elt->attributes[c])) == 0)
      return elt->attributes[c];
  }
  return NULL;
}

int
cc_xml_elt_get_num_attributes(const cc_xml_elt * elt)
{
  return elt->attributes.getLength();
}

const cc_xml_attr **
cc_xml_elt_get_attributes(const cc_xml_elt * elt)
{
  assert(elt);
  return const_cast<const cc_xml_attr **>(elt->attributes.getArrayPtr());
}

// *************************************************************************

cc_xml_elt *
cc_xml_elt_get_parent(const cc_xml_elt * elt)
{
  assert(elt);
  return elt->parent;
}

int
cc_xml_elt_get_num_children(const cc_xml_elt * elt)
{
  assert(elt);
  return elt->children.getLength();
}

/*!
  Returns the number of child elements of the given type.
*/

int
cc_xml_elt_get_num_children_of_type(const cc_xml_elt * elt, const char * type)
{
  assert(elt);
  int count = 0;
  const int numchildren = elt->children.getLength();
  for (int i = 0; i < numchildren; ++i) {
    if (strcmp(type, cc_xml_elt_get_type(elt->children[i])) == 0) ++count;
  }
  return count;
}

/*!
  Returns the child element at index \a childidx.

  The index has to be an existing index, invalid values are considered programming errors.
*/

cc_xml_elt *
cc_xml_elt_get_child(const cc_xml_elt * elt, int childidx)
{
  assert(elt);
  assert(childidx >= 0);
  assert(childidx < cc_xml_elt_get_num_children(elt));
  return elt->children[childidx];
}

/*!
  Returns the child index of the child element.

  Giving a child element that is not a child is considered a programming error.
*/

int
cc_xml_elt_get_child_index(const cc_xml_elt * elt, const cc_xml_elt * child)
{
  assert(elt);
  const int numchildren = elt->children.getLength();
  for (int idx = 0; idx < numchildren; ++idx) {
    if (elt->children[idx] == child) return idx;
  }
  assert(!"no such child");
  return -1;
}

/*
  Returns the child element's index when only counting elements of that given type.

  Giving a child element that is not a child is considered a programming error.
*/

int
cc_xml_elt_get_child_type_index(const cc_xml_elt * elt, const cc_xml_elt * child)
{
  assert(elt);
  const int numchildren = elt->children.getLength();
  int idx = -1;
  const char * type = cc_xml_elt_get_type(child);
  for (int i = 0; i < numchildren; ++i) {
    if (strcmp(cc_xml_elt_get_type(elt->children[i]), type) == 0) ++idx;
    if (elt->children[i] == child) return idx;
  }
  assert(!"no such child");
  return -1;
}

cc_xml_path *
cc_xml_elt_get_path(const cc_xml_elt * elt)
{
  assert(elt);
  cc_xml_path * path = cc_xml_path_new();
  while ( elt != NULL ) {
    const cc_xml_elt * parent = cc_xml_elt_get_parent(elt);
    if ( parent != NULL ) {
      int idx = cc_xml_elt_get_child_type_index(parent, elt);
      cc_xml_path_prepend_x(path, elt->type, idx);
    }
    elt = parent;
  }
  return path;
}

cc_xml_elt *
cc_xml_elt_get_child_of_type(const cc_xml_elt * elt, const char * type, int idx)
{
  assert(elt);
  if ( !elt->children.getLength() ) return NULL;
  int i;
  for ( i = 0; i < elt->children.getLength(); i++ ) {
    if ( strcmp(elt->children[i]->type, type) == 0 ) {
      if ( idx == 0 ) return elt->children[i];
      idx -= 1;
    }
  }
  return NULL;
}

cc_xml_elt *
cc_xml_elt_get_child_of_type_x(cc_xml_elt * elt, const char * type, int idx)
{
  assert(elt);
  if ( elt->children.getLength() ) {
    int i;
    for ( i = 0; i < elt->children.getLength(); i++ ) {
      if ( strcmp(elt->children[i]->type, type) == 0 ) {
        if ( idx == 0 ) return elt->children[i];
        idx -= 1;
      }
    }
  }
  cc_xml_elt * child = NULL;
  while ( idx >= 0 ) {
    child = cc_xml_elt_new();
    cc_xml_elt_set_type_x(child, type);
    cc_xml_elt_add_child_x(elt, child);
    idx -= 1;
  }
  return child;
}

// *************************************************************************

void
cc_xml_elt_set_parent_x(cc_xml_elt * elt, cc_xml_elt * parent)
{
  assert(elt);
  elt->parent = parent;
}

void
cc_xml_elt_add_child_x(cc_xml_elt * elt, cc_xml_elt * child)
{
  assert(elt);
  assert(child);
  if (child->parent != NULL) {
    // FIXME: ERROR - element already a child of another element
    return;
  }

  //int numchildren = cc_xml_elt_get_num_children(elt);
  elt->children.append(child);
  child->parent = elt;
}

/*!  This function will not free the child being removed.  Giving a
  nonexistent child to this function is a programming error and will
  result in an assert.
*/

void
cc_xml_elt_remove_child_x(cc_xml_elt * elt, cc_xml_elt * child)
{
  assert(elt);
  assert(child);
  const int numchildren = elt->children.getLength();
  for (int i = 0; i < numchildren; ++i) {
    if (elt->children[i] == child) {
      elt->children.remove(i);
      child->parent = NULL;
      return;
    }
  }
  assert(!"no such child");
}

void
cc_xml_elt_insert_child_x(cc_xml_elt * elt, cc_xml_elt * child, int idx)
{
  assert(elt);
  assert(child);
  if (child->parent != NULL) {
    // FIXME: error, child already a child of another element
    return;
  }
  const int numchildren = elt->children.getLength();
  assert(idx >= 0 && idx <= numchildren);
  elt->children.insert(child, idx);
  child->parent = elt;
}

int
cc_xml_elt_replace_child_x(cc_xml_elt * elt, cc_xml_elt * oldchild, cc_xml_elt * newchild)
{
  assert(elt);
  int idx = cc_xml_elt_get_child_index(elt, oldchild);
  if ( idx == -1 ) return FALSE;
  cc_xml_elt_remove_child_x(elt, oldchild);
  cc_xml_elt_insert_child_x(elt, newchild, idx);
  return TRUE;
}

// *************************************************************************

int
cc_xml_elt_get_boolean(const cc_xml_elt * elt, int * value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  const char * data = cc_xml_elt_get_data(elt);
  assert(value != NULL);
  if ( data == NULL ) return FALSE;
  if ( cc_xml_strieq(data, "true") ||
       cc_xml_strieq(data, "on") ||
       cc_xml_strieq(data, "t") ) {
    *value = TRUE;
    return TRUE;
  }
  if ( cc_xml_strieq(data, "false") ||
       cc_xml_strieq(data, "off") ||
       cc_xml_strieq(data, "f") ) {
    *value = FALSE;
    return TRUE;
  }
  return FALSE;
}

int
cc_xml_elt_get_integer(const cc_xml_elt * elt, int * value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  const char * data = cc_xml_elt_get_data(elt);
  assert(value != NULL);
  if ( data == NULL ) return FALSE;
  if ( sscanf(data, "%d", value) == 1 ) return TRUE;
  return FALSE;
}

int
cc_xml_elt_get_uint64(const cc_xml_elt * elt, uint64_t * value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  const char * data = cc_xml_elt_get_data(elt);
  assert(value != NULL);
  if ( data == NULL ) return FALSE;
  if ( sscanf(data, "%lld", value) == 1 ) return TRUE; // FIXME: unsigned
  return FALSE;
}

int
cc_xml_elt_get_int64(const cc_xml_elt * elt, int64_t * value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  const char * data = cc_xml_elt_get_data(elt);
  assert(value != NULL);
  if ( data == NULL ) return FALSE;
  if ( sscanf(data, "%lld", value) == 1 ) return TRUE;
  return FALSE;
}

int
cc_xml_elt_get_uint32(const cc_xml_elt * elt, uint32_t * value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  const char * data = cc_xml_elt_get_data(elt);
  assert(value != NULL);
  if ( data == NULL ) return FALSE;
  if ( sscanf(data, "%u", value) == 1 ) return TRUE; // FIXME: unsigned
  return FALSE;
}

int
cc_xml_elt_get_int32(const cc_xml_elt * elt, int32_t * value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  const char * data = cc_xml_elt_get_data(elt);
  assert(value != NULL);
  if ( data == NULL ) return FALSE;
  if ( sscanf(data, "%u", value) == 1 ) return TRUE;
  return FALSE;
}

int
cc_xml_elt_get_float(const cc_xml_elt * elt, float * value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  const char * data = cc_xml_elt_get_data(elt);
  assert(value != NULL);
  if ( data == NULL ) return FALSE;
  if ( sscanf(data, "%g", value) == 1 ) return TRUE;
  return FALSE;
}

int
cc_xml_elt_get_double(const cc_xml_elt * elt, double * value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  const char * data = cc_xml_elt_get_data(elt);
  assert(value != NULL);
  if ( data == NULL ) return FALSE;
  if ( sscanf(data, "%lg", value) == 1 ) return TRUE;
  return FALSE;
}

void
cc_xml_elt_set_boolean_x(cc_xml_elt * elt, int value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  cc_xml_elt_set_cdata_x(elt, value ? "true" : "false");
}

void
cc_xml_elt_set_integer_x(cc_xml_elt * elt, int value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  cc_string str;
  cc_string_construct(&str);
  cc_string_sprintf(&str, "%d", value);
  cc_xml_elt_set_cdata_x(elt, cc_string_get_text(&str));
  cc_string_clean(&str);
}

void
cc_xml_elt_set_uint64_x(cc_xml_elt * elt, uint64_t value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  cc_string str;
  cc_string_construct(&str);
  cc_string_sprintf(&str, "%lld", value); // FIXME: unsigned
  cc_xml_elt_set_cdata_x(elt, cc_string_get_text(&str));
  cc_string_clean(&str);
}

void
cc_xml_elt_set_int64_x(cc_xml_elt * elt, int64_t value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  cc_string str;
  cc_string_construct(&str);
  cc_string_sprintf(&str, "%lld", value);
  cc_xml_elt_set_cdata_x(elt, cc_string_get_text(&str));
  cc_string_clean(&str);
}

void
cc_xml_elt_set_uint32_x(cc_xml_elt * elt, uint32_t value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  cc_string str;
  cc_string_construct(&str);
  cc_string_sprintf(&str, "%ld", value); // FIXME: unsigned
  cc_xml_elt_set_cdata_x(elt, cc_string_get_text(&str));
  cc_string_clean(&str);
}

void
cc_xml_elt_set_int32_x(cc_xml_elt * elt, int32_t value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  cc_string str;
  cc_string_construct(&str);
  cc_string_sprintf(&str, "%ld", value);
  cc_xml_elt_set_cdata_x(elt, cc_string_get_text(&str));
  cc_string_clean(&str);
}

void
cc_xml_elt_set_float_x(cc_xml_elt * elt, float value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  cc_string str;
  cc_string_construct(&str);
  cc_string_sprintf(&str, "%g", value);
  cc_xml_elt_set_cdata_x(elt, cc_string_get_text(&str));
  cc_string_clean(&str);
}

void
cc_xml_elt_set_double_x(cc_xml_elt * elt, double value)
{
  assert(elt);
  if ( strcmp(elt->type, COIN_XML_CDATA_TYPE) != 0 ) {
    if ( (cc_xml_elt_get_num_children(elt) == 1) &&
         (strcmp(elt->children[0]->type, COIN_XML_CDATA_TYPE) == 0) ) {
      elt = elt->children[0];
    } else {
      // FIXME: warn about structure senselessness
    }
  }
  cc_string str;
  cc_string_construct(&str);
  cc_string_sprintf(&str, "%lf", value);
  cc_xml_elt_set_cdata_x(elt, cc_string_get_text(&str));
  cc_string_clean(&str);
}

// *************************************************************************

void
cc_xml_elt_strip_whitespace_x(cc_xml_elt * elt)
{
  // FIXME: update to reflect cdata change - remove function probably
  assert(elt->type && strcmp(elt->type, COIN_XML_CDATA_TYPE) == 0);
  assert(0);
  const char * startptr = elt->data;
  const char * endptr = startptr + (strlen(startptr) - 1);
  while ( *startptr ) {
    switch ( *startptr ) {
    case '\t':
    case '\r':
    case '\n':
    case ' ':
      break;
    default:
      goto cont1;
    }
    startptr++;
  }
  assert(0 && "just whitespace from the front");
cont1:
  while ( endptr > startptr ) {
    switch ( *endptr ) {
    case '\t':
    case '\r':
    case '\n':
    case ' ':
      break;
    default:
      goto cont2;
    }
    endptr--;
  }
  assert(0 && "makes no sense at all");
cont2:
  endptr++;
  if ( (startptr == elt->data) &&
       (endptr == (startptr + strlen(startptr))) ) return;
  char * substr = cc_xml_strndup(startptr, endptr - startptr);
  cc_xml_elt_set_cdata_x(elt, substr);
  delete [] substr;
}

// *************************************************************************

void
cc_xml_elt_dump_to_file(const cc_xml_elt * elt, int indent, FILE * fp)
{
  // todo
  // - support notation for empty elements and shortcuts
  // - document-wide settings?  how to propagate...
  assert( elt && elt->type != NULL );
  int i;
  if ( cc_xml_elt_get_num_children(elt) == 1 &&
       strcmp(COIN_XML_CDATA_TYPE, elt->children[0]->type) == 0 ) {
    for ( i = 0; i < indent; i++ ) fprintf(fp, " ");
    if ( elt->children[0]->data != NULL )
      fprintf(fp, "<%s>%s</%s>\n", elt->type, elt->children[0]->data, elt->type);
    else
      fprintf(fp, "<%s/>\n", elt->type);
  } else if ( strcmp(COIN_XML_CDATA_TYPE, elt->type) != 0 ) {
    for ( i = 0; i < indent; i++ ) fprintf(fp, " ");
    fprintf(fp, "<%s>\n", elt->type);
    const int children = cc_xml_elt_get_num_children(elt);
    for ( i = 0; i < children; i++ ) {
      cc_xml_elt * child = cc_xml_elt_get_child(elt, i);
      cc_xml_elt_dump_to_file(child, indent + 2, fp);
    }
    for ( i = 0; i < indent; i++ ) fprintf(fp, " ");
    fprintf(fp, "</%s>\n", elt->type);
  } else if ( elt->data != NULL ) {
    for ( i = 0; i < indent; i++ ) fprintf(fp, " ");
    fprintf(fp, "%s\n", elt->data);
  }
}

// *************************************************************************

/*!
  \fn cc_xml_elt * cc_xml_elt_get_traversal_next(cc_xml_elt * root, cc_xml_elt * here)

  Utility function for flattening recursive traversals to do-while loops.

  \ingroup coin_cc_xml_elt
*/

cc_xml_elt *
cc_xml_elt_get_traversal_next(const cc_xml_elt * root, cc_xml_elt * here)
{
  assert(root && here);

  // first traverse into children - but ignore any "cdata" element...
  if (cc_xml_elt_get_num_children(here) > 0) {
    // Check if we can find a child that is not cdata..
    for (int i = 0; i < cc_xml_elt_get_num_children(here); ++i) {
      if (strcmp(COIN_XML_CDATA_TYPE, here->children[i]->type) != 0) {
        return cc_xml_elt_get_child(here, i);
      }
    }
  }

  // if we're here then 'here' has no children (except possibly cdata).
  do {
    cc_xml_elt * parent = cc_xml_elt_get_parent(here);
    if (parent == NULL) return NULL; // here is the root
    int idx = cc_xml_elt_get_child_index(parent, here);
    // if 'here' was the last child then set here as parent..
    if (idx == (cc_xml_elt_get_num_children(parent) - 1)) {
      here = parent;
      // if we're back to root, simply quit..
      if (here == root) return NULL;
    } else {
      // there is more children than 'here' left, find someone that's not "cdata".
      do {
          ++idx;
          here = cc_xml_elt_get_child(parent, idx);

          // return this element if it is not a "cdata" element.
          if (strcmp(COIN_XML_CDATA_TYPE, here->type) != 0) return here;

      } while (idx != (cc_xml_elt_get_num_children(parent) - 1));
    }
  } while (TRUE);

  return NULL;
} // cc_xml_elt_get_traversal_next()

// *************************************************************************

const cc_xml_elt *
cc_xml_elt_find(const cc_xml_elt * root, const cc_xml_path * path)
{
  assert(root && path);
  cc_xml_elt * elt = const_cast<cc_xml_elt *>(root);
  while ( (elt != NULL) && (!cc_xml_path_match_p(path, elt)) ) {
    elt = cc_xml_elt_get_traversal_next(root, elt);
  }
  return elt;
} // cc_xml_elt_find()

const cc_xml_elt *
cc_xml_elt_find_next(const cc_xml_elt * root, cc_xml_elt * from, cc_xml_path * path)
{
  assert(root && from && path);
  cc_xml_elt * elt = from;
  do {
    elt = cc_xml_elt_get_traversal_next(root, elt);
  } while ( (elt != NULL) && (!cc_xml_path_match_p(path, elt)) );
  return elt;
} // cc_xml_elt_find_next()

// *************************************************************************

cc_xml_elt *
cc_xml_elt_create_x(cc_xml_elt * from, cc_xml_path * path)
{
  assert(from && path);
  cc_xml_elt * current = from;
  const char * type;
  int idx;
  int length = cc_xml_path_get_length(path);
  int i;
  for ( i = 0; i < length; i++ ) {
    int lastpos = -1, lastidx = -1;
    type = cc_xml_path_get_type(path, i);
    idx = cc_xml_path_get_index(path, i);
    if ( current->children.getLength() ) {
      int child;
      for ( child = 0; child < current->children.getLength(); child++ ) {
        if ( strcmp(type, cc_xml_elt_get_type(current->children[child])) == 0 ) {
          lastpos = child;
          lastidx += 1;
          if ( idx == -1 || idx == lastidx ) {
            // match - enter this child
            goto cont;
          }
        }
      }
      // no match - need to create for idx idx
      if ( idx == -1 ) idx = lastidx + 1;
      while ( lastidx < idx ) {
        // insert empty children after lastpos - enter last one
        lastidx += 1;
        lastpos += 1;
        cc_xml_elt * elt = cc_xml_elt_new();
        cc_xml_elt_set_type_x(elt, type);
        if ( lastpos == 0 ) lastpos = cc_xml_elt_get_num_children(current);
        cc_xml_elt_insert_child_x(current, elt, lastpos);
      }
    } else {
      // no children - need to create them
      lastpos = -1;
      lastidx = -1;
      if ( idx == -1 ) idx = 0;
      while ( lastidx < idx ) {
        cc_xml_elt * newelt = cc_xml_elt_new();
        cc_xml_elt_set_type_x(newelt, type);
        cc_xml_elt_add_child_x(current, newelt);
        lastidx += 1;
        lastpos += 1;
      }
    }
cont:
    current = current->children[lastpos];
  }
  return current;
} // cc_xml_elt_create_x()

// *************************************************************************

size_t
cc_xml_elt_calculate_size(const cc_xml_elt * elt, int indent, int indentincrement)
{
  assert(elt);
  assert(elt->type);
  assert(indent >= 0);

  size_t bytes = 0;

// macro to advance a number of bytes
#define ADVANCE_NUM_BYTES(num) \
  do { bytes += num; } while (0)

// macro to advance a number of blanks (indentation)
#define ADVANCE_NUM_SPACES(num) \
  do { bytes += num; } while (0)

// macro to increment bytecount for string literal
#define ADVANCE_STRING_LITERAL(str) \
  do { bytes += (sizeof(str) - 1); } while (0)

// macro to increment bytecount for runtime string
#define ADVANCE_STRING(str) \
  do { bytes += strlen(str); } while (0)

  // duplicate block - see cc_xml_elt_write_to_buffer()
  if (elt->type && strcmp(elt->type, COIN_XML_CDATA_TYPE) == 0) {
    // this is a leaf element character data container
    ADVANCE_STRING(elt->cdata);
  } else {
    ADVANCE_NUM_SPACES(indent);
    ADVANCE_STRING_LITERAL("<");
    ADVANCE_STRING(elt->type);

    int c;
    const int numattributes = elt->attributes.getLength();
    for (c = 0; c < numattributes; ++c) {
      ADVANCE_STRING_LITERAL(" ");
      ADVANCE_NUM_BYTES(cc_xml_attr_calculate_size(elt->attributes[c]));
    }

    const int numchildren = elt->children.getLength();
    if (numchildren == 0) { // close element directly
      ADVANCE_STRING_LITERAL("/>\n");
    } else if ((numchildren == 1) &&
               (strcmp(cc_xml_elt_get_type(elt->children[0]), COIN_XML_CDATA_TYPE) == 0)) {
      ADVANCE_STRING_LITERAL(">");
      ADVANCE_STRING(cc_xml_elt_get_cdata(elt->children[0]));
      ADVANCE_STRING_LITERAL("</");
      ADVANCE_STRING(elt->type);
      ADVANCE_STRING_LITERAL(">\n");
    } else {
      ADVANCE_STRING_LITERAL(">\n");
      for (c = 0; c < numchildren; ++c) {
        ADVANCE_NUM_BYTES(cc_xml_elt_calculate_size(elt->children[c], indent + indentincrement, indentincrement));
      }
      ADVANCE_NUM_SPACES(indent);
      ADVANCE_STRING_LITERAL("</");
      ADVANCE_STRING(elt->type);
      ADVANCE_STRING_LITERAL(">\n");
    }
  }

#undef ADVANCE_NUM_BYTES
#undef ADVANCE_NUM_SPACES
#undef ADVANCE_STRING
#undef ADVANCE_STRING_LITERAL

  return bytes;
}

size_t
cc_xml_elt_write_to_buffer(const cc_xml_elt * elt, char * buffer, size_t bufsize, int indent, int indentincrement)
{
  assert(elt);
  assert(elt->type);
  assert(indent >= 0);

  const size_t assumed = cc_xml_elt_calculate_size(elt, indent, indentincrement);

  size_t bytes = 0;
  char * hereptr = buffer;
  size_t bytesleft = bufsize;

// macro to advance buffer pointer and decrement bytesleft count
#define ADVANCE_NUM_BYTES(len)          \
  do { const size_t length = (len);        \
       bytes += length;                 \
       hereptr += length;               \
       bytesleft -= length; } while (0)

// macro to copy in a string literal and advance pointers
#define ADVANCE_STRING_LITERAL(str)                \
  do { const size_t strlength = (sizeof(str) - 1); \
       strcpy(hereptr, str);        \
       ADVANCE_NUM_BYTES(strlength); } while (0)

// macro to copy in a runtime string and advance pointers
#define ADVANCE_STRING(str)                                 \
  do { const size_t strlength = strlen(str); \
       strcpy(hereptr, str);                    \
       ADVANCE_NUM_BYTES(strlength); } while (0)

// macro to advance a number of blanks (indentation)
#define ADVANCE_NUM_SPACES(num)                  \
  do { const size_t strlength = (num);              \
       memset(hereptr, ' ', strlength);          \
       ADVANCE_NUM_BYTES(strlength); } while (0)

  // ***********************************************************************
  // almost duplicate block - see cc_xml_elt_calculate_size()
  if (elt->type && strcmp(elt->type, COIN_XML_CDATA_TYPE) == 0) {
    // this is a leaf element character data container
    ADVANCE_STRING(elt->cdata);
  } else {
    ADVANCE_NUM_SPACES(indent);
    ADVANCE_STRING_LITERAL("<");
    ADVANCE_STRING(elt->type);

    int c;
    const int numattributes = elt->attributes.getLength();
    for (c = 0; c < numattributes; ++c) {
      ADVANCE_STRING_LITERAL(" ");
      ADVANCE_NUM_BYTES(cc_xml_attr_write_to_buffer(elt->attributes[c], hereptr, bytesleft));
    }

    const int numchildren = elt->children.getLength();
    if (numchildren == 0) { // close element directly
      ADVANCE_STRING_LITERAL("/>\n");
    } else if ((numchildren == 1) &&
               (strcmp(cc_xml_elt_get_type(elt->children[0]), COIN_XML_CDATA_TYPE) == 0)) {
      ADVANCE_STRING_LITERAL(">");
      ADVANCE_STRING(cc_xml_elt_get_cdata(elt->children[0]));
      ADVANCE_STRING_LITERAL("</");
      ADVANCE_STRING(elt->type);
      ADVANCE_STRING_LITERAL(">\n");
    } else {
      ADVANCE_STRING_LITERAL(">\n");
      for (c = 0; c < numchildren; ++c) {
        ADVANCE_NUM_BYTES(cc_xml_elt_write_to_buffer(elt->children[c], hereptr, bytesleft, indent + indentincrement, indentincrement));
      }
      ADVANCE_NUM_SPACES(indent);
      ADVANCE_STRING_LITERAL("</");
      ADVANCE_STRING(elt->type);
      ADVANCE_STRING_LITERAL(">\n");
    }
  }

#undef ADVANCE_NUM_BYTES
#undef ADVANCE_NUM_SPACES
#undef ADVANCE_STRING
#undef ADVANCE_STRING_LITERAL

  assert(bytes == assumed);
  return bytes;
}

// *************************************************************************
