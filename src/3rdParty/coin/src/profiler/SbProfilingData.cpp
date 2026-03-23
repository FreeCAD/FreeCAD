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
  \struct SbNodeProfilingData
  \brief Data structure for gathering scene graph traversal profiling information for one node.

  \ingroup coin_profiler
*/

/*!
  \class SbProfilingData SbProfilingData.h Inventor/annex/SbProfilingData.h
  \brief Data structure for gathering scene graph traversal profiling information.

  \ingroup coin_profiler
*/

#include <Inventor/annex/Profiler/SbProfilingData.h>
#include "coindefs.h"

#include <algorithm> // std::reverse
#include <cstring>
#include <map>
#include <vector>

#include <Inventor/SbName.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/nodes/SoNode.h>

// *************************************************************************

/*!
  \typedef void * SbProfilingNodeKey

  Type definition for the key node. This is a void * to ensure
  that it is not inadvertently dereferenced.
*/

/*!
  \typedef int16_t SbProfilingNodeTypeKey

  Type definition for the key node type.
*/

/*!
  \typedef const char * SbProfilingNodeNameKey

  Type definition for the key node name.
*/

// *************************************************************************

// SbNodeProfilingData - internal structure containing profiling data
// for one node.
struct SbNodeProfilingData {
  SbProfilingNodeKey node;
  SbProfilingNodeNameKey nodename;
  SbProfilingNodeTypeKey nodetype;
  int parentidx;
  int childidx;

  SbTime traversaltime;
  size_t memorysize;
  size_t texturesize;
  int traversalcount;

  struct {
    unsigned int glcached : 1;
    unsigned int culled : 1;
  } flags;

  inline SbNodeProfilingData(void);
  inline int operator == (const SbNodeProfilingData & rhs) const;
  inline int operator != (const SbNodeProfilingData & rhs) const;

}; // SbNodeProfilingData

struct SbTypeProfilingData {
  SbTime totaltime;
  SbTime maximumtime;
  int count;

  inline SbTypeProfilingData(void);

}; // SbTypeProfilingData

struct SbNameProfilingData {
  SbTime totaltime;
  SbTime maximumtime;
  int count;

  inline SbNameProfilingData(void);

}; // SbNameProfilingData

// inlined methods

SbNodeProfilingData::SbNodeProfilingData(void)
: node(NULL), /* nodename(NULL), */ nodetype(0),
  parentidx(-1), childidx(0),
  traversaltime(0.0), memorysize(0), texturesize(0), traversalcount(0)
{
  this->flags.glcached = 0;
  this->flags.culled = 0;
}

int
SbNodeProfilingData::operator == (const SbNodeProfilingData & rhs) const {
  return (memcmp(this, &rhs, sizeof(SbNodeProfilingData)) == 0);
}

int
SbNodeProfilingData::operator != (const SbNodeProfilingData & rhs) const {
  return (memcmp(this, &rhs, sizeof(SbNodeProfilingData)) != 0);
}

SbTypeProfilingData::SbTypeProfilingData(void)
: totaltime(0.0), maximumtime(0.0), count(0)
{
}

SbNameProfilingData::SbNameProfilingData(void)
: totaltime(0.0), maximumtime(0.0), count(0)
{
}

// *************************************************************************

class SbProfilingDataP {
public:

  std::vector<SbNodeProfilingData> nodeData;
  int lastPathIndex;

  std::map<SbProfilingNodeTypeKey, SbTypeProfilingData> nodeTypeData;
  std::map<SbProfilingNodeNameKey, SbNameProfilingData> nodeNameData;

}; // SbProfilingDataP

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  Constructor.
*/
SbProfilingData::SbProfilingData(void)
{
  this->constructorInit();
}

/*!
  Copy constructor.
*/
SbProfilingData::SbProfilingData(const SbProfilingData & rhs)
{
  this->constructorInit();
  this->operator = (rhs);
}

/*!
  Destructor.
*/
SbProfilingData::~SbProfilingData(void)
{
  this->reset();
}

/*!
  Common initialization code for the constructors.
*/
void
SbProfilingData::constructorInit(void)
{
  // NB: if resource allocation is added, rewrite reset() to not call here
  this->actionType = SoType::badType();
  this->actionStartTime = SbTime::zero();
  this->actionStopTime = SbTime::zero();
  PRIVATE(this)->lastPathIndex = -1;
}

/*!
  Remove all stored data.
*/
void
SbProfilingData::reset(void)
{
  this->constructorInit();
  PRIVATE(this)->nodeData.clear();
  PRIVATE(this)->nodeTypeData.clear();
  PRIVATE(this)->nodeNameData.clear();
  assert(PRIVATE(this)->nodeData.size() == 0);
  assert(PRIVATE(this)->nodeTypeData.size() == 0);
  assert(PRIVATE(this)->nodeNameData.size() == 0);
}

