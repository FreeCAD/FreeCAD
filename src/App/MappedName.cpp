/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <unordered_set>
#endif

#include "MappedName.h"

#include "Base/Console.h"

//#include <boost/functional/hash.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>


FC_LOG_LEVEL_INIT("MappedName", true, 2);

namespace Data {

void MappedName::compact() const
{
    auto self = const_cast<MappedName*>(this); //FIXME this is a workaround for a single call in ElementMap::addName()

    if (this->raw) {
        self->data = QByteArray(self->data.constData(), self->data.size());
        self->raw = false;
    }

#if 0
    static std::unordered_set<QByteArray, ByteArrayHasher> PostfixSet;
    if (this->postfix.size()) {
        auto res = PostfixSet.insert(this->postfix);
        if (!res.second)
            self->postfix = *res.first;
    }
#endif
}


int MappedName::findTagInElementName(long* tag, int* len, const char* postfix,
                                     char* type, bool negative, bool recursive) const
{
    bool hex = true;
    int pos = this->rfind(POSTFIX_TAG);

    // Example name, tagPosfix == ;:H
    // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
    //                                     ^
    //                                     |
    //                                    pos

    if(pos < 0) {
        pos = this->rfind(POSTFIX_DECIMAL_TAG);
        if (pos < 0)
            return -1;
        hex = false;
    }
    int offset = pos + (int)POSTFIX_TAG_SIZE;
    long _tag = 0;
    int _len = 0;
    char sep = 0;
    char sep2 = 0;
    char tp = 0;
    char eof = 0;

    int size;
    const char *s = this->toConstString(offset, size);

    // check if the number followed by the tagPosfix is negative
    bool isNegative = (s[0] == '-');
    if (isNegative) {
        ++s;
        --size;
    }
    boost::iostreams::stream<boost::iostreams::array_source> iss(s, size);
    if (!hex) {
        // no hex is an older version of the encoding scheme
        iss >> _tag >> sep;
    } else {
        // The purpose of tag postfix is to encode one model operation. The
        // 'tag' field is used to record the own object ID of that model shape,
        // and the 'len' field indicates the length of the operation codes
        // before the tag postfix. These fields are in hex. The trailing 'F' is
        // the shape type of this element, 'F' for face, 'E' edge, and 'V' vertex.
        //
        // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
        //                     |              |   ^^ ^^
        //                     |              |   |   |
        //                     ---len = 0x10---  tag len

        iss >> std::hex;
        // _tag field can be skipped, if it is 0
        if (s[0] == ',' || s[0] == ':')
            iss >> sep;
        else
            iss >> _tag >> sep;
    }

    if (isNegative)
        _tag = -_tag;

    if (sep == ':') {
        // ':' is followed by _len field.
        //
        // For decTagPostfix() (i.e. older encoding scheme), this is the length
        // of the string before the entire postfix (A postfix may contain
        // multiple segments usually separated by ELEMENT_MAP_PREFIX.
        //
        // For newer POSTFIX_TAG, this counts the number of characters that
        // proceeds this tag postfix segment that forms the op code (see
        // example above).
        //
        // The reason of this change is so that the postfix can stay the same
        // regardless of the prefix, which can increase memory efficiency.
        //
        iss >> _len >> sep2 >> tp >> eof;

        // The next separator to look for is either ':' for older tag postfix, or ','
        if (!hex && sep2 == ':')
            sep2 = ',';
    }
    else if (hex && sep == ',') {
        // ',' is followed by a single character that indicates the element type.
        iss >> tp >> eof;
        sep = ':';
        sep2 = ',';
    }

    if (_len < 0 || sep != ':' || sep2 != ',' || tp == 0 || eof != 0)
        return -1;

    if (hex) {
        if (pos-_len < 0)
           return -1;
        if (_len && recursive && (tag || len)) {
            // in case of recursive tag postfix (used by hierarchy element
            // map), look for any embedded tag postfix
            int next = MappedName::fromRawData(*this, pos-_len, _len).rfind(POSTFIX_TAG);
            if (next >= 0) {
                next += pos - _len;
                // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
                //                     ^               ^
                //                     |               |
                //                    next            pos
                //
                // There maybe other operation codes after this embedded tag
                // postfix, search for the sperator.
                //
                int end;
                if (pos == next)
                    end = -1;
                else
                    end = MappedName::fromRawData(*this, next+1, pos-next-1).find(ELEMENT_MAP_PREFIX);
                if (end >= 0) {
                    end += next+1;
                    // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
                    //                            ^
                    //                            |
                    //                           end
                    _len = pos - end;
                    // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
                    //                            |       |
                    //                            -- len --
                } else
                    _len = 0;
            }
        }

        // Now convert the 'len' field back to the length of the remaining name
        //
        // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
        // |                         |
        // ----------- len -----------
        _len = pos - _len;
    }
    if(type)
        *type = tp;
    if(tag) {
        if (_tag == 0 && recursive)
            return MappedName(*this, 0, _len).findTagInElementName(tag, len, postfix, type, negative);
        if(_tag>0 || negative)
            *tag = _tag;
        else
            *tag = -_tag;
    }
    if(len)
        *len = _len;
    if(postfix)
        this->toString(*postfix, pos);
    return pos;
}

}