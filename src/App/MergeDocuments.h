// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Base/Persistence.h>
#include <fastsignals/signal.h>

namespace zipios
{
class ZipInputStream;
}

namespace App
{
class Document;
class DocumentObject;
class AppExport MergeDocuments: public Base::Persistence
{
public:
    explicit MergeDocuments(App::Document* doc);
    ~MergeDocuments() override;
    bool isVerbose() const
    {
        return verbose;
    }
    void setVerbose(bool on)
    {
        verbose = on;
    }
    unsigned int getMemSize() const override;
    std::vector<App::DocumentObject*> importObjects(std::istream&);
    void importObject(const std::vector<App::DocumentObject*>& o, Base::XMLReader& r);
    void exportObject(const std::vector<App::DocumentObject*>& o, Base::Writer& w);
    void Save(Base::Writer& w) const override;
    void Restore(Base::XMLReader& r) override;
    void SaveDocFile(Base::Writer& w) const override;
    void RestoreDocFile(Base::Reader& r) override;

    const std::map<std::string, std::string>& getNameMap() const
    {
        return nameMap;
    }

private:
    bool guiup {false};
    bool verbose {true};
    zipios::ZipInputStream* stream {nullptr};
    App::Document* appdoc {nullptr};
    std::vector<App::DocumentObject*> objects;
    std::map<std::string, std::string> nameMap;
    using Connection = fastsignals::connection;
    Connection connectExport;
    Connection connectImport;
};

}  // namespace App
