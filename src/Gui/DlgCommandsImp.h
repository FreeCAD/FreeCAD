/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGCOMMANDS_IMP_H
#define GUI_DIALOG_DLGCOMMANDS_IMP_H

#include "PropertyPage.h"
#include <memory>

class QTreeWidgetItem;

namespace Gui {
class Command;
namespace Dialog {
class Ui_DlgCustomCommands;

/** Shows an overview of all available commands of all groups and modules.
 * You can customize your workbenches just by drag&dropping any commands
 * onto the toolbars or commandbars. But you cannot modify any standard toolbars or
 * commandbars such as "File, View, ...". It is only poosible to
 * customize your own toolbars or commandbars.
 * \author Werner Mayer
 */
class DlgCustomCommandsImp : public CustomizeActionPage
{
  Q_OBJECT

public:
    DlgCustomCommandsImp(QWidget* parent = nullptr);
    ~DlgCustomCommandsImp();

protected Q_SLOTS:
    void onGroupActivated(QTreeWidgetItem *i);
    void onDescription(QTreeWidgetItem *i);
    void onAddMacroAction(const QByteArray&);
    void onRemoveMacroAction(const QByteArray&);
    void onModifyMacroAction(const QByteArray&);

protected:
    void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui_DlgCustomCommands> ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGCOMMANDS_IMP_H
