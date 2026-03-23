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

#include <Inventor/annex/Profiler/utils/SoProfilingReportGenerator.h>
#include "coindefs.h"

#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

#include <vector>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/threads/SbMutex.h>
#include <Inventor/annex/Profiler/SbProfilingData.h>
#include "tidbitsp.h"

// *************************************************************************

/*!
  \class SoProfilingReportGenerator SoProfilingReportGenerator.h Profiler/tools/SoProfilingReportGenerator.h
  \brief Convenience report generator functionality.

  With this static class, you can conveniently produce a profiling
  data report from an SbProfilingData object.

  \since Coin 3.0
  \ingroup coin_profiler
*/

// *************************************************************************

class SoProfilingReportGeneratorP {
public:
  static SbMutex * mutex;

  typedef int SortFunction(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpTimeAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpTimeDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpTimeMaxAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpTimeMaxDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpTimeAvgAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpTimeAvgDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpCountAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpCountDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpAlphanumericAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpAlphanumericDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpMemAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpMemDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpGfxMemAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);
  static int cmpGfxMemDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2);

  typedef void PrintFunction(const SbProfilingData & data, SbString & string, int idx);
  static void printName(const SbProfilingData & data, SbString & string, int idx);
  static void printType(const SbProfilingData & data, SbString & string, int idx);
  static void printCount(const SbProfilingData & data, SbString & string, int idx);
  static void printTimeSecs(const SbProfilingData & data, SbString & string, int idx);
  static void printTimeSecsMax(const SbProfilingData & data, SbString & string, int idx);
  static void printTimeSecsAvg(const SbProfilingData & data, SbString & string, int idx);
  static void printTimeMSecs(const SbProfilingData & data, SbString & string, int idx);
  static void printTimeMSecsMax(const SbProfilingData & data, SbString & string, int idx);
  static void printTimeMSecsAvg(const SbProfilingData & data, SbString & string, int idx);
  static void printTimePercent(const SbProfilingData & data, SbString & string, int idx);
  static void printTimePercentMax(const SbProfilingData & data, SbString & string, int idx);
  static void printTimePercentAvg(const SbProfilingData & data, SbString & string, int idx);
  static void printMemBytes(const SbProfilingData & data, SbString & string, int idx);
  static void printMemKilobytes(const SbProfilingData & data, SbString & string, int idx);
  static void printGfxMemBytes(const SbProfilingData & data, SbString & string, int idx);
  static void printGfxMemKilobytes(const SbProfilingData & data, SbString & string, int idx);

};

SbMutex * SoProfilingReportGeneratorP::mutex = NULL;

// *************************************************************************

class SbProfilingReportSortCriteria {
public:
  SbProfilingReportSortCriteria(void)
    : numfunctions(0), functions(NULL)
  {
  }

  ~SbProfilingReportSortCriteria(void)
  {
    delete [] this->functions;
  }

  int numfunctions;
  SoProfilingReportGeneratorP::SortFunction ** functions;
};

/*!
  Returns a sorting criteria setting object that will make generate()
  sort the results based on the given argument list in that left-to-right
  priority order.

  Always end the argument list with TERMINATE_ARGLIST.

  \sa freeCriteria
*/
SbProfilingReportSortCriteria *
SoProfilingReportGenerator::getReportSortCriteria(const SbList< SortOrder > & order)
{
  SbProfilingReportSortCriteria * criteria = new SbProfilingReportSortCriteria;

  criteria->numfunctions=order.getLength();

  criteria->functions =
    new SoProfilingReportGeneratorP::SortFunction * [ criteria->numfunctions ];

  for (int idx=0;idx<order.getLength();++idx) {
    switch (order[idx]) {
    case SoProfilingReportGenerator::TIME_ASC:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpTimeAsc;
      break;
    case SoProfilingReportGenerator::TIME_DES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpTimeDes;
      break;
    case SoProfilingReportGenerator::TIME_MAX_ASC:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpTimeMaxAsc;
      break;
    case SoProfilingReportGenerator::TIME_MAX_DES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpTimeMaxDes;
      break;
    case SoProfilingReportGenerator::TIME_AVG_ASC:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpTimeAvgAsc;
      break;
    case SoProfilingReportGenerator::TIME_AVG_DES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpTimeAvgDes;
      break;
    case SoProfilingReportGenerator::COUNT_ASC:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpCountAsc;
      break;
    case SoProfilingReportGenerator::COUNT_DES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpCountDes;
      break;
    case SoProfilingReportGenerator::ALPHANUMERIC_ASC:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpAlphanumericAsc;
      break;
    case SoProfilingReportGenerator::ALPHANUMERIC_DES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpAlphanumericDes;
      break;
    case SoProfilingReportGenerator::MEM_ASC:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpMemAsc;
      break;
    case SoProfilingReportGenerator::MEM_DES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpMemDes;
      break;
    case SoProfilingReportGenerator::GFX_MEM_ASC:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpGfxMemAsc;
      break;
    case SoProfilingReportGenerator::GFX_MEM_DES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::cmpGfxMemDes;
      break;
    default:
      assert(!"not a supported sort order");
      break;
    }
  }

  return criteria;
}

