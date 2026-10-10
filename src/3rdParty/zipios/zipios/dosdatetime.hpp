#pragma once
#ifndef ZIPIOS_DOSDATETIME_HPP
#define ZIPIOS_DOSDATETIME_HPP

/*
  Zipios -- a small C++ library that provides easy access to .zip files.

  Copyright (c) 2019-2022  Made to Order Software Corp.  All Rights Reserved

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/** \file
 * \brief Define a type to manage date and time in MS-DOS format.
 *
 * This file describes a structure used to describe the MS-DOS format as
 * used in the FAT file system.
 */

#include <cstdint>
#include <ctime>

#include <time.h>


namespace zipios
{


class DOSDateTime
{
public:
    typedef uint32_t            dosdatetime_t;

    static dosdatetime_t const  g_min_dosdatetime = 0x00210000;     // Jan  1, 1980  00:00:00
    static dosdatetime_t const  g_max_dosdatetime = 0xFF9FBF7D;     // Dec 31, 2107  23:59:59

    bool                        isValid() const;
    int                         daysInMonth() const;
    int                         getSecond() const;
    int                         getMinute() const;
    int                         getHour() const;
    int                         getMDay() const;
    int                         getMonth() const;
    int                         getYear() const;
    void                        setSecond(int second);
    void                        setMinute(int minute);
    void                        setHour(int hour);
    void                        setMDay(int mday);
    void                        setMonth(int month);
    void                        setYear(int year);
    dosdatetime_t               getDOSDateTime() const;
    void                        setDOSDateTime(dosdatetime_t datetime);
    void                        setUnixTimestamp(std::time_t unix_timestamp);
    std::time_t                 getUnixTimestamp() const;

protected:
    dosdatetime_t               m_dosdatetime = 0;
};


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
#endif
