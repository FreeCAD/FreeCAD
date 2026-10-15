#pragma once
#ifndef ENDOFCENTRALDIRECTORY_HPP
#define ENDOFCENTRALDIRECTORY_HPP

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
 * \brief Declaration of the zipios::ZipEndOfCentralDirectory class.
 *
 * This file contains the class used to handle the "end of central directory"
 * information found in a Zip archive.
 *
 * It includes the necessary functions to read and write the central
 * directory and local header fields in a Zip archive.
 */

#include "zipios_common.hpp"

#include <string>


namespace zipios
{


class ZipEndOfCentralDirectory
{
public:
                        ZipEndOfCentralDirectory(std::string const & zip_comment = std::string());

    size_t              getCentralDirectorySize() const;
    size_t              getCount() const;
    offset_t            getOffset() const;
    void                setCentralDirectorySize(size_t size);
    void                setCount(size_t c);
    void                setOffset(offset_t new_offset);

    bool                read(::zipios::buffer_t const & buf, size_t pos);
    void                write(std::ostream & os);

private:
    // some of the fields found in a Zip archive ZipEndOfCentralDirectory
    size_t              m_central_directory_entries = 0;
    size_t              m_central_directory_size = 0;
    offset_t            m_central_directory_offset = 0;
    std::string         m_zip_comment = std::string();
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
