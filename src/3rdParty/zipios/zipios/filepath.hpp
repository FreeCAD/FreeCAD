#pragma once
#ifndef ZIPIOS_FILEPATH_HPP
#define ZIPIOS_FILEPATH_HPP

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

/** \file
 * \brief Define the zipios::FilePath class.
 *
 * zipios::FilePath is used to manage paths and filenames to files
 * and directories.
 *
 * It is used by the zipios::FileEntry to represent the virtual filename
 * that the virtual file has.
 */

#include "zipios/zipios-config.hpp"

#include <ctime>
#include <string>


namespace zipios
{


class FilePath
{
public:
                        FilePath(std::string const & path = std::string());

                        operator std::string () const;
    FilePath &          operator = (std::string const & path);
    FilePath            operator + (FilePath const & name) const;
    bool                operator == (char const * rhs) const;
    friend bool         operator == (char const * lhs, FilePath const & rhs);
    bool                operator == (std::string const & rhs) const;
    friend bool         operator == (std::string const & lhs, FilePath const & rhs);
    bool                operator == (FilePath const & rhs) const;
    // TBD: add all the other comparison operators for completeness
    void                clear();
    std::string         filename() const;
    size_t              length() const;
    size_t              size() const;
    bool                empty() const;
    bool                exists() const;
    bool                isRegular() const;
    bool                isDirectory() const;
    bool                isCharSpecial() const;
    bool                isBlockSpecial() const;
    bool                isSocket() const;
    bool                isFifo() const;
    size_t              fileSize() const;
    std::time_t         lastModificationTime() const;

private:
    void                check() const;

    std::string         m_path = std::string();
    mutable os_stat_t   m_stat = {};
    mutable bool        m_checked = false;
    mutable bool        m_exists = false;
};


std::ostream & operator << (std::ostream & os, FilePath const & path);


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
#endif
