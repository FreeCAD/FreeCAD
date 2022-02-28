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


#ifndef APP_MERGEDOCUMENTS_H
#define APP_MERGEDOCUMENTS_H

#include <Base/Persistence.h>

namespace zipios {
class ZipInputStream;
}

namespace App {
class Document;
class DocumentObject;
class AppExport MergeDocuments : public Base::Persistence
{
public:
    MergeDocuments(App::Document* doc);
    ~MergeDocuments();
    bool isVerbose() const { return verbose; }
    void setVerbose(bool on) { verbose = on; }
    unsigned int getMemSize (void) const;
    std::vector<App::DocumentObject*> importObjects(std::istream&);
    void importObject(const std::vector<App::DocumentObject*>& o, Base::XMLReader & r);
    void exportObject(const std::vector<App::DocumentObject*>& o, Base::Writer & w);
    void Save (Base::Writer & w) const;
    void Restore(Base::XMLReader &r);
    void SaveDocFile (Base::Writer & w) const;
    void RestoreDocFile(Base::Reader & r);

    const std::map<std::string, std::string> &getNameMap() const {return nameMap;}

private:
    bool guiup;
    bool verbose;
    zipios::ZipInputStream* stream;
    App::Document* appdoc;
    std::vector<App::DocumentObject*> objects;
    std::map<std::string, std::string> nameMap;
    typedef boost::signals2::connection Connection;
    Connection connectExport;
    Connection connectImport;
};

} // namespace App

#endif // APP_MERGEDOCUMENTS_H
