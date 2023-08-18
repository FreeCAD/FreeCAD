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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QApplication>
# include <QKeyEvent>
# include <QLineEdit>
# include <QAbstractItemView>
#endif

#include "Application.h"
#include "ShortcutManager.h"
#include "Command.h"
#include "Action.h"
#include "BitmapFactory.h"
#include "CommandCompleter.h"

using namespace Gui;

namespace {

struct CmdInfo {
    Command *cmd = nullptr;
    QIcon icon;
    bool iconChecked = false;
};
std::vector<CmdInfo> _Commands;
int _CommandRevision;
const int CommandNameRole = Qt::UserRole;
bool _ShortcutSignalConnected = false;

class CommandModel : public QAbstractItemModel
{
    int revision = 0;

public:
    explicit CommandModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
        update();
        if (!_ShortcutSignalConnected) {
            _ShortcutSignalConnected = true;
            QObject::connect(ShortcutManager::instance(), &ShortcutManager::shortcutChanged, []{_CommandRevision = 0;});
        }
    }

    void update()
    {
        auto &manager = Application::Instance->commandManager();
        if (revision == _CommandRevision  && _CommandRevision == manager.getRevision())
            return;
        beginResetModel();
        revision = manager.getRevision();
        if (revision != _CommandRevision) {
            _CommandRevision = revision;
            _CommandRevision = manager.getRevision();
            _Commands.clear();
            for (auto &v : manager.getCommands()) {
                _Commands.emplace_back();
                auto &info = _Commands.back();
                info.cmd = v.second;
            }
        }
        endResetModel();
    }

    QModelIndex parent(const QModelIndex &) const override
    {
        return {};
    }

    QVariant data(const QModelIndex & index, int role) const override
    {
        if (index.row() < 0 || index.row() >= (int)_Commands.size())
            return {};

        auto &info = _Commands[index.row()];

        switch(role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            QString title = QStringLiteral("%1 (%2)").arg(
                    Action::commandMenuText(info.cmd),
                    QString::fromUtf8(info.cmd->getName()));
            QString shortcut = info.cmd->getShortcut();
            if (!shortcut.isEmpty())
                title += QStringLiteral(" (%1)").arg(shortcut);
            return title;
        }
        case Qt::ToolTipRole:
            return Action::commandToolTip(info.cmd);

        case Qt::DecorationRole:
            if (!info.iconChecked) {
                info.iconChecked = true;
                if(info.cmd->getPixmap())
                    info.icon = BitmapFactory().iconFromTheme(info.cmd->getPixmap());
            }
            return info.icon;

        case CommandNameRole:
            return QByteArray(info.cmd->getName());

        default:
            break;
        }
        return {};
    }

    QModelIndex index(int row, int, const QModelIndex &) const override
    {
        return this->createIndex(row, 0);
    }

    int rowCount(const QModelIndex &) const override
    {
        return (int)(_Commands.size());
    }

    int columnCount(const QModelIndex &) const override
    {
        return 1;
    }
};

} // anonymous namespace

// --------------------------------------------------------------------

CommandCompleter::CommandCompleter(QLineEdit *lineedit, QObject *parent)
    : QCompleter(parent)
{
    this->setModel(new CommandModel(this));
    this->setFilterMode(Qt::MatchContains);
    this->setCaseSensitivity(Qt::CaseInsensitive);
    this->setCompletionMode(QCompleter::PopupCompletion);
    this->setWidget(lineedit);
    connect(lineedit, &QLineEdit::textEdited, this, &CommandCompleter::onTextChanged);
    connect(this, qOverload<const QModelIndex&>(&CommandCompleter::activated),
            this, &CommandCompleter::onCommandActivated);
    connect(this, qOverload<const QString&>(&CommandCompleter::highlighted),
            lineedit, &QLineEdit::setText);
}

bool CommandCompleter::eventFilter(QObject *o, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress
            && (o == this->widget() || o == this->popup()))
    {
        QKeyEvent * ke = static_cast<QKeyEvent*>(ev);
        switch(ke->key()) {
        case Qt::Key_Escape: {
            auto edit = qobject_cast<QLineEdit*>(this->widget());
            if (edit && edit->text().size()) {
                edit->setText(QString());
                popup()->hide();
                return true;
            } else if (popup()->isVisible()) {
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
                if (index.isValid())
                    onCommandActivated(index);
                else
                    complete();
                ev->setAccepted(true);
                return true;
            }
        default:
            break;
        }
    }
    return QCompleter::eventFilter(o, ev);
}

void CommandCompleter::onCommandActivated(const QModelIndex &index)
{
    QByteArray name = completionModel()->data(index, CommandNameRole).toByteArray();
    Q_EMIT commandActivated(name);
}

void CommandCompleter::onTextChanged(const QString &txt)
{
    // Do not activate completer if less than 3 characters for better
    // performance.
    if (txt.size() < 3 || !widget())
        return;

    static_cast<CommandModel*>(this->model())->update();

    this->setCompletionPrefix(txt);
    QRect rect = widget()->rect();
    if (rect.width() < 300)
        rect.setWidth(300);
    this->complete(rect);
}

#include "moc_CommandCompleter.cpp"
