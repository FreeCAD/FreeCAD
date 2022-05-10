/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGKEYBOARD_IMP_H
#define GUI_DIALOG_DLGKEYBOARD_IMP_H

#include "PropertyPage.h"
#include <memory>

class QTreeWidgetItem;

namespace Gui {
namespace Dialog {
class Ui_DlgCustomKeyboard;

/** Shows an overview of all available commands of all groups and modules.
 * You can customize your workbenches just by drag&dropping any commands
 * onto the toolbars or commandbars. But you cannot modify any standard toolbars or
 * commandbars such as "File, View, ...". It is only possible to
 * customize your own toolbars or commandbars.
 * \author Werner Mayer
 */
class DlgCustomKeyboardImp : public CustomizeActionPage
{
    Q_OBJECT

public:
    DlgCustomKeyboardImp( QWidget* parent = nullptr );
    ~DlgCustomKeyboardImp();

protected:
    void showEvent(QShowEvent* e);

protected Q_SLOTS:
    void on_categoryBox_activated(int index);
    void on_commandTreeWidget_currentItemChanged(QTreeWidgetItem*);
    void on_buttonAssign_clicked();
    void on_buttonClear_clicked();
    void on_buttonReset_clicked();
    void on_buttonResetAll_clicked();
    void on_editShortcut_textChanged(const QString&);
    void onAddMacroAction(const QByteArray&);
    void onRemoveMacroAction(const QByteArray&);
    void onModifyMacroAction(const QByteArray&);

protected:
    void changeEvent(QEvent *e);
    void setShortcutOfCurrentAction(const QString&);

private:
    std::unique_ptr<Ui_DlgCustomKeyboard> ui;
    bool firstShow;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGKEYBOARD_IMP_H
