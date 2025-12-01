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
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QAbstractItemView>
#include <QSortFilterProxyModel>

#include "Application.h"
#include "ShortcutManager.h"
#include "Command.h"
#include "Action.h"
#include "BitmapFactory.h"
#include "CommandCompleter.h"
#include "WorkbenchManager.h"

using namespace Gui;

namespace
{

struct CmdInfo
{
    Command* cmd = nullptr;
    QIcon icon;
    bool iconChecked = false;
};
std::vector<CmdInfo> _Commands;
int _CommandRevision;
const int CommandNameRole = Qt::UserRole;
const int CommandMenuTextRole = Qt::UserRole + 1;  // menu text only (for palette)
const int CommandGroupRole = Qt::UserRole + 2;     // group/workbench name
bool _ShortcutSignalConnected = false;

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
                QAbstractItemModel::dataChanged(createIndex(0, 0), createIndex(_Commands.size() - 1, 0));
            }
        }
    }

    void update()
    {
        auto& manager = Application::Instance->commandManager();
        if (revision == _CommandRevision && _CommandRevision == manager.getRevision()) {
            return;
        }
        beginResetModel();
        revision = manager.getRevision();
        if (revision != _CommandRevision) {
            _CommandRevision = revision;
            _CommandRevision = manager.getRevision();
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

        // check if command is active to grey out if not
        bool isActive = true;
        if (filterInactive) {
            if (info.cmd->getAction() && info.cmd->getAction()->action()) {
                isActive = info.cmd->getAction()->action()->isEnabled();
            }
            else {
                // no action exists so assume inactive
                isActive = false;
            }
        }

        switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole: {
                QString title = QStringLiteral("%1 (%2)").arg(
                    Action::commandMenuText(info.cmd),
                    QString::fromUtf8(info.cmd->getName())
                );
                QString shortcut = info.cmd->getShortcut();
                if (!shortcut.isEmpty()) {
                    title += QStringLiteral(" [%1]").arg(shortcut);
                }
                return title;
            }
            case Qt::ToolTipRole:
                // return just the tooltip text without formatting for the description line
                // (richFormat = false)
                return Action::commandToolTip(info.cmd, false);

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
                if (!isActive) {
                    return QColor(Qt::gray);
                }
                break;

            case CommandNameRole:
                return QByteArray(info.cmd->getName());

            // custom role for menu text only (without internal name)
            case CommandMenuTextRole: {
                QString title = Action::commandMenuText(info.cmd);
                QString shortcut = info.cmd->getShortcut();
                if (!shortcut.isEmpty()) {
                    title += QStringLiteral(" [%1]").arg(shortcut);
                }
                return title;
            }

            // custom role for workbench/group name
            case CommandGroupRole:
                return QString::fromUtf8(info.cmd->getGroupName());

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

        bool isActive = true;
        if (filterInactive) {
            if (info.cmd->getAction() && info.cmd->getAction()->action()) {
                isActive = info.cmd->getAction()->action()->isEnabled();
            }
            else {
                // no action exists, so assume inactive
                isActive = false;
            }
        }

        // so if item is visible but not active, keep it, but don't add `ItemIsEnabled` so
        // it won't be possible to select it
        if (!isActive) {
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
        auto sourceModel = static_cast<CommandModel*>(this->sourceModel());
        if (!sourceModel) {
            return QSortFilterProxyModel::lessThan(left, right);
        }

        std::string activeWorkbench = WorkbenchManager::instance()->activeName();
        if (left.row() < 0 || left.row() >= static_cast<int>(_Commands.size()) || right.row() < 0
            || right.row() >= static_cast<int>(_Commands.size())) {
            return QSortFilterProxyModel::lessThan(left, right);
        }

        const Command* leftCmd = _Commands[left.row()].cmd;
        const Command* rightCmd = _Commands[right.row()].cmd;

        if (!leftCmd || !rightCmd) {
            return QSortFilterProxyModel::lessThan(left, right);
        }

        // check if command is active and prioritize active one if the other is not
        bool leftActive = (sourceModel->flags(left) & Qt::ItemIsEnabled) != 0;
        bool rightActive = (sourceModel->flags(right) & Qt::ItemIsEnabled) != 0;
        if (leftActive != rightActive) {
            return leftActive > rightActive;
        }

        // next prioritize commands that are from the same workbench
        // (currently used)
        std::string leftGroup = leftCmd->getGroupName();
        std::string rightGroup = rightCmd->getGroupName();
        bool leftIsActiveWB = (leftGroup == activeWorkbench);
        bool rightIsActiveWB = (rightGroup == activeWorkbench);

        if (leftIsActiveWB != rightIsActiveWB) {
            return leftIsActiveWB > rightIsActiveWB;
        }

        // use alphabetic sorting as last resort
        return QSortFilterProxyModel::lessThan(left, right);
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

    // get source model and set filter flag
    // also re-sort after changing the filter state just to be sure
    // we get most fresh data
    if (auto sourceModel = static_cast<CommandModel*>(proxyModel->sourceModel())) {
        sourceModel->setFilterInactive(filter);
        proxyModel->invalidate();
        proxyModel->sort(0);
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
