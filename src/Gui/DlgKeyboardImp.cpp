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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QAction>
# include <QHeaderView>
# include <QMessageBox>
#endif

#include <Base/Parameter.h>

#include "DlgKeyboardImp.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Widgets.h"
#include "Window.h"

using namespace Gui::Dialog;

namespace Gui { namespace Dialog {
typedef std::vector< std::pair<QLatin1String, QString> > GroupMap;

struct GroupMap_find {
    const QLatin1String& item;
    GroupMap_find(const QLatin1String& item) : item(item) {}
    bool operator () (const std::pair<QLatin1String, QString>& elem) const
    {
        return elem.first == item;
    }
};
}
}

/* TRANSLATOR Gui::Dialog::DlgCustomKeyboardImp */

/**
 *  Constructs a DlgCustomKeyboardImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomKeyboardImp::DlgCustomKeyboardImp( QWidget* parent  )
  : CustomizeActionPage(parent), firstShow(true)
{
    this->setupUi(this);

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    std::map<std::string,Command*> sCommands = cCmdMgr.getCommands();

    GroupMap groupMap;
    groupMap.push_back(std::make_pair(QLatin1String("File"), QString()));
    groupMap.push_back(std::make_pair(QLatin1String("Edit"), QString()));
    groupMap.push_back(std::make_pair(QLatin1String("View"), QString()));
    groupMap.push_back(std::make_pair(QLatin1String("Standard-View"), QString()));
    groupMap.push_back(std::make_pair(QLatin1String("Tools"), QString()));
    groupMap.push_back(std::make_pair(QLatin1String("Window"), QString()));
    groupMap.push_back(std::make_pair(QLatin1String("Help"), QString()));
    groupMap.push_back(std::make_pair(QLatin1String("Macros"), qApp->translate("Gui::MacroCommand", "Macros")));

    for (std::map<std::string,Command*>::iterator it = sCommands.begin(); it != sCommands.end(); ++it) {
        QLatin1String group(it->second->getGroupName());
        QString text = qApp->translate(it->second->className(), it->second->getGroupName());
        GroupMap::iterator jt;
        jt = std::find_if(groupMap.begin(), groupMap.end(), GroupMap_find(group));
        if (jt != groupMap.end()) {
            if (jt->second.isEmpty())
                jt->second = text;
        }
        else {
            groupMap.push_back(std::make_pair(group, text));
        }
    }

    int index = 0;
    for (GroupMap::iterator it = groupMap.begin(); it != groupMap.end(); ++it, ++index) {
        categoryBox->addItem(it->second);
        categoryBox->setItemData(index, QVariant(it->first), Qt::UserRole);
    }

    QStringList labels; 
    labels << tr("Icon") << tr("Command");
    commandTreeWidget->setHeaderLabels(labels);
    commandTreeWidget->header()->hide();
    commandTreeWidget->setIconSize(QSize(32, 32));
#if QT_VERSION >= 0x050000
    commandTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
#else
    commandTreeWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);
#endif

    assignedTreeWidget->setHeaderLabels(labels);
    assignedTreeWidget->header()->hide();
}

/** Destroys the object and frees any allocated resources */
DlgCustomKeyboardImp::~DlgCustomKeyboardImp()
{
}

void DlgCustomKeyboardImp::showEvent(QShowEvent* e)
{
    Q_UNUSED(e); 
    // If we did this already in the constructor we wouldn't get the vertical scrollbar if needed.
    // The problem was noticed with Qt 4.1.4 but may arise with any later version.
    if (firstShow) {
        on_categoryBox_activated(categoryBox->currentIndex());
        firstShow = false;
    }
}

/** Shows the description for the corresponding command */
void DlgCustomKeyboardImp::on_commandTreeWidget_currentItemChanged(QTreeWidgetItem* item)
{
    if (!item)
        return;

    QVariant data = item->data(1, Qt::UserRole);
    QByteArray name = data.toByteArray(); // command name

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    Command* cmd = cCmdMgr.getCommandByName(name.constData());
    if (cmd) {
        if (cmd->getAction()) {
            QKeySequence ks = cmd->getAction()->shortcut();
            QKeySequence ks2 = QString::fromLatin1(cmd->getAccel());
            QKeySequence ks3 = editShortcut->text();

            if (ks.isEmpty())
                accelLineEditShortcut->setText( tr("none") );
            else
                accelLineEditShortcut->setText(ks.toString(QKeySequence::NativeText));

            buttonAssign->setEnabled(!editShortcut->text().isEmpty() && (ks != ks3));
            buttonReset->setEnabled((ks != ks2));
        } else {
          QKeySequence ks = QString::fromLatin1(cmd->getAccel());
            if (ks.isEmpty())
                accelLineEditShortcut->setText( tr("none") );
            else
                accelLineEditShortcut->setText(ks.toString(QKeySequence::NativeText));
            buttonAssign->setEnabled(false);
            buttonReset->setEnabled(false);
        }
    }

    textLabelDescription->setText(item->toolTip(1));
}

