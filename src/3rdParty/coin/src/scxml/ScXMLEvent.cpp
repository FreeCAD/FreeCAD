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

#include <Inventor/scxml/ScXMLEvent.h>

/*!
  \class ScXMLEvent ScXMLEvent.h Inventor/scxml/ScXMLEvent.h
  \brief Base class for events sent to SCXML state machines.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>
#include <map>

#include <Inventor/scxml/ScXML.h>

// *************************************************************************

class ScXMLEvent::PImpl {
public:
  PImpl(void) { }
  ~PImpl(void)
  {
    // FIXME: delete strings in associations map (values only)
  }

  std::map<const char *, const char *> associations;
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_OBJECT_SOURCE(ScXMLEvent);

void
ScXMLEvent::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLEvent, ScXMLObject, "ScXMLObject");
}

void
ScXMLEvent::cleanClass(void)
{
  ScXMLEvent::classTypeId = SoType::badType();
}

ScXMLEvent::ScXMLEvent(void)
: name(SbName::empty())
{
}

ScXMLEvent::~ScXMLEvent(void)
{
}

/*!
  This method is for setting a string that will identify this particular
  event, having this particular state, and can be used for event matching
  in the SCXML descriptions.

  The string should, according to the specification, be a set of tokens consisting
  of alphanumeric characters, separated with periods (.). This limitation
  is not enforced by this implementation.
*/
void
ScXMLEvent::setEventName(const SbName & namearg)
{
  this->name = namearg;
}

void
ScXMLEvent::setAssociation(const char * key, const char * value)
{
  assert(key && value);
  SbName dictentry(key);
  std::map<const char *, const char *>::iterator findit =
    PRIVATE(this)->associations.find(dictentry.getString());
  char * valuecopy = new char [strlen(value) + 1];
  strcpy(valuecopy, value);
  if (findit != PRIVATE(this)->associations.end()) {
    delete [] findit->second;
    findit->second = valuecopy;
  } else {
    std::pair<const char *, const char *> newentry(dictentry.getString(), valuecopy);
    PRIVATE(this)->associations.insert(newentry);
  }
}

const char *
ScXMLEvent::getAssociation(const char * key) const
{
  assert(key);
  SbName dictentry(key);
  std::map<const char *, const char *>::const_iterator findit =
    PRIVATE(this)->associations.find(dictentry.getString());
  if (findit != PRIVATE(this)->associations.end()) {
    return findit->second;
  }
  return NULL;
}

size_t
ScXMLEvent::getNumAssociations(void) const
{
  return PRIVATE(this)->associations.size();
}

size_t
ScXMLEvent::getAssociationKeys(SbList<const char *> & keys) const
{
  std::map<const char *, const char *>::const_iterator it =
    PRIVATE(this)->associations.begin();
  size_t count = 0;
  while (it != PRIVATE(this)->associations.end()) {
    keys.append(it->first);
    ++count;
    ++it;
  }
  return count;
}

ScXMLEvent *
ScXMLEvent::clone(void) const
{
  SoType thistype = this->getTypeId();
  assert(thistype.canCreateInstance());
  ScXMLEvent * newevent = static_cast<ScXMLEvent *>(thistype.createInstance());
  newevent->copyContents(this);
  return newevent;
}

void
ScXMLEvent::copyContents(const ScXMLEvent * rhs)
{
  this->name = rhs->name;
  SbList<const char *> keys;
  size_t numkeys = rhs->getAssociationKeys(keys);
  for (size_t i = 0; i < numkeys; ++i) {
    const char * akey = keys[(int)i];
    this->setAssociation(akey, rhs->getAssociation(akey));
  }
}

#undef PRIVATE
