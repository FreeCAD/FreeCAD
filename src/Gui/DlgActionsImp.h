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


#ifndef GUI_DIALOG_DLGACTIONS_IMP_H
#define GUI_DIALOG_DLGACTIONS_IMP_H

#include <memory>
#include "PropertyPage.h"
#include <QDialog>
#include <QList>
#include <QPair>

class QListWidgetItem;
class QTreeWidgetItem;
class QLabel;
class QGridLayout;
class QLineEdit;

namespace Gui {
namespace Dialog {

class Ui_DlgCustomActions;

/** This class implements the creation of user defined actions executing a recorded macro.
 * It is possible to use these actions in own toolbars or commandbars.
 * @see Gui::Action
 * @see MacroCommand
 * @see Command
 * \author Werner Mayer
 */
class DlgCustomActionsImp : public CustomizeActionPage
{
    Q_OBJECT

public:
    DlgCustomActionsImp( QWidget* parent = 0 );
    ~DlgCustomActionsImp();

Q_SIGNALS:
    void addMacroAction( const QByteArray& );
    void removeMacroAction( const QByteArray& );
    void modifyMacroAction( const QByteArray& );

protected:
    /** Trigger for reparent event. */
    bool event(QEvent* e);
    void showEvent(QShowEvent* e);
    void changeEvent(QEvent *e);

protected Q_SLOTS:
    /** Enables/disables buttons for deletion */
    void on_actionListWidget_itemActivated( QTreeWidgetItem *i );
    /** Opens a iconview to select a pixmap */
    void on_buttonChoosePixmap_clicked();
    /** Adds a custom action */
    void on_buttonAddAction_clicked();
    /** Deletes a custom action */
    void on_buttonRemoveAction_clicked();
    /** Shows the setup of the action */
    void on_buttonReplaceAction_clicked();
    void onAddMacroAction(const QByteArray&);
    void onRemoveMacroAction(const QByteArray&);
    void onModifyMacroAction(const QByteArray&);

private:
    /** Shows all actions and their pixmaps if available  */
    void showActions();
    /** Name for the new created action */
    QString newActionName();

private:
    std::unique_ptr<Ui_DlgCustomActions> ui;
    bool bShown; /**< For internal use only*/
    QString m_sPixmap; /**< Name of the specified pixmap */
};

class Ui_DlgChooseIcon;
class IconDialog : public QDialog
{
    Q_OBJECT

public:
    IconDialog(QWidget* parent);
    ~IconDialog();
    void resizeEvent(QResizeEvent*);
    QListWidgetItem* currentItem() const;

private Q_SLOTS:
    void onAddIconPath();

private:
    Ui_DlgChooseIcon *ui;
};

class IconFolders : public QDialog
{
    Q_OBJECT

public:
    IconFolders(const QStringList&, QWidget* parent);
    ~IconFolders();
    QStringList getPaths() const;

private Q_SLOTS:
    void addFolder();
    void removeFolder();

private:
    bool restart;
    int maxLines;
    QGridLayout* gridLayout;
    QLabel* textLabel;
    QPushButton* addButton;
    QList< QPair<QLineEdit*, QPushButton*> > buttonMap;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGACTIONS_IMP_H
