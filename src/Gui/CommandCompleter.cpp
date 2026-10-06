/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#include <algorithm>
#include <unordered_set>
#include <QApplication>
#include <QHash>
#include <QKeyEvent>
#include <QLineEdit>
#include <QAbstractItemView>
#include <QSortFilterProxyModel>
#include <QTextDocumentFragment>

#include "Application.h"
#include "ShortcutManager.h"
#include "Command.h"
#include "Action.h"
#include "BitmapFactory.h"
#include "CommandCompleter.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "Language/Translator.h"

using namespace Gui;

namespace
{

struct CmdInfo
{
    Command* cmd = nullptr;
    QIcon icon;
    bool iconChecked = false;
    // the texts are worked out once, they are thrown away with the list when commands, shortcuts
    // or the language change
    bool textCached = false;
    QString display;
    QString menuText;
    QString toolTip;
    QString group;
    // set before sorting
    bool active = true;
    int rank = 0;
};
std::vector<CmdInfo> _Commands;
int _CommandRevision;
std::string commandsLanguage;
bool _ShortcutSignalConnected = false;

void cacheText(CmdInfo& info)
{
    if (info.textCached) {
        return;
    }
    info.textCached = true;

    info.menuText = Action::commandMenuText(info.cmd);
    info.display = QStringLiteral("%1 (%2)").arg(info.menuText, QString::fromUtf8(info.cmd->getName()));
    QString shortcut = info.cmd->getShortcut();
    if (!shortcut.isEmpty()) {
        info.display += QStringLiteral(" [%1]").arg(shortcut);
        info.menuText += QStringLiteral(" [%1]").arg(shortcut);
    }
    info.toolTip = Action::commandToolTip(info.cmd, false);
    if (info.toolTip.contains(QLatin1Char('<'))) {
        info.toolTip = QTextDocumentFragment::fromHtml(info.toolTip).toPlainText();
    }
    info.group = QString::fromUtf8(info.cmd->getGroupName());
}

/// The commands on the active workbench's own toolbars, including those in their drop-down groups.
/// The standard toolbars are left out, every workbench shows them.
std::unordered_set<std::string> commandsOfActiveWorkbench()
{
    std::unordered_set<std::string> names;
    auto workbench = WorkbenchManager::instance()->active();
    if (!workbench) {
        return names;
    }

    std::unordered_set<std::string> standardCommands;
    for (const auto& toolbar : StdWorkbench().getToolbarItems()) {
        standardCommands.insert(toolbar.second.begin(), toolbar.second.end());
    }
    for (const auto& toolbar : workbench->getToolbarItems()) {
        for (const auto& name : toolbar.second) {
            if (standardCommands.count(name) == 0) {
                names.insert(name);
            }
        }
    }

    // Python groups name their commands in a property, the actions of C++ groups are the actions
    // of the commands themselves
    QHash<const QAction*, const char*> commandOfAction;
    for (const auto& info : _Commands) {
        auto action = info.cmd->getAction();
        if (action && action->action()) {
            commandOfAction.insert(action->action(), info.cmd->getName());
        }
    }
    auto& manager = Application::Instance->commandManager();
    const std::vector<std::string> toolbarCommands(names.begin(), names.end());
    for (const auto& name : toolbarCommands) {
        auto command = manager.getCommandByName(name.c_str());
        auto group = command ? qobject_cast<ActionGroup*>(command->getAction()) : nullptr;
        if (!group) {
            continue;
        }
        for (auto action : group->actions()) {
            QByteArray child = action->property("CommandName").toByteArray();
            if (child.isEmpty()) {
                child = commandOfAction.value(action);
            }
            if (!child.isEmpty()) {
                names.insert(child.toStdString());
            }
        }
    }
    return names;
}

class CommandModel: public QAbstractItemModel
{
    int revision = 0;
    bool filterInactive = false;

public:
    explicit CommandModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
        update();
        if (!_ShortcutSignalConnected) {
            _ShortcutSignalConnected = true;
            QObject::connect(ShortcutManager::instance(), &ShortcutManager::shortcutChanged, [] {
                _CommandRevision = 0;
            });
        }
    }

