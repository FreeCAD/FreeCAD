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


#ifndef _AppMappedName_h_
#define _AppMappedName_h_


#include <string>

#include <boost/algorithm/string/predicate.hpp>

#include <QByteArray>
#include <QHash>

#include "ComplexGeoData.h"


namespace Data
{


class AppExport MappedName
{
public:

	explicit MappedName(const char * name, int size = -1)
		: raw(false)
	{
        if (!name) return;
        if (boost::starts_with(name, ComplexGeoData::elementMapPrefix()))
            name += ComplexGeoData::elementMapPrefix().size();

        data = size < 0 ? QByteArray(name) : QByteArray(name, size);
    }

	explicit MappedName(const std::string & name)
        : raw(false) 
    {
        int size = name.size();
        const char *n = name.c_str();
        if (boost::starts_with(name, ComplexGeoData::elementMapPrefix())) {
            n += ComplexGeoData::elementMapPrefix().size();
            size -= ComplexGeoData::elementMapPrefix().size();
        }
        data = QByteArray(n, size);
    }
/*
    explicit MappedName(const IndexedName & element)
        : data(element.getType()), raw(false)
    {
        if (element.getIndex() > 0)
            data += QByteArray::number(element.getIndex());
    }
*/
    MappedName()
        : raw(false)
    {}

	MappedName(const MappedName & other)
		: data(other.data), postfix(other.postfix), raw(other.raw)
	{}

	MappedName(const MappedName & other, int startpos, int size = -1)
        : raw(false)
    {
        append(other, startpos, size);
    }

	MappedName(const MappedName & other, const char *postfix)
		: data(other.data + other.postfix), postfix(postfix), raw(false)
	{}

#if QT_VERSION  >= 0x050200
    MappedName(MappedName &&other)
        : data(std::move(other.data)), postfix(std::move(other.postfix)), raw(other.raw)
    {}
#endif

    static MappedName fromRawData(const char * name, int size = -1)
    {
        MappedName res;
        if (name) {
            res.data = QByteArray::fromRawData(name, size>=0 ? size : qstrlen(name));
            res.raw = true;
        }
        return res;
    }

    static MappedName fromRawData(const QByteArray & data)
    {
        return fromRawData(data.constData(), data.size());
    }

    static MappedName fromRawData(const MappedName &other, int startpos, int size = -1)
    {
        if (startpos < 0)
            startpos = 0;

        if (startpos >= other.size())
            return MappedName();

        if (startpos >= other.data.size())
            return MappedName(other, startpos, size);

        MappedName res;
        res.raw = true;
        if (size < 0)
            size = other.size() - startpos;

        if (size < other.data.size() - startpos) {
            res.data = QByteArray::fromRawData(other.data.constData() + startpos, size);
        }
        else {
            res.data = QByteArray::fromRawData(other.data.constData() + startpos, other.data.size() - startpos);
            size -= other.data.size() - startpos;
            if (size == other.postfix.size())
                res.postfix = other.postfix;
            else if (size)
                res.postfix.append(other.postfix.constData(), size);
        }
        return res;
    }

    MappedName & operator=(const MappedName & other)
    {
		this->data = other.data;
		this->postfix = other.postfix;
        this->raw = other.raw;
        return *this;
    }

    MappedName & operator=(const std::string & other)
    {
        *this = MappedName(other);
        return *this;
    }

    MappedName & operator=(const char * other)
    {
        *this = MappedName(other);
        return *this;
    }


#if QT_VERSION  >= 0x050200
    MappedName & operator=(MappedName &&other)
    {
        this->data = std::move(other.data);
        this->postfix = std::move(other.postfix);
        this->raw  = other.raw;
        return *this;
    }
#endif

    friend std::ostream & operator<<(std::ostream & s, const MappedName & n)
    {
        s.write(n.data.constData(), n.data.size());
        s.write(n.postfix.constData(), n.postfix.size());
        return s;
    }

    bool operator==(const MappedName & other) const
    {
        if (this->size() != other.size()) {
            return false;
        }

        if (this->data.size() == other.data.size()) {
            return this->data == other.data && this->postfix == other.postfix;
        }

        const auto &smaller = this->data.size() < other.data.size() ? *this : other;
        const auto &larger = this->data.size() < other.data.size() ? other: *this;
        
        if (!larger.data.startsWith(smaller.data)) {
            return false;
        }

        QByteArray tmp = QByteArray::fromRawData(
                larger.data.constData() + smaller.data.size(),
                larger.data.size() - smaller.data.size()
                );
        
        if (!smaller.postfix.startsWith(tmp)) {
            return false;
        }

        tmp = QByteArray::fromRawData(
                smaller.postfix.constData() + tmp.size(),
                smaller.postfix.size() - tmp.size()
                );

        return tmp == larger.postfix;
    }