/** Shows all commands of this category */
void DlgCustomKeyboardImp::on_categoryBox_activated(int index)
{
    QVariant data = categoryBox->itemData(index, Qt::UserRole);
    QString group = data.toString();
    commandTreeWidget->clear();
    buttonAssign->setEnabled(false);
    buttonReset->setEnabled(false);
    accelLineEditShortcut->clear();
    editShortcut->clear();

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    std::vector<Command*> aCmds = cCmdMgr.getGroupCommands( group.toLatin1() );

    if (group == QLatin1String("Macros")) {
        for (std::vector<Command*>::iterator it = aCmds.begin(); it != aCmds.end(); ++it) {
            QTreeWidgetItem* item = new QTreeWidgetItem(commandTreeWidget);
            item->setText(1, QString::fromUtf8((*it)->getMenuText()));
            item->setToolTip(1, QString::fromUtf8((*it)->getToolTipText()));
            item->setData(1, Qt::UserRole, QByteArray((*it)->getName()));
            item->setSizeHint(0, QSize(32, 32));
            if ((*it)->getPixmap())
                item->setIcon(0, BitmapFactory().iconFromTheme((*it)->getPixmap()));
        }
    }
    else {
        for (std::vector<Command*>::iterator it = aCmds.begin(); it != aCmds.end(); ++it) {
            QTreeWidgetItem* item = new QTreeWidgetItem(commandTreeWidget);
            item->setText(1, qApp->translate((*it)->className(), (*it)->getMenuText()));
            item->setToolTip(1, qApp->translate((*it)->className(), (*it)->getToolTipText()));
            item->setData(1, Qt::UserRole, QByteArray((*it)->getName()));
            item->setSizeHint(0, QSize(32, 32));
            if ((*it)->getPixmap())
                item->setIcon(0, BitmapFactory().iconFromTheme((*it)->getPixmap()));
        }
    }
}

/** Assigns a new accelerator to the selected command. */
void DlgCustomKeyboardImp::on_buttonAssign_clicked()
{
    QTreeWidgetItem* item = commandTreeWidget->currentItem();
    if (!item)
        return;

    QVariant data = item->data(1, Qt::UserRole);
    QByteArray name = data.toByteArray(); // command name

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    Command* cmd = cCmdMgr.getCommandByName(name.constData());
    if (cmd && cmd->getAction()) {
        Action* action = cmd->getAction();
        QKeySequence shortcut = editShortcut->text();
        action->setShortcut(shortcut.toString(QKeySequence::NativeText));
        accelLineEditShortcut->setText(editShortcut->text());
        editShortcut->clear();

        // update the tool tip
        QString accel = shortcut.toString(QKeySequence::NativeText);
        QString toolTip = QCoreApplication::translate(cmd->className(),
            cmd->getToolTipText());
        if (!accel.isEmpty()) {
            if (!toolTip.isEmpty()) {
                QString tip = QString::fromLatin1("%1 (%2)")
                    .arg(toolTip, accel);
                action->setToolTip(tip);
            }
        }
        else {
            action->setToolTip(toolTip);
        }

        // update the status tip
        QString statusTip = QCoreApplication::translate(cmd->className(),
            cmd->getStatusTip());
        if (statusTip.isEmpty())
            statusTip = toolTip;
        if (!accel.isEmpty()) {
            if (!statusTip.isEmpty()) {
                QString tip = QString::fromLatin1("(%1)\t%2")
                    .arg(accel, statusTip);
                action->setStatusTip(tip);
            }
        }
        else {
            action->setStatusTip(statusTip);
        }

        ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Shortcut");
        hGrp->SetASCII(name.constData(), accelLineEditShortcut->text().toUtf8());
        buttonAssign->setEnabled(false);
        buttonReset->setEnabled(true);
    }
}