/*!
  Assignment operator.
*/
SbProfilingData &
SbProfilingData::operator = (const SbProfilingData & rhs)
{
  this->reset();
  this->actionType = rhs.actionType;
  this->actionStartTime = rhs.actionStartTime;
  this->actionStopTime = rhs.actionStopTime;
  PRIVATE(this)->lastPathIndex = -1;
  PRIVATE(this)->nodeData = PRIVATE(&rhs)->nodeData;
  PRIVATE(this)->nodeTypeData = PRIVATE(&rhs)->nodeTypeData;
  PRIVATE(this)->nodeNameData = PRIVATE(&rhs)->nodeNameData;
  assert(PRIVATE(this)->nodeData.size() == PRIVATE(&rhs)->nodeData.size());
  return *this;
}

// search dst for the given path in src, and if nonexistent create it
static void
findPath(const std::vector<SbNodeProfilingData> & src, std::vector<SbNodeProfilingData> & dst, int srcentryidx, int & matchidx, int & parentidx)
{
  matchidx = -1;
  parentidx = -1;

  // By going from 0 and outwards, the source-vector parent node
  // will always already exist in the target-vector, which should
  // help a lot.
  const int numdestentries = (int)dst.size();

  // FIXME: optimize this part

  bool match = false;
  for (int i = 0; i < numdestentries && !match; ++i) {
    if (dst[i].node == src[srcentryidx].node &&
        dst[i].childidx == src[srcentryidx].childidx) {
      // we have a potential match
      int dstidx = dst[i].parentidx, srcidx = src[srcentryidx].parentidx;
      while ((srcidx != -1) &&
             (dstidx != -1) &&
             (src[srcidx].node == dst[dstidx].node) &&
             (src[srcidx].childidx == dst[dstidx].childidx)) {
        srcidx = src[srcidx].parentidx;
        dstidx = dst[dstidx].parentidx;
      }
      if (srcidx == -1 && dstidx == -1) { // match!
        match = true;
        matchidx = i;
      }
    }
  }

  if (match) {
    parentidx = dst[matchidx].parentidx;
  } else {
    matchidx = -1;

    // search for parent instead
    srcentryidx = src[srcentryidx].parentidx;
    if (srcentryidx == -1) { // root doesn't have parent
      parentidx = -1;
      return;
    }

    for (int i = 0; i < numdestentries && !match; ++i) {
      if (dst[i].node == src[srcentryidx].node &&
          dst[i].childidx == src[srcentryidx].childidx) {
        // we have a potential match
        int dstidx = dst[i].parentidx, srcidx = src[srcentryidx].parentidx;
        while ((srcidx != -1) &&
               (dstidx != -1) &&
               (src[srcidx].node == dst[dstidx].node) &&
               (src[srcidx].childidx == dst[dstidx].childidx)) {
          srcidx = src[srcidx].parentidx;
          dstidx = dst[dstidx].parentidx;
        }
        if (srcidx == -1 && dstidx == -1) { // match!
          match = true;
          parentidx = i;
        }
      }
    }
  }
}