    bool operator!=(const MappedName & other) const
    {
        return !(this->operator==(other));
    }

	MappedName operator+(const MappedName & other) const
	{
        MappedName res(*this);
        res += other;
        return res;
	}

	MappedName operator+(const char * other) const
	{
        MappedName res(*this);
        res += other;
        return res;
	}

	MappedName operator+(const std::string & other) const
	{
        MappedName res(*this);
        res += other;
        return res;
	}

	MappedName operator+(const QByteArray & other) const
	{
        MappedName res(*this);
        res += other;
        return res;
	}

	MappedName & operator+=(const char * other)
	{
        if (other && other[0])
            this->postfix.append(other, -1);
		return *this;
	}

	MappedName & operator+=(const std::string & other)
	{
        if (other.size()) {
            this->postfix.reserve(this->postfix.size() + other.size());
            this->postfix.append(other.c_str(), other.size());
        }
		return *this;
	}

	MappedName & operator+=(const QByteArray & other)
	{
        this->postfix += other;
		return *this;
	}

	MappedName & operator+=(const MappedName & other)
	{
        append(other);
		return *this;
	}

    void append(const char * d, int size = -1)
    {
        if (d && size) {
            if (size < 0)
                size = qstrlen(d);
            if (empty())
                this->data.append(d, size);
            else
                this->postfix.append(d, size);
        }
    }

    void append(const MappedName & other, int startpos = 0, int size = -1)
    {
        if (startpos < 0)
            startpos = 0;
        else if (startpos > other.size())
            return;
        if (size < 0 || size + startpos > other.size())
            size = other.size() - startpos;

        int count = size;
        if (startpos < other.data.size()) {
            if (count > other.data.size() - startpos)
                count = other.data.size() - startpos;
            if (startpos == 0 && count == other.data.size() && this->empty()) {
                this->data = other.data;
                this->raw = other.raw;
            } else
                append(other.data.constData() + startpos, count);
            startpos = 0;
            size -= count;
        } else
            startpos -= other.data.size();
        if (size) {
            if (startpos == 0 && size == other.postfix.size()) {
                if (this->empty())
                    this->data = other.postfix;
                else if (this->postfix.isEmpty())
                    this->postfix = other.postfix;
                else
                    this->postfix += other.postfix;
            } else
                append(other.postfix.constData() + startpos, size);
        }
    }

    std::string toString(int startpos, int len=-1) const
    {
        std::string res;
        return toString(res, startpos, len);
    }

    const char * toString(std::string &s, int startpos=0, int len=-1) const
    {
        std::size_t offset = s.size();
        int count = this->size();
        if (startpos < 0)
            startpos = 0;
        else if (startpos >= count)
            return s.c_str()+s.size();
        if (len < 0 || len > count - startpos)
            len = count - startpos;
        s.reserve(s.size() + len);
        if (startpos < this->data.size()) {
            count = this->data.size() - startpos;
            if (len < count)
                count = len;
            s.append(this->data.constData()+startpos, count);
            len -= count;
        }
        s.append(this->postfix.constData(), len);
        return s.c_str() + offset;
    }

    const char * toConstString(int offset, int &size) const
    {
        if (offset < 0)
            offset = 0;
        if (offset > this->data.size()) {
            offset -= this->data.size();
            if (offset > this->postfix.size()) {
                size = 0;
                return "";
            }
            size = this->postfix.size() - offset;
            return this->postfix.constData() + offset;
        }
        size = this->data.size() - offset;
        return this->data.constData() + offset;
    }

    QByteArray toRawBytes(int offset=0, int size=-1) const
    {
        if (offset < 0)
            offset = 0;
        if (offset >= this->size())
            return QByteArray();
        if (size < 0 || size > this->size() - offset)
            size = this->size() - offset;
        if (offset >= this->data.size()) {
            offset -= this->data.size();
            return QByteArray::fromRawData(this->postfix.constData()+offset, size);
        }
        if (size <= this->data.size() - offset)
            return QByteArray::fromRawData(this->data.constData()+offset, size);

        QByteArray res(this->data.constData()+offset, this->data.size()-offset);
        res.append(this->postfix.constData(), size - this->data.size() + offset);
        return res;
    }

    const QByteArray & dataBytes() const
    {
        return this->data;
    }

    const QByteArray & postfixBytes() const
    {
        return this->postfix;
    }

