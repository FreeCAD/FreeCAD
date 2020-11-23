/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
#include <CXX/Objects.hxx>
#include <Base/Handle.h>
#include <Base/Persistence.h>

namespace App {

class StringHasher;
class StringID;
typedef Base::Reference<StringID> StringIDRef;

class AppExport StringID: public Base::BaseClass, public Base::Handled {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    enum Flag {
        Binary,
        Hashed,
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

    virtual ~StringID(){}

    long value() const {return _id;}
    const QByteArray &data() const {return _data;}

    bool isBinary() const {return _flags.test(Binary);}
    bool isHashed() const {return _flags.test(Hashed);}

    virtual PyObject *getPyObject() override;
    std::string toString() const;
    static long fromString(const char *name, bool eof=true);
    static StringIDRef getNullID();
    bool isNull() const;

    std::string dataToText() const;

    friend class StringHasher;

private:
    long _id;
    QByteArray _data;
    std::string _cache;
    std::bitset<8> _flags;
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
    StringIDRef getID(QByteArray data, bool binary, bool hashable=true);

    /** Obtain the reference counted StringID object from numerical id
     *
     * This function exists because the stored string may be one way hashed,
     * and the original text is not persistent. The caller use this function to
     * retrieve the reference count ID object after restore
     */
    StringIDRef getID(long id) const;

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

    class HashMap;

private:
    long lastID() const;
    void saveStream(std::ostream &s) const;
    void restoreStream(std::istream &s, std::size_t count);

private:
    std::unique_ptr<HashMap> _hashes;
    mutable std::string _filename;
};

typedef Base::Reference<StringHasher> StringHasherRef;

}

#endif
