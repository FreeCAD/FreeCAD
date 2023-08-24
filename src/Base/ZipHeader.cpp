/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#include "ZipHeader.h"
#include <zipios++/zipinputstream.h>

using zipios::ConstEntryPointer;
using zipios::FileCollection;
using zipios::ZipHeader;


ZipHeader::ZipHeader(std::istream &inp, int s_off, int e_off)
    : _input(inp)
    , _vs(s_off, e_off)
{
    init(_input);
}

/** Create a copy of this instance. */
FileCollection *ZipHeader::clone() const
{
    return new ZipHeader(*this);
}

void ZipHeader::close()
{
    _valid = false;
}

std::istream *ZipHeader::getInputStream(const ConstEntryPointer &entry)
{
    if (!_valid)
        throw zipios::InvalidStateException("Attempt to use an invalid FileCollection");
    return getInputStream(entry->getName());
}

std::istream *ZipHeader::getInputStream(const std::string &entry_name, MatchPath matchpath)
{
    if (!_valid)
        throw zipios::InvalidStateException("Attempt to use an invalid ZipHeader");

    zipios::ConstEntryPointer ent = getEntry(entry_name, matchpath);

    if (!ent)
        return nullptr;
    else
        return new zipios::ZipInputStream(_input,
                                          static_cast<const zipios::ZipCDirEntry *>(ent.get())->getLocalHeaderOffset() + _vs.startOffset());
}

bool ZipHeader::init(std::istream &_zipfile)
{
    // Check stream error state
    if (!_zipfile) {
        setError("Error reading from file");
        return false;
    }

    _valid = readCentralDirectory(_zipfile);
    return _valid;
}

bool ZipHeader::readCentralDirectory(std::istream &_zipfile)
{
    // Find and read eocd.
    if (!readEndOfCentralDirectory(_zipfile))
        throw zipios::FCollException("Unable to find zip structure: End-of-central-directory");

    // Position read pointer to start of first entry in central dir.
    _vs.vseekg(_zipfile, _eocd.offset(), std::ios::beg);

    int entry_num = 0;
    // Giving the default argument in the next line to keep Visual C++ quiet
    _entries.resize(_eocd.totalCount(), nullptr);
    while ((entry_num < _eocd.totalCount())) {
        zipios::ZipCDirEntry *ent = new zipios::ZipCDirEntry;
        _entries[entry_num] = ent;
        _zipfile >> *ent;
        if (!_zipfile) {
            if (_zipfile.bad())
                throw zipios::IOException("Error reading zip file while reading zip file central directory");
            else if (_zipfile.fail())
                throw zipios::FCollException("Zip file consistency problem. Failure while reading zip file central directory");
            else if (_zipfile.eof())
                throw zipios::IOException("Premature end of file while reading zip file central directory");
        }
        ++entry_num;
    }

    // Consistency check. eocd should start here

    int pos = _vs.vtellg(_zipfile);
    _vs.vseekg(_zipfile, 0, std::ios::end);
    int remaining = static_cast<int>(_vs.vtellg(_zipfile)) - pos;
    if (remaining != _eocd.eocdOffSetFromEnd())
        throw zipios::FCollException("Zip file consistency problem. Zip file data fields are inconsistent with zip file layout");

    // Consistency check 2, are local headers consistent with
    // cd headers
    if (!confirmLocalHeaders(_zipfile))
        throw zipios::FCollException("Zip file consistency problem. Zip file data fields are inconsistent with zip file layout");

    return true;
}

bool ZipHeader::readEndOfCentralDirectory(std::istream &_zipfile)
{
    zipios::BackBuffer bb(_zipfile, _vs);
    int read_p = -1;
    bool found = false;
    while (!found) {
        if (read_p < 0) {
            if (!bb.readChunk(read_p)) {
                found = false;
                break;
            }
        }
        if (_eocd.read(bb, read_p)) {
            found = true;
            break;
        }
        --read_p;
    }

    return found;
}

bool ZipHeader::confirmLocalHeaders(std::istream &_zipfile)
{
    zipios::Entries::const_iterator it;
    zipios::ZipCDirEntry *ent{};
    int inconsistencies = 0;
    zipios::ZipLocalEntry zlh;
    for (it = _entries.begin(); it != _entries.end(); ++it) {
        ent = static_cast<zipios::ZipCDirEntry *>((*it).get());
        _vs.vseekg(_zipfile, ent->getLocalHeaderOffset(), std::ios::beg);
        _zipfile >> zlh;
        if (!_zipfile || zlh != *ent) {
            inconsistencies++;
            _zipfile.clear();
        }
    }
    return !inconsistencies;
}

void ZipHeader::setError(std::string /*error_str*/)
{
    _valid = false;
}
