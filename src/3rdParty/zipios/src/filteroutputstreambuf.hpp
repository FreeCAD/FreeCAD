#pragma once
#ifndef FILTEROUTPUTSTREAMBUF_HPP
#define FILTEROUTPUTSTREAMBUF_HPP

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
 * \brief Declarations of the zipios::FilterOutputStreambuf.
 *
 * This file includes the declaration of the zipios::FileterOutputStreambuf
 * class which is used as a base class to filter output data.
 */

#include <iostream>


namespace zipios
{


class FilterOutputStreambuf : public std::streambuf
{
public:
                                    FilterOutputStreambuf(std::streambuf * outbuf);
                                    FilterOutputStreambuf(FilterOutputStreambuf const & rhs) = delete;
    virtual                         ~FilterOutputStreambuf();

    FilterOutputStreambuf &         operator = (FilterOutputStreambuf const & rhs) = delete;

protected:
    std::streambuf *                m_outbuf = nullptr;
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
