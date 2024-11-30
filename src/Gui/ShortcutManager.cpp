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
# include <QShortcutEvent>
# include <QApplication>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <Base/Tools.h>
#include "ShortcutManager.h"
#include "Command.h"
#include "Window.h"
#include "Action.h"

using namespace Gui;

ShortcutManager::ShortcutManager()
{
    hShortcuts = WindowParameter::getDefaultParameter()->GetGroup("Shortcut");
    hShortcuts->Attach(this);
    hPriorities = hShortcuts->GetGroup("Priorities");
    hPriorities->Attach(this);
    hSetting = hShortcuts->GetGroup("Settings");
    hSetting->Attach(this);
    timeout = hSetting->GetInt("ShortcutTimeout", 300);
    timer.setSingleShot(true);

    QObject::connect(&timer, &QTimer::timeout, [this](){onTimer();});

    topPriority = 0;
    for (const auto &v : hPriorities->GetIntMap()) {
        priorities[v.first] = v.second;
        if (topPriority < v.second)
            topPriority = v.second;
    }
    if (topPriority == 0)
        topPriority = 100;

    QApplication::instance()->installEventFilter(this);
}

ShortcutManager::~ShortcutManager()
{
    hShortcuts->Detach(this);
    hSetting->Detach(this);
    hPriorities->Detach(this);
}

static ShortcutManager *Instance;
ShortcutManager *ShortcutManager::instance()
{
    if (!Instance)
        Instance = new ShortcutManager;
    return Instance;
}

void ShortcutManager::destroy()
{
    delete Instance;
    Instance = nullptr;
}

void ShortcutManager::OnChange(Base::Subject<const char*> &src, const char *reason)
{
    if (hSetting == &src) {
        if (boost::equals(reason, "ShortcutTimeout"))
            timeout = hSetting->GetInt("ShortcutTimeout");
        return;
    }

    if (busy)
        return;

    if (hPriorities == &src) {
        int p = hPriorities->GetInt(reason, 0);
        if (p == 0)
            priorities.erase(reason);
        else
            priorities[reason] = p;
        if (topPriority < p)
            topPriority = p;
        priorityChanged(reason, p);
        return;
    }

    Base::StateLocker lock(busy);
    auto cmd = Application::Instance->commandManager().getCommandByName(reason);
    if (cmd) {
        auto accel = cmd->getAccel();
        if (!accel) accel = "";
        QKeySequence oldShortcut = cmd->getShortcut();
        QKeySequence newShortcut = getShortcut(reason, accel);
        if (oldShortcut != newShortcut) {
            cmd->setShortcut(newShortcut.toString());
            shortcutChanged(reason, oldShortcut);
        }
    }
}

void ShortcutManager::reset(const char *cmd)
{
    if (cmd && cmd[0]) {
        QKeySequence oldShortcut = getShortcut(cmd);
        hShortcuts->RemoveASCII(cmd);
        if (oldShortcut != getShortcut(cmd))
            shortcutChanged(cmd, oldShortcut);

        int oldPriority = getPriority(cmd);
        hPriorities->RemoveInt(cmd);
        if (oldPriority != getPriority(cmd))
            priorityChanged(cmd, oldPriority);
    }
}

void ShortcutManager::resetAll()
{
    {
        Base::StateLocker lock(busy);
        hShortcuts->Clear();
        hPriorities->Clear();
        for (auto cmd : Application::Instance->commandManager().getAllCommands()) {
            if (cmd->getAction()) {
                auto accel = cmd->getAccel();
                if (!accel) accel = "";
                cmd->setShortcut(getShortcut(nullptr, accel));
            }
        }
    }
    shortcutChanged("", QKeySequence());
    priorityChanged("", 0);
}

QString ShortcutManager::getShortcut(const char *cmdName, const char *accel)
{
    if (!accel) {
        if (auto cmd = Application::Instance->commandManager().getCommandByName(cmdName)) {
            accel = cmd->getAccel();
            if (!accel)
                accel = "";
        }
    }
    QString shortcut;
    if (cmdName)
        shortcut = QString::fromLatin1(hShortcuts->GetASCII(cmdName, accel).c_str());
    else
        shortcut = QString::fromLatin1(accel);
    return QKeySequence(shortcut).toString(QKeySequence::NativeText);
}

