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
# include <QInputDialog>
# include <QHeaderView>
# include <QMessageBox>
# include <QToolBar>
#endif

#include "DlgToolbarsImp.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "ToolBarManager.h"
#include "MainWindow.h"
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
 *  TRUE to construct a modal dialog.
 */
DlgCustomToolbars::DlgCustomToolbars(DlgCustomToolbars::Type t, QWidget* parent)
    : CustomizeActionPage(parent), type(t)
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
        if (jt != groupMap.end())
            jt->second = text;
        else
            groupMap.push_back(std::make_pair(group, text));
    }

    int index = 0;
    for (GroupMap::iterator it = groupMap.begin(); it != groupMap.end(); ++it, ++index) {
        categoryBox->addItem(it->second);
        categoryBox->setItemData(index, QVariant(it->first), Qt::UserRole);
    }

    // fills the combo box with all available workbenches
    QStringList workbenches = Application::Instance->workbenches();
    workbenches.sort();
    index = 0;
    for (QStringList::Iterator it = workbenches.begin(); it != workbenches.end(); ++it, ++index) {
        QPixmap px = Application::Instance->workbenchIcon(*it);
        QString mt = Application::Instance->workbenchMenuText(*it);
        if ( px.isNull() )
            workbenchBox->addItem(mt);
        else
            workbenchBox->addItem(px, mt);
        workbenchBox->setItemData(index, QVariant(*it), Qt::UserRole);
    }

    QStringList labels; 
    labels << tr("Icon") << tr("Command");
    commandTreeWidget->setHeaderLabels(labels);
    commandTreeWidget->header()->hide();
    commandTreeWidget->setIconSize(QSize(32, 32));
    commandTreeWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);

    labels.clear(); labels << tr("Command");
    toolbarTreeWidget->setHeaderLabels(labels);
    toolbarTreeWidget->header()->hide();

    on_categoryBox_activated(categoryBox->currentIndex());
    Workbench* w = WorkbenchManager::instance()->active();
    if (w) {
        QString name = QString::fromAscii(w->name().c_str());
        int index = workbenchBox->findData(name);
        workbenchBox->setCurrentIndex(index);
    }
    on_workbenchBox_activated(workbenchBox->currentIndex());
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
    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toAscii());

    CustomizeActionPage::hideEvent(event);
}

void DlgCustomToolbars::on_categoryBox_activated(int index)
{
    QVariant data = categoryBox->itemData(index, Qt::UserRole);
    QString group = data.toString();
    commandTreeWidget->clear();

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    std::vector<Command*> aCmds = cCmdMgr.getGroupCommands(group.toAscii());

    // Create a separator item
    QTreeWidgetItem* sepitem = new QTreeWidgetItem(commandTreeWidget);
    sepitem->setText(1, tr("<Separator>"));
    sepitem->setData(1, Qt::UserRole, QByteArray("Separator"));
    sepitem->setSizeHint(0, QSize(32, 32));
    for (std::vector<Command*>::iterator it = aCmds.begin(); it != aCmds.end(); ++it) {
        QTreeWidgetItem* item = new QTreeWidgetItem(commandTreeWidget);
        item->setText(1, qApp->translate((*it)->className(), (*it)->getMenuText()));
        item->setToolTip(1, qApp->translate((*it)->className(), (*it)->getToolTipText()));
        item->setData(1, Qt::UserRole, QByteArray((*it)->getName()));
        item->setSizeHint(0, QSize(32, 32));
        if ((*it)->getPixmap())
            item->setIcon(0, BitmapFactory().pixmap((*it)->getPixmap()));
    }
}

void DlgCustomToolbars::on_workbenchBox_activated(int index)
{
    QVariant data = workbenchBox->itemData(index, Qt::UserRole);
    QString workbench = data.toString();
    toolbarTreeWidget->clear();

    QByteArray workbenchname = workbench.toAscii();
    importCustomToolbars(workbenchname);
}

