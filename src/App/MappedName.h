// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *   Copyright (c) 2023 FreeCAD Project Association                         *
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

#pragma once

#include <memory>
#include <string>

#include <QByteArray>
#include <QHash>
#include <QVector>
#include <utility>

#include "ElementNamingUtils.h"
#include "IndexedName.h"
#include "StringHasher.h"


namespace Data
{

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

/**
 * @brief A class for managing element map names.
 * @ingroup ElementMapping
 *
 * The MappedName class maintains a two-part name: the first part ("data") is
 * considered immutable once created, while the second part ("postfix") can be
 * modified/appended to by later operations.  It uses shared data when possible
 * (see the fromRawData() members). Despite storing data and postfix
 * separately, they can be accessed via calls to size(), operator[], etc. as
 * though they were a single array.
 */
class AppExport MappedName
{
public:
    /**
     * @brief Create a MappedName from a C string.
     *
     * Create a MappedName from a C string, optionally prefixed by an element
     * map prefix, which will be omitted from the stored MappedName.
     *
     * @param[in] name The new name. A deep copy is made.
     * @param[in] size Optional, the length of the name string. If not
     * provided, the string must be null-terminated.
     */
    explicit MappedName(const char* name, int size = -1);

    /**
     * @brief Create a MappedName from a C++ std::string.
     *
     * Create a MappedName from a C++ std::string, optionally prefixed by an
     * element map prefix, which will be omitted from the stored MappedName.
     *
     * @param nameString The new name. A deep copy is made.
     */
    explicit MappedName(const std::string& nameString);

    /**
     * @brief Create a MappedName from an IndexedName.
     *
     * Create a MappedName from an IndexedName. If non-zero, the numerical part
     * of the IndexedName is appended as text to the MappedName. In that case
     * the memory is *not* shared between the original IndexedName and the
     * MappedName.
     */
    explicit MappedName(const IndexedName& element)
        : data(QByteArray::fromRawData(element.getType(),
                                       static_cast<int>(qstrlen(element.getType()))))
        , raw(true)
    {
        if (element.getIndex() > 0) {
            this->data += QByteArray::number(element.getIndex());
            this->raw = false;
        }
    }


    /// Create a MappedName from a StringIdRef.
    explicit MappedName(const App::StringIDRef& sid)
        : raw(false)
    {
        sid.toBytes(this->data);
    }

    MappedName()
        : raw(false)
    {}

    MappedName(const MappedName& other) = default;

    /**
     * @brief Copy constructor with start position offset and size.
     *
     * Copy constructor with start position offset and optional size. The data
     * is *not* reused.
     *
     * @param[in] other The MappedName to copy.
     * @param[in] startPosition An integer offset to start the copy from.
     * @param[in] size The number of bytes to copy.
     *
     * @see append() for details about how the copy behaves for various sizes
     * and start positions
     */
    MappedName(const MappedName& other, int startPosition, int size = -1)
        : raw(false)
    {
        append(other, startPosition, size);
    }

    /**
     * @brief Copy constructor with additional postfix
     *
     * @param[in] other The mapped name to copy. Its data and postfix become
     * the new MappedName's data.
     *
     * @param postfix The postfix for the new MappedName.
     */
    MappedName(const MappedName& other, const char* postfix)
        : data(other.data + other.postfix)
        , postfix(postfix)
        , raw(false)
    {}

    MappedName(MappedName&& other) noexcept
        : data(std::move(other.data))
        , postfix(std::move(other.postfix))
        , raw(other.raw)
    {}

    ~MappedName() = default;

    /**
     * @brief Construct a MappedName from raw character data.
     *
     * Construct a MappedName from raw character data (including null characters, if size is
     * provided). No copy is made: the data is used in place.
     *
     * @param name The raw data to use.
     * @param size The number of bytes to access. If omitted, name must be null-terminated.
     *
     * @return a new MappedName with name as its data.
     */
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

    /**
     * @brief Construct a MappedName from QByteArray data.
     *
     * Construct a MappedName from QByteArray data (including any embedded null characters).
     *
     * @param[in] data The original data. No copy is made, the data is shared
     * with the other instance.
     *
     * @return a new MappedName with data as its data.
     */
    static MappedName fromRawData(const QByteArray& data)
    {
        return fromRawData(data.constData(), data.size());
    }

    /**
     * @brief Construct a MappedName from another MappedName.
     *
     * @param[in] other The MappedName to copy from. The data is usually not
     * copied, but in some cases a partial copy may be made to support a slice
     * that extends across other's data into its postfix.
     * @param[in] startPosition The position to start the reference at.
     * @param[in] size The number of bytes to access. If omitted, continues
     * from startPosition to the end of available data (including postfix).
     *
     * @return a new MappedName sharing (possibly a subset of) data with other.
     *
     * @see append() For details about how the copy behaves for various sizes
     * and start positions.
     */
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

    /// Share data with another MappedName.
    MappedName& operator=(const MappedName& other) = default;

    /// Create a new MappedName from a std::string: the string's data is copied.
    MappedName& operator=(const std::string& other)
    {
        *this = MappedName(other);
        return *this;
    }

    /// Create a new MappedName from a const char *. The character data is copied.
    MappedName& operator=(const char* other)
    {
        *this = MappedName(other);
        return *this;
    }

    /// Move-construct a MappedName.
    MappedName& operator=(MappedName&& other) noexcept
    {
        this->data = std::move(other.data);
        this->postfix = std::move(other.postfix);
        this->raw = other.raw;
        return *this;
    }

    /**
     * @brief Write to a stream as the name with postfix directly appended to it.
     *
     * Note that there is no special handling for null or non-ASCII characters,
     * they are simply written to the stream.
     */
    friend std::ostream& operator<<(std::ostream& stream, const MappedName& mappedName)
    {
        stream.write(mappedName.data.constData(), mappedName.data.size());
        stream.write(mappedName.postfix.constData(), mappedName.postfix.size());
        return stream;
    }

    /**
     * @brief Two MappedNames are equal if the concatenation of their data and postfix is equal.
     *
     * The individual data and postfix may NOT be equal in this case.
     */
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

    /// Check if two mapped names are inequal.
    bool operator!=(const MappedName& other) const
    {
        return !(this->operator==(other));
    }

    /**
     * @brief Concatenate two mapped names.
     *
     * @param[in] other The mapped name to append.
     *
     * @return A new MappedName whose data is the LHS argument's data and whose
     * postfix is the LHS argument's postfix with the RHS argument's data and
     * postfix appended to it.
     */
    MappedName operator+(const MappedName& other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    /**
     * @brief Concatenate two mapped names.
     *
     * @param[in] other The mapped name as a string to append.
     *
     * @return A new MappedName whose data is the LHS argument's data and whose
     * postfix is the LHS argument's postfix with the RHS argument appended to
     * it. The character data is copied.
     */
    MappedName operator+(const char* other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    /// @copydoc operator+(const char*) const
    MappedName operator+(const std::string& other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    /**
     * @brief Concatenate two mapped names.
     *
     * @param[in] other The mapped name as a QByteArray.
     *
     * @return A new MappedName whose data is the LHS argument's data and whose
     * postfix is the LHS argument's postfix with the RHS argument appended to
     * it.
     */
    MappedName operator+(const QByteArray& other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    /**
     * @brief Appends other to this instance's postfix.
     *
     * @p other must be a null-terminated C string. The character data from the
     * string is copied.
     *
     * @param[in] other The mapped name as a string.
     *
     * @return This with the other's postfix appended.
     */
    MappedName& operator+=(const char* other)
    {
        if (other && (other[0] != 0)) {
            this->postfix.append(other, static_cast<int>(qstrlen(other)));
        }
        return *this;
    }

    /// @copydoc operator+=(const char* other)
    MappedName& operator+=(const std::string& other)
    {
        if (!other.empty()) {
            this->postfix.reserve(this->postfix.size() + static_cast<int>(other.size()));
            this->postfix.append(other.c_str(), static_cast<int>(other.size()));
        }
        return *this;
    }

    /**
     * @brief Appends other to this instance's postfix.
     *
     * The data may be either copied or shared, depending on whether
     * this->postfix is empty (in which case the data is shared) or non-empty
     * (in which case it is copied).
     *
     * @param[in] other The mapped name as a QByteArray.
     * @return This with the other's data appended.
     */
    MappedName& operator+=(const QByteArray& other)
    {
        this->postfix += other;
        return *this;
    }

    /**
     * @brief Appends other to this instance's postfix.
     *
     * Appends other to this instance's postfix, unless this is empty, in which
     * case this acts like operator=, and makes this instance's data equal to
     * other's data, and this instance's postfix equal to the other instance's
     * postfix.
     *
     * @param[in] other The mapped name to append.
     * @return This with the other's data appended.
     */
    MappedName& operator+=(const MappedName& other)
    {
        append(other);
        return *this;
    }

    /**
     * @brief Append to this mapped name.
     *
     * Add @p dataToAppend to this mapped name. If the current name is empty,
     * this becomes the new data element. If this MappedName already has data,
     * then the data is appended to the postfix.
     *
     * @param[in] dataToAppend The data to add. A deep copy is made.
     * @param[in] size The number of bytes to copy. If omitted, @p dataToAppend
     * must be null-terminated.
     */
    void append(const char* dataToAppend, int size = -1)
    {
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

    /**
     * @brief Append to this mapped name.
     *
     * Treating both this and other as single continuous byte arrays, append
     * other to this. If this is empty, then other's data is shared with this
     * instance's data beginning at @p startPosition.  If this is *not* empty,
     * then all data is appended to the postfix. If the copy crosses the
     * boundary between other's data and its postfix, then if this instance was
     * empty, the new data stops where other's data stops, and the remainder of
     * the copy is placed in the suffix.  Otherwise the copy simply continues
     * as though there was no distinction between other's data and suffix.
     *
     * @param[in] other The MappedName to obtain the data from. The data is
     * shared when possible, depending on the details of startPosition, size,
     * and this->empty().
     *
     * @param startPosition The byte to start the copy at. Must be a positive
     * non-zero integer less than the length of other's combined data +
     * postfix.
     *
     * @param size The number of bytes to copy. Must not overrun the end of
     * other's combined data storage when taking @p startPosition into
     * consideration.
     */
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


        if (startPosition < other.data.size())  // if starting inside data
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
        else  // else starting inside postfix
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

    /**
     * @brief Create a string representation.
     *
     * Create a std::string from this instance, starting at startPosition, and
     * extending len bytes.
     *
     * @param[in] startPosition The offset into the data.
     * @param[in] len The number of bytes to output.
     *
     * @return A new std::string containing the bytes copied from this
     * instance's data and postfix (depending on startPosition and len).
     *
     * @note No effort is made to ensure that these are valid ASCII characters,
     * and it is possible the data includes embedded null characters, non-ASCII
     * data, etc.
     */
    std::string toString(int startPosition = 0, int len = -1) const
    {
        std::string res;
        return appendToBuffer(res, startPosition, len);
    }

    /**
     * @brief Append this mapped name to a buffer.
     *
     * Given a (possibly non-empty) std::string buffer, append this instance to it, starting at a
     * specified position, and continuing for a specified number of bytes.
     *
     * @param[in,out] buffer The string buffer to append to.
     *
     * @param[in] startPosition The position in this instance's data/postfix to
     * start at (defaults to * zero). Must be less than the total length of the
     * data plus the postfix.
     *
     * @param[in] len The number of bytes to append. If omitted, defaults to
     * appending all available data starting at startPosition.
     *
     * @return A pointer to the beginning of the appended data within buffer.
     *
     * @note No effort is made to ensure that these are valid ASCII characters,
     * and it is possible the data includes embedded null characters, non-ASCII
     * data, etc.
     */
    const char* appendToBuffer(std::string& buffer, int startPosition = 0, int len = -1) const
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

    /**
     * @brief Convert this mapped name to raw bytes.
     *
     * Get access to raw byte data. When possible, data is shared between this
     * instance and the returned QByteArray. If the combination of offset and
     * size results in data that crosses the boundary between this->data and
     * this->postfix, the data must be copied in order to provide access as a
     * continuous array of bytes.
     *
     * @param[in] offset The start position of the raw data access.
     *
     * @param[in] size The number of bytes to access. If omitted, the resulting
     * QByteArray includes everything starting from offset to the end,
     * including any postfix data.
     *
     * @return A new QByteArray that shares data with this instance if
     * possible, or is a new copy if required by offset and size.
     */
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

    /// Direct access to the stored QByteArray of data. A copy is never made.
    const QByteArray& dataBytes() const
    {
        return this->data;
    }

    /// Direct access to the stored QByteArray of postfix. A copy is never made.
    const QByteArray& postfixBytes() const
    {
        return this->postfix;
    }

    /// Convenience function providing access to the pointer to the beginning of the postfix data.
    const char* constPostfix() const
    {
        return this->postfix.constData();
    }

    // No constData() because 'data' is allowed to contain raw data, which may not end with 0.

    /**
     * @brief Provide access to the content of this instance.
     *
     * If either postfix or data is empty, no copy is made and the original
     * QByteArray is returned, sharing data with this instance. If this
     * instance contains both data and postfix, a new QByteArray is created and
     * stores a copy of the data and postfix concatenated together.
     *
     * @return Either a new or the current QByteArray.
     */
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

    /**
     * @brief Create an IndexedName from the mapped name.
     *
     * Create an IndexedName from the data portion of this MappedName. If this
     * data has a postfix, the function returns an empty IndexedName. The
     * function will fail if this->data contains anything other than the ASCII
     * letter a-z, A-Z, and the underscore, with an optional integer suffix,
     * returning an empty IndexedName (e.g. an IndexedName that evaluates to
     * boolean false and isNull() == true).
     *
     * @return a new IndexedName that shares its data with this instance's data
     * member.
     */
    IndexedName toIndexedName() const
    {
        if (this->postfix.isEmpty()) {
            return IndexedName(this->data);
        }
        return IndexedName();
    }

    /**
     * @brief Create a prefixed string from the mapped name.
     *
     * Create and return a string version of this MappedName prefixed by the
     * ComplexGeoData element map prefix, if this MappedName cannot be
     * converted to an indexed name.
     */
    std::string toPrefixedString() const
    {
        std::string res;
        appendToBufferWithPrefix(res);
        return res;
    }

    /**
     * @brief Append this mapped name to a string buffer.
     *
     * Append this MappedName to a provided string buffer, including the
     * ComplexGeoData element map prefix if the MappedName cannot be converted
     * to an IndexedName.
     *
     * @param[in,out] buf A (possibly non-empty) string to append this
     * MappedName to.
     *
     * @return A pointer to the beginning of the buffer.
     */
    const char* appendToBufferWithPrefix(std::string& buf) const
    {
        if (!toIndexedName()) {
            buf += ELEMENT_MAP_PREFIX;
        }
        appendToBuffer(buf);
        return buf.c_str();
    }

    /**
     * @brief Compare two mapped names.
     *
     * Equivalent to C++20 operator<=>. Performs byte-by-byte comparison of
     * this and other, starting at the first byte and continuing through both
     * data and postfix, ignoring which is which. If the combined data and
     * postfix members are of unequal size but start with the same data, the
     * shorter array is considered "less than" the longer.
     *
     * @param[in] other The mapped name to compare.
     *
     * @return < 0 if this is less than other, 0 if they are equal and > 0 if
     * this is greater than other.
     */
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

    /// Check if this mapped name is less than @p other.
    bool operator<(const MappedName& other) const
    {
        return compare(other) < 0;
    }

    /**
     * @brief Index into the mapped name.
     *
     * Treat this MappedName as a single continuous array of bytes, beginning
     * with data and continuing through postfix.
     *
     * @param index The byte offset to access.
     *
     * @return The byte at the specified offset, or 0 if the offset is out of
     * range.
     */
    char operator[](int index) const
    {
        if (index < 0) {
            index = 0;
        }
        if (index >= this->data.size()) {
            if (index - this->data.size() > this->postfix.size() - 1) {
                index = this->postfix.size() - 1;
            }
            return this->postfix[index - this->data.size()];
        }
        return this->data[index];
    }

    /**
     * @brief Get the combined size of data and postfix.
     *
     * Treat this MappedName as a single continuous array of bytes, returning the combined size
     * of the data and postfix.
     *
     * @return The total number of bytes in data + postfix.
     */
    int size() const
    {
        return this->data.size() + this->postfix.size();
    }

    /**
     * @brief Check if the mapped name is empty.
     *
     * Treat this MappedName as a single continuous array of bytes, returning true only if both
     * data and prefix are empty.
     *
     * @return true if there are no bytes in data or postfix.
     */
    bool empty() const
    {
        return this->data.isEmpty() && this->postfix.isEmpty();
    }

    /**
     * @brief Check whether this mapped name is shared data.
     *
     * It is safe to access data only if it has been copied prior. To force a copy
     * please \see compact().
     *
     * @return True if this is shared data, or false if a unique copy has been made.
     */
    bool isRaw() const
    {
        return this->raw;
    }

    /**
     * @brief Copy the mapped name.
     *
     * If this is shared data, a new unshared copy is made and returned. If it
     * is already unshared no new copy is made, a new instance is returned that
     * shares is data with the current instance.
     */
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

    /// Ensure that this data is unshared, making a copy if necessary.
    void compact() const;

    /**
     * @brief Boolean conversion is the inverse of empty().
     *
     * @return True if there is data in either the data or postfix, and false
     * if there is nothing in either.
     */
    explicit operator bool() const
    {
        return !empty();
    }

    /**
     * @brief Reset this instance.
     *
     * Clear anything in data and postfix.
     */
    void clear()
    {
        this->data.clear();
        this->postfix.clear();
        this->raw = false;
    }

    /**
     * @brief Find a string of characters in this mapped name.
     *
     * Find a string of characters in this mapped name. The bytes must occur
     * either entirely in the data, or entirely in the postfix: a string that
     * overlaps the two will not be found.
     *
     * @param[in] searchTarget A null-terminated C string to search for.
     * @param[in] startPosition A byte offset to start the search at.
     *
     * @return The position of the target in this instance, or -1 if the target
     * is not found.
     */
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

    /// @copydoc find(const char*,int) const
    int find(const std::string& searchTarget, int startPosition = 0) const
    {
        return find(searchTarget.c_str(), startPosition);
    }

    /**
     * @brief Reverse find a string of characters in this mapped name.
     *
     * Find a string of characters in this mapped name, starting at the back of
     * postfix and proceeding in reverse through the data. The bytes must occur
     * either entirely in the data, or entirely in the postfix: a string that
     * overlaps the two will not be found.
     *
     * @param[in] searchTarget A null-terminated C string to search for.
     *
     * @param[in] startPosition A byte offset to start the search at. Negative
     * numbers are supported and count back from the end of the concatenated
     * data (as in QByteArray::lastIndexOf()).
     *
     * @return The position of the target in this instance, or -1 if the target
     * is not found.
     */
    int rfind(const char* searchTarget, int startPosition = -1) const
    {
        if (!searchTarget) {
            return -1;
        }
        if (startPosition < 0 || startPosition >= this->data.size()) {
            if (startPosition >= data.size()) {
                startPosition -= data.size();
            }
            int res = this->postfix.lastIndexOf(searchTarget, startPosition);
            if (res >= 0) {
                return res + this->data.size();
            }
            startPosition = -1;
        }
        return this->data.lastIndexOf(searchTarget, startPosition);
    }

    /// @copydoc rfind(const char*,int) const
    int rfind(const std::string& searchTarget, int startPosition = -1) const
    {
        return rfind(searchTarget.c_str(), startPosition);
    }

    /**
     * @brief Check if this mapped name ends with the search target.
     *
     * If there is a postfix, only the postfix is considered. If not, then only
     * the data is considered. A search string that overlaps the two will not
     * be found.
     *
     * @param[in] searchTarget The string to search for at the end of this mapped name.
     *
     * @return true if this MappedName ends with the search target. 
     */
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

    /// @copydoc endsWith(const char*) const
    bool endsWith(const std::string& searchTarget) const
    {
        return endsWith(searchTarget.c_str());
    }

    /**
     * @brief Check if this mapped name starts with the search target.
     *
     * If there is a postfix, only the postfix is considered. If not, then only
     * the data is considered. A search string that overlaps the two will not
     * be found.
     *
     * @param[in] searchTarget The search target to match.
     * @param[in] offset An offset to perform the match at.
     *
     * @return True if this MappedName begins with the target bytes.
     */
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

    /// @copydoc startsWith(const QByteArray&,int) const
    bool startsWith(const char* searchTarget, int offset = 0) const
    {
        if (!searchTarget) {
            return false;
        }
        return startsWith(
            QByteArray::fromRawData(searchTarget, static_cast<int>(qstrlen(searchTarget))),
            offset);
    }

    /// @copydoc startsWith(const QByteArray&,int) const
    bool startsWith(const std::string& searchTarget, int offset = 0) const
    {
        return startsWith(
            QByteArray::fromRawData(searchTarget.c_str(), static_cast<int>(searchTarget.size())),
            offset);
    }

    /**
     * @brief Extract information from an encoded element name.
     *
     * Extract tagOut and other information from an encoded element name.
     *
     * @param[out] tagOut: optional pointer to receive the extracted tagOut
     * @param[out] lenOut: optional pointer to receive the length field after
     * the tagOut field.  This gives the length of the previous hashed element
     * name starting from the beginning of the give element name.
     * @param[out] postfixOut: optional pointer to receive the postfixOut
     * starting at the found tagOut field.
     * @param[out] typeOut: optional pointer to receive the element typeOut character
     * @param[in] negative: return negative tagOut as it is. If disabled, then
     * always return positive tagOut. Negative tagOut is sometimes used for
     * element disambiguation.
     * @param[in] recursive: recursively find the last non-zero tagOut
     *
     * @return Return the end position of the tagOut field, or return -1 if not
     * found.
     */
    int findTagInElementName(long* tagOut = nullptr,
                             int* lenOut = nullptr,
                             std::string* postfixOut = nullptr,
                             char* typeOut = nullptr,
                             bool negative = false,
                             bool recursive = true) const;

    /// Get a hash for this MappedName
    std::size_t hash() const
    {
        return qHash(data, qHash(postfix));
    }

private:
    QByteArray data;
    QByteArray postfix;
    bool raw;
};


using ElementIDRefs = QVector<::App::StringIDRef>;

struct MappedNameRef
{
    MappedName name;
    ElementIDRefs sids;
    std::unique_ptr<MappedNameRef> next;

    MappedNameRef() = default;

    ~MappedNameRef() = default;

    MappedNameRef(MappedName name, ElementIDRefs sids = ElementIDRefs())
        : name(std::move(name))
        , sids(std::move(sids))
    {
        compact();
    }

    MappedNameRef(const MappedNameRef& other)
        : name(other.name)
        , sids(other.sids)
    {}

    MappedNameRef(MappedNameRef&& other) noexcept
        : name(std::move(other.name))
        , sids(std::move(other.sids))
        , next(std::move(other.next))
    {}

    MappedNameRef& operator=(const MappedNameRef& other) noexcept
    {
        name = other.name;
        sids = other.sids;
        return *this;
    }

    MappedNameRef& operator=(MappedNameRef&& other) noexcept
    {
        name = std::move(other.name);
        sids = std::move(other.sids);
        next = std::move(other.next);
        return *this;
    }

    explicit operator bool() const
    {
        return !name.empty();
    }

    void append(const MappedName& _name, const ElementIDRefs _sids = ElementIDRefs())
    {
        if (!_name) {
            return;
        }
        if (!this->name) {
            this->name = _name;
            this->sids = _sids;
            compact();
            return;
        }
        std::unique_ptr<MappedNameRef> mappedName(new MappedNameRef(_name, _sids));
        if (!this->next) {
            this->next = std::move(mappedName);
        }
        else {
            this->next.swap(mappedName);
            this->next->next = std::move(mappedName);
        }
    }

    void compact()
    {
        if (sids.size() > 1) {
            std::sort(sids.begin(), sids.end());
            sids.erase(std::unique(sids.begin(), sids.end()), sids.end());
        }
    }

    bool erase(const MappedName& _name)
    {
        if (this->name == _name) {
            this->name.clear();
            this->sids.clear();
            if (this->next) {
                this->name = std::move(this->next->name);
                this->sids = std::move(this->next->sids);
                std::unique_ptr<MappedNameRef> tmp;
                tmp.swap(this->next);
                this->next = std::move(tmp->next);
            }
            return true;
        }

        for (std::unique_ptr<MappedNameRef>* ptr = &this->next; *ptr; ptr = &(*ptr)->next) {
            if ((*ptr)->name == _name) {
                std::unique_ptr<MappedNameRef> tmp;
                tmp.swap(*ptr);
                *ptr = std::move(tmp->next);
                return true;
            }
        }
        return false;
    }

    void clear()
    {
        this->name.clear();
        this->sids.clear();
        this->next.reset();
    }
};


// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)


}  // namespace Data
