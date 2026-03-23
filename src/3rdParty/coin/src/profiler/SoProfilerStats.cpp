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

#include <Inventor/annex/Profiler/nodes/SoProfilerStats.h>

#include <map>

#include <Inventor/annex/Profiler/elements/SoProfilerElement.h>
#include <Inventor/annex/Profiler/SbProfilingData.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/elements/SoCacheElement.h>

#include "nodes/SoSubNodeP.h"
#include "misc/SbHash.h"

// decide whether to expose profiling stats by node type or by
// user-specified node names
#define BY_TYPE 1

// *************************************************************************

/*!
  \class SoProfilerStats SoProfilerStats.h Inventor/annex/Profiler/nodes/SoProfilerStats.h
  \brief The SoProfilerStats class is a node for exposing profiling results gathered by \a SoProfilerElement.

  TODO: Describe functionality and provide usage example.

  \sa SoProfilerElement
  \ingroup coin_profiler
*/

/*!
  \var SoMFName SoProfilerStats::renderedNodeType

  Name of types traversed during the current render traversal.

  This field is tightly connected to the \a renderingTimePerNodeType field as
  it will contain the same number of elements, and each value
  in \a renderingTimePerNodeType matches the node type with the same index in
  this field.

  \sa SoProfilerStats::renderingTimePerNodeType
*/

/*!
  \var SoMFTime SoProfilerStats::renderingTimePerNodeType

  Time spent, during the current render traversal, per node type.

  This field is tightly connected to the \a renderedNodeType field as
  it will contain the same number of elements, and each value
  in this field matches the node type with the same index in
  \a renderedNodeType.

  \sa SoProfilerStats::renderedNodeType
*/

// *************************************************************************

#define PUBLIC(obj) ((obj)->master)

class SoProfilerStatsP {
private:
  struct TypeTimings {
    SbTime max, total;
    int32_t count;
  };

public:
  SoProfilerStatsP(void) : master(NULL) {
  }
  ~SoProfilerStatsP(void) { }

  void doAction(SoAction * action);
  void clearProfilingData(void);
  void updateActionTimingMaps(SoProfilerElement * e, SoAction * action);
  void updateActionTimingFields(SoProfilerElement * e);
  void updateNodeTypeTimingMap(SoProfilerElement * e);
  void updateNodeTypeTimingFields();

  std::map<int16_t, SbProfilingData *> action_map;
  std::map<int16_t, TypeTimings> type_timings;
  std::map<const char *, TypeTimings> name_timings;

  SoProfilerStats * master;

  SbHash<int16_t, SbTime> action_timings;
}; // SoProfilerStatsP

void
SoProfilerStatsP::doAction(SoAction * action)
{
  // every SoGLRenderAction traversal will set a flag that causes
  // the profiling data to be cleared at the next traversal, regardless of
  // type
  static SbBool clear_state = FALSE;
  if (clear_state) {
    this->clearProfilingData();
    clear_state = FALSE;
  }

  this->action_timings.put(SoGLRenderAction::getClassTypeId().getKey(),
                           SbTime::zero());

  SoState * state = action->getState();
  if (state->isElementEnabled(SoCacheElement::getClassStackIndex())) {
    SoCacheElement::invalidate(state);
  }

  SoProfilerElement * e = SoProfilerElement::get(state);
  if (!e) { return; }


  std::map<int16_t, SbProfilingData *>::iterator it =
    this->action_map.find(action->getTypeId().getKey());
  if (it != this->action_map.end()) {
    (*((*it).second)) += e->getProfilingData();
  } else {
    SbProfilingData * data = new SbProfilingData(e->getProfilingData());
    std::pair<int16_t, SbProfilingData *> entry(action->getTypeId().getKey(), data);
    this->action_map.insert(entry);
  }

  this->updateNodeTypeTimingMap(e);
  this->updateActionTimingMaps(e, action);

  if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
    this->updateNodeTypeTimingFields();
    updateActionTimingFields(e);
    PUBLIC(this)->profilingUpdate.touch();

    clear_state = TRUE;
  }
} // doAction

void
SoProfilerStatsP::clearProfilingData(void)
{
  std::map<int16_t, SbProfilingData *>::iterator it, end;
  for (it = this->action_map.begin(), end = this->action_map.end();
       it != end; ++it) {
    delete (*it).second;
  }
  this->action_map.clear();
  this->action_timings.clear();
#if BY_TYPE
  this->type_timings.clear();
#else
  this->name_timings.clear();
#endif
} // clearActionMap

