/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef BASE_TIMEINFO_H
#define BASE_TIMEINFO_H

// Std. configurations


#include <cstdio>
#if defined(FC_OS_BSD)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif
#include <ctime>

#ifdef __GNUC__
# include <cstdint>
#endif

#include <string>
#include <FCGlobal.h>

#if defined(FC_OS_BSD)
struct timeb
{
    int64_t time;
    unsigned short millitm;
};
#endif

namespace Base
{
/// BaseClass class and root of the type system
class BaseExport TimeInfo
{

public:
    /// Construction
    TimeInfo();
    TimeInfo(const TimeInfo&) = default;
    /// Destruction
    virtual ~TimeInfo();

    /// sets the object to the actual system time
    void setCurrent();
    void setTime_t (int64_t seconds);

    int64_t getSeconds() const;
    unsigned short  getMiliseconds() const;

    void operator =  (const TimeInfo &time);
    bool operator == (const TimeInfo &time) const;
    bool operator != (const TimeInfo &time) const;

    bool operator <  (const TimeInfo &time) const;
    bool operator <= (const TimeInfo &time) const;
    bool operator >= (const TimeInfo &time) const;
    bool operator >  (const TimeInfo &time) const;

    static std::string currentDateTimeString();
    static std::string diffTime(const TimeInfo &timeStart,const TimeInfo &timeEnd = TimeInfo());
    static float diffTimeF(const TimeInfo &timeStart,const TimeInfo &timeEnd  = TimeInfo());
    bool isNull() const;
    static TimeInfo null();

protected:
#if defined (_MSC_VER)
    struct _timeb timebuffer;
#elif defined(__GNUC__)
    struct timeb timebuffer;
#endif
};


 inline int64_t TimeInfo::getSeconds() const
 {
     return timebuffer.time;
 }

 inline unsigned short  TimeInfo::getMiliseconds() const
 {
     return timebuffer.millitm;
 }

inline bool
TimeInfo::operator != (const TimeInfo &time) const
{
    return (timebuffer.time != time.timebuffer.time || timebuffer.millitm != time.timebuffer.millitm);
}

inline void
TimeInfo::operator = (const TimeInfo &time)
{
    timebuffer = time.timebuffer;
}

inline bool
TimeInfo::operator == (const TimeInfo &time) const
{
    return (timebuffer.time == time.timebuffer.time && timebuffer.millitm == time.timebuffer.millitm);
}

inline bool
TimeInfo::operator <  (const TimeInfo &time) const
{
    if (timebuffer.time == time.timebuffer.time)
        return timebuffer.millitm < time.timebuffer.millitm;
    else
        return timebuffer.time < time.timebuffer.time;
}

inline bool
TimeInfo::operator <= (const TimeInfo &time) const
{
    if (timebuffer.time == time.timebuffer.time)
        return timebuffer.millitm <= time.timebuffer.millitm;
    else
        return timebuffer.time <= time.timebuffer.time;
}

inline bool
TimeInfo::operator >= (const TimeInfo &time) const
{
    if (timebuffer.time == time.timebuffer.time)
        return timebuffer.millitm >= time.timebuffer.millitm;
    else
        return timebuffer.time >= time.timebuffer.time;
}

inline bool
TimeInfo::operator >  (const TimeInfo &time) const
{
    if (timebuffer.time == time.timebuffer.time)
        return timebuffer.millitm > time.timebuffer.millitm;
    else
        return timebuffer.time > time.timebuffer.time;
}

} //namespace Base


#endif // BASE_TIMEINFO_H

