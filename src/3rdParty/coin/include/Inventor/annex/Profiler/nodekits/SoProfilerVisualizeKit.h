#ifndef COIN_SOPROFILERVISUALIZEKIT
#define COIN_SOPROFILERVISUALIZEKIT

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

#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFTrigger.h>
#include <Inventor/fields/SoMFNode.h>
#include <Inventor/tools/SbPimplPtr.h>

struct SoProfilerVisualizeKitP;

class COIN_DLL_API SoProfilerVisualizeKit : public SoBaseKit {
  typedef SoBaseKit inherited;
  SO_KIT_HEADER(SoProfilerVisualizeKit);
  SO_KIT_CATALOG_ENTRY_HEADER(top);
  SO_KIT_CATALOG_ENTRY_HEADER(pretree);
  SO_KIT_CATALOG_ENTRY_HEADER(visualtree);

public:
  static void initClass(void);
  SoProfilerVisualizeKit(void);

  /// Set this to be the root of the scenegraph we visualize.
  SoSFNode root;

  /// Set this to be the stats field.
  SoSFTrigger statsTrigger;
  SoSFNode stats;

  // Fields picked up from SoProfilerStats follow here.
  SoMFNode separatorsWithGLCaches;

protected:
  virtual ~SoProfilerVisualizeKit(void);

private:
  /// NOT IMPLEMENTED
  SoProfilerVisualizeKit(const SoProfilerVisualizeKit&);
  /// NOT IMPLEMENTED
  SoProfilerVisualizeKit &operator=(const SoProfilerVisualizeKit&);
  SbPimplPtr<SoProfilerVisualizeKitP> pimpl;
};

#endif //!COIN_SOPROFILERVISUALIZEKIT
