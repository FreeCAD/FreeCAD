/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <QListWidget>

class QPlainTextEdit;

namespace Py
{
class Object;
class List;
class String;
}  // namespace Py
namespace Gui
{

class CallTip
{
public:
    enum Type
    {
        Unknown,
        Module,
        Class,
        Method,
        Member,
        Property
    };
    QString name;
    QString description;
    QString parameter;
    Type type {Unknown};
};

/**
 * @author Werner Mayer
 */
class CallTipsList: public QListWidget
{
    Q_OBJECT

public:
    /// Construction
    CallTipsList(QPlainTextEdit* parent);
    /// Destruction
    ~CallTipsList() override;

    void keyboardSearch(const QString&) override;
    void showTips(const QString&);
    void validateCursor();

protected:
    bool eventFilter(QObject*, QEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

private Q_SLOTS:
    void callTipItemActivated(QListWidgetItem* item);

private:
    QString extractContext(const QString&) const;
    QMap<QString, CallTip> extractTips(const QString&) const;
    void extractTipsFromObject(Py::Object&, Py::List&, QMap<QString, CallTip>&) const;
    void extractTipsFromProperties(Py::Object&, QMap<QString, CallTip>&) const;
    QString stripWhiteSpace(const QString&) const;
    Py::Object getAttrWorkaround(Py::Object&, Py::String&) const;

private:
    QPlainTextEdit* textEdit;
    int cursorPos;
    mutable bool validObject;
    bool doCallCompletion;
    QList<int> hideKeys;
    QList<int> compKeys;
};

}  // namespace Gui
