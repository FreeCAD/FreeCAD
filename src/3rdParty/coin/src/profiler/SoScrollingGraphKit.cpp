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
  \class SoScrollingGraphKit SoScrollingGraphKit.h Inventor/annex/Profiler/nodekits/SoScrollingGraphKit.h
  \brief The SoScrollingGraphKit element class is yet to be documented.

  \ingroup coin_profiler
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_NODEKITS

#include <Inventor/annex/Profiler/nodekits/SoScrollingGraphKit.h>

#include <cstdlib>
#include <cstdio>

#include <memory>
#include <vector>

#include <Inventor/SbTime.h>
#include <Inventor/SbColor.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/misc/SoRefPtr.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>

#include "misc/SbHash.h"
#include "nodekits/SoSubKitP.h"

// *************************************************************************

//namespace {

class Graph {
public:
  SbName key;
  int index;
  SbColor color;
  float baseval;
  float basetarget;
};

class Datum {
public:
  Datum(void) : next(NULL) { }
  SbTime when;
  SbList<float> datum;
  Datum * next;
}; // Datum

//} // anonymous

// *************************************************************************

class SoScrollingGraphKitP {
public:
  SoScrollingGraphKitP(void) : kit(NULL), first(NULL), last(NULL) {
    this->cachedmaxvalue = 0.0f;
    this->cachedrealmaxvalue = 0.0f;
  }
  ~SoScrollingGraphKitP(void) {
    SbList<const char *> keys;
    this->graphs.makeKeyList(keys);
    for (int c = 0; c < keys.getLength(); ++c) {
      Graph * obj = NULL;
      if (this->graphs.get(keys[c], obj)) { delete obj; }
    }
    this->graphs.clear();

    while (this->first) {
      Datum * datum = this->first;
      this->first = this->first->next;
      delete datum;
    }
  }

  SoRefPtr<SoSeparator> chart;
  std::unique_ptr<SoFieldSensor> addValuesSensor;

  void pullStatistics(void);
  Graph * getGraph(const SbName & key);
  Graph * getGraph(int idx);
  void addDatum(Datum * newDatum);

  void generateStackedBarsChart(void);

  SbHash<const char *, Graph *> graphs;

  SoScrollingGraphKit * kit;
  Datum * first;
  Datum * last;

  float cachedmaxvalue;
  float cachedrealmaxvalue;
}; // SoScrollingGraphKitP

// *************************************************************************

#define PRIVATE(obj) ((obj)->pimpl)

SO_KIT_SOURCE(SoScrollingGraphKit);

void
SoScrollingGraphKit::initClass(void)
{
  SO_KIT_INIT_CLASS(SoScrollingGraphKit, SoBaseKit, "BaseKit");
}

