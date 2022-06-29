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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QHeaderView>
# include <QInputDialog>
# include <QMenu>
# include <QMessageBox>
# include <QToolBar>
# include <QToolButton>
#endif

#include "DlgToolbarsImp.h"
#include "ui_DlgToolbars.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "MainWindow.h"
#include "ToolBarManager.h"
#include "Widgets.h"
#include "Workbench.h"
#include "WorkbenchManager.h"


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

/* TRANSLATOR Gui::Dialog::DlgCustomToolbars */

/**
 *  Constructs a DlgCustomToolbars which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomToolbars::DlgCustomToolbars(DlgCustomToolbars::Type t, QWidget* parent)
    : CustomizeActionPage(parent)
    , ui(new Ui_DlgCustomToolbars)
    , type(t)
{
    ui->setupUi(this);
    ui->moveActionRightButton->setIcon(BitmapFactory().iconFromTheme("button_right"));
    ui->moveActionLeftButton->setIcon(BitmapFactory().iconFromTheme("button_left"));
    ui->moveActionDownButton->setIcon(BitmapFactory().iconFromTheme("button_down"));
    ui->moveActionUpButton->setIcon(BitmapFactory().iconFromTheme("button_up"));

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
        QString text = it->second->translatedGroupName();
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
        ui->categoryBox->addItem(it->second);
        ui->categoryBox->setItemData(index, QVariant(it->first), Qt::UserRole);
    }

    // fills the combo box with all available workbenches
    QStringList workbenches = Application::Instance->workbenches();
    workbenches.sort();
    index = 1;
    ui->workbenchBox->addItem(QApplication::windowIcon(), tr("Global"));
    ui->workbenchBox->setItemData(0, QVariant(QString::fromLatin1("Global")), Qt::UserRole);
    for (QStringList::Iterator it = workbenches.begin(); it != workbenches.end(); ++it) {
        QPixmap px = Application::Instance->workbenchIcon(*it);
        QString mt = Application::Instance->workbenchMenuText(*it);
        if (mt != QLatin1String("<none>")) {
            if (px.isNull())
                ui->workbenchBox->addItem(mt);
            else
                ui->workbenchBox->addItem(px, mt);
            ui->workbenchBox->setItemData(index, QVariant(*it), Qt::UserRole);
            index++;
        }
    }

    QStringList labels;
    labels << tr("Icon") << tr("Command");
    ui->commandTreeWidget->setHeaderLabels(labels);
    ui->commandTreeWidget->header()->hide();
    ui->commandTreeWidget->setIconSize(QSize(32, 32));
    ui->commandTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    labels.clear(); labels << tr("Command");
    ui->toolbarTreeWidget->setHeaderLabels(labels);
    ui->toolbarTreeWidget->header()->hide();

    on_categoryBox_activated(ui->categoryBox->currentIndex());
    Workbench* w = WorkbenchManager::instance()->active();
    if (w) {
        QString name = QString::fromLatin1(w->name().c_str());
        int index = ui->workbenchBox->findData(name);
        ui->workbenchBox->setCurrentIndex(index);
    }
    on_workbenchBox_activated(ui->workbenchBox->currentIndex());
}

/** Destroys the object and frees any allocated resources */
DlgCustomToolbars::~DlgCustomToolbars()
{
}

void DlgCustomToolbars::addCustomToolbar(const QString&)
{
}

void DlgCustomToolbars::removeCustomToolbar(const QString&)
{
}

void DlgCustomToolbars::renameCustomToolbar(const QString&, const QString&)
{
}

void DlgCustomToolbars::addCustomCommand(const QString&, const QByteArray&)
{
}

void DlgCustomToolbars::removeCustomCommand(const QString&, const QByteArray&)
{
}

void DlgCustomToolbars::moveUpCustomCommand(const QString&, const QByteArray&)
{
}

void DlgCustomToolbars::moveDownCustomCommand(const QString&, const QByteArray&)
{
}