/*!
  Returns a sensible default that can be used for a given profiling
  data categorization.
*/
SbProfilingReportSortCriteria *
SoProfilingReportGenerator::getDefaultReportSortCriteria(DataCategorization category)
{
  SbList< SortOrder > order(1);
  switch (category) {
  case NODES:
  case NAMES:
    order.append(TIME_DES);
    break;
  case TYPES:
    order.append(TIME_MAX_DES);
    break;
  default:
    assert(!"not a supported sort order");
    break;
  }
  return getReportSortCriteria(order);
}

void
SoProfilingReportGenerator::freeCriteria(SbProfilingReportSortCriteria * criteria)
{
  assert(criteria);
  delete criteria;
}

// *************************************************************************

class SbProfilingReportPrintCriteria {
public:
  SbProfilingReportPrintCriteria(void)
    : numfunctions(0), functions(NULL), needstringlengths(FALSE)
  {
  }
  ~SbProfilingReportPrintCriteria(void) {
    delete [] this->functions;
  }

  int numfunctions;
  SoProfilingReportGeneratorP::PrintFunction ** functions;
  SbBool needstringlengths;
};

/*!
  Returns a printing criteria setting object that will make generate()
  send a formatted string as the text argument based on the given argument
  list.  If you force TERMINATE_ARGLIST into the first argument, the
  text string will be empty.

  Always end the argument list with TERMINATE_ARGLIST.

  \sa freeCriteria
*/
SbProfilingReportPrintCriteria *
SoProfilingReportGenerator::getReportPrintCriteria(const SbList<Column> & order)
{
  SbProfilingReportPrintCriteria * criteria = new SbProfilingReportPrintCriteria;
  criteria->numfunctions = order.getLength();

  criteria->functions =
    new SoProfilingReportGeneratorP::PrintFunction * [ criteria->numfunctions ];

  for (int idx=0;idx<order.getLength();++idx) {
    switch (order[idx]) {
    case SoProfilingReportGenerator::NAME:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printName;
      criteria->needstringlengths = TRUE;
      break;
    case SoProfilingReportGenerator::TYPE:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printType;
      criteria->needstringlengths = TRUE;
      break;
    case SoProfilingReportGenerator::COUNT:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printCount;
      break;
    case SoProfilingReportGenerator::TIME_SECS:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimeSecs;
      break;
    case SoProfilingReportGenerator::TIME_SECS_MAX:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimeSecsMax;
      break;
    case SoProfilingReportGenerator::TIME_SECS_AVG:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimeSecsAvg;
      break;
    case SoProfilingReportGenerator::TIME_MSECS:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimeMSecs;
      break;
    case SoProfilingReportGenerator::TIME_MSECS_MAX:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimeMSecsMax;
      break;
    case SoProfilingReportGenerator::TIME_MSECS_AVG:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimeMSecsAvg;
      break;
    case SoProfilingReportGenerator::TIME_PERCENT:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimePercent;
      break;
    case SoProfilingReportGenerator::TIME_PERCENT_MAX:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimePercentMax;
      break;
    case SoProfilingReportGenerator::TIME_PERCENT_AVG:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printTimePercentAvg;
      break;
    case SoProfilingReportGenerator::MEM_BYTES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printMemBytes;
      break;
    case SoProfilingReportGenerator::MEM_KILOBYTES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printMemKilobytes;
      break;
    case SoProfilingReportGenerator::GFX_MEM_BYTES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printGfxMemBytes;
      break;
    case SoProfilingReportGenerator::GFX_MEM_KILOBYTES:
      criteria->functions[idx] = SoProfilingReportGeneratorP::printGfxMemKilobytes;
      break;
    default:
      assert(!"unsupported column/format id");
      break;
    }
  }

  return criteria;
}

