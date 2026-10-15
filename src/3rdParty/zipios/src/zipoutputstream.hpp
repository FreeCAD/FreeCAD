#pragma once
#ifndef ZIPOUTPUTSTREAM_HPP
#define ZIPOUTPUTSTREAM_HPP

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
 * \brief Define the zipios::ZipOutputStream class.
 *
 * This file includes the class declaration of the zipios::ZipOutputStream
 * which is used to compress data being saved in a Zip archive.
 */

#include "zipoutputstreambuf.hpp"


namespace zipios
{

class ZipOutputStreambuf;

class ZipOutputStream : public std::ostream
{
public:
                    ZipOutputStream(std::ostream & os);
    virtual         ~ZipOutputStream();

    void            closeEntry();
    void            close();
    void            finish();
    void            putNextEntry(FileEntry::pointer_t entry);
    void            setComment(std::string const & comment);

private:
    std::unique_ptr<ZipOutputStreambuf> m_ozf = std::unique_ptr<ZipOutputStreambuf>();
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
