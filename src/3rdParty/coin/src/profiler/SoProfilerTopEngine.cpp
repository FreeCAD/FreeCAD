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
  \class SoProfilerTopEngine Inventor/annex/Profiler/engines/SoProfilerTopEngine.h
  \brief The SoProfilerTopEngine class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/annex/Profiler/engines/SoProfilerTopEngine.h>

#include <cstdlib>
#include <cstring>

#include <Inventor/errors/SoDebugError.h>

#include <Inventor/SbString.h>
#include <Inventor/fields/SoMFString.h>

#include "engines/SoSubEngineP.h"

// FIXME: swap two data-arrays instead of new'ing one for each evaluate(),
// so we don't add unnecessary exhaustion to the memory allocator subsystem
// when profiling.

// FIXME: document fields/enums/class purpose

class SoProfilerTopEngineP {
public:
  SoProfilerTopEngineP(void) : datasize(0), data(NULL) { }
  ~SoProfilerTopEngineP(void) {
    delete [] data;
    data = NULL;
    //delete [] tmpdata;
    //tmpdata = NULL;
  }

  struct StatDataItem {
    SbName name;
    int32_t count;
    // decayed inline
    SbTime timing;
    SbTime timingavg;
    SbTime timingmax;
  };

  int datasize;
  StatDataItem * data;
  //StatDataItem * tmpdata;

  static int qsort_time_dec(const void *, const void *);
  static int qsort_time_avg_dec(const void *, const void *);
  static int qsort_time_max_dec(const void *, const void *);
  static int qsort_count_dec_time_dec(const void *, const void *);
  static int qsort_count_dec_time_avg_dec(const void *, const void *);
  static int qsort_count_dec_time_max_dec(const void *, const void *);
  static int qsort_alphanumeric_inc(const void *, const void *);

};

// *************************************************************************

#define PRIVATE(obj) ((obj)->pimpl)

SO_ENGINE_SOURCE(SoProfilerTopEngine);

void
SoProfilerTopEngine::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoProfilerTopEngine);
}

SoProfilerTopEngine::SoProfilerTopEngine(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoProfilerTopEngine);

  SO_ENGINE_DEFINE_ENUM_VALUE(Column, NAME);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, COUNT);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_SECS);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_SECS_MAX);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_SECS_AVG);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_MSECS);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_MSECS_MAX);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_MSECS_AVG);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_PERCENT);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_PERCENT_MAX);
  SO_ENGINE_DEFINE_ENUM_VALUE(Column, TIME_PERCENT_AVG);
  SO_ENGINE_SET_SF_ENUM_TYPE(columns, Column);

  SO_ENGINE_DEFINE_ENUM_VALUE(SortOrder, TIME_DEC);
  SO_ENGINE_DEFINE_ENUM_VALUE(SortOrder, TIME_MAX_DEC);
  SO_ENGINE_DEFINE_ENUM_VALUE(SortOrder, TIME_AVG_DEC);
  SO_ENGINE_DEFINE_ENUM_VALUE(SortOrder, COUNT_DEC_TIME_DEC);
  SO_ENGINE_DEFINE_ENUM_VALUE(SortOrder, COUNT_DEC_TIME_MAX_DEC);
  SO_ENGINE_DEFINE_ENUM_VALUE(SortOrder, COUNT_DEC_TIME_AVG_DEC);
  SO_ENGINE_DEFINE_ENUM_VALUE(SortOrder, ALPHANUMERIC_INC);
  SO_ENGINE_SET_SF_ENUM_TYPE(sortOrder, SortOrder);

  SO_ENGINE_ADD_INPUT(statisticsNames, (SbName::empty()));
  //this->statisticsNames.setNum(0);
  //this->statisticsNames.setDefault(TRUE);

  SO_ENGINE_ADD_INPUT(statisticsTimings, (SbTime::zero()));
  //this->statisticsTimings.setNum(0);
  //this->statisticsTimings.setDefault(TRUE);

  SO_ENGINE_ADD_INPUT(statisticsTimingsMax, (SbTime::zero()));
  //this->statisticsTimingsMax.setNum(0);
  //this->statisticsTimingsMax.setDefault(TRUE);

  SO_ENGINE_ADD_INPUT(statisticsCounts, (0));
  //this->statisticsCounts.setNum(0);
  //this->statisticsCounts.setDefault(TRUE);

  SO_ENGINE_ADD_INPUT(columns, (NAME));
  int cols[] = { NAME, COUNT, TIME_MSECS, TIME_PERCENT, TIME_MSECS_AVG, TIME_MSECS_MAX };
  this->columns.setNum(sizeof(cols) / sizeof(cols[0]));
  this->columns.setValues(0, sizeof(cols) / sizeof(cols[0]), cols);
  this->columns.setDefault(TRUE);

  SO_ENGINE_ADD_INPUT(sortOrder, (TIME_DEC));
  SO_ENGINE_ADD_INPUT(maxLines, (16));
  SO_ENGINE_ADD_INPUT(decay, (0.9f));

  SO_ENGINE_ADD_OUTPUT(prettyText, SoMFString);
}

