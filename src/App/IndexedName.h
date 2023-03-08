// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#ifndef _IndexedName_h_
#define _IndexedName_h_

#include <cassert>
#include <cstring>
#include <ostream>
#include <string>
#include <vector>

#include <QByteArray>

#include "FCGlobal.h"


namespace Data
{

class AppExport IndexedName {
public:
    explicit IndexedName(const char *name = nullptr, int _index = 0)
        : index(0)
    {
        if (!name)
            this->type = "";
        else {
            set(name);
            if (_index)
                this->index = _index;
        }
    }

    IndexedName(const char *name,
                const std::vector<const char*> & types,
                bool allowOthers=true)
    {
        set(name, -1, types, allowOthers);
    }

    explicit IndexedName(const QByteArray & data)
    {
        set(data.constData(), data.size());
    }

    IndexedName(const IndexedName &other)
        : type(other.type), index(other.index)
    {}

    static IndexedName fromConst(const char *name, int index) {
        IndexedName res;
        res.type = name;
        res.index = index;
        return res;
    }

    IndexedName & operator=(const IndexedName & other)
    {
        this->index = other.index;
        this->type = other.type;
        return *this;
    }

    friend std::ostream & operator<<(std::ostream & s, const IndexedName & e)
    {
        s << e.type;
        if (e.index > 0)
            s << e.index;
        return s;
    }

    bool operator==(const IndexedName & other) const
    {
        return this->index == other.index
            && (this->type == other.type
                    || std::strcmp(this->type, other.type)==0);
    }

    IndexedName & operator+=(int offset)
    {
        this->index += offset;
        assert(this->index >= 0);
        return *this;
    }

    IndexedName & operator++()
    {
        ++this->index;
        return *this;
    }

    IndexedName & operator--()
    {
        --this->index;
        assert(this->index >= 0);
        return *this;
    }

    bool operator!=(const IndexedName & other) const
    {
        return !(this->operator==(other));
    }

    const char * toString(std::string & s) const
    {
        // Note! s is not cleared on purpose.
        std::size_t offset = s.size();
        s += this->type;
        if (this->index > 0)
            s += std::to_string(this->index);
        return s.c_str() + offset;
    }

    int compare(const IndexedName & other) const
    {
		int res = std::strcmp(this->type, other.type);
        if (res)
            return res;
        if (this->index < other.index)
            return -1;
        if (this->index > other.index)
            return 1;
        return 0;
    }

    bool operator<(const IndexedName & other) const
    {
        return compare(other) < 0;
    }

	char operator[](int index) const
	{
		return this->type[index];
	}

	const char * getType() const { return this->type; }

	int getIndex() const { return this->index; }

    void setIndex(int index) { assert(index>=0); this->index = index; }

	bool isNull() const { return !this->type[0]; }

    explicit operator bool() const { return !isNull(); }

protected:
    void set(const char *,
             int len = -1,
             const std::vector<const char *> &types = {},
             bool allowOthers = true);

private:
    const char * type;
    int index;
};

}

#endif _IndexedName_h_