/** Clears the accelerator of the selected command. */
void DlgCustomKeyboardImp::on_buttonClear_clicked()
{
    QTreeWidgetItem* item = commandTreeWidget->currentItem();
    if (!item)
        return;

    QVariant data = item->data(1, Qt::UserRole);
    QByteArray name = data.toByteArray(); // command name

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    Command* cmd = cCmdMgr.getCommandByName(name.constData());
    if (cmd && cmd->getAction()) {
        Action* action = cmd->getAction();
        action->setShortcut(QString());
        accelLineEditShortcut->clear();
        editShortcut->clear();

        // update the tool tip
        QString toolTip = QCoreApplication::translate(cmd->className(),
            cmd->getToolTipText());
        action->setToolTip(toolTip);

        // update the status tip
        QString statusTip = QCoreApplication::translate(cmd->className(),
            cmd->getStatusTip());
        if (statusTip.isEmpty())
            statusTip = toolTip;
        action->setStatusTip(statusTip);

        ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Shortcut");
        hGrp->SetASCII(name.constData(), accelLineEditShortcut->text().toUtf8());
        buttonAssign->setEnabled(false);
        buttonReset->setEnabled(true);
    }
}

/** Resets the accelerator of the selected command to the default. */
void DlgCustomKeyboardImp::on_buttonReset_clicked()
{
    QTreeWidgetItem* item = commandTreeWidget->currentItem();
    if (!item)
        return;

    QVariant data = item->data(1, Qt::UserRole);
    QByteArray name = data.toByteArray(); // command name

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    Command* cmd = cCmdMgr.getCommandByName(name.constData());
    if (cmd && cmd->getAction()) {
        cmd->getAction()->setShortcut(QString::fromLatin1(cmd->getAccel()));
        QString txt = cmd->getAction()->shortcut().toString(QKeySequence::NativeText);
        accelLineEditShortcut->setText((txt.isEmpty() ? tr("none") : txt));
        ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Shortcut");
        hGrp->RemoveASCII(name.constData());
    }

    buttonReset->setEnabled( false );
}

/** Resets the accelerator of all commands to the default. */
void DlgCustomKeyboardImp::on_buttonResetAll_clicked()
{
    CommandManager & cCmdMgr = Application::Instance->commandManager();
    std::vector<Command*> cmds = cCmdMgr.getAllCommands();
    for (std::vector<Command*>::iterator it = cmds.begin(); it != cmds.end(); ++it) {
        if ((*it)->getAction()) {
          (*it)->getAction()->setShortcut(QKeySequence(QString::fromLatin1((*it)->getAccel()))
                                          .toString(QKeySequence::NativeText));
        }
    }

    WindowParameter::getDefaultParameter()->RemoveGrp("Shortcut");
    buttonReset->setEnabled(false);
}

/** Checks for an already occupied shortcut. */
void DlgCustomKeyboardImp::on_editShortcut_textChanged(const QString& sc)
{
    assignedTreeWidget->clear();
    QTreeWidgetItem* item = commandTreeWidget->currentItem();
    if (!item)
        return;
    QVariant data = item->data(1, Qt::UserRole);
    QByteArray name = data.toByteArray(); // command name

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    Command* cmd = cCmdMgr.getCommandByName(name.constData());
    if (cmd && !cmd->getAction()) {
        buttonAssign->setEnabled(false); // command not in use
        return;
    }

    buttonAssign->setEnabled(true);
    QKeySequence ks(sc);
    if (!ks.isEmpty() && !editShortcut->isNone()) {
        int countAmbiguous = 0;
        QString ambiguousCommand;
        QString ambiguousMenu;
        std::vector<Command*> ambiguousCommands;

        CommandManager & cCmdMgr = Application::Instance->commandManager();
        std::vector<Command*> cmds = cCmdMgr.getAllCommands();
        for (std::vector<Command*>::iterator it = cmds.begin(); it != cmds.end(); ++it) {
            QList<QAction*> acts;
            if ((*it)->getAction()) {
                // A command may have several QAction's. So, check all of them if one of them matches (See bug #0002160)
                QList<QAction*> acts = (*it)->getAction()->findChildren<QAction*>();
                for (QList<QAction*>::iterator jt = acts.begin(); jt != acts.end(); ++jt) {
                    if ((*jt)->shortcut() == ks) {
                        ++countAmbiguous;
                        ambiguousCommands.push_back(*it);
                        ambiguousCommand = QString::fromLatin1((*it)->getName()); // store the last one
                        ambiguousMenu = qApp->translate((*it)->className(), (*it)->getMenuText());

                        QTreeWidgetItem* item = new QTreeWidgetItem(assignedTreeWidget);
                        item->setText(1, qApp->translate((*it)->className(), (*it)->getMenuText()));
                        item->setToolTip(1, qApp->translate((*it)->className(), (*it)->getToolTipText()));
                        item->setData(1, Qt::UserRole, QByteArray((*it)->getName()));
                        item->setSizeHint(0, QSize(32, 32));
                        if ((*it)->getPixmap())
                            item->setIcon(0, BitmapFactory().iconFromTheme((*it)->getPixmap()));
                        break;
                    }
                }
            }
        }

        if (countAmbiguous > 0)
            assignedTreeWidget->resizeColumnToContents(0);
        
        if (countAmbiguous > 1) {
            QMessageBox::warning(this, tr("Multiple defined shortcut"),
                                 tr("The shortcut '%1' is defined more than once. This could result into unexpected behaviour.").arg(sc) );
            editShortcut->setFocus();
            buttonAssign->setEnabled(false);
        }
        else if (countAmbiguous == 1 && ambiguousCommand != QLatin1String(name)) {
            int ret = QMessageBox::warning(this, tr("Already defined shortcut"),
                                 tr("The shortcut '%1' is already assigned to '%2'.\n\nDo you want to override it?")
                                 .arg(sc, ambiguousMenu), QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                for (auto* cmd : ambiguousCommands) {
                    Action* action = cmd->getAction();
                    action->setShortcut(QString());
                }
            }
            else {
                editShortcut->setFocus();
                buttonAssign->setEnabled(false);
            }
        }
        else {
            if (cmd && cmd->getAction() && cmd->getAction()->shortcut() == ks)
                buttonAssign->setEnabled(false);
        }
    }
    else {
        if (cmd && cmd->getAction() && cmd->getAction()->shortcut().isEmpty())
            buttonAssign->setEnabled(false); // both key sequences are empty
    }
}