/*!
  Add profiling data from another data set.
*/
SbProfilingData &
SbProfilingData::operator += (const SbProfilingData & rhs)
{
  assert(this != &rhs);

  if (PRIVATE(this)->nodeData.size() == 0) {
    return this->operator = (rhs);
  }

  if (rhs.actionType == SoType::badType()) {
    // nada - assume same as this
  } else if (this->actionType == SoType::badType()) {
    this->actionType = rhs.actionType;
  } else if (this->actionType != rhs.actionType) {
    this->actionType = SoType::badType();
  }

  if (this->actionStartTime == SbTime::zero()) {
    this->actionStartTime = rhs.actionStartTime;
    this->actionStopTime = rhs.actionStopTime;
  } else {
    this->actionStopTime += rhs.getActionDuration();
  }

  const std::vector<SbNodeProfilingData> & src = PRIVATE(&rhs)->nodeData;
  std::vector<SbNodeProfilingData> & dst = PRIVATE(this)->nodeData;

  { // nodeData
    const int numsrcentries = (int)src.size();
    for (int c = 0; c < numsrcentries; ++c) {
      int matchidx = -1, parentidx = -1;
      findPath(src, dst, c, matchidx, parentidx);
      if (matchidx == -1) {
        SbNodeProfilingData data;
        data.node = src[c].node;
        data.childidx = src[c].childidx;
        data.parentidx = parentidx;
        data.nodetype = src[c].nodetype;
        data.nodename = src[c].nodename;
        dst.push_back(data);
        matchidx = (int)dst.size() - 1;
      }
      // accumulate data (something about this really doesn't make sense)
      dst[matchidx].traversaltime += src[c].traversaltime;
      dst[matchidx].memorysize += src[c].memorysize;
      dst[matchidx].texturesize += src[c].texturesize;
      dst[matchidx].traversalcount += src[c].traversalcount;
    }
  }

  { // nodeTypeData
    typedef std::map<SbProfilingNodeTypeKey, SbTypeProfilingData> maptype;
    maptype::const_iterator srctypeit = PRIVATE(&rhs)->nodeTypeData.begin();
    while (srctypeit != PRIVATE(&rhs)->nodeTypeData.end()) {
      maptype::iterator dsttypeit = PRIVATE(this)->nodeTypeData.find(srctypeit->first);
      if (dsttypeit != PRIVATE(this)->nodeTypeData.end()) {
        dsttypeit->second.totaltime += srctypeit->second.totaltime;
        dsttypeit->second.count += srctypeit->second.count;
        if (srctypeit->second.maximumtime > dsttypeit->second.maximumtime) {
          dsttypeit->second.maximumtime = srctypeit->second.maximumtime;
        }
      } else {
        // new type entry - copy data in
        PRIVATE(this)->nodeTypeData.insert(*srctypeit);
      }
      ++srctypeit;
    }
  }

  { // nodeNameData
    typedef std::map<SbProfilingNodeNameKey, SbNameProfilingData> maptype;
    maptype::const_iterator srctypeit = PRIVATE(&rhs)->nodeNameData.begin();
    while (srctypeit != PRIVATE(&rhs)->nodeNameData.end()) {
      maptype::iterator dsttypeit = PRIVATE(this)->nodeNameData.find(srctypeit->first);
      if (dsttypeit != PRIVATE(this)->nodeNameData.end()) {
        dsttypeit->second.totaltime += srctypeit->second.totaltime;
        dsttypeit->second.count += srctypeit->second.count;
        if (srctypeit->second.maximumtime > dsttypeit->second.maximumtime) {
          dsttypeit->second.maximumtime = srctypeit->second.maximumtime;
        }
      } else {
        // new type entry - copy data in
        PRIVATE(this)->nodeNameData.insert(*srctypeit);
      }
      ++srctypeit;
    }
  }

  assert(PRIVATE(this)->nodeData.size() >= PRIVATE(&rhs)->nodeData.size());
  assert(PRIVATE(this)->nodeTypeData.size() >= PRIVATE(&rhs)->nodeTypeData.size());
  assert(PRIVATE(this)->nodeNameData.size() >= PRIVATE(&rhs)->nodeNameData.size());

  return *this;
}

/*!
  Register which type of action we are recording statistics for.
*/

void
SbProfilingData::setActionType(SoType actiontype)
{
  this->actionType = actiontype;
}

/*!
  Return the action type set for this SbProfilingData.
*/

SoType
SbProfilingData::getActionType(void) const
{
  return this->actionType;
}

/*!
  Set traversal start time.
*/

void
SbProfilingData::setActionStartTime(SbTime starttime)
{
  this->actionStartTime = starttime;
}

/*!
  Return the action start time.
*/

SbTime
SbProfilingData::getActionStartTime(void) const
{
  return this->actionStartTime;
}

/*!
  Set traversal stop time.
*/

void
SbProfilingData::setActionStopTime(SbTime stoptime)
{
  this->actionStopTime = stoptime;
}

/*!
  Return the action stop time.
*/

SbTime
SbProfilingData::getActionStopTime(void) const
{
  return this->actionStopTime;
}

/*!
  Return the time the action has spent on the traversal that was profiled.
*/

SbTime
SbProfilingData::getActionDuration(void) const
{
  if ((this->actionStopTime == SbTime::zero()) &&
      (this->actionStartTime != SbTime::zero())) {
    // action still running... (?)
    return SbTime::getTimeOfDay() - this->actionStartTime;
  }
  return (this->actionStopTime - this->actionStartTime);
}

// *************************************************************************

/*
 * Check if the path matches the given index in the nodedata vector.
 */

SbBool
SbProfilingData::isPathMatch(const SoFullPath * fullpath, int pathlen, int idx)
{
  assert(pathlen > 0 && pathlen <= fullpath->getLength());
  while (pathlen > 0 && idx != -1) {
    SbProfilingNodeKey node =
      static_cast<SbProfilingNodeKey>(fullpath->getNode(pathlen-1));
    int childidx = fullpath->getIndex(pathlen-1);
    if (PRIVATE(this)->nodeData[idx].node != node) return FALSE;
    if (PRIVATE(this)->nodeData[idx].childidx != childidx) return FALSE;
    idx = PRIVATE(this)->nodeData[idx].parentidx;
    --pathlen;
  }
  if (pathlen == 0 && idx == -1) return TRUE;
  return FALSE;
}

// *************************************************************************

/*!
  Return the index of the tail node in the path.
  If node is not registered, add it and return that index.
*/

