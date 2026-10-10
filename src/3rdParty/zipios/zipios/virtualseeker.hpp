#pragma once
#ifndef ZIPIOS_VIRTUALSEEKER_HPP
#define ZIPIOS_VIRTUALSEEKER_HPP

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
 * \brief Define the zipios::VirtualSeeker class.
 *
 * The zipios::VirtualSeeker class defines a start offset and end offset
 * that are used to represent a Zip archive in a larger file.
 *
 * This can be used as a file that include resources useful to your
 * software.
 *
 * \sa zipios::openEmbeddedZipFile()
 * \sa appendzip.cpp
 */

#include "zipios/zipios-config.hpp"


namespace zipios
{


class VirtualSeeker
{
public:
                    VirtualSeeker(offset_t start_offset = 0, offset_t end_offset = 0);

    void            setOffsets(offset_t start_offset, offset_t end_offset);
    void            getOffsets(offset_t & start_offset, offset_t & end_offset) const;
    offset_t        startOffset() const;
    offset_t        endOffset() const;
    void            vseekg(std::istream & is, offset_t offset, std::ios::seekdir sd) const;
    std::streampos  vtellg(std::istream & is) const;

private:
    offset_t        m_start_offset = 0;
    offset_t        m_end_offset = 0;
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