/*!
  Returns a default printing criteria object that can be used sensibly with
  the given profiling data categorization.
*/
SbProfilingReportPrintCriteria *
SoProfilingReportGenerator::getDefaultReportPrintCriteria(DataCategorization category)
{
  SbList<Column> order;
  switch (category) {
  case TYPES:
    order.append(TYPE);
    order.append(COUNT);
    order.append(TIME_PERCENT);
    order.append(TIME_MSECS);
    order.append(TIME_MSECS_MAX);
    break;
  case NAMES:
    order.append(NAME);
    order.append(TIME_PERCENT);
    order.append(TIME_MSECS);
    break;
  case NODES:
    order.append(NAME);
    order.append(TIME_PERCENT);
    order.append(TIME_MSECS);
    order.append(MEM_KILOBYTES);
    break;
  default:
    assert(!"unsupported category");
    break;
  }
  return getReportPrintCriteria(order);
}

void
SoProfilingReportGenerator::freeCriteria(SbProfilingReportPrintCriteria * criteria)
{
  assert(criteria);
  delete criteria;
}

// *************************************************************************

static const SbProfilingData * profdata = NULL;
static const int * arraystart = NULL;
static const int * arrayend = NULL;
static SoProfilingReportGenerator::DataCategorization sortcategory = SoProfilingReportGenerator::TYPES;
static SbProfilingReportSortCriteria * sortingconfig = NULL;
static SbList<SbProfilingNodeNameKey> * namekeys = NULL;
static SbList<SbProfilingNodeTypeKey> * typekeys = NULL;
static int longestnamelength = 0;
static int longesttypenamelength = 0;

int
gencompare(const void * ptr1, const void * ptr2)
{
  if (ptr1 < arraystart || ptr1 > arrayend || ptr2 < arraystart || ptr2 > arrayend) {
    assert("sorting not delayed for correct");
    return 0;
  }
  assert(sortingconfig);

  const int idx1 = (static_cast<const int *>(ptr1))[0];
  const int idx2 = (static_cast<const int *>(ptr2))[0];

  int result = 0, sorteridx = 0;
  while (result == 0 && sorteridx < sortingconfig->numfunctions) {
    result = sortingconfig->functions[sorteridx](*profdata, sortcategory, idx1, idx2);
    ++sorteridx;
  }
  return result;
}

/*!
  Allocate mutexes and other such infrastructure constructs needed
  by the report generator.
*/
void
SoProfilingReportGenerator::init(void)
{
  assert(SoProfilingReportGeneratorP::mutex == NULL);
  SoProfilingReportGeneratorP::mutex = new SbMutex;
}

namespace {
// class to make sure the mutex is always unlocked again when leaving scope
class MutexLocker {
  SbMutex * const mutex;
public:
  MutexLocker(SbMutex * m) : mutex(m) {
    assert(mutex);
    mutex->lock();
  }
  ~MutexLocker(void) {
    mutex->unlock();
  }
};
}

