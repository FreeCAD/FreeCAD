/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#ifndef APP_STRINGID_H
#define APP_STRINGID_H

#include <bitset>
#include <memory>

#include <QByteArray>
#include <QVector>

#include <Base/Bitmask.h>
#include <Base/Handle.h>
#include <Base/Persistence.h>
#include <CXX/Objects.hxx>


namespace Data{
class MappedName;
}

namespace App {

class StringHasher;
class StringID;
class StringIDRef;
typedef Base::Reference<StringHasher> StringHasherRef;

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
class AppExport StringID: public Base::BaseClass, public Base::Handled {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    /// Flag of the stored string data
    enum class Flag {
        /// No flag
        None            = 0,
        /// The stored data is binary
        Binary          = 1 << 0,
        /// The stored data is the sha1 hash of the original content
        Hashed          = 1 << 1,
        /** Postfix is encoded as #<hex>, e.g. #1b, where the hex integer part
         * refers to another StringID.
         */
        PostfixEncoded  = 1 << 2,
        /// The data is splited as prefix and postfix
        Postfixed       = 1 << 3,
        /// The prefix data is split as text + index
        Indexed         = 1 << 4,
        /** The prefix data is encoded as #<hex>, e.g. #1b, where the hex
         * integer part refers to another StringID.
         */
        PrefixID        = 1 << 5,
        /** The prefix split as text + index, where the text is encoded
         * using another StringID.
         */
        PrefixIDIndex   = 1 << 6,
        /// The string ID is persistent regardless of internal mark */
        Persistent      = 1 << 7,
        /// Internal marked used to check if the string ID is used
        Marked          = 1 << 8,
    };
    typedef Base::Flags<Flag> Flags;

    /** Constructor
     * @param id: integer ID of this StringID
     * @param data: input data
     * @param flags: flags describes the data
     *
     * User code is not supposed to create StringID directly, but through StringHasher::getID()
     */
    StringID(long id, const QByteArray &data, const Flags &flags=Flag::None)
        :_id(id),_data(data),_flags(flags)
    {}

    /// Constructs an empty StringID
    StringID()
        :_id(0), _flags(Flag::None)
    {}

    virtual ~StringID();

    /// Returns the ID of this StringID
    long value() const {return _id;}

    /// Returns all related StringIDs that used to encode this StringID
    const QVector<StringIDRef> &relatedIDs() const {return _sids;}

    /// @name Flag accessors
    //@{
    bool isBinary() const {return _flags.testFlag(Flag::Binary);}
    bool isHashed() const {return _flags.testFlag(Flag::Hashed);}
    bool isPostfixed() const {return _flags.testFlag(Flag::Postfixed);}
    bool isPostfixEncoded() const {return _flags.testFlag(Flag::PostfixEncoded);}
    bool isIndexed() const {return _flags.testFlag(Flag::Indexed);}
    bool isPrefixID() const {return _flags.testFlag(Flag::PrefixID);}
    bool isPrefixIDIndex() const {return _flags.testFlag(Flag::PrefixIDIndex);}
    bool isMarked() const {return _flags.testFlag(Flag::Marked);}
    bool isPersistent() const {return _flags.testFlag(Flag::Persistent);}
    //@}

    /// Checks if this StringID is from the input hasher
    bool isFromSameHasher(const StringHasherRef & hasher) const
    {
        return this->_hasher == hasher;
    }

    /// Returns the owner hasher
    StringHasherRef getHasher() const
    {
        return StringHasherRef(_hasher);
    }

    /// Returns the data (prefix)
    const QByteArray data() const {return _data;}
    /// Returns the postfix
    const QByteArray postfix() const {return _postfix;}

    virtual PyObject *getPyObject() override;
    /// Returns a Python tuple containing both the text and index
    PyObject *getPyObjectWithIndex(int index);

    /** Convert to string represtation of this StringID
     * @param index: optional index
     *
     * The format is #<id>. And if index is non zero, then #<id>:<index>. Both
     * <id> and <index> are in hex format.
     */
    std::string toString(int index) const;

    /// Light weight structure of holding a string ID and associated index
    struct IndexID {
        long id;
        int index;

        explicit operator bool() const {
            return id > 0;
        }

        friend std::ostream & operator << (std::ostream &s, const IndexID & id) {
            s << id.id;
            if (id.index)
                s << ':' << id.index;
            return s;
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
    static IndexID fromString(const char *name, bool eof=true, int size = -1);

    /** Parse string to get ID and index
     * @param bytes: input data
     * @param eof: Whether to check the end of string. If true, then the input
     *             string must contain only the string representation of this
     *             StringID
     *
     * The input string is expected to be in the format of #<id> or with index
     * #<id>:<index>, where both id and index are in hex digits.
     */
    static IndexID fromString(const QByteArray &bytes, bool eof=true) {
        return fromString(bytes.constData(), eof, bytes.size());
    }

    /** Get the text content of this StringID
     * @param index: optional index
     * @return Return the text content of this StringID. If the data is binary,
     *         then output in base64 encoded string. 
     */
    std::string dataToText(int index) const;

    /** Get the content of this StringID as QByteArray
     * @param bytes: output bytes
     * @param index: opttional index.
     */
    void toBytes(QByteArray &bytes, int index) const {
        if (_postfix.size())
            bytes = _data + _postfix;
        else if (index)
            bytes = _data + QByteArray::number(index);
        else
            bytes = _data;
    }

    /// Mark this StringID as used
    void mark() const;

    /// Mark the StringID as persistent regardless of usage mark
    void setPersistent(bool enable)
    {
        _flags.setFlag(Flag::Persistent, enable);
    }

    bool operator<(const StringID &other) const {
        return compare(other) < 0;
    }

    /** Compare StringID
     * @param other: the other StringID for comparison
     * @return Returns -1 if less than the other StringID, 1 if greater, or 0 if equal
     */
    int compare(const StringID &other) const {
        if (_hasher < other._hasher)
            return -1;
        if (_hasher > other._hasher)
            return 1;
        if (_id < other._id)
            return -1;
        if (_id > other._id)
            return 1;
        return 0;
    }

    friend class StringHasher;

private:
    long _id;
    QByteArray _data;
    QByteArray _postfix;
    StringHasher *_hasher = nullptr;
    mutable Flags _flags;
    mutable QVector<StringIDRef> _sids;
};

//////////////////////////////////////////////////////////////////////////

/** Counted reference to a StringID instance
 */
class StringIDRef
{
public:
    StringIDRef()
        :_sid(nullptr), _index(0)
    {}

    StringIDRef(StringID* p, int index=0)
        : _sid(p), _index(index)
    {
        if (_sid)
            _sid->ref();
    }

    StringIDRef(const StringIDRef & other)
        : _sid(other._sid)
        , _index(other._index)
    {
        if (_sid)
            _sid->ref();
    }

    StringIDRef(StringIDRef && other)
        : _sid(other._sid)
        , _index(other._index)
    {
        other._sid = nullptr;
    }

    StringIDRef(const StringIDRef & other, int index)
        : _sid(other._sid)
        , _index(index)
    {
        if (_sid)
            _sid->ref();
    }

    ~StringIDRef()
    {
        if (_sid)
            _sid->unref();
    }

    void reset(const StringIDRef & p = StringIDRef()) {
        *this = p;
    }

    void reset(const StringIDRef &p, int index) {
        *this = p;
        this->_index = index;
    }

    void swap(StringIDRef &p) {
        if(*this != p) {
            auto tmp = p;
            p = *this;
            *this = tmp;
        }
    }

    StringIDRef & operator=(StringID* p) {
        if (_sid == p)
            return *this;
        if (_sid)
            _sid->unref();
        _sid = p;
        if (_sid)
            _sid->ref();
        this->_index = 0;
        return *this;
    }

    StringIDRef & operator=(const StringIDRef & p) {
        if (_sid != p._sid) {
            if (_sid)
                _sid->unref();
            _sid = p._sid;
            if (_sid)
                _sid->ref();
        }
        this->_index = p._index;
        return *this;
    }

    StringIDRef & operator=(StringIDRef && p) {
        if (_sid != p._sid) {
            if (_sid)
                _sid->unref();
            _sid = p._sid;
            p._sid = nullptr;
        }
        this->_index = p._index;
        return *this;
    }

    bool operator<(const StringIDRef & p) const {
        if (!_sid)
            return true;
        if (!p._sid)
            return false;
        int res = _sid->compare(*p._sid);
        if (res < 0)
            return true;
        if (res > 0)
            return false;
        return _index < p._index;
    }

    bool operator==(const StringIDRef & p) const {
        return _sid == p._sid && _index == p._index;
    }

    bool operator!=(const StringIDRef & p) const {
        return _sid != p._sid || _index != p._index;
    }

    explicit operator bool() const {
        return _sid != nullptr;
    }

    int getRefCount(void) const {
        if (_sid)
            return _sid->getRefCount();
        return 0;
    }

    std::string toString() const {
        if (_sid)
            return _sid->toString(_index);
        return std::string();
    }

    std::string dataToText() const {
        if (_sid)
            return _sid->dataToText(_index);
        return std::string();
    }

    const char * constData() const {
        if (_sid) {
            assert(_index == 0);
            assert(_sid->postfix().isEmpty());
            return _sid->data().constData();
        }
        return "";
    }

    const StringID & deref() const {
        return *_sid;
    }

    long value() const {
        if (_sid)
            return _sid->value();
        return 0;
    }

    QVector<StringIDRef> relatedIDs() const {
        if (_sid)
            return _sid->relatedIDs();
        return QVector<StringIDRef>();
    }

    bool isBinary() const {
        if (_sid)
            return _sid->isBinary();
        return false;
    }

