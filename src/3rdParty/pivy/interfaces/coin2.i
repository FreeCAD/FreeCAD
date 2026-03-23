/*
 * Copyright (c) 2002-2007 Systems in Motion
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

// define PY_2 for c++ preprocessor
#ifndef PY_2
#define PY_2
#endif

// define PY_2 for swig preprocessor
%{
#ifndef PY_2
#define PY_2
#endif
%}



%define COIN_MODULE_DOCSTRING
"Pivy is a Coin binding for Python. Coin is a high-level 3D graphics
library with a C++ Application Programming Interface. Coin uses
scene-graph data structures to render real-time graphics suitable for
mostly all kinds of scientific and engineering visualization
applications."
%enddef

%module(package="pivy", docstring=COIN_MODULE_DOCSTRING) coin

%{
#if defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#undef max
#undef ERROR
#undef DELETE
#endif

#undef ANY

#include "coin_header_includes.h"

/* make GLState in SoGLLazyElement known to SWIG */
typedef SoGLLazyElement::GLState GLState;
%}

/* enable autodoc'ing for the generated wrapper */
%feature("autodoc", "1");

/* let SWIG handle reference counting for all SoBase derived classes */
%feature("ref") SoBase "$this->ref();"
%feature("unref") SoBase "$this->unref();"

/* include the typemaps common to all pivy modules */
%include pivy_common_typemaps.i
%include coin_header_includes.h

/*
  removes all the properties for fields in classes derived from
  SoFieldContainer. this makes way for the dynamic access to fields
  as attributes.
  
  Note: this has to be the last code in the pivy file, therefore it
  is after all other SWIG declarations!
*/

%pythoncode %{        
for x in locals().values():
  if isinstance(x, type) and issubclass(x, SoFieldContainer):
    for name, thing in x.__dict__.items():
      if isinstance(thing, property):
        delattr(x, name)
%}