#define OUTPUT_PADDING "  "
/*!
  Generate a sorted, formatted report, by calling a callback until the
  number of entries are exhausted or the callback returns STOP.
*/
void
SoProfilingReportGenerator::generate(const SbProfilingData & data,
                                     DataCategorization categorization,
                                     SbProfilingReportSortCriteria * sort,
                                     SbProfilingReportPrintCriteria * print,
                                     int count,
                                     SbBool addheader,
                                     ReportCB * reportcallback,
                                     void * userdata)
{
  assert(reportcallback);
  assert(sort);
  assert(print);

  MutexLocker locker(SoProfilingReportGeneratorP::mutex); // lock mutex until scope is exited

  profdata = &data;
  sortingconfig = sort;

  if (print->needstringlengths) {
    longestnamelength = data.getLongestNameLength();
    longesttypenamelength = data.getLongestTypeNameLength();
  }

  if (categorization == NODES) {
    int c = 0;
    const int numindexes = data.getNumNodeEntries();
    if (numindexes == 0) {
      profdata = NULL;
      sortingconfig = NULL;
      return;
    }
    std::vector<int> indexarray(numindexes);
    for (c = 0; c < numindexes; ++c) {
      indexarray[c] = c;
    }

    sortcategory = NODES;
    arraystart = &indexarray[0];
    arrayend = &indexarray[numindexes-1];

    qsort(indexarray.data(), numindexes, sizeof(int), gencompare);

    // output
    const int maxindexes = (count > 0) ? SbMin(numindexes, count) : numindexes;
    c = addheader ? -1 : 0;
    for (; c < maxindexes; ++c) {
      SbString text;
      int entryidx = -1;
      if (c == -1) {
        for (int i = 0; i < print->numfunctions; ++i) {
          SbString part;
          print->functions[i](data, part, -1);
          text += part;
          if ((i + 1) < print->numfunctions) { text += OUTPUT_PADDING; }
        }
      } else {
        entryidx = indexarray[c];
        for (int i = 0; i < print->numfunctions; ++i) {
          SbString part;
          print->functions[i](data, part, entryidx);
          text += part;
          if ((i + 1) < print->numfunctions) { text += OUTPUT_PADDING; }
        }
      }
      /*CallbackResponse response = */reportcallback(userdata, entryidx, text.getString());
    }
  }
  else if (categorization == NAMES) {
    int c = 0;
    if (namekeys == NULL) {
      namekeys = new SbList<SbProfilingNodeNameKey>;
    } else {
      namekeys->truncate(0);
    }
    data.getStatsForNamesKeyList(*namekeys);

    const int numindexes = namekeys->getLength();
    if (numindexes == 0) {
      profdata = NULL;
      sortingconfig = NULL;
      return;
    }
    std::vector<int> indexarray(numindexes);
    for (c = 0; c < numindexes; ++c) {
      indexarray[c] = c;
    }

    sortcategory = NAMES;
    arraystart = &indexarray[0];
    arrayend = &indexarray[numindexes-1];

    qsort(indexarray.data(), numindexes, sizeof(int), gencompare);

    // output
    const int maxindexes = (count > 0) ? SbMin(numindexes, count) : numindexes;
    c = addheader ? -1 : 0;
    for (; c < maxindexes; ++c) {
      SbString text;
      int entryidx = -1;
      if (c == -1) {
        for (int i = 0; i < print->numfunctions; ++i) {
          SbString part;
          print->functions[i](data, part, -1);
          text += part;
          if ((i + 1) < print->numfunctions) { text += OUTPUT_PADDING; }
        }
      } else {
        const SbProfilingNodeNameKey namekey = (*namekeys)[indexarray[c]];
        entryidx = namekeys->find(namekey);
        for (int i = 0; i < print->numfunctions; ++i) {
          SbString part;
          print->functions[i](data, part, entryidx);
          text += part;
          if ((i + 1) < print->numfunctions) { text += OUTPUT_PADDING; }
        }
      }
      /*CallbackResponse response = */reportcallback(userdata, entryidx, text.getString());
    }
  }
  else if (categorization == TYPES) {
    int c = 0;
    if (typekeys == NULL) {
      typekeys = new SbList<SbProfilingNodeTypeKey>;
    } else {
      typekeys->truncate(0);
    }
    data.getStatsForTypesKeyList(*typekeys);

    const int numindexes = typekeys->getLength();
    if (numindexes == 0) {
      profdata = NULL;
      sortingconfig = NULL;
      return;
    }
    std::vector<int> indexarray(numindexes);
    for (c = 0; c < numindexes; ++c) {
      indexarray[c] = c;
    }

    sortcategory = TYPES;
    arraystart = &indexarray[0];
    arrayend = &indexarray[numindexes-1];

    qsort(indexarray.data(), numindexes, sizeof(int), gencompare);

    // output
    const int maxindexes = (count > 0) ? SbMin(numindexes, count) : numindexes;
    c = addheader ? -1 : 0;
    for (; c < maxindexes; ++c) {
      SbString text;
      int entryidx = -1;
      if (c == -1) {
        for (int i = 0; i < print->numfunctions; ++i) {
          SbString part;
          print->functions[i](data, part, -1);
          text += part;
          if ((i + 1) < print->numfunctions) { text += OUTPUT_PADDING; }
        }
      } else {
        const SbProfilingNodeTypeKey typekey = (*typekeys)[indexarray[c]];
        entryidx = typekeys->find(typekey);
        for (int i = 0; i < print->numfunctions; ++i) {
          SbString part;
          print->functions[i](data, part, entryidx);
          text += part;
          if ((i + 1) < print->numfunctions) { text += OUTPUT_PADDING; }
        }
      }
      /*CallbackResponse response = */reportcallback(userdata, entryidx, text.getString());
    }
  }
  else {
    assert(!"no such data categorization implemented");
  }

  profdata = NULL;
  sortingconfig = NULL;
  arraystart = NULL;
  arrayend = NULL;
}

