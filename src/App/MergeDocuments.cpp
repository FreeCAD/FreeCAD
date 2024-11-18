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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <stack>
#endif

#include <QCoreApplication>
#include <App/Document.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "MergeDocuments.h"


using namespace App;
namespace sp = std::placeholders;

namespace App {

class XMLMergeReader : public Base::XMLReader
{
public:
    XMLMergeReader(std::map<std::string, std::string>& name, const char* FileName, std::istream& str)
      : Base::XMLReader(FileName, str), nameMap(name)
    {}

    void addName(const char* s1, const char* s2) override
    {
        nameMap[s1] = s2;
    }
    const char* getName(const char* name) const override
    {
        std::map<std::string, std::string>::const_iterator it = nameMap.find(name);
        if (it != nameMap.end())
            return it->second.c_str();
        else
            return name;
    }
    bool doNameMapping() const override
    {
        return true;
    }
protected:


private:
    std::map<std::string, std::string>& nameMap;
    using PropertyTag = std::pair<std::string, std::string>;
    std::stack<PropertyTag> propertyStack;
};
}

MergeDocuments::MergeDocuments(App::Document* doc)
    : appdoc(doc)
{
    //NOLINTBEGIN
    connectExport = doc->signalExportObjects.connect
        (std::bind(&MergeDocuments::exportObject, this, sp::_1, sp::_2));
    connectImport = doc->signalImportObjects.connect
        (std::bind(&MergeDocuments::importObject, this, sp::_1, sp::_2));
    //NOLINTEND

    QCoreApplication* app = QCoreApplication::instance();
    if (app && app->inherits("QApplication")) {
        guiup = true;
    }
}

MergeDocuments::~MergeDocuments()
{
    connectExport.disconnect();
    connectImport.disconnect();
}

unsigned int MergeDocuments::getMemSize () const
{
    return 0;
}

std::vector<App::DocumentObject*>
MergeDocuments::importObjects(std::istream& input)
{
    this->nameMap.clear();
    this->stream = new zipios::ZipInputStream(input);
    XMLMergeReader reader(this->nameMap,"<memory>", *stream);
    reader.setVerbose(isVerbose());
    std::vector<App::DocumentObject*> objs = appdoc->importObjects(reader);

    delete this->stream;
    this->stream = nullptr;

    return objs;
}

void MergeDocuments::importObject(const std::vector<App::DocumentObject*>& o, Base::XMLReader & r)
{
    objects = o;
    Restore(r);
    r.readFiles(*this->stream);
}

void MergeDocuments::exportObject(const std::vector<App::DocumentObject*>& o, Base::Writer & w)
{
    objects = o;
    Save(w);
}

void MergeDocuments::Save (Base::Writer & w) const
{
    // Save view provider stuff
    if (guiup) {
        w.addFile("GuiDocument.xml", this);
    }
}

void MergeDocuments::Restore(Base::XMLReader &r)
{
    // Restore view provider stuff
    if (guiup) {
        r.addFile("GuiDocument.xml", this);
    }
}

void MergeDocuments::SaveDocFile (Base::Writer & w) const
{
    // Save view provider stuff
    appdoc->signalExportViewObjects(this->objects, w);
}

void MergeDocuments::RestoreDocFile(Base::Reader & r)
{
    // Restore view provider stuff
    appdoc->signalImportViewObjects(this->objects, r, this->nameMap);
}
