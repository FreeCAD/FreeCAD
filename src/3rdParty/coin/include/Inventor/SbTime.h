#ifndef COIN_SBTIME_H
#define COIN_SBTIME_H

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

#include <cstdio>

#include <Inventor/system/inttypes.h>
#include <Inventor/SbBasic.h>
#include <Inventor/SbString.h>

// Avoid problem with Microsoft Visual C++ Win32 API headers (they
// #define "max" (in 3 different header files, no less)).
#ifdef max
#define SBTIME_UNDEF_MAX
#undef max
#endif // max


class COIN_DLL_API SbTime {
public:
  SbTime(void);
  SbTime(const double sec);
  SbTime(const int32_t sec, const long usec);
  SbTime(const struct timeval * const tv);
  static SbTime getTimeOfDay(void);
  void setToTimeOfDay(void);
  static SbTime zero(void);

  static SbTime max(void);
  static SbTime maxTime(void);
  static void sleep(int msec);
  void setValue(const double sec);
  void setValue(const int32_t sec, const long usec);
  void setValue(const struct timeval * const tv);
  void setMsecValue(const unsigned long msec);
  double getValue(void) const;
  void getValue(time_t & sec, long & usec) const;
  void getValue(struct timeval * tv) const;
  unsigned long getMsecValue(void) const;
  SbString format(const char * const fmt = "%S.%i") const;
  SbString formatDate(const char * const fmt = NULL) const;
  SbBool parsedate(const char * const date);
  friend COIN_DLL_API SbTime operator +(const SbTime & t0, const SbTime & t1);
  friend COIN_DLL_API SbTime operator -(const SbTime & t0, const SbTime & t1);
  SbTime & operator +=(const SbTime & tm);
  SbTime & operator -=(const SbTime & tm);
  SbTime operator-(void) const;
  friend COIN_DLL_API SbTime operator *(const double s, const SbTime & tm);
  friend COIN_DLL_API SbTime operator *(const SbTime & tm, const double s);
  friend COIN_DLL_API SbTime operator /(const SbTime & tm, const double s);
  SbTime & operator *=(const double s);
  SbTime & operator /=(const double s);
  double operator /(const SbTime & tm) const;
  SbTime operator %(const SbTime & tm) const;
  int operator ==(const SbTime & tm) const;
  int operator !=(const SbTime & tm) const;
  SbBool operator <(const SbTime & tm) const;
  SbBool operator >(const SbTime & tm) const;
  SbBool operator <=(const SbTime & tm) const;
  SbBool operator >=(const SbTime & tm) const;

  void print(FILE * fp) const;

private:
  double dtime;
  void addToString(SbString & str, const double val) const;
};

COIN_DLL_API SbTime operator +(const SbTime & t0, const SbTime & t1);
COIN_DLL_API SbTime operator -(const SbTime & t0, const SbTime & t1);
COIN_DLL_API SbTime operator *(const double s, const SbTime & tm);
COIN_DLL_API SbTime operator *(const SbTime & tm, const double s);
COIN_DLL_API SbTime operator /(const SbTime & tm, const double s);

// Avoid problem with Microsoft Win32 API headers (see above).
// Redefine macro max() back to a definition compatible with what it
// is in the MSVC header files.
#ifdef SBTIME_UNDEF_MAX
#define max(a,b) (((a) > (b)) ? (a) : (b))
#undef SBTIME_UNDEF_MAX
#endif // SBTIME_UNDEF_MAX

#endif // !COIN_SBTIME_H