#undef OUTPUT_PADDING

// *************************************************************************
// QSORT() HOOKS

int
SoProfilingReportGeneratorP::cmpTimeAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  double diff = 0.0;
  switch (category) {
  case SoProfilingReportGenerator::NODES:
  {
    const SbTime time1(data.getNodeTiming(idx1));
    const SbTime time2(data.getNodeTiming(idx2));
    diff = time1.getValue() - time2.getValue();
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total1, max1, total2, max2;
    uint32_t count1, count2;
    data.getStatsForName((*namekeys)[idx1], total1, max1, count1);
    data.getStatsForName((*namekeys)[idx2], total2, max2, count2);
    diff = total1.getValue() - total2.getValue();
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total1, max1, total2, max2;
    uint32_t count1, count2;
    data.getStatsForType((*typekeys)[idx1], total1, max1, count1);
    data.getStatsForType((*typekeys)[idx2], total2, max2, count2);
    diff = total1.getValue() - total2.getValue();
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }

  if (diff < 0.0) return -1;
  else if (diff > 0.0) return 1;
  else return 0;
}

int
SoProfilingReportGeneratorP::cmpTimeDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  return -SoProfilingReportGeneratorP::cmpTimeAsc(data, category, idx1, idx2);
}

int
SoProfilingReportGeneratorP::cmpTimeMaxAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  double diff = 0.0;
  switch (category) {
  case SoProfilingReportGenerator::NODES:
    // no average or maximum when looking at single nodes
    return SoProfilingReportGeneratorP::cmpTimeAsc(data, category, idx1, idx2);
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total1, max1, total2, max2;
    uint32_t count1, count2;
    data.getStatsForName((*namekeys)[idx1], total1, max1, count1);
    data.getStatsForName((*namekeys)[idx2], total2, max2, count2);
    diff = max1.getValue() - max2.getValue();
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total1, max1, total2, max2;
    uint32_t count1, count2;
    data.getStatsForType((*typekeys)[idx1], total1, max1, count1);
    data.getStatsForType((*typekeys)[idx2], total2, max2, count2);
    diff = max1.getValue() - max2.getValue();
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
  if (diff < 0.0) return -1;
  else if (diff > 0.0) return 1;
  else return 0;
}

int
SoProfilingReportGeneratorP::cmpTimeMaxDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  return -SoProfilingReportGeneratorP::cmpTimeMaxAsc(data, category, idx1, idx2);
}

int
SoProfilingReportGeneratorP::cmpTimeAvgAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  double diff = 0.0;
  switch (category) {
  case SoProfilingReportGenerator::NODES:
    // no average or maximum when looking at single nodes
    return SoProfilingReportGeneratorP::cmpTimeAsc(data, category, idx1, idx2);
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total1, max1, total2, max2;
    uint32_t count1, count2;
    data.getStatsForName((*namekeys)[idx1], total1, max1, count1);
    data.getStatsForName((*namekeys)[idx2], total2, max2, count2);
    diff = (total1.getValue() / static_cast<float>(count1))
      - (total2.getValue() / static_cast<float>(count2));
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total1, max1, total2, max2;
    uint32_t count1, count2;
    data.getStatsForType((*typekeys)[idx1], total1, max1, count1);
    data.getStatsForType((*typekeys)[idx2], total2, max2, count2);
    diff = (total1.getValue() / static_cast<float>(count1))
      - (total2.getValue() / static_cast<float>(count2));
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
  if (diff < 0.0) return -1;
  else if (diff > 0.0) return 1;
  else return 0;
}