    const char * constPostfix() const
    {
        return this->postfix.constData();
    }

    // No constData() because 'data' is allow to contain raw data, which may
    // not end with 0.
#if 0
    void char * constData() const
    {
        return this->data.constData();
    }
#endif

    QByteArray toBytes() const
    {
        if (this->postfix.isEmpty())
            return this->data;
        if (this->data.isEmpty())
            return this->postfix;
        return this->data + this->postfix;
    }
/*
    IndexedName toIndexedName() const
    {
        if (this->postfix.isEmpty())
            return IndexedName(this->data);
        return IndexedName();
    }

    std::string toPrefixedString() const
    {
        std::string res;
        toPrefixedString(res);
        return res;
    }

    const char *toPrefixedString(std::string &buf) const
    {
        if (!toIndexedName())
            buf += ComplexGeoData::elementMapPrefix();
        toString(buf);
        return buf.c_str();
    }
*/    
    int compare(const MappedName &other) const
    {
        int asize = this->size();
        int bsize = other.size();
        for (int i=0, count=std::min(asize, bsize); i<count; ++i) {
            char a = this->operator[](i);
            char b = other[i];
            if (a < b)
                return -1;
            if (a > b)
                return 1;
        }
        if (asize < bsize)
            return -1;
        if (asize > bsize)
            return 1;
        return 0;
    }

    bool operator<(const MappedName & other) const
    {
        return compare(other) < 0;
    }

	char operator[](int index) const
	{
        if (index >= this->data.size())
            return this->postfix[index - this->data.size()];
		return this->data[index];
	}

	int size() const
    {
        return this->data.size() + this->postfix.size();
    }

	bool empty() const
    {
        return this->data.isEmpty() && this->postfix.isEmpty();
    }

    bool isRaw() const
    {
        return this->raw;
    }

    MappedName copy() const
    {
        if (!this->raw)
            return *this;
        MappedName res;
        res.data.append(this->data.constData(), this->data.size());
        res.postfix = this->postfix;
        return res;
    }

    void compact() const;

    explicit operator bool() const
    {
        return !empty();
    }

	void clear()
    {
        this->data.clear();
        this->postfix.clear();
        this->raw = false;
    }

    int find(const char *d, int startpos = 0) const
    {
        if (!d)
            return -1;
        if (startpos < 0)
            startpos = 0;
        if (startpos < this->data.size()) {
            int res = this->data.indexOf(d, startpos);
            if (res >= 0)
                return res;
            startpos = 0;
        } else
            startpos -= this->data.size();
        int res = this->postfix.indexOf(d, startpos);
        if (res < 0)
            return res;
        return res + this->data.size();
    }

    int find(const std::string &d, int startpos = 0) const
    {
        return find(d.c_str(), startpos);
    }

    int rfind(const char *d, int startpos = -1) const
    {
        if (!d)
            return -1;
        if (startpos < 0 || startpos > this->postfix.size()) {
            if (startpos > postfix.size())
                startpos -= postfix.size();
            int res = this->postfix.lastIndexOf(d, startpos);
            if (res >= 0)
                return res + this->data.size();
            startpos = -1;
        }
        return this->data.lastIndexOf(d, startpos);
    }

    int rfind(const std::string &d, int startpos = -1) const
    {
        return rfind(d.c_str(), startpos);
    }

    bool endsWith(const char *s) const
    {
        if (!s)
            return false;
        if (this->postfix.size())
            return this->postfix.endsWith(s);
        return this->data.endsWith(s);
    }

    bool endsWith(const std::string &s) const
    {
        return endsWith(s.c_str());
    }

    bool startsWith(const QByteArray & s, int offset = 0) const
    {
        if (s.size() > size() - offset)
            return false;
        if (offset || (this->data.size() && this->data.size() < s.size()))
            return toRawBytes(offset, s.size()) == s;
        if (this->data.size())
            return this->data.startsWith(s);
        return this->postfix.startsWith(s);
    }

    bool startsWith(const char *s, int offset = 0) const
    {
        if (!s)
            return false;
        return startsWith(QByteArray::fromRawData(s, qstrlen(s)), offset);
    }

    bool startsWith(const std::string &s, int offset = 0) const
    {
        return startsWith(QByteArray::fromRawData(s.c_str(), s.size()), offset);
    }

    std::size_t hash() const
    {
#if QT_VERSION  >= 0x050000
        return qHash(data, qHash(postfix));
#else
        return qHash(data) ^ qHash(postfix);
#endif
    }

private:
	QByteArray data;
	QByteArray postfix;
    bool raw;
};



} //namespace Data


#endif