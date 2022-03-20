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
# include <QTreeWidgetItemIterator>
# include <algorithm>
# include <vector>
#endif

#include "DlgCommandsImp.h"
#include "ui_DlgCommands.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"


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

/* TRANSLATOR Gui::Dialog::DlgCustomCommandsImp */

/**
 *  Constructs a DlgCustomCommandsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomCommandsImp::DlgCustomCommandsImp( QWidget* parent  )
  : CustomizeActionPage(parent)
  , ui(new Ui_DlgCustomCommands)
{
    ui->setupUi(this);

    // paints for active and inactive the same color
    QPalette pal = ui->categoryTreeWidget->palette();
    pal.setColor(QPalette::Inactive, QPalette::Highlight, pal.color(QPalette::Active, QPalette::Highlight));
    pal.setColor(QPalette::Inactive, QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::HighlightedText));
    ui->categoryTreeWidget->setPalette( pal );

    connect(ui->commandTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(onDescription(QTreeWidgetItem*)));
    connect(ui->categoryTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(onGroupActivated(QTreeWidgetItem*)));

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

    QStringList labels; labels << tr("Category");
    ui->categoryTreeWidget->setHeaderLabels(labels);
    for (GroupMap::iterator it = groupMap.begin(); it != groupMap.end(); ++it) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->categoryTreeWidget);
        item->setText(0, it->second);
        item->setData(0, Qt::UserRole, QVariant(it->first));
    }

    labels.clear();
    labels << tr("Icon") << tr("Command");
    ui->commandTreeWidget->setHeaderLabels(labels);
    ui->commandTreeWidget->header()->hide();
    ui->commandTreeWidget->setIconSize(QSize(32, 32));
    ui->commandTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    ui->categoryTreeWidget->setCurrentItem(ui->categoryTreeWidget->topLevelItem(0));
}

/** Destroys the object and frees any allocated resources */
DlgCustomCommandsImp::~DlgCustomCommandsImp()
{
}

/** Shows the description for the corresponding command */
void DlgCustomCommandsImp::onDescription(QTreeWidgetItem *item)
{
    if (item)
        ui->textLabel->setText(item->toolTip(1));
    else
        ui->textLabel->setText(QString());
}

/** Shows all commands of this category */
void DlgCustomCommandsImp::onGroupActivated(QTreeWidgetItem* groupItem)
{
    if (!groupItem)
        return;

    QVariant data = groupItem->data(0, Qt::UserRole);
    QString group = data.toString();
    ui->commandTreeWidget->clear();

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    std::vector<Command*> aCmds = cCmdMgr.getGroupCommands(group.toLatin1());
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

    ui->textLabel->setText(QString());
}

void DlgCustomCommandsImp::onAddMacroAction(const QByteArray& macro)
{
    QTreeWidgetItem* item = ui->categoryTreeWidget->currentItem();
    if (!item)
        return;

    QVariant data = item->data(0, Qt::UserRole);
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

void DlgCustomCommandsImp::onRemoveMacroAction(const QByteArray& macro)
{
    QTreeWidgetItem* item = ui->categoryTreeWidget->currentItem();
    if (!item)
        return;

    QVariant data = item->data(0, Qt::UserRole);
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

void DlgCustomCommandsImp::onModifyMacroAction(const QByteArray& macro)
{
    QTreeWidgetItem* item = ui->categoryTreeWidget->currentItem();
    if (!item)
        return;

    QVariant data = item->data(0, Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros"))
    {
        CommandManager & cCmdMgr = Application::Instance->commandManager();
        Command* pCmd = cCmdMgr.getCommandByName(macro);
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
                if (item->isSelected())
                    onDescription(item);
                break;
            }
        }
    }
}

void DlgCustomCommandsImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        QStringList labels; labels << tr("Category");
        ui->categoryTreeWidget->setHeaderLabels(labels);

        CommandManager & cCmdMgr = Application::Instance->commandManager();
        QTreeWidgetItemIterator it(ui->categoryTreeWidget);
        while (*it) {
            QVariant data = (*it)->data(0, Qt::UserRole);
            std::vector<Command*> aCmds = cCmdMgr.getGroupCommands(data.toByteArray());
            if (!aCmds.empty()) {
                QString text = aCmds[0]->translatedGroupName();
                (*it)->setText(0, text);
            }
            ++it;
        }
        onGroupActivated(ui->categoryTreeWidget->topLevelItem(0));
    }
    QWidget::changeEvent(e);
}

#include "moc_DlgCommandsImp.cpp"
