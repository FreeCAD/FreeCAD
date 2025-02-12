/****************************************************************************
 *   Copyright (c) 2019 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#ifndef GUI_DIALOG_DLGADDPROPERTY_H
#define GUI_DIALOG_DLGADDPROPERTY_H

#include <unordered_set>
#include <QDialog>
#include <FCGlobal.h>

namespace App {
class PropertyContainer;
}

namespace Gui {
namespace Dialog {

class Ui_DlgAddProperty;
class GuiExport DlgAddProperty : public QDialog
{
    Q_OBJECT

public:
    DlgAddProperty(QWidget *parent, std::unordered_set<App::PropertyContainer*> &&);
    ~DlgAddProperty() override;

    void accept() override;

private:
    std::unordered_set<App::PropertyContainer*> containers;
    std::unique_ptr<Ui_DlgAddProperty> ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGADDPROPERTY_H
