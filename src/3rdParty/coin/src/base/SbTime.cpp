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
  \class SbTime SbTime.h Inventor/SbTime.h
  \brief The SbTime class instances represents time values.

  \ingroup coin_base

  SbTime is a convenient way of doing system independent
  representation and calculations on time values of high resolution.
*/

// *************************************************************************

#include <Inventor/SbTime.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cerrno>
#include <cmath>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_TIME_H
#include <ctime> // struct timeval (Linux)
#endif // HAVE_TIME_H

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> // struct timeval (IRIX)
#endif // HAVE_SYS_TIME_H

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/base/time.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/threads/thread.h>

#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strlen;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

// FIXME: I don't agree with this willy-nilly, "gut feeling"
// constant. As far as I can tell from a quick look, the exceptions in
// the code using this should be handled in other ways.
//
// E.g. this:
//
//   if (s==0.0) { this->dtime /= s + SMALLEST_DOUBLE_TIMEUNIT; }
//
// ..could better be written as
//
//   if (s==0.0) { this->dtime = DBL_MAX; }
//
// If there are no protests, I'll take care of removing this constant.
//
// 20050526 mortene.

static const double SMALLEST_DOUBLE_TIMEUNIT  = 1.0/1000000.0;

// *************************************************************************

/*!
  The default constructor sets up a time instance of 0 seconds.
*/
SbTime::SbTime(void)
{
  this->setValue(0.0);
}

/*!
  Construct and initialize an SbTime instance to a time specified
  as \a sec seconds.
 */
SbTime::SbTime(const double sec)
{
  this->setValue(sec);
}

/*!
  Construct and initialize an SbTime instance to a date and time
  \a sec number of seconds and \a usec number of microseconds.
 */
SbTime::SbTime(const int32_t sec, const long usec)
{
  this->setValue(sec, usec);
}

/*!
  Construct and initialize an SbTime instance to the date and time
  given by the \a struct \a timeval. For information on the \a timeval
  structure, please consult your system developer documentation.
 */
SbTime::SbTime(const struct timeval * const tv)
{
  this->setValue(tv);
}

/*!
  Returns an SbTime instance with the current clock time. The current
  time will be given as a particular number of seconds and
  microseconds since 00:00:00 January 1, 1970, in Coordinated
  Universal Time (UTC).

  \sa setToTimeOfDay().
 */
SbTime
SbTime::getTimeOfDay(void)
{
  SbTime t(cc_time_gettimeofday());
  return t;
}

/*!
  Set this SbTime to be the current clock time. The current time
  will be given as a particular number of seconds and microseconds since
  00:00:00.00 1st January 1970.

  \sa getTimeOfDay().
 */
void
SbTime::setToTimeOfDay(void)
{
  (*this) = SbTime::getTimeOfDay();
}

/*!
  Returns an SbTime instance representing zero time.

  \sa zero().
 */
SbTime
SbTime::zero(void)
{
  return SbTime(0.0);
}

/*!
  Returns an SbTime instance representing the maximum representable
  time/date.

  \sa zero().
*/
SbTime
SbTime::maxTime(void)
{
  return SbTime(static_cast<double>(INT_MAX) + 0.999999);
}


// This is needed because of the hackery in SbTime.h to avoid problems
// with the Microsoft Visual C++ header files, see comment in
// SbTime.h for additional information.
#ifdef max
#undef max
#endif // max

/*!
  Returns an SbTime instance representing the maximum representable
  time/date.

  This method is not available under Microsoft Windows, as max() crashes with
  a define macro Microsoft has polluted the global namespace with.

  \sa zero().
*/
SbTime
SbTime::max(void)
{
  return SbTime::maxTime();
}

/*!
  Suspends the current thread for \a msec milliseconds.
  
  \sa cc_sleep().

  \since Coin 3.0
*/
void
SbTime::sleep(int msec)
{
  cc_sleep(msec/1000.0f);
}

/*!
  Reset an SbTime instance to \a sec number of seconds.

  \sa getValue().
 */
void
SbTime::setValue(const double sec)
{
  this->dtime = sec;
}

/*!
  Reset an SbTime instance to \a sec number of seconds
  and \a usec number of microseconds.

  \sa getValue().
 */
void
SbTime::setValue(const int32_t sec, const long usec)
{
  this->dtime = static_cast<double>(sec) + static_cast<double>(usec)/1000000.0;
}