void ShortcutManager::setShortcut(const char *cmdName, const char *accel)
{
    if (cmdName && cmdName[0]) {
        setTopPriority(cmdName);
        if (!accel)
            accel = "";
        if (auto cmd = Application::Instance->commandManager().getCommandByName(cmdName)) {
            auto defaultAccel = cmd->getAccel();
            if (!defaultAccel)
               defaultAccel = "";
           if (QKeySequence(QString::fromLatin1(accel)) == QKeySequence(QString::fromLatin1(defaultAccel))) {
                hShortcuts->RemoveASCII(cmdName);
                return;
           }
        }
        hShortcuts->SetASCII(cmdName, accel);
    }
}

bool ShortcutManager::checkShortcut(QObject *o, const QKeySequence &key)
{
    auto focus = QApplication::focusWidget();
    if (!focus)
        return false;
    auto action = qobject_cast<QAction*>(o);
    if (!action)
        return false;

    const auto &index = actionMap.get<1>();
    auto iter = index.lower_bound(ActionKey(key));
    if (iter == index.end())
        return false;

    // disable and enqueue the action in order to try other alternativeslll
    action->setEnabled(false);
    pendingActions.emplace_back(action, key.count(), 0);

    // check for potential partial match, i.e. longer key sequences
    bool flush = true;
    bool found = false;
    for (auto it = iter; it != index.end(); ++it) {
        if (key.matches(it->key.shortcut) == QKeySequence::NoMatch)
            break;
        if (action == it->action) {
            // There maybe more than one action with the exact same shortcut.
            // However, we only disable and enqueue the triggered action.
            // Because, QAction::isEnabled() does not check if the action is
            // active under its current ShortcutContext. We would have to check
            // its parent widgets visibility which may or may not be reliable.
            // Instead, we rely on QEvent::Shortcut to be sure to enqueue only
            // active shortcuts. We'll fake the current key sequence below,
            // which will trigger all possible matches one by one.
            pendingActions.back().priority = getPriority(it->key.name);
            found = true;
        }
        else if (it->action && it->action->isEnabled()) {
            flush = false;
            if (found)
                break;
        }
    }

    if (flush) {
        // We'll flush now because there is no potential match with further
        // keystrokes, so no need to wait for timer.
        lastFocus = nullptr;
        onTimer();
        return true;
    }

    lastFocus = focus;
    pendingSequence = key;

    // Qt's shortcut state machine favors shortest match (which is ridiculous,
    // unless I'm mistaken?). We'll do longest match. We've disabled all
    // shortcuts that can match the current key sequence. Now replay the sequence
    // and wait for the next keystroke.
    for (int i=0; i<key.count(); ++i) {
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        int k = key[i];
#else
        int k = key[i].key();
#endif
        Qt::KeyboardModifiers modifiers;
        if ((k & Qt::SHIFT) == Qt::SHIFT)
            modifiers |= Qt::ShiftModifier;
        if ((k & Qt::CTRL) == Qt::CTRL)
            modifiers |= Qt::ControlModifier;
        if ((k & Qt::ALT) == Qt::ALT)
            modifiers |= Qt::AltModifier;
        if ((k & Qt::META) == Qt::META)
            modifiers |= Qt::MetaModifier;
        k &= ~(Qt::SHIFT|Qt::CTRL|Qt::ALT|Qt::META);
        QKeyEvent *kev = new QKeyEvent(QEvent::KeyPress, k, modifiers, 0, 0, 0);
        QApplication::postEvent(focus, kev);
        kev = new QKeyEvent(QEvent::KeyRelease, k, modifiers, 0, 0, 0);
        QApplication::postEvent(focus, kev);
    }
    timer.start(timeout);
    return true;
}

bool ShortcutManager::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::KeyPress:
        lastFocus = nullptr;
        break;
    case QEvent::Shortcut:
        if (timeout > 0) {
            auto sev = static_cast<QShortcutEvent*>(ev);
            if (checkShortcut(o, sev->key())) {
                // shortcut event handled here, so filter out the event
                return true;
            } else {
                // Not handled. Clear any existing pending actions.
                timer.stop();
                for (const auto &info : pendingActions) {
                    if (info.action)
                        info.action->setEnabled(true);
                }
                pendingActions.clear();
                lastFocus = nullptr;
            }
        }
        break;
    case QEvent::ActionChanged:
        if (auto action = qobject_cast<QAction*>(o)) {
            auto &index = actionMap.get<0>();
            auto it = index.find(reinterpret_cast<intptr_t>(action));
            if (action->shortcut().isEmpty()) {
                if (it != index.end()) {
                    QKeySequence oldShortcut = it->key.shortcut;
                    index.erase(it);
                    actionShortcutChanged(action, oldShortcut);
                }
                break;
            }

            QByteArray name;
            if (auto fcAction = qobject_cast<Action*>(action->parent())) {
                if (fcAction->command() && fcAction->command()->getName())
                    name = fcAction->command()->getName();
            }
            if (name.isEmpty()) {
                name = action->objectName().size() ?
                    action->objectName().toUtf8() : action->text().toUtf8();
                if (name.isEmpty())
                    name = "~";
                else
                    name = QByteArray("~ ") + name;
            }
            if (it != index.end()) {
                if (it->key.shortcut == action->shortcut() && it->key.name == name)
                    break;
                QKeySequence oldShortcut = it->key.shortcut;
                index.replace(it, ActionData{action, name});
                actionShortcutChanged(action, oldShortcut);
            } else {
                index.insert(ActionData{action, name});
                actionShortcutChanged(action, QKeySequence());
            }
        }
        break;
    default:
        break;
    }
    return false;
}

