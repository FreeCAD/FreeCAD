#ifndef COIN_SONODEPROFILING_H
#define COIN_SONODEPROFILING_H

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

#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/system/gl.h>
#include <Inventor/annex/Profiler/SoProfiler.h>
#include <Inventor/annex/Profiler/elements/SoProfilerElement.h>

#include "misc/SoDBP.h" // for global envvar COIN_PROFILER
#include "profiler/SoProfilerP.h"

/*
  The SoNodeProfiling class contains instrumentation code for scene
  graph profiling.

  The convention for using these preTraversal and postTraversal
  methods is that the parent node only applies these around child
  traversal invocations, and not around itself.  This is really
  important to get straight.  As a consequence of this, the applying
  action also needs to add preTraversal()/postTraversal() around
  invoking traversal of the root.

  If you combine doing both, then you get a lot of double-booking of
  timings and negative timing offsets, which causes mayhem in the
  statistics, and was a mess to figure out.
*/

class SoNodeProfiling {
public:
  SoNodeProfiling(void)
    : pretime(SbTime::zero()), entryindex(-1)
  {
  }

  void preTraversal(SoAction * action)
  {
    if (!SoNodeProfiling::isActive(action)) return;

    SoState * state = action->getState();
    SoProfilerElement * profilerelt = SoProfilerElement::get(state);
    SbProfilingData & data = profilerelt->getProfilingData();
    const SoFullPath * fullpath =
      static_cast<const SoFullPath *>(action->getCurPath());
    this->entryindex = data.getIndex(fullpath, TRUE);
    assert(this->entryindex != -1);
    size_t managedmem = 0, unmanagedmem = 0;
    fullpath->getTail()->getFieldsMemorySize(managedmem, unmanagedmem);
    data.setNodeFootprint(this->entryindex,
                          SbProfilingData::MEMORY_SIZE, managedmem);
    data.setNodeFootprint(this->entryindex,
                          SbProfilingData::VIDEO_MEMORY_SIZE, 0);
    this->pretime = SbTime::getTimeOfDay();
  }

  void postTraversal(SoAction * action)
  {
    if (!SoNodeProfiling::isActive(action)) return;

    if (action->isOfType(SoGLRenderAction::getClassTypeId()) &&
        SoProfilerP::shouldSyncGL())
      glFinish();

    const SbTime duration(SbTime::getTimeOfDay() - this->pretime);

    SoState * state = action->getState();
    SoProfilerElement * profilerelt = SoProfilerElement::get(state);
    SbProfilingData & data = profilerelt->getProfilingData();

    assert(this->entryindex != -1);
    int parentindex = data.getParentIndex(this->entryindex);
    if (parentindex != -1) {
      data.preOffsetNodeTiming(parentindex, -duration);
    }

    // see if a children offset has been stored for us and just add timing
    // duration data to that
    const SbTime childrenoffset(data.getNodeTiming(this->entryindex));
    // childrenoffset will deduct the child node timings from this timing
    const SbTime adjusted(childrenoffset + duration);
    assert(adjusted.getValue() >= 0.0);
    data.setNodeTiming(this->entryindex, adjusted);
#if 0 // DEBUG
    const SoFullPath * fullpath = (const SoFullPath *)action->getCurPath();
    SoDebugError::postInfo("Profiling",
                           "%20s (%d): duration %g, offset %g, adjusted %g",
                           fullpath->getTail()->getTypeId().getName().getString(),
                           fullpath->getLength(),
                           duration.getValue(), childrenoffset.getValue(),
                           adjusted.getValue());
#endif
  }

  static bool isActive(SoAction * action)
  {
    if (!SoProfiler::isEnabled()) return false;
    SoState * state = action->getState();
    return state->isElementEnabled(SoProfilerElement::getClassStackIndex()) ? true : false;
  }

private:
  SbTime pretime;
  int entryindex;

};

#endif // !COIN_SONODEPROFILING_H
