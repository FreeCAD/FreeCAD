#ifndef COIN_SOSCROLLINGGRAPHKIT_H
#define COIN_SOSCROLLINGGRAPHKIT_H

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

#include <Inventor/tools/SbPimplPtr.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoMFColor.h>
#include <Inventor/fields/SoMFName.h>

class SoSensor;
class SoScrollingGraphKitP;

class SoScrollingGraphKit : public SoBaseKit {
  typedef SoBaseKit inherited;
  SO_KIT_HEADER(SoScrollingGraphKit);
  SO_KIT_CATALOG_ENTRY_HEADER(scene);

public:
  static void initClass(void);

  SoScrollingGraphKit(void);

  enum GraphicsType {
    LINES,
    STACKED_BARS,
    DEFAULT_GRAPHICS = STACKED_BARS
  };

  enum RangeType {
    ABSOLUTE_ACCUMULATIVE,
    //ABSOLUTE_OVERWRITE,
    //RELATIVE_ACCUMULATIVE,
    //RELATIVE_OVERWRITE,
    DEFAULT_RANGETYPE = ABSOLUTE_ACCUMULATIVE
  };

  // config
  SoSFEnum graphicsType;
  SoSFEnum rangeType;
  SoSFTime seconds; // seconds to pass over graph area (20)
  SoMFColor colors; // rotating color list

  // geometry
  SoSFVec3f viewportSize; // input
  SoSFVec3f position;     // input
  SoSFVec3f size;         // input

  // the dynamic inputs
  SoMFName addKeys;
  SoMFFloat addValues; // input

protected:
  virtual ~SoScrollingGraphKit(void);

  static void addValuesCB(void * closure, SoSensor * sensor);

private:
  SbPimplPtr<SoScrollingGraphKitP> pimpl;

  // NOT IMPLEMENTED
  SoScrollingGraphKit(const SoScrollingGraphKit &);
  SoScrollingGraphKit & operator = (const SoScrollingGraphKit &);

}; // SoScrollingGraphKit

#endif // !COIN_SOSCROLLINGGRAPHKIT_H
