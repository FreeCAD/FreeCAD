/***************************************************************************
*   (c) Jürgen Riegel (juergen.riegel@web.de) 2014                        *
*                                                                         *
*   This file is part of the FreeCAD CAx development system.              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU Library General Public License (LGPL)   *
*   as published by the Free Software Foundation; either version 2 of     *
*   the License, or (at your option) any later version.                   *
*   for detail see the LICENCE text file.                                 *
*                                                                         *
*   FreeCAD is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU Library General Public License for more details.                  *
*                                                                         *
*   You should have received a copy of the GNU Library General Public     *
*   License along with FreeCAD; if not, write to the Free Software        *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
*   USA                                                                   *
*                                                                         *
*   Juergen Riegel 2014                                                   *
***************************************************************************/


#ifndef GUI_ActiveObjectList_H
#define GUI_ActiveObjectList_H

#include <map>
#include "Tree.h"
namespace App {
    class DocumentObject;
}

namespace Gui
{
    class Document;
    class ViewProviderDocumentObject;

	/** List of active or special objects
	* This class holds a list of objects with a special name.
	* Its mainly used to points to something like the active Body or Part in a edit session.
	* The class is used the viewer (editor) of a document.
	* @see Gui::MDIViewer
	* @author Jürgen Riegel
	*/
    class GuiExport ActiveObjectList 
    {
    public:
        ActiveObjectList(Document *doc)
            :_Doc(doc)
        {}

        template<typename _T>
        inline _T getObject(const char* name, App::DocumentObject **parent=0, std::string *subname=0) const {
            auto it = _ObjectMap.find(name);
            if(it==_ObjectMap.end())
                return 0;
            return dynamic_cast<_T>(getObject(it->second,true,parent,subname));
        }
        void setObject(App::DocumentObject*, const char*, const char *subname=0,
                const Gui::HighlightMode& m = HighlightMode::UserDefined);
        bool hasObject(const char*)const;
        void objectDeleted(const ViewProviderDocumentObject& viewProviderIn);
        bool hasObject(App::DocumentObject *obj, const char *, const char *subname=0) const;

    private:
        struct ObjectInfo;
        void setHighlight(const ObjectInfo &info, Gui::HighlightMode mode, bool enable);
        App::DocumentObject *getObject(const ObjectInfo &info, bool resolve,
                App::DocumentObject **parent=0, std::string *subname=0) const;
        ObjectInfo getObjectInfo(App::DocumentObject *obj, const char *subname) const;

    private:
        struct ObjectInfo {
            App::DocumentObject *obj;
            std::string subname;
        };
        std::map<std::string, ObjectInfo> _ObjectMap;
        Document *_Doc;
    };

} //namespace Gui

static const char PDBODYKEY[] = "pdbody";
static const char PARTKEY[] = "part";

#endif 
