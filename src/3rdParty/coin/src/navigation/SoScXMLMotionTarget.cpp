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

#include <Inventor/navigation/SoScXMLMotionTarget.h>
#include "coindefs.h"

/*!
  \class SoScXMLMotionTarget SoScXMLMotionTarget.h Inventor/navigation/SoScXMLMotionTarget.h
  \brief to be used in parallel with other non-moving targets when parallel states are implemented.

  \ingroup coin_navigation
*/

#include <cassert>

#include <Inventor/navigation/SoScXMLNavigation.h>


SCXML_OBJECT_SOURCE(SoScXMLMotionTarget);

void
SoScXMLMotionTarget::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(SoScXMLMotionTarget, SoScXMLNavigationTarget, "SoScXMLNavigationTarget");
}

void
SoScXMLMotionTarget::cleanClass(void)
{
  SoScXMLMotionTarget::classTypeId = SoType::badType();
}

SoScXMLMotionTarget * SoScXMLMotionTarget::theSingleton = NULL;

SoScXMLMotionTarget *
SoScXMLMotionTarget::constructSingleton(void)
{
  assert(SoScXMLMotionTarget::theSingleton == NULL);
  SoScXMLMotionTarget::theSingleton =
    static_cast<SoScXMLMotionTarget *>(SoScXMLMotionTarget::classTypeId.createInstance());
  return SoScXMLMotionTarget::theSingleton;
}

void
SoScXMLMotionTarget::destructSingleton(void)
{
  assert(SoScXMLMotionTarget::theSingleton != NULL);
  delete SoScXMLMotionTarget::theSingleton;
  SoScXMLMotionTarget::theSingleton = NULL;
}

SoScXMLMotionTarget *
SoScXMLMotionTarget::singleton(void)
{
  assert(SoScXMLMotionTarget::theSingleton != NULL);
  return SoScXMLMotionTarget::theSingleton;
}

SoScXMLMotionTarget::SoScXMLMotionTarget(void)
{
  this->setEventTargetType(SOSCXML_NAVIGATION_TARGETTYPE);
  this->setEventTargetName("Motion");
}

SoScXMLMotionTarget::~SoScXMLMotionTarget(void)
{
}

SbBool
SoScXMLMotionTarget::processOneEvent(const ScXMLEvent * COIN_UNUSED_ARG(event))
{
  return FALSE;
}