SoProfilerTopEngine::~SoProfilerTopEngine(void)
{
}

// *************************************************************************
// QSORT callbacks
int
SoProfilerTopEngineP::qsort_time_dec(const void * p1, const void * p2)
{
  const StatDataItem * item1 = static_cast<const StatDataItem *>(p1);
  const StatDataItem * item2 = static_cast<const StatDataItem *>(p2);
  double diff = item2->timing.getValue() - item1->timing.getValue();
  if (diff < 0.0) return -1;
  if (diff > 0.0) return 1;
  return 0;
}

int
SoProfilerTopEngineP::qsort_time_max_dec(const void * p1, const void * p2)
{
  const StatDataItem * item1 = static_cast<const StatDataItem *>(p1);
  const StatDataItem * item2 = static_cast<const StatDataItem *>(p2);
  double diff = item2->timingmax.getValue() - item1->timingmax.getValue();
  if (diff < 0.0) return -1;
  if (diff > 0.0) return 1;
  return 0;
}

int
SoProfilerTopEngineP::qsort_time_avg_dec(const void * p1, const void * p2)
{
  const StatDataItem * item1 = static_cast<const StatDataItem *>(p1);
  const StatDataItem * item2 = static_cast<const StatDataItem *>(p2);
  double diff = item2->timingavg.getValue() - item1->timingavg.getValue();
  if (diff < 0.0) return -1;
  if (diff > 0.0) return 1;
  return 0;
}

int
SoProfilerTopEngineP::qsort_count_dec_time_dec(const void * p1, const void * p2)
{
  const StatDataItem * item1 = static_cast<const StatDataItem *>(p1);
  const StatDataItem * item2 = static_cast<const StatDataItem *>(p2);

  int diff = item2->count - item1->count;

  if (diff == 0) // use secondary sort-key
    diff = SoProfilerTopEngineP::qsort_time_dec(p1, p2);

  return diff;
}

int
SoProfilerTopEngineP::qsort_count_dec_time_max_dec(const void * p1, const void * p2)
{
  const StatDataItem * item1 = static_cast<const StatDataItem *>(p1);
  const StatDataItem * item2 = static_cast<const StatDataItem *>(p2);

  int diff = item2->count - item1->count;

  if (diff == 0) // use secondary sort-key
    diff = SoProfilerTopEngineP::qsort_time_max_dec(p1, p2);

  return diff;
}

int
SoProfilerTopEngineP::qsort_count_dec_time_avg_dec(const void * p1, const void * p2)
{
  const StatDataItem * item1 = static_cast<const StatDataItem *>(p1);
  const StatDataItem * item2 = static_cast<const StatDataItem *>(p2);

  int diff = item2->count - item1->count;

  if (diff == 0) // use secondary sort-key
    diff = SoProfilerTopEngineP::qsort_time_avg_dec(p1, p2);

  return diff;
}

int
SoProfilerTopEngineP::qsort_alphanumeric_inc(const void * p1, const void * p2)
{
  const StatDataItem * item1 = static_cast<const StatDataItem *>(p1);
  const StatDataItem * item2 = static_cast<const StatDataItem *>(p2);
  return strcmp(item1->name.getString(), item2->name.getString());
}

// *************************************************************************