/*!
  Reset an SbTime instance to the date and time given by the \a timeval
  struct. For information on the \a timeval struct, please consult your
  developer system documentation.

  \sa getValue().
 */
void
SbTime::setValue(const struct timeval * const tv)
{
  this->dtime = tv->tv_sec;
  this->dtime += static_cast<double>(tv->tv_usec)/1000000.0;
}

/*!
  Set the time by \a msec number of milliseconds.

  \sa getMsecValue().
 */
void
SbTime::setMsecValue(const unsigned long msec)
{
  this->setValue(static_cast<double>(msec) / 1000.0);
}

/*!
  Return time as number of seconds.

  \sa setValue().
 */
double
SbTime::getValue(void) const
{
  return this->dtime;
}

/*!
  Return number of seconds and microseconds which the SbTime
  instance represents.

  \sa setValue().
 */
void
SbTime::getValue(time_t & sec, long & usec) const
{
  sec = static_cast<time_t>(this->dtime);
  double us = fmod(this->dtime, 1.0) * 1000000.0;
  usec = static_cast<long>(us + (us < 0.0 ? -0.5 : 0.5));
}

/*!
  Returns the time as a \a timeval structure. For information on the \a timeval
  structure, please consult your system developer documentation.

  \sa setValue().
 */
void
SbTime::getValue(struct timeval * tv) const
{
  tv->tv_sec = static_cast<SIM_TIMEVAL_TV_SEC_T>(this->dtime);
  double us = fmod(this->dtime, 1.0) * 1000000.0;
  tv->tv_usec = static_cast<SIM_TIMEVAL_TV_USEC_T>(us + (us < 0.0 ? -0.5 : 0.5));
}

/*!
  Return number of milliseconds which the SbTime instance represents.

  Important note: you should in general avoid using this function, as
  it has an inherent API design flaw (from the original SGI Open
  Inventor design). The problem is that an unsigned long wraps around
  in a fairly short time when used for counting milliseconds: in less
  than 50 days. (And since SbTime instances are often initialized to
  be the time since the start of the epoch (i.e. 1970-01-01 00:00), the
  value will have wrapped around many, many times.)

  You are probably better off using the getValue() method which
  returns a double for the number of seconds, then multiply by 1000.0
  if you need to know the current number of milliseconds of the SbTime
  instance.

  \sa setMsecValue()
 */
unsigned long
SbTime::getMsecValue(void) const
{
  double d = this->dtime * 1000.0;

  // Check for overflow in the double->ulong cast at return.
  if (d > static_cast<double>(ULONG_MAX)) {
#if COIN_DEBUG
    static SbBool first = TRUE;
    if (first) {
      SoDebugError::postWarning("SbTime::getMsecValue",
                                "timer overflow -- consider using "
                                "SbTime::getValue() instead");
      first = FALSE;
    }
#endif // COIN_DEBUG

    // Wrap the value. This actually happens automatically on x86
    // Linux, MIPS IRIX, x86 MSWin etc when casting from a too large
    // double to an unsigned long -- but not on for instance Mac OS X
    // when the endianness is most significant byte first. And indeed,
    // Kernighan & Ritchie's "The C Programming Language", 2nd edition
    // says: "[...] if the resulting value [of a conversion from a
    // floating type to an integral type] cannot be represented in the
    // integral type, the behavior is undefined."
    //
    // So we do the modulo explicitly to be guaranteed to get the same
    // behavior on every platform. (And the reason we do a modulo
    // instead of for instance a clamp or just ignore the problem and
    // return garbage, is that it is likely that there is application
    // code already written for Coin / SGI/TGS Inventor which depends
    // on the modulo behavior.)

    d = fmod(d, ULONG_MAX);
  }


  return static_cast<unsigned long>(d);
}

/*!
  Uses the formatting specified below to return a string representation
  of the stored date/time. Any format specifiers must be prefixed with
  a '%' symbol, any other text in the format string \a fmt will be
  copied directly to the resultant SbString.

  %% - insert a single '%'.<BR>
  %D - number of days.<BR>
  %H - number of hours.<BR>
  %h - remaining hours after subtracting number of days.<BR>
  %M - number of minutes.<BR>
  %m - remaining minutes after subtracting the total number of hours.<BR>
  %S - number of seconds.<BR>
  %s - remaining seconds after subtracting the total number of minutes.<BR>
  %I - number of milliseconds.<BR>
  %i - remaining milliseconds after subtracting the total number of seconds.<BR>
  %U - number of microseconds.<BR>
  %u - remaining microseconds after subtracting the total number of mseconds.<BR>

  The result shows UTC time, not corrected for local time zone
  nor daylight savings time.
  
  \sa formatDate().
 */
