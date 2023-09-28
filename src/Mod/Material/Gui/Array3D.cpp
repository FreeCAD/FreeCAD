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
#include <QPushButton>
#endif

#include <Gui/MainWindow.h>

#include <Mod/Material/App/Exceptions.h>

#include "Array3D.h"
#include "ArrayDelegate.h"
#include "ArrayModel.h"
#include "ui_Array3D.h"


using namespace MatGui;

Array3D::Array3D(const QString& propertyName, Materials::Material* material, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_Array3D)
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
            std::static_pointer_cast<Materials::Material3DArray>(_property->getMaterialValue());
    }
    else {
        _value = nullptr;
    }

    setupDefault();
    setupDepthArray();
    setupArray();

    Base::Console().Log("Material '%s'\n", material->getName().toStdString().c_str());
    Base::Console().Log("\tproperty '%s'\n", propertyName.toStdString().c_str());

    // connect(ui->splitter, &QSplitter::event,
    //         this, &Array3D::onSplitter);

    connect(ui->standardButtons->button(QDialogButtonBox::Ok),
            &QPushButton::clicked,
            this,
            &Array3D::onOk);
    connect(ui->standardButtons->button(QDialogButtonBox::Cancel),
            &QPushButton::clicked,
            this,
            &Array3D::onCancel);
}

bool Array3D::onSplitter(QEvent* e)
{
    Q_UNUSED(e)

    return false;
}

void Array3D::setupDefault()
{
    if (_property == nullptr) {
        return;
    }

    try {
        auto& column1 = _property->getColumn(0);
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
                    &Array3D::defaultValueChanged);
        }
    }
    catch (const Materials::PropertyNotFound&) {
        return;
    }
}

void Array3D::defaultValueChanged(const Base::Quantity& value)
{
    _value->setDefault(QVariant::fromValue(value));
}

void Array3D::setDepthColumnDelegate(QTableView* table)
{
    auto& column = _property->getColumn(0);
    table->setItemDelegateForColumn(0,
                                    new ArrayDelegate(column.getType(), column.getUnits(), this));
}

void Array3D::setDepthColumnWidth(QTableView* table)
{
    table->setColumnWidth(0, 100);
}

void Array3D::setupDepthArray()
{
    if (_property == nullptr) {
        return;
    }

    auto table = ui->table3D;
    auto model = new Array3DDepthModel(_property, _value, this);
    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::AllEditTriggers);

    setDepthColumnWidth(table);
    setDepthColumnDelegate(table);
}

void Array3D::setColumnWidths(QTableView* table)
{
    int length = _property->columns();
    for (int i = 0; i < length; i++) {
        table->setColumnWidth(i, 100);
    }
}

void Array3D::setColumnDelegates(QTableView* table)
{
    int length = _property->columns();
    for (int i = 0; i < length; i++) {
        auto& column = _property->getColumn(i);
        table->setItemDelegateForColumn(
            i,
            new ArrayDelegate(column.getType(), column.getUnits(), this));
    }
}

void Array3D::setupArray()
{
    if (_property == nullptr) {
        return;
    }

    auto table = ui->table2D;
    auto model = new Array3DModel(_property, _value, this);
    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::AllEditTriggers);

    setColumnWidths(table);
    setColumnDelegates(table);
}

void Array3D::onOk(bool checked)
{
    Q_UNUSED(checked)

    QDialog::accept();
}

void Array3D::onCancel(bool checked)
{
    Q_UNUSED(checked)

    QDialog::reject();
}

#include "moc_Array3D.cpp"
