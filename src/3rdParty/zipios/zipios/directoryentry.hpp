#pragma once
#ifndef ZIPIOS_DIRECTORYENTRY_HPP
#define ZIPIOS_DIRECTORYENTRY_HPP

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
 * \brief Define the zipios::DirectoryEntry class.
 *
 * This file declares the zipios::DirectoryEntry class which is used
 * to handle zipios::FileEntry that are found when reading a directory
 * on disk.
 *
 * \sa zipios::DirectoryCollection
 */

#include "zipios/fileentry.hpp"


namespace zipios
{


class DirectoryEntry : public FileEntry
{
public:
                            DirectoryEntry(FilePath const & filename, std::string const & comment = std::string());
    virtual pointer_t       clone() const override;
    virtual                 ~DirectoryEntry() override;

    virtual bool            isEqual(FileEntry const & file_entry) const override;
    uint32_t                computeCRC32() const;
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
