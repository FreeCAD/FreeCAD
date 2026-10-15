#pragma once
#ifndef ZIPINPUSTREAMBUF_HPP
#define ZIPINPUSTREAMBUF_HPP

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
 * \brief Define the zipios::ZipInputStreambuf class.
 *
 * This file defines the zipios::ZipInputStreambuf class which is
 * used to read the data of files found in a Zip archive.
 */

#include "inflateinputstreambuf.hpp"

#include "ziplocalentry.hpp"


namespace zipios
{


class ZipInputStreambuf : public InflateInputStreambuf
{
public:
                            ZipInputStreambuf(std::streambuf * inbuf, offset_t start_pos = -1);
                            ZipInputStreambuf(ZipInputStreambuf const & src) = delete;
    ZipInputStreambuf &     operator = (ZipInputStreambuf const & rhs) = delete;
    virtual                 ~ZipInputStreambuf() override;

protected:
    virtual std::streambuf::int_type    underflow() override;

private:
    ZipLocalEntry           m_current_entry = ZipLocalEntry();
    offset_t                m_remain = 0;     // For STORED entry only. the number of bytes that
                                              // has not been put in the m_outvec yet.
};


} // namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
#endif