void
SoProfilerStatsP::updateActionTimingMaps(SoProfilerElement * e,
                                         SoAction * action)
{
  int16_t type = action->getTypeId().getKey();
  SbTime time = SbTime::zero();
  SbBool found = this->action_timings.get(type, time);
  if (found)
    time += e->getProfilingData().getActionDuration();
  else
    time = e->getProfilingData().getActionDuration();

  this->action_timings.put(type, time);
} // updateActionTimingMaps

void
SoProfilerStatsP::updateNodeTypeTimingMap(SoProfilerElement * e)
{
  const SbProfilingData & data = e->getProfilingData();
#if BY_TYPE
  SbList<SbProfilingNodeTypeKey> keys;
  data.getStatsForTypesKeyList(keys);
  int keyCount = keys.getLength();
  for (int i = 0; i < keyCount; ++i) {
    SbProfilingNodeTypeKey k = keys[i];
    std::map<int16_t, TypeTimings>::iterator it = this->type_timings.find(k);
    if (it != this->type_timings.end()) {
      TypeTimings & timings = it->second;
      SbTime totaltime, maxtime;
      uint32_t count;
      data.getStatsForType(k, totaltime, maxtime, count);
      timings.total += totaltime;
      timings.max += maxtime;
      timings.count += count;
    } else {
      TypeTimings timings;
      SbTime totaltime, maxtime;
      uint32_t count;
      data.getStatsForType(k, totaltime, maxtime, count);
      timings.total = totaltime;
      timings.max = maxtime;
      timings.count = count;
      std::pair<int16_t, TypeTimings> entry(k, timings);
      this->type_timings.insert(entry);
    }
  }
#else
  SbList<SbProfilingNodeNameKey> keys;
  data.getStatsForNamesKeyList(keys);
  int keyCount = keys.getLength();
  for (int i = 0; i < keyCount; ++i) {
    SbProfilingNodeNameKey k = keys[i];
    std::map<const char *, TypeTimings>::iterator it = this->name_timings.find(k);
    if (it != this->name_timings.end()) {
      TypeTimings & timings = it->second;
      SbTime totaltime, maxtime;
      uint32_t count;
      data.getStatsForName(k, totaltime, maxtime, count);
      it->second.total += totaltime;
      it->second.max += maxtime;
      it->second.count += count;
    } else {
      TypeTimings timings;
      SbTime totaltime, maxtime;
      uint32_t count;
      data.getStatsForName(k, totaltime, maxtime, count);
      timings.total = totaltime;
      timings.max = maxtime;
      timings.count = count;
      std::pair<SbProfilingNodeNameKey, TypeTimings> entry(k, timings);
      this->name_timings.insert(entry);
    }
  }
#endif
} // updateNodeTypeTimingMaps

void
SoProfilerStatsP::updateNodeTypeTimingFields()
{
#if BY_TYPE
  int keyCount = (int) this->type_timings.size();
#else
  int keyCount = (int) this->name_timings.size();
#endif

  PUBLIC(this)->renderedNodeType.setNum(keyCount);
  SbName * typeNameArr = PUBLIC(this)->renderedNodeType.startEditing();

  PUBLIC(this)->renderingTimePerNodeType.setNum(keyCount);
  SbTime * renderingTimePerNodeTypeArr = PUBLIC(this)->renderingTimePerNodeType.startEditing();

  PUBLIC(this)->renderingTimeMaxPerNodeType.setNum(keyCount);
  SbTime * renderingTimeMaxPerNodeTypeArr = PUBLIC(this)->renderingTimeMaxPerNodeType.startEditing();

  PUBLIC(this)->renderedNodeTypeCount.setNum(keyCount);
  uint32_t * nodeTypeCountArr = PUBLIC(this)->renderedNodeTypeCount.startEditing();

  int index = 0;
#if BY_TYPE
  std::map<int16_t, TypeTimings>::iterator it, end;
  for (it = this->type_timings.begin(), end = this->type_timings.end();
       it != end; ++it, ++index) {
    typeNameArr[index] = SoType::fromKey(it->first).getName();
#else
  std::map<const char *, TypeTimings>::iterator it, end;
  for (it = this->name_timings.begin(), end = this->name_timings.end();
       it != end; ++it, ++index) {
    typeNameArr[index] = SbName(it->first);
#endif
    renderingTimePerNodeTypeArr[index] = it->second.total;
    renderingTimeMaxPerNodeTypeArr[index] = it->second.max;
    nodeTypeCountArr[index] = it->second.count;
  }

  PUBLIC(this)->renderedNodeType.finishEditing();
  PUBLIC(this)->renderingTimePerNodeType.finishEditing();
  PUBLIC(this)->renderingTimeMaxPerNodeType.finishEditing();
  PUBLIC(this)->renderedNodeTypeCount.finishEditing();
} // updateNodeTypeTimingFields

void
SoProfilerStatsP::updateActionTimingFields(SoProfilerElement * COIN_UNUSED_ARG(e))
{
  SbList<int16_t> actions;
  this->action_timings.makeKeyList(actions);
  int actionCount = actions.getLength();

  PUBLIC(this)->profiledAction.setNum(actionCount);
  SbName * profiledActionArr = PUBLIC(this)->profiledAction.startEditing();

  PUBLIC(this)->profiledActionTime.setNum(actionCount);
  SbTime * profiledActionTimeArr = PUBLIC(this)->profiledActionTime.startEditing();

  for (int i = 0; i < actionCount; ++i) {
    int16_t type = actions[i];
    profiledActionArr[i] = SoType::fromKey(type).getName();

    SbTime time = SbTime::zero();
    this->action_timings.get(type, time);
    profiledActionTimeArr[i] = time;
  }

  PUBLIC(this)->profiledAction.finishEditing();
  PUBLIC(this)->profiledActionTime.finishEditing();
} // updateActionTimingFields


#undef PUBLIC


#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoProfilerStats);

// *************************************************************************

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoProfilerStats::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoProfilerStats, SO_FROM_COIN_3_0);

  SO_ENABLE(SoGLRenderAction, SoProfilerElement);
  SO_ENABLE(SoHandleEventAction, SoProfilerElement);
}

