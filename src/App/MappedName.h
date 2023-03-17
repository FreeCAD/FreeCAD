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


#ifndef APP_MAPPED_NAME_H
#define APP_MAPPED_NAME_H


#include <string>

#include <boost/algorithm/string/predicate.hpp>

#include <QByteArray>
#include <QHash>

#include "ComplexGeoData.h"
#include "IndexedName.h"


namespace Data
{

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

class AppExport MappedName
{
public:
    /// Create a MappedName from a C string, optionally prefixed by an element map prefix, which
    /// will be omitted from the stored MappedName.
    ///
    /// \param name The new name. A deep copy is made.
    /// \param size Optional, the length of the name string. If not provided, the string must be
    /// null-terminated.
    explicit MappedName(const char* name, int size = -1)
        : raw(false)
    {
        if (!name) {
            return;
        }
        if (boost::starts_with(name, ComplexGeoData::elementMapPrefix())) {
            name += ComplexGeoData::elementMapPrefix().size();
        }

        data = size < 0 ? QByteArray(name) : QByteArray(name, size);
    }

    /// Create a MappedName from a C++ std::string, optionally prefixed by an element map prefix,
    /// which will be omitted from the stored MappedName.
    ///
    /// \param name The new name. A deep copy is made.
    explicit MappedName(const std::string& nameString)
        : raw(false)
    {
        auto size = nameString.size();
        const char* name = nameString.c_str();
        if (boost::starts_with(nameString, ComplexGeoData::elementMapPrefix())) {
            name += ComplexGeoData::elementMapPrefix().size();
            size -= ComplexGeoData::elementMapPrefix().size();
        }
        data = QByteArray(name, static_cast<int>(size));
    }

    /// Create a MappedName from an IndexedName. If non-zero, the numerical part of the IndexedName
    /// is appended as text to the MappedName. In that case the memory is *not* shared between the
    /// original IndexedName and the MappedName.
    explicit MappedName(const IndexedName& element)
        : data(element.getType()),
          raw(false)
    {
        if (element.getIndex() > 0) {
            data += QByteArray::number(element.getIndex());
        }
    }

    MappedName()
        : raw(false)
    {}

    MappedName(const MappedName& other) = default;

    // FIXME if you pass a raw MappedName into these constructors they will
    // reset raw to false and things will break. is this intended?

    /// Copy constructor with start position offset and optional size. The data is *not* reused.
    ///
    /// \param other The MappedName to copy
    /// \param startPosition an integer offset to start the copy from
    /// \param size the number of bytes to copy. If not specified
    MappedName(const MappedName& other, int startPosition, int size = -1)
        : raw(false)
    {
        append(other, startPosition, size);
    }

    MappedName(const MappedName& other, const char* postfix)
        : data(other.data + other.postfix),
          postfix(postfix),
          raw(false)
    {}

    MappedName(MappedName&& other) noexcept
        : data(std::move(other.data)),
          postfix(std::move(other.postfix)),
          raw(other.raw)
    {}

    ~MappedName() = default;

    static MappedName fromRawData(const char* name, int size = -1)
    {
        MappedName res;
        if (name) {
            res.data =
                QByteArray::fromRawData(name, size >= 0 ? size : static_cast<int>(qstrlen(name)));
            res.raw = true;
        }
        return res;
    }

    static MappedName fromRawData(const QByteArray& data)
    {
        return fromRawData(data.constData(), data.size());
    }

    static MappedName fromRawData(const MappedName& other, int startPosition, int size = -1)
    {
        if (startPosition < 0) {
            startPosition = 0;
        }

        if (startPosition >= other.size()) {
            return {};
        }

        if (startPosition >= other.data.size()) {
            return {other, startPosition, size};
        }

        MappedName res;
        res.raw = true;
        if (size < 0) {
            size = other.size() - startPosition;
        }

        if (size < other.data.size() - startPosition) {
            res.data = QByteArray::fromRawData(other.data.constData() + startPosition, size);
        }
        else {
            res.data = QByteArray::fromRawData(other.data.constData() + startPosition,
                                               other.data.size() - startPosition);
            size -= other.data.size() - startPosition;
            if (size == other.postfix.size()) {
                res.postfix = other.postfix;
            }
            else if (size != 0) {
                res.postfix.append(other.postfix.constData(), size);
            }
        }
        return res;
    }

    MappedName& operator=(const MappedName& other) = default;

    MappedName& operator=(const std::string& other)
    {
        *this = MappedName(other);
        return *this;
    }

    MappedName& operator=(const char* other)
    {
        *this = MappedName(other);
        return *this;
    }


    MappedName& operator=(MappedName&& other) noexcept
    {
        this->data = std::move(other.data);
        this->postfix = std::move(other.postfix);
        this->raw = other.raw;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& stream, const MappedName& mappedName)
    {
        stream.write(mappedName.data.constData(), mappedName.data.size());
        stream.write(mappedName.postfix.constData(), mappedName.postfix.size());
        return stream;
    }

