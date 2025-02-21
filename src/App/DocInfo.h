/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <boost/signals2/connection.hpp>


// Key on absolute path.
// Because of possible symbolic links, multiple entry may refer to the same
// file. We used to rely on QFileInfo::canonicalFilePath to resolve it, but
// has now been changed to simply use the absoluteFilePath(), and rely on user
// to be aware of possible duplicated file location. The reason being that
// some user (especially Linux user) use symlink to organize file tree.

class QString;

namespace App {
class DocInfo;
using DocInfoPtr = std::shared_ptr<DocInfo>;
using DocInfoMap = std::map<QString, DocInfoPtr>;

class Document;
class DocumentObject;
class PropertyXLink;

class DocInfo: public std::enable_shared_from_this<DocInfo>
{
public:
    using Connection = boost::signals2::scoped_connection;
    Connection connFinishRestoreDocument;
    Connection connPendingReloadDocument;
    Connection connDeleteDocument;
    Connection connSaveDocument;
    Connection connDeletedObject;

    DocInfoMap::iterator myPos;
    std::string myPath;
    App::Document* pcDoc {nullptr};
    std::set<PropertyXLink*> links;

    static DocInfoMap getMap();
    static std::string getDocPath(const char* filename,
                                  App::Document* pDoc,
                                  bool relative,
                                  QString* fullPath = nullptr);

    static DocInfoPtr
    get(const char* filename, App::Document* pDoc, PropertyXLink* l, const char* objName);

    static QString getFullPath(const char* p);
    QString getFullPath() const;
    const char* filePath() const
    {
        return myPath.c_str();
    }

    void deinit();
    void init(DocInfoMap::iterator pos, const char* objName, PropertyXLink* l);
    void attach(Document* doc);
    void remove(PropertyXLink* l)
    {
        auto it = links.find(l);
        if (it != links.end()) {
            links.erase(it);
            if (links.empty()) {
                deinit();
            }
        }
    }

    static void restoreDocument(const App::Document& doc);
    void slotFinishRestoreDocument(const App::Document& doc);

    void slotSaveDocument(const App::Document& doc);
    void slotDeleteDocument(const App::Document& doc);

    bool hasXLink(const App::Document* doc) const;

    static void breakLinks(App::DocumentObject* obj, bool clear);
};

}  // namespace App