SoScrollingGraphKit::SoScrollingGraphKit(void)
{
  PRIVATE(this)->kit = this;

  SO_KIT_INTERNAL_CONSTRUCTOR(SoScrollingGraphKit);
  SO_KIT_ADD_CATALOG_ENTRY(scene, SoSeparator, TRUE, this, "", FALSE);
  SO_KIT_INIT_INSTANCE();

  SO_KIT_DEFINE_ENUM_VALUE(GraphicsType, LINES);
  SO_KIT_DEFINE_ENUM_VALUE(GraphicsType, STACKED_BARS);
  SO_KIT_DEFINE_ENUM_VALUE(GraphicsType, DEFAULT_GRAPHICS);
  SO_KIT_SET_SF_ENUM_TYPE(graphicsType, GraphicsType);

  SO_KIT_DEFINE_ENUM_VALUE(RangeType, ABSOLUTE_ACCUMULATIVE);
  //SO_KIT_DEFINE_ENUM_VALUE(RangeType, ABSOLUTE_OVERWRITE);
  //SO_KIT_DEFINE_ENUM_VALUE(RangeType, RELATIVE_ACCUMULATIVE);
  //SO_KIT_DEFINE_ENUM_VALUE(RangeType, RELATIVE_OVERWRITE);
  SO_KIT_DEFINE_ENUM_VALUE(RangeType, DEFAULT_RANGETYPE);
  SO_KIT_SET_SF_ENUM_TYPE(rangeType, RangeType);

  SO_KIT_ADD_FIELD(graphicsType, (DEFAULT_GRAPHICS));
  SO_KIT_ADD_FIELD(rangeType, (DEFAULT_RANGETYPE));
  SO_KIT_ADD_FIELD(seconds, (SbTime(5.0)));
  SO_KIT_ADD_FIELD(colors, (SbColor(0.0f, 0.0f, 0.0f)));
  SbColor default_colors[] = {
    SbColor(1.0f, 0.0f, 0.0f),
    SbColor(0.0f, 1.0f, 0.0f),
    SbColor(0.0f, 0.0f, 1.0f),
    SbColor(1.0f, 0.0f, 1.0f),
    SbColor(1.0f, 1.0f, 0.0f),
    SbColor(0.0f, 1.0f, 1.0f)
  };
  const int numdefaultcolors =
    sizeof(default_colors) / sizeof(default_colors[0]);
  this->colors.setNum(numdefaultcolors);
  this->colors.setValues(0, numdefaultcolors, default_colors);
  this->colors.setDefault(TRUE);

  SO_KIT_ADD_FIELD(viewportSize, (SbVec3f(512.0f, 512.0f, 0.0f)));
  SO_KIT_ADD_FIELD(position, (SbVec3f(4.0f, 4.0f, 0.0f)));
  SO_KIT_ADD_FIELD(size, (SbVec3f(256.0f, 100.0f, 0.0f)));

  SO_KIT_ADD_FIELD(addKeys, (SbName::empty()));
  this->addKeys.setNum(0);
  this->addKeys.setDefault(TRUE);
  SO_KIT_ADD_FIELD(addValues, (0.0f));
  this->addValues.setNum(0);
  this->addValues.setDefault(TRUE);

  PRIVATE(this)->addValuesSensor.reset(new SoFieldSensor);
  PRIVATE(this)->addValuesSensor->setFunction(SoScrollingGraphKit::addValuesCB);
  PRIVATE(this)->addValuesSensor->setData(this);
  PRIVATE(this)->addValuesSensor->attach(&(this->addValues));

  PRIVATE(this)->chart.reset(static_cast<SoSeparator *>(this->getAnyPart("scene", TRUE)));
}

SoScrollingGraphKit::~SoScrollingGraphKit(void)
{
}

void
SoScrollingGraphKit::addValuesCB(void * closure, SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoScrollingGraphKit * kit = static_cast<SoScrollingGraphKit *>(closure);

  PRIVATE(kit)->pullStatistics();
  switch (kit->graphicsType.getValue()) {
  case SoScrollingGraphKit::LINES:
    //PRIVATE(kit)->generateLineChart();
    //break;
  case SoScrollingGraphKit::STACKED_BARS:
    PRIVATE(kit)->generateStackedBarsChart();
    break;
  default:
    //PRIVATE(kit)->generateLineChart();
    PRIVATE(kit)->generateStackedBarsChart();
    break;
  }
}

void
SoScrollingGraphKitP::pullStatistics(void)
{
  SbTime now(SbTime::getTimeOfDay());
  const int numvalues = kit->addValues.getNum();
  if (kit->addKeys.getNum() != numvalues) {
    SoDebugError::post("SoScrollingGraphKitP::pullStatistics",
                       "data out of sync (%d keys, %d values)",
                       numvalues, kit->addKeys.getNum());
  }

  Datum * datum = new Datum;
  datum->when = now;

  int c;
  for (c = 0; c < numvalues; ++c) { // make sure all the grraph structs needed are created
    SbName key = kit->addKeys[c];
    /*Graph * graph = */this->getGraph(key);
  }
  const int numgraphs = this->graphs.getNumElements();
  for (c = 0; c < numgraphs; ++c) { // initialize default for all graphs
    datum->datum.append(0.0f);
  }
  for (c = 0; c < numvalues; ++c) { // fill values for the graphs we have data for
    SbName key = kit->addKeys[c];
    Graph * graph = this->getGraph(key);
    datum->datum[graph->index] = kit->addValues[c];
  }
  this->addDatum(datum);
}

void
SoScrollingGraphKitP::addDatum(Datum * newDatum)
{
  if (this->first == NULL) {
    this->first = this->last = newDatum;
  } else {
    this->last->next = newDatum;
    this->last = newDatum;
  }

  SbTime maxtime(this->kit->seconds.getValue());
  while ((this->first->next != NULL) &&
         ((newDatum->when - this->first->next->when) > maxtime)) {
    Datum * datum = this->first;
    this->first = this->first->next;
    delete datum;
  }
}

