// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************************************
 *                                                                                                 *
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>                       *
 *   Copyright (c) 2023 FreeCAD Project Association                                                *
 *                                                                                                 *
 *   This file is part of FreeCAD.                                                                 *
 *                                                                                                 *
 *   FreeCAD is free software: you can redistribute it and/or modify it under the terms of the     *
 *   GNU Lesser General Public License as published by the Free Software Foundation, either        *
 *   version 2.1 of the License, or (at your option) any later version.                            *
 *                                                                                                 *
 *   FreeCAD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;          *
 *   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     *
 *   See the GNU Lesser General Public License for more details.                                   *
 *                                                                                                 *
 *   You should have received a copy of the GNU Lesser General Public License along with           *
 *   FreeCAD. If not, see <https://www.gnu.org/licenses/>.                                         *
 *                                                                                                 *
 **************************************************************************************************/

#pragma once

#include <FCConfig.h>

#include <bitset>
#include <memory>

#include <QByteArray>
#include <QVector>

#include <Base/Bitmask.h>
#include <Base/Handle.h>
#include <Base/Persistence.h>
#include <CXX/Objects.hxx>
#include <utility>

#include <Base/PyObjectBase.h>


namespace Data
{
class MappedName;
}

namespace App
{

class StringHasher;
class StringID;
class StringIDRef;
using StringHasherRef = Base::Reference<StringHasher>;

/** Class to store a string
 *
 * The main purpose of this class is to provide an efficient storage of the
 * mapped geometry element name (i.e. the new Topological Naming), but it can
 * also be used as a general purpose string table.
 *
 * The StringID is to be stored in a string table (StringHasher), and be
 * referred to by an integer ID. The stored data can be optionally divided into
 * two parts, prefix and postfix. This is because a new mapped name is often
 * created by adding some common postfix to an existing name, so data sharing
 * can be improved using the following techniques:
 *
 *      a) reference count (through QByteArray) the main data part,
 *
 *      b) (recursively) encode prefix and/or postfix as an integer (in the
 *         format of #<hex>, e.g. #1b) that references another StringID,
 *
 *      c) Check index based name in prefix, e.g. Edge1, Vertex2, and encode
 *         only the text part as StringID. The index is stored separately in
 *         reference class StringIDRef to maximize data sharing.
 */
class AppExport StringID: public Base::BaseClass, public Base::Handled
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();  // NOLINT

public:
    /// Flag of the stored string data
    enum class Flag
    {
        /// No flag
        None = 0,
        /// The stored data is binary
        Binary = 1 << 0,
        /// The stored data is the sha1 hash of the original content
        Hashed = 1 << 1,
        /** Postfix is encoded as #<hex>, e.g. #1b, where the hex integer part
         * refers to another StringID.
         */
        PostfixEncoded = 1 << 2,
        /// The data is split as prefix and postfix
        Postfixed = 1 << 3,
        /// The prefix data is split as text + index
        Indexed = 1 << 4,
        /** The prefix data is encoded as #<hex>, e.g. #1b, where the hex
         * integer part refers to another StringID.
         */
        PrefixID = 1 << 5,
        /** The prefix split as text + index, where the text is encoded
         * using another StringID.
         */
        PrefixIDIndex = 1 << 6,
        /// The string ID is persistent regardless of internal mark
        Persistent = 1 << 7,
        /// Internal marked used to check if the string ID is used
        Marked = 1 << 8,
    };
    using Flags = Base::Flags<Flag>;

    /** Constructor
     * @param id: integer ID of this StringID
     * @param data: input data
     * @param flags: flags describes the data
     *
     * User code is not supposed to create StringID directly, but through StringHasher::getID()
     */
    StringID(long id, QByteArray data, const Flags& flags = Flag::None)
        : _id(id)
        , _data(std::move(data))
        , _flags(flags)
    {}

    /// Constructs an empty StringID
    StringID()
        : _id(0)
        , _flags(Flag::None)
    {}

    StringID(const StringID& other) = delete;
    StringID(StringID&& other) noexcept = delete;
    StringID& operator=(const StringID& rhs) = delete;
    StringID& operator=(StringID&& rhs) noexcept = delete;

    ~StringID() override;

    /// Returns the ID of this StringID
    long value() const
    {
        return _id;
    }

