#pragma once
#ifndef ZIPIOS_STREAMENTRY_HPP
#define ZIPIOS_STREAMENTRY_HPP

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
 * \brief Define the zipios::StreamEntry class.
 *
 * This file declares the zipios::StreamEntry class which is used
 * to handle zipios::FileEntry that are represented by a stream.
 * This gives you the ability to create an entry from an in memory
 * stream.
 */

#include "zipios/fileentry.hpp"


namespace zipios
{


class StreamEntry : public FileEntry
{
public:
    typedef std::shared_ptr<StreamEntry>        pointer_t;

                                    StreamEntry(
                                              std::istream & is
                                            , FilePath const & filename
                                            , std::string const & comment = std::string());
    virtual FileEntry::pointer_t    clone() const override;
    virtual                         ~StreamEntry() override;

    virtual bool                    isEqual(FileEntry const & file_entry) const override;
    uint32_t                        computeCRC32() const;
    std::istream &                  getStream() const;

private:
    std::istream &                  f_istream;
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