int
SbProfilingData::getIndex(const SoPath * path, SbBool create)
{
  const SoFullPath * fullpath = static_cast<const SoFullPath *>(path);
  if ((PRIVATE(this)->lastPathIndex != -1) &&
      isPathMatch(fullpath, fullpath->getLength(),
                  PRIVATE(this)->lastPathIndex)) {
    return PRIVATE(this)->lastPathIndex;
  }
  int idx = -1;
  if (create) {
    idx =  this->getIndexCreate(fullpath, fullpath->getLength());
  } else {
    idx = this->getIndexNoCreate(fullpath, fullpath->getLength());
  }
  if (idx != -1) { PRIVATE(this)->lastPathIndex = idx; }
  return idx;
}

/*!
  Return the index of the parent of the node entry at index \a idx.
  If entry is a root entry, -1 is returned.
*/

int
SbProfilingData::getParentIndex(int idx) const
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));
  return PRIVATE(this)->nodeData[idx].parentidx;
}

/*
 * Return the index of the tail node in the path ("tail" node at pathlen
 * position). If node is not registered, add it and return that index.
 */

int
SbProfilingData::getIndexCreate(const SoFullPath * fullpath, int COIN_UNUSED_ARG(pathlen))
{

  std::vector<int> lastentrypathindexes;
  int idx = (int)PRIVATE(this)->nodeData.size() - 1;
  while (idx != -1) {
    lastentrypathindexes.push_back(idx);
    idx = PRIVATE(this)->nodeData[idx].parentidx;
  }
  std::reverse(lastentrypathindexes.begin(), lastentrypathindexes.end());

  int samelength = 0;
  if (lastentrypathindexes.size() > 0) {
    const int pathlength =
      SbMin(fullpath->getLength(), (int) lastentrypathindexes.size());
    while (samelength < pathlength) {
      if ((PRIVATE(this)->nodeData[lastentrypathindexes[samelength]].node !=
           static_cast<SbProfilingNodeKey>(fullpath->getNode(samelength))) ||
          (PRIVATE(this)->nodeData[lastentrypathindexes[samelength]].childidx !=
           fullpath->getIndex(samelength))) {
        break;
      }
      ++samelength;
    }
  }

  if (samelength == 0) {
    // FIXME: need to check for the possibility of multiple roots
    // this is rooted in a new root - add it

    SbNodeProfilingData data;
    SoNode * rootnode = fullpath->getNode(0);
    assert(rootnode != NULL);
    data.node = static_cast<SbProfilingNodeKey>(rootnode);
    data.nodetype = static_cast<SbProfilingNodeTypeKey>(rootnode->getTypeId().getKey());
    data.nodename = static_cast<SbProfilingNodeNameKey>(rootnode->getName().getString());
    PRIVATE(this)->nodeData.push_back(data);

    ++samelength;
    lastentrypathindexes.clear();
    lastentrypathindexes.push_back((int)PRIVATE(this)->nodeData.size() - 1);
  }

  int pos = samelength;
  idx = lastentrypathindexes[pos-1];
  ++pos;
  while (pos <= fullpath->getLength()) {
    idx = this->getIndexForwardCreate(fullpath, pos, idx);
    ++pos;
  }

  return idx;
}

/*
 * This function looks for an existing path, and will not create the
 * entries if they do not exist.
 */

int
SbProfilingData::getIndexNoCreate(const SoPath * path, int COIN_UNUSED_ARG(pathlen)) const
{
  const SoFullPath * fullpath = static_cast<const SoFullPath *>(path);

  std::vector<int> lastentrypathindexes;
  int idx = (int)PRIVATE(this)->nodeData.size() - 1;
  while (idx != -1) {

    lastentrypathindexes.push_back(idx);
    idx = PRIVATE(this)->nodeData[idx].parentidx;

  }

  std::reverse(lastentrypathindexes.begin(), lastentrypathindexes.end());

  int samelength = 0;
  if (lastentrypathindexes.size() > 0) {
    const int pathlength =
      SbMin(fullpath->getLength(), (int) lastentrypathindexes.size());
    while (samelength < pathlength) {
      if ((PRIVATE(this)->nodeData[lastentrypathindexes[samelength]].node !=
           static_cast<SbProfilingNodeKey>(fullpath->getNode(samelength))) ||
          (PRIVATE(this)->nodeData[lastentrypathindexes[samelength]].childidx !=
           fullpath->getIndex(samelength))) {
        break;
      }
      ++samelength;
    }
  }

  if (samelength == 0) {
    // FIXME: get
    return -1;
  }

  int pos = samelength;
  idx = lastentrypathindexes[pos-1];
  ++pos;
  while (pos < fullpath->getLength() && idx != -1) {
    idx = this->getIndexForwardNoCreate(fullpath, pos, idx);
    ++pos;
  }

  return idx;
}

// *************************************************************************

/*
 * This function is used when you have a partial path leading up to
 * the parent of the tail ("tail" at pathlen). Then you can take some
 * shortcuts and make some assumptions that saves some time. This
 * function is intended used only as a subfunction of the getIndex()
 * function for reverse-finding/building the missing data-structure
 * from the given path.
 */