void DlgCustomKeyboardImp::onAddMacroAction(const QByteArray& macro)
{
    QVariant data = categoryBox->itemData(categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros"))
    {
        CommandManager & cCmdMgr = Application::Instance->commandManager();
        Command* pCmd = cCmdMgr.getCommandByName(macro);

        QTreeWidgetItem* item = new QTreeWidgetItem(commandTreeWidget);
        item->setText(1, QString::fromUtf8(pCmd->getMenuText()));
        item->setToolTip(1, QString::fromUtf8(pCmd->getToolTipText()));
        item->setData(1, Qt::UserRole, macro);
        item->setSizeHint(0, QSize(32, 32));
        if (pCmd->getPixmap())
            item->setIcon(0, BitmapFactory().iconFromTheme(pCmd->getPixmap()));
    }
}

void DlgCustomKeyboardImp::onRemoveMacroAction(const QByteArray& macro)
{
    QVariant data = categoryBox->itemData(categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros"))
    {
        for (int i=0; i<commandTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* item = commandTreeWidget->topLevelItem(i);
            QByteArray command = item->data(1, Qt::UserRole).toByteArray();
            if (command == macro) {
                commandTreeWidget->takeTopLevelItem(i);
                delete item;
                break;
            }
        }
    }
}

void DlgCustomKeyboardImp::onModifyMacroAction(const QByteArray& macro)
{
    QVariant data = categoryBox->itemData(categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros"))
    {
        CommandManager & cCmdMgr = Application::Instance->commandManager();
        Command* pCmd = cCmdMgr.getCommandByName(macro);
        for (int i=0; i<commandTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* item = commandTreeWidget->topLevelItem(i);
            QByteArray command = item->data(1, Qt::UserRole).toByteArray();
            if (command == macro) {
                item->setText(1, QString::fromUtf8(pCmd->getMenuText()));
                item->setToolTip(1, QString::fromUtf8(pCmd->getToolTipText()));
                item->setData(1, Qt::UserRole, macro);
                item->setSizeHint(0, QSize(32, 32));
                if (pCmd->getPixmap())
                    item->setIcon(0, BitmapFactory().iconFromTheme(pCmd->getPixmap()));
                if (commandTreeWidget->isItemSelected(item))
                    textLabelDescription->setText(item->toolTip(1));
                break;
            }
        }
    }
}

void DlgCustomKeyboardImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->retranslateUi(this);
        int count = categoryBox->count();

        CommandManager & cCmdMgr = Application::Instance->commandManager();
        for (int i=0; i<count; i++) {
            QVariant data = categoryBox->itemData(i, Qt::UserRole);
            std::vector<Command*> aCmds = cCmdMgr.getGroupCommands(data.toByteArray());
            if (!aCmds.empty()) {
                QString text = qApp->translate(aCmds[0]->className(), aCmds[0]->getGroupName());
                categoryBox->setItemText(i, text);
            }
        }
        on_categoryBox_activated(categoryBox->currentIndex());
    }
    QWidget::changeEvent(e);
}

#include "moc_DlgKeyboardImp.cpp"
