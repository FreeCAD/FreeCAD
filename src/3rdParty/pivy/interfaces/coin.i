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

%define COIN_MODULE_DOCSTRING
"Pivy is a Coin binding for Python. Coin is a high-level 3D graphics
library with a C++ Application Programming Interface. Coin uses
scene-graph data structures to render real-time graphics suitable for
mostly all kinds of scientific and engineering visualization
applications."
%enddef

%module(package="pivy", docstring=COIN_MODULE_DOCSTRING) coin

// stdint is not wrapped automatically anymore with swig4.0
// https://stackoverflow.com/questions/40959436/swig-python-detected-a-memory-leak-of-type-uint32-t-no-destructor-found
%include "stdint.i"

%begin %{
#define PY_SSIZE_T_CLEAN
%}

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

%{
/*
  Workaround for FILE* typemap. Import IO module instead of using extern PyTypeObject PyIOBase_Type,
  because the windows python lib does not export PyIOBase_Type.
  Copied from: https://github.com/Kagami/pygraphviz/commit/fe442dc16accb629c3feaf157af75f67ccabbd6e
*/
#if PY_MAJOR_VERSION >= 3
static PyObject *PyIOBase_TypeObj;

static int init_file_emulator(void)
{
    PyObject *io = PyImport_ImportModule("_io");
    if (io == NULL)
        return -1;
    PyIOBase_TypeObj = PyObject_GetAttrString(io, "_IOBase");
    if (PyIOBase_TypeObj == NULL)
        return -1;
    return 0;
}
#endif
%}

%init %{
#if PY_MAJOR_VERSION >= 3
if (init_file_emulator() < 0) {
  #if (SWIG_VERSION < 0x040400)
    return NULL;
  #else
    return 0;
  #endif
}
#endif
%}

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
for key in list(locals()):
  x = locals()[key]
  if isinstance(x, type) and issubclass(x, SoFieldContainer):
    for name in list(x.__dict__):
      thing = x.__dict__[name]
      if isinstance(thing, property):
        delattr(x, name)
%}