void
SoProfilerTopEngine::evaluate(void)
{
#if 0
  this->statisticsNames.evaluate();
  this->statisticsTimings.evaluate();
  this->statisticsTimingsMax.evaluate();
  this->statisticsCounts.evaluate();
#endif

  const int inputsize = this->statisticsNames.getNum();

  if (this->statisticsTimings.getNum() != inputsize) {
#if 0
    SoDebugError::post("SoProfilerTopEngine::evaluate",
                       "statisticsNames (#%d) and statisticsTimings (#%d) "
                       "have different sizes.",
                       inputsize, this->statisticsTimings.getNum());
#endif
    return;
  }

  // printf("items: %d\n", inputsize);
  const bool have_counts = (this->statisticsCounts.getNum() == inputsize);
  const bool have_maxtimings = (this->statisticsTimingsMax.getNum() == inputsize);
  const bool have_olddata =
    ((PRIVATE(this)->datasize == inputsize) && (PRIVATE(this)->data != NULL));
  /*const int olddatasize = PRIVATE(this)->datasize;*/
  SoProfilerTopEngineP::StatDataItem * const olddata = PRIVATE(this)->data;

  //if (PRIVATE(this)->tmpdata && PRIVATE(this)->datasize == inputsize) {
  //  PRIVATE(this)->data = PRIVATE(this)->tmpdata;
  //  PRIVATE(this)->tmpdata = olddata;
  //} else {
    PRIVATE(this)->data = new SoProfilerTopEngineP::StatDataItem [ inputsize ];
    PRIVATE(this)->datasize = inputsize;
  //}

  const float decayvalue = SbMax(0.0f, SbMin(1.0f, this->decay.getValue()));
  const float newvaluefactor = 1.0f - decayvalue;

  int c;
  SbTime totaltime(SbTime::zero());
  for (c = 0; c < inputsize; ++c) {
    PRIVATE(this)->data[c].name = this->statisticsNames[c];
    int oldidx = -1;
    for (int j = 0; have_olddata && j < inputsize && oldidx == -1; ++j) {
      if (PRIVATE(this)->data[c].name == olddata[j].name) oldidx = j;
    }
    const bool do_decaying = (oldidx != -1);

    if (do_decaying) {
      PRIVATE(this)->data[c].timing =
        newvaluefactor * this->statisticsTimings[c] +
        decayvalue * olddata[oldidx].timing;
    } else {
      PRIVATE(this)->data[c].timing = this->statisticsTimings[c];
    }

    totaltime += PRIVATE(this)->data[c].timing;

    if (have_counts) {
      PRIVATE(this)->data[c].count = this->statisticsCounts[c];
      float divider = float(PRIVATE(this)->data[c].count);
      if (divider == 0.0f) divider = 1.0f;
      if (do_decaying) {
        PRIVATE(this)->data[c].timingavg =
          newvaluefactor * (PRIVATE(this)->data[c].timing / divider) +
          decayvalue * olddata[oldidx].timingavg;
      } else {
        PRIVATE(this)->data[c].timingavg =
          PRIVATE(this)->data[c].timing / divider;
      }
    } else {
      PRIVATE(this)->data[c].count = 0;
      PRIVATE(this)->data[c].timingavg = PRIVATE(this)->data[c].timing;
    }

    if (have_maxtimings) {
      if (do_decaying) {
        PRIVATE(this)->data[c].timingmax =
          newvaluefactor * this->statisticsTimingsMax[c] +
          decayvalue * olddata[oldidx].timingmax;
      } else {
        PRIVATE(this)->data[c].timingmax = this->statisticsTimingsMax[c];
      }
    } else {
      // fallback on the average value
      PRIVATE(this)->data[c].timingmax = PRIVATE(this)->data[c].timingavg;
    }
  }

  // void qsort(void *base, size_t nmemb, size_t size,
  //            int (*compar)(const void *, const void *));

  const SortOrder sorting = static_cast<SortOrder>(this->sortOrder.getValue());

  const size_t memsize = sizeof(SoProfilerTopEngineP::StatDataItem);
  switch (sorting) {
  case TIME_DEC:
    qsort(PRIVATE(this)->data, PRIVATE(this)->datasize, memsize,
          SoProfilerTopEngineP::qsort_time_dec);
    break;
  case TIME_MAX_DEC:
    qsort(PRIVATE(this)->data, PRIVATE(this)->datasize, memsize,
          SoProfilerTopEngineP::qsort_time_max_dec);
    break;
  case TIME_AVG_DEC:
    qsort(PRIVATE(this)->data, PRIVATE(this)->datasize, memsize,
          SoProfilerTopEngineP::qsort_time_avg_dec);
    break;
  case COUNT_DEC_TIME_DEC:
    qsort(PRIVATE(this)->data, PRIVATE(this)->datasize, memsize,
          SoProfilerTopEngineP::qsort_count_dec_time_dec);
    break;
  case COUNT_DEC_TIME_MAX_DEC:
    qsort(PRIVATE(this)->data, PRIVATE(this)->datasize, memsize,
          SoProfilerTopEngineP::qsort_count_dec_time_max_dec);
    break;
  case COUNT_DEC_TIME_AVG_DEC:
    qsort(PRIVATE(this)->data, PRIVATE(this)->datasize, memsize,
          SoProfilerTopEngineP::qsort_count_dec_time_avg_dec);
    break;
  case ALPHANUMERIC_INC:
    qsort(PRIVATE(this)->data, PRIVATE(this)->datasize, memsize,
          SoProfilerTopEngineP::qsort_alphanumeric_inc);
    break;
  default:
    SoDebugError::post("SoProfilerTopEngine::evaluate", "invalid sort-order");
    break;
  }

  const int numlines = SbMin(static_cast<int>(this->maxLines.getValue()), this->statisticsNames.getNum());

  // find longest name (of the complete nameset (to stay static))
  // and maxcount
  size_t namelen = 0;
  int32_t maxcount = 0;
  for (c = 0; c < this->statisticsNames.getNum(); ++c) {
    namelen = SbMax(namelen, strlen(PRIVATE(this)->data[c].name.getString()));
    maxcount = SbMax(maxcount, PRIVATE(this)->data[c].count);
  }

  // set up the needed format strings
  SbString namefmt; namefmt.sprintf("%%-%ds ", namelen);

  SbString countfmt;
  countfmt.sprintf("%ld", maxcount * 10);
  countfmt.sprintf("%%%dd ", countfmt.getLength());

  SbString timesecsfmt("%8.6f ");
  SbString timemsecsfmt("%4.0fms ");
  SbString timepercentfmt("%5.1f%% ");

  const int numcols = this->columns.getNum();

  SO_ENGINE_OUTPUT(this->prettyText, SoMFString, setNum(numlines));
  for (c = 0; c < numlines; ++c) {
    SbString entryline;
    SbString element;
    for (int col = 0; col < numcols; ++col) {
      switch (static_cast<Column>(this->columns[col])) {
      case NAME:
        element.sprintf(namefmt.getString(),
                        PRIVATE(this)->data[c].name.getString());
        break;
      case COUNT:
        element.sprintf(countfmt.getString(), PRIVATE(this)->data[c].count);
        break;
      case TIME_SECS:
        element.sprintf(timesecsfmt.getString(),
                        PRIVATE(this)->data[c].timing.getValue());
        break;
      case TIME_SECS_MAX:
        element.sprintf(timesecsfmt.getString(),
                        PRIVATE(this)->data[c].timingmax.getValue());
        break;
      case TIME_SECS_AVG:
        element.sprintf(timesecsfmt.getString(),
                        PRIVATE(this)->data[c].timingavg.getValue());
        break;
      case TIME_MSECS:
        element.sprintf(timemsecsfmt.getString(),
                        (PRIVATE(this)->data[c].timing.getValue() * 1000.0));
        break;
      case TIME_MSECS_MAX:
        element.sprintf(timemsecsfmt.getString(),
                        (PRIVATE(this)->data[c].timingmax.getValue() * 1000.0));
        break;
      case TIME_MSECS_AVG:
        element.sprintf(timemsecsfmt.getString(),
                        (PRIVATE(this)->data[c].timingavg.getValue() * 1000.0));
        break;
      case TIME_PERCENT:
        element.sprintf(timepercentfmt.getString(),
                        (100.0 * (PRIVATE(this)->data[c].timing.getValue() /
                                  totaltime.getValue())));
        break;
      case TIME_PERCENT_MAX:
        element.sprintf(timepercentfmt.getString(),
                        (100.0 * (PRIVATE(this)->data[c].timingmax.getValue() /
                                  totaltime.getValue())));
        break;
      case TIME_PERCENT_AVG:
        element.sprintf(timepercentfmt.getString(),
                        (100.0 * (PRIVATE(this)->data[c].timingavg.getValue() /
                                  totaltime.getValue())));
        break;
      default:
        element = " ? ";
        break;
      }
      entryline += element;
    }
    SO_ENGINE_OUTPUT(this->prettyText, SoMFString, set1Value(c, entryline));
  }

  delete [] olddata;
  // olddata = NULL;
}

#undef PRIVATE