int
SbProfilingData::getIndexForwardCreate(const SoFullPath * fullpath, int pathlen, int parentidx)
{
  assert(parentidx != -1); // illegal usage
  assert(parentidx < static_cast<int>(PRIVATE(this)->nodeData.size()));
  assert(pathlen > 1);

  SbProfilingNodeKey parent =
    static_cast<SbProfilingNodeKey>(fullpath->getNode(pathlen - 2));
  int pidx = fullpath->getIndex(pathlen - 2);
  SoNode * tailnode = fullpath->getNode(pathlen - 1);
  SbProfilingNodeKey tail = static_cast<SbProfilingNodeKey>(tailnode);
  int tidx = fullpath->getIndex(pathlen - 1);

  assert(parent == PRIVATE(this)->nodeData[parentidx].node);
  assert(pidx == PRIVATE(this)->nodeData[parentidx].childidx);

  const int nodedatacount = (int)PRIVATE(this)->nodeData.size();
  for (int idx = parentidx + 1; idx < nodedatacount; ++idx) {
    if ((PRIVATE(this)->nodeData[idx].node == tail) &&
        (PRIVATE(this)->nodeData[idx].childidx == tidx)) { // found it!
      return idx;
    }
  }

  // entry not found - add entry and return new index
  SbNodeProfilingData data;
  data.node = tail;
  data.nodetype = static_cast<SbProfilingNodeTypeKey>(tailnode->getTypeId().getKey());
  data.nodename = static_cast<SbProfilingNodeNameKey>(tailnode->getName().getString());
  data.parentidx = parentidx;
  data.childidx = tidx;
  PRIVATE(this)->nodeData.push_back(data);

  return (int)PRIVATE(this)->nodeData.size() - 1;
}

/*
 *
 */

int
SbProfilingData::getIndexForwardNoCreate(const SoFullPath * fullpath, int pathlen, int parentidx) const
{
  assert(parentidx != -1); // illegal usage
  assert(pathlen > 1);

  SbProfilingNodeKey parent =
    static_cast<SbProfilingNodeKey>(fullpath->getNode(pathlen - 2));
  int pidx = fullpath->getIndex(pathlen - 2);
  SbProfilingNodeKey tail =
    static_cast<SbProfilingNodeKey>(fullpath->getNode(pathlen - 1));
  int tidx = fullpath->getIndex(pathlen - 1);

  assert(parent == PRIVATE(this)->nodeData[parentidx].node);
  assert(pidx == PRIVATE(this)->nodeData[parentidx].childidx);

  const int nodedatacount = (int)PRIVATE(this)->nodeData.size();
  for (int idx = parentidx + 1; idx < nodedatacount; ++idx) {
    if ((PRIVATE(this)->nodeData[idx].node == tail) &&
        (PRIVATE(this)->nodeData[idx].childidx == tidx)) { // found it!
      return idx;
    }
  }
  return -1;
}

// *************************************************************************

/*!
  This function calls the index version of setNodeTiming after having
  fetched the index.
*/
void
SbProfilingData::setNodeTiming(const SoPath * path, SbTime timing)
{
  assert(path);
  assert(path->getLength() > 0);
  assert(timing.getValue() >= 0.0);

  const int idx = this->getIndex(path);
  this->setNodeTiming(idx, timing);
}

/*!
  This method sets the timing for a node, as if it was new data to
  be registered. This means that counters of various types are
  implicitly incremented and similar things.  To avoid those
  side effects, use offsetNodeTiming, which leaves all the counters
  alone.

  \sa offsetNodeTiming
 */
void
SbProfilingData::setNodeTiming(int idx, SbTime timing)
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));
  assert(timing.getValue() >= 0.0);

  // 1) set for path (node)
  PRIVATE(this)->nodeData[idx].traversaltime = timing;
  PRIVATE(this)->nodeData[idx].traversalcount = 1;

  // 2) set for type
  SbProfilingNodeTypeKey typekey = PRIVATE(this)->nodeData[idx].nodetype;
  std::map<SbProfilingNodeTypeKey, SbTypeProfilingData>::iterator typeit =
    PRIVATE(this)->nodeTypeData.find(typekey);
  if (typeit != PRIVATE(this)->nodeTypeData.end()) {
    typeit->second.totaltime += timing;
    typeit->second.count += 1;
    if (typeit->second.maximumtime < timing) {
      typeit->second.maximumtime = timing;
    }
  } else {
    SbTypeProfilingData data;
    data.totaltime = timing;
    data.maximumtime = timing;
    data.count = 1;
    PRIVATE(this)->nodeTypeData.insert(std::pair<SbProfilingNodeTypeKey, SbTypeProfilingData>(typekey, data));
  }

  // 3) set for name

  // should we include timings at all named nodes up through the path
  // all the way to the root?
  const bool inclusive = false;

  int parentidx = idx;
  while (parentidx != -1) {
    SbProfilingNodeNameKey namekey =
      PRIVATE(this)->nodeData[parentidx].nodename;
    if (namekey != SbName::empty().getString()) {
      std::map<SbProfilingNodeNameKey, SbNameProfilingData>::iterator nameit =
        PRIVATE(this)->nodeNameData.find(namekey);
      if (nameit != PRIVATE(this)->nodeNameData.end()) {
        nameit->second.totaltime += timing;
        if (idx == parentidx) { // entry at named node level
          // DISABLED: we won't know the "unit" time for this aggregate
          // time-sum so we can't give maximum unit time. we'll need to
          // store total-time from preTraversal() to figure it
          // out i think. 20080304 larsa
          //if (nameit->second.maximumtime < timing) {
          //  nameit->second.maximumtime = timing;
          //}
          nameit->second.count += 1;
        }
      } else {
        SbNameProfilingData data;
        data.totaltime = timing;
        //data.maximumtime = timing;
        if (idx == parentidx) {
          data.count += 1;
        } else {
          // FIXME: implement proper action
        }
        PRIVATE(this)->nodeNameData.insert(std::pair<SbProfilingNodeNameKey, SbNameProfilingData>(namekey, data));
      }
      if (!inclusive) break;
    }
    parentidx = PRIVATE(this)->nodeData[parentidx].parentidx;
  }
}

