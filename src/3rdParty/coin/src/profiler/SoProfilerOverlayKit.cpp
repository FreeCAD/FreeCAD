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
  \class SoProfilerOverlayKit SoProfilerOverlayKit.h Inventor/annex/Profiler/nodekits/SoProfilerOverlayKit.h
  \brief The SoProfilerOverlayKit element class is yet to be documented.

  \ingroup coin_profiler
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_NODEKITS

#include <Inventor/annex/Profiler/nodekits/SoProfilerOverlayKit.h>
#include "coindefs.h"

#include <Inventor/system/gl.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/annex/Profiler/nodes/SoProfilerStats.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#include "nodekits/SoSubKitP.h"

namespace {
  void enableDepthTest(void * COIN_UNUSED_ARG(userdata), SoAction * COIN_UNUSED_ARG(action))
  {
    glEnable(GL_DEPTH_TEST);
  }

  void disableDepthTest(void * COIN_UNUSED_ARG(userdata), SoAction * COIN_UNUSED_ARG(action))
  {
    glDisable(GL_DEPTH_TEST);
  }

  void grabViewportInfo(void * userdata, SoAction * action)
  {
    SoState * state = action->getState();
    const int eltindex = SoViewportRegionElement::getClassStackIndex();
    if (state->isElementEnabled(eltindex)) {
      const SbViewportRegion & vp = SoViewportRegionElement::get(state);
      SoProfilerOverlayKit * kit = static_cast<SoProfilerOverlayKit *>(userdata);
      SbVec2s pixels = vp.getViewportSizePixels();
      kit->viewportSize = SbVec3f(float(pixels[0]), float(pixels[1]), 0.0f);
    }
  }
};

struct SoProfilerOverlayKitP
{
};

#define PRIVATE(obj) ((obj)->pimpl)

SO_KIT_SOURCE(SoProfilerOverlayKit);

// Doc in superclass.
void
SoProfilerOverlayKit::initClass(void)
{
  SO_KIT_INTERNAL_INIT_CLASS(SoProfilerOverlayKit, SO_FROM_COIN_3_0);
}

/*!
  Constructor.
 */
SoProfilerOverlayKit::SoProfilerOverlayKit(void)
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoProfilerOverlayKit);
  SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, TRUE, this, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(profilingStats, SoProfilerStats, FALSE,
                           topSeparator, viewportInfo, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(viewportInfo, SoCallback, TRUE, topSeparator,
                           overlayCamera, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(overlayCamera, SoOrthographicCamera, TRUE,
                           topSeparator, depthTestOff, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(depthTestOff, SoCallback, TRUE, topSeparator,
                           overlaySep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(overlaySep, SoSeparator, TRUE, topSeparator,
                           depthTestOn, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(depthTestOn, SoCallback, TRUE, topSeparator, "",
                           FALSE);

  SO_KIT_INIT_INSTANCE();

  SO_KIT_ADD_FIELD(viewportSize, (SbVec3f(100.0f, 100.0f, 0.0f)));

  SoOrthographicCamera * camera =
    static_cast<SoOrthographicCamera *>(this->getAnyPart("overlayCamera", TRUE));
  camera->viewportMapping = SoCamera::LEAVE_ALONE;

  SoCallback * viewportCB =
    static_cast<SoCallback *>(this->getAnyPart("viewportInfo", TRUE));
  assert(viewportCB &&
         (viewportCB->getTypeId() == SoCallback::getClassTypeId()));
  viewportCB->setCallback(grabViewportInfo, this);

  SoCallback * beforeCB =
    static_cast<SoCallback *>(this->getAnyPart("depthTestOff", TRUE));
  beforeCB->setCallback(disableDepthTest);
  SoCallback * afterCB =
    static_cast<SoCallback *>(this->getAnyPart("depthTestOn", TRUE));
  afterCB->setCallback(enableDepthTest);
}

/*!
  Destructor.
 */
SoProfilerOverlayKit::~SoProfilerOverlayKit(void)
{
}

/*!
  Display profiling scene graph on top of the regular viewport.

  \param node Scene graph to display
 */
void
SoProfilerOverlayKit::addOverlayGeometry(SoNode * node)
{
  SoNode * sep = this->getAnyPart("overlaySep", TRUE);
  assert(sep->isOfType(SoGroup::getClassTypeId()));
  static_cast<SoGroup *>(sep)->addChild(node);
}

#undef PRIVATE

#endif // HAVE_NODEKITS
