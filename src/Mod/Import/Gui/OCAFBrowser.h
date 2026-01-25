// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <QIcon>
#include <TCollection_ExtendedString.hxx>
#include <TDF_IDList.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDocStd_Document.hxx>
#include <TNaming_NamedShape.hxx>
#include <string>

class QString;
class QTreeWidget;
class QTreeWidgetItem;

namespace ImportGui
{
class OCAFBrowser
{
public:
    explicit OCAFBrowser(const Handle(TDocStd_Document) & hDoc);
    void load(QTreeWidget*);

    static void showDialog(const QString& title, const Handle(TDocStd_Document) & hDoc);

private:
    void load(const TDF_Label& label, QTreeWidgetItem* item, const QString&);
    static std::string toString(const TCollection_ExtendedString& extstr);
    static QString toText(const Handle(TDataStd_TreeNode) & treeNode);
    static QString toText(const Handle(TNaming_NamedShape) & namedShape);

private:
    QIcon myGroupIcon;
    TDF_IDList myList;
    Handle(TDocStd_Document) pDoc;
};

}  // namespace ImportGui