std::vector<std::pair<QByteArray, QAction*>> ShortcutManager::getActionsByShortcut(const QKeySequence &shortcut)
{
    const auto &index = actionMap.get<1>();
    std::vector<std::pair<QByteArray, QAction*>> res;
    std::multimap<int, const ActionData*, std::greater<>> map;
    for (auto it = index.lower_bound(ActionKey(shortcut)); it != index.end(); ++it) {
        if (it->key.shortcut != shortcut)
            break;
        if (it->key.name != "~" && it->action)
            map.emplace(getPriority(it->key.name), &(*it));
    }
    for (const auto &v : map)
        res.emplace_back(v.second->key.name, v.second->action);
    return res;
}

void ShortcutManager::setPriorities(const std::vector<QByteArray> &actions)
{
    if (actions.empty())
        return;
    // Keep the same top priority of the given action, and adjust the rest. Can
    // go negative if necessary
    int current = 0;
    for (const auto &name : actions)
        current = std::max(current, getPriority(name));
    if (current == 0)
        current = (int)actions.size();
    setPriority(actions.front(), current);
    ++current;
    for (const auto &name : actions) {
        int p = getPriority(name);
        if (p <= 0 || p >= current) {
            if (--current == 0)
                --current;
            setPriority(name, current);
        } else
            current = p;
    }
}

int ShortcutManager::getPriority(const char *cmdName)
{
    if (!cmdName)
        return 0;
    auto it = priorities.find(cmdName);
    if (it == priorities.end())
        return 0;
    return it->second;
}

void ShortcutManager::setPriority(const char *cmdName, int p)
{
    if (p == 0)
        hPriorities->RemoveInt(cmdName);
    else
        hPriorities->SetInt(cmdName, p);
}

void ShortcutManager::setTopPriority(const char *cmdName)
{
    ++topPriority;
    hPriorities->SetInt(cmdName, topPriority);
}

void ShortcutManager::onTimer()
{
    timer.stop();

    QAction *found = nullptr;
    int priority = -INT_MAX;
    int seq_length = 0;
    for (const auto &info : pendingActions) {
        if (info.action) {
            info.action->setEnabled(true);
            if (info.seq_length > seq_length
                    || (info.seq_length == seq_length
                        && info.priority > priority))
            {
                priority = info.priority;
                seq_length = info.seq_length;
                found = info.action;
            }
        }
    }
    if (found)
        found->activate(QAction::Trigger);
    pendingActions.clear();

    if (lastFocus && lastFocus == QApplication::focusWidget()) {
        // We are here because we have withheld some previous triggered action.
        // We then disabled the action, and faked the same key strokes in order
        // to wait for more potential match of longer key sequence. We use
        // a timer to end the wait and trigger the pending action.
        //
        // However, Qt's internal shorcutmap state machine is still armed with
        // our fake key strokes. So we try to fake some more obscure symbol key
        // stroke below, hoping to reset Qt's state machine.

        const auto &index = actionMap.get<1>();
        static const std::string symbols = "~!@#$%^&*()_+";
        QString shortcut = pendingSequence.toString() + QStringLiteral(", Ctrl+");
        for (int s : symbols) {
            QKeySequence k(shortcut + QLatin1Char(s));
            auto it = index.lower_bound(ActionKey(k));
            if (it->key.shortcut != k) {
                QKeyEvent *kev = new QKeyEvent(QEvent::KeyPress, s, Qt::ControlModifier, 0, 0, 0);
                QApplication::postEvent(lastFocus, kev);
                kev = new QKeyEvent(QEvent::KeyRelease, s, Qt::ControlModifier, 0, 0, 0);
                QApplication::postEvent(lastFocus, kev);
                break;
            }
        }
    }
}

#include "moc_ShortcutManager.cpp"