void DlgCustomToolbars::importCustomToolbars(const QByteArray& name)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Workbench");
    const char* subgroup = (type == Toolbar ? "Toolbar" : "Toolboxbar");
    hGrp = hGrp->GetGroup(name.constData())->GetGroup(subgroup);

    std::vector<Base::Reference<ParameterGrp> > hGrps = hGrp->GetGroups();
    CommandManager& rMgr = Application::Instance->commandManager();
    for (std::vector<Base::Reference<ParameterGrp> >::iterator it = hGrps.begin(); it != hGrps.end(); ++it) {
        // create a toplevel item
        QTreeWidgetItem* toplevel = new QTreeWidgetItem(toolbarTreeWidget);
        bool active = (*it)->GetBool("Active", true);
        toplevel->setCheckState(0, (active ? Qt::Checked : Qt::Unchecked));

        // get the elements of the subgroups
        std::vector<std::pair<std::string,std::string> > items = (*it)->GetASCIIMap();
        for (std::vector<std::pair<std::string,std::string> >::iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            if (it2->first == "Separator") {
                QTreeWidgetItem* item = new QTreeWidgetItem(toplevel);
                item->setText(0, tr("<Separator>"));
                item->setData(0, Qt::UserRole, QByteArray("Separator"));
                item->setSizeHint(0, QSize(32, 32));
            } else if (it2->first == "Name") {
                QString toolbarName = QString::fromUtf8(it2->second.c_str());
                toplevel->setText(0, toolbarName);
            } else {
                Command* pCmd = rMgr.getCommandByName(it2->first.c_str());
                if (pCmd) {
                    // command name
                    QTreeWidgetItem* item = new QTreeWidgetItem(toplevel);
                    item->setText(0, qApp->translate(pCmd->className(), pCmd->getMenuText()));
                    item->setData(0, Qt::UserRole, QByteArray(it2->first.c_str()));
                    if (pCmd->getPixmap())
                        item->setIcon(0, BitmapFactory().pixmap(pCmd->getPixmap()));
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
    for (int i=0; i<toolbarTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* toplevel = toolbarTreeWidget->topLevelItem(i);
        QString groupName = QString::fromAscii("Custom_%1").arg(i+1);
        QByteArray toolbarName = toplevel->text(0).toUtf8();
        ParameterGrp::handle hToolGrp = hGrp->GetGroup(groupName.toAscii());
        hToolGrp->SetASCII("Name", toolbarName.constData());
        hToolGrp->SetBool("Active", toplevel->checkState(0) == Qt::Checked);
        for (int j=0; j<toplevel->childCount(); j++) {
            QTreeWidgetItem* child = toplevel->child(j);
            QByteArray commandName = child->data(0, Qt::UserRole).toByteArray();
            if (commandName == "Separator") {
                hToolGrp->SetASCII(commandName, commandName);
            } else {
                Command* pCmd = rMgr.getCommandByName(commandName);
                if (pCmd) {
                    hToolGrp->SetASCII(pCmd->getName(), pCmd->getAppModuleName());
                }
            }
        }
    }
}

/** Adds a new action */
void DlgCustomToolbars::on_moveActionRightButton_clicked()
{
    QTreeWidgetItem* item = commandTreeWidget->currentItem();
    if (item) {
        QTreeWidgetItem* current = toolbarTreeWidget->currentItem();
        if (!current)
            current = toolbarTreeWidget->topLevelItem(0);
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

    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toAscii());
}

/** Removes an action */
void DlgCustomToolbars::on_moveActionLeftButton_clicked()
{
    QTreeWidgetItem* item = toolbarTreeWidget->currentItem();
    if (item && item->parent() && toolbarTreeWidget->isItemSelected(item)) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        parent->takeChild(index);
        QByteArray data = item->data(0, Qt::UserRole).toByteArray();
        removeCustomCommand(parent->text(0), data);
        delete item;
    }

    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toAscii());
}

/** Noves up an action */
void DlgCustomToolbars::on_moveActionUpButton_clicked()
{
    QTreeWidgetItem* item = toolbarTreeWidget->currentItem();
    if (item && item->parent() && toolbarTreeWidget->isItemSelected(item)) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        if (index > 0) {
            parent->takeChild(index);
            parent->insertChild(index-1, item);
            toolbarTreeWidget->setCurrentItem(item);
            QByteArray data = item->data(0, Qt::UserRole).toByteArray();
            moveUpCustomCommand(parent->text(0), data);
        }
    }

    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toAscii());
}