void DlgCustomToolbars::hideEvent(QHideEvent * event)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());

    CustomizeActionPage::hideEvent(event);
}

void DlgCustomToolbars::on_categoryBox_activated(int index)
{
    QVariant data = ui->categoryBox->itemData(index, Qt::UserRole);
    QString group = data.toString();
    ui->commandTreeWidget->clear();

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    std::vector<Command*> aCmds = cCmdMgr.getGroupCommands(group.toLatin1());

    // Create a separator item
    QTreeWidgetItem* sepitem = new QTreeWidgetItem(ui->commandTreeWidget);
    sepitem->setText(1, tr("<Separator>"));
    sepitem->setData(1, Qt::UserRole, QByteArray("Separator"));
    sepitem->setSizeHint(0, QSize(32, 32));

    if (group == QLatin1String("Macros")) {
        for (std::vector<Command*>::iterator it = aCmds.begin(); it != aCmds.end(); ++it) {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->commandTreeWidget);
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
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->commandTreeWidget);
            item->setText(1, qApp->translate((*it)->className(), (*it)->getMenuText()));
            item->setToolTip(1, qApp->translate((*it)->className(), (*it)->getToolTipText()));
            item->setData(1, Qt::UserRole, QByteArray((*it)->getName()));
            item->setSizeHint(0, QSize(32, 32));
            if ((*it)->getPixmap())
                item->setIcon(0, BitmapFactory().iconFromTheme((*it)->getPixmap()));
        }
    }
}

void DlgCustomToolbars::on_workbenchBox_activated(int index)
{
    QVariant data = ui->workbenchBox->itemData(index, Qt::UserRole);
    QString workbench = data.toString();
    ui->toolbarTreeWidget->clear();

    QByteArray workbenchname = workbench.toLatin1();
    importCustomToolbars(workbenchname);
}

void DlgCustomToolbars::importCustomToolbars(const QByteArray& name)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Workbench");
    const char* subgroup = (type == Toolbar ? "Toolbar" : "Toolboxbar");
    if (!hGrp->HasGroup(name.constData()))
        return;
    hGrp = hGrp->GetGroup(name.constData());
    if (!hGrp->HasGroup(subgroup))
        return;
    hGrp = hGrp->GetGroup(subgroup);
    std::string separator = "Separator";

    std::vector<Base::Reference<ParameterGrp> > hGrps = hGrp->GetGroups();
    CommandManager& rMgr = Application::Instance->commandManager();
    for (std::vector<Base::Reference<ParameterGrp> >::iterator it = hGrps.begin(); it != hGrps.end(); ++it) {
        // create a toplevel item
        QTreeWidgetItem* toplevel = new QTreeWidgetItem(ui->toolbarTreeWidget);
        bool active = (*it)->GetBool("Active", true);
        toplevel->setCheckState(0, (active ? Qt::Checked : Qt::Unchecked));

        // get the elements of the subgroups
        std::vector<std::pair<std::string,std::string> > items = (*it)->GetASCIIMap();
        for (std::vector<std::pair<std::string,std::string> >::iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            // since we have stored the separators to the user parameters as (key, pair) we had to
            // make sure to use a unique key because otherwise we cannot store more than
            // one.
            if (it2->first.substr(0, separator.size()) == separator) {
                QTreeWidgetItem* item = new QTreeWidgetItem(toplevel);
                item->setText(0, tr("<Separator>"));
                item->setData(0, Qt::UserRole, QByteArray("Separator"));
                item->setSizeHint(0, QSize(32, 32));
            }
            else if (it2->first == "Name") {
                QString toolbarName = QString::fromUtf8(it2->second.c_str());
                toplevel->setText(0, toolbarName);
            }
            else {
                Command* pCmd = rMgr.getCommandByName(it2->first.c_str());
                if (pCmd) {
                    // command name
                    QTreeWidgetItem* item = new QTreeWidgetItem(toplevel);
                    item->setText(0, qApp->translate(pCmd->className(), pCmd->getMenuText()));
                    item->setData(0, Qt::UserRole, QByteArray(it2->first.c_str()));
                    if (pCmd->getPixmap())
                        item->setIcon(0, BitmapFactory().iconFromTheme(pCmd->getPixmap()));
                    item->setSizeHint(0, QSize(32, 32));
                }
                else {
                    // If corresponding module is not yet loaded do not lose the entry
                    QTreeWidgetItem* item = new QTreeWidgetItem(toplevel);
                    item->setText(0, tr("%1 module not loaded").arg(QString::fromStdString(it2->second)));
                    item->setData(0, Qt::UserRole, QByteArray(it2->first.c_str()));
                    item->setData(0, Qt::WhatsThisPropertyRole, QByteArray(it2->second.c_str()));
                    item->setSizeHint(0, QSize(32, 32));
                }
            }
        }
    }
}

