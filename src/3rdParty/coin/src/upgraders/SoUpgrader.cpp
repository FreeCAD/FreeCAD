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

/*!
  \class SoUpgrader SoUpgrader.h
  \brief The SoUpgrader class is used to support Inventor files with version < 2.1.

  \ingroup coin_nodes

  This class is needed since some nodes in earlier versions of
  OpenInventor had different fields than nodes in Inventor V2.1.

*/

// *************************************************************************

#include "upgraders/SoUpgrader.h"

#include <cstddef> // for NULL
#include <cassert>

#include <Inventor/SbName.h>
#include <Inventor/SbString.h>
#include <Inventor/errors/SoDebugError.h>

#include "tidbitsp.h"
#include "io/SoInputP.h"
#include "misc/SbHash.h"
#include "upgraders/SoPackedColorV20.h"
#include "upgraders/SoShapeHintsV10.h"

// *************************************************************************

// an upgrader lookup dict.
//
// FIXME: add a new method to SoType (other than SoType::fromName),
// that checks if a type with a name is registered _without_
// attempting to load the type from a shared object/dll.  pederb.
//
// FIXME: replace this with a real set datatype abstraction. 20050524 mortene.
typedef SbHash<const char *, void *> NameSet;
static NameSet * soupgrader_namedict = NULL;
static SbBool soupgrader_isinitialized = FALSE;

static void
soupgrader_cleanup(void)
{
  delete soupgrader_namedict;
  soupgrader_namedict = NULL;
  soupgrader_isinitialized = FALSE;
}

// add a name to the upgrader lookup dict.
static void
soupgrader_add_to_namedict(const SbString & name)
{
  assert(soupgrader_namedict);

  // Note: the SbString->SbName wrapping is necessary, or the const
  // char* will _not_ be valid upon the SbString going out of scope
  // (while SbName makes permanent const char* references).
  soupgrader_namedict->put(SbName(name.getString()).getString(), NULL);

  // Create lookup both with and without the "So" prefix. This is
  // necessary for the hash lookup in soupgrader_exists() to match
  // with both permutations.

  SbString tmp;
  if (name.compareSubString("So") == 0) {
    tmp = name.getSubString(2);
  }
  else {
    tmp = "So";
    tmp += name;
  }

  // Note: the SbString->SbName wrapping is necessary, see above
  // comment.
  soupgrader_namedict->put(SbName(tmp.getString()).getString(), NULL);
}

static SbBool
soupgrader_exists(const SbName & name)
{
  assert(soupgrader_namedict);
  void * dummy;
  return soupgrader_namedict->get(name.getString(), dummy);
}

#define SOUPGRADER_ADD_TYPE(_class_) \
  _class_::initClass(); \
  soupgrader_add_to_namedict(SO__QUOTE(_class_))

//
// init all upgradable classes. Can be called multiple times.
//
static void
soupgrader_init_classes(void)
{
  if (!soupgrader_isinitialized) {
    soupgrader_namedict = new NameSet;

    SOUPGRADER_ADD_TYPE(SoPackedColorV20);
    SOUPGRADER_ADD_TYPE(SoShapeHintsV10);

    soupgrader_isinitialized = TRUE;
    coin_atexit((coin_atexit_f*) soupgrader_cleanup, CC_ATEXIT_NORMAL);
  }
}

/*!
  Try creating a node of name \a name with Inventor version \a ivversion.

  Returns NULL if no such node exists.
*/
SoBase *
SoUpgrader::tryCreateNode(const SbName & name, const float ivversion)
{
  if ((ivversion == 1.0f) || (ivversion == 2.0f)) {
    soupgrader_init_classes();

    SbString s(name.getString());
    s += (ivversion == 1.0f) ? "V10" : "V20";

    if (soupgrader_exists(s.getString())) {
      SoType type = SoType::fromName(SbName(s.getString()));
      if (type.canCreateInstance()) {
        SoBase * b = (SoBase*) type.createInstance();
        if (SoInputP::debug()) {
          SoDebugError::postInfo("SoUpgrader::tryCreateNode",
                                 "name=='%s', ivversion==%f => SoBase==%p",
                                 name.getString(), ivversion, b);
        }
        return b;
      }
    }
  }
  return NULL;
}

/*!
  Upgrade \a base, usually created using SoUpgrader::tryCreateNode(),
  to the latest version of the same node.
*/
SoBase *
SoUpgrader::createUpgrade(const SoBase * base)
{
  soupgrader_init_classes();

  SoType type = base->getTypeId();

  if (type == SoPackedColorV20::getClassTypeId()) {
    SoPackedColorV20 * pp = (SoPackedColorV20*) base;
    return (SoBase*) pp->createUpgrade();
  }
  else if (type == SoShapeHintsV10::getClassTypeId()) {
    SoShapeHintsV10 * pp = (SoShapeHintsV10*) base;
    return (SoBase*) pp->createUpgrade();
  }
  else {
    SoDebugError::post("SoUpgrader::createUpgrade",
                       "No upgrade functionality available for %s",
                       type.getName().getString());
  }
  return NULL;
}

#undef SOUPGRADER_ADD_TYPE
