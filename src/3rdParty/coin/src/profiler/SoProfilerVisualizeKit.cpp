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
  \class SoProfilerVisualizeKit SoProfilerVisualizeKit.h Inventor/annex/Profiler/nodekits/SoProfilerVisualizeKit.h
  \brief The SoProfilerVisualizeKit element class is yet to be documented.

  \ingroup coin_profiler
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_NODEKITS

#include <Inventor/annex/Profiler/nodekits/SoProfilerVisualizeKit.h>
#include "coindefs.h"

#include <memory>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/annex/Profiler/nodekits/SoNodeVisualize.h>
#include <Inventor/annex/Profiler/nodes/SoProfilerStats.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include "nodekits/SoSubKitP.h"

namespace {
  // anonymous namespace for trigger functions, to change how
  // visualization occur.
  void cacheSensorCB(void * data, SoSensor * COIN_UNUSED_ARG(sense)){
    SoProfilerVisualizeKit * kit = (SoProfilerVisualizeKit*)data;
    /*SoNode * root = */kit->root.getValue();
    if(!kit->root.getValue())
      return;

    SoNodeVisualize * nv = (SoNodeVisualize*)kit->getPart("visualtree", FALSE);
    if(nv){
      SoProfilerStats * stats = (SoProfilerStats*)kit->stats.getValue();
      assert(stats->getTypeId() == SoProfilerStats::getClassTypeId() &&
             "Wake up and get your act together, will you?");

      nv->traverse(stats);
    }
  }

  void rootChangedCB(void * data, SoSensor * COIN_UNUSED_ARG(sense)){
    // FIXME 20071109 rolvs: Is it possible to automatically detect structural changes
    // in the scene graph? Perhaps from Inventor/misc/SoNotRec.h or something. Now
    // we build the SoNodeVisualize-tree on every root-change
    SoProfilerVisualizeKit * kit = (SoProfilerVisualizeKit*)data;
    if(!kit->getPart("visualtree", FALSE))
      if(kit->root.getValue())
        kit->setPart("visualtree", SoNodeVisualize::visualizeTree(kit->root.getValue(), 1));
  }

  void statsCB(void * data, SoSensor * COIN_UNUSED_ARG(s)) {
    SoProfilerVisualizeKit * kit = (SoProfilerVisualizeKit*)data;
    kit->statsTrigger.disconnect();
    SoNode * node = kit->stats.getValue();
    if (node != NULL) {
      if (node->isOfType(SoProfilerStats::getClassTypeId())) {
        SoProfilerStats * statsnode = (SoProfilerStats *)node;
        kit->statsTrigger.connectFrom(&statsnode->profilingUpdate);
      } else {
#if COIN_DEBUG
        static SbBool first = TRUE;
        if (first) {
          SoDebugError::postWarning("SoProfilerVisualizeKit.cpp rootChangedCB",
                                    "The node i SoProfilerVisualizeKit::stats"
                                    "must be of type SoProfilerStats.",
                                    kit->getTypeId().getName().getString());
          first = FALSE;
        }
#endif // COIN_DEBUG
      }
    }
  }

  void statsTriggerCB(void * data, SoSensor * s) {
    SoProfilerVisualizeKit * kit = (SoProfilerVisualizeKit*)data;
    if (kit->stats.getValue() != NULL)
      cacheSensorCB(data, s);
  }
};

struct SoProfilerVisualizeKitP
{
  // Sensors
  std::unique_ptr<SoFieldSensor> cacheSensor;
  std::unique_ptr<SoFieldSensor> rootSensor;
  std::unique_ptr<SoFieldSensor> statsTriggerSensor;
  std::unique_ptr<SoFieldSensor> statsSensor;
};

#define PRIVATE(x) ((x)->pimpl)

SO_KIT_SOURCE(SoProfilerVisualizeKit);


// Docs from parent
void SoProfilerVisualizeKit::initClass()
{
  SO_KIT_INTERNAL_INIT_CLASS(SoProfilerVisualizeKit, SO_FROM_COIN_3_0);
}


SoProfilerVisualizeKit::SoProfilerVisualizeKit()
{
  SO_KIT_INTERNAL_CONSTRUCTOR(SoProfilerVisualizeKit);
  SO_KIT_ADD_CATALOG_ENTRY(top, SoSeparator, FALSE, this, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(pretree, SoGroup, FALSE, top, visualtree, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(visualtree, SoNodeVisualize, TRUE, top, "", TRUE);
  SO_KIT_INIT_INSTANCE();

  SO_KIT_ADD_FIELD(stats, (NULL));
  SO_KIT_ADD_FIELD(statsTrigger, ());
  SO_KIT_ADD_FIELD(root, (NULL));
  SO_KIT_ADD_FIELD(separatorsWithGLCaches, (NULL));

  this->separatorsWithGLCaches.setNum(0);
  this->separatorsWithGLCaches.setDefault(FALSE);
  PRIVATE(this)->cacheSensor.reset(new SoFieldSensor(cacheSensorCB, this));
  PRIVATE(this)->cacheSensor->attach(&this->separatorsWithGLCaches);
  PRIVATE(this)->rootSensor.reset(new SoFieldSensor(rootChangedCB, this));
  PRIVATE(this)->rootSensor->attach(&this->root);
  PRIVATE(this)->statsTriggerSensor.reset(new SoFieldSensor(statsTriggerCB, this));
  PRIVATE(this)->statsTriggerSensor->attach(&this->statsTrigger);
  PRIVATE(this)->statsSensor.reset(new SoFieldSensor(statsCB, this));
  PRIVATE(this)->statsSensor->attach(&this->stats);

  SoGroup * pretree = (SoGroup *)this->getAnyPart("pretree", TRUE);

  SoScale * treescale = new SoScale;
  treescale->scaleFactor.setValue(0.02f, 0.02f, 1.0f);
  SoLightModel * lm = new SoLightModel;
  lm->model.setValue(SoLightModel::BASE_COLOR);
  SoBaseColor * bc = new SoBaseColor;
  bc->rgb.setValue(1.0, 1.0, 1.0);
  SoDirectionalLight * dl = new SoDirectionalLight;
  SoMaterial * m = new SoMaterial;
  SoTranslation * tr = new SoTranslation;
  tr->translation.setValue(0.0f, 0.0f, -2.0f);
  pretree->addChild(treescale);
  pretree->addChild(lm);
  pretree->addChild(dl);
  pretree->addChild(m);
  pretree->addChild(tr);
}

SoProfilerVisualizeKit::~SoProfilerVisualizeKit()
{
}

/*
void
SoProfilerVisualizeKit::GLRender(SoGLRenderAcion * action)
{
  SoState * state = action->getState();
  if (state->isElementEnabled(SoCacheElement::getClassStackIndex())) {
    SoCacheElement::invalidate(state);
  }

}
*/

#undef PRIVATE

#endif // HAVE_NODEKITS
