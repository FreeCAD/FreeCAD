#ifndef COIN_SOCONVERTALL_H
#define COIN_SOCONVERTALL_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

#include <Inventor/engines/SoFieldConverter.h>
#include <Inventor/engines/SoEngineOutput.h>

// *************************************************************************

class SoConvertAll : public SoFieldConverter {
  typedef SoFieldConverter inherited;

  // SoConvertAll doesn't have a createInstance() method (because it
  // doesn't have a default constructor), so use the ABSTRACT macros.
  SO_ENGINE_ABSTRACT_HEADER(SoConvertAll);

public:
  static void initClass(void);
  SoConvertAll(const SoType from, const SoType to);

protected:
  virtual ~SoConvertAll(void);
  virtual void evaluate(void);
  virtual SoField * getInput(SoType type);
  virtual SoEngineOutput * getOutput(SoType type);

private:
  typedef void converter_func(SoField * from, SoField * to);
  converter_func * convertvalue;

  SoField * input;
  SoEngineOutput output;

  SoFieldData * inputdata_instance;
  SoEngineOutputData * outputdata_instance;
};

#endif // !COIN_SOCONVERTALL_H