int
SoProfilingReportGeneratorP::cmpTimeAvgDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  return -SoProfilingReportGeneratorP::cmpTimeAvgAsc(data, category, idx1, idx2);
}

int
SoProfilingReportGeneratorP::cmpCountAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  switch (category) {
  case SoProfilingReportGenerator::NODES:
    // count always 1
    return 0;
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total1, max1, total2, max2;
    uint32_t count1, count2;
    data.getStatsForName((*namekeys)[idx1], total1, max1, count1);
    data.getStatsForName((*namekeys)[idx2], total2, max2, count2);
    return static_cast<int>(count1 - count2);
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total1, max1, total2, max2;
    uint32_t count1, count2;
    data.getStatsForType((*typekeys)[idx1], total1, max1, count1);
    data.getStatsForType((*typekeys)[idx2], total2, max2, count2);
    return static_cast<int>(count1 - count2);
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
  return 0;
}

int
SoProfilingReportGeneratorP::cmpCountDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  return -SoProfilingReportGeneratorP::cmpCountAsc(data, category, idx1, idx2);
}

int
SoProfilingReportGeneratorP::cmpAlphanumericAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  switch (category) {
  case SoProfilingReportGenerator::NODES:
  {
    // node name, fallback type name
    SbName name1(data.getNodeName(idx1));
    if (name1 == SbName::empty()) {
      name1 = data.getNodeType(idx1).getName();
    }
    SbName name2(data.getNodeName(idx2));
    if (name2 == SbName::empty()) {
      name2 = data.getNodeType(idx2).getName();
    }
    return strcmp(name1.getString(), name2.getString());
  }
  case SoProfilingReportGenerator::NAMES:
  {
    // group name
    return strcmp((*namekeys)[idx1], (*namekeys)[idx2]);
  }
  case SoProfilingReportGenerator::TYPES:
  {
    const SoType type1(SoType::fromKey((*typekeys)[idx1]));
    const SoType type2(SoType::fromKey((*typekeys)[idx2]));
    return strcmp(type1.getName().getString(), type2.getName().getString());
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
  return 0;
}

int
SoProfilingReportGeneratorP::cmpAlphanumericDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  return -SoProfilingReportGeneratorP::cmpAlphanumericAsc(data, category, idx1, idx2);
}

int
SoProfilingReportGeneratorP::cmpMemAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  switch (category) {
  case SoProfilingReportGenerator::NODES:
  {
    size_t footprint1 = data.getNodeFootprint(idx1, SbProfilingData::MEMORY_SIZE);
    size_t footprint2 = data.getNodeFootprint(idx2, SbProfilingData::MEMORY_SIZE);
    return static_cast<int>(footprint1 - footprint2);
  }
  case SoProfilingReportGenerator::NAMES:
    // not implemented
    break;
  case SoProfilingReportGenerator::TYPES:
    // not supported
    break;
  default:
    assert(!"unsupported report categorization");
    break;
  }
  return 0;
}

int
SoProfilingReportGeneratorP::cmpMemDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  return -SoProfilingReportGeneratorP::cmpMemAsc(data, category, idx1, idx2);
}

int
SoProfilingReportGeneratorP::cmpGfxMemAsc(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  switch (category) {
  case SoProfilingReportGenerator::NODES:
  {
    size_t footprint1 = data.getNodeFootprint(idx1, SbProfilingData::VIDEO_MEMORY_SIZE);
    size_t footprint2 = data.getNodeFootprint(idx2, SbProfilingData::VIDEO_MEMORY_SIZE);
    return static_cast<int>(footprint1 - footprint2);
  }
  case SoProfilingReportGenerator::NAMES:
    // not implemented
    break;
  case SoProfilingReportGenerator::TYPES:
    // no implemented
    break;
  default:
    assert(!"unsupported report categorization");
    break;
  }
  return 0;
}

int
SoProfilingReportGeneratorP::cmpGfxMemDes(const SbProfilingData & data, SoProfilingReportGenerator::DataCategorization category, int idx1, int idx2)
{
  return -SoProfilingReportGeneratorP::cmpGfxMemAsc(data, category, idx1, idx2);
}

// *************************************************************************
// PRETTY-PRINTING HOOKS

