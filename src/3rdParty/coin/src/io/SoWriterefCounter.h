#ifndef COIN_SOWRITEREFCOUNTER_H
#define COIN_SOWRITEREFCOUNTER_H

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

class SoWriterefCounterP;
class SoOutput;
class SoBase;
class SbString;

#ifndef COIN_INTERNAL
#error this is a private header file
#endif

#include <Inventor/SbBasic.h>
#include <Inventor/SbName.h>

class SoWriterefCounter {
public:
  static void initClass(void);
  
  static void setInstancePrefix(const SbString & s);
  
  static void create(SoOutput * out, SoOutput * copyfrom);
  static void destruct(SoOutput * out);

  static SoWriterefCounter * instance(SoOutput * out);

  SbBool shouldWrite(const SoBase * base) const;
  SbName getWriteName(const SoBase * base) const;
  SbBool hasMultipleWriteRefs(const SoBase * base) const;
  int getWriteref(const SoBase * base) const;
  void setWriteref(const SoBase * base, const int ref);
  void removeWriteref(const SoBase * base);
  void decrementWriteref(const SoBase * base);
  
  SbBool isInGraph(const SoBase * base) const;
  void setInGraph(const SoBase * base, const SbBool ingraph);
  void debugCleanup(void);
  
  enum RefId {
    // Reference id if we don't need to add a suffix to the node name
    NOSUFFIX = -2,
    // Reference id if no DEF instance of a node is written yet
    FIRSTWRITE = -1
  };

  int addReference(const SoBase * base);
  int findReference(const SoBase * base) const;
  void setReference(const SoBase * base, int refid);
  void removeSoBase2IdRef(const SoBase * base);

  static SbBool debugWriterefs(void);

protected:
  SoWriterefCounter(SoOutput * out, SoOutput * copyfrom);
  ~SoWriterefCounter();

private:
  void cleanup(void);

  SoWriterefCounterP * pimpl;

};

#endif // COIN_SOWRITERREFCOUNTER_H
