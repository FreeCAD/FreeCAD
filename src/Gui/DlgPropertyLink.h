/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGPROPERTYLINK_H
#define GUI_DIALOG_DLGPROPERTYLINK_H

#include <QDialog>
#include <QAbstractItemView>

#define FC_XLINK_VALUE_INDEX 5

namespace Gui { namespace Dialog {

class Ui_DlgPropertyLink;
class DlgPropertyLink : public QDialog
{
    Q_OBJECT

public:
    DlgPropertyLink(const QStringList& list, QWidget* parent = 0, Qt::WindowFlags fl = 0, bool xlink=false);
    ~DlgPropertyLink();

    void setSelectionMode(QAbstractItemView::SelectionMode mode);
    void accept();
    QStringList propertyLink() const;
    QVariantList propertyLinkList() const;

private Q_SLOTS:
    void on_checkObjectType_toggled(bool);
    void on_typeTree_itemSelectionChanged();
    void on_searchBox_textChanged(const QString&);
    void on_comboBox_currentIndexChanged(int);
    void onItemExpanded(QTreeWidgetItem * item);

private:
    QTreeWidgetItem *createItem(App::DocumentObject *obj, QTreeWidgetItem *parent);
    void findObjects();

private:
    QStringList link;
    Ui_DlgPropertyLink* ui;
    std::set<App::DocumentObject*> inList;
    std::set<std::string> types;
    bool refreshTypes = true;
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_DIALOG_DLGPROPERTYLINK_H

