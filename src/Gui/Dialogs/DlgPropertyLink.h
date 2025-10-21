// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef GUI_DIALOG_DLGPROPERTYLINK_H
#define GUI_DIALOG_DLGPROPERTYLINK_H

#include <QDialog>

#include "Dialogs/DlgDocumentObject.h"


 namespace Gui::Dialog {

class DlgPropertyLink : public DlgDocumentObject
{
    Q_OBJECT

public:
    explicit DlgPropertyLink(QWidget* parent = nullptr);

    QList<App::SubObjectT> currentLinks() const;
    QList<App::SubObjectT> originalLinks() const;

    void init(const App::DocumentObjectT &prop, bool tryFilter=true);

    static QString linksToPython(const QList<App::SubObjectT>& links);

    static QList<App::SubObjectT> getLinksFromProperty(const App::PropertyLinkBase *prop);

    static QString formatObject(App::Document *ownerDoc, App::DocumentObject *obj, const char *sub);

    static QString formatObject(App::Document *ownerDoc, const App::SubObjectT &sobj) {
        return formatObject(ownerDoc, sobj.getObject(), sobj.getSubName().c_str());
    }

    static QString formatLinks(App::Document *ownerDoc, QList<App::SubObjectT> links);

protected:
    void onClicked(QAbstractButton *button) override;

private:
    App::DocumentObjectT objProp;
    std::set<App::DocumentObject*> inList;
    QList<App::SubObjectT> oldLinks;
};

} // namespace Gui::Dialog

#endif // GUI_DIALOG_DLGPROPERTYLINK_H