    /// Returns all related StringIDs that used to encode this StringID
    const QVector<StringIDRef>& relatedIDs() const
    {
        return _sids;
    }

    /// @name Flag accessors
    //@{
    inline bool isBinary() const;
    inline bool isHashed() const;
    inline bool isPostfixed() const;
    inline bool isPostfixEncoded() const;
    inline bool isIndexed() const;
    inline bool isPrefixID() const;
    inline bool isPrefixIDIndex() const;
    inline bool isMarked() const;
    inline bool isPersistent() const;
    //@}

    /// Checks if this StringID is from the input hasher
    bool isFromSameHasher(const StringHasherRef& hasher) const
    {
        return this->_hasher == hasher;
    }

    /// Returns the owner hasher
    StringHasherRef getHasher() const
    {
        return {_hasher};
    }

    /// Returns the data (prefix)
    const QByteArray& data() const
    {
        return _data;
    }

    /// Returns the postfix
    QByteArray postfix() const
    {
        return _postfix;
    }

    /// Sets the postfix
    void setPostfix(QByteArray postfix)
    {
        _postfix = std::move(postfix);
    }

    PyObject* getPyObject() override;
    /// Returns a Python tuple containing both the text and index
    PyObject* getPyObjectWithIndex(int index);

    /** Convert to string representation of this StringID
     * @param index: optional index
     *
     * The format is #<id>. And if index is non zero, then #<id>:<index>. Both
     * <id> and <index> are in hex format.
     */
    std::string toString(int index = 0) const;

    /// Light weight structure of holding a string ID and associated index
    struct IndexID
    {
        long id;
        int index;

        explicit operator bool() const
        {
            return id > 0;
        }

        friend std::ostream& operator<<(std::ostream& stream, const IndexID& indexID)
        {
            stream << indexID.id;
            if (indexID.index != 0) {
                stream << ':' << indexID.index;
            }
            return stream;
        }
    };

    /** Parse string to get ID and index
     * @param name: input string
     * @param eof: Whether to check the end of string. If true, then the input
     *             string must contain only the string representation of this
     *             StringID
     * @param size: input string size, or -1 if the input string is zero terminated.
     * @return Return the integer ID and index.
     *
     * The input string is expected to be in the format of #<id> or with index
     * #<id>:<index>, where both id and index are in hex digits.
     */
    static IndexID fromString(const char* name, bool eof = true, int size = -1);

    /** Parse string to get ID and index
     * @param bytes: input data
     * @param eof: Whether to check the end of string. If true, then the input
     *             string must contain only the string representation of this
     *             StringID
     *
     * The input string is expected to be in the format of #<id> or with index
     * #<id>:<index>, where both id and index are in hex digits.
     */
    static IndexID fromString(const QByteArray& bytes, bool eof = true)
    {
        return fromString(bytes.constData(), eof, bytes.size());
    }

    /** Get the text content of this StringID
     * @param index: optional index
     * @return Return the text content of this StringID. If the data is binary,
     *         then output in base64 encoded string.
     */
    std::string dataToText(int index = 0) const;

    /** Get the content of this StringID as QByteArray
     * @param index: optional index.
     */
    QByteArray dataToBytes(int index = 0) const
    {
        QByteArray res(_data);
        if (index != 0) {
            res += QByteArray::number(index);
        }
        if (_postfix.size() != 0) {
            res += _postfix;
        }
        return res;
    }

    /// Mark this StringID as used
    void mark() const;

    /// Mark the StringID as persistent regardless of usage mark
    inline void setPersistent(bool enable);

    bool operator<(const StringID& other) const
    {
        return compare(other) < 0;
    }

    /** Compare StringID
     * @param other: the other StringID for comparison
     * @return Returns -1 if less than the other StringID, 1 if greater, or 0 if equal
     */
    int compare(const StringID& other) const
    {
        if (_hasher < other._hasher) {
            return -1;
        }
        if (_hasher > other._hasher) {
            return 1;
        }
        if (_id < other._id) {
            return -1;
        }
        if (_id > other._id) {
            return 1;
        }
        return 0;
    }

    friend class StringHasher;

private:
    long _id;
    QByteArray _data;
    QByteArray _postfix;
    StringHasher* _hasher = nullptr;
    mutable Flags _flags;
    mutable QVector<StringIDRef> _sids;
};

