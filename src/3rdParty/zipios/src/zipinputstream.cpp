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
 * \brief Implementation of zipios::ZipInputStream.
 *
 * This file includes the implementation of the zipios::ZipInputStream
 * class which is a filter used to read files from Zip archives, files
 * that can be compressed using the zlib library.
 */

#include "zipinputstream.hpp"

#include <fstream>


namespace zipios
{


/** \class ZipInputStream
 * \brief The ZipInputStream to read data from a Zip archive.
 *
 * ZipInputStream is an istream that gets its input from a zip file. The
 * interface was redesigned in version 2.x to be more C++ like.
 *
 * \note
 * The getNextEntry() was removed because we cannot make it work here.
 * The old implementation would let someone read all the local directory
 * entries one after another. Only that is not correct and since this class
 * is not publicly exposed anymore, it wouldn't be available anyway.
 */


/** \brief Initialize a ZipInputStream from a filename and position.
 *
 * This constructor creates a ZIP file stream by attaching itself to
 * a file as defined by the specified filename and a position to the
 * header of the file being read.
 *
 * \param[in] filename  The name of a valid zip file.
 * \param[in] pos position to reposition the istream to before reading.
 */
ZipInputStream::ZipInputStream(std::string const & filename, std::streampos pos)
    : std::istream(nullptr)
    , m_ifs(std::make_unique<std::ifstream>(filename, std::ios::in | std::ios::binary))
    , m_ifs_ref(*m_ifs)
    , m_izf(std::make_unique<ZipInputStreambuf>(m_ifs_ref.rdbuf(), pos))
{
    // properly initialize the stream with the newly allocated buffer
    init(m_izf.get());
}


ZipInputStream::ZipInputStream(std::istream & is)
    : std::istream(nullptr)
    , m_ifs_ref(is)
    , m_izf(std::make_unique<ZipInputStreambuf>(m_ifs_ref.rdbuf(), 0))
{
    // properly initialize the stream with the newly allocated buffer
    init(m_izf.get());
}


/** \brief Clean up the input stream.
 *
 * The destructor ensures that all resources used by the class get
 * released.
 */
ZipInputStream::~ZipInputStream()
{
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
