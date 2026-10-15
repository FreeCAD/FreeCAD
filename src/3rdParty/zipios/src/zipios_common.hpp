#pragma once
#ifndef ZIPIOS_COMMON_HPP
#define ZIPIOS_COMMON_HPP

/*
  Zipios -- a small C++ library that provides easy access to .zip files.

  Copyright (C) 2000-2007  Thomas Sondergaard
  Copyright (c) 2015-2022  Made to Order Software Corp.  All Rights Reserved

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
 * \brief Various functions used throughout the library.
 *
 * This file defines a small set of functions that do not really have
 * another place to live.
 */

#include "zipios/zipios-config.hpp"

#include <vector>
#include <sstream>
#include <cstdint>

#if defined( ZIPIOS_WINDOWS )
typedef int32_t ssize_t;
#endif


/** \brief Concatenate two vectors together.
 *
 * This function appends vector v2 to vector v1 using a push_back()
 * of all the elements of v2.
 *
 * Note that the function fails to append anything is you write something
 * like this:
 *
 * \code
 *      v1 += v1;
 * \endcode
 *
 * Instead, create a new vector and then add the source twice, something
 * like this:
 *
 * \code
 *      vector<std::string> temp;
 *      temp += v1;
 *      v1 += temp;
 * \endcode
 *
 * \warning
 * This template is not put in the namespace because in some situation
 * it may not be visible (particularly in tests/catch_common.cpp,
 * somehow.) If you know of a fix for that then it would be great.
 * (I tried "using zipios;" but it did not work, maybe a specialization?)
 *
 * \param[in,out] v1  The vector which receives a copy of v2.
 * \param[in]  v2  The vector to concatenate at the end of v1.
 */
template<class Type>
void operator += (std::vector<Type> & v1, std::vector<Type> const & v2)
{
    // make sure these are not the same vector or the insert()
    // is not unlikely to fail badly; it is expected that the
    // user does not try to duplicate an array...
    if(&v1 != &v2)
    {
        v1.reserve(v1.size() + v2.size());
        v1.insert(v1.end(), v2.begin(), v2.end());
    }
}


namespace zipios
{


extern char const g_separator;


typedef std::ostringstream OutputStringStream;


typedef std::vector<unsigned char>      buffer_t;


void     zipRead(std::istream & is, uint32_t & value);
void     zipRead(std::istream & is, uint16_t & value);
void     zipRead(std::istream & is, uint8_t &  value);
void     zipRead(std::istream & is, buffer_t & buffer, ssize_t const count);
void     zipRead(std::istream & is, std::string & str, ssize_t const count);

void     zipRead(buffer_t const & is, size_t & pos, uint32_t & value);
void     zipRead(buffer_t const & is, size_t & pos, uint16_t & value);
void     zipRead(buffer_t const & is, size_t & pos, uint8_t &  value);
void     zipRead(buffer_t const & is, size_t & pos, buffer_t & buffer, ssize_t const count);
void     zipRead(buffer_t const & is, size_t & pos, std::string & str, ssize_t const count);

void     zipWrite(std::ostream & os, uint32_t const & value);
void     zipWrite(std::ostream & os, uint16_t const & value);
void     zipWrite(std::ostream & os, uint8_t const &  value);
void     zipWrite(std::ostream & os, buffer_t const & buffer);
void     zipWrite(std::ostream & os, std::string const & str);


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
#endif
