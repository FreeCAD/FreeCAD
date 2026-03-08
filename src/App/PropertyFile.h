// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#pragma once

#include <string>

#include "PropertyStandard.h"


namespace Base
{
class Writer;
}

namespace App
{

/** File properties
 * This property holds a file name
 */
class AppExport PropertyFile: public PropertyString
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyFile();
    ~PropertyFile() override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyFileItem";
    }

    void setPyObject(PyObject*) override;
    virtual void setFilter(const std::string filter);
    virtual std::string getFilter() const;

private:
    std::string m_filter;
};

/** File include properties
 * This property doesn't only save the file name like PropertyFile
 * it also includes the file itself into the document. The file
 * doesn't get loaded into memory, it gets copied from the document
 * archive into the document transient directory. There it is accessible for
 * the algorithms. You get the transient path through getDocTransientPath()
 * It's allowed to read the file, it's not allowed to write the file
 * directly in the transient path! That would undermine the Undo/Redo
 * framework. It's only allowed to use setValue() to change the file.
 * If you give a file name outside the transient dir to setValue() it
 * will copy the file. If you give a file name in the transient path it
 * will just rename and use the same file. You can use getExchangeTempFile() to
 * get a file name in the transient dir to write a new file version.
 */
class AppExport PropertyFileIncluded: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyFileIncluded();
    ~PropertyFileIncluded() override;

    void setValue(const char* sFile, const char* sName = nullptr);
    const char* getValue() const;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyTransientFileItem";
    }
    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && _BaseFileName == static_cast<decltype(this)>(&other)->_BaseFileName
            && _OriginalName == static_cast<decltype(this)>(&other)->_OriginalName
            && _cValue == static_cast<decltype(this)>(&other)->_cValue;
    }

    /** get a temp file name in the transient path of the document.
     * Using this file for new Version of the file and set
     * this file with setValue() is the fastest way to change
     * the File.
     */
    std::string getExchangeTempFile() const;
    std::string getOriginalFileName() const;

    bool isEmpty() const
    {
        return _cValue.empty();
    }

    void setFilter(std::string filter);
    std::string getFilter() const;

protected:
    // get the transient path if the property is in a DocumentObject
    std::string getDocTransientPath() const;
    std::string getUniqueFileName(const std::string&, const std::string&) const;
    void aboutToSetValue() override;

protected:
    mutable std::string _cValue;
    mutable std::string _BaseFileName;
    mutable std::string _OriginalName;

private:
    std::string m_filter;
};


}  // namespace App
