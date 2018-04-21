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
#include <QByteArray>
#include <CXX/Objects.hxx>
#include <Base/Handle.h>
#include <Base/Persistence.h>

namespace App {

class StringID;
typedef Base::Reference<StringID> StringIDRef;

class AppExport StringID: public Base::BaseClass, public Base::Handled {
    TYPESYSTEM_HEADER();
public:
    StringID(long id, const QByteArray &data, bool binary, bool hashed)
        :_id(id),_data(data),_binary(binary),_hashed(hashed)
    {}
    virtual ~StringID(){}
    long value() const {return _id;}
    const QByteArray &data() const {return _data;}
    bool isBinary() const {return _binary;}
    bool isHashed() const {return _hashed;}
    virtual PyObject *getPyObject() override;
    std::string toString() const;
    static long fromString(const char *name, bool eof=true);
    static StringIDRef getNullID();
    bool isNull() const;

    std::string dataToText() const;
private:
    long _id;
    QByteArray _data;
    bool _binary;
    bool _hashed;
};

class AppExport StringHasher: public Base::Persistence, public Base::Handled {

    TYPESYSTEM_HEADER();

public:
    StringHasher();
    virtual ~StringHasher();

    virtual unsigned int getMemSize (void) const override;
    virtual void Save (Base::Writer &/*writer*/) const override;
    virtual void Restore(Base::XMLReader &/*reader*/) override;

    /** Maps an arbitary string to an integer
     *
     * These function internally hashes the string, and stroes the hash in a
     * map to integer. The hashes of the strings passed to this function are
     * persisted, which means the returned ID is an unique identifier of the
     * string. The function return the interger as a shared pointer to
     * reference count the ID so that it is possible to prune any unused hash
     *
     * The purpose of function is to provide a short form of a stable string
     * hash.
     */
    StringIDRef getID(const char *text, int len=-1);

    /** Map text or binary data to an integer */
    StringIDRef getID(QByteArray data, bool binary);

    /** Obtain the reference counted StringID object from numerical id
     *
     * This function exists because the string hash is a one way function, and
     * the original text is not persistent. The caller use this function to
     * retieve the reference count ID object after restore
     */
    StringIDRef getID(long id) const;

    std::map<long,StringIDRef> getIDMap() const;

    /// Clear all string hashes
    void clear();

    /// Size of the hash table
    size_t size() const;

    /// Return the number of hashes that are used by others
    size_t count() const;

    virtual PyObject *getPyObject(void);

    void setSaveAll(bool enable);
    bool getSaveAll() const;

    void setThreshold(int threshold);
    int getThreshold() const;

    class HashMap;

private:
    long lastID() const;

private:
    std::unique_ptr<HashMap> _hashes;
};

typedef Base::Reference<StringHasher> StringHasherRef;

}

#endif
