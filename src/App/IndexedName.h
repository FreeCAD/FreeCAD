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

#ifndef APP_INDEXEDNAME_H
#define APP_INDEXEDNAME_H

#include <cassert>
#include <cstring>
#include <ostream>
#include <string>
#include <vector>

#include <QByteArray>
#include <QHash>

#include "FCGlobal.h"


namespace Data
{

/// The IndexedName class provides a very memory-efficient data structure to hold a name and an index
/// value, and to perform various comparisons and validations of those values. The name must only
/// consist of upper- and lower-case ASCII characters and the underscore ('_') character. The index
/// must be a positive integer. The string representation of this IndexedName is the name followed by
/// the index, with no spaces between: an IndexedName may be constructed from this string. For
/// example "EDGE1" or "FACE345" might be the names of elements that use an IndexedName. If there is
/// then an "EDGE2", only a pointer to the original stored name "EDGE" is retained.
///
/// The memory efficiency of the class comes from re-using the same character storage for names that
/// match, while retaining their differing indices. This is achieved by either using user-provided
/// const char * names (provided as a list of typeNames and presumed to never be deallocated), or by
/// maintaining an internal list of names that have been used before, and can be re-used later.
class AppExport IndexedName {
public:

    /// Construct from a name and an optional index. If the name contains an index it is read, but
    /// is used as the index *only* if _index parameter is unset. If the _index parameter is given
    /// it overrides any trailing integer in the name. Index must be positive, and name must contain
    /// only ASCII letters and the underscore character. If these conditions are not met, name is
    /// set to the empty string, and isNull() will return true.
    ///
    /// \param name The new name - ASCII letters and underscores only, with optional integer suffix.
    /// This memory will be copied into a new internal storage location and need not be persistent.
    /// \param _index The new index - if provided, it overrides any suffix provided by name
    explicit IndexedName(const char *name = nullptr, int _index = 0)
        : index(0)
    {
        assert(_index >= 0);
        if (!name) {
            this->type = "";
        }
        else {
            set(name);
            if (_index > 0) {
                this->index = _index;
            }
        }
    }

    /// Create an indexed name that is restricted to a list of preset type names. If it appears in
    /// that list, only a pointer to the character storage in the list is retained: the memory
    /// locations pointed at by the list must never be destroyed once they have been used to create
    /// names. If allowOthers is true (the default) then a requested name that is not in the list
    /// will be added to a static internal storage table, and its memory then re-used for later
    /// objects with the same name. If allowOthers is false, then the name request is rejected, and
    /// the name is treated as null.
    ///
    /// \param name The new name - ASCII letters and underscores only, with optional integer suffix
    /// \param allowedTypeNames A vector of allowed names. Storage locations must persist for the
    /// entire run of the program.
    /// \param allowOthers Whether a name not in allowedTypeNames is permitted. If true (the
    /// default) then a name not in allowedTypeNames is added to a static internal storage vector
    /// so that it can be re-used later without additional memory allocation.
    IndexedName(const char *name,
                const std::vector<const char*> & allowedTypeNames,
                bool allowOthers=true) : type(""), index(0)
    {
        set(name, -1, allowedTypeNames, allowOthers);
    }

    /// Construct from a QByteArray, but explicitly making a copy of the name on its first
    /// occurrence. If this is a name that has already been stored internally, no additional copy
    /// is made.
    ///
    /// \param data The QByteArray to copy the data from
    explicit IndexedName(const QByteArray & data) : type(""), index(0)
    {
        set(data.constData(), data.size());
    }

    /// Given constant name and an index, re-use the existing memory for the name, not making a copy
    /// of it, or scanning any existing storage for it. The name must never become invalid for the
    /// lifetime of the object it names. This memory will never be re-used by another object.
    ///
    /// \param name The name of the object. This memory is NOT copied and must be persistent.
    /// \param index A positive, non-zero integer
    /// \return An IndexedName with the given name and index, re-using the existing memory for name
    static IndexedName fromConst(const char *name, int index) {
        assert (index >= 0);
        IndexedName res;
        res.type = name;
        res.index = index;
        return res;
    }

    /// Given an existing std::string, *append* this name to it. If index is not zero, this will
    /// include the index.
    ///
    /// \param buffer A (possibly non-empty) string buffer to append the name to.
    void appendToStringBuffer(std::string & buffer) const
    {
        buffer += this->type;
        if (this->index > 0) {
            buffer += std::to_string(this->index);
        }
    }

    /// Create and return a new std::string with this name in it.
    ///
    /// \return A newly-created string with the IndexedName in it (e.g. "EDGE42")
    std::string toString() const
    {
        std::string result;
        this->appendToStringBuffer(result);
        return result;
    }

    /// An indexedName is represented as the simple concatenation of the name and its index, e.g.
    /// "EDGE1" or "FACE42".
    friend std::ostream & operator<<(std::ostream & stream, const IndexedName & indexedName)
    {
        stream << indexedName.type;
        if (indexedName.index > 0) {
            stream << indexedName.index;
        }
        return stream;
    }