//////////////////////////////////////////////////////////////////////////

/** Counted reference to a StringID instance
 */
class StringIDRef
{
public:
    /// Default construction results in an empty StringIDRef object: it will evaluate to boolean
    /// "false" if queried.
    StringIDRef()
        : _sid(nullptr)
        , _index(0)
    {}

    /// Standard construction from a heap-allocated StringID. This reference-counting class manages
    /// the lifetime of the StringID, ensuring it is deallocated when its reference count goes to
    /// zero.
    /// \param stringID A pointer to a StringID allocated with "new"
    /// \param index (optional) An index value to store along with the StringID. Defaults to zero.
    StringIDRef(StringID* stringID, int index = 0)
        : _sid(stringID)
        , _index(index)
    {
        if (_sid) {
            _sid->ref();
        }
    }

    /// Copy construction results in an incremented reference count for the stored StringID
    StringIDRef(const StringIDRef& other)
        : _sid(other._sid)
        , _index(other._index)
    {
        if (_sid) {
            _sid->ref();
        }
    }

    /// Move construction does NOT increase the reference count of the StringID (instead, it
    /// invalidates the pointer in the moved object).
    StringIDRef(StringIDRef&& other) noexcept
        : _sid(other._sid)
        , _index(other._index)
    {
        other._sid = nullptr;
    }

    StringIDRef(const StringIDRef& other, int index)
        : _sid(other._sid)
        , _index(index)
    {
        if (_sid) {
            _sid->ref();
        }
    }

    ~StringIDRef()
    {
        if (_sid) {
            _sid->unref();
        }
    }

    void reset(const StringIDRef& stringID = StringIDRef())
    {
        *this = stringID;
    }

    void reset(const StringIDRef& stringID, int index)
    {
        *this = stringID;
        this->_index = index;
    }

    void swap(StringIDRef& stringID)
    {
        if (*this != stringID) {
            auto tmp = stringID;
            stringID = *this;
            *this = tmp;
        }
    }

    StringIDRef& operator=(StringID* stringID)
    {
        if (_sid == stringID) {
            return *this;
        }
        if (_sid) {
            _sid->unref();
        }
        _sid = stringID;
        if (_sid) {
            _sid->ref();
        }
        this->_index = 0;
        return *this;
    }

    StringIDRef& operator=(const StringIDRef& stringID)
    {
        if (&stringID == this) {
            return *this;
        }
        if (_sid != stringID._sid) {
            if (_sid) {
                _sid->unref();
            }
            _sid = stringID._sid;
            if (_sid) {
                _sid->ref();
            }
        }
        this->_index = stringID._index;
        return *this;
    }

    StringIDRef& operator=(StringIDRef&& stringID) noexcept
    {
        if (_sid != stringID._sid) {
            if (_sid) {
                _sid->unref();
            }
            _sid = stringID._sid;
            stringID._sid = nullptr;
        }
        this->_index = stringID._index;
        return *this;
    }

    bool operator<(const StringIDRef& stringID) const
    {
        if (!stringID._sid) {
            return false;
        }
        if (!_sid) {
            return true;
        }
        int res = _sid->compare(*stringID._sid);
        if (res < 0) {
            return true;
        }
        if (res > 0) {
            return false;
        }
        return _index < stringID._index;
    }

    bool operator==(const StringIDRef& stringID) const
    {
        if (_sid && stringID._sid) {
            return _sid->compare(*stringID._sid) == 0 && _index == stringID._index;
        }
        return _sid == stringID._sid;
    }

    bool operator!=(const StringIDRef& stringID) const
    {
        return !(*this == stringID);
    }

    explicit operator bool() const
    {
        return _sid != nullptr;
    }

    int getRefCount() const
    {
        if (_sid) {
            return _sid->getRefCount();
        }
        return 0;
    }

    std::string toString() const
    {
        if (_sid) {
            return _sid->toString(_index);
        }
        return {};
    }

    std::string dataToText() const
    {
        if (_sid) {
            return _sid->dataToText(_index);
        }
        return {};
    }

    /// Get a reference to the data: only makes sense if index and postfix are both empty, but
    /// calling code is responsible for ensuring that.
    const char* constData() const
    {
        if (_sid) {
            assert(_index == 0);
            assert(_sid->postfix().isEmpty());
            return _sid->data().constData();
        }
        return "";
    }