void
SoProfilingReportGeneratorP::printName(const SbProfilingData & data, SbString & string, int entryidx)
{
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    if (entryidx == -1) {
      SbString format;
      format.sprintf("%%-%ds", SbMax(longestnamelength, longesttypenamelength));
      string.sprintf(format.getString(), "NAME");
    } else {
      SbName nodename(data.getNodeName(entryidx));
      if (nodename == SbName::empty()) {
        nodename = data.getNodeType(entryidx).getName();
      }
      SbString format;
      format.sprintf("%%-%ds", SbMax(longestnamelength, longesttypenamelength));
      string.sprintf(format.getString(), nodename.getString());
    }
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    if (entryidx == -1) {
      SbString format;
      format.sprintf("%%-%ds", longestnamelength);
      string.sprintf(format.getString(), "NAME");
    } else {
      SbName name((*namekeys)[entryidx]);
      SbString format;
      format.sprintf("%%-%ds", longestnamelength);
      string.sprintf(format.getString(), name.getString());
    }
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    if (entryidx == -1) {
      SbString format;
      format.sprintf("%%-%ds", longesttypenamelength);
      string.sprintf(format.getString(), "TYPE");
    } else {
      SoType nodetype = SoType::fromKey((*typekeys)[entryidx]);
      SbString format;
      format.sprintf("%%-%ds", longesttypenamelength);
      string.sprintf(format.getString(), nodetype.getName().getString());
    }
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printType(const SbProfilingData & data, SbString & string, int entryidx)
{
  SbString format;
  format.sprintf("%%-%ds", longesttypenamelength);
  if (entryidx == -1) {
    string.sprintf(format.getString(), "TYPE");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbName nodetype(data.getNodeType(entryidx).getName());
    string.sprintf(format.getString(), nodetype.getString());
    break;
  }
  case SoProfilingReportGenerator::NAMES:
    string.sprintf(format.getString(), "???");
    break;
  case SoProfilingReportGenerator::TYPES:
  {
    SbName nodetype(SoType::fromKey((*typekeys)[entryidx]).getName());
    string.sprintf(format.getString(), nodetype.getString());
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printCount(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%4d";
  if (entryidx == -1) {
    string.sprintf("%4s", "NUM");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    string.sprintf(formatstring, 1);
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, count);
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, count);
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimeSecs(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%8.6fs";
  if (entryidx == -1) {
    string.sprintf("%9s", "TOTAL");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, timing.getValue());
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, total.getValue());
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, total.getValue());
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimeSecsMax(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%8.6fs";
  if (entryidx == -1) {
    string.sprintf("%9s", "MAXIMUM");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, timing.getValue());
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, max.getValue());
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, max.getValue());
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimeSecsAvg(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%8.6fs";
  if (entryidx == -1) {
    string.sprintf("%9s", "AVERAGE");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, timing.getValue());
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, (total.getValue() / static_cast<double>(count)));
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, (total.getValue() / static_cast<double>(count)));
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimeMSecs(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%6.2fms";
  if (entryidx == -1) {
    string.sprintf("%8s", "TOTAL");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, timing.getValue() * 1000.0);
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, total.getValue() * 1000.0);
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, total.getValue() * 1000.0);
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimeMSecsMax(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%6.2fms";
  if (entryidx == -1) {
    string.sprintf("%8s", "MAXIMUM");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, timing.getValue() * 1000.0);
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, max.getValue() * 1000.0);
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, max.getValue() * 1000.0);
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimeMSecsAvg(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%6.2fms";
  if (entryidx == -1) {
    string.sprintf("%8s", "AVERAGE");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, timing.getValue() * 1000.0);
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, (total.getValue() * 1000.0) / static_cast<double>(count));
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, (total.getValue() * 1000.0) / static_cast<double>(count));
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimePercent(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%5.1f%%";
  if (entryidx == -1) {
    string.sprintf("%6s", "TOTAL");
    return;
  }
  const SbTime duration = data.getActionDuration();
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, (timing.getValue() * 100.0) / duration.getValue());
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, (total.getValue() * 100.0) / duration.getValue());
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, (total.getValue() * 100.0) / duration.getValue());
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimePercentMax(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%5.1f%%";
  if (entryidx == -1) {
    string.sprintf("%6s", "MAX");
    return;
  }
  const SbTime duration = data.getActionDuration();
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, timing.getValue() * 100.0 / duration.getValue());
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, (max.getValue() * 100.0) / duration.getValue());
    break;
  }
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, (max.getValue() * 100.0) / duration.getValue());
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printTimePercentAvg(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%5.1f%%";
  if (entryidx == -1) {
    string.sprintf("%6s", "AVG");
    return;
  }
  const SbTime duration = data.getActionDuration();
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    SbTime timing = data.getNodeTiming(entryidx);
    string.sprintf(formatstring, timing.getValue() * 100.0 / duration.getValue());
    break;
  }
  case SoProfilingReportGenerator::NAMES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForName((*namekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, ((total.getValue() / static_cast<double>(count)) * 100.0) / duration.getValue());
    break;
  }
    break;
  case SoProfilingReportGenerator::TYPES:
  {
    SbTime total, max;
    uint32_t count;
    data.getStatsForType((*typekeys)[entryidx], total, max, count);
    string.sprintf(formatstring, ((total.getValue() / static_cast<double>(count)) * 100.0) / duration.getValue());
    break;
  }
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printMemBytes(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%8ldB";
  if (entryidx == -1) {
    string.sprintf("%9s", "MEMORY");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    size_t footprint = data.getNodeFootprint(entryidx, SbProfilingData::MEMORY_SIZE);
    string.sprintf(formatstring, footprint);
    break;
  }
  case SoProfilingReportGenerator::NAMES:
    string.sprintf(formatstring, 0);
    break;
  case SoProfilingReportGenerator::TYPES:
    string.sprintf(formatstring, 0);
    break;
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printMemKilobytes(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%6.1fKB";
  if (entryidx == -1) {
    string.sprintf("%8s", "MEMORY");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    size_t footprint = data.getNodeFootprint(entryidx, SbProfilingData::MEMORY_SIZE);
    string.sprintf(formatstring, static_cast<double>(footprint) / 1024.0);
    break;
  }
  case SoProfilingReportGenerator::NAMES:
    string.sprintf(formatstring, 0.0);
    break;
  case SoProfilingReportGenerator::TYPES:
    string.sprintf(formatstring, 0.0);
    break;
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printGfxMemBytes(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%8ldB";
  if (entryidx == -1) {
    string.sprintf("%9s", "GFX MEM");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    size_t footprint = data.getNodeFootprint(entryidx, SbProfilingData::VIDEO_MEMORY_SIZE);
    string.sprintf(formatstring, footprint);
    break;
  }
  case SoProfilingReportGenerator::NAMES:
    string.sprintf(formatstring, 0);
    break;
  case SoProfilingReportGenerator::TYPES:
    string.sprintf(formatstring, 0);
    break;
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

void
SoProfilingReportGeneratorP::printGfxMemKilobytes(const SbProfilingData & data, SbString & string, int entryidx)
{
  static const char formatstring[] = "%6.1fKB";
  if (entryidx == -1) {
    string.sprintf("%8s", "GFX MEM");
    return;
  }
  switch (sortcategory) {
  case SoProfilingReportGenerator::NODES:
  {
    size_t footprint = data.getNodeFootprint(entryidx, SbProfilingData::VIDEO_MEMORY_SIZE);
    string.sprintf(formatstring, static_cast<double>(footprint) / 1024.0);
    break;
  }
  case SoProfilingReportGenerator::NAMES:
    string.sprintf(formatstring, 0.0);
    break;
  case SoProfilingReportGenerator::TYPES:
    string.sprintf(formatstring, 0.0);
    break;
  default:
    assert(!"unsupported report categorization");
    break;
  }
}

// *************************************************************************

/*!
  Ready-made callback that can be used with generate() to print to stdout.

  \sa generate
*/
SoProfilingReportGenerator::CallbackResponse
SoProfilingReportGenerator::stdoutCB(void * COIN_UNUSED_ARG(userdata), int COIN_UNUSED_ARG(entryidx), const char * text)
{
  fprintf(coin_get_stdout(), "%s\n", text);
  return CONTINUE;
}

/*!
  Ready-made callback that can be used with generate() to print to stderr.

  \sa generate
*/
SoProfilingReportGenerator::CallbackResponse
SoProfilingReportGenerator::stderrCB(void * COIN_UNUSED_ARG(userdata), int COIN_UNUSED_ARG(entryidx), const char * text)
{
  fprintf(coin_get_stderr(), "%s\n", text);
  return CONTINUE;
}