void DlgCustomToolbars::exportCustomToolbars(const QByteArray& workbench)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Workbench");
    const char* subgroup = (type == Toolbar ? "Toolbar" : "Toolboxbar");
    hGrp = hGrp->GetGroup(workbench.constData())->GetGroup(subgroup);
    hGrp->Clear();

    CommandManager& rMgr = Application::Instance->commandManager();
    for (int i=0; i<ui->toolbarTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* toplevel = ui->toolbarTreeWidget->topLevelItem(i);
        QString groupName = QString::fromLatin1("Custom_%1").arg(i+1);
        QByteArray toolbarName = toplevel->text(0).toUtf8();
        ParameterGrp::handle hToolGrp = hGrp->GetGroup(groupName.toLatin1());
        hToolGrp->SetASCII("Name", toolbarName.constData());
        hToolGrp->SetBool("Active", toplevel->checkState(0) == Qt::Checked);

        // since we store the separators to the user parameters as (key, pair) we must
        // make sure to use a unique key because otherwise we cannot store more than
        // one.
        int suffixSeparator = 1;
        for (int j=0; j<toplevel->childCount(); j++) {
            QTreeWidgetItem* child = toplevel->child(j);
            QByteArray commandName = child->data(0, Qt::UserRole).toByteArray();
            if (commandName == "Separator") {
                QByteArray key = commandName + QByteArray::number(suffixSeparator);
                suffixSeparator++;
                hToolGrp->SetASCII(key, commandName);
            }
            else {
                Command* pCmd = rMgr.getCommandByName(commandName);
                if (pCmd) {
                    hToolGrp->SetASCII(pCmd->getName(), pCmd->getAppModuleName());
                }
                else {
                    QByteArray moduleName = child->data(0, Qt::WhatsThisPropertyRole).toByteArray();
                    hToolGrp->SetASCII(commandName, moduleName);
                }
            }
        }
    }
}