    const StringID& deref() const
    {
        return *_sid;
    }

    long value() const
    {
        if (_sid) {
            return _sid->value();
        }
        return 0;
    }

    QVector<StringIDRef> relatedIDs() const
    {
        if (_sid) {
            return _sid->relatedIDs();
        }
        return {};
    }

    bool isBinary() const
    {
        if (_sid) {
            return _sid->isBinary();
        }
        return false;
    }

    bool isHashed() const
    {
        if (_sid) {
            return _sid->isHashed();
        }
        return false;
    }

    void toBytes(QByteArray& bytes) const
    {
        // TODO: return the QByteArray instead of passing in by reference
        if (_sid) {
            bytes = _sid->dataToBytes(_index);
        }
    }

    PyObject* getPyObject()
    {
        if (_sid) {
            return _sid->getPyObjectWithIndex(_index);
        }
        Py_INCREF(Py_None);
        return Py_None;
    }

    void mark() const
    {
        if (_sid) {
            _sid->mark();
        }
    }

    bool isMarked() const
    {
        return _sid && _sid->isMarked();  // NOLINT
    }

    bool isFromSameHasher(const StringHasherRef& hasher) const
    {
        return _sid && _sid->isFromSameHasher(hasher);  // NOLINT
    }

    StringHasherRef getHasher() const
    {
        if (_sid) {
            return _sid->getHasher();
        }
        return {};
    }

    void setPersistent(bool enable)
    {
        if (_sid) {
            _sid->setPersistent(enable);
        }
    }

    /// Used predominantly by the unit test code to verify that index is set correctly. In general
    /// user code should not need to call this function.
    int getIndex() const
    {
        return _index;
    }

    friend class StringHasher;

private:
    StringID* _sid;
    int _index;
};


/// \brief A bidirectional map  of strings and their integer identifier.
///
/// Maps an arbitrary text string to a unique integer ID, maintaining a reference-counted shared
/// pointer for each. This permits elimination of unused strings based on their reference
/// count. If a duplicate string is added, no additional copy is made, and a new reference to the
/// original storage is returned (incrementing the reference counter of the instance).
///
/// If the string is longer than a given threshold, instead of storing the string, its SHA1 hash is
/// stored (and the original string discarded). This allows an upper threshold on the length of a
/// stored string, while still effectively guaranteeing uniqueness in the table.
class AppExport StringHasher: public Base::Persistence, public Base::Handled
{

    TYPESYSTEM_HEADER_WITH_OVERRIDE();  // NOLINT

public:
    StringHasher();
    ~StringHasher() override;

    StringHasher(const StringHasher&) = delete;
    StringHasher(StringHasher&&) noexcept = delete;
    StringHasher& operator=(StringHasher& other) = delete;
    StringHasher& operator=(StringHasher&& other) noexcept = delete;

    unsigned int getMemSize() const override;
    void Save(Base::Writer& /*writer*/) const override;
    void Restore(Base::XMLReader& /*reader*/) override;
    void SaveDocFile(Base::Writer& /*writer*/) const override;
    void RestoreDocFile(Base::Reader& /*reader*/) override;
    void setPersistenceFileName(const char* name) const;
    const std::string& getPersistenceFileName() const;

    /** Maps an arbitrary string to an integer
     *
     * @param text: input string.
     * @param len: length of the string: optional if the string is null-terminated.
     * @param hashable: whether hashing the string is permitted.
     * @return A shared pointer to the internally-stored StringID.
     *
     * Maps an arbitrary text string to a unique integer ID, returning a reference-counted shared
     * pointer to the StringID. This permits elimination of unused strings based on their reference
     * count. If a duplicate string is added, no additional copy is made, and a new reference to the
     * original storage is returned (incrementing the reference counter of the instance).
     *
     * If \c hashable is true and the string is longer than the threshold setting of this
     * StringHasher, only the SHA1 hash of the string is stored: the original content of the string
     * is discarded. If \c hashable is false, the string is copied and stored inside a StringID
     * instance.
     *
     * The purpose of this function is to provide a short form of a stable string identification.
     */
    StringIDRef getID(const char* text, int len = -1, bool hashable = false);

    /// Options for string string data
    enum class Option
    {
        /// No option is set
        None = 0,