/*!
  This function will adjust node timings without touching traversal counters.
*/
void
SbProfilingData::preOffsetNodeTiming(int idx, SbTime timing)
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));
  // 1) adjust for path (node)
  PRIVATE(this)->nodeData[idx].traversaltime += timing;

#if 0
  // 2) adjust for type
  SbProfilingNodeKey tailnode = PRIVATE(this)->nodeData[idx].node;
  SbProfilingNodeTypeKey typekey = PRIVATE(this)->nodeData[idx].nodetype;
  std::map<SbProfilingNodeTypeKey, SbTypeProfilingData>::iterator typeit =
    PRIVATE(this)->nodeTypeData.find(typekey);
  if (typeit != PRIVATE(this)->nodeTypeData.end()) {
    typeit->second.totaltime += timing;
  } else {
    SbTypeProfilingData data;
    data.totaltime = timing;
    PRIVATE(this)->nodeTypeData.insert(std::pair<SbProfilingNodeTypeKey, SbTypeProfilingData>(typekey, data));
  }

  // 3) adjust for name

  // should we include timings at all named nodes up through the path
  // all the way to the root?
  const bool inclusive = false;

  int parentidx = idx;
  while (parentidx != -1) {
    SbProfilingNodeNameKey namekey = PRIVATE(this)->nodeData[parentidx].nodename;
    if (namekey != SbName::empty().getString()) {
      std::map<SbProfilingNodeNameKey, SbNameProfilingData>::iterator nameit =
        PRIVATE(this)->nodeNameData.find(namekey);
      if (nameit != PRIVATE(this)->nodeNameData.end()) {
        nameit->second.totaltime += timing;
        if (idx == parentidx) { // entry at named node level
          // DISABLED: we won't know the "unit" time for this aggregate
          // time-sum so we can't give maximum unit time. we'll need to
          // store total-time from preTraversal() to figure it
          // out i think. 20080304 larsa
          //if (nameit->second.maximumtime < timing) {
          //  nameit->second.maximumtime = timing;
          //}
        }
      } else {
        SbNameProfilingData data;
        data.totaltime = timing;
        PRIVATE(this)->nodeNameData.insert(std::pair<SbProfilingNodeNameKey, SbNameProfilingData>(namekey, data));
      }
      if (!inclusive) break;
    }
    parentidx = PRIVATE(this)->nodeData[parentidx].parentidx;
  }
#endif
}

/*!
  Returns the timing for a node.
*/
SbTime
SbProfilingData::getNodeTiming(const SoPath * path, unsigned int flags) const
{
  const SoFullPath * fullpath = static_cast<const SoFullPath *>(path);
  int idx = this->getIndexNoCreate(fullpath, fullpath->getLength());
  return this->getNodeTiming(idx, flags);
}

/*!
*/

SbTime
SbProfilingData::getNodeTiming(int idx, unsigned int flags) const
{
  if (idx == -1) return SbTime::zero();
  SbTime sum = PRIVATE(this)->nodeData[idx].traversaltime;
  if ((flags & INCLUDE_CHILDREN) != 0) {
    // FIXME: find all children of the node ad add to the sum
  }
  return sum;
}

/*!
*/

void
SbProfilingData::setNodeFootprint(const SoPath * path, FootprintType footprinttype, size_t footprint)
{
  assert(path);
  assert(static_cast<const SoFullPath *>(path)->getLength() > 0);

  const SoFullPath * fullpath = static_cast<const SoFullPath *>(path);
  const int idx = this->getIndexCreate(fullpath, fullpath->getLength());
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));

  this->setNodeFootprint(idx, footprinttype, footprint);
}

