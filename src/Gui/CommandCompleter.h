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

#pragma once

#include <FCGlobal.h>
#include <QCompleter>

class QLineEdit;

namespace Gui
{

/**
 * Command name auto completer.
 *
 * This class provides an auto completer for a QLineEdit widget. The auto
 * completer supports keyword search in command title, internal name, and
 * shortcut.
 */
class GuiExport CommandCompleter: public QCompleter
{
    Q_OBJECT
public:
    explicit CommandCompleter(QLineEdit* edit, QObject* parent = nullptr);

Q_SIGNALS:
    /// Triggered when a command is selected in the completer
    void commandActivated(const QByteArray& name);

protected Q_SLOTS:
    void onTextChanged(const QString&);
    void onCommandActivated(const QModelIndex&);

protected:
    bool eventFilter(QObject*, QEvent* ev) override;
};

}  // namespace Gui
