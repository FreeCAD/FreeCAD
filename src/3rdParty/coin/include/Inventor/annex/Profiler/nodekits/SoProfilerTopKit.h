#ifndef COIN_SOPROFILERTOPKIT
#define COIN_SOPROFILERTOPKIT

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

#include <Inventor/annex/Profiler/nodekits/SoProfilerOverlayKit.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/tools/SbPimplPtr.h>

class SoProfilerTopKitP;

class COIN_DLL_API SoProfilerTopKit : public SoProfilerOverlayKit {
  typedef SoProfilerOverlayKit inherited;
  SO_KIT_HEADER(SoProfilerTopKit);
  SO_KIT_CATALOG_ENTRY_HEADER(textSep);
  SO_KIT_CATALOG_ENTRY_HEADER(color);
  SO_KIT_CATALOG_ENTRY_HEADER(translation);
  SO_KIT_CATALOG_ENTRY_HEADER(text);
  SO_KIT_CATALOG_ENTRY_HEADER(graph);

public:
  static void initClass(void);
  SoProfilerTopKit(void);

  SoSFColor txtColor;
  SoSFInt32 lines;

  SoSFVec2f topKitSize;    // output set from internal parts
  SoSFVec3f position;      // input set from SoProfilerOverlayKit

protected:
  virtual ~SoProfilerTopKit(void);

private:
  SbPimplPtr<SoProfilerTopKitP> pimpl;
  friend class SoProfilerTopKitP;

  SoProfilerTopKit(const SoProfilerTopKit & rhs);
  SoProfilerTopKit & operator = (const SoProfilerTopKit & rhs);

}; // SoProfilerTopKit

#endif //!COIN_SOPROFILERTOPKIT
