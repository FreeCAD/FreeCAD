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

#ifndef IMPORT_OCAFBROWSER_H
#define IMPORT_OCAFBROWSER_H

#include <QIcon>
#include <TCollection_ExtendedString.hxx>
#include <TDF_IDList.hxx>
#include <TDocStd_Document.hxx>
#include <string>

class QString;
class QTreeWidget;
class QTreeWidgetItem;

namespace ImportGui
{
class OCAFBrowser
{
public:
    explicit OCAFBrowser(Handle(TDocStd_Document) hDoc);
    void load(QTreeWidget*);

    static void showDialog(const QString& title, Handle(TDocStd_Document) hDoc);

private:
    void load(const TDF_Label& label, QTreeWidgetItem* item, const QString&);
    std::string toString(const TCollection_ExtendedString& extstr) const;

private:
    QIcon myGroupIcon;
    TDF_IDList myList;
    Handle(TDocStd_Document) pDoc;
};

}  // namespace ImportGui

#endif  // IMPORT_OCAFBROWSER_H
