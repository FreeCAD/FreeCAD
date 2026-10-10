#pragma once
#ifndef ZIPIOS_ZIPFILE_HPP
#define ZIPIOS_ZIPFILE_HPP

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
 * \brief Define the zipios::ZipFile class.
 *
 * This file defines the main class of the Zipios library. The
 * zipios::ZipFile class is the one used to read or write
 * Zip archives.
 *
 * Note that a Zip archive is a zipios::FileCollection. When reading,
 * you get zipios::FileEntry objects that are defined internally
 * such as the zipios::ZipCDirEntry. When writing, you get
 * zipios::FileEntry objects from a zipios::DirectoryCollection.
 */

#include "zipios/filecollection.hpp"
#include "zipios/virtualseeker.hpp"


namespace zipios
{


class ZipFile : public FileCollection
{
public:
    static pointer_t            openEmbeddedZipFile(std::string const & filename);

                                ZipFile();
                                ZipFile(std::string const & filename, offset_t s_off = 0, offset_t e_off = 0);
                                ZipFile(std::istream & is, offset_t s_off = 0, offset_t e_off = 0);
    virtual pointer_t           clone() const override;
    virtual                     ~ZipFile() override;

    virtual stream_pointer_t    getInputStream(
                                          std::string const & entry_name
                                        , MatchPath matchpath = MatchPath::MATCH) override;
    static void                 saveCollectionToArchive(
                                          std::ostream & os
                                        , FileCollection & collection
                                        , std::string const & zip_comment = std::string());

private:
    void                        init(std::istream & is);

    VirtualSeeker               m_vs = VirtualSeeker();
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
