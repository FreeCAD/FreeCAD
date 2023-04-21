/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_VECTORLISTEDITOR_H
#define GUI_VECTORLISTEDITOR_H

#include <QAbstractTableModel>
#include <QDialog>
#include <QItemDelegate>
#include <QList>

#include <memory>
#include <Base/Vector3D.h>


namespace Gui {

class VectorTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit VectorTableModel(int decimals, QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags (const QModelIndex & index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    void setValues(const QList<Base::Vector3d>& d);
    const QList<Base::Vector3d>& values() const;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    QList<Base::Vector3d> vectors;
    int decimals;
};

class VectorTableDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit VectorTableDelegate(int decimals, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    int decimals;
};

class Ui_VectorListEditor;
class VectorListEditor : public QDialog
{
    Q_OBJECT

public:
    explicit VectorListEditor(int decimals, QWidget* parent = nullptr);
    ~VectorListEditor() override;

    void setValues(const QList<Base::Vector3d>&);
    const QList<Base::Vector3d>& getValues() const;

    void accept() override;
    void reject() override;

private Q_SLOTS:
    void addRow();
    void removeRow();
    void acceptCurrent();
    void setCurrentRow(int);
    void clickedRow(const QModelIndex&);

private:
    std::unique_ptr<Ui_VectorListEditor> ui;
    VectorTableModel* model;
    QList<Base::Vector3d> data;
};

} // namespace Gui

#endif // GUI_VECTORLISTEDITOR_H
