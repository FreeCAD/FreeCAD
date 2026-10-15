#pragma once
#ifndef ZIPCENTRALDIRECTORYENTRY_HPP
#define ZIPCENTRALDIRECTORYENTRY_HPP

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
 * \brief Declaration of the zipios::ZipCentralDirectoryEntry, which represents a
 *        directory Zip archive entry.
 *
 * This header file contains the zipios::ZipCentralDirectoryEntry class used
 * to read the central directory fields found in a zip archive.
 * It can also be used to write a central directory to an output
 * Zip archive.
 */

#include "ziplocalentry.hpp"


namespace zipios
{


class ZipCentralDirectoryEntry : public ZipLocalEntry
{
public:
                                ZipCentralDirectoryEntry();
                                ZipCentralDirectoryEntry(FileEntry const & entry);
    virtual pointer_t           clone() const override;
    virtual                     ~ZipCentralDirectoryEntry() override;

    virtual size_t              getHeaderSize() const override;

    virtual void                read(std::istream & is) override;
    virtual void                write(std::ostream & os) override;
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