/** Moves down an action */
void DlgCustomToolbars::on_moveActionDownButton_clicked()
{
    QTreeWidgetItem* item = toolbarTreeWidget->currentItem();
    if (item && item->parent() && toolbarTreeWidget->isItemSelected(item)) {
        QTreeWidgetItem* parent = item->parent();
        int index = parent->indexOfChild(item);
        if (index < parent->childCount()-1) {
            parent->takeChild(index);
            parent->insertChild(index+1, item);
            toolbarTreeWidget->setCurrentItem(item);
            QByteArray data = item->data(0, Qt::UserRole).toByteArray();
            moveDownCustomCommand(parent->text(0), data);
        }
    }

    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toAscii());
}

void DlgCustomToolbars::on_newButton_clicked()
{
    bool ok;
    QString text = QString::fromAscii("Custom%1").arg(toolbarTreeWidget->topLevelItemCount()+1);
    text = QInputDialog::getText(this, tr("New toolbar"), tr("Toolbar name:"), QLineEdit::Normal, text, &ok);
    if (ok) {
        // Check for duplicated name
        for (int i=0; i<toolbarTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* toplevel = toolbarTreeWidget->topLevelItem(i);
            QString groupName = toplevel->text(0);
            if (groupName == text) {
                QMessageBox::warning(this, tr("Duplicated name"), tr("The toolbar name '%1' is already used").arg(text));
                return;
            }
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(toolbarTreeWidget);
        item->setText(0, text);
        item->setCheckState(0, Qt::Checked);
        toolbarTreeWidget->setItemExpanded(item, true);

        QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
        QString workbench = data.toString();
        exportCustomToolbars(workbench.toAscii());
        addCustomToolbar(text);
    }
}

void DlgCustomToolbars::on_deleteButton_clicked()
{
    QTreeWidgetItem* item = toolbarTreeWidget->currentItem();
    if (item && !item->parent() && toolbarTreeWidget->isItemSelected(item)) {
        int index = toolbarTreeWidget->indexOfTopLevelItem(item);
        toolbarTreeWidget->takeTopLevelItem(index);
        removeCustomToolbar(item->text(0));
        delete item;
    }

    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    QString workbench = data.toString();
    exportCustomToolbars(workbench.toAscii());
}

void DlgCustomToolbars::on_renameButton_clicked()
{
    bool renamed = false;
    QTreeWidgetItem* item = toolbarTreeWidget->currentItem();
    if (item && !item->parent() && toolbarTreeWidget->isItemSelected(item)) {
        bool ok;
        QString old_text = item->text(0);
        QString text = QInputDialog::getText(this, tr("Rename toolbar"), tr("Toolbar name:"),
            QLineEdit::Normal, old_text, &ok);
        if (ok && text != old_text) {
            // Check for duplicated name
            for (int i=0; i<toolbarTreeWidget->topLevelItemCount(); i++) {
                QTreeWidgetItem* toplevel = toolbarTreeWidget->topLevelItem(i);
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
        QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
        QString workbench = data.toString();
        exportCustomToolbars(workbench.toAscii());
    }
}

void DlgCustomToolbars::onAddMacroAction(const QByteArray& macro)
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
        item->setBackgroundColor(0, Qt::lightGray);
        if (pCmd->getPixmap())
            item->setIcon(0, BitmapFactory().pixmap(pCmd->getPixmap()));
    }
}

void DlgCustomToolbars::onRemoveMacroAction(const QByteArray& macro)
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

void DlgCustomToolbars::onModifyMacroAction(const QByteArray& macro)
{
    QVariant data = categoryBox->itemData(categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros"))
    {
        CommandManager & cCmdMgr = Application::Instance->commandManager();
        Command* pCmd = cCmdMgr.getCommandByName(macro);
        // the left side
        for (int i=0; i<commandTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* item = commandTreeWidget->topLevelItem(i);
            QByteArray command = item->data(1, Qt::UserRole).toByteArray();
            if (command == macro) {
                item->setText(1, QString::fromUtf8(pCmd->getMenuText()));
                item->setToolTip(1, QString::fromUtf8(pCmd->getToolTipText()));
                item->setData(1, Qt::UserRole, macro);
                item->setSizeHint(0, QSize(32, 32));
                item->setBackgroundColor(0, Qt::lightGray);
                if (pCmd->getPixmap())
                    item->setIcon(0, BitmapFactory().pixmap(pCmd->getPixmap()));
                break;
            }
        }
        // the right side
        for (int i=0; i<toolbarTreeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* toplevel = toolbarTreeWidget->topLevelItem(i);
            for (int j=0; j<toplevel->childCount(); j++) {
                QTreeWidgetItem* item = toplevel->child(j);
                QByteArray command = item->data(0, Qt::UserRole).toByteArray();
                if (command == macro) {
                    item->setText(0, QString::fromUtf8(pCmd->getMenuText()));
                    if (pCmd->getPixmap())
                        item->setIcon(0, BitmapFactory().pixmap(pCmd->getPixmap()));
                }
            }
        }
    }
}

void DlgCustomToolbars::changeEvent(QEvent *e)
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

// -------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::DlgCustomToolbarsImp */

/**
 *  Constructs a DlgCustomToolbarsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
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
    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QToolBar* bar = getMainWindow()->addToolBar(name);
        bar->setObjectName(name);
    }
}

void DlgCustomToolbarsImp::removeCustomToolbar(const QString& name)
{
    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) return;
        QToolBar* tb = bars.front();
        getMainWindow()->removeToolBar(tb);
        delete tb;
    }
}

void DlgCustomToolbarsImp::renameCustomToolbar(const QString& old_name, const QString& new_name)
{
    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(old_name);
        if (bars.size() != 1) return;
        QToolBar* tb = bars.front();
        tb->setObjectName(new_name);
        tb->setWindowTitle(new_name);
    }
}

void DlgCustomToolbarsImp::addCustomCommand(const QString& name, const QByteArray& cmd)
{
    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) return;
        CommandManager& mgr = Application::Instance->commandManager();
        mgr.addTo(cmd, bars.front());
    }
}

void DlgCustomToolbarsImp::removeCustomCommand(const QString& name, const QByteArray& cmd)
{
    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) return;
        QList<QAction*> actions = bars.front()->actions();
        for (QList<QAction*>::ConstIterator it = actions.begin(); it != actions.end(); ++it) {
            if ((*it)->data().toByteArray() == cmd) {
                bars.front()->removeAction(*it);
                break;
            }
        }
    }
}

void DlgCustomToolbarsImp::moveUpCustomCommand(const QString& name, const QByteArray& cmd)
{
    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) return;
        QList<QAction*> actions = bars.front()->actions();
        QAction* before=0;
        for (QList<QAction*>::ConstIterator it = actions.begin(); it != actions.end(); ++it) {
            if ((*it)->data().toByteArray() == cmd) {
                if (before != 0) {
                    bars.front()->removeAction(*it);
                    bars.front()->insertAction(before, *it);
                    break;
                }
            }

            before = *it;
        }
    }
}

void DlgCustomToolbarsImp::moveDownCustomCommand(const QString& name, const QByteArray& cmd)
{
    QVariant data = workbenchBox->itemData(workbenchBox->currentIndex(), Qt::UserRole);
    Workbench* w = WorkbenchManager::instance()->active();
    if (w && w->name() == std::string((const char*)data.toByteArray())) {
        QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>(name);
        if (bars.size() != 1) return;
        QList<QAction*> actions = bars.front()->actions();
        for (QList<QAction*>::ConstIterator it = actions.begin(); it != actions.end(); ++it) {
            if ((*it)->data().toByteArray() == cmd) {
                QAction* act = *it;
                if (*it == actions.back())
                    break; // we're already on the last element
                ++it;
                // second last item
                if (*it == actions.back()) {
                    bars.front()->removeAction(act);
                    bars.front()->addAction(act);
                    break;
                }
                ++it;
                bars.front()->removeAction(act);
                bars.front()->insertAction(*it, act);
                break;
            }
        }
    }
}

void DlgCustomToolbarsImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
    }
    DlgCustomToolbars::changeEvent(e);
}


/* TRANSLATOR Gui::Dialog::DlgCustomToolBoxbarsImp */

/**
 *  Constructs a DlgCustomToolBoxbarsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
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