SbString
SbTime::format(const char * const fmt) const
{
#if COIN_DEBUG
  if (fmt==NULL) {
    SoDebugError::postWarning("SbTime::format",
                              "Format string is NULL.");
    return SbString("");
  }
#endif // COIN_DEBUG

  SbString str("");
  double dtmp;

  int idx = 0;
  char c;
  while ((c = fmt[idx]) != '\0') {
    if (c != '%') str += c;
    else {
      char m = fmt[++idx];
      switch (m) {
      case '%':
        str += m;
        break;

      case 'D':
        this->addToString(str, this->dtime / 60.0 / 60.0 / 24.0);
        break;

      case 'H':
        this->addToString(str, this->dtime / 60.0 / 60.0);
        break;

      case 'M':
        this->addToString(str, this->dtime / 60.0);
        break;

      case 'S':
        this->addToString(str, this->dtime);
        break;

      case 'I':
        this->addToString(str, this->dtime * 1000.0);
        break;

      case 'U':
        this->addToString(str, this->dtime * 1000000.0);
        break;

      case 'h':
        dtmp = this->dtime / 60.0 / 60.0 / 24.0;
        dtmp = this->dtime - floor(dtmp) * 60.0 * 60.0 * 24.0;
        dtmp = dtmp / 60.0 / 60.0;
        dtmp = floor(dtmp);
        if (dtmp < 10.0) str += '0';
        str.addIntString(static_cast<int>(dtmp));
        break;

      case 'm':
        dtmp = this->dtime / 60.0 / 60.0;
        dtmp = this->dtime - floor(dtmp) * 60.0 * 60.0;
        dtmp = dtmp / 60.0;
        dtmp = floor(dtmp);
        if (dtmp < 10.0) str += '0';
        str.addIntString(static_cast<int>(dtmp));
        break;

      case 's':
        dtmp = this->dtime / 60.0;
        dtmp = this->dtime - floor(dtmp) * 60.0;
        dtmp = floor(dtmp);
        if (dtmp < 10.0) str += '0';
        str.addIntString(static_cast<int>(dtmp));
        break;

      case 'i':
        dtmp = fmod(this->dtime, 1.0);
        dtmp *= 1000.0;
        dtmp = floor(dtmp);
        if (dtmp < 100.0) str += '0';
        if (dtmp < 10.0) str += '0';
        str.addIntString(static_cast<int>(dtmp));
        break;

      case 'u':
        dtmp = fmod(this->dtime, 1.0);
        dtmp *= 1000000.0;
        dtmp = floor(dtmp);
        if (dtmp < 100000.0) str += '0';
        if (dtmp < 10000.0) str += '0';
        if (dtmp < 1000.0) str += '0';
        if (dtmp < 100.0) str += '0';
        if (dtmp < 10.0) str += '0';
        str.addIntString(static_cast<int>(dtmp));
        break;

      default:
#if COIN_DEBUG
        SoDebugError::postWarning("SbTime::format",
                                  "Unknown formatting char '%c'.", m);
#endif // COIN_DEBUG
        break;
      }
    }

    idx++;
  }

  return str;
}

/*!
  Accepts the formatting identifiers specified by the POSIX strftime()
  function to return a string representation of the stored date. Check
  your reference documentation for strftime() for information on the
  format modifiers available.

  Default formatting is used if \a fmt is \c NULL. Note that the
  default formatting is different on Microsoft Windows systems versus
  all other systems. For Windows, it is \c "%#c", for other systems it
  is \c "%A, %D %r" (again, see system documentation on strftime() for
  more information).

  The value of SbTime will be interpreted as seconds since 00:00:00
  1970-01-01.
  
  The result shows local time, according to local time zone and
  daylight savings time (if and when applicable).

  \sa format().
*/
SbString
SbTime::formatDate(const char * const fmt) const
{
  const char * format = fmt;
  if (format == NULL) {
#ifdef HAVE_WIN32_API
    format = "%#c";
#else // ! HAVE_WIN32_API
    format = "%A, %D %r";
#endif // ! HAVE_WIN32_API
  }

  if (format[0] == '\0') return SbString("");

  const size_t buffersize = 256;
  char buffer[buffersize];
  char * bufferpt = buffer;
  time_t secs = static_cast<time_t>(this->dtime);
  size_t currentsize = buffersize;

#if defined(HAVE_LOCALTIME_R)
  struct tm tm_buf;
  struct tm * ts = &tm_buf;
  (void)localtime_r(&secs, ts);
#elif defined(_WIN32) && defined(HAVE_LOCALTIME_S)
  struct tm tm_buf;
  struct tm * ts = &tm_buf;
  (void)localtime_s(ts, &secs);
#else
  struct tm * ts = localtime(&secs);
#endif

  size_t ret = strftime(bufferpt, currentsize, format, ts);
  if ((ret == 0) || (ret == currentsize)) {
    bufferpt = NULL;
    // The resulting string was too large, so we will allocate
    // a subsequently larger buffer until the date string fits.
    do {
      delete[] bufferpt;
      currentsize *= 2;
      bufferpt = new char[currentsize];
      ret = strftime(bufferpt, currentsize, format, ts);
    } while ((ret == 0) || (ret == currentsize));
  }

  if (bufferpt == buffer) {
    return SbString(bufferpt);
  }
  else {
    SbString s(bufferpt);
    delete[] bufferpt;
    return s;
  }
}

