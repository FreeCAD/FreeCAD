/***************************************************************************
 *   Copyright (c) Riegel         <juergen.riegel@web.de>                  *
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
TimeInfo::~TimeInfo()
{
}


//**************************************************************************
// separator for other implemetation aspects

void TimeInfo::setCurrent(void)
{
#if defined (_MSC_VER)
    _ftime( &timebuffer );
#elif defined(__GNUC__)
    ftime( &timebuffer );
#endif
}

void TimeInfo::setTime_t (uint64_t seconds)
{
    timebuffer.time = seconds;
}

std::string TimeInfo::currentDateTimeString()
{
#if (QT_VERSION >= 0x050300)
    return QDateTime::currentDateTime().toTimeSpec(Qt::OffsetFromUTC)
        .toString(Qt::ISODate).toStdString();
#else
    QDateTime local = QDateTime::currentDateTime();
    QDateTime utc = local.toUTC();
    utc.setTimeSpec(Qt::LocalTime);
    int utcOffset = utc.secsTo(local);
    local.setUtcOffset(utcOffset);
    QString dm = local.toString(Qt::ISODate);
    return dm.toStdString();
#endif
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

    return float(ds) + float(dms) * 0.001;
}

TimeInfo TimeInfo::null()
{
    TimeInfo ti;
    ti.timebuffer.time = 0;
    ti.timebuffer.millitm = 0;
    return ti;
}

bool TimeInfo::isNull() const
{
    return (*this) == TimeInfo::null();
}
