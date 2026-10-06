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
    QString title;
    QString display;
    QString menuText;
    QString toolTip;
    QString group;
    // set before sorting
    bool active = true;
    int rank = 0;
    int match = 0;
    // a drop-down whose entries are all listed as commands of their own
    bool coveredGroup = false;
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

    info.title = Action::commandMenuText(info.cmd);
    info.menuText = info.title;
    info.display = QStringLiteral("%1 (%2)").arg(info.title, QString::fromUtf8(info.cmd->getName()));
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

/// How well a title matches the search text: 0 when it starts with it, 1 when one of its words
/// does, 2 otherwise.
int matchQuality(const QString& title, const QString& text)
{
    if (text.isEmpty() || title.startsWith(text, Qt::CaseInsensitive)) {
        return 0;
    }
    for (qsizetype i = 1; i < title.size(); ++i) {
        if (!title.at(i - 1).isLetterOrNumber()
            && QStringView(title).mid(i).startsWith(text, Qt::CaseInsensitive)) {
            return 1;
        }
    }
    return 2;
}

/// The command of each action. The actions in the drop-down of a C++ group are the actions of its
/// commands.
QHash<const QAction*, const char*> commandsByAction()
{
    QHash<const QAction*, const char*> commandOfAction;
    for (const auto& info : _Commands) {
        auto action = info.cmd->getAction();
        if (action && action->action()) {
            commandOfAction.insert(action->action(), info.cmd->getName());
        }
    }
    return commandOfAction;
}

struct GroupEntries
{
    std::vector<QByteArray> commands;
    // false when the drop-down holds an entry that isn't a command of its own
    bool allCommands = false;
};

/// The commands in the drop-down of a group command. Python groups name them in a property.
GroupEntries entriesOfGroup(Command* command, const QHash<const QAction*, const char*>& commandOfAction)
{
    GroupEntries entries;
    auto group = qobject_cast<ActionGroup*>(command->getAction());
    if (!group) {
        return entries;
    }
    entries.allCommands = true;
    for (auto action : group->actions()) {
        if (action->isSeparator()) {
            continue;
        }
        QByteArray name = action->property("CommandName").toByteArray();
        if (name.isEmpty()) {
            name = commandOfAction.value(action);
        }
        if (name.isEmpty()) {
            entries.allCommands = false;
        }
        else {
            entries.commands.push_back(name);
        }
    }
    return entries;
}

/// The commands on the active workbench's own toolbars, including those in their drop-down groups.
/// The standard toolbars are left out, every workbench shows them.
std::unordered_set<std::string> commandsOfActiveWorkbench(
    const QHash<const QAction*, const char*>& commandOfAction
)
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

    auto& manager = Application::Instance->commandManager();
    const std::vector<std::string> toolbarCommands(names.begin(), names.end());
    for (const auto& name : toolbarCommands) {
        if (auto command = manager.getCommandByName(name.c_str())) {
            for (const auto& child : entriesOfGroup(command, commandOfAction).commands) {
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
        const auto commandOfAction = commandsByAction();
        // the group name of a command can't tell if it belongs to the active workbench, it differs
        // from the workbench name
        const auto workbenchCommands = commandsOfActiveWorkbench(commandOfAction);
        bool changed = false;
        for (auto& info : _Commands) {
            cacheText(info);
            bool active = true;
            bool coveredGroup = false;
            if (filterInactive) {
                auto action = info.cmd->getAction();
                // no action exists so assume inactive
                active = action && action->action() && action->action()->isEnabled();
                const auto entries = entriesOfGroup(info.cmd, commandOfAction);
                coveredGroup = entries.allCommands && !entries.commands.empty();
            }
            // active commands first, then the ones of the active workbench
            const bool inWorkbench = workbenchCommands.count(info.cmd->getName()) > 0;
            int rank = (active ? 0 : 2) + (inWorkbench ? 0 : 1);
            if (active != info.active || rank != info.rank || coveredGroup != info.coveredGroup) {
                info.active = active;
                info.rank = rank;
                info.coveredGroup = coveredGroup;
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

    /// Works out how well each command matches the search text. Returns true if the order changes.
    bool setSearchText(const QString& text)
    {
        bool changed = false;
        for (auto& info : _Commands) {
            cacheText(info);
            const int match = matchQuality(info.title, text);
            if (match != info.match) {
                info.match = match;
                changed = true;
            }
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
        // another model may have rebuilt the shared list already
        if (!index.isValid() || index.row() >= static_cast<int>(_Commands.size())) {
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

    /// The command palette ranks the commands and leaves out drop-downs whose entries are listed
    /// anyway; other command searches list all of them alphabetically. Returns true if this changes.
    bool setPaletteMode(bool palette)
    {
        const bool changed = paletteMode != palette;
        paletteMode = palette;
        return changed;
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        Q_UNUSED(sourceParent)
        if (!paletteMode || sourceRow < 0 || sourceRow >= static_cast<int>(_Commands.size())) {
            return true;
        }
        return !_Commands[sourceRow].coveredGroup;
    }

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        const int count = static_cast<int>(_Commands.size());
        if (left.row() < 0 || left.row() >= count || right.row() < 0 || right.row() >= count) {
            return QSortFilterProxyModel::lessThan(left, right);
        }
        auto& leftInfo = _Commands[left.row()];
        auto& rightInfo = _Commands[right.row()];
        // the ranks are kept with the shared command list, so only the palette may use them
        if (paletteMode && leftInfo.rank != rightInfo.rank) {
            return leftInfo.rank < rightInfo.rank;
        }
        if (paletteMode && leftInfo.match != rightInfo.match) {
            return leftInfo.match < rightInfo.match;
        }
        cacheText(leftInfo);
        cacheText(rightInfo);
        return QString::compare(leftInfo.display, rightInfo.display, Qt::CaseInsensitive) < 0;
    }

private:
    bool paletteMode = false;
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
        const bool reordered = proxyModel->setPaletteMode(filter);
        if (sourceModel->updateRanks() || reordered) {
            proxyModel->invalidate();
        }
    }
}

void CommandCompleter::setSearchText(const QString& text)
{
    auto proxyModel = static_cast<CommandSortFilterProxyModel*>(this->model());
    if (!proxyModel) {
        return;
    }
    if (auto sourceModel = static_cast<CommandModel*>(proxyModel->sourceModel())) {
        if (sourceModel->setSearchText(text)) {
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
    // performance.
    if (txt.size() < 3 || !widget()) {
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
