#pragma once
#ifndef BACKBUFFER_HPP
#define BACKBUFFER_HPP

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
 * \brief The header file for zipios::BackBuffer
 *
 * The zipios::BackBuffer class is used to read a file backward.
 */

#include "zipios/virtualseeker.hpp"

#include "zipios_common.hpp"


namespace zipios
{


class BackBuffer : public ::zipios::buffer_t
{
public:
                            BackBuffer(std::istream & is, VirtualSeeker const & vs = VirtualSeeker(), ssize_t const chunk_size = 1024);

    ssize_t                 readChunk(ssize_t & read_pointer);

private:
    VirtualSeeker           m_vs = VirtualSeeker();
    ssize_t                 m_chunk_size = 0;
    std::istream &          m_is;
    std::streampos          m_file_pos = 0;
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
