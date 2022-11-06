/****************************************************************************
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <cstdlib>
# include <unordered_set>
#endif

#include <QHash>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include "DocumentObject.h"
#include "MappedElement.h"

using namespace Data;

struct ByteArray
{
    ByteArray(const QByteArray &b)
        :bytes(b)
    {}

    ByteArray(const ByteArray &other)
        :bytes(other.bytes)
    {}

    ByteArray(ByteArray &&other)
        :bytes(std::move(other.bytes))
    {}

    void mutate() const
    {
        QByteArray copy;
        copy.append(bytes.constData(), bytes.size());
        bytes = copy;
    }

    bool operator==(const ByteArray & other) const {
        return bytes == other.bytes;
    }

    mutable QByteArray bytes;
};

struct ByteArrayHasher
{
    std::size_t operator()(const ByteArray &bytes) const
    {
        return qHash(bytes.bytes);
    }

    std::size_t operator()(const QByteArray &bytes) const
    {
        return qHash(bytes);
    }
};

void IndexedName::set(const char *name,
                      int len,
                      const std::vector<const char*> &types,
                      bool allowOthers)
{
    static std::unordered_set<ByteArray, ByteArrayHasher> NameSet;

    if (len < 0)
        len = static_cast<int>(std::strlen(name));
    int i;
    for (i=len-1; i>=0; --i) {
        if (name[i]<'0' || name[i]>'9')
            break;
    }
    ++i;
    this->index = std::atoi(name+i);

    for (int j=0; j<i; ++j) {
        if (name[j] == '_'
                || (name[j] >= 'a' && name[j] <= 'z' )
                || (name[j] >= 'A' && name[j] <= 'Z'))
            continue;
        this->type = "";
        return;
    }

    for (const char * type : types) {
        int j=0;
        for (const char *n=name, *t=type; *n; ++n) {
            if (*n != *t || j >= i)
                break;
            ++j;
            ++t;
            if (!*t) {
                this->type = type;
                return;
            }
        }
    }

    if (allowOthers) {
        auto res = NameSet.insert(QByteArray::fromRawData(name, i));
        if (res.second)
           res.first->mutate();
        this->type = res.first->bytes.constData();
    } else
        this->type = "";
}

void MappedName::compact() const
{
    auto self = const_cast<MappedName*>(this);

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

bool ElementNameComp::operator()(const MappedName &a, const MappedName &b) const {
    size_t size = std::min(a.size(),b.size());
    if(!size)
        return a.size()<b.size();
    size_t i=0;
    if(b[0] == '#') {
        if(a[0]!='#')
            return true;
        // If both string starts with '#', compare the following hex digits by
        // its integer value.
        int res = 0;
        for(i=1;i<size;++i) {
            unsigned char ac = (unsigned char)a[i];
            unsigned char bc = (unsigned char)b[i];
            if(std::isxdigit(bc)) {
                if(!std::isxdigit(ac))
                    return true;
                if(res==0) {
                    if(ac<bc)
                        res = -1;
                    else if(ac>bc)
                        res = 1;
                }
            }else if(std::isxdigit(ac))
                return false;
            else
                break;
        }
        if(res < 0)
            return true;
        else if(res > 0)
            return false;

        for (; i<size; ++i) {
            char ac = a[i];
            char bc = b[i];
            if (ac < bc)
                return true;
            if (ac > bc)
                return false;
        }
        return a.size()<b.size();
    }
    else if (a[0] == '#')
        return false;

    // If the string does not start with '#', compare the non-digits prefix
    // using lexical order.
    for(i=0;i<size;++i) {
        unsigned char ac = (unsigned char)a[i];
        unsigned char bc = (unsigned char)b[i];
        if(!std::isdigit(bc)) {
            if(std::isdigit(ac))
                return true;
            if(ac<bc)
                return true;
            if(ac>bc)
                return false;
        } else if(!std::isdigit(ac)) {
            return false;
        } else
            break;
    }

    // Then compare the following digits part by integer value
    int res = 0;
    for(;i<size;++i) {
        unsigned char ac = (unsigned char)a[i];
        unsigned char bc = (unsigned char)b[i];
        if(std::isdigit(bc)) {
            if(!std::isdigit(ac))
                return true;
            if(res==0) {
                if(ac<bc)
                    res = -1;
                else if(ac>bc)
                    res = 1;
            }
        }else if(std::isdigit(ac))
            return false;
        else
            break;
    }
    if(res < 0)
        return true;
    else if(res > 0)
        return false;

    // Finally, compare the remaining tail using lexical order
    for (; i<size; ++i) {
        char ac = a[i];
        char bc = b[i];
        if (ac < bc)
            return true;
        if (ac > bc)
            return false;
    }
    return a.size()<b.size();
}

HistoryItem::HistoryItem(App::DocumentObject *obj, const Data::MappedName &name)
    :obj(obj),tag(0),element(name)
{
    if(obj)
        tag = obj->getID();
}

const std::string & MappedChildElements::prefix()
{
    static std::string _prefix(ComplexGeoData::elementMapPrefix() + ":R");
    return _prefix;
}
