#ifndef COIN_SBPROFILINGDATA_H
#define COIN_SBPROFILINGDATA_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/SbTime.h>
#include <Inventor/SoType.h>
#include <Inventor/SbName.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/tools/SbPimplPtr.h>

class SoNode;
class SoPath;
class SoFullPath;
class SbProfilingDataP;

typedef void * SbProfilingNodeKey; // void since it should not be dereferenced
typedef int16_t SbProfilingNodeTypeKey;
typedef const char * SbProfilingNodeNameKey;

class COIN_DLL_API SbProfilingData {
public:
  SbProfilingData(void);
  SbProfilingData(const SbProfilingData & rhs);
  ~SbProfilingData(void);

  void setActionType(SoType actiontype);
  SoType getActionType(void) const;

  void setActionStartTime(SbTime starttime);
  SbTime getActionStartTime(void) const;
  void setActionStopTime(SbTime stoptime);
  SbTime getActionStopTime(void) const;
  SbTime getActionDuration(void) const;

  // profiling setters
  enum FootprintType {
    MEMORY_SIZE,
    VIDEO_MEMORY_SIZE
  };

  enum NodeFlag {
    GL_CACHED_FLAG,
    CULLED_FLAG
  };

  enum NodeDataQueryFlags {
    INCLUDE_CHILDREN = 0x01
  };

  void setNodeTiming(const SoPath * path, SbTime timing);
  void setNodeTiming(int idx, SbTime timing);
  void preOffsetNodeTiming(int idx, SbTime timing);
  SbTime getNodeTiming(const SoPath * path, unsigned int queryflags = 0) const;
  SbTime getNodeTiming(int idx, unsigned int queryflags = 0) const;

  void setNodeFootprint(const SoPath * path, FootprintType type,
                        size_t footprint);
  void setNodeFootprint(int idx, FootprintType type, size_t footprint);
  size_t getNodeFootprint(const SoPath * path, FootprintType type,
                          unsigned int queryflags = 0) const;
  size_t getNodeFootprint(int idx, FootprintType type,
                          unsigned int queryflags = 0) const;

  void setNodeFlag(const SoPath * path, NodeFlag flag, SbBool on);
  void setNodeFlag(int idx, NodeFlag flag, SbBool on);
  SbBool getNodeFlag(const SoPath * path, NodeFlag flag) const;
  SbBool getNodeFlag(int idx, NodeFlag flag) const;

  int getIndex(const SoPath * path, SbBool create = FALSE);
  int getParentIndex(int idx) const;

  SoType getNodeType(int idx) const;
  SbName getNodeName(int idx) const;

  int getLongestNameLength(void) const;
  int getLongestTypeNameLength(void) const;

  int getNumNodeEntries(void) const;

  typedef void SbProfilingDataCB(void * userdata, const SbProfilingData & data, const SbList<SoNode *> & pointers, SbList<int> & childindices, int idx);
  void reportAll(SbProfilingDataCB * callback, void * userdata) const;

  // read out pre-categorized data
  void getStatsForTypesKeyList(SbList<SbProfilingNodeTypeKey> & keys_out) const;
  void getStatsForType(SbProfilingNodeTypeKey type,
                       SbTime & total, SbTime & max, uint32_t & count) const;

  void getStatsForNamesKeyList(SbList<SbProfilingNodeNameKey> & keys_out) const;
  void getStatsForName(SbProfilingNodeNameKey name,
                       SbTime & total, SbTime & max, uint32_t & count) const;

  // statistics management
  void reset(void);

  SbProfilingData & operator = (const SbProfilingData & rhs);
  SbProfilingData & operator += (const SbProfilingData & rhs);

  int operator == (const SbProfilingData & rhs) const;
  int operator != (const SbProfilingData & rhs) const;


  // debug - return profiling data overhead
  size_t getProfilingDataSize(void) const;

protected:
  SoType actionType;
  SbTime actionStartTime;
  SbTime actionStopTime;

private:
  SbPimplPtr<SbProfilingDataP> pimpl;

  void constructorInit(void);

  SbBool isPathMatch(const SoFullPath * path, int pathlen, int idx);

  int getIndexCreate(const SoFullPath * path, int pathlen);
  int getIndexNoCreate(const SoPath * path, int pathlen) const;
  int getIndexForwardCreate(const SoFullPath * path, int pathlen,
                            int parentindex);
  int getIndexForwardNoCreate(const SoFullPath * path, int pathlen,
                              int parentindex) const;

}; // SbProfilingData

#endif // !COIN_SBPROFILINGDATA_H
