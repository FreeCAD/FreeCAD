// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once


#include "StyleParameters/ParameterManager.h"

#include <QDialog>
#include <QTreeView>
#include <optional>

QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE

namespace Gui
{

namespace StyleParameters
{
class ParameterManager;
}

QT_BEGIN_NAMESPACE
namespace Ui
{
class DlgThemeEditor;
}
QT_END_NAMESPACE

class GuiExport TokenTreeView: public QTreeView
{
    Q_OBJECT
public:
    using QTreeView::QTreeView;

protected:
    void keyPressEvent(QKeyEvent* event) override;

Q_SIGNALS:
    void requestRemove(const QModelIndex& index);
};

class GuiExport StyleParametersModel: public QAbstractItemModel,
                                      public StyleParameters::ParameterSource
{
    Q_OBJECT

    class Node;

public:
    struct Item;
    struct GroupItem;
    struct ParameterItem;

    enum Column : std::uint8_t
    {
        ParameterName,
        ParameterExpression,
        ParameterType,
        ParameterPreview,
        ColumnCount
    };

    FC_DISABLE_COPY_MOVE(StyleParametersModel);

    explicit StyleParametersModel(const std::list<ParameterSource*>& sources, QObject* parent = nullptr);

    ~StyleParametersModel() override;

    std::list<StyleParameters::Parameter> all() const override;
    std::optional<StyleParameters::Parameter> get(const std::string& name) const override;

    void reset();
    void flush() override;

    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex index(int row, int col, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    Node* node(const QModelIndex& index) const;
    Item* item(const QModelIndex& index) const;

    template<typename T>
    T* item(const QModelIndex& index) const
    {
        return dynamic_cast<T*>(item(index));
    }

    bool isAddPlaceholder(const QModelIndex& index) const;

public Q_SLOTS:
    void removeItem(const QModelIndex& index);

Q_SIGNALS:
    void newParameterAdded(const QModelIndex& index);

private:
    std::list<ParameterSource*> sources;
    std::unique_ptr<StyleParameters::ParameterManager> manager;
    std::unique_ptr<Node> root;
};

class GuiExport DlgThemeEditor: public QDialog
{
    Q_OBJECT

    class Delegate;

public:
    FC_DISABLE_COPY_MOVE(DlgThemeEditor);

    explicit DlgThemeEditor(QWidget* parent = nullptr);

    ~DlgThemeEditor() override;

public Q_SLOTS:
    void handleButtonClick(QAbstractButton* button);

private:
    std::unique_ptr<Ui::DlgThemeEditor> ui;
    std::unique_ptr<StyleParameters::ParameterManager> manager;
    std::unique_ptr<StyleParametersModel> model;
};
}  // namespace Gui
