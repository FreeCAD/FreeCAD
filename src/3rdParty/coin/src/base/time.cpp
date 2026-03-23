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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif /* HAVE_WINDOWS_H */

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* gettimeofday() */
#endif /* HAVE_SYS_TIME_H */

#ifdef HAVE_TIME_H
#include <ctime>
#endif /* HAVE_TIME_H */

/* On Mac OS X / Darwin, timeb.h uses time_t from time.h, so the order
   of these two includes needs to be preserved. */
#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h> /* struct _timeb or struct timeb */
#endif /* HAVE_SYS_TIMEB_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* gettimeofday() */
#endif /* HAVE_UNISTD_H */

#include <cassert>

#include <Inventor/C/base/time.h>
#include <Inventor/C/errors/debugerror.h>

#include "coindefs.h"

/* ********************************************************************** */

#ifdef HAVE_QUERYPERFORMANCECOUNTER
static int highperf_available = -1;
static double highperf_start = -1;
static double highperf_tick = -1;
#endif /* HAVE_QUERYPERFORMANCECOUNTER */

/* The Win32 QueryPerformanceCounter() strategy is based on code
   submitted by Jan Peciva (aka PCJohn). */
static SbBool
cc_internal_queryperformancecounter(cc_time * COIN_UNUSED_ARG(t))
{
#ifdef HAVE_QUERYPERFORMANCECOUNTER
  if (highperf_available == -1) {
    LARGE_INTEGER frequency;
    highperf_available = (QueryPerformanceFrequency(&frequency) != 0);
    if (highperf_available) {
      highperf_tick = 1.0 / frequency.QuadPart;

      {
        time_t tt = time(NULL);
        LARGE_INTEGER counter;
        (void)QueryPerformanceCounter(&counter);
        highperf_start = tt - ((double)counter.QuadPart * highperf_tick);
      }
    }
  }

  if (highperf_available) {
    LARGE_INTEGER counter;
    BOOL b = QueryPerformanceCounter(&counter);
    assert(b && "QueryPerformanceCounter() failed even though QueryPerformanceFrequency() worked");
    *t = (double)counter.QuadPart * highperf_tick + highperf_start;
    return TRUE;
  }
  return FALSE;
#else /* !HAVE_QUERYPERFORMANCECOUNTER */
  return FALSE;
#endif /* !HAVE_QUERYPERFORMANCECOUNTER */
}

static SbBool
cc_internal_gettimeofday(cc_time * t)
{
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;
  int result = gettimeofday(&tv, NULL);
  if (COIN_DEBUG && (result < 0)) {
    cc_debugerror_postwarning("SbTime_gettimeofday",
                              "Something went wrong (invalid timezone "
                              "setting?). Result is undefined.");
  }
  *t = tv.tv_sec;
  *t += static_cast<double>((tv.tv_usec))/1000000.0;
  return TRUE;
#else /* !HAVE_GETTIMEOFDAY */
  return FALSE;
#endif /* !HAVE_GETTIMEOFDAY */
}

static SbBool
cc_internal_ftime(cc_time * t)
{
#ifdef HAVE__FTIME
  struct _timeb timebuffer;
  _ftime(&timebuffer);
  /* FIXME: should use timezone field of struct _timeb as well. 20011023 mortene. */
  *t = (double)timebuffer.time + (double)timebuffer.millitm / 1000.0;
  return TRUE;
#elif defined(HAVE_FTIME)
  struct timeb timebuffer;
  /* FIXME: should use timezone field of struct _timeb as well. 20011023 mortene. */
  ftime(&timebuffer);
  *t = static_cast<double>(timebuffer.time) + static_cast<double>(timebuffer.millitm) / 1000.0;
  return TRUE;
#else /* HAVE_FTIME */
  return FALSE;
#endif /* !HAVE_FTIME */
}

/* ********************************************************************** */

/*!
  Returns current time in seconds.
  FIXME: Specify call overhead and resolution.
*/
cc_time
cc_time_gettimeofday(void)
{
  cc_time t = 0.0;
  if (cc_internal_queryperformancecounter(&t)) { return t; }
  if (cc_internal_gettimeofday(&t)) { return t; }
  if (cc_internal_ftime(&t)) { return t; }
  /* FIXME: write better debug output. 20011218 mortene. */
  cc_debugerror_post("cc_time_gettimeofday", "unable to find current time");
  return 0.0;
}

/* ********************************************************************** */
