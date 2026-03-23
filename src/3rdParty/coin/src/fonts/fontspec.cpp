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

#include "fonts/fontspec.h"

#include <cstring>

#include "glue/freetype.h"

void
cc_fontspec_construct(cc_font_specification * spec,
                      const char * name_style, float size, float complexity)
{
  const char * tmpstr, * tmpptr;

  spec->size = size;
  spec->complexity = complexity;

  cc_string_construct(&spec->name);
  cc_string_set_text(&spec->name, name_style);

  cc_string_construct(&spec->style);

  /* handle the previously allowed ':Bold Italic' case for fontconfig */
  /* FIXME: this is an ugly non robust workaround. it would be better
     to agree on an abstract fontname matching schema that is then
     consistently applied upon all font backend
     implementations. 20040929 tamer. */
  if (cc_fcglue_available()) {
    tmpstr = cc_string_get_text(&spec->name);

    if ((tmpptr = strchr(tmpstr, ':'))) {
      char * tmpptrspace;
      if ((tmpptrspace = (char *) strchr(tmpptr, ' '))) {
        *tmpptrspace = ':';
      }
    }

    return;
  }

  /* Check if style is included in the fontname using the
     "family:style" syntax. */
  tmpstr = cc_string_get_text(&spec->name);
  if ((tmpptr = strchr(tmpstr, ':'))) {
    const int pos = (int)(tmpptr - tmpstr);
    const int namelen = cc_string_length(&spec->name);

    int trimstyleend, trimnamestart;
    int trimposstyle = pos + 1;
    int trimposname = pos - 1;

    while (tmpstr[trimposstyle] == ' ') {
      ++trimposstyle;
    }

    while (tmpstr[trimposname] == ' ') {
      --trimposname;
    }

    cc_string_set_text(&spec->style, cc_string_get_text(&spec->name));
    cc_string_remove_substring(&spec->style, 0, trimposstyle - 1);
    cc_string_remove_substring(&spec->name, trimposname + 1, namelen-1);

    trimstyleend = cc_string_length(&spec->style);
    trimposstyle = trimstyleend;
    tmpstr = cc_string_get_text(&spec->style);

    while (tmpstr[trimstyleend-1] == ' ') {
      --trimstyleend;
    }

    if(trimstyleend !=  trimposstyle) {
      cc_string_remove_substring(&spec->style, trimstyleend, cc_string_length(&spec->style) - 1);
    }

    tmpstr = cc_string_get_text(&spec->name);
    trimnamestart = 0;
    while (tmpstr[trimnamestart] == ' ') {
      ++trimnamestart;
    }

    if (trimnamestart != 0) {
      cc_string_remove_substring(&spec->name, 0, trimnamestart-1);
    }

  }
}

void
cc_fontspec_copy(const cc_font_specification * from,
                 cc_font_specification * to)
{
  to->size = from->size;
  to->complexity = from->complexity;

  cc_string_construct(&to->name);
  cc_string_set_string(&to->name, &from->name);
  cc_string_construct(&to->style);
  cc_string_set_string(&to->style, &from->style);
}

void
cc_fontspec_clean(cc_font_specification * spec)
{
  cc_string_clean(&spec->name);
  cc_string_clean(&spec->style);
}
