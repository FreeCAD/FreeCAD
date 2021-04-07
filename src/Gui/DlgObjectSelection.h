/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/
#ifndef GUI_DLGOBJECTSELECTION_H
#define GUI_DLGOBJECTSELECTION_H

#include <QDialog>

namespace Gui {

class Ui_DlgObjectSelection;
class GuiExport DlgObjectSelection : public QDialog
{
    Q_OBJECT

public:
    DlgObjectSelection(const std::vector<App::DocumentObject*> &objs,
            QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgObjectSelection();

    std::vector<App::DocumentObject*> getSelections() const;
    void accept();
    void reject();

private Q_SLOTS:
    void onItemExpanded(QTreeWidgetItem * item);
    void onItemChanged(QTreeWidgetItem * item, int);
    void onItemSelectionChanged();
    void onDepSelectionChanged();

private:
    QTreeWidgetItem *createItem(App::DocumentObject *obj, QTreeWidgetItem *parent);
    App::DocumentObject *objFromItem(QTreeWidgetItem *item);

private:
    struct Info {
        std::map<App::DocumentObject *, Info*> inList;
        std::map<App::DocumentObject *, Info*> outList;
        std::vector<QTreeWidgetItem*> items;
        QTreeWidgetItem *depItem = 0;
        Qt::CheckState checkState = Qt::Checked;
    };
    std::map<App::DocumentObject *,Info> objMap;
    Ui_DlgObjectSelection* ui;
    std::set<App::DocumentObject*> sels;
};

} // namespace Gui


#endif // GUI_DLGOBJECTSELECTION_H

