#pragma once
#ifndef DEFLATEOUTPUTSTREAMBUF_HPP
#define DEFLATEOUTPUTSTREAMBUF_HPP

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
 * \brief Header file that defines zipios::DeflateOutputStreambuf.
 *
 * This file is used to declare the zipios::DeflateOutputStreambuf class which
 * is used to compress data using zlib.
 *
 * The counterpart is the class zipios::InflateInputStreambuf.
 */

#include "filteroutputstreambuf.hpp"

#include "zipios/fileentry.hpp"

#include <cstdint>

#include <zlib.h>


namespace zipios
{

class DeflateOutputStreambuf : public FilterOutputStreambuf
{
public:
                            DeflateOutputStreambuf(std::streambuf * outbuf);
                            DeflateOutputStreambuf(DeflateOutputStreambuf const & rhs) = delete;
    virtual                 ~DeflateOutputStreambuf();

    DeflateOutputStreambuf & operator = (DeflateOutputStreambuf const & rhs) = delete;

    bool                    init(FileEntry::CompressionLevel compression_level);
    void                    closeStream();
    uint32_t                getCrc32() const;
    size_t                  getSize() const;

protected:
    virtual int             overflow(int c = EOF);
    virtual int             sync();

    uint32_t                m_overflown_bytes = 0;
    std::vector<char>       m_invec = std::vector<char>();
    uint32_t                m_crc32 = 0;

private:
    void                    endDeflation();
    void                    flushOutvec();

    z_stream                m_zs = z_stream();
    bool                    m_zs_initialized = false;

    std::vector<char>       m_outvec = std::vector<char>();
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
