#ifndef COIN_SOPROFILINGREPORTGENERATOR_H
#define COIN_SOPROFILINGREPORTGENERATOR_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/lists/SbList.h>

class SbProfilingData;
class SbProfilingReportSortCriteria;   // opaque internal
class SbProfilingReportPrintCriteria;  // opaque internal

class COIN_DLL_API SoProfilingReportGenerator {
public:
  static void init(void);

  enum Column {
    NAME,
    TYPE,
    COUNT,
    TIME_SECS,
    TIME_SECS_MAX,
    TIME_SECS_AVG,
    TIME_MSECS,
    TIME_MSECS_MAX,
    TIME_MSECS_AVG,
    TIME_PERCENT,
    TIME_PERCENT_MAX,
    TIME_PERCENT_AVG,
    MEM_BYTES,
    MEM_KILOBYTES,
    GFX_MEM_BYTES,
    GFX_MEM_KILOBYTES
  };

  enum SortOrder {
    TIME_ASC,
    TIME_DES,
    TIME_MAX_ASC,
    TIME_MAX_DES,
    TIME_AVG_ASC,
    TIME_AVG_DES,
    COUNT_ASC,
    COUNT_DES,
    ALPHANUMERIC_ASC,
    ALPHANUMERIC_DES,
    MEM_ASC,
    MEM_DES,
    GFX_MEM_ASC,
    GFX_MEM_DES
  };

  enum DataCategorization {
    TYPES,
    NAMES,
    NODES
  };

  enum CallbackResponse {
    CONTINUE,
    STOP
  };

  static SbProfilingReportSortCriteria * getReportSortCriteria(const SbList< SortOrder > & order);
  static SbProfilingReportSortCriteria * getDefaultReportSortCriteria(DataCategorization category);

  static SbProfilingReportPrintCriteria * getReportPrintCriteria(const SbList< Column > & order);
  static SbProfilingReportPrintCriteria * getDefaultReportPrintCriteria(DataCategorization category);
  static void freeCriteria(SbProfilingReportSortCriteria * criteria);
  static void freeCriteria(SbProfilingReportPrintCriteria * criteria);

  typedef CallbackResponse ReportCB(void * userdata, int entrynum, const char * text);

  static void generate(const SbProfilingData & data,
                       DataCategorization categorization,
                       SbProfilingReportSortCriteria * sort,
                       SbProfilingReportPrintCriteria * print,
                       int count,
                       SbBool addheader,
                       ReportCB * reportcallback,
                       void * userdata);

  static CallbackResponse stdoutCB(void * userdata, int entrynum, const char * text);
  static CallbackResponse stderrCB(void * userdata, int entrynum, const char * text);

}; // SoProfilingReportGenerator

#endif // !COIN_SOPROFILINGREPORTGENERATOR_H
