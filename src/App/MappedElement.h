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


#ifndef _AppMappedElement_h_
#define _AppMappedElement_h_

#include <cstring>
#include <memory>
#include <cctype>
#include <boost/algorithm/string/predicate.hpp>
#include <QByteArray>
#include <QHash>
#include "ComplexGeoData.h"
#include "StringHasher.h"

namespace App
{
class DocumentObject;
}

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

class AppExport MappedName
{
public:
    MappedName()
        :raw(false)
    {}

#if QT_VERSION  >= 0x050200
    MappedName(MappedName &&other)
        :data(std::move(other.data))
        ,postfix(std::move(other.postfix))
        ,raw(other.raw)
    {}

    MappedName & operator=(MappedName &&other)
    {
        this->data = std::move(other.data);
        this->postfix = std::move(other.postfix);
        this->raw  = other.raw;
        return *this;
    }
#endif

	explicit MappedName(const char * name, int size = -1)
		:raw(false)
	{
        if (!name) return;
        if (boost::starts_with(name, ComplexGeoData::elementMapPrefix()))
            name += ComplexGeoData::elementMapPrefix().size();
        if (size < 0)
            data = QByteArray(name);
        else
            data = QByteArray(name, size);
    }

	explicit MappedName(const std::string & name)
    {
        int size = name.size();
        const char *n = name.c_str();
        if (boost::starts_with(name, ComplexGeoData::elementMapPrefix())) {
            n += ComplexGeoData::elementMapPrefix().size();
            size -= ComplexGeoData::elementMapPrefix().size();
        }
        data = QByteArray(n, size);
    }

    explicit MappedName(const IndexedName & element)
        :data(element.getType()), raw(false)
    {
        if (element.getIndex() > 0)
            data += QByteArray::number(element.getIndex());
    }

    explicit MappedName(const App::StringIDRef & sid)
        :raw(false)
    {
        sid.toBytes(this->data);
    }

	MappedName(const MappedName & other)
		:data(other.data), postfix(other.postfix), raw(other.raw)
	{}

	MappedName(const MappedName & other, int from, int size = -1)
        : raw(false)
    {
        append(other, from, size);
    }

	MappedName(const MappedName & other, const char *postfix)
		:data(other.data + other.postfix)
        ,postfix(postfix)
        ,raw(false)
	{}

    static MappedName fromRawData(const char * name, int size = -1)
    {
        MappedName res;
        if (name) {
            res.data = QByteArray::fromRawData(name,
                    size>=0 ? size: qstrlen(name));
            res.raw = true;
        }
        return res;
    }

    static MappedName fromRawData(const QByteArray & data)
    {
        return fromRawData(data.constData(), data.size());
    }