    bool operator==(const MappedName& other) const
    {
        if (this->size() != other.size()) {
            return false;
        }

        if (this->data.size() == other.data.size()) {
            return this->data == other.data && this->postfix == other.postfix;
        }

        const auto& smaller = this->data.size() < other.data.size() ? *this : other;
        const auto& larger = this->data.size() < other.data.size() ? other : *this;

        if (!larger.data.startsWith(smaller.data)) {
            return false;
        }

        QByteArray tmp = QByteArray::fromRawData(larger.data.constData() + smaller.data.size(),
                                                 larger.data.size() - smaller.data.size());

        if (!smaller.postfix.startsWith(tmp)) {
            return false;
        }

        tmp = QByteArray::fromRawData(smaller.postfix.constData() + tmp.size(),
                                      smaller.postfix.size() - tmp.size());

        return tmp == larger.postfix;
    }

    bool operator!=(const MappedName& other) const
    {
        return !(this->operator==(other));
    }

    MappedName operator+(const MappedName& other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    MappedName operator+(const char* other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    MappedName operator+(const std::string& other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    MappedName operator+(const QByteArray& other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    MappedName& operator+=(const char* other)
    {
        if (other && (other[0] != 0)) {
            this->postfix.append(other, -1);
        }
        return *this;
    }

    MappedName& operator+=(const std::string& other)
    {
        if (!other.empty()) {
            this->postfix.reserve(this->postfix.size() + static_cast<int>(other.size()));
            this->postfix.append(other.c_str(), static_cast<int>(other.size()));
        }
        return *this;
    }

    MappedName& operator+=(const QByteArray& other)
    {
        this->postfix += other;
        return *this;
    }

    MappedName& operator+=(const MappedName& other)
    {
        append(other);
        return *this;
    }

    void append(const char* dataToAppend, int size = -1)
    {
        // FIXME raw not assigned?
        if (dataToAppend && (size != 0)) {
            if (size < 0) {
                size = static_cast<int>(qstrlen(dataToAppend));
            }
            if (empty()) {
                this->data.append(dataToAppend, size);
            }
            else {
                this->postfix.append(dataToAppend, size);
            }
        }
    }

    void append(const MappedName& other, int startPosition = 0, int size = -1)
    {
        // enforce 0 <= startPosition <= other.size
        if (startPosition < 0) {
            startPosition = 0;
        }
        else if (startPosition > other.size()) {
            return;
        }

        // enforce 0 <= size <= other.size - startPosition
        if (size < 0 || size > other.size() - startPosition) {
            size = other.size() - startPosition;
        }


        if (startPosition < other.data.size())// if starting inside data
        {
            int count = size;
            // make sure count doesn't exceed data size and end up in postfix
            if (count > other.data.size() - startPosition) {
                count = other.data.size() - startPosition;
            }

            // if this is empty append in data else append in postfix
            if (startPosition == 0 && count == other.data.size() && this->empty()) {
                this->data = other.data;
                this->raw = other.raw;
            }
            else {
                append(other.data.constData() + startPosition, count);
            }

            // setup startPosition and count to continue appending the remainder to postfix
            startPosition = 0;
            size -= count;
        }
        else// else starting inside postfix
        {
            startPosition -= other.data.size();
        }

        // if there is still data to be added to postfix
        if (size != 0) {
            if (startPosition == 0 && size == other.postfix.size()) {
                if (this->empty()) {
                    this->data = other.postfix;
                }
                else if (this->postfix.isEmpty()) {
                    this->postfix = other.postfix;
                }
                else {
                    this->postfix += other.postfix;
                }
            }
            else {
                append(other.postfix.constData() + startPosition, size);
            }
        }
    }

    std::string toString(int startPosition, int len = -1) const
    {
        std::string res;
        return toString(res, startPosition, len);
    }

    const char* toString(std::string& buffer, int startPosition = 0, int len = -1) const
    {
        std::size_t offset = buffer.size();
        int count = this->size();
        if (startPosition < 0) {
            startPosition = 0;
        }
        else if (startPosition >= count) {
            return buffer.c_str() + buffer.size();
        }
        if (len < 0 || len > count - startPosition) {
            len = count - startPosition;
        }
        buffer.reserve(buffer.size() + len);
        if (startPosition < this->data.size()) {
            count = this->data.size() - startPosition;
            if (len < count) {
                count = len;
            }
            buffer.append(this->data.constData() + startPosition, count);
            len -= count;
        }
        buffer.append(this->postfix.constData(), len);
        return buffer.c_str() + offset;
    }

    // if offset is inside data return data, if offset is > data.size
    //(ends up in postfix) return postfix
    const char* toConstString(int offset, int& size) const
    {
        if (offset < 0) {
            offset = 0;
        }
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

    QByteArray toRawBytes(int offset = 0, int size = -1) const
    {
        if (offset < 0) {
            offset = 0;
        }
        if (offset >= this->size()) {
            return {};
        }
        if (size < 0 || size > this->size() - offset) {
            size = this->size() - offset;
        }
        if (offset >= this->data.size()) {
            offset -= this->data.size();
            return QByteArray::fromRawData(this->postfix.constData() + offset, size);
        }
        if (size <= this->data.size() - offset) {
            return QByteArray::fromRawData(this->data.constData() + offset, size);
        }

        QByteArray res(this->data.constData() + offset, this->data.size() - offset);
        res.append(this->postfix.constData(), size - this->data.size() + offset);
        return res;
    }

    const QByteArray& dataBytes() const
    {
        return this->data;
    }

    const QByteArray& postfixBytes() const
    {
        return this->postfix;
    }

    const char* constPostfix() const
    {
        return this->postfix.constData();
    }

    // No constData() because 'data' is allowed to contain raw data, which may not end with 0.

    QByteArray toBytes() const
    {
        if (this->postfix.isEmpty()) {
            return this->data;
        }
        if (this->data.isEmpty()) {
            return this->postfix;
        }
        return this->data + this->postfix;
    }

    IndexedName toIndexedName() const
    {
        if (this->postfix.isEmpty()) {
            return IndexedName(this->data);
        }
        return IndexedName();
    }

    std::string toPrefixedString() const
    {
        std::string res;
        toPrefixedString(res);
        return res;
    }

    const char* toPrefixedString(std::string& buf) const
    {
        if (!toIndexedName()) {
            buf += ComplexGeoData::elementMapPrefix();
        }
        toString(buf);
        return buf.c_str();
    }

    int compare(const MappedName& other) const
    {
        int thisSize = this->size();
        int otherSize = other.size();
        for (int i = 0, count = std::min(thisSize, otherSize); i < count; ++i) {
            char thisChar = this->operator[](i);
            char otherChar = other[i];
            if (thisChar < otherChar) {
                return -1;
            }
            if (thisChar > otherChar) {
                return 1;
            }
        }
        if (thisSize < otherSize) {
            return -1;
        }
        if (thisSize > otherSize) {
            return 1;
        }
        return 0;
    }

    bool operator<(const MappedName& other) const
    {
        return compare(other) < 0;
    }

    char operator[](int index) const
    {
        // FIXME overflow underflow checks?
        if (index >= this->data.size()) {
            return this->postfix[index - this->data.size()];
        }
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
        if (!this->raw) {
            return *this;
        }
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

    int find(const char* searchTarget, int startPosition = 0) const
    {
        if (!searchTarget) {
            return -1;
        }
        if (startPosition < 0) {
            startPosition = 0;
        }
        if (startPosition < this->data.size()) {
            int res = this->data.indexOf(searchTarget, startPosition);
            if (res >= 0) {
                return res;
            }
            startPosition = 0;
        }
        else {
            startPosition -= this->data.size();
        }
        int res = this->postfix.indexOf(searchTarget, startPosition);
        if (res < 0) {
            return res;
        }
        return res + this->data.size();
    }

    int find(const std::string& searchTarget, int startPosition = 0) const
    {
        return find(searchTarget.c_str(), startPosition);
    }

    int rfind(const char* searchTarget, int startPosition = -1) const
    {
        if (!searchTarget) {
            return -1;
        }
        if (startPosition < 0
            || startPosition > this->postfix.size()) {// FIXME should be this->data.size
            if (startPosition > postfix.size()) {
                startPosition -= postfix.size();
            }
            int res = this->postfix.lastIndexOf(searchTarget, startPosition);
            if (res >= 0) {
                return res + this->data.size();
            }
            startPosition = -1;
        }
        return this->data.lastIndexOf(searchTarget, startPosition);
    }

    int rfind(const std::string& searchTarget, int startPosition = -1) const
    {
        return rfind(searchTarget.c_str(), startPosition);
    }

    bool endsWith(const char* searchTarget) const
    {
        if (!searchTarget) {
            return false;
        }
        if (this->postfix.size() != 0) {
            return this->postfix.endsWith(searchTarget);
        }
        return this->data.endsWith(searchTarget);
    }

    bool endsWith(const std::string& searchTarget) const
    {
        return endsWith(searchTarget.c_str());
    }

    bool startsWith(const QByteArray& searchTarget, int offset = 0) const
    {
        if (searchTarget.size() > size() - offset) {
            return false;
        }
        if ((offset != 0)
            || ((this->data.size() != 0) && this->data.size() < searchTarget.size())) {
            return toRawBytes(offset, searchTarget.size()) == searchTarget;
        }
        if (this->data.size() != 0) {
            return this->data.startsWith(searchTarget);
        }
        return this->postfix.startsWith(searchTarget);
    }

    bool startsWith(const char* searchTarget, int offset = 0) const
    {
        if (!searchTarget) {
            return false;
        }
        return startsWith(
            QByteArray::fromRawData(searchTarget, static_cast<int>(qstrlen(searchTarget))), offset);
    }

    bool startsWith(const std::string& searchTarget, int offset = 0) const
    {
        return startsWith(
            QByteArray::fromRawData(searchTarget.c_str(), static_cast<int>(searchTarget.size())),
            offset);
    }

    std::size_t hash() const
    {
        return qHash(data, qHash(postfix));
    }

private:
    QByteArray data;
    QByteArray postfix;
    bool raw;
};

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)


}// namespace Data


#endif// APP_MAPPED_NAME_H