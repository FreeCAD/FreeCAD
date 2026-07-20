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

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Base/ByteBuffer.h>
#include <Base/BytesView.h>
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
     * @brief Create a MappedName from a string view.
     *
     * Create a MappedName from a string view, optionally prefixed by an element
     * map prefix, which will be omitted from the stored MappedName.
     *
     * @param[in] name The new name. A deep copy is made.
     */
    explicit MappedName(std::string_view name)
    {
        constexpr std::string_view elementMapPrefix {ELEMENT_MAP_PREFIX, ELEMENT_MAP_PREFIX_SIZE};
        if (name.starts_with(elementMapPrefix)) {
            name.remove_prefix(ELEMENT_MAP_PREFIX_SIZE);
        }
        data = Base::ByteBuffer::copy(Base::BytesView(name.data(), name.size()));
    }

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
    explicit MappedName(const char* name, int size = -1)
        : MappedName(name ? std::string_view(name,
                                             size < 0 ? std::strlen(name)
                                                      : static_cast<std::size_t>(size))
                          : std::string_view())
    {}

    /**
     * @brief Create a MappedName from a C++ std::string.
     *
     * Create a MappedName from a C++ std::string, optionally prefixed by an
     * element map prefix, which will be omitted from the stored MappedName.
     *
     * @param[in] nameString The new name. A deep copy is made.
     */
    explicit MappedName(const std::string& nameString)
        : MappedName(std::string_view(nameString))
    {}

    /**
     * @brief Create a MappedName from an IndexedName.
     *
     * Create a MappedName from an IndexedName. If non-zero, the numerical part
     * of the IndexedName is appended as text to the MappedName. In that case
     * the memory is *not* shared between the original IndexedName and the
     * MappedName.
     */
    explicit MappedName(const IndexedName& element)
        : data(Base::ByteBuffer::borrow(
              Base::BytesView(element.getType(), std::strlen(element.getType()))
          ))
        , raw(true)
    {
        if (element.getIndex() > 0) {
            this->data.append(std::to_string(element.getIndex()));
            this->raw = false;
        }
    }


    /// Create a MappedName from a StringIdRef.
    explicit MappedName(const App::StringIDRef& sid)
    {
        Base::ByteBuffer bytes;
        sid.toBytes(bytes);
        this->data = std::move(bytes);
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
        : data(other.toBytes())
    {
        if (postfix) {
            this->postfix = Base::ByteBuffer::copy(postfix);
        }
        this->raw = false;
    }

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
            if (size < 0) {
                size = static_cast<int>(std::strlen(name));
            }
            res.data = Base::ByteBuffer::borrow(
                Base::BytesView(name, static_cast<std::size_t>(size))
            );
            res.raw = true;
        }
        return res;
    }

    /**
     * @brief Construct a MappedName from byte view data.
     *
     * Construct a MappedName from byte view data (including any embedded null
     * characters).
     *
     * @param[in] bytes The original data. No copy is made: the returned object
     * borrows the provided memory.
     *
     * @return a new MappedName with bytes as its data.
     */
    static MappedName fromRawData(Base::BytesView bytes)
    {
        return fromRawData(bytes.data(), static_cast<int>(bytes.size()));
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

        if (startPosition >= static_cast<int>(other.data.size())) {
            return {other, startPosition, size};
        }

        MappedName res;
        res.raw = true;
        if (size < 0) {
            size = other.size() - startPosition;
        }

        const int dataSize = static_cast<int>(other.data.size());
        if (size < dataSize - startPosition) {
            res.data = Base::ByteBuffer::borrow(
                Base::BytesView(other.data.data() + startPosition, static_cast<std::size_t>(size))
            );
        }
        else {
            res.data = Base::ByteBuffer::borrow(
                Base::BytesView(other.data.data() + startPosition,
                                static_cast<std::size_t>(dataSize - startPosition))
            );
            size -= dataSize - startPosition;
            const int postfixSize = static_cast<int>(other.postfix.size());
            if (size == postfixSize) {
                res.postfix = other.postfix;
            }
            else if (size != 0) {
                res.postfix = Base::ByteBuffer::borrow(
                    Base::BytesView(other.postfix.data(), static_cast<std::size_t>(size))
                );
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
        stream.write(mappedName.data.data(), static_cast<std::streamsize>(mappedName.data.size()));
        stream.write(mappedName.postfix.data(),
                     static_cast<std::streamsize>(mappedName.postfix.size()));
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

        if (!bytesStartsWith(larger.data.view(), smaller.data.view())) {
            return false;
        }

        Base::BytesView tmp = Base::BytesView(
            larger.data.data() + smaller.data.size(),
            larger.data.size() - smaller.data.size()
        );

        if (!bytesStartsWith(smaller.postfix.view(), tmp)) {
            return false;
        }

        Base::BytesView remaining = Base::BytesView(
            smaller.postfix.data() + tmp.size(),
            smaller.postfix.size() - tmp.size()
        );

        return remaining == larger.postfix.view();
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
     * @param[in] other The byte view to append.
     *
     * @return A new MappedName whose data is the LHS argument's data and whose
     * postfix is the LHS argument's postfix with the RHS argument appended to
     * it.
     */
    MappedName operator+(Base::BytesView other) const
    {
        MappedName res(*this);
        res += other;
        return res;
    }

    MappedName operator+(const Base::ByteBuffer& other) const
    {
        return (*this) + other.view();
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
            this->postfix.append(other);
        }
        return *this;
    }

    /// @copydoc operator+=(const char* other)
    MappedName& operator+=(const std::string& other)
    {
        if (!other.empty()) {
            this->postfix.reserve(this->postfix.size() + other.size());
            this->postfix.append(other);
        }
        return *this;
    }

    MappedName& operator+=(Base::BytesView other)
    {
        this->postfix.append(other);
        return *this;
    }

    MappedName& operator+=(const Base::ByteBuffer& other)
    {
        return (*this) += other.view();
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
                size = static_cast<int>(std::strlen(dataToAppend));
            }
            if (empty()) {
                this->data = Base::ByteBuffer::copy(
                    Base::BytesView(dataToAppend, static_cast<std::size_t>(size))
                );
                this->raw = false;
            }
            else {
                this->postfix.append(Base::BytesView(dataToAppend, static_cast<std::size_t>(size)));
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


        const int otherDataSize = static_cast<int>(other.data.size());
        const int otherPostfixSize = static_cast<int>(other.postfix.size());

        if (startPosition < otherDataSize)  // if starting inside data
        {
            int count = size;
            // make sure count doesn't exceed data size and end up in postfix
            if (count > otherDataSize - startPosition) {
                count = otherDataSize - startPosition;
            }

            // if this is empty append in data else append in postfix
            if (startPosition == 0 && count == otherDataSize && this->empty()) {
                this->data = other.data;
                this->raw = other.raw;
            }
            else {
                append(other.data.data() + startPosition, count);
            }

            // setup startPosition and count to continue appending the remainder to postfix
            startPosition = 0;
            size -= count;
        }
        else  // else starting inside postfix
        {
            startPosition -= otherDataSize;
        }

        // if there is still data to be added to postfix
        if (size != 0) {
            if (startPosition == 0 && size == otherPostfixSize) {
                if (this->empty()) {
                    this->data = other.postfix;
                    this->raw = other.raw;
                }
                else if (this->postfix.empty()) {
                    this->postfix = other.postfix;
                }
                else {
                    this->postfix.append(other.postfix.view());
                }
            }
            else {
                append(other.postfix.data() + startPosition, size);
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
        const int dataSize = static_cast<int>(this->data.size());
        if (startPosition < dataSize) {
            count = dataSize - startPosition;
            if (len < count) {
                count = len;
            }
            buffer.append(this->data.data() + startPosition, count);
            len -= count;
        }
        buffer.append(this->postfix.data(), len);
        return buffer.c_str() + offset;
    }

    // if offset is inside data return data, if offset is > data.size
    //(ends up in postfix) return postfix
    const char* toConstString(int offset, int& size) const
    {
        if (offset < 0) {
            offset = 0;
        }
        const int dataSize = static_cast<int>(this->data.size());
        const int postfixSize = static_cast<int>(this->postfix.size());
        if (offset > dataSize) {
            offset -= dataSize;
            if (offset > postfixSize) {
                size = 0;
                return "";
            }
            size = postfixSize - offset;
            return this->postfix.data() + offset;
        }
        size = dataSize - offset;
        return this->data.data() + offset;
    }

    /**
     * @brief Convert this mapped name to raw bytes.
     *
     * Get access to raw byte data. When possible, data is borrowed from this
     * instance. If the combination of offset and size results in data that
     * crosses the boundary between this->data and this->postfix, the data must
     * be copied in order to provide access as a continuous array of bytes.
     *
     * @param[in] offset The start position of the raw data access.
     *
     * @param[in] size The number of bytes to access. If omitted, the resulting
     * byte buffer includes everything starting from offset to the end,
     * including any postfix data.
     *
     * @return A new buffer that borrows from this instance if possible, or is a
     * new copy if required by offset and size.
     */
    Base::ByteBuffer toRawBytes(int offset = 0, int size = -1) const
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
        const int dataSize = static_cast<int>(this->data.size());
        if (offset >= dataSize) {
            offset -= dataSize;
            return Base::ByteBuffer::borrow(
                Base::BytesView(this->postfix.data() + offset, static_cast<std::size_t>(size))
            );
        }
        if (size <= dataSize - offset) {
            return Base::ByteBuffer::borrow(
                Base::BytesView(this->data.data() + offset, static_cast<std::size_t>(size))
            );
        }

        Base::ByteBuffer res = Base::ByteBuffer::copy(
            Base::BytesView(this->data.data() + offset,
                            static_cast<std::size_t>(dataSize - offset))
        );
        const int remaining = size - (dataSize - offset);
        res.append(Base::BytesView(this->postfix.data(), static_cast<std::size_t>(remaining)));
        return res;
    }

    /// Direct access to the stored data bytes (no copy).
    Base::BytesView dataBytes() const
    {
        return this->data.view();
    }

    /// Direct access to the stored postfix bytes (no copy).
    Base::BytesView postfixBytes() const
    {
        return this->postfix.view();
    }

    /// Convenience function providing access to the pointer to the beginning of the postfix data.
    const char* constPostfix() const
    {
        return this->postfix.data();
    }

    // No constData() because 'data' is allowed to contain raw data, which may not end with 0.

    /**
     * @brief Provide access to the content of this instance.
     *
     * If either postfix or data is empty, no copy is made and the original
     * buffer is returned. If this instance contains both data and postfix, a
     * new buffer is created and stores a copy of the data and postfix
     * concatenated together.
     *
     * @return Either a new or the current byte buffer.
     */
    Base::ByteBuffer toBytes() const
    {
        if (this->postfix.empty()) {
            return this->data;
        }
        if (this->data.empty()) {
            return this->postfix;
        }
        Base::ByteBuffer res = Base::ByteBuffer::copy(this->data.view());
        res.append(this->postfix.view());
        return res;
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
        if (this->postfix.empty()) {
            return IndexedName(Base::BytesView(this->data.data(), this->data.size()));
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
        if (this->empty()) {
            return '\0';
        }
        if (index < 0) {
            index = 0;
        }
        const std::size_t dataSize = this->data.size();
        const std::size_t postfixSize = this->postfix.size();
        std::size_t idx = static_cast<std::size_t>(index);
        if (idx >= dataSize) {
            if (postfixSize == 0U) {
                return this->data.data()[dataSize - 1U];
            }
            idx -= dataSize;
            if (idx >= postfixSize) {
                idx = postfixSize - 1U;
            }
            return this->postfix.data()[idx];
        }
        return this->data.data()[idx];
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
        return static_cast<int>(this->data.size() + this->postfix.size());
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
        return this->data.empty() && this->postfix.empty();
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
        res.data = Base::ByteBuffer::copy(this->data.view());
        res.postfix = this->postfix;
        res.raw = false;
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
     * @param[in] searchTarget The bytes to search for.
     * @param[in] startPosition A byte offset to start the search at.
     *
     * @return The position of the target in this instance, or -1 if the target
     * is not found.
     */
    int find(Base::BytesView searchTarget, int startPosition = 0) const
    {
        if (startPosition < 0) {
            startPosition = 0;
        }
        const std::size_t dataSize = this->data.size();
        if (static_cast<std::size_t>(startPosition) < dataSize) {
            const std::size_t found =
                this->data.view().find(searchTarget, static_cast<std::size_t>(startPosition));
            if (found != Base::BytesView::npos) {
                return static_cast<int>(found);
            }
            startPosition = 0;
        }
        else {
            startPosition -= static_cast<int>(dataSize);
        }
        const std::size_t found =
            this->postfix.view().find(searchTarget, static_cast<std::size_t>(startPosition));
        if (found == Base::BytesView::npos) {
            return -1;
        }
        return static_cast<int>(found + dataSize);
    }

    /// @copydoc find(Base::BytesView,int) const
    int find(const char* searchTarget, int startPosition = 0) const
    {
        if (!searchTarget) {
            return -1;
        }
        return find(Base::BytesView(searchTarget), startPosition);
    }

    /// @copydoc find(Base::BytesView,int) const
    int find(const std::string& searchTarget, int startPosition = 0) const
    {
        return find(Base::BytesView(searchTarget.data(), searchTarget.size()), startPosition);
    }

    /**
     * @brief Reverse find a string of characters in this mapped name.
     *
     * Find a string of characters in this mapped name, starting at the back of
     * postfix and proceeding in reverse through the data. The bytes must occur
     * either entirely in the data, or entirely in the postfix: a string that
     * overlaps the two will not be found.
     *
     * @param[in] searchTarget The bytes to search for.
     *
     * @param[in] startPosition A byte offset to start the search at. Negative
     * numbers are supported and count back from the end of the concatenated
     * data.
     *
     * @return The position of the target in this instance, or -1 if the target
     * is not found.
     */
    int rfind(Base::BytesView searchTarget, int startPosition = -1) const
    {
        const int totalSize = this->size();
        if (searchTarget.empty()) {
            if (startPosition < 0) {
                return totalSize;
            }
            return std::min(startPosition, totalSize);
        }

        if (totalSize == 0) {
            return -1;
        }

        if (startPosition < 0) {
            startPosition = totalSize + startPosition;
        }
        if (startPosition >= totalSize) {
            startPosition = totalSize - 1;
        }
        if (startPosition < 0) {
            return -1;
        }

        const std::size_t dataSize = this->data.size();
        if (static_cast<std::size_t>(startPosition) >= dataSize) {
            const std::size_t from = static_cast<std::size_t>(startPosition) - dataSize;
            const std::size_t found =
                this->postfix.view().rfind(searchTarget, std::min(from, this->postfix.size() - 1U));
            if (found != Base::BytesView::npos) {
                return static_cast<int>(found + dataSize);
            }
            if (dataSize == 0U) {
                return -1;
            }
            startPosition = static_cast<int>(dataSize) - 1;
        }

        const std::size_t from = std::min<std::size_t>(static_cast<std::size_t>(startPosition), dataSize - 1U);
        const std::size_t found = this->data.view().rfind(searchTarget, from);
        if (found == Base::BytesView::npos) {
            return -1;
        }
        return static_cast<int>(found);
    }

    /// @copydoc rfind(Base::BytesView,int) const
    int rfind(const char* searchTarget, int startPosition = -1) const
    {
        if (!searchTarget) {
            return -1;
        }
        return rfind(Base::BytesView(searchTarget), startPosition);
    }

    /// @copydoc rfind(Base::BytesView,int) const
    int rfind(const std::string& searchTarget, int startPosition = -1) const
    {
        return rfind(Base::BytesView(searchTarget.data(), searchTarget.size()), startPosition);
    }

    /**
     * @brief Check if this mapped name ends with the search target.
     *
     * If there is a postfix, only the postfix is considered. If not, then only
     * the data is considered. A search string that overlaps the two will not
     * be found.
     *
     * @param[in] searchTarget The bytes to search for at the end of this mapped name.
     *
     * @return true if this MappedName ends with the search target.
     */
    bool endsWith(Base::BytesView searchTarget) const
    {
        if (!this->postfix.empty()) {
            return bytesEndsWith(this->postfix.view(), searchTarget);
        }
        return bytesEndsWith(this->data.view(), searchTarget);
    }

    /// @copydoc endsWith(Base::BytesView) const
    bool endsWith(const char* searchTarget) const
    {
        if (!searchTarget) {
            return false;
        }
        return endsWith(Base::BytesView(searchTarget));
    }

    /// @copydoc endsWith(Base::BytesView) const
    bool endsWith(const std::string& searchTarget) const
    {
        return endsWith(Base::BytesView(searchTarget.data(), searchTarget.size()));
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
    bool startsWith(Base::BytesView searchTarget, int offset = 0) const
    {
        if (offset < 0) {
            offset = 0;
        }
        if (searchTarget.size() > static_cast<std::size_t>(size() - offset)) {
            return false;
        }
        if ((offset != 0) || (!this->data.empty() && this->data.size() < searchTarget.size())) {
            return toRawBytes(offset, static_cast<int>(searchTarget.size())).view() == searchTarget;
        }
        if (!this->data.empty()) {
            return bytesStartsWith(this->data.view(), searchTarget);
        }
        return bytesStartsWith(this->postfix.view(), searchTarget);
    }

    /// @copydoc startsWith(Base::BytesView,int) const
    bool startsWith(const char* searchTarget, int offset = 0) const
    {
        if (!searchTarget) {
            return false;
        }
        return startsWith(Base::BytesView(searchTarget), offset);
    }

    /// @copydoc startsWith(Base::BytesView,int) const
    bool startsWith(const std::string& searchTarget, int offset = 0) const
    {
        return startsWith(Base::BytesView(searchTarget.data(), searchTarget.size()), offset);
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

private:
    static bool bytesStartsWith(Base::BytesView value, Base::BytesView prefix)
    {
        if (prefix.size() > value.size()) {
            return false;
        }
        return std::memcmp(value.data(), prefix.data(), prefix.size()) == 0;
    }

    static bool bytesEndsWith(Base::BytesView value, Base::BytesView suffix)
    {
        if (suffix.size() > value.size()) {
            return false;
        }
        const std::size_t offset = value.size() - suffix.size();
        return std::memcmp(value.data() + offset, suffix.data(), suffix.size()) == 0;
    }

private:
    Base::ByteBuffer data;
    Base::ByteBuffer postfix;
    bool raw {false};
};


using ElementIDRefs = std::vector<::App::StringIDRef>;

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
        if (sids.size() > 1U) {
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