        /// The input data is binary
        Binary = 1 << 0,

        /// Hashing is permitted for this input data. If the data length is longer than the
        /// threshold setting of the StringHasher, it will be sha1 hashed before storing, and the
        /// original content of the string is discarded.
        Hashable = 1 << 1,

        /// Do not copy the data: assume it is constant and exists for the lifetime of this hasher.
        /// If this option is not set, the data will be copied before storing.
        NoCopy = 1 << 2,
    };
    using Options = Base::Flags<Option>;

    /** Map text or binary data to an integer
     *
     * @param data: input data.
     * @param options: options describing how to store the data.
     * @return A shared pointer to the internally stored StringID.
     *
     * \sa getID (const char*, int, bool);
     */
    StringIDRef getID(const QByteArray& data, Options options = Option::Hashable);

    /** Map geometry element name to an integer */
    StringIDRef getID(const Data::MappedName& name, const QVector<StringIDRef>& sids);

    /** Obtain the reference counted StringID object from numerical id
     *
     * @param id: string ID
     * @param index: optional index of the string ID
     * @return Return a shared pointer to the internally stored StringID.
     *
     * This function exists because the stored string may be one way hashed,
     * and the original text is not persistent. The caller use this function to
     * retrieve the reference count ID object after restore
     */
    StringIDRef getID(long id, int index = 0) const;

    /** Obtain the reference counted StringID object from numerical id and index
     *
     * @param id: string ID with index
     * @return Return a shared pointer to the internally stored StringID.
     */
    StringIDRef getID(const StringID::IndexID& id) const
    {
        return getID(id.id, id.index);
    }

    std::map<long, StringIDRef> getIDMap() const;

    /// Clear all string hashes
    void clear();

    /// Size of the hash table
    size_t size() const;

    /// Return the number of hashes that are used by others
    size_t count() const;

    PyObject* getPyObject() override;

    /** Enable/disable saving all string ID
     *
     * If saveAll is true, then compact() does nothing even when called explicitly. Setting
     * saveAll it to false causes compact() to be run immediately.
     */
    void setSaveAll(bool enable);
    bool getSaveAll() const;

    /** Set threshold of string hashing
     *
     * For hashable strings that are longer than this threshold, the string will
     * be replaced by its sha1 hash.
     */
    void setThreshold(int threshold);
    int getThreshold() const;

    /** Clear internal marks
     *
     * The internal marks on internally stored StringID instances are used to
     * check if the StringID is used.
     */
    void clearMarks() const;

    /// Compact string storage by eliminating unused strings from the table.
    void compact();

    class HashMap;
    friend class StringID;

protected:
    StringID* insert(const StringIDRef& sid);
    long lastID() const;
    void saveStream(std::ostream& stream) const;
    void restoreStream(std::istream& stream, std::size_t count);
    void restoreStreamNew(std::istream& stream, std::size_t count);

private:
    std::unique_ptr<HashMap>
        _hashes;  ///< Bidirectional map of StringID and its index (a long int).
    mutable std::string _filename;
};
}  // namespace App

ENABLE_BITMASK_OPERATORS(App::StringID::Flag)
ENABLE_BITMASK_OPERATORS(App::StringHasher::Option)

namespace App
{
inline bool StringID::isBinary() const
{
    return _flags.testFlag(Flag::Binary);
}
inline bool StringID::isHashed() const
{
    return _flags.testFlag(Flag::Hashed);
}
inline bool StringID::isPostfixed() const
{
    return _flags.testFlag(Flag::Postfixed);
}
inline bool StringID::isPostfixEncoded() const
{
    return _flags.testFlag(Flag::PostfixEncoded);
}
inline bool StringID::isIndexed() const
{
    return _flags.testFlag(Flag::Indexed);
}
inline bool StringID::isPrefixID() const
{
    return _flags.testFlag(Flag::PrefixID);
}
inline bool StringID::isPrefixIDIndex() const
{
    return _flags.testFlag(Flag::PrefixIDIndex);
}
inline bool StringID::isMarked() const
{
    return _flags.testFlag(Flag::Marked);
}
inline bool StringID::isPersistent() const
{
    return _flags.testFlag(Flag::Persistent);
}
inline void StringID::setPersistent(bool enable)
{
    _flags.setFlag(Flag::Persistent, enable);
}
}  // namespace App