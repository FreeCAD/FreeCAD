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

#include <memory>
#include <bitset>

#include <QByteArray>
#include <QVector>

#include <CXX/Objects.hxx>
#include <Base/Handle.h>
#include <Base/Persistence.h>

namespace Data{
class MappedName;
}

namespace App {

class StringHasher;
class StringID;
class StringIDRef;
typedef Base::Reference<StringHasher> StringHasherRef;

class AppExport StringID: public Base::BaseClass, public Base::Handled {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    enum Flag {
        Binary,
        Hashed,
        PostfixEncoded,
        Postfixed,
        Indexed,
        PrefixID,
        PrefixIDIndex,
        Persistent,
        Marked,
    };
    StringID(long id, const QByteArray &data, bool binary, bool hashed)
        :_id(id),_data(data)
    {
        if(binary) _flags.set(Binary);
        if(hashed) _flags.set(Hashed);
    }

    StringID(long id, const QByteArray &data, uint8_t flags)
        :_id(id),_data(data),_flags(flags)
    {}

    StringID()
        :_id(0),_flags(0)
    {}

    virtual ~StringID();

    long value() const {return _id;}
    const QVector<StringIDRef> &relatedIDs() const {return _sids;}

    bool isBinary() const {return _flags.test(Binary);}
    bool isHashed() const {return _flags.test(Hashed);}
    bool isPostfixed() const {return _flags.test(Postfixed);}
    bool isPostfixEncoded() const {return _flags.test(PostfixEncoded);}
    bool isIndexed() const {return _flags.test(Indexed);}
    bool isPrefixID() const {return _flags.test(PrefixID);}
    bool isPrefixIDIndex() const {return _flags.test(PrefixIDIndex);}
    bool isMarked() const {return _flags.test(Marked);}
    bool isPersistent() const {return _flags.test(Persistent);}

    bool isFromSameHasher(const StringHasherRef & hasher) const
    {
        return this->_hasher == hasher;
    }

    StringHasherRef getHasher() const
    {
        return StringHasherRef(_hasher);
    }

    const QByteArray data() const {return _data;}
    const QByteArray postfix() const {return _postfix;}

    virtual PyObject *getPyObject() override;
    PyObject *getPyObjectWithIndex(int index);

    std::string toString(int index) const;

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
    static IndexID fromString(const char *name, bool eof=true, int size = -1);

    static IndexID fromString(const QByteArray &bytes, bool eof=true) {
        return fromString(bytes.constData(), eof, bytes.size());
    }

    std::string dataToText(int index) const;

    void toBytes(QByteArray &bytes, int index) const {
        if (_postfix.size())
            bytes = _data + _postfix;
        else if (index)
            bytes = _data + QByteArray::number(index);
        else
            bytes = _data;
    }

    void mark() const;

    void setPersistent(bool enable)
    {
        _flags.set(Persistent, enable);
    }

    bool operator<(const StringID &other) const {
        return compare(other) < 0;
    }

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
    mutable std::bitset<32> _flags;
    mutable QVector<StringIDRef> _sids;
};

//////////////////////////////////////////////////////////////////////////

class StringIDRef
{
public:
    StringIDRef()
        :_sid(0), _index(0)
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
     * The function maps an arbitrary text string to a unique integer ID, which
     * is returned as a shared pointer to reference count the ID so that it is
     * possible to prune any unused strings.
     *
     * If the string is longer than the threshold setting of this StringHasher,
     * it will be sha1 hashed before storing, and the original content of the
     * string is discarded.
     *
     * The purpose of function is to provide a short form of a stable string
     * identification.
     */
    StringIDRef getID(const char *text, int len=-1, bool hashable=false);

    /** Map text or binary data to an integer */
    StringIDRef getID(const QByteArray & data, bool binary, bool hashable=true, bool nocopy=false);

    /** Map geometry element name to an integer */
    StringIDRef getID(const Data::MappedName & name,
                      const QVector<StringIDRef> & sids);

    /** Obtain the reference counted StringID object from numerical id
     *
     * This function exists because the stored string may be one way hashed,
     * and the original text is not persistent. The caller use this function to
     * retrieve the reference count ID object after restore
     */
    StringIDRef getID(long id, int index = 0) const;

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

    void setSaveAll(bool enable);
    bool getSaveAll() const;

    void setThreshold(int threshold);
    int getThreshold() const;

    void clearMarks() const;

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

#endif
