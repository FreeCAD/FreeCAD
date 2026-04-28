// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Thomas Anderson <ta@nextgenengineering>            *
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

#include <QAbstractListModel>
#include <QComboBox>
#include <QListView>
#include <QTreeView>
#include "PropertyPage.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

class Command;
class QPushButton;

namespace Gui
{
namespace Dialog
{
class ButtonView: public QListView
{
    Q_OBJECT
public:
    explicit ButtonView(QWidget* parent = nullptr);
    void selectButton(int number);
Q_SIGNALS:
    void changeCommandSelection(const QString& commandName);
public Q_SLOTS:
    void goSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void goChangedCommand(const QString& commandName);
};

class ButtonModel: public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ButtonModel(QObject* parent);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void insertButtonRows(int number);
    void setCommand(int row, QString command);
    void goButtonPress(int number);
    void goMacroRemoved(const QByteArray& macroName);
    void goClear();
    void loadConfig(const char* RequiredDeviceName);

private:
    void load3DConnexionButtonMapping(boost::property_tree::ptree ButtonMapTree);
    void load3DConnexionButtons(const char* RequiredDeviceName);
    ParameterGrp::handle spaceballButtonGroup() const;
    QString getLabel(const int& number) const;
};

class CommandView: public QTreeView
{
    Q_OBJECT
public:
    explicit CommandView(QWidget* parent = nullptr);
public Q_SLOTS:
    void goChangeCommandSelection(const QString& commandName);
private Q_SLOTS:
    void goClicked(const QModelIndex& index);
Q_SIGNALS:
    void changedCommand(const QString& commandName);
};

class CommandNode
{
public:
    enum NodeType
    {
        RootType,
        GroupType,
        CommandType
    };

    explicit CommandNode(NodeType typeIn);
    ~CommandNode();

    NodeType nodeType;
    Command* aCommand;
    QString labelText;
    CommandNode* parent;
    QList<CommandNode*> children;
};

class CommandModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit CommandModel(QObject* parent = nullptr);
    ~CommandModel() override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    void goAddMacro(const QByteArray& macroName);
    void goRemoveMacro(const QByteArray& macroName);

private:
    CommandNode* rootNode;
    CommandNode* nodeFromIndex(const QModelIndex& index) const;
    void initialize();
    void groupCommands(const QString& groupName);
    QStringList orderedGroups();
};

class PrintModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    PrintModel(QObject* parent, ButtonModel* buttonModelIn, CommandModel* commandModelIn);
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    ButtonModel* buttonModel;
    CommandModel* commandModel;
};

class DlgCustomizeSpaceball: public CustomizeActionPage
{
    Q_OBJECT
public:
    explicit DlgCustomizeSpaceball(QWidget* parent = nullptr);
    ~DlgCustomizeSpaceball() override;

protected:
    void changeEvent(QEvent* e) override;
    bool event(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

protected Q_SLOTS:
    void onAddMacroAction(const QByteArray& macroName) override;
    void onRemoveMacroAction(const QByteArray& macroName) override;
    void onModifyMacroAction(const QByteArray& macroName) override;

private Q_SLOTS:
    void goClear();
    void goPrint();

private:
    void setupButtonModelView();
    void setupCommandModelView();
    void setupLayout();
    void setMessage(const QString& message);
    QStringList getModels();

    ButtonView* buttonView;
    ButtonModel* buttonModel;
    CommandView* commandView;
    CommandModel* commandModel;
    QPushButton* clearButton;
    QPushButton* printReference;
    QComboBox* devModel;
};
}  // namespace Dialog
}  // namespace Gui