    /// True only if both the name and index compare exactly equal.
    bool operator==(const IndexedName & other) const
    {
        return this->index == other.index
            && (this->type == other.type
                || std::strcmp(this->type, other.type)==0);
    }

    /// Increments the index by the given offset. Does not affect the text part of the name.
    IndexedName & operator+=(int offset)
    {
        this->index += offset;
        assert(this->index >= 0);
        return *this;
    }

    /// Pre-increment operator: increases the index of this element by one.
    IndexedName & operator++()
    {
        ++this->index;
        return *this;
    }

    /// Pre-decrement operator: decreases the index of this element by one. Must not make the index
    /// negative (only checked when compiled in debug mode).
    IndexedName & operator--()
    {
        --this->index;
        assert(this->index >= 0);
        return *this;
    }

    /// True if either the name or the index compare not equal.
    bool operator!=(const IndexedName & other) const
    {
        return !(this->operator==(other));
    }

    /// Equivalent to C++20's operator <=>
    int compare(const IndexedName & other) const
    {
        int res = std::strcmp(this->type, other.type);
        if (res != 0) {
            return res;
        }
        if (this->index < other.index) {
            return -1;
        }
        if (this->index > other.index) {
            return 1;
        }
        return 0;
    }

    /// Provided to enable sorting operations: the comparison is first lexicographical for the text
    /// element of the names, then numerical for the indices.
    bool operator<(const IndexedName & other) const
    {
        return compare(other) < 0;
    }

    /// Allow direct memory access to the individual characters of the text portion of the name.
    /// NOTE: input is not range-checked when compiled in release mode.
    char operator[](int input) const
    {
        assert(input >= 0);
        assert(input < static_cast<int>(std::strlen(this->type)));
        // When we support C++20 we can use std::span<> to eliminate the clang-tidy warning
        // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
        return this->type[input];
    }

    /// Get a pointer to text part of the name - does NOT make a copy, returns direct memory access
    const char * getType() const { return this->type; }

    /// Get the numerical part of the name
    int getIndex() const { return this->index; }

    /// Set the numerical part of the name (note that there is no equivalent function to allow
    /// changing the text part of the name, which is immutable once created).
    ///
    /// \param input The new index. Must be a positive non-zero integer
    void setIndex(int input) { assert(input>=0); this->index = input; }

    /// A name is considered "null" if its text component is an empty string.
    // When we support C++20 we can use std::span<> to eliminate the clang-tidy warning
    // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
    bool isNull() const { return this->type[0] == '\0'; }

    /// Boolean conversion provides the opposite of isNull(), yielding true when the text part of
    /// the name is NOT the empty string.
    explicit operator bool() const { return !isNull(); }

protected:
    /// Apply the IndexedName rules and either store the characters of a new type or a reference to
    /// the characters in a type named in types, or stored statically within this function. If len
    /// is not set, or set to -1 (the default), then the provided string in name is scanned for its
    /// length using strlen (e.g. it must be null-terminated).
    ///
    /// \param name The new name. If necessary a copy is made, this char * need not be persistent
    /// \param length The length of name
    /// \param allowedNames A vector of storage locations of allowed names. These storage locations
    /// must be persistent for the duration of the program run.
    /// \param allowOthers If true (the default), then if name is not in allowedNames it is allowed,
    /// and it is added to internal storage (making a copy of the name if this is its first
    /// occurrence).
    void set(const char *name,
             int length = -1,
             const std::vector<const char *> & allowedNames = {},
             bool allowOthers = true);

private:
    const char * type;
    int index;
};


/// A thin wrapper around a QByteArray providing the ability to force a copy of the data at any
/// time, even if it isn't being written to. The standard assignment operator for this class *does*
/// make a copy of the data, unlike the standard assignment operator for QByteArray.
struct ByteArray
{
    explicit ByteArray(QByteArray other)
        :bytes(std::move(other))
    {}

    ByteArray(const ByteArray& other) = default;

    ByteArray(ByteArray&& other) noexcept
        :bytes(std::move(other.bytes))
    {}

    ~ByteArray() = default;

    /// Guarantee that the stored QByteArray does not share its memory with another instance.
    void ensureUnshared() const
    {
        QByteArray copy;
        copy.append(bytes.constData(), bytes.size());
        bytes = copy;
    }

    bool operator==(const ByteArray& other) const {
        return bytes == other.bytes;
    }

    ByteArray &operator=(const ByteArray & other) {
        bytes.clear();
        bytes.append(other.bytes.constData(), other.bytes.size());
        return *this;
    }

    ByteArray &operator= (ByteArray&& other) noexcept
    {
        bytes = std::move(other.bytes);
        return *this;
    }

    mutable QByteArray bytes;
};


struct ByteArrayHasher
{
    std::size_t operator()(const ByteArray& bytes) const
    {
        return qHash(bytes.bytes);
    }

    std::size_t operator()(const QByteArray& bytes) const
    {
        return qHash(bytes);
    }
};

}

#endif // APP_INDEXEDNAME_H
