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

#include <boost/signals2/connection.hpp>
#include <memory>
#include <QPointer>
#include <QAction>
#include "PropertyPage.h"

class QTreeWidget;
class QTreeWidgetItem;
class QComboBox;
class QLineEdit;
class QAbstractButton;

namespace Gui {

class AccelLineEdit;

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
    explicit DlgCustomKeyboardImp( QWidget* parent = nullptr );
    ~DlgCustomKeyboardImp() override;

    /** Public helper function for handling command widgets
     *
     * @param commandTreeWidget: a tree widget listing commands
     * @param separatorItem: optional separator item
     * @param comboGroups: a combo box widget for choosing categories of commands
     * @param editCommand: a line edit for searching command with auto complete
     * @param priroityList: a tree widget listing commands with the same shortcut in order of priority
     * @param buttonUp: a button widget to increase command priority
     * @param buttonDown: a button widget to decrease command priority
     * @param editShortcut: an accelerator editor for setting user defined shortcut
     * @param currentShortcut: optional accelerator editor showing the current shortcut of a command
     *
     * @return Return a boost signal connection for monitoring command changes.
     * Most disconnect the signal before widgets gets destroyed.
     */
    static boost::signals2::connection
        initCommandWidgets(QTreeWidget *commandTreeWidget,
                           QTreeWidgetItem *separatorItem,
                           QComboBox *comboGroups,
                           QLineEdit *editCommand,
                           QTreeWidget *priorityList = nullptr,
                           QAbstractButton *buttonUp = nullptr,
                           QAbstractButton *buttonDown = nullptr,
                           AccelLineEdit *editShortcut = nullptr,
                           AccelLineEdit *currentShortcut = nullptr);

protected:
    void showEvent(QShowEvent* e) override;

    /** @name Internal helper function for handling command list widgets
     */
    //@{
    static void initCommandCompleter(QLineEdit *,
                                     QComboBox *combo,
                                     QTreeWidget *treeWidget,
                                     QTreeWidgetItem *separatorItem);
    static boost::signals2::connection initCommandList(QTreeWidget *, QTreeWidgetItem *, QComboBox *combo);
    static void initPriorityList(QTreeWidget *, QAbstractButton *buttonUp, QAbstractButton *buttonDown);
    static void populateCommandGroups(QComboBox *);
    static void populateCommandList(QTreeWidget *, QTreeWidgetItem *, QComboBox *);
    static void populatePriorityList(QTreeWidget *priorityList,
                                     AccelLineEdit *editor,
                                     AccelLineEdit *current);
    //@}
protected Q_SLOTS:
    void on_categoryBox_activated(int index);
    void on_commandTreeWidget_currentItemChanged(QTreeWidgetItem*);
    void on_buttonAssign_clicked();
    void on_buttonClear_clicked();
    void on_buttonReset_clicked();
    void on_buttonResetAll_clicked();
    void on_editShortcut_textChanged(const QString&);
    void onAddMacroAction(const QByteArray&) override;
    void onRemoveMacroAction(const QByteArray&) override;
    void onModifyMacroAction(const QByteArray&) override;

protected:
    void changeEvent(QEvent *e) override;
    void setShortcutOfCurrentAction(const QString&);

private:
    std::unique_ptr<Ui_DlgCustomKeyboard> ui;
    bool firstShow;
    boost::signals2::scoped_connection conn;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGKEYBOARD_IMP_H