/** Adds a new action */
void DlgCustomToolbars::on_moveActionRightButton_clicked()
{
    QTreeWidgetItem* item = ui->commandTreeWidget->currentItem();
    if (item) {
        QTreeWidgetItem* current = ui->toolbarTreeWidget->currentItem();
        if (!current)
            current = ui->toolbarTreeWidget->topLevelItem(0);
        else if (current->parent())
            current = current->parent();
        if (current && !current->parent()) {
            QTreeWidgetItem* copy = new QTreeWidgetItem(current);
            copy->setText(0, item->text(1));
            copy->setIcon(0, item->icon(0));
            QByteArray data = item->data(1, Qt::UserRole).toByteArray();
            copy->setData(0, Qt::UserRole, data);
            copy->setSizeHint(0, QSize(32, 32));
            addCustomCommand(current->text(0), data);
        }
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

/** Removes an action */
void DlgCustomToolbars::on_moveActionLeftButton_clicked()
{
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && item->parent() && item->isSelected()) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        parent->takeChild(index);

        // In case a separator should be moved we have to count the separators
        // which come before this one.
        // This is needed so that we can distinguish in removeCustomCommand
        // which separator it is.
        QByteArray data = item->data(0, Qt::UserRole).toByteArray();
        if (data == "Separator") {
            int countSep = 1;
            for (int i=0; i<index-1; i++) {
                QByteArray d = parent->child(i)->data(0, Qt::UserRole).toByteArray();
                if (d == "Separator") {
                    countSep++;
                }
            }

            data += QByteArray::number(countSep);
        }
        removeCustomCommand(parent->text(0), data);
        delete item;
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

/** Moves up an action */
void DlgCustomToolbars::on_moveActionUpButton_clicked()
{
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && item->parent() && item->isSelected()) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        if (index > 0) {
            // In case a separator should be moved we have to count the separators
            // which come before this one.
            // This is needed so that we can distinguish in moveUpCustomCommand
            // which separator it is.
            QByteArray data = item->data(0, Qt::UserRole).toByteArray();
            if (data == "Separator") {
                int countSep = 1;
                for (int i=0; i<index; i++) {
                    QByteArray d = parent->child(i)->data(0, Qt::UserRole).toByteArray();
                    if (d == "Separator") {
                        countSep++;
                    }
                }

                data += QByteArray::number(countSep);
            }

            parent->takeChild(index);
            parent->insertChild(index-1, item);
            ui->toolbarTreeWidget->setCurrentItem(item);

            moveUpCustomCommand(parent->text(0), data);
        }
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

/** Moves down an action */
void DlgCustomToolbars::on_moveActionDownButton_clicked()
{
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && item->parent() && item->isSelected()) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        if (index < parent->childCount()-1) {
            // In case a separator should be moved we have to count the separators
            // which come before this one.
            // This is needed so that we can distinguish in moveDownCustomCommand
            // which separator it is.
            QByteArray data = item->data(0, Qt::UserRole).toByteArray();
            if (data == "Separator") {
                int countSep = 1;
                for (int i=0; i<index; i++) {
                    QByteArray d = parent->child(i)->data(0, Qt::UserRole).toByteArray();
                    if (d == "Separator") {
                        countSep++;
                    }
                }

                data += QByteArray::number(countSep);
            }

            parent->takeChild(index);
            parent->insertChild(index+1, item);
            ui->toolbarTreeWidget->setCurrentItem(item);

            moveDownCustomCommand(parent->text(0), data);
        }
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

void DlgCustomToolbars::on_newButton_clicked()
{
    bool ok;
    QString text = QString::fromLatin1("Custom%1").arg(ui->toolbarTreeWidget->topLevelItemCount()+1);
    text = QInputDialog::getText(this, tr("New toolbar"), tr("Toolbar name:"), QLineEdit::Normal, text, &ok, Qt::MSWindowsFixedSizeDialogHint);
    if (ok) {
        // Check for duplicated name
        for (int i=0; i<ui->toolbarTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* toplevel = ui->toolbarTreeWidget->topLevelItem(i);
            QString groupName = toplevel->text(0);
            if (groupName == text) {
                QMessageBox::warning(this, tr("Duplicated name"), tr("The toolbar name '%1' is already used").arg(text));
                return;
            }
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(ui->toolbarTreeWidget);
        item->setText(0, text);
        item->setCheckState(0, Qt::Checked);
        item->setExpanded(true);

        QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
        QString workbench = data.toString();
        exportCustomToolbars(workbench.toLatin1());
        addCustomToolbar(text);
    }
}

void DlgCustomToolbars::on_deleteButton_clicked()
{
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && !item->parent() && item->isSelected()) {
        int index = ui->toolbarTreeWidget->indexOfTopLevelItem(item);
        ui->toolbarTreeWidget->takeTopLevelItem(index);
        removeCustomToolbar(item->text(0));
        delete item;
    }

    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toLatin1());
}

void DlgCustomToolbars::on_renameButton_clicked()
{
    bool renamed = false;
    QTreeWidgetItem* item = ui->toolbarTreeWidget->currentItem();
    if (item && !item->parent() && item->isSelected()) {
        bool ok;
        QString old_text = item->text(0);
        QString text = QInputDialog::getText(this, tr("Rename toolbar"), tr("Toolbar name:"),
            QLineEdit::Normal, old_text, &ok, Qt::MSWindowsFixedSizeDialogHint);
        if (ok && text != old_text) {
            // Check for duplicated name
            for (int i=0; i<ui->toolbarTreeWidget->topLevelItemCount(); i++) {
                QTreeWidgetItem* toplevel = ui->toolbarTreeWidget->topLevelItem(i);
                QString groupName = toplevel->text(0);
                if (groupName == text && toplevel != item) {
                    QMessageBox::warning(this, tr("Duplicated name"), tr("The toolbar name '%1' is already used").arg(text));
                    return;
                }
            }

            item->setText(0, text);
            renameCustomToolbar(old_text, text);
            renamed = true;
        }
    }

    if (renamed) {
        QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
        QString workbench = data.toString();
        exportCustomToolbars(workbench.toLatin1());
    }
}

void DlgCustomToolbars::onAddMacroAction(const QByteArray& macro)
{
    QVariant data = ui->categoryBox->itemData(ui->categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros"))
    {
        CommandManager & cCmdMgr = Application::Instance->commandManager();
        Command* pCmd = cCmdMgr.getCommandByName(macro);

        QTreeWidgetItem* item = new QTreeWidgetItem(ui->commandTreeWidget);
        item->setText(1, QString::fromUtf8(pCmd->getMenuText()));
        item->setToolTip(1, QString::fromUtf8(pCmd->getToolTipText()));
        item->setData(1, Qt::UserRole, macro);
        item->setSizeHint(0, QSize(32, 32));
        if (pCmd->getPixmap())
            item->setIcon(0, BitmapFactory().iconFromTheme(pCmd->getPixmap()));
    }
}

void DlgCustomToolbars::onRemoveMacroAction(const QByteArray& macro)
{
    QVariant data = ui->categoryBox->itemData(ui->categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros"))
    {
        for (int i=0; i<ui->commandTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* item = ui->commandTreeWidget->topLevelItem(i);
            QByteArray command = item->data(1, Qt::UserRole).toByteArray();
            if (command == macro) {
                ui->commandTreeWidget->takeTopLevelItem(i);
                delete item;
                break;
            }
        }
    }
}

void DlgCustomToolbars::onModifyMacroAction(const QByteArray& macro)
{
    QVariant data = ui->categoryBox->itemData(ui->categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros"))
    {
        CommandManager & cCmdMgr = Application::Instance->commandManager();
        Command* pCmd = cCmdMgr.getCommandByName(macro);
        // the left side
        for (int i=0; i<ui->commandTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* item = ui->commandTreeWidget->topLevelItem(i);
            QByteArray command = item->data(1, Qt::UserRole).toByteArray();
            if (command == macro) {
                item->setText(1, QString::fromUtf8(pCmd->getMenuText()));
                item->setToolTip(1, QString::fromUtf8(pCmd->getToolTipText()));
                item->setData(1, Qt::UserRole, macro);
                item->setSizeHint(0, QSize(32, 32));
                if (pCmd->getPixmap())
                    item->setIcon(0, BitmapFactory().iconFromTheme(pCmd->getPixmap()));
                break;
            }
        }
        // the right side
        for (int i=0; i<ui->toolbarTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* toplevel = ui->toolbarTreeWidget->topLevelItem(i);
            for (int j=0; j<toplevel->childCount(); j++) {
                QTreeWidgetItem* item = toplevel->child(j);
                QByteArray command = item->data(0, Qt::UserRole).toByteArray();
                if (command == macro) {
                    item->setText(0, QString::fromUtf8(pCmd->getMenuText()));
                    if (pCmd->getPixmap())
                        item->setIcon(0, BitmapFactory().iconFromTheme(pCmd->getPixmap()));
                }
            }
        }
    }
}

void DlgCustomToolbars::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        int count = ui->categoryBox->count();

        CommandManager & cCmdMgr = Application::Instance->commandManager();
        for (int i=0; i<count; i++) {
            QVariant data = ui->categoryBox->itemData(i, Qt::UserRole);
            std::vector<Command*> aCmds = cCmdMgr.getGroupCommands(data.toByteArray());
            if (!aCmds.empty()) {
                QString text = aCmds[0]->translatedGroupName();
                ui->categoryBox->setItemText(i, text);
            }
        }
        on_categoryBox_activated(ui->categoryBox->currentIndex());
    }
    QWidget::changeEvent(e);
}

// -------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::DlgCustomToolbarsImp */

/**
 *  Constructs a DlgCustomToolbarsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomToolbarsImp::DlgCustomToolbarsImp( QWidget* parent )
    : DlgCustomToolbars(DlgCustomToolbars::Toolbar, parent)
{
}

/** Destroys the object and frees any allocated resources */
DlgCustomToolbarsImp::~DlgCustomToolbarsImp()
{
}

void DlgCustomToolbarsImp::addCustomToolbar(const QString& name)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QToolBar* bar = getMainWindow()->addToolBar(name);
        bar->setObjectName(name);
    }
}

