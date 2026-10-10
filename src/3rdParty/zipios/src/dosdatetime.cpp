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
 * \brief Implementation of the zipios::DOSDateTime class.
 *
 * This file is the implementation of the zipios::DOSDateTime class
 * which converts dates between Unix timestamps and DOS Date Time
 * timestamps.
 *
 * Keep in mind that the dates in a zip file use your local time,
 * whatever that is at the time you create the file. The get/set Unix
 * timestamp functions adjust the date to UTC as required.
 *
 * \sa https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-dosdatetimetofiletime
 */

#include "zipios/dosdatetime.hpp"

#include "zipios/zipiosexceptions.hpp"


namespace zipios
{


DOSDateTime::dosdatetime_t const  DOSDateTime::g_min_dosdatetime;     // Jan  1, 1980  00:00:00
DOSDateTime::dosdatetime_t const  DOSDateTime::g_max_dosdatetime;     // Dec 31, 2107  23:59:59




/** \brief Union used to convert the uint32_t to fields and vice versa.
 *
 * This union is used by the functions below to convert the basic
 * uint32_t dosdatetime_t values in a list of 6 fields.
 */
union dosdatetime_convert_t
{
    DOSDateTime::dosdatetime_t      m_dosdatetime;
    struct fields
    {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        DOSDateTime::dosdatetime_t      m_year   : 7;  // add 1980
        DOSDateTime::dosdatetime_t      m_month  : 4;  // 1 to 12
        DOSDateTime::dosdatetime_t      m_mday   : 5;  // 1 to 31
        DOSDateTime::dosdatetime_t      m_hour   : 5;
        DOSDateTime::dosdatetime_t      m_minute : 6;
        DOSDateTime::dosdatetime_t      m_second : 5;   // WARNING: the precision is every 2 seconds (0, 2, 4, etc.)
#else
        DOSDateTime::dosdatetime_t      m_second : 5;   // WARNING: the precision is every 2 seconds (0, 2, 4, etc.)
        DOSDateTime::dosdatetime_t      m_minute : 6;
        DOSDateTime::dosdatetime_t      m_hour   : 5;
        DOSDateTime::dosdatetime_t      m_mday   : 5;  // 1 to 31
        DOSDateTime::dosdatetime_t      m_month  : 4;  // 1 to 12
        DOSDateTime::dosdatetime_t      m_year   : 7;  // add 1980
#endif
    } m_fields;
};



namespace
{

/** \brief Number of days in a month.
 *
 * This table is used in the daysInMonth() function to determine the number
 * of days in the month. It is ignored if the month is February.
 */
int const g_days_in_month[12] = {
    /* Jan */   31,
    /* Feb */   0, // special handling
    /* Mar */   31,
    /* Apr */   30,
    /* May */   31,
    /* Jun */   30,
    /* Jul */   31,
    /* Aug */   31,
    /* Sep */   30,
    /* Oct */   31,
    /* Nov */   30,
    /* Dec */   31
};


int const g_ydays[12] = {
    /* Jan */   0,
    /* Feb */   31,
    /* Mar */   31 + 0, // special handling
    /* Apr */   31 + 0 + 31,
    /* May */   31 + 0 + 31 + 30,
    /* Jun */   31 + 0 + 31 + 30 + 31,
    /* Jul */   31 + 0 + 31 + 30 + 31 + 30,
    /* Aug */   31 + 0 + 31 + 30 + 31 + 30 + 31,
    /* Sep */   31 + 0 + 31 + 30 + 31 + 30 + 31 + 31,
    /* Oct */   31 + 0 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    /* Nov */   31 + 0 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    /* Dec */   31 + 0 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
};


}



/** \brief Check whether this DOS Date & Date is valid.
 *
 * This function verifies to see whether the DOS Date & Time it holds is
 * valid. By default, the value is set to zero which represents an invalid
 * date (_not set_).
 *
 * \note
 * Remember that Zip file Date & Time are saved in an old MS-DOS format
 * which did not respect UTC. It instead represents local time. This
 * function returns true when the local time is valid, since the same
 * time will be shared around the globe, it will always be considered
 * valid, but the Unix timestamp can look like a mismatch (the minimum
 * and maximum possible time stamps represented in a Unix time_t variable
 * will vary depending on your timezone settings.)
 *
 * \return true if the date is considered valid (represents an actual date
 * and time).
 *
 * \sa daysInMonth()
 */
bool DOSDateTime::isValid() const
{
    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    return conv.m_fields.m_second < 30  // remember we only keep `sec / 2` in a DOS time field
        && conv.m_fields.m_minute < 60
        && conv.m_fields.m_hour < 24
        && conv.m_fields.m_mday > 0
        && conv.m_fields.m_mday <= daysInMonth()
        && conv.m_fields.m_month > 0
        && conv.m_fields.m_month < 13;
}


/** \brief Calculate the number of days in this date's month.
 *
 * This function uses this object's current month to determine how
 * many days are expected in that month.
 *
 * If the month field is invalid (not a number between 1 and 12 inclusive)
 * then the function returns -1 to indicate an error.
 *
 * The number of days is always 30 or 31 except for the month of February
 * in which case the function may return 28 or 29. For this reason, your
 * date must have the month and year you want to use to get a valid result
 * when calling this function.
 *
 * \note
 * This function is used to verify that the date month day is valid.
 *
 * \return The number of days in this date's current month.
 *
 * \sa isValid()
 */
int DOSDateTime::daysInMonth() const
{
    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;

    if(conv.m_fields.m_month == 0
    || conv.m_fields.m_month > 12)
    {
        return -1;
    }

    if(conv.m_fields.m_month == 2)
    {
        // Feb. depends on the year
        //
        int year = conv.m_fields.m_year + 1980;

        return ((year) % 400) == 0
                    ? 29
                    : (((year) % 100) == 0
                        ? 28
                        : (((year) % 4) == 0
                            ? 29
                            : 28));
    }

    return g_days_in_month[conv.m_fields.m_month - 1];
}


/** \brief Get the second.
 *
 * This function returns the second this DOSDateTime object represents.
 *
 * The second is between 0 and 59 inclusive.
 *
 * \note
 * The DOSDateTime format only supports 5 bits for seconds. In other words,
 * the number can't go all the way from 0 to 59. Instead, the second
 * is saved without bit 0. In other words, only an even number of second
 * is saved. In other words, 0, 2, 4, 6, up to 58.
 *
 * \return The second this DOS Date & Time represents.
 */
int DOSDateTime::getSecond() const
{
    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    return conv.m_fields.m_second * 2;
}


/** \brief Get the minute.
 *
 * This function returns the minute this DOSDateTime object represents.
 *
 * The minute is between 0 and 59 inclusive.
 *
 * \return The minute this DOS Date & Time represents.
 */
int DOSDateTime::getMinute() const
{
    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    return conv.m_fields.m_minute;
}


/** \brief Get the hour.
 *
 * This function returns the hour this DOSDateTime object represents.
 *
 * The hour is between 0 and 23 inclusive.
 *
 * \return The hour this DOS Date & Time represents.
 */
int DOSDateTime::getHour() const
{
    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    return conv.m_fields.m_hour;
}


/** \brief Get the day of the month.
 *
 * This function returns the day of month this DOSDateTime object represents.
 *
 * The day is between 1 and 31. To know whether a day is valid, use the
 * daysInMonth() function which returns the maximum number of days for a
 * given month.
 *
 * \return The day of the month this DOS Date & Time represents.
 */
int DOSDateTime::getMDay() const
{
    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    return conv.m_fields.m_mday;
}


/** \brief Get the month.
 *
 * This function returns the month this DOSDateTime object represents.
 *
 * The month is between 1 and 12.
 *
 * \return The month this DOS Date & Time represents.
 */
int DOSDateTime::getMonth() const
{
    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    return conv.m_fields.m_month;
}


/** \brief Get the year.
 *
 * This function returns the year this DOSDateTime object represents.
 *
 * The year is limited between 1980 and 2107.
 *
 * \return The year this DOS Date & Time represents.
 */
int DOSDateTime::getYear() const
{
    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    return conv.m_fields.m_year + 1980;
}


/** \brief Set the second.
 *
 * This function can be used to only change the second of a DOSDateTime
 * object.
 *
 * \note
 * The DOSDateTime format only supports 5 bits for seconds. In other words,
 * the number can't go all the way from 0 to 59. Instead, the second
 * is saved without bit 0. In other words, only an even number of second
 * is saved. In other words, 0, 2, 4, 6, up to 58.
 *
 * \attention
 * Leap seconds are not supported. Trying to pass 60 to this function
 * raises an InvalidException error.
 *
 * \exception InvalidException
 * The second is expected to be set to a number between 0 and 59 inclusive.
 * No exception is raised if an odd number is passed down. Bit 0 is simply
 * ignored.
 *
 * \param[in] second  The new DOSDateTime number of seconds.
 */
void DOSDateTime::setSecond(int second)
{
    if(second < 0
    || second > 59)
    {
        throw InvalidException("Second is out of range for an MS-DOS Date & Time object. Range is [0, 59].");
    }

    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    conv.m_fields.m_second = second / 2;
    m_dosdatetime = conv.m_dosdatetime;
}


/** \brief Set the minute.
 *
 * This function can be used to only change the minute of a DOSDateTime
 * object.
 *
 * \exception InvalidException
 * The minute is expected to be set to a number between 0 and 59 inclusive.
 *
 * \param[in] minute  The new DOSDateTime number of minutes.
 */
void DOSDateTime::setMinute(int minute)
{
    if(minute < 0
    || minute > 59)
    {
        throw InvalidException("Minute is out of range for an MS-DOS Date & Time object. Range is [0, 59].");
    }

    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    conv.m_fields.m_minute = minute;
    m_dosdatetime = conv.m_dosdatetime;
}


/** \brief Set the hour.
 *
 * This function can be used to only change the hour of a DOSDateTime
 * object.
 *
 * \exception InvalidException
 * The hour is expected to be set to a number between 0 and 23 inclusive.
 *
 * \param[in] hour  The new DOSDateTime number of hours.
 */
void DOSDateTime::setHour(int hour)
{
    if(hour < 0
    || hour > 23)
    {
        throw InvalidException("Hour is out of range for an MS-DOS Date & Time object. Range is [0, 23].");
    }

    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    conv.m_fields.m_hour = hour;
    m_dosdatetime = conv.m_dosdatetime;
}


/** \brief Set the day of the month.
 *
 * This function can be used to only change the day of the month of a
 * DOSDateTime object.
 *
 * \exception InvalidException
 * The day of the month is expected to be set to a number between 1 and 31
 * inclusive. An exception is raised if out of that range. Note that no
 * exception is raised if the day is invalid for a certain month because
 * at this point we can't be sure what the month will be. To verify once
 * you are done setting all the individual values, call isValid() to do
 * a final verification.
 *
 * \param[in] mday  The new DOSDateTime day of the month.
 */
void DOSDateTime::setMDay(int mday)
{
    if(mday < 1
    || mday > 31)
    {
        throw InvalidException("Day of the month is out of range for an MS-DOS Date & Time object. Range is [1, 31].");
    }

    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    conv.m_fields.m_mday = mday;
    m_dosdatetime = conv.m_dosdatetime;
}


/** \brief Set the month.
 *
 * This function can be used to only change the month of a DOSDateTime object.
 *
 * \exception InvalidException
 * The month is expected to be set to a number between 1 and 12 inclusive.
 *
 * \param[in] month  The new DOSDateTime month.
 */
void DOSDateTime::setMonth(int month)
{
    if(month < 1
    || month > 12)
    {
        throw InvalidException("Month out of range for an MS-DOS Date & Time object. Range is [1, 12].");
    }

    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    conv.m_fields.m_month = month;
    m_dosdatetime = conv.m_dosdatetime;
}


/** \brief Set the year.
 *
 * This function can be used to only change the year this DOSDateTime object
 * represents.
 *
 * \exception InvalidException
 * The year is limited between 1980 and 2107. This exception is raised if the
 * year to out of this range.
 */
void DOSDateTime::setYear(int year)
{
    if(year < 1980
    || year > 2107)
    {
        throw InvalidException("Year out of range for an MS-DOS Date & Time object. Range is [1980, 2107] (1).");
    }

    dosdatetime_convert_t conv;
    conv.m_dosdatetime = m_dosdatetime;
    conv.m_fields.m_year = year - 1980;
    m_dosdatetime = conv.m_dosdatetime;
}


/** \brief Retrieve the DOSDateTime value as is.
 *
 * This function returns the DOSDateTime value as is. It can then be used
 * in the zip file.
 *
 * \return The dosdatetime_t timestamp.
 */
DOSDateTime::dosdatetime_t DOSDateTime::getDOSDateTime() const
{
    return m_dosdatetime;
}


/** \brief Set the DOSDateTime value as is.
 *
 * This function sets this DOSDateTime object's timestamp to the specified
 * value. Any value is accepted by this function. To verify that the new
 * value is a valid date, use the isValid() function.
 *
 * \param[in] datetime  The DOS Date & Time value.
 */
void DOSDateTime::setDOSDateTime(dosdatetime_t datetime)
{
    m_dosdatetime = datetime;
}


/** \brief Set the DOSDateTime value from a Unix timestamp.
 *
 * This function accepts a Unix timestamp that gets converted to a
 * DOSDateTime object.
 *
 * A Unix timestamp is a time_t number representing seconds. 0 represents
 * the date Jan 1, 1970 at 00:00:00.
 *
 * The smallest Unix timestamp that can be represented in a DOSDateTime
 * object is 315532800 (0x12cea600) in UTC. Since the timestamp gets
 * converted to local time, though, the boundaries vary with the user's
 * timezone.
 *
 * The minimum date is represented as 0x00210000 in a DOSDateTime object.
 * This represents Jan 1, 1980 at 00:00:00, local time.
 *
 * The largest Unix timestamp that can be represented in a DOSDateTime
 * object is 4354819199 (0x10391447f) in UTC, since the timestamp gets
 * converted to local time, though, the boundaries vary with the user's
 * timezone.
 *
 * The maximum date is represented as 0xff9fbf7d in a DOSDateTime object.
 * Note that the Unix timestamp requires a 64 bit `time_t` representation
 * in order to reach the maximum DOSDateTime. With a 32 bit number,
 * the maximum is around 2037, about 70 years short. This maximum date
 * represents Dec 31, 2107 at 23:59:59, local time.
 *
 * \attention
 * The DOSDateTime object only holds even seconds. Odd seconds are lost
 * at the time this function gets called.
 *
 * \exception InvalidException
 * In 64 bits, a Unix timestamp can represent very large dates in both
 * directions (in the past and future.) If the Unix timestamp represents a
 * local date and time before Jan 1, 1980 at 00:00:00 or after
 * Dec 31, 2107 at 23:59:58, then this function raises this exception.
 * It is likely to raise an exception on Dec 31, 2107 at 23:59:59 because
 * we round the time to the next even second.
 *
 * \param[in] unix_timestamp  The time and stamp in Unix format.
 */
void DOSDateTime::setUnixTimestamp(std::time_t unix_timestamp)
{
    // round up to the next second
    //
    unix_timestamp += 1;
    unix_timestamp &= ~1;

    struct tm t;
#ifdef ZIPIOS_WINDOWS
    localtime_s(&t, &unix_timestamp);
#else
    localtime_r(&unix_timestamp, &t);
#endif

//std::cout << "test with: " << unix_timestamp << " -- " << t.tm_year
//          << " (" << (t.tm_year < 1980 - 1900 ? 1 : 0)
//          << ", " << (t.tm_year > 2107 - 1900 ? 1 : 0)
//          << ")\n";

    if(t.tm_year < 1980 - 1900
    || t.tm_year > 2107 - 1900)
    {
        throw InvalidException("Year out of range for an MS-DOS Date & Time object. Range is [1980, 2107] (2).");
    }

    dosdatetime_convert_t conv;
    conv.m_fields.m_second = t.tm_sec / 2; // already rounded up to the next second, so just divide by 2 is enough here
    conv.m_fields.m_minute = t.tm_min;
    conv.m_fields.m_hour   = t.tm_hour;
    conv.m_fields.m_mday   = t.tm_mday;
    conv.m_fields.m_month  = t.tm_mon + 1;
    conv.m_fields.m_year   = t.tm_year + 1900 - 1980;

    m_dosdatetime = conv.m_dosdatetime;
}


/** \brief Retrieve the DOSDateTime as a Unix timestamp.
 *
 * This function returns the DOSDateTime converted to a Unix timestamp.
 * On 64 bit platforms, all DOSDateTime can be converted to a Unix timestamp.
 * On a 32 bit platform, however, dates after 2037 can't be represented by
 * the Unix timestamp so this function throws (note that we did not check
 * the exact threshold because at this point I don't think it's too important.)
 *
 * \exception InvalidException
 * On 32 bit platform, dates that can't be represented in a Unix timestamp
 * throw this exception.
 *
 * \return The Unix timestamp representing the DOSDateTime object.
 *
 * \sa setUnixTimestamp()
 */
std::time_t DOSDateTime::getUnixTimestamp() const
{
    if(isValid())
    {
        dosdatetime_convert_t conv;
        conv.m_dosdatetime = m_dosdatetime;

        struct tm t;
        t.tm_sec   = conv.m_fields.m_second * 2;      // we lost the bottom bit, nothing we can do about it here
        t.tm_min   = conv.m_fields.m_minute;
        t.tm_hour  = conv.m_fields.m_hour;
        t.tm_mday  = conv.m_fields.m_mday;
        t.tm_mon   = conv.m_fields.m_month - 1;
        t.tm_year  = conv.m_fields.m_year + 1980 - 1900;
        t.tm_wday  = 0;
        t.tm_yday  = 0;
        t.tm_isdst = -1;

//std::cerr << "date to Unix timestamp: " << (t.tm_mon + 1) << " " << t.tm_mday << ", " << (t.tm_year + 1900)
//                                 << " " << t.tm_hour << ":" << t.tm_min << ":" << t.tm_sec << "\n";

        if(sizeof(std::time_t) == 4
        && t.tm_year >= 2038)
        {
            // the exact date is Jan 19, 2038 at 03:13:07 UTC
            // see https://en.wikipedia.org/wiki/Year_2038_problem
            //
            // we have no problem with 64 bits, max. year is about 292,000,000,000
            // although the tm_year is an int, so really we're limited to 2 billion
            // years, again just fine for a DOS Date is limited to 2107...
            //
            throw InvalidException("Year out of range for a 32 bit Unix Timestamp object. Range is (1901, 2038).");
        }

        // the zip file format expects dates in local time, not UTC
        // so I use mktime() directly
        //
        return mktime(&t);

//        // mktime() makes use of the timezone, here is some code that
//        // replaces mktime() with a UTC date conversion
//        //
//        time_t const year = t.tm_year + 1900;
//        time_t timestamp = (year - 1970LL) * 31536000LL
//                         + ((year - 1969LL) / 4LL) * 86400LL
//                         - ((year - 1901LL) / 100LL) * 86400LL
//                         + ((year - 1601LL) / 400LL) * 86400LL
//                         + (t.tm_mday + g_ydays[t.tm_mon] - 1) * 86400LL
//                         + t.tm_hour * 3600LL
//                         + t.tm_min * 60LL
//                         + t.tm_sec * 1LL;
//        if(t.tm_mon >= 2)
//        {
//            // add seconds in February
//            //
//            timestamp += (year % 400 == 0
//                        ? 29                    // for year 2000
//                        : (year % 100 == 0
//                            ? 28                // for year 2100
//                            : (year % 4 == 0
//                                ? 29
//                                : 28))) * 86400LL;
//        }
//
//        return timestamp;
    }

    return 0;
}




} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