Graph *
SoScrollingGraphKitP::getGraph(const SbName & key)
{
  assert(key != SbName::empty());
  Graph * graph = NULL;
  if (!this->graphs.get(key.getString(), graph)) {
    graph = new Graph;
    graph->key = key;
    graph->index = this->graphs.getNumElements();
    graph->color = this->kit->colors[graph->index % this->kit->colors.getNum()];
    graph->baseval = graph->basetarget = 0.0f;
    this->graphs.put(key.getString(), graph);

    printf("Adding graph #%d for '%s', color #%02x%02x%02x\n",
           graph->index + 1, key.getString(), (uint8_t)(graph->color[0] * 255.0f),
           (uint8_t) (graph->color[1] * 255.0), (uint8_t) (graph->color[2] * 255.0));
  }
  assert(graph);
  return graph;
}

Graph *
SoScrollingGraphKitP::getGraph(int idx)
{
  SbList<const char *> keys;
  this->graphs.makeKeyList(keys);
  for (int i = 0; i < keys.getLength(); ++i) {
    Graph * graph = NULL;
    this->graphs.get(keys[i], graph);
    if (graph->index == idx) return graph;
  }
  assert(!"serious problem - did not find graph index data");
  return NULL;
}


#define LINEUP 1
#define SMOOTH_DECAY 1

template <class Type>
Type *
new_node(const char * buffer)
{
  Type * node = new Type;
  if (!node->set(buffer)) {
    assert(!"node field arguments error");
    node->ref();
    node->unref();
    return NULL;
  }
  return node;
}

inline
float
decayvalue(float current, float target, float decay = 0.95f)
{
#if SMOOTH_DECAY
  return (current * decay) + (target * (1.0f - decay));
#else
  return target;
#endif
}