void DlgCustomToolbarsImp::removeCustomToolbar(const QString& name)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1)
            return;

        QToolBar* tb = bars.front();
        getMainWindow()->removeToolBar(tb);
        delete tb;
    }
}

void DlgCustomToolbarsImp::renameCustomToolbar(const QString& old_name, const QString& new_name)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(old_name);
        if (bars.size() != 1)
            return;

        QToolBar* tb = bars.front();
        tb->setObjectName(new_name);
        tb->setWindowTitle(new_name);
    }
}

QList<QAction*> DlgCustomToolbarsImp::getActionGroup(QAction* action)
{
    QList<QAction*> group;
    QList<QWidget*> widgets = action->associatedWidgets();
    for (QList<QWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
        QToolButton* tb = qobject_cast<QToolButton*>(*it);
        if (tb) {
            QMenu* menu = tb->menu();
            if (menu) {
                group = menu->actions();
                break;
            }
        }
    }
    return group;
}

void DlgCustomToolbarsImp::setActionGroup(QAction* action, const QList<QAction*>& group)
{
    // See also ActionGroup::addTo()
    QList<QWidget*> widgets = action->associatedWidgets();
    for (QList<QWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
        QToolButton* tb = qobject_cast<QToolButton*>(*it);
        if (tb) {
            QMenu* menu = tb->menu();
            if (!menu) {
                tb->setPopupMode(QToolButton::MenuButtonPopup);
                tb->setObjectName(QString::fromLatin1("qt_toolbutton_menubutton"));
                QMenu* menu = new QMenu(tb);
                menu->addActions(group);
                tb->setMenu(menu);
            }
        }
    }
}

