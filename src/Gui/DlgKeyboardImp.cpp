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
#include <boost/signals2/connection.hpp>
#ifndef _PreComp_
# include <QAction>
# include <QHeaderView>
# include <QMessageBox>
# include <QTimer>
#endif

#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Base/Console.h>

#include "DlgKeyboardImp.h"
#include "ui_DlgKeyboard.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Window.h"
#include "PrefWidgets.h"
#include "ShortcutManager.h"
#include "CommandCompleter.h"


using namespace Gui::Dialog;

namespace Gui { namespace Dialog {
using GroupMap = std::vector< std::pair<QLatin1String, QString> >;

struct GroupMap_find {
    const QLatin1String& item;
    explicit GroupMap_find(const QLatin1String& item) : item(item) {}
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
  : CustomizeActionPage(parent)
  , ui(new Ui_DlgCustomKeyboard)
  , firstShow(true)
{
    ui->setupUi(this);
    setupConnections();

    // Force create actions for all commands with shortcut to register with ShortcutManager
    for (auto cmd : Application::Instance->commandManager().getAllCommands()) {
        if (cmd->getShortcut().size())
            cmd->initAction();
    }
    QObject::connect(ShortcutManager::instance(), &ShortcutManager::shortcutChanged, this,
        [](const char *cmdName) {
            if (auto cmd = Application::Instance->commandManager().getCommandByName(cmdName))
                cmd->initAction();
        });

    conn = initCommandWidgets(ui->commandTreeWidget,
                              nullptr,
                              ui->categoryBox,
                              ui->editCommand,
                              ui->assignedTreeWidget,
                              ui->buttonUp,
                              ui->buttonDown,
                              ui->editShortcut,
                              ui->accelLineEditShortcut);

    ui->shortcutTimeout->onRestore();
    QTimer *timer = new QTimer(this);
    QObject::connect(ui->shortcutTimeout, qOverload<int>(&QSpinBox::valueChanged), timer, [=](int) {
        timer->start(100);
    });
    QObject::connect(timer, &QTimer::timeout, [=]() {
        ui->shortcutTimeout->onSave();
    });
}

/** Destroys the object and frees any allocated resources */
DlgCustomKeyboardImp::~DlgCustomKeyboardImp() = default;

void DlgCustomKeyboardImp::setupConnections()
{
    connect(ui->categoryBox, qOverload<int>(&QComboBox::activated),
            this, &DlgCustomKeyboardImp::onCategoryBoxActivated);
    connect(ui->commandTreeWidget, &QTreeWidget::currentItemChanged,
            this, &DlgCustomKeyboardImp::onCommandTreeWidgetCurrentItemChanged);
    connect(ui->buttonAssign, &QPushButton::clicked,
            this, &DlgCustomKeyboardImp::onButtonAssignClicked);
    connect(ui->buttonClear, &QPushButton::clicked,
            this, &DlgCustomKeyboardImp::onButtonClearClicked);
    connect(ui->buttonReset, &QPushButton::clicked,
            this, &DlgCustomKeyboardImp::onButtonResetClicked);
    connect(ui->buttonResetAll, &QPushButton::clicked,
            this, &DlgCustomKeyboardImp::onButtonResetAllClicked);
    connect(ui->editShortcut, &AccelLineEdit::textChanged,
            this, &DlgCustomKeyboardImp::onEditShortcutTextChanged);
}

void DlgCustomKeyboardImp::initCommandCompleter(QLineEdit *edit,
                                                QComboBox *combo,
                                                QTreeWidget *commandTreeWidget,
                                                QTreeWidgetItem *separatorItem)
{
    edit->setPlaceholderText(tr("Type to search..."));
    auto completer = new CommandCompleter(edit, edit);

    QObject::connect(completer, &CommandCompleter::commandActivated,
        [=](const QByteArray &name) {
            CommandManager & cCmdMgr = Application::Instance->commandManager();
            Command *cmd = cCmdMgr.getCommandByName(name.constData());
            if (!cmd)
                return;

            QString group = QString::fromLatin1(cmd->getGroupName());
            int index = combo->findData(group);
            if (index < 0)
                return;
            if (index != combo->currentIndex()) {
                QSignalBlocker blocker(combo);
                combo->setCurrentIndex(index);
                populateCommandList(commandTreeWidget, separatorItem, combo);
            }
            for (int i=0 ; i<commandTreeWidget->topLevelItemCount(); ++i) {
                QTreeWidgetItem *item = commandTreeWidget->topLevelItem(i);
                if (item->data(1, Qt::UserRole).toByteArray() == name) {
                    commandTreeWidget->setCurrentItem(item);
                    return;
                }
            }
        });
}

void DlgCustomKeyboardImp::populateCommandList(QTreeWidget *commandTreeWidget,
                                               QTreeWidgetItem *separatorItem,
                                               QComboBox *combo) 
{
    QByteArray current;
    if (auto item = commandTreeWidget->currentItem())
        current = item->data(1, Qt::UserRole).toByteArray();

    if (separatorItem) {
        commandTreeWidget->takeTopLevelItem(commandTreeWidget->indexOfTopLevelItem(separatorItem));
    }
    commandTreeWidget->clear();
    if (separatorItem) {
        commandTreeWidget->addTopLevelItem(separatorItem);
    }

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    auto group = combo->itemData(combo->currentIndex(), Qt::UserRole).toByteArray();
    auto cmds = group == "All" ? cCmdMgr.getAllCommands()
                                : cCmdMgr.getGroupCommands(group.constData());
    QTreeWidgetItem *currentItem = nullptr;
    for (const Command *cmd : cmds) {
        QTreeWidgetItem* item = new QTreeWidgetItem(commandTreeWidget);
        item->setText(1, Action::commandMenuText(cmd));
        item->setToolTip(1, Action::commandToolTip(cmd));
        item->setData(1, Qt::UserRole, QByteArray(cmd->getName()));
        item->setSizeHint(0, QSize(32, 32));
        if (auto pixmap = cmd->getPixmap())
            item->setIcon(0, BitmapFactory().iconFromTheme(pixmap));
        item->setText(2, cmd->getShortcut());
        if (auto accel = cmd->getAccel())
            item->setText(3, QKeySequence(QString::fromLatin1(accel)).toString());

        if (current == cmd->getName())
            currentItem = item;
    }
    if (currentItem)
        commandTreeWidget->setCurrentItem(currentItem);
    commandTreeWidget->resizeColumnToContents(2);
    commandTreeWidget->resizeColumnToContents(3);
}

boost::signals2::connection
DlgCustomKeyboardImp::initCommandList(QTreeWidget *commandTreeWidget,
                                      QTreeWidgetItem *separatorItem,
                                      QComboBox *combo)
{
    QStringList labels;
    labels << tr("Icon") << tr("Command") << tr("Shortcut") << tr("Default");
    commandTreeWidget->setHeaderLabels(labels);
    commandTreeWidget->setIconSize(QSize(32, 32));
    commandTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    commandTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    commandTreeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    populateCommandGroups(combo);

    // Using a timer to respond to command change for performance, and also
    // because macro command may be added before proper initialization (null
    // menu text, etc.)
    QTimer *timer = new QTimer(combo);
    timer->setSingleShot(true);

    QObject::connect(timer, &QTimer::timeout, [=](){
        populateCommandGroups(combo);
        populateCommandList(commandTreeWidget, separatorItem, combo);
    });

    QObject::connect(ShortcutManager::instance(), &ShortcutManager::shortcutChanged, timer, [timer]() {
        timer->start(100);
    });

    QObject::connect(combo, qOverload<int>(&QComboBox::activated), timer, [timer]() {
        timer->start(100);
    });

    return Application::Instance->commandManager().signalChanged.connect([timer](){
        timer->start(100);
    });
}

void DlgCustomKeyboardImp::initPriorityList(QTreeWidget *priorityList,
                                            QAbstractButton *buttonUp,
                                            QAbstractButton *buttonDown)
{
    QStringList labels;
    labels << tr("Name") << tr("Title");
    priorityList->setHeaderLabels(labels);
    priorityList->header()->hide();
    priorityList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    priorityList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    auto updatePriorityList = [priorityList](bool up) {
        auto item = priorityList->currentItem();
        if (!item)
            return;

        int index = priorityList->indexOfTopLevelItem(item);
        if (index < 0)
            return;
        if ((index == 0 && up)
                || (index == priorityList->topLevelItemCount()-1 && !up))
            return;

        std::vector<QByteArray> actions;
        for (int i=0; i<priorityList->topLevelItemCount(); ++i) {
            auto item = priorityList->topLevelItem(i);
            actions.push_back(item->data(0, Qt::UserRole).toByteArray());
        }

        auto it = actions.begin() + index;
        auto itNext = up ? it - 1 : it + 1;
        std::swap(*it, *itNext);
        ShortcutManager::instance()->setPriorities(actions);
    };

    QObject::connect(buttonUp, &QAbstractButton::clicked, [=](){updatePriorityList(true);});
    QObject::connect(buttonDown, &QAbstractButton::clicked, [=](){updatePriorityList(false);});
    QObject::connect(priorityList, &QTreeWidget::currentItemChanged,
        [=](QTreeWidgetItem *item){
            buttonUp->setEnabled(item!=nullptr);
            buttonDown->setEnabled(item!=nullptr);
        }
    );
}

boost::signals2::connection
DlgCustomKeyboardImp::initCommandWidgets(QTreeWidget *commandTreeWidget,
                                         QTreeWidgetItem *separatorItem,
                                         QComboBox *comboGroups,
                                         QLineEdit *editCommand,
                                         QTreeWidget *priorityList,
                                         QAbstractButton *buttonUp,
                                         QAbstractButton *buttonDown,
                                         Gui::AccelLineEdit *editShortcut,
                                         Gui::AccelLineEdit *currentShortcut)
{
    initCommandCompleter(editCommand, comboGroups, commandTreeWidget, separatorItem);
    auto conn = initCommandList(commandTreeWidget, separatorItem, comboGroups);

    if (priorityList && buttonUp && buttonDown) {
        initPriorityList(priorityList, buttonUp, buttonDown);

        auto timer = new QTimer(priorityList);
        timer->setSingleShot(true);
        if (currentShortcut) {
            QObject::connect(currentShortcut, &QLineEdit::textChanged, timer, [timer]() {
                timer->start(200);
            });
        }
        QObject::connect(editShortcut, &QLineEdit::textChanged, timer, [timer]() {
            timer->start(200);
        });
        QObject::connect(ShortcutManager::instance(), &ShortcutManager::priorityChanged, timer, [timer](){
            timer->start(200);
        });
        QObject::connect(timer, &QTimer::timeout, [=]() {
            populatePriorityList(priorityList, editShortcut, currentShortcut);
        });
    }

    return conn;
}

void DlgCustomKeyboardImp::populatePriorityList(QTreeWidget *priorityList,
                                                Gui::AccelLineEdit *editor,
                                                Gui::AccelLineEdit *curShortcut)
{
    QByteArray current;
    if (auto currentItem = priorityList->currentItem())
        current = currentItem->data(0, Qt::UserRole).toByteArray();

    priorityList->clear();
    QString sc;
    if (!editor->isNone() && editor->text().size())
        sc = editor->text();
    else if (curShortcut && !curShortcut->isNone())
        sc = curShortcut->text();

    auto actionList = ShortcutManager::instance()->getActionsByShortcut(sc);
    QTreeWidgetItem *currentItem = nullptr;
    for (const auto &info : actionList) {
        if (!info.second)
            continue;
        QTreeWidgetItem* item = new QTreeWidgetItem(priorityList);
        item->setText(0, QString::fromUtf8(info.first));
        item->setText(1, Action::cleanTitle(info.second->text()));
        item->setToolTip(0, info.second->toolTip());
        item->setIcon(0, info.second->icon());
        item->setData(0, Qt::UserRole, info.first);
        if (current == info.first)
            currentItem = item;
    }
    priorityList->resizeColumnToContents(0);
    priorityList->resizeColumnToContents(1);
    if (currentItem)
        priorityList->setCurrentItem(currentItem);
}

void DlgCustomKeyboardImp::populateCommandGroups(QComboBox *combo)
{
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

    for (const auto & sCommand : sCommands) {
        QLatin1String group(sCommand.second->getGroupName());
        QString text = sCommand.second->translatedGroupName();
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
    groupMap.push_back(std::make_pair(QLatin1String("All"), tr("All")));

    for (const auto & it : groupMap) {
        if (combo->findData(it.first) < 0) {
            combo->addItem(it.second);
            combo->setItemData(combo->count()-1, QVariant(it.first), Qt::UserRole);
        }
    }
}

void DlgCustomKeyboardImp::showEvent(QShowEvent* e)
{
    Q_UNUSED(e);
    // If we did this already in the constructor we wouldn't get the vertical scrollbar if needed.
    // The problem was noticed with Qt 4.1.4 but may arise with any later version.
    if (firstShow) {
        ui->categoryBox->activated(ui->categoryBox->currentIndex());
        firstShow = false;
    }
}

/** Shows the description for the corresponding command */
void DlgCustomKeyboardImp::onCommandTreeWidgetCurrentItemChanged(QTreeWidgetItem* item)
{
    if (!item)
        return;

    QVariant data = item->data(1, Qt::UserRole);
    QByteArray name = data.toByteArray(); // command name

    CommandManager & cCmdMgr = Application::Instance->commandManager();
    Command* cmd = cCmdMgr.getCommandByName(name.constData());
    if (cmd) {
        QKeySequence ks = ShortcutManager::instance()->getShortcut(
                cmd->getName(), cmd->getAccel());
        QKeySequence ks2 = QString::fromLatin1(cmd->getAccel());
        QKeySequence ks3 = ui->editShortcut->text();
        if (ks.isEmpty())
            ui->accelLineEditShortcut->setText( tr("none") );
        else
            ui->accelLineEditShortcut->setText(ks.toString(QKeySequence::NativeText));

        ui->buttonAssign->setEnabled(!ui->editShortcut->text().isEmpty() && (ks != ks3));
        ui->buttonReset->setEnabled((ks != ks2));
    }
}

/** Shows all commands of this category */
void DlgCustomKeyboardImp::onCategoryBoxActivated(int)
{
    ui->buttonAssign->setEnabled(false);
    ui->buttonReset->setEnabled(false);
    ui->accelLineEditShortcut->clear();
    ui->editShortcut->clear();
}

void DlgCustomKeyboardImp::setShortcutOfCurrentAction(const QString& accelText)
{
    QTreeWidgetItem* item = ui->commandTreeWidget->currentItem();
    if (!item)
        return;

    QVariant data = item->data(1, Qt::UserRole);
    QByteArray name = data.toByteArray(); // command name

    QString nativeText;
    if (!accelText.isEmpty()) {
        QKeySequence shortcut = accelText;
        nativeText = shortcut.toString(QKeySequence::NativeText);
        ui->accelLineEditShortcut->setText(accelText);
        ui->editShortcut->clear();
    }
    else {
        ui->accelLineEditShortcut->clear();
        ui->editShortcut->clear();
    }
    ShortcutManager::instance()->setShortcut(name, nativeText.toLatin1());

    ui->buttonAssign->setEnabled(false);
    ui->buttonReset->setEnabled(true);
}

/** Assigns a new accelerator to the selected command. */
void DlgCustomKeyboardImp::onButtonAssignClicked()
{
    setShortcutOfCurrentAction(ui->editShortcut->text());
}

/** Clears the accelerator of the selected command. */
void DlgCustomKeyboardImp::onButtonClearClicked()
{
    setShortcutOfCurrentAction(QString());
}

/** Resets the accelerator of the selected command to the default. */
void DlgCustomKeyboardImp::onButtonResetClicked()
{
    QTreeWidgetItem* item = ui->commandTreeWidget->currentItem();
    if (!item)
        return;

    QVariant data = item->data(1, Qt::UserRole);
    QByteArray name = data.toByteArray(); // command name
    ShortcutManager::instance()->reset(name);

    QString txt = ShortcutManager::instance()->getShortcut(name);
    ui->accelLineEditShortcut->setText((txt.isEmpty() ? tr("none") : txt));
    ui->buttonReset->setEnabled( false );
}

/** Resets the accelerator of all commands to the default. */
void DlgCustomKeyboardImp::onButtonResetAllClicked()
{
    ShortcutManager::instance()->resetAll();
    ui->buttonReset->setEnabled(false);
}

/** Checks for an already occupied shortcut. */
void DlgCustomKeyboardImp::onEditShortcutTextChanged(const QString& )
{
    QTreeWidgetItem* item = ui->commandTreeWidget->currentItem();
    if (item) {
        QVariant data = item->data(1, Qt::UserRole);
        QByteArray name = data.toByteArray(); // command name

        CommandManager & cCmdMgr = Application::Instance->commandManager();
        Command* cmd = cCmdMgr.getCommandByName(name.constData());

        if (!ui->editShortcut->isNone())
            ui->buttonAssign->setEnabled(true);
        else {
            if (cmd && cmd->getAction() && cmd->getAction()->shortcut().isEmpty())
                ui->buttonAssign->setEnabled(false); // both key sequences are empty
        }
    }
}

void DlgCustomKeyboardImp::onAddMacroAction(const QByteArray&)
{
}

void DlgCustomKeyboardImp::onRemoveMacroAction(const QByteArray&)
{
}

void DlgCustomKeyboardImp::onModifyMacroAction(const QByteArray&)
{
    QVariant data = ui->categoryBox->itemData(ui->categoryBox->currentIndex(), Qt::UserRole);
    QString group = data.toString();
    if (group == QLatin1String("Macros")) {
        ui->categoryBox->activated(ui->categoryBox->currentIndex());
    }
}

void DlgCustomKeyboardImp::changeEvent(QEvent *e)
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
        ui->categoryBox->activated(ui->categoryBox->currentIndex());
    }
    else if (e->type() == QEvent::StyleChange) {
        ui->categoryBox->activated(ui->categoryBox->currentIndex());
    }

    QWidget::changeEvent(e);
}

#include "moc_DlgKeyboardImp.cpp"