/*!
*/

void
SbProfilingData::setNodeFootprint(int idx, FootprintType footprinttype, size_t footprint)
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));

  switch (footprinttype) {
  case MEMORY_SIZE:
    PRIVATE(this)->nodeData[idx].memorysize = footprint;
    break;
  case VIDEO_MEMORY_SIZE:
    PRIVATE(this)->nodeData[idx].texturesize = footprint;
    break;
  default:
    break;
  }
}

/*!
*/

size_t
SbProfilingData::getNodeFootprint(const SoPath * path, FootprintType footprinttype, unsigned int flags) const
{
  const SoFullPath * fullpath = static_cast<const SoFullPath *>(path);
  const int idx = this->getIndexNoCreate(fullpath, fullpath->getLength());
  if (idx == -1) return 0;

  return this->getNodeFootprint(idx, footprinttype, flags);
}

/*!
*/

size_t
SbProfilingData::getNodeFootprint(int idx, FootprintType footprinttype, unsigned int flags) const
{
  assert(idx >= 0);
  size_t footprint = 0;
  switch (footprinttype) {
  case MEMORY_SIZE:
    footprint = PRIVATE(this)->nodeData[idx].memorysize;
    break;
  case VIDEO_MEMORY_SIZE:
    footprint = PRIVATE(this)->nodeData[idx].texturesize;
    break;
  default:
    break;
  }
  if ((flags & INCLUDE_CHILDREN) != 0) {
    // FIXME: add children data to footprint
  }
  return footprint;
}

/*!
*/

void
SbProfilingData::setNodeFlag(const SoPath * path, NodeFlag flag, SbBool on)
{
  assert(path);
  assert(static_cast<const SoFullPath *>(path)->getLength() > 0);

  const SoFullPath * fullpath = static_cast<const SoFullPath *>(path);
  const int idx = this->getIndexCreate(fullpath, fullpath->getLength());
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));
  this->setNodeFlag(idx, flag, on);
}

/*!
*/

void
SbProfilingData::setNodeFlag(int idx, NodeFlag flag, SbBool on)
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));

  switch (flag) {
  case GL_CACHED_FLAG:
    PRIVATE(this)->nodeData[idx].flags.glcached = on ? 1 : 0;
    break;
  case CULLED_FLAG:
    PRIVATE(this)->nodeData[idx].flags.culled = on ? 1 : 0;
    break;
  default:
    break;
  }
}

/*!
*/

SbBool
SbProfilingData::getNodeFlag(const SoPath * path, NodeFlag flag) const
{
  const SoFullPath * fullpath = static_cast<const SoFullPath *>(path);
  const int idx = this->getIndexNoCreate(fullpath, fullpath->getLength());
  if (idx == -1) return 0;
  return this->getNodeFlag(idx, flag);
}

/*!
*/

SbBool
SbProfilingData::getNodeFlag(int idx, NodeFlag flag) const
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));

  switch (flag) {
  case GL_CACHED_FLAG:
    return PRIVATE(this)->nodeData[idx].flags.glcached ? TRUE : FALSE;
    break;
  case CULLED_FLAG:
    return PRIVATE(this)->nodeData[idx].flags.culled ? TRUE : FALSE;
    break;
  default:
    break;
  }
  return FALSE;
}

/*!
*/
SoType
SbProfilingData::getNodeType(int idx) const
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));
  return SoType::fromKey(PRIVATE(this)->nodeData[idx].nodetype);
}

/*!
*/
SbName
SbProfilingData::getNodeName(int idx) const
{
  assert(idx >= 0 && idx < static_cast<int>(PRIVATE(this)->nodeData.size()));
  return SbName(PRIVATE(this)->nodeData[idx].nodename);
}

/*!
 */

int
SbProfilingData::getLongestNameLength(void) const
{
  size_t longest = 0;
  std::map<SbProfilingNodeNameKey, SbNameProfilingData>::const_iterator it =
    PRIVATE(this)->nodeNameData.begin();
  while (it != PRIVATE(this)->nodeNameData.end()) {
    const size_t len = strlen(it->first);
    if (len > longest) longest = len;
    ++it;
  }
  return (int)longest;
}

/*!
 */

int
SbProfilingData::getLongestTypeNameLength(void) const
{
  size_t longest = 0;
  std::map<SbProfilingNodeTypeKey, SbTypeProfilingData>::const_iterator it =
    PRIVATE(this)->nodeTypeData.begin();
  while (it != PRIVATE(this)->nodeTypeData.end()) {
    SoType type = SoType::fromKey(it->first);
    const size_t len = strlen(type.getName().getString());
    if (len > longest) longest = len;
    ++it;
  }
  return (int)longest;
}

/*!
 */

int
SbProfilingData::getNumNodeEntries(void) const
{
  return (int)PRIVATE(this)->nodeData.size();
}

