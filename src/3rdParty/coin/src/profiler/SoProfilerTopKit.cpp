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
  \class SoProfilerTopKit SoProfilerTopKit.h Inventor/annex/Profiler/nodekits/SoProfilerTopKit.h
  \brief The SoProfilerTopKit element class is yet to be documented.

  \ingroup coin_profiler
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_NODEKITS

#include <Inventor/annex/Profiler/nodekits/SoProfilerTopKit.h>
#include "coindefs.h"

#include <Inventor/annex/Profiler/engines/SoProfilerTopEngine.h>
#include <Inventor/annex/Profiler/nodes/SoProfilerStats.h>
#include <Inventor/annex/Profiler/nodekits/SoScrollingGraphKit.h>
#include <Inventor/misc/SoRefPtr.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/engines/SoCalculator.h>

#include "nodekits/SoSubKitP.h"

#define PUBLIC(obj) ((obj)->master)
#define PRIVATE(obj) ((obj)->pimpl)

class SoProfilerTopKitP
{
public:
  SoProfilerTopKitP()
    : geometryEngine(NULL),
      topListEngine(NULL),
      last_stats(NULL)
  { }

  SoProfilerTopKit * master;

  static void statsNodeChanged(void *, SoSensor *);

  void detachFromStats();
  void attachToStats();

  SoRefPtr<SoCalculator> geometryEngine;
  SoRefPtr<SoProfilerTopEngine> topListEngine;
  SoProfilerStats * last_stats;
  SoFieldSensor * stats_sensor;
};

void
SoProfilerTopKitP::statsNodeChanged(void * userdata, SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoProfilerTopKit * thisp = (SoProfilerTopKit *)userdata;

  SoProfilerStats * stats =
    (SoProfilerStats *)thisp->getPart("profilingStats", FALSE);
  if (PRIVATE(thisp)->last_stats != stats) {
    PRIVATE(thisp)->last_stats = stats;
    PRIVATE(thisp)->detachFromStats();
    PRIVATE(thisp)->attachToStats();
  }
}

void
SoProfilerTopKitP::detachFromStats()
{
  this->topListEngine->statisticsNames.disconnect();
  this->topListEngine->statisticsCounts.disconnect();
  this->topListEngine->statisticsTimings.disconnect();
  this->topListEngine->statisticsTimingsMax.disconnect();
  this->topListEngine->maxLines.disconnect();

  SoScrollingGraphKit * graph =
    (SoScrollingGraphKit *) PUBLIC(this)->getAnyPart("graph", TRUE);
  assert(graph && graph->isOfType(SoScrollingGraphKit::getClassTypeId()));
  graph->addKeys.disconnect();
  graph->addValues.disconnect();
}

void
SoProfilerTopKitP::attachToStats()
{
  SoProfilerStats * statsNode =
    (SoProfilerStats *)PUBLIC(this)->getPart("profilingStats",
                                             FALSE);
  if (statsNode == NULL)
    return ;

  this->topListEngine->statisticsNames.connectFrom(&statsNode->renderedNodeType);
  this->topListEngine->statisticsCounts.connectFrom(&statsNode->renderedNodeTypeCount);
  this->topListEngine->statisticsTimings.connectFrom(&statsNode->renderingTimePerNodeType);
  this->topListEngine->statisticsTimingsMax.connectFrom(&statsNode->renderingTimeMaxPerNodeType);
  this->topListEngine->maxLines.connectFrom(&PUBLIC(this)->lines);


  SoScrollingGraphKit * graph =
    (SoScrollingGraphKit *) PUBLIC(this)->getAnyPart("graph", TRUE);
  assert(graph && graph->isOfType(SoScrollingGraphKit::getClassTypeId()));
  graph->addKeys.connectFrom(&statsNode->profiledAction);
  graph->addValues.connectFrom(&statsNode->profiledActionTime);
}


SO_KIT_SOURCE(SoProfilerTopKit);

void SoProfilerTopKit::initClass()
{
  SO_KIT_INTERNAL_INIT_CLASS(SoProfilerTopKit, SO_FROM_COIN_3_0);
}

SoProfilerTopKit::SoProfilerTopKit(void)
{
  PRIVATE(this)->master = this;

  SO_KIT_INTERNAL_CONSTRUCTOR(SoProfilerTopKit);
  SO_KIT_ADD_CATALOG_ENTRY(textSep, SoSeparator, TRUE, overlaySep, graph, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(color, SoBaseColor, TRUE, textSep, translation, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(translation, SoTranslation, TRUE, textSep, text, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(text, SoText2, TRUE, textSep, graph, FALSE);

  SO_KIT_ADD_CATALOG_ENTRY(graph, SoScrollingGraphKit, TRUE, overlaySep, "", FALSE);

  SO_KIT_ADD_FIELD(txtColor, (1.0, 1.0, 1.0));
  SO_KIT_ADD_FIELD(lines, (16));

  SO_KIT_ADD_FIELD(topKitSize, (SbVec2f(100.0f, 100.0f)));
  SO_KIT_ADD_FIELD(position, (SbVec3f(4.0f, 4.0f, 0.0f)));

  SO_KIT_INIT_INSTANCE();

  PRIVATE(this)->topListEngine.reset(new SoProfilerTopEngine);
  PRIVATE(this)->topListEngine->decay.setValue(0.99f);

  PRIVATE(this)->geometryEngine.reset(new SoCalculator);

  const char * expr[] = {
    // A = viewportsize, B = wanted position
    // assuming translation from center (0,0), and flipped y-axis
    // trans = - (viewportsize_n / 2) + (pixelsize * position)
    "ta = -1.0 + ((2.0 / A[0]) * B[0])",
    "tb = 1.0 - ((2.0 / A[1]) * (B[1] + 10.0))",
    "oA = vec3f(ta, tb, -1)"
  };

  const int num = sizeof(expr) / sizeof(const char *);
  PRIVATE(this)->geometryEngine->expression.setNum(num);
  PRIVATE(this)->geometryEngine->expression.setValues(0, num, expr);

  PRIVATE(this)->geometryEngine->A.connectFrom(&viewportSize);
  PRIVATE(this)->geometryEngine->B.connectFrom(&position);

  SoTranslation * transe = (SoTranslation *) this->getAnyPart("translation",
                                                              TRUE);
  transe->translation.connectFrom(&(PRIVATE(this)->geometryEngine->oA));

  SoText2 * txt = (SoText2 *) this->getAnyPart("text", TRUE);
  txt->string.connectFrom(&PRIVATE(this)->topListEngine->prettyText);

  SoBaseColor * colorNd = (SoBaseColor *) this->getAnyPart("color", TRUE);
  colorNd->rgb.connectFrom(&this->txtColor);

  PRIVATE(this)->attachToStats();

  PRIVATE(this)->stats_sensor =
    new SoFieldSensor(PRIVATE(this)->statsNodeChanged, this);
  PRIVATE(this)->stats_sensor->attach(&this->profilingStats);
}

SoProfilerTopKit::~SoProfilerTopKit(void)
{
}

#undef PRIVATE
#undef PUBLIC
#endif // HAVE_NODEKITS
