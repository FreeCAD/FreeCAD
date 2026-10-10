#pragma once
#ifndef GZIPOUTPUTSTREAM_HPP
#define GZIPOUTPUTSTREAM_HPP

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
 * \brief This file defines zipios::GZIPOutputStream.
 *
 * This file declares the zipios::GZipOutputStream class which handles
 * Zip archive content while saving to file.
 */

#include "gzipoutputstreambuf.hpp"

#include <memory>


namespace zipios
{


class GZIPOutputStream : public std::ostream
{
public:
                                            GZIPOutputStream(std::ostream & os, FileEntry::CompressionLevel compression_level);
                                            GZIPOutputStream(std::string const & filename, FileEntry::CompressionLevel compression_level);
    virtual                                 ~GZIPOutputStream();

    void                                    setFilename(std::string const & filename);
    void                                    setComment(std::string const & comment);
    void                                    close();
    void                                    finish();

private:
    std::unique_ptr<std::ofstream>          m_ofs = std::unique_ptr<std::ofstream>();
    std::unique_ptr<GZIPOutputStreambuf>    m_ozf = std::unique_ptr<GZIPOutputStreambuf>();
};


} // zipios namespace.

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
#endif
