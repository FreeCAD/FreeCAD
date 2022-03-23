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
namespace bp = boost::placeholders;

namespace App {

class XMLMergeReader : public Base::XMLReader
{
public:
    XMLMergeReader(std::map<std::string, std::string>& name, const char* FileName, std::istream& str)
      : Base::XMLReader(FileName, str), nameMap(name)
    {}

    void addName(const char* s1, const char* s2)
    {
        nameMap[s1] = s2;
    }
    const char* getName(const char* name) const
    {
        std::map<std::string, std::string>::const_iterator it = nameMap.find(name);
        if (it != nameMap.end())
            return it->second.c_str();
        else
            return name;
    }
    bool doNameMapping() const
    {
        return true;
    }
protected:

    // It is not safe to change potential object name reference at this level.
    // For example, a LinkSub with sub element name Face1 may also be some
    // object's name that may potentially be mapped. In addition, with the
    // introduction of full qualified SubName reference, the Sub value inside
    // LinkSub may require customized mapping. So we move the mapping logic to
    // various link property's Restore() function.
#if 0
    void startElement(const XMLCh* const uri, const XMLCh* const localname,
                      const XMLCh* const qname,
                      const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs)
    {
        Base::XMLReader::startElement(uri, localname, qname, attrs);
        if (LocalName == "Property")
            propertyStack.push(std::make_pair(AttrMap["name"],AttrMap["type"]));

        if (!propertyStack.empty()) {
            // replace the stored object name with the real one
            if (LocalName == "Link" || LocalName == "LinkSub" || (LocalName == "String" && propertyStack.top().first == "Label")) {
                for (std::map<std::string, std::string>::iterator it = AttrMap.begin(); it != AttrMap.end(); ++it) {
                    std::map<std::string, std::string>::const_iterator jt = nameMap.find(it->second);
                    if (jt != nameMap.end())
                        it->second = jt->second;
                }
            }
            // update the expression if name of the object is used
            else if (LocalName == "Expression") {
                std::map<std::string, std::string>::iterator it = AttrMap.find("expression");
                if (it != AttrMap.end()) {
                    // search for the part before the first dot that should be the object name.
                    std::string expression = it->second;
                    std::string::size_type dotpos = expression.find_first_of(".");
                    if (dotpos != std::string::npos) {
                        std::string name = expression.substr(0, dotpos);
                        std::map<std::string, std::string>::const_iterator jt = nameMap.find(name);
                        if (jt != nameMap.end()) {
                            std::string newexpression = jt->second;
                            newexpression += expression.substr(dotpos);
                            it->second = newexpression;
                        }
                    }
                }
            }
        }
    }

    void endElement(const XMLCh* const uri, const XMLCh *const localname, const XMLCh *const qname)
    {
        Base::XMLReader::endElement(uri, localname, qname);
        if (LocalName == "Property")
            propertyStack.pop();
    }
#endif

private:
    std::map<std::string, std::string>& nameMap;
    typedef std::pair<std::string, std::string> PropertyTag;
    std::stack<PropertyTag> propertyStack;
};
}

MergeDocuments::MergeDocuments(App::Document* doc) : guiup(false), verbose(true), stream(nullptr), appdoc(doc)
{
    connectExport = doc->signalExportObjects.connect
        (boost::bind(&MergeDocuments::exportObject, this, bp::_1, bp::_2));
    connectImport = doc->signalImportObjects.connect
        (boost::bind(&MergeDocuments::importObject, this, bp::_1, bp::_2));

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

unsigned int MergeDocuments::getMemSize (void) const
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