// FIXME: write a few examples for the following doc.
/*!
  This method takes a date string and converts it to the internal
  SbTime format.  The date string must conform to one of three
  formats, namely the RFC 822 / RFC 1123 format (Wkd, DD Mnth YYYY
  HH:MM:SS GMT), the RFC 850 / RFC 1036 format (Weekday, DD-Mnth-YY
  HH:MM:SS GMT), or the asctime() format (Wkdy Mnth D HH:MM:SS YYYY).

  Feeding an invalid date string to this method will make it return
  \a FALSE.
*/
SbBool
SbTime::parsedate(const char * const date)
{
  // FIXME: make method 100% robust for erroneous date strings.
  // 19981001 mortene.
  // FIXME: ditto -- 20020916 larsa

  // FIXME: accept datestrings conforming to ISO 8601. 20000331 mortene.

#if COIN_DEBUG
  if (!date) {
    SoDebugError::postWarning("SbTime::parsedate",
                              "date string is NULL.");
    return FALSE;
  }
#endif // COIN_DEBUG

#if 0 // debug
  SoDebugError::postInfo("SbTime::parseDate", "date string: '%s'", date);
#endif // debug

  struct tm time;
  char months[12][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  const char * dateptr = date;
  while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0')
    dateptr++; // we don't care if it is wednesday
  if (*dateptr == '\0') return FALSE;
  dateptr -= 2; // step back
  if ( dateptr < date ) return FALSE;
  if (dateptr[0] != 'y' && dateptr[1] == ',') { // RFC 822 / RFC 1123 format
    // FORMAT: Wkd, DD Mnth YYYY HH:MM:SS GMT
#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SbTime::parseDate", "date format: RFC 822");
#endif // debug

    dateptr += 2;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;
    time.tm_mday = atoi(dateptr);
#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SbTime::parseDate", "Day of month: %d",
                           time.tm_mday);
#endif // debug
    while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;

    int i;
    for (i=0; i < 12; i++) {
      if (! coin_strncasecmp(dateptr, months[i], 3)) {
        time.tm_mon = i;
        break;
      }
    }
    if (i==12) {
#if COIN_DEBUG
      SoDebugError::post("SbTime::parsedate", "Can't grok month name '%s'.",
                         SbString(dateptr).getSubString(0, 2).getString());
#endif // COIN_DEBUG
      return FALSE;
    }

#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SbTime::parseDate", "Month: %d", time.tm_mon);
#endif // debug
    while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;
    time.tm_year = atoi(dateptr) - 1900;
    while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;
    time.tm_hour = atoi(dateptr);
    while (*dateptr != ':' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    dateptr++;
    time.tm_min = atoi(dateptr);
    while (*dateptr != ':' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    dateptr++;
    time.tm_sec = atoi(dateptr);
    time.tm_wday = 0;
    time.tm_yday = 0;
    time.tm_isdst = 0;
  } else if (dateptr[1] == ',') { // RFC 850 / RFC 1036 format
    // FORMAT: Weekday, DD-Mnth-YY HH:MM:SS GMT
#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SbTime::parseDate", "date format: RFC 850");
#endif // debug

    dateptr += 2;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;
    time.tm_mday = atoi(dateptr);
    while (*dateptr != '-') dateptr++;
    dateptr++;

    int i;
    for (i=0; i < 12; i++) {
      if (! coin_strncasecmp(dateptr, months[i], 3)) {
        time.tm_mon = i;
        break;
      }
    }
    if (i==12) {
#if COIN_DEBUG
      SoDebugError::post("SbTime::parsedate", "Can't grok month name '%s'.",
                         SbString(dateptr).getSubString(0, 2).getString());
#endif // COIN_DEBUG
      return FALSE;
    }

    while (*dateptr != '-' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    dateptr++;
    // put number of years since 1900 into tm_year
    time.tm_year = atoi(dateptr);
    if ( time.tm_year < 70 ) time.tm_year += 100;

    while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;
    time.tm_hour = atoi(dateptr);
    while (*dateptr != ':' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    dateptr++;
    time.tm_min = atoi(dateptr);
    while (*dateptr != ':' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    dateptr++;
    time.tm_sec = atoi(dateptr);
    time.tm_wday = 0;
    time.tm_yday = 0;
    time.tm_isdst = 0;
  } else { // assumed to be ANSI C's asctime() format
    // format: Wkdy Mnth  D HH:MM:SS YYYY
#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SbTime::parseDate", "date format: asctime()");
#endif // debug

    while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;

    int i;
    for (i=0; i < 12; i++) {
      if (! coin_strncasecmp(dateptr, months[i], 3)) {
        time.tm_mon = i;
        break;
      }
    }
    if (i==12) {
#if COIN_DEBUG
      SoDebugError::post("SbTime::parsedate", "Can't grok month name '%s'.",
                         SbString(dateptr).getSubString(0, 2).getString());
#endif // COIN_DEBUG
      return FALSE;
    }

    while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;
    time.tm_mday = atoi(dateptr);
    while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;
    time.tm_hour = atoi(dateptr);
    while (*dateptr != ':' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    dateptr++;
    time.tm_min = atoi(dateptr);
    while (*dateptr != ':' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    dateptr++;
    time.tm_sec = atoi(dateptr);
    while (*dateptr != ' ' && *dateptr != '\t' && *dateptr != '\0') dateptr++;
    if (*dateptr == '\0') return FALSE;
    while (*dateptr == ' ' || *dateptr == '\t') dateptr++;
    time.tm_year = atoi(dateptr) - 1900;
    time.tm_wday = 0;
    time.tm_yday = 0;
    time.tm_isdst = 0;
  }

  this->dtime = static_cast<double>(mktime(&time));
  return TRUE;
}

/*!
  \relates SbTime

  Add the two SbTimes and return the result.
 */
SbTime
operator +(const SbTime & t0, const SbTime & t1)
{
  SbTime t = t0;
  t += t1;
  return t;
}

/*!
  \relates SbTime

  Subtract \a t1 from \a t0 and return the result.
 */
SbTime
operator -(const SbTime & t0, const SbTime & t1)
{
  SbTime t = t0;
  t -= t1;
  return t;
}

/*!
  Add \a tm to time value and return reference to self.
 */
SbTime&
SbTime::operator +=(const SbTime & tm)
{
  this->dtime += tm.dtime;
  return *this;
}

/*!
  Subtract \a tm from time value and return reference to self.
 */
SbTime&
SbTime::operator -=(const SbTime & tm)
{
  this->dtime -= tm.dtime;
  return *this;
}

/*!
  Return the negated time.
 */
SbTime
SbTime::operator-(void) const
{
  return SbTime(-this->getValue());
}

/*!
  \relates SbTime

  Multiply time value \a tm with \a s and return result.
 */
SbTime
operator *(const double s, const SbTime & tm)
{
  SbTime t = tm;
  t *= s;
  return t;
}

/*!
  \relates SbTime

  Multiply time value \a tm with \a s and return result.
 */
SbTime
operator *(const SbTime & tm, const double s)
{
  return s * tm;
}

/*!
  \relates SbTime

  Divide time value \a tm with \a s and return result.
 */
SbTime
operator /(const SbTime & tm, const double s)
{
  SbTime t = tm;
  t /= s;
  return t;
}

/*!
  \relates SbTime

  Multiply time value with \a s and return reference to self.
 */
SbTime&
SbTime::operator *=(const double s)
{
  this->dtime *= s;
  return *this;
}

/*!
  \relates SbTime

  Divide time value with \a s and return reference to self.
 */
SbTime&
SbTime::operator /=(const double s)
{
#if COIN_DEBUG
  if (s==0.0) {
    SoDebugError::postWarning("SbTime::operator/=",
                              "Argument is zero => Division by zero.");
    this->dtime /= s + SMALLEST_DOUBLE_TIMEUNIT;
    return *this;
  }
#endif // COIN_DEBUG

  this->dtime /= s;
  return *this;
}

/*!
  \relates SbTime

  Find the factor between this SbTime and the one given in \a tm, and
  return the result.
 */
double
SbTime::operator /(const SbTime & tm) const
{
#if COIN_DEBUG
  if (tm.getValue()==0.0) {
    SoDebugError::postWarning("SbTime::operator/",
                              "Argument tm is zero => Division by zero.");
    return 1.0/SMALLEST_DOUBLE_TIMEUNIT;
  }
#endif // COIN_DEBUG

  return this->getValue()/tm.getValue();
}

/*!
  Returns the remainder time when dividing on \a tm.
 */
SbTime
SbTime::operator %(const SbTime & tm) const
{
#if COIN_DEBUG
  if (tm.getValue()==0.0) {
    SoDebugError::postWarning("SbTime::operator%",
                              "Argument tm is zero => Division by zero.");
    return SbTime(1.0/SMALLEST_DOUBLE_TIMEUNIT);
  }
#endif // COIN_DEBUG

  return SbTime(fmod(this->getValue(), tm.getValue()));
}

/*!
  Check if the time value is equal to that of \a tm.
 */
int
SbTime::operator ==(const SbTime & tm) const
{
  if (fabs(this->dtime-tm.dtime) < (SMALLEST_DOUBLE_TIMEUNIT/2.0)) return TRUE;
  return FALSE;
}

/*!
  Check if the time value is not equal to that of \a tm.
 */
int
SbTime::operator !=(const SbTime & tm) const
{
  return !(*this == tm);
}

/*!
  Compares with \a tm and return TRUE if less.
 */
SbBool
SbTime::operator <(const SbTime & tm) const
{
  double diff = tm.dtime - this->dtime;
  if ((diff>0.0) && (fabs(diff) > (SMALLEST_DOUBLE_TIMEUNIT/2.0))) return TRUE;
  return FALSE;
}

/*!
  Compares with \a tm and return TRUE if larger than.
 */
SbBool
SbTime::operator >(const SbTime & tm) const
{
  double diff = tm.dtime - this->dtime;
  if ((diff<0.0) && (fabs(diff) > (SMALLEST_DOUBLE_TIMEUNIT/2.0))) return TRUE;
  return FALSE;
}

/*!
  Compares with \a tm and return TRUE if less or equal.
 */
SbBool
SbTime::operator <=(const SbTime & tm) const
{
  if (*this < tm) return TRUE;
  return (*this == tm);
}

/*!
  Compares with \a tm and return TRUE if larger or equal.
 */
SbBool
SbTime::operator >=(const SbTime & tm) const
{
  if (*this > tm) return TRUE;
  return (*this == tm);
}

/*!
  \COININTERNAL

  Concatenate a string representation of \a val to \a str, ignoring
  any decimals.
 */
void
SbTime::addToString(SbString & str, const double v) const
{
  double val = v;

  // Handle sign.
  if (val < 0.0) {
    str += '-';
    val = -val;
  }

  // Code below depends on val != 0.0.
  if (val == 0.0) {
    str += '0';
    return;
  }

  while (val > static_cast<double>(INT_MAX)) {
    int steps = 0;
    double vcopy = val;

    // "Clamp" value to within bounds of an integer.
    while (val > static_cast<double>(INT_MAX)) {
      val /= 10.0;
      steps++;
    }

    // Add to string.
    val = floor(val);
    str.addIntString(static_cast<int>(val));

    int scopy = steps;

    // Calculate remainder.
    while (steps) {
      val *= 10.0;
      steps--;
    }
    val = vcopy - val;

    // Add any trailing zeros.
    if (val == 0.0) {
      while (scopy) {
        str += '0';
        scopy--;
      }
    }
  }

  if (val != 0.0) str.addIntString(static_cast<int>(val));
}


/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbTime::print(FILE * fp) const
{
#if COIN_DEBUG
  struct timeval tm;
  this->getValue(&tm);
  SbString str = this->formatDate();
  (void)fprintf(fp, "%s", str.getString());
  (void)fprintf(fp, ", secs: %ld, msecs: %ld\n", static_cast<long int>(tm.tv_sec),
               static_cast<long int>(tm.tv_usec));
#endif // COIN_DEBUG
}