/*!
*/
void
SbProfilingData::reportAll(SbProfilingDataCB * callback, void * userdata) const
{
  //std::vector<SbNodeProfilingData>::const_iterator it = PRIVATE(this)->nodeData.begin();
  const int numnodedata = (int)PRIVATE(this)->nodeData.size();
  for (int idx = 0; idx < numnodedata; ++idx) {
    SbList<SoNode *> pointers;
    SbList<int> indices;

    for (int nodeidx = idx;
         nodeidx != -1;
         nodeidx = PRIVATE(this)->nodeData[nodeidx].parentidx) {
      pointers.append(static_cast<SoNode *>(PRIVATE(this)->nodeData[nodeidx].node));
      indices.append(PRIVATE(this)->nodeData[nodeidx].childidx);
    }

    // reverse lists
    const int pathlen = pointers.getLength();
    for (int c = 0; c < (pathlen / 2); ++c) {
      SoNode * tempnode = pointers[c];
      pointers[c] = pointers[pathlen-1-c];
      pointers[pathlen-1-c] = tempnode;
      int tempidx = indices[c];
      indices[c] = indices[pathlen-1-c];
      indices[pathlen-1-c] = tempidx;
    }

    callback(userdata, *this, pointers, indices, idx);
  }
}

/*!
  Returns the amount of memory allocated for this data structure.
*/
size_t
SbProfilingData::getProfilingDataSize(void) const
{
  size_t nodestatsize =
    PRIVATE(this)->nodeData.capacity() * sizeof(SbNodeProfilingData);
  size_t typestatsize =
    PRIVATE(this)->nodeTypeData.size() * sizeof(SbTypeProfilingData);
  size_t namestatsize =
    PRIVATE(this)->nodeNameData.size() * sizeof(SbNameProfilingData);
  return nodestatsize + typestatsize + namestatsize + sizeof(SbProfilingDataP);
}

/*!
*/

void
SbProfilingData::getStatsForTypesKeyList(SbList<SbProfilingNodeTypeKey> & keys_out) const
{
  keys_out.truncate(0);
  std::map<SbProfilingNodeTypeKey, SbTypeProfilingData>::const_iterator it =
    PRIVATE(this)->nodeTypeData.begin();
  while (it != PRIVATE(this)->nodeTypeData.end()) {
    keys_out.append(it->first);
    ++it;
  }
}

/*!
*/

void
SbProfilingData::getStatsForType(SbProfilingNodeTypeKey type,
                                 SbTime & totaltime, SbTime & maxtime,
                                 uint32_t & count) const
{
  std::map<SbProfilingNodeTypeKey, SbTypeProfilingData>::const_iterator it =
    PRIVATE(this)->nodeTypeData.find(type);
  assert(it != PRIVATE(this)->nodeTypeData.end());
  totaltime = it->second.totaltime;
  maxtime = it->second.maximumtime;
  count = it->second.count;
}

// *************************************************************************

/*!
*/

void
SbProfilingData::getStatsForNamesKeyList(SbList<SbProfilingNodeNameKey> & keys_out) const
{
  keys_out.truncate(0);
  std::map<SbProfilingNodeNameKey, SbNameProfilingData>::const_iterator it =
    PRIVATE(this)->nodeNameData.begin();
  while (it != PRIVATE(this)->nodeNameData.end()) {
    keys_out.append(it->first);
    ++it;
  }
}

/*!
*/

void
SbProfilingData::getStatsForName(SbProfilingNodeNameKey name,
                                 SbTime & totaltime, SbTime & maxtime,
                                 uint32_t & count) const
{
  std::map<SbProfilingNodeNameKey, SbNameProfilingData>::const_iterator it =
    PRIVATE(this)->nodeNameData.find(name);
  assert(it != PRIVATE(this)->nodeNameData.end());
  totaltime = it->second.totaltime;
  // FIXME: maximum time for when grouping into name is not yet supported
  maxtime = it->second.maximumtime;
  count = it->second.count;
}

// *************************************************************************

int
SbProfilingData::operator == (const SbProfilingData & rhs) const
{
  if (this->actionType != rhs.actionType) return FALSE;
  if (this->actionStartTime != rhs.actionStartTime) return FALSE;
  if (this->actionStopTime != rhs.actionStopTime) return FALSE;
  if (PRIVATE(this)->nodeData.size() != PRIVATE(&rhs)->nodeData.size())
    return FALSE;

  for (int c = (int)PRIVATE(this)->nodeData.size() - 1; c >= 0; --c) {
    if (PRIVATE(this)->nodeData[c] != PRIVATE(&rhs)->nodeData[c])
      return FALSE;
  }

  // NOTE: the type and name info maps are not checked, because they
  // are just aggregates of the nodedata records and would be equal if
  // the node data is.

  return TRUE;
}

int
SbProfilingData::operator != (const SbProfilingData & rhs) const
{
  return !((*this) == rhs);
}

// *************************************************************************

#undef PRIVATE
