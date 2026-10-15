#pragma once
#ifndef GZIPOUTPUTSTREAMBUF_HPP
#define GZIPOUTPUTSTREAMBUF_HPP

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
 * \brief File defining zipios::GZIPOutputStreambuf.
 *
 * This file includes the declaration of the zipios::GZIPOutputStreambuf
 * class which is used to output a file in a Zip archive when we want to
 * compress it with the zlib library.
 */

#include "deflateoutputstreambuf.hpp"


namespace zipios
{


class GZIPOutputStreambuf : public DeflateOutputStreambuf
{
public:
                  GZIPOutputStreambuf(std::streambuf * outbuf, FileEntry::CompressionLevel compression_level);
    virtual       ~GZIPOutputStreambuf() override;

    void          setFilename(std::string const & filename);
    void          setComment(std::string const & comment);
    void          close();
    void          finish();

protected:
    virtual int   overflow(int c = EOF) override;
    virtual int   sync() override;

private:
    void          writeHeader();
    void          writeTrailer();
    void          writeInt(uint32_t i);

    std::string   m_filename = std::string();
    std::string   m_comment = std::string();
    bool          m_open = false;
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
