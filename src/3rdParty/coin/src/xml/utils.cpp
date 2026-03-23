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

#include "utils.h"

#include <Inventor/C/basic.h>
#include <Inventor/C/XML/types.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>

char *
cc_xml_load_file(const char * path)
{
  FILE * fd = fopen(path, "rb");
  if ( !fd ) return NULL;
  fseek(fd, 0, SEEK_END);
  const long bufsize = ftell(fd);
  fseek(fd, 0, SEEK_SET);
  char * buffer = new char [ bufsize + 1 ];
  size_t pos = 0, bytes;
  while ( pos != bufsize ) {
    bytes = fread(buffer + pos, 1, bufsize - pos, fd);
    if ( bytes == 0 ) {
      // fprintf(stderr, "fread() returned %d\n", bytes);
    } else {
      pos += bytes;
    }
  }
  buffer[bufsize] = '\0';
  fclose(fd); // close opened file
  return buffer;
}

// *************************************************************************

char *
cc_xml_strndup(const char * str, size_t len)
{
  char * buf = new char [ len + 1 ];
  assert(buf != NULL);
  memcpy(buf, str, len);
  buf[len] = '\0';
  return buf;
}

char *
cc_xml_strdup(const char * str)
{
  return cc_xml_strndup(str, strlen(str));
}

// *************************************************************************

/* since true/false is returned, stricmp() was an unfortunate name */
int
cc_xml_strieq(const char * s1, const char * s2)
{
  while ( *s1 && *s2 )
    if ( tolower(*s1++) != tolower(*s2++) ) return FALSE;
  if ( *s1 || *s2 ) return FALSE;
  return TRUE;
}

#if 0
int
sc_whitespace_p(const char * string)
{
  assert(string != NULL);
  while ( *string ) {
    switch ( *string ) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      break;
    default:
      return FALSE;
    }
    string++;
  }
  return TRUE;
}
#endif
