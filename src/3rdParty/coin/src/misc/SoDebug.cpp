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

#include <SoDebug.h>

#include <cstdarg>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <Inventor/C/tidbits.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SbName.h>

#include "tidbitsp.h"

inline unsigned int SbHashFunc(const void * key);
#include "misc/SbHash.h"
inline unsigned int SbHashFunc(const void * key)
{
  return SbHashFunc(reinterpret_cast<size_t>(key));
}

// *************************************************************************

/*!
  \class SoDebug SoDebug.h SoDebug.h
  The SoDebug class is a small collection of debugging related functions.
*/

// *************************************************************************

namespace {

/*!
  \struct SoDebug_internal
  COININTERNAL
*/

struct SoDebug_internal {
  static SbHash<void *, char *> * namedict;
  static void delete_namedict(void);
};

SbHash<void *, char *> * SoDebug_internal::namedict = NULL;

} // anonymous namespace

/*!
  This is a portable getenv-wrapper.

  \sa coin_getenv()
*/

const char *
SoDebug::GetEnv(const char * var)
{
  return coin_getenv(var);
}

/*!
  Real-time printf designed for use when use of standard printf() would
  cause timing problems.

  NOT IMPLEMENTED.  Currently it just forwards to printf().
*/

void
SoDebug::RTPrintf(const char * formatstr, ...)
{
  // FIXME: should print to string buffer, dump complete buffer now
  // and then instead
  va_list args;
  va_start(args, formatstr);
  vprintf(formatstr, args);
  va_end(args);
}

/*!
  Associate a name with an arbitrary pointer.  You can fetch the name of
  the pointer later with PtrName().

  \sa PtrName()
*/

void
SoDebug::NamePtr(const char * name, void * ptr)
{
  if ( SoDebug_internal::namedict == NULL ) {
    SoDebug_internal::namedict = new SbHash<void *, char *>;
    coin_atexit(SoDebug_internal::delete_namedict, CC_ATEXIT_NORMAL);
  }
  char * data = NULL;
  if ( SoDebug_internal::namedict->get(ptr, data) ) {
    free(data);
    SoDebug_internal::namedict->erase(ptr);
  }
  data = strdup(name);
  SoDebug_internal::namedict->put(ptr, data);
}

/*!
  Returns the name set on a pointer with NamePtr().  If no name has been
  set, "<unnamed>" is returned.

  \sa NamePtr()
*/


const char *
SoDebug::PtrName(void * ptr)
{
  static const char fallback[] = "<noName>";
  if ( SoDebug_internal::namedict == NULL ) return fallback;
  char * data = NULL;
  if ( SoDebug_internal::namedict->get(ptr, data) ) {
    if ( data ) {
      return data;
    }
  }
  return fallback;
}

/*!
  Writes the node to stdout.
*/

void
SoDebug::write(SoNode * node)
{
  node->ref();
  SoOutput out;
  if (node->getNodeType() == SoNode::VRML2) {
    out.setHeaderString("#VRML V2.0 utf8");
  }
  SoWriteAction wa(&out);
  wa.apply(node);
  node->unrefNoDelete();
}

/*!
  Writes the node to the given filename, or /tmp/debug.iv if filename is NULL.
*/

void
SoDebug::writeToFile(SoNode * node, const char * filename)
{
  node->ref();
  const char * fname = filename ? filename : "/tmp/debug.iv";
  SoOutput out;
  out.openFile(fname);
  if (node->getNodeType() == SoNode::VRML2) {
    out.setHeaderString("#VRML V2.0 utf8");
  }
  SoWriteAction wa(&out);
  wa.apply(node);
  out.closeFile();
  node->unrefNoDelete();
}

/*!
  Not implemented.
*/

void
SoDebug::writeField(SoField * field)
{
  SoFieldContainer * container = field->getContainer();
  if (!container) return;
  SbName name;
  container->getFieldName(field, name);

  SoOutput out;
  field->write(&out, name);
}

/*!
  Not implemented.
*/

void
SoDebug::printName(SoBase * base)
{
  SbName name = base->getName();
  if (name != SbName::empty()) {
    puts(name.getString());
  } else {
    puts(" not named ");
  }
}

// *************************************************************************

void
SoDebug_internal::delete_namedict(void)
{
  for(
     SbHash<void *, char *>::const_iterator iter =
       namedict->const_begin();
      iter!=namedict->const_end();
      ++iter
      ) {
    free(iter->obj);
  }

  delete namedict;
  namedict = NULL;
}

// *************************************************************************