/*!
  Constructor
 */
SoProfilerStats::SoProfilerStats(void)
{
  PRIVATE(this)->master = this;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoProfilerStats);

  SO_NODE_ADD_FIELD(renderedNodeType, (""));
  SO_NODE_ADD_FIELD(renderingTimePerNodeType, (0.0f));
  SO_NODE_ADD_FIELD(renderingTimeMaxPerNodeType, (0.0f));
  SO_NODE_ADD_FIELD(renderedNodeTypeCount, (0));
  SO_NODE_ADD_FIELD(profiledAction, (""));
  SO_NODE_ADD_FIELD(profiledActionTime, (0.0f));
  SO_NODE_ADD_FIELD(profilingUpdate, ());

  this->renderedNodeType.setNum(0);
  this->renderedNodeType.setDefault(TRUE);
  this->renderingTimePerNodeType.setNum(0);
  this->renderingTimePerNodeType.setDefault(TRUE);
  this->renderingTimeMaxPerNodeType.setNum(0);
  this->renderingTimeMaxPerNodeType.setDefault(TRUE);
  this->renderedNodeTypeCount.setNum(0);
  this->renderedNodeTypeCount.setDefault(TRUE);
  this->profiledAction.setNum(0);
  this->profiledAction.setDefault(TRUE);
  this->profiledActionTime.setNum(0);
  this->profiledActionTime.setDefault(TRUE);

}

/*!
  Destructor
 */
SoProfilerStats::~SoProfilerStats()
{
}

// *************************************************************************

// Doc from superclass.
void
SoProfilerStats::GLRender(SoGLRenderAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::handleEvent(SoHandleEventAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::rayPick(SoRayPickAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::getBoundingBox(SoGetBoundingBoxAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::audioRender(SoAudioRenderAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::callback(SoCallbackAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::getMatrix(SoGetMatrixAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::pick(SoPickAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::search(SoSearchAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::write(SoWriteAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  PRIVATE(this)->doAction(action);
}

// Doc from superclass.
void
SoProfilerStats::notify(SoNotList * COIN_UNUSED_ARG(l))
{
  // we disable notifications from this node, to avoid constant
  // redraws of the scene graph.
  //
  // and it doesn't expose any state relevant for any other nodes that
  // do "real work" in the scene graph anyway, so this is the correct
  // thing to do.
}

const SbProfilingData &
SoProfilerStats::getProfilingData(SoType actiontype) const
{
  std::map<int16_t, SbProfilingData *>::const_iterator it =
    PRIVATE(this)->action_map.find(actiontype.getKey());
  if (it != PRIVATE(this)->action_map.end()) {
    return (*((*it).second));
  }

  static SbProfilingData nodata;
  return nodata;
}

// *************************************************************************

#undef PRIVATE
#undef BY_TYPE
