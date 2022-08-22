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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
# include <QDateTime>
# if defined(FC_OS_LINUX) || defined(__MINGW32__)
# include <sys/time.h>
# endif
#endif

#include "TimeInfo.h"


using namespace Base;


/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
TimeInfo::TimeInfo()
{
    setCurrent();
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
TimeInfo::~TimeInfo() = default;


//**************************************************************************
// separator for other implementation aspects

void TimeInfo::setCurrent()
{
#if defined (FC_OS_BSD) || defined(FC_OS_LINUX) || defined(__MINGW32__)
    struct timeval t;
    gettimeofday(&t, nullptr);
    timebuffer.time = t.tv_sec;
    timebuffer.millitm = t.tv_usec / 1000;
#elif defined(FC_OS_WIN32)
    _ftime(&timebuffer);
#else
    ftime(&timebuffer); // deprecated
#endif
}

void TimeInfo::setTime_t (int64_t seconds)
{
    timebuffer.time = seconds;
}

std::string TimeInfo::currentDateTimeString()
{
    return QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC)
        .toString(Qt::ISODate).toStdString();
}

std::string TimeInfo::diffTime(const TimeInfo &timeStart,const TimeInfo &timeEnd )
{
   std::stringstream str;
   str << diffTimeF(timeStart,timeEnd);
   return str.str();
}

float TimeInfo::diffTimeF(const TimeInfo &timeStart,const TimeInfo &timeEnd )
{
    int64_t ds = int64_t(timeEnd.getSeconds() - timeStart.getSeconds());
    int dms = int(timeEnd.getMiliseconds()) - int(timeStart.getMiliseconds());

    return float(ds) + float(dms) * 0.001f;
}

TimeInfo TimeInfo::null()
{
    TimeInfo ti;
    ti.timebuffer = {};
    return ti;
}

bool TimeInfo::isNull() const
{
    return (*this) == TimeInfo::null();
}
