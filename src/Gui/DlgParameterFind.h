/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGPARAMETERFIND_H
#define GUI_DIALOG_DLGPARAMETERFIND_H

#include <QDialog>
#include <Base/Parameter.h>

class QTreeWidgetItem;

namespace Gui {
namespace Dialog {

class Ui_DlgParameterFind;
class DlgParameterImp;

class GuiExport DlgParameterFind : public QDialog
{ 
    Q_OBJECT

public:
    DlgParameterFind(DlgParameterImp* parent);
    ~DlgParameterFind();

    void accept();
    void reject();

private Q_SLOTS:
    void on_lineEdit_textChanged(const QString&);
    void on_checkGroups_toggled(bool);
    void on_checkNames_toggled(bool);
    void on_checkValues_toggled(bool);

private:
    struct Options {
        QString text;
        bool group = true;
        bool name = true;
        bool value = true;
        bool match = false;
    };
    QTreeWidgetItem* findItem(QTreeWidgetItem* root, const Options& opt) const;
    bool matches(QTreeWidgetItem* item, const Options& opt) const;

private:
    Ui_DlgParameterFind* ui;
    DlgParameterImp* dialog;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGPARAMETERFIND_H
