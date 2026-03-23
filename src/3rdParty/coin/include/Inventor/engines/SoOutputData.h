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

#ifndef COIN_OUTPUTDATA_H
#define COIN_OUTPUTDATA_H

#include <Inventor/SbBasic.h>
#include <Inventor/lists/SbList.h>

class SoEngine;
class SoNodeEngine;
class SoFieldContainer;
class SoEngineOutput;
class SbName;
class SoInput;
class SoOutput;
class SoOutputDataEntry;
class SoType;

class COIN_DLL_API SoEngineOutputData {
public:
  SoEngineOutputData(void);
  SoEngineOutputData(const SoEngineOutputData * data);
  SoEngineOutputData(int approxnum);
  ~SoEngineOutputData(void);

  void addOutput(const SoEngine * base, const char *name,
                 const SoEngineOutput * output, SoType type);
  void addOutput(const SoNodeEngine * base, const char *name,
                 const SoEngineOutput * output, SoType type);

  int getNumOutputs(void) const;
  const SbName & getOutputName(int index) const;
  SoEngineOutput * getOutput(const SoEngine * engine, int index) const;
  SoEngineOutput * getOutput(const SoNodeEngine * engine, int index) const;
  int getIndex(const SoEngine * engine, const SoEngineOutput * output) const;
  int getIndex(const SoNodeEngine * engine, const SoEngineOutput * output) const;
  const SoType & getType(int index) const;
  SbBool readDescriptions(SoInput * input, SoEngine * engine) const;
  void writeDescriptions(SoOutput * out, SoEngine * engine) const;

private:
  SbBool hasOutput(const char * name) const;
  void addOutputInternal(const SoFieldContainer * base, const char *name,
                         const SoEngineOutput * output, SoType type);
  SoEngineOutput * getOutputInternal(const SoFieldContainer * base, int index) const;
  int getIndexInternal(const SoFieldContainer * base, const SoEngineOutput * output) const;

  SbList <SoOutputDataEntry*> outputlist;
};

#endif // !COIN_OUTPUTDATA_H
