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

#ifndef GUI_DIALOG_DLGCUSTOMIZESPACEBALL_H
#define GUI_DIALOG_DLGCUSTOMIZESPACEBALL_H

#include <QTreeView>
#include <QListView>
#include <QAbstractListModel>
#include "PropertyPage.h"

class Command;
class QPushButton;

namespace Gui
{
    namespace Dialog
    {
        class ButtonView : public QListView
        {
            Q_OBJECT
        public:
            ButtonView(QWidget *parent = 0);
            void selectButton(int number);
        Q_SIGNALS:
            void changeCommandSelection(const QString& commandName);
        private Q_SLOTS:
            void goSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

        public Q_SLOTS:
            void goChangedCommand(const QString& commandName);
        };

        class ButtonModel : public QAbstractListModel
        {
            Q_OBJECT
        public:
            ButtonModel(QObject *parent);
            virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
            virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
            void insertButtonRows(int number);
            void setCommand(int row, QString command);
            void goButtonPress(int number);
            void goMacroRemoved(const QByteArray& macroName);
            void goClear();
        private:
            ParameterGrp::handle spaceballButtonGroup() const;
            QString getLabel(const int &number) const;
        };

        class CommandView : public QTreeView
        {
            Q_OBJECT
        public:
            CommandView(QWidget *parent = 0);
        public Q_SLOTS:
            void goChangeCommandSelection(const QString& commandName);
        private Q_SLOTS:
            void goClicked(const QModelIndex &index);
        Q_SIGNALS:
            void changedCommand(const QString& commandName);
        };

        class CommandNode
        {
        public:
            enum NodeType {RootType, GroupType, CommandType};

            CommandNode(NodeType typeIn);
            ~CommandNode();

            NodeType nodeType;
            Command *aCommand;
            QString labelText;
            CommandNode *parent;
            QList<CommandNode *> children;
        };

        class CommandModel : public QAbstractItemModel
        {
            Q_OBJECT
        public:
            CommandModel(QObject *parent = 0);
            ~CommandModel();
            virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
            virtual QModelIndex parent(const QModelIndex &index) const;
            virtual int rowCount(const QModelIndex &parent) const;
            virtual int columnCount(const QModelIndex &parent) const;
            virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
            virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
            virtual Qt::ItemFlags flags (const QModelIndex &index) const;
            void goAddMacro(const QByteArray &macroName);
            void goRemoveMacro(const QByteArray &macroName);
        private:
            CommandNode *rootNode;
            CommandNode* nodeFromIndex(const QModelIndex &index) const;
            void initialize();
            void groupCommands(const QString& groupName);
            QStringList orderedGroups();
        };

        class PrintModel : public QAbstractTableModel
        {
            Q_OBJECT
        public:
            PrintModel(QObject *parent, ButtonModel *buttonModelIn, CommandModel *commandModelIn);
            virtual int rowCount(const QModelIndex &parent) const;
            virtual int columnCount(const QModelIndex &parent) const;
            virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
            virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        private:
            ButtonModel *buttonModel;
            CommandModel *commandModel;
        };

        class DlgCustomizeSpaceball : public CustomizeActionPage
        {
            Q_OBJECT
        public:
            DlgCustomizeSpaceball(QWidget *parent = 0);
            virtual ~DlgCustomizeSpaceball();
        protected:
            void changeEvent(QEvent *e);
            virtual bool event(QEvent *event);
            virtual void hideEvent(QHideEvent *event);
            virtual void showEvent (QShowEvent *event);

        protected Q_SLOTS:
            void onAddMacroAction(const QByteArray &macroName);
            void onRemoveMacroAction(const QByteArray &macroName);
            void onModifyMacroAction(const QByteArray &macroName);

        private Q_SLOTS:
            void goClear();
            void goPrint();

        private:
            void setupButtonModelView();
            void setupCommandModelView();
            void setupLayout();
            void setMessage(const QString& message);

            ButtonView *buttonView;
            ButtonModel *buttonModel;
            CommandView *commandView;
            CommandModel *commandModel;
            QPushButton *clearButton;
            QPushButton *printReference;
        };
    }
}

#endif //GUI_DIALOG_DLGCUSTOMIZESPACEBALL_H
