/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMessageBox>
#endif

#include <Gui/MainWindow.h>

#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/Exceptions.h>
#include "Array2D.h"
#include "ui_Array2D.h"
#include "ArrayDelegate.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

Array2DModel::Array2DModel(Materials::MaterialProperty *property, Materials::Material2DArray *value, QObject *parent) :
    QAbstractTableModel(parent),
    _property(property),
    _value(value)
{}

Array2DModel::~Array2DModel()
{}

int Array2DModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0; // No children

    return _value->rows() + 5; // Will always have 1 empty row
}

int Array2DModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return _property->columns().size();
}

QVariant Array2DModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
       return QString();

    return QVariant();
}

QVariant Array2DModel::headerData(int section, Qt::Orientation orientation,
                    int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal)
        {
            auto column = _property->getColumn(section);
            return QVariant(column.getName());
        }

        // Vertical header
        if (section == (rowCount() - 1))
            return QVariant(QString::fromStdString("*"));
        return QVariant(section + 1);
    }

    return QVariant();
}

bool Array2DModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_EMIT dataChanged(index, index);
    return true;
}

Qt::ItemFlags Array2DModel::flags(const QModelIndex& index) const
{
    return (QAbstractTableModel::flags(index) | Qt::ItemIsEditable);
}


// Resizing functions
bool Array2DModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row + count);

    endInsertRows();

    return false;
}

bool Array2DModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count);

    endRemoveRows();

    return false;
}

bool Array2DModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

bool Array2DModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_UNUSED(column);
    Q_UNUSED(count);
    Q_UNUSED(parent);

    return false;
}

//===

Array2D::Array2D(const QString &propertyName, Materials::Material *material, QWidget* parent)
  : QDialog(parent), ui(new Ui_Array2D)
{
    ui->setupUi(this);

    Base::Console().Log("Material '%s'\n", material->getName().toStdString().c_str());
    Base::Console().Log("\tproperty '%s'\n", propertyName.toStdString().c_str());

    if (material->hasPhysicalProperty(propertyName))
    {
        _property = &(material->getPhysicalProperty(propertyName));
    } else if (material->hasAppearanceProperty(propertyName)) {
        _property = &(material->getAppearanceProperty(propertyName));
    } else {
        _property = nullptr;
    }
    if (_property)
        _value = static_cast<Materials::Material2DArray *>(_property->getValue());
    else
        _value = nullptr;

    setupDefault();
    setupArray();

    connect(ui->standardButtons, &QDialogButtonBox::accepted,
            this, &Array2D::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected,
            this, &Array2D::reject);
}

Array2D::~Array2D()
{
    // no need to delete child widgets, Qt does it all for us
}

void Array2D::setupDefault()
{
    if (_property == nullptr)
        return;

    try
    {
        auto column1 = _property->getColumn(0);
        QString label =
            QString::fromStdString("Default ") + column1.getName();
        ui->labelDefault->setText(label);
        if (column1.getPropertyType() == QString::fromStdString("Quantity"))
        {
            ui->inputDefault->setUnitText(column1.getUnits());
        }
    }
    catch(const Materials::PropertyNotFound&)
    {
        return;
    }
    
    
}

void Array2D::setHeaders(QStandardItemModel *model)
{
    QStringList headers;
    for (auto column: _property->columns())
        headers.append(column.getName());
    model->setHorizontalHeaderLabels(headers);
}

void Array2D::setColumnWidths(QTableView *table)
{
    int length = _property->columns().size();
    for (int i = 0; i < length; i++)
        table->setColumnWidth(i, 100);
}

void Array2D::setColumnDelegates(QTableView *table)
{
    int length = _property->columns().size();
    for (int i = 0; i < length; i++) {
        auto column = _property->getColumn(i);
        table->setItemDelegateForColumn(i, new ArrayDelegate(column.getType(), column.getUnits(), this));
    }
}

void Array2D::setupArray()
{
    if (_property == nullptr)
        return;

    auto table = ui->tableView;
    auto model = new Array2DModel(_property, _value, this);
    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // table->horizontalHeader()->setModel(model);
    // table->horizontalHeader()->show();

    // setHeaders(model);
    setColumnWidths(table);
    setColumnDelegates(table);

    // Materials::Material2DArray *value = static_cast<Materials::Material2DArray *>(_property->getValue());
    // int length = _property->columns().size();
    // for (int i = 0; i <= value->rows(); i++)
    // {
    //     QList<QStandardItem*> items;

    //     for (int j = 0; j < length; j++)
    //     {
    //         auto item = new QStandardItem();
    //         items.append(item);
    //     }

    //     model->appendRow(items);
    // }
}

void Array2D::accept()
{
    QDialog::accept();
}

void Array2D::reject()
{
    QDialog::reject();
}

#include "moc_Array2D.cpp"