    static MappedName fromRawData(const MappedName &other, int from, int size = -1)
    {
        if (from < 0)
            from = 0;

        if (from >= other.size())
            return MappedName();

        if (from >= other.data.size())
            return MappedName(other, from, size);

        MappedName res;
        res.raw = true;
        if (size < 0)
            size = other.size() - from;

        if (size < other.data.size()-from)
            res.data = QByteArray::fromRawData(
                    other.data.constData()+from, size);
        else {
            res.data = QByteArray::fromRawData(
                    other.data.constData()+from, other.data.size()-from);
            size -= other.data.size() - from;
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

    friend std::ostream & operator<<(std::ostream & s, const MappedName & n)
    {
        s.write(n.data.constData(), n.data.size());
        s.write(n.postfix.constData(), n.postfix.size());
        return s;
    }

    bool operator==(const MappedName & other) const
    {
        if (this->size() != other.size())
            return false;
        if (this->data.size() == other.data.size())
            return this->data == other.data && this->postfix == other.postfix;
        const auto &a = this->data.size() < other.data.size() ? *this : other;
        const auto &b = this->data.size() < other.data.size() ? other: *this;
        if (!b.data.startsWith(a.data))
            return false;
        QByteArray tmp = QByteArray::fromRawData(
                b.data.constData() + a.data.size(),
                b.data.size() - a.data.size());
        if (!a.postfix.startsWith(tmp))
            return false;
        tmp = QByteArray::fromRawData(
                a.postfix.constData() + tmp.size(),
                a.postfix.size() - tmp.size());
        return tmp == b.postfix;
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

    void append(const MappedName & other, int from = 0, int size = -1)
    {
        if (from < 0)
            from = 0;
        else if (from > other.size())
            return;
        if (size < 0 || size + from > other.size())
            size = other.size() - from;

        int count = size;
        if (from < other.data.size()) {
            if (count > other.data.size() - from)
                count = other.data.size() - from;
            if (from == 0 && count == other.data.size() && this->empty()) {
                this->data = other.data;
                this->raw = other.raw;
            } else
                append(other.data.constData() + from, count);
            from = 0;
            size -= count;
        } else
            from -= other.data.size();
        if (size) {
            if (from == 0 && size == other.postfix.size()) {
                if (this->empty())
                    this->data = other.postfix;
                else if (this->postfix.isEmpty())
                    this->postfix = other.postfix;
                else
                    this->postfix += other.postfix;
            } else
                append(other.postfix.constData() + from, size);
        }
    }

    std::string toString(int from, int len=-1) const
    {
        std::string res;
        return toString(res, from, len);
    }

    const char * toString(std::string &s, int from=0, int len=-1) const
    {
        std::size_t offset = s.size();
        int count = this->size();
        if (from < 0)
            from = 0;
        else if (from >= count)
            return s.c_str()+s.size();
        if (len < 0 || len > count - from)
            len = count - from;
        s.reserve(s.size() + len);
        if (from < this->data.size()) {
            count = this->data.size() - from;
            if (len < count)
                count = len;
            s.append(this->data.constData()+from, count);
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

    int find(const char *d, int from = 0) const
    {
        if (!d)
            return -1;
        if (from < 0)
            from = 0;
        if (from < this->data.size()) {
            int res = this->data.indexOf(d, from);
            if (res >= 0)
                return res;
            from = 0;
        } else
            from -= this->data.size();
        int res = this->postfix.indexOf(d, from);
        if (res < 0)
            return res;
        return res + this->data.size();
    }

    int find(const std::string &d, int from = 0) const
    {
        return find(d.c_str(), from);
    }

    int rfind(const char *d, int from = -1) const
    {
        if (!d)
            return -1;
        if (from < 0 || from > this->postfix.size()) {
            if (from > postfix.size())
                from -= postfix.size();
            int res = this->postfix.lastIndexOf(d, from);
            if (res >= 0)
                return res + this->data.size();
            from = -1;
        }
        return this->data.lastIndexOf(d, from);
    }

    int rfind(const std::string &d, int from = -1) const
    {
        return rfind(d.c_str(), from);
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

struct AppExport MappedElement
{
	IndexedName index;
	MappedName name;

    MappedElement()
    {}

	MappedElement(const IndexedName & idx, const MappedName & n)
		: index(idx), name(n)
	{}

	MappedElement(const MappedName & n, const IndexedName & idx)
		: index(idx), name(n)
	{}

    MappedElement(const MappedElement & other)
        : index(other.index), name(other.name)
    {}

    MappedElement(MappedElement && other)
        : index(std::move(other.index)), name(std::move(other.name))
    {}

    MappedElement & operator=(MappedElement && other)
    {
        this->index = std::move(other.index);
        this->name = std::move(other.name);
        return *this;
    }

    MappedElement & operator=(const MappedElement & other)
    {
        this->index = other.index;
        this->name = other.name;
        return *this;
    }

    bool operator==(const MappedElement &other) const
    {
        return this->index == other.index && this->name == other.name;
    }

    bool operator!=(const MappedElement &other) const
    {
        return this->index != other.index || this->name != other.name;
    }

    bool operator<(const MappedElement &other) const
    {
        int res = this->index.compare(other.index);
        if (res < 0)
            return true;
        if (res > 0)
            return false;
        return this->name < other.name;
    }
};

struct AppExport HistoryItem {
    App::DocumentObject *obj;
    long tag;
    Data::MappedName element;
    Data::IndexedName index;
    std::vector<Data::MappedName> intermediates;
    HistoryItem(App::DocumentObject *obj, const Data::MappedName &name);
};

struct AppExport ElementNameComp {
    /** Comparison function to make topo name more stable
     *
     * The sorting decompose the name into either of the following two forms
     *      '#' + hex_digits + tail
     *      non_digits + digits + tail
     *
     * The non-digits part is compared lexically, while the digits part is
     * compared by its integer value.
     *
     * The reason for this is to prevent name with bigger digits (usually means
     * comes late in history) comes early when sorting.
     */
    bool operator()(const MappedName &a, const MappedName &b) const;
};

typedef QVector<App::StringIDRef> ElementIDRefs;

struct AppExport MappedChildElements
{
    IndexedName indexedName;
    int count;
    int offset;
    long tag;
    ElementMapPtr elementMap;
    QByteArray postfix;
    ElementIDRefs sids;

    static const std::string & prefix();
};

} //namespace Data


#endif