void DlgCustomToolbarsImp::addCustomCommand(const QString& name, const QByteArray& cmd)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1)
            return;

        if (cmd == "Separator") {
            QAction* action = bars.front()->addSeparator();
            action->setData(QByteArray("Separator"));
        }
        else {
            CommandManager& mgr = Application::Instance->commandManager();
            if (mgr.addTo(cmd, bars.front())) {
                QAction* action = bars.front()->actions().last();
                // See ToolBarManager::setup(ToolBarItem* , QToolBar* )
                // We have to add the user data in order to identify the command in
                // removeCustomCommand(), moveUpCustomCommand() or moveDownCustomCommand()
                if (action && action->data().isNull())
                    action->setData(cmd);
            }
        }
    }
}

void DlgCustomToolbarsImp::removeCustomCommand(const QString& name, const QByteArray& userdata)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1)
            return;

        QByteArray cmd = userdata;
        int numSep = 0, indexSep = 0;
        if (cmd.startsWith("Separator")) {
            numSep = cmd.mid(9).toInt();
            cmd = "Separator";
        }
        QList<QAction*> actions = bars.front()->actions();
        for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
            if ((*it)->data().toByteArray() == cmd) {
                // if we move a separator then make sure to pick up the right one
                if (numSep > 0) {
                    if (++indexSep < numSep)
                        continue;
                }
                bars.front()->removeAction(*it);
                break;
            }
        }
    }
}

