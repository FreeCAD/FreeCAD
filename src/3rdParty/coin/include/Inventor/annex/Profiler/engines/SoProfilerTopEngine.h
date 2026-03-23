#ifndef COIN_SOPROFILERTOPENGINE_H
#define COIN_SOPROFILERTOPENGINE_H

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

#include <Inventor/engines/SoSubEngine.h>
#include <Inventor/engines/SoEngine.h>
#include <Inventor/engines/SoEngineOutput.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoMFName.h>
#include <Inventor/fields/SoMFEnum.h>
#include <Inventor/fields/SoMFTime.h>
#include <Inventor/fields/SoMFUInt32.h>
#include <Inventor/tools/SbPimplPtr.h>

class SoProfilerTopEngineP;

class COIN_DLL_API SoProfilerTopEngine : public SoEngine {
  typedef SoEngine inherited;
  SO_ENGINE_HEADER(SoProfilerTopEngine);

public:
  static void initClass(void);
  SoProfilerTopEngine(void);

  enum Column {
    NAME,
    COUNT,
    TIME_SECS,
    TIME_SECS_MAX,
    TIME_SECS_AVG,
    TIME_MSECS,
    TIME_MSECS_MAX,
    TIME_MSECS_AVG,
    TIME_PERCENT,
    TIME_PERCENT_MAX,
    TIME_PERCENT_AVG
  };

  enum SortOrder {
    TIME_DEC,
    TIME_MAX_DEC,
    TIME_AVG_DEC,
    COUNT_DEC_TIME_DEC,
    COUNT_DEC_TIME_MAX_DEC,
    COUNT_DEC_TIME_AVG_DEC,
    ALPHANUMERIC_INC
  };

  // inputs
  SoMFName statisticsNames;
  SoMFTime statisticsTimings;
  SoMFTime statisticsTimingsMax;
  SoMFUInt32 statisticsCounts;

  // output-related
  SoMFEnum columns;   // [ NAME, COUNT, TIME_MSECS, TIME_PERCENT ]
  SoSFEnum sortOrder; // TIME_DEC
  SoSFInt32 maxLines; // 16
  SoSFFloat decay;    // 0.0  ([0.0 - 1.0>)

  // result
  SoEngineOutput prettyText; // SoMFString

protected:
  virtual ~SoProfilerTopEngine(void);

  virtual void evaluate(void);

private:
  SbPimplPtr<SoProfilerTopEngineP> pimpl;

  SoProfilerTopEngine(const SoProfilerTopEngine & rhs); // disable
  SoProfilerTopEngine & operator = (const SoProfilerTopEngine & rhs); // disable

}; // SoProfilerTopEngine

#endif // !COIN_SOPROFILERTOPENGINE_H
