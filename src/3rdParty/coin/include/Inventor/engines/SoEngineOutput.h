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

#ifndef COIN_SOENGINEOUTPUT_H
#define COIN_SOENGINEOUTPUT_H

#include <Inventor/SoType.h>
#include <Inventor/lists/SoFieldList.h>
#include <Inventor/lists/SbList.h>

class SoNotList;
class SoFieldContainer;
class SoEngine;
class SoNodeEngine;

class COIN_DLL_API SoEngineOutput {
public:
  SoEngineOutput(void);
  virtual ~SoEngineOutput(void);

  SoType getConnectionType(void) const;
  int getForwardConnections(SoFieldList & fl) const;
  void enable(const SbBool flag);
  SbBool isEnabled(void) const;
  SoEngine * getContainer(void) const;
  SoNodeEngine * getNodeContainer(void) const;
  SbBool isNodeEngineOutput(void) const;

  void setContainer(SoEngine * engine);
  void setNodeContainer(SoNodeEngine * nodeengine);
  void addConnection(SoField * f);
  void removeConnection(SoField * f);
  int getNumConnections(void) const;
  SoField * operator[](int i) const;

  void prepareToWrite(void) const;
  void doneWriting(void) const;

  void touchSlaves(SoNotList * nl, SbBool donotify);

  SoFieldContainer * getFieldContainer(void);

private:
  SbBool enabled;
  SoEngine * container; // FIXME: change to SoFieldContainer pointer
  SoFieldList slaves;
  SbList<SbBool> fieldnotiflist;
};

#endif // !COIN_SOENGINEOUTPUT_H