void DlgCustomToolbarsImp::moveUpCustomCommand(const QString& name, const QByteArray& userdata)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1)
            return;

        QByteArray cmd = userdata;
        int numSep = 0, indexSep = 0;
        if (cmd.startsWith("Separator")) {
            numSep = cmd.mid(9).toInt();
            cmd = "Separator";
        }
        QList<QAction*> actions = bars.front()->actions();
        QAction* before=nullptr;
        for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
            if ((*it)->data().toByteArray() == cmd) {
                // if we move a separator then make sure to pick up the right one
                if (numSep > 0) {
                    if (++indexSep < numSep) {
                        before = *it;
                        continue;
                    }
                }
                if (before != nullptr) {
                    QList<QAction*> group = getActionGroup(*it);
                    bars.front()->removeAction(*it);
                    bars.front()->insertAction(before, *it);
                    if (!group.isEmpty())
                        setActionGroup(*it, group);
                    break;
                }
            }

            before = *it;
        }
    }
}

void DlgCustomToolbarsImp::moveDownCustomCommand(const QString& name, const QByteArray& userdata)
{
    QVariant data = ui->workbenchBox->itemData(ui->workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1)
            return;

        QByteArray cmd = userdata;
        int numSep = 0, indexSep = 0;
        if (cmd.startsWith("Separator")) {
            numSep = cmd.mid(9).toInt();
            cmd = "Separator";
        }
        QList<QAction*> actions = bars.front()->actions();
        for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
            if ((*it)->data().toByteArray() == cmd) {
                // if we move a separator then make sure to pick up the right one
                if (numSep > 0) {
                    if (++indexSep < numSep)
                        continue;
                }
                QAction* act = *it;
                if (*it == actions.back())
                    break; // we're already on the last element
                ++it;
                // second last item
                if (*it == actions.back()) {
                    QList<QAction*> group = getActionGroup(act);
                    bars.front()->removeAction(act);
                    bars.front()->addAction(act);
                    if (!group.isEmpty())
                        setActionGroup(act, group);
                    break;
                }
                ++it;
                QList<QAction*> group = getActionGroup(act);
                bars.front()->removeAction(act);
                bars.front()->insertAction(*it, act);
                if (!group.isEmpty())
                    setActionGroup(act, group);
                break;
            }
        }
    }
}

void DlgCustomToolbarsImp::changeEvent(QEvent *e)
{
    DlgCustomToolbars::changeEvent(e);
}


/* TRANSLATOR Gui::Dialog::DlgCustomToolBoxbarsImp */

/**
 *  Constructs a DlgCustomToolBoxbarsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomToolBoxbarsImp::DlgCustomToolBoxbarsImp( QWidget* parent )
    : DlgCustomToolbars(DlgCustomToolbars::Toolboxbar, parent)
{
    setWindowTitle( tr( "Toolbox bars" ) );
}

/** Destroys the object and frees any allocated resources */
DlgCustomToolBoxbarsImp::~DlgCustomToolBoxbarsImp()
{
}

void DlgCustomToolBoxbarsImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        setWindowTitle( tr( "Toolbox bars" ) );
    }
    DlgCustomToolbars::changeEvent(e);
}

#include "moc_DlgToolbarsImp.cpp"