void
SoScrollingGraphKitP::generateStackedBarsChart(void)
{
  const int numgraphs = this->graphs.getNumElements();
  if (numgraphs == 0) return;

  std::vector<SoBaseColor *> colors(numgraphs);
  std::vector<SoCoordinate3 *> coords(numgraphs);
  std::vector<SoLineSet *> lines(numgraphs);
  std::vector<SoTranslation *> texttrans(numgraphs);
  std::vector<SoText2 *> textnodes(numgraphs);

  if (this->chart->getNumChildren() != (numgraphs * 4 + 3) ||
      !(this->chart->getChild(2+2)->isOfType(SoLineSet::getClassTypeId()))) {
    this->chart->removeAllChildren();

    this->chart->addChild(new_node<SoTranslation>("translation -1 -0.99 0"));
    this->chart->addChild(new_node<SoScale>("scaleFactor 2 0.5 1"));

    for (int c = 0; c < numgraphs; ++c) {
      this->chart->addChild(colors[c] = new SoBaseColor);
      { // the text for the graph name
        SoSeparator * sep = new SoSeparator;
        sep->addChild(texttrans[c] = new SoTranslation);
        sep->addChild(textnodes[c] = new SoText2);
        this->chart->addChild(sep);
      }
      this->chart->addChild(coords[c] = new SoCoordinate3);
      this->chart->addChild(lines[c] = new SoLineSet);
    }
    {
      SoSeparator * sep = new SoSeparator;
      sep->addChild(new_node<SoBaseColor>("rgb 1 1 1"));
      sep->addChild(new SoTranslation);
      sep->addChild(new SoText2);
      this->chart->addChild(sep);
    }
  } else {
    for (int c = 0; c < numgraphs; ++c) {
      colors[c] = static_cast<SoBaseColor *>(this->chart->getChild(2 + c * 4 + 0));
      assert(colors[c]->isOfType(SoBaseColor::getClassTypeId()));
      { // the text for the graph name
        SoSeparator * sep = static_cast<SoSeparator *>(this->chart->getChild(2 + c * 4 + 1));
        assert(sep->isOfType(SoSeparator::getClassTypeId()));
        texttrans[c] = static_cast<SoTranslation *>(sep->getChild(0));
        textnodes[c] = static_cast<SoText2 *>(sep->getChild(1));
      }
      coords[c] = static_cast<SoCoordinate3 *>(this->chart->getChild(2 + c * 4 + 2));
      assert(coords[c]->isOfType(SoCoordinate3::getClassTypeId()));
      lines[c] = static_cast<SoLineSet *>(this->chart->getChild(2 + c * 4 + 3));
      assert(lines[c]->isOfType(SoLineSet::getClassTypeId()));
    }
  }

  int numdata = 0;
  SbTime end(SbTime::zero());
  Datum * datum = this->first;
  float maxvalue = 0.0f;
#if LINEUP
  SbList<float> maxvalues;
  {
    while (datum) {
      ++numdata;
      while (maxvalues.getLength() < datum->datum.getLength()) {
        maxvalues.append(0.0f);
      }
      for (int j = 0; j < datum->datum.getLength(); ++j) {
        float val = datum->datum[j];
        if (val > maxvalues[j]) { maxvalues[j] = val; }
      }
      end = datum->when;
      datum = datum->next;
    }
    for (int j = 0; j < maxvalues.getLength(); ++j) {
      Graph * graph = this->getGraph(j);
      if (j == 0) {
        graph->basetarget = 0.0f;
      } else {
        graph->basetarget = maxvalue; // maxvalues[j-1];
      }
      maxvalue += maxvalues[j];
      graph->baseval = decayvalue(graph->baseval, graph->basetarget);
    }
  }
#else
  while (datum) {
    ++numdata;
    float sum = 0.0f;
    for (int j = 0; j < datum->datum.getLength(); ++j) {
      sum += datum->datum[j];
    }
    if (sum > maxvalue) maxvalue = sum;
    end = datum->when;
    datum = datum->next;
  }
#endif

  this->cachedrealmaxvalue = maxvalue;
  this->cachedmaxvalue = decayvalue(this->cachedmaxvalue, this->cachedrealmaxvalue);

  // printf("values: %d, maxvalue: %f\n", numdata, maxvalue);

  // printf("num data values: %d\n", numdata);
  const float seconds = (float) this->kit->seconds.getValue().getValue();

  for (int graphidx = 0; graphidx < numgraphs; ++graphidx) {
    Graph * graph = this->getGraph(graphidx);

    colors[graphidx]->rgb = graph->color;

    coords[graphidx]->point.setNum(numdata * 2);
    SbVec3f * coordarray = coords[graphidx]->point.startEditing();

    datum = this->first;
    int idx = 0;
    float maxypos = 0.0f, maxprevypos = 0.0f;
    while (datum) {
      /*const int numvalues = datum->datum.getLength();*/

      float value = 0.0f, prev = 0.0f;
#if LINEUP
      // prev = (graph->index == 0) ? 0.0f : maxvalues[graph->index - 1];
      prev = graph->baseval;
      value = prev + datum->datum[graph->index];
#else
      for (int j = 0; j <= graph->index && j < numvalues; ++j) {
        prev = value;
        value += datum->datum[j];
      }
#endif

      const float xpos = 1.0f - ((float)((end - datum->when).getValue()) / seconds);
      const float ypos = (this->cachedrealmaxvalue > 0.0f) ? (value / this->cachedrealmaxvalue) : value;
      const float prevypos = (this->cachedrealmaxvalue > 0.0f) ? (prev / this->cachedrealmaxvalue) : prev;

      //printf("coord %d: %f %f\n", idx, xpos, ypos);
      coordarray[idx*2] = SbVec3f(xpos, prevypos, 0.0f);
      coordarray[idx*2+1] = SbVec3f(xpos, ypos, 0.0f);

      ++idx;
      datum = datum->next;

      if (maxypos < ypos) maxypos = ypos;
      if (maxprevypos < prevypos) maxprevypos = prevypos;
    }
    coords[graphidx]->point.finishEditing();

    lines[graphidx]->numVertices.setNum(idx);
    int32_t * countarray = lines[graphidx]->numVertices.startEditing();
    for (int j = 0; j < idx; ++j) {
      countarray[j] = 2;
    }
    lines[graphidx]->numVertices.finishEditing();

    textnodes[graphidx]->string.setValue(graph->key.getString());
    texttrans[graphidx]->translation.setValue(SbVec3f(0.0f, (maxypos + graph->baseval * 2.0f) * 0.5f, 0.0f));
  }


  // graph space is now between 0-1 in both directions

  {
    SoSeparator * sep = static_cast<SoSeparator *>(this->chart->getChild(numgraphs * 4 + 2));
    assert(sep && sep->isOfType(SoSeparator::getClassTypeId()));
    assert(sep->getNumChildren() == 3);
    SoTranslation * trans = static_cast<SoTranslation *>(sep->getChild(1));
    assert(trans && trans->isOfType(SoTranslation::getClassTypeId()));
    trans->translation.setValue(0.98f, 1.0f, 0.0f);
    SoText2 * text = static_cast<SoText2 *>(sep->getChild(2));
    assert(text && text->isOfType(SoText2::getClassTypeId()));

    SbString content;
    content.sprintf("%6.0f ms _", this->cachedmaxvalue * 1000.0f);
    text->justification.setValue(SoText2::RIGHT);
    text->string.setValue(content.getString());
  }
}

// *************************************************************************

#undef PRIVATE
#undef LINEUP
#undef SMOOTH_DECAY

#endif // HAVE_NODEKITS