    void setFilterInactive(bool filter)
    {
        if (filterInactive != filter) {
            filterInactive = filter;
            // notify views that all data has changed (for greying out)
            if (!_Commands.empty()) {
                Q_EMIT dataChanged(
                    createIndex(0, 0),
                    createIndex(static_cast<int>(_Commands.size()) - 1, 0)
                );
            }
        }
    }

    /// Works out which commands can run and which belong to the active workbench. Returns true if
    /// the order changes.
    bool updateRanks()
    {
        // the group name of a command can't tell if it belongs to the active workbench, it differs
        // from the workbench name
        const std::unordered_set<std::string> workbenchCommands = commandsOfActiveWorkbench();
        bool changed = false;
        for (auto& info : _Commands) {
            cacheText(info);
            bool active = true;
            if (filterInactive) {
                auto action = info.cmd->getAction();
                // no action exists so assume inactive
                active = action && action->action() && action->action()->isEnabled();
            }
            // active commands first, then the ones of the active workbench
            const bool inWorkbench = workbenchCommands.count(info.cmd->getName()) > 0;
            int rank = (active ? 0 : 2) + (inWorkbench ? 0 : 1);
            if (active != info.active || rank != info.rank) {
                info.active = active;
                info.rank = rank;
                changed = true;
            }
        }
        if (changed && !_Commands.empty()) {
            Q_EMIT dataChanged(
                createIndex(0, 0),
                createIndex(static_cast<int>(_Commands.size()) - 1, 0)
            );
        }
        return changed;
    }

    void update()
    {
        auto& manager = Application::Instance->commandManager();
        const std::string language = Translator::instance()->activeLanguage();
        if (revision == _CommandRevision && _CommandRevision == manager.getRevision()
            && language == commandsLanguage) {
            return;
        }
        beginResetModel();
        revision = manager.getRevision();
        if (revision != _CommandRevision || language != commandsLanguage) {
            _CommandRevision = revision;
            _CommandRevision = manager.getRevision();
            commandsLanguage = language;
            _Commands.clear();
            for (auto& v : manager.getCommands()) {
                _Commands.emplace_back();
                auto& info = _Commands.back();
                info.cmd = v.second;
            }
        }
        endResetModel();
    }

    QModelIndex parent(const QModelIndex&) const override
    {
        return {};
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (index.row() < 0 || index.row() >= (int)_Commands.size()) {
            return {};
        }

        auto& info = _Commands[index.row()];
        if (role != Qt::DecorationRole && role != CommandNameRole) {
            cacheText(info);
        }

        switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                return info.display;

            case Qt::ToolTipRole:
                return info.toolTip;

            case Qt::DecorationRole:
                if (!info.iconChecked) {
                    info.iconChecked = true;
                    if (info.cmd->getPixmap()) {
                        info.icon = BitmapFactory().iconFromTheme(info.cmd->getPixmap());
                    }
                }
                return info.icon;

            case Qt::ForegroundRole:
                // grey out inactive commands
                if (filterInactive && !info.active) {
                    return QColor(Qt::gray);
                }
                break;

            case CommandNameRole:
                return QByteArray(info.cmd->getName());

            case CommandMenuTextRole:
                return info.menuText;

            case CommandGroupRole:
                return info.group;

            default:
                break;
        }
        return {};
    }

    QModelIndex index(int row, int, const QModelIndex&) const override
    {
        return this->createIndex(row, 0);
    }

    int rowCount(const QModelIndex&) const override
    {
        return (int)(_Commands.size());
    }

    int columnCount(const QModelIndex&) const override
    {
        return 1;
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid()) {
            return Qt::NoItemFlags;
        }

        const auto& info = _Commands[index.row()];

        // so if item is visible but not active, keep it, but don't add `ItemIsEnabled` so
        // it won't be possible to select it
        if (filterInactive && !info.active) {
            return Qt::ItemIsSelectable;
        }

        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
};

// proxy sort model to prioritize active commands before inactive ones
class CommandSortFilterProxyModel: public QSortFilterProxyModel
{
public:
    explicit CommandSortFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setSortRole(Qt::DisplayRole);
    }

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        const int count = static_cast<int>(_Commands.size());
        if (left.row() < 0 || left.row() >= count || right.row() < 0 || right.row() >= count) {
            return QSortFilterProxyModel::lessThan(left, right);
        }
        auto& leftInfo = _Commands[left.row()];
        auto& rightInfo = _Commands[right.row()];
        if (leftInfo.rank != rightInfo.rank) {
            return leftInfo.rank < rightInfo.rank;
        }
        cacheText(leftInfo);
        cacheText(rightInfo);
        return QString::compare(leftInfo.display, rightInfo.display, Qt::CaseInsensitive) < 0;
    }
};

}  // anonymous namespace