    bool isHashed() const {
        if (_sid)
            return _sid->isHashed();
        return false;
    }

    void toBytes(QByteArray &bytes) const {
        if (_sid)
            _sid->toBytes(bytes, _index);
    }

    PyObject *getPyObject(void) {
        if (_sid)
            return _sid->getPyObjectWithIndex(_index);
        Py_INCREF(Py_None);
        return Py_None;
    }

    void mark() const {
        if (_sid)
            _sid->mark();
    }

    bool isMarked() const {
        return _sid && _sid->isMarked();
    }

    bool isFromSameHasher(const StringHasherRef & hasher) const
    {
        return _sid && _sid->isFromSameHasher(hasher);
    }

    StringHasherRef getHasher() const
    {
        if (_sid)
            return _sid->getHasher();
        return StringHasherRef();
    }

    void setPersistent(bool enable)
    {
        if (_sid)
            _sid->setPersistent(enable);
    }

    friend class StringHasher;

private:
    StringID *_sid;
    int _index;
};

/// A String table to map string from/to a unique integer
class AppExport StringHasher: public Base::Persistence, public Base::Handled {

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    StringHasher();
    virtual ~StringHasher();

    virtual unsigned int getMemSize (void) const override;
    virtual void Save (Base::Writer &/*writer*/) const override;
    virtual void Restore(Base::XMLReader &/*reader*/) override;
    virtual void SaveDocFile (Base::Writer &/*writer*/) const override;
    virtual void RestoreDocFile (Base::Reader &/*reader*/) override;
    void setPersistenceFileName(const char *name) const;
    const std::string &getPersistenceFileName() const;

    /** Maps an arbitrary string to an integer
     *
     * @param text: input string.
     * @param len: length of the string, or -1 if the string is 0 terminated.
     * @param hashable: whether the string is hashable.
     * @return Return a shared pointer to the internally stored StringID.
     *
     * The function maps an arbitrary text string to a unique integer ID, which
     * is returned as a shared pointer to reference count the ID so that it is
     * possible to prune any unused strings.
     *
     * If \c hashable is true and the string is longer than the threshold
     * setting of this StringHasher, it will be sha1 hashed before storing, and
     * the original content of the string is discarded. If else, the string is
     * copied and stored inside a StringID instance.
     *
     * The purpose of function is to provide a short form of a stable string
     * identification.
     */
    StringIDRef getID(const char *text, int len=-1, bool hashable=false);

    /// Option for string string data
    enum class Option {
        /// No option
        None     = 0,
        /// The input data is binary
        Binary   = 1 << 0,
        /** The input data is hashable. If the data length is longer than the
         * threshold setting of the StringHasher, it will be sha1 hashed before
         * storing, and the original content of the string is discarded.
          */
        Hashable = 1 << 1,
        /// Do not copy the data, assuming the data is constant. If this option
        //is not set, the data will be copied before storing.
        NoCopy   = 1 << 2,
    };
    typedef Base::Flags<Option> Options;

    /** Map text or binary data to an integer
     *
     * @param data: input data.
     * @param options: options describing how to store the data. @sa Option.
     * @return Return a shared pointer to the internally stored StringID.
     *
     * The function maps an arbitrary text string to a unique integer ID, which
     * is returned as a shared pointer to reference count the ID so that it is
     * possible to prune any unused strings.
     *
     * The purpose of function is to provide a short form of a stable string
     * identification.
     */
    StringIDRef getID(const QByteArray & data, Options options=Option::Hashable);

    /** Map geometry element name to an integer */
    StringIDRef getID(const Data::MappedName & name,
                      const QVector<StringIDRef> & sids);

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
    StringIDRef getID(const StringID::IndexID &id) const {
        return getID(id.id, id.index);
    }

    std::map<long,StringIDRef> getIDMap() const;

    /// Clear all string hashes
    void clear();

    /// Size of the hash table
    size_t size() const;

    /// Return the number of hashes that are used by others
    size_t count() const;

    virtual PyObject *getPyObject(void) override;

    /** Enable/disable saving all string ID
     * 
     * If disabled, then only save string ID that are used.
     */
    void setSaveAll(bool enable);
    bool getSaveAll() const;

    /** Set threshold of string hashing
     *
     * For hashable string that are longer than the threshold, the string will
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

    /// Compact string storage
    void compact();

    class HashMap;
    friend class StringID;

protected:
    StringID * insert(const StringIDRef & sid);
    long lastID() const;
    void saveStream(std::ostream &s) const;
    void restoreStream(std::istream &s, std::size_t count);
    void restoreStreamNew(std::istream &s, std::size_t count);

private:
    std::unique_ptr<HashMap> _hashes;
    mutable std::string _filename;
};

}

ENABLE_BITMASK_OPERATORS(App::StringID::Flag)
ENABLE_BITMASK_OPERATORS(App::StringHasher::Option)

#endif
