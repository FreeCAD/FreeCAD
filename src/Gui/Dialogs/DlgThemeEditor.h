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

#ifndef DLGTHEMEEDITOR_H
#define DLGTHEMEEDITOR_H

#include <QAbstractTableModel>
#include <QDialog>

#include "StyleParameters.h"

namespace Gui {

namespace StyleParameters
{
class ParameterManager;
}

QT_BEGIN_NAMESPACE
namespace Ui { class DlgThemeEditor; }
QT_END_NAMESPACE

class GuiExport StyleParametersModel: public QAbstractItemModel
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

    enum Role
    {
        SpanRole = Qt::UserRole + 1
    };

    explicit StyleParametersModel(StyleParameters::ParameterManager* manager,
                                  QObject* parent = nullptr);
    ~StyleParametersModel() override;

    void refresh();

    int rowCount(const QModelIndex&) const override;
    int columnCount(const QModelIndex&) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex index(int row, int col, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    void walk(std::function<void(const QModelIndex&)> func, const QModelIndex& index = {}) const;

    Node* node(const QModelIndex& index) const;
    Item* item(const QModelIndex& index) const;

    template <typename T>
    T* item(const QModelIndex& index) const
    {
        return dynamic_cast<T*>(item(index));
    }

    bool isAddPlaceholder(const QModelIndex& index) const;


Q_SIGNALS:
    void newParameterAdded(const QModelIndex& index);

private:
    StyleParameters::ParameterManager* manager;
    std::unique_ptr<Node> root;
};

class GuiExport DlgThemeEditor : public QDialog {
    Q_OBJECT

    class Delegate;

public:
    explicit DlgThemeEditor(QWidget *parent = nullptr);
    ~DlgThemeEditor() override;

private Q_SLOTS:
    void loadDataFromIndex(const QModelIndex& index);

private:
    std::unique_ptr<Ui::DlgThemeEditor> ui;
    std::unique_ptr<StyleParameters::ParameterManager> manager;
    std::unique_ptr<StyleParametersModel> model;
};
} // Gui

#endif //DLGTHEMEEDITOR_H