// --------------------------------------------------------------------

CommandCompleter::CommandCompleter(QLineEdit* lineedit, QObject* parent)
    : QCompleter(parent)
{
    auto sourceModel = new CommandModel(this);
    auto proxyModel = new CommandSortFilterProxyModel(this);
    proxyModel->setSourceModel(sourceModel);
    proxyModel->sort(0);

    this->setModel(proxyModel);
    this->setFilterMode(Qt::MatchContains);
    this->setCaseSensitivity(Qt::CaseInsensitive);
    this->setCompletionMode(QCompleter::PopupCompletion);
    this->setWidget(lineedit);
    connect(lineedit, &QLineEdit::textEdited, this, &CommandCompleter::onTextChanged);
    connect(
        this,
        qOverload<const QModelIndex&>(&CommandCompleter::activated),
        this,
        &CommandCompleter::onCommandActivated
    );
    connect(this, qOverload<const QString&>(&CommandCompleter::highlighted), lineedit, &QLineEdit::setText);
}

void CommandCompleter::setFilterInactive(bool filter)
{
    auto proxyModel = static_cast<CommandSortFilterProxyModel*>(this->model());
    if (!proxyModel) {
        return;
    }

    // pick up commands added since the last time, then sort again only when the order changes
    if (auto sourceModel = static_cast<CommandModel*>(proxyModel->sourceModel())) {
        sourceModel->update();
        sourceModel->setFilterInactive(filter);
        if (sourceModel->updateRanks()) {
            proxyModel->invalidate();
        }
    }
}

bool CommandCompleter::eventFilter(QObject* o, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress && (o == this->widget() || o == this->popup())) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
        switch (ke->key()) {
            case Qt::Key_Escape: {
                auto edit = qobject_cast<QLineEdit*>(this->widget());
                if (edit && edit->text().size()) {
                    edit->setText(QString());
                    popup()->hide();
                    return true;
                }
                else if (popup()->isVisible()) {
                    popup()->hide();
                    return true;
                }
                break;
            }
            case Qt::Key_Tab: {
                if (this->popup()->isVisible()) {
                    QKeyEvent kevent(ke->type(), Qt::Key_Down, Qt::NoModifier);
                    qApp->sendEvent(this->popup(), &kevent);
                    return true;
                }
                break;
            }
            case Qt::Key_Backtab: {
                if (this->popup()->isVisible()) {
                    QKeyEvent kevent(ke->type(), Qt::Key_Up, Qt::NoModifier);
                    qApp->sendEvent(this->popup(), &kevent);
                    return true;
                }
                break;
            }
            case Qt::Key_Enter:
            case Qt::Key_Return:
                if (o == this->widget()) {
                    auto index = currentIndex();
                    if (index.isValid()) {
                        onCommandActivated(index);
                    }
                    else {
                        complete();
                    }
                    ev->setAccepted(true);
                    return true;
                }
            default:
                break;
        }
    }
    return QCompleter::eventFilter(o, ev);
}

void CommandCompleter::onCommandActivated(const QModelIndex& index)
{
    QByteArray name = completionModel()->data(index, CommandNameRole).toByteArray();
    Q_EMIT commandActivated(name);
}

void CommandCompleter::onTextChanged(const QString& txt)
{
    // Do not activate completer if less than 3 characters for better
    // performance, unless called explicitly via complete()
    if (txt.size() < 3 && txt.size() > 0) {
        return;
    }

    // get the source model through the proxy model
    auto proxyModel = static_cast<CommandSortFilterProxyModel*>(this->model());
    if (proxyModel) {
        auto sourceModel = static_cast<CommandModel*>(proxyModel->sourceModel());
        if (sourceModel) {
            sourceModel->update();
        }
    }

    this->setCompletionPrefix(txt);
    QRect rect = widget()->rect();
    if (rect.width() < 300) {
        rect.setWidth(300);
    }
    this->complete(rect);
}

#include "moc_CommandCompleter.cpp"
