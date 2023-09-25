/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMessageBox>
#endif

#include <Gui/MainWindow.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/Materials.h>

#include "Array2D.h"
#include "ArrayDelegate.h"
#include "ArrayModel.h"
#include "ui_Array2D.h"


using namespace MatGui;

/* TRANSLATOR MatGui::Array2D */

Array2D::Array2D(const QString& propertyName, Materials::Material* material, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_Array2D)
{
    ui->setupUi(this);

    if (material->hasPhysicalProperty(propertyName)) {
        _property = &(material->getPhysicalProperty(propertyName));
    }
    else if (material->hasAppearanceProperty(propertyName)) {
        _property = &(material->getAppearanceProperty(propertyName));
    }
    else {
        _property = nullptr;
    }
    if (_property) {
        _value =
            std::static_pointer_cast<Materials::Material2DArray>(_property->getMaterialValue());
    }
    else {
        _value = nullptr;
    }

    setupDefault();
    setupArray();

    connect(ui->standardButtons, &QDialogButtonBox::accepted, this, &Array2D::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected, this, &Array2D::reject);
}

void Array2D::setupDefault()
{
    if (_property == nullptr) {
        return;
    }

    try {
        const Materials::MaterialProperty& column1 = _property->getColumn(0);
        QString label = QString::fromStdString("Default ") + column1.getName();
        ui->labelDefault->setText(label);
        if (column1.getPropertyType() == QString::fromStdString("Quantity")) {
            ui->editDefault->setMinimum(std::numeric_limits<double>::min());
            ui->editDefault->setMaximum(std::numeric_limits<double>::max());
            ui->editDefault->setUnitText(_property->getColumnUnits(0));
            ui->editDefault->setValue(_value->getDefault().getValue().value<Base::Quantity>());

            connect(ui->editDefault,
                    qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged),
                    this,
                    &Array2D::defaultValueChanged);
        }
    }
    catch (const Materials::PropertyNotFound&) {
        return;
    }
}

void Array2D::setHeaders(QStandardItemModel* model)
{
    QStringList headers;
    auto columns = _property->getColumns();
    for (auto column = columns.begin(); column != columns.end(); column++) {
        headers.append(column->getName());
    }
    model->setHorizontalHeaderLabels(headers);
}

void Array2D::setColumnWidths(QTableView* table)
{
    int length = _property->columns();
    for (int i = 0; i < length; i++) {
        table->setColumnWidth(i, 100);
    }
}

void Array2D::setColumnDelegates(QTableView* table)
{
    int length = _property->columns();
    for (int i = 0; i < length; i++) {
        const Materials::MaterialProperty& column = _property->getColumn(i);
        table->setItemDelegateForColumn(
            i,
            new ArrayDelegate(column.getType(), column.getUnits(), this));
    }
}

void Array2D::setupArray()
{
    if (_property == nullptr) {
        return;
    }

    auto table = ui->tableView;
    auto model = new Array2DModel(_property, _value, this);
    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::AllEditTriggers);

    setColumnWidths(table);
    setColumnDelegates(table);
}

void Array2D::defaultValueChanged(const Base::Quantity& value)
{
    _value->setDefault(QVariant::fromValue(value));
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
