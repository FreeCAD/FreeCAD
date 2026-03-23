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
  \class SoProfilerElement SoProfilerElement.h Inventor/annex/Profiler/SoProfilerElement.h
  \brief The SoProfilerElement element class is for registering statistics during scene graph traversals.

  \ingroup coin_profiler
*/

// *************************************************************************

#include <Inventor/annex/Profiler/elements/SoProfilerElement.h>

#include <cassert>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoNode.h>

// *************************************************************************

SO_ELEMENT_SOURCE(SoProfilerElement);

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoProfilerElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoProfilerElement, SoElement);
}

/*!
  This is the required method of fetching the SoProfilerElement instance.
*/
SoProfilerElement *
SoProfilerElement::get(SoState * state)
{
  assert(state);
  if (!state->isElementEnabled(SoProfilerElement::classStackIndex)) {
    return NULL;
  }

  SoElement * elt = state->getElementNoPush(SoProfilerElement::classStackIndex);
  return static_cast<SoProfilerElement *>(elt);
}

SoProfilerElement::~SoProfilerElement(void)
{
}

// *************************************************************************

void
SoProfilerElement::push(SoState *)
{
  // NOTE: if this triggers, someone has probably used
  // SoState->getElement(stackindex) to fetch the element instead of
  // using the required SoProfilerElemenet::get(SoState *)
  assert(!"programming error: SoProfilerElement should not be stack-pushed");
  SoDebugError::post("SoProfilerElement::push",
                     "programming error: SoProfilerElement should not be stack-pushed");
}

void
SoProfilerElement::pop(SoState *, const SoElement *)
{
  // NOTE: if this triggers, someone has probably used
  // SoState->getElement(stackindex) to fetch the element instead of
  // using the required SoProfilerElemenet::get(SoState *)
  assert(!"programming error: SoProfilerElement should not be stack-pushed");
  SoDebugError::post("SoProfilerElement::pop",
                     "programming error: SoProfilerElement should not be stack-pushed");
}

// *************************************************************************

SbBool
SoProfilerElement::matches(const SoElement * element) const
{
  if (element == this) return TRUE;
  const SoProfilerElement * pelement =
    static_cast<const SoProfilerElement *>(element);
  if (pelement->getProfilingData() == this->getProfilingData()) return TRUE;
  return FALSE;
}

SoElement *
SoProfilerElement::copyMatchInfo(void) const
{
  SoProfilerElement * elem =
    static_cast<SoProfilerElement *>(this->getTypeId().createInstance());
  elem->getProfilingData() = this->getProfilingData();
  return elem;
}

// *************************************************************************

/*
void
SoProfilerElement::setTimingProfile(SoNode * node, SbTime t, SoNode * parent)
{
  assert(node);
  this->data.accumulateTraversalTime(parent ? parent->getTypeId() : SoType::badType(),
                                     node->getTypeId(), t);

  if (parent == NULL) return;
  this->data.setChildTiming(parent, node, t);
}
*/

// *************************************************************************

/*
void
SoProfilerElement::setHasGLCache(SoNode * node, SbBool hascache)
{
  this->data.setHasGLCache(node, hascache);
}

void
SoProfilerElement::startTraversalClock()
{
  this->data.setActionStartTime(SbTime::getTimeOfDay());
}

SbTime
SoProfilerElement::timeSinceTraversalStart()
{
  if (this->data.getActionStartTime() == SbTime::zero())
    return SbTime::zero();
  return SbTime::getTimeOfDay() - this->data.getActionStartTime();
}
*/

const SbProfilingData &
SoProfilerElement::getProfilingData(void) const
{
  return this->data;
}

SbProfilingData &
SoProfilerElement::getProfilingData(void)
{
  return this->data;
}

// *************************************************************************
