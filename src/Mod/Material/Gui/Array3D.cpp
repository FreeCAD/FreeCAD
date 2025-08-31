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
#include <QMenu>
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

Array3D::Array3D(const QString& propertyName,
                 const std::shared_ptr<Materials::Material>& material,
                 QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_Array3D)
    , _material(material)
{
    ui->setupUi(this);

    if (material->hasPhysicalProperty(propertyName)) {
        _property = material->getPhysicalProperty(propertyName);
    }
    else if (material->hasAppearanceProperty(propertyName)) {
        _property = material->getAppearanceProperty(propertyName);
    }
    else {
        Base::Console().log("Property '%s' not found\n", propertyName.toStdString().c_str());
        _property = nullptr;
    }
    if (_property) {
        _value =
            std::static_pointer_cast<Materials::Array3D>(_property->getMaterialValue());
    }
    else {
        _value = nullptr;
    }

    setupDepthArray();
    setupArray();

    ui->table3D->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->table3D, &QWidget::customContextMenuRequested, this, &Array3D::onDepthContextMenu);

    ui->table2D->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->table2D, &QWidget::customContextMenuRequested, this, &Array3D::on2DContextMenu);

    _deleteDepthAction.setText(tr("Delete Row"));
    connect(&_deleteDepthAction, &QAction::triggered, this, &Array3D::onDepthDelete);
    ui->table3D->addAction(&_deleteDepthAction);

    _delete2DAction.setText(tr("Delete Row"));
    connect(&_delete2DAction, &QAction::triggered, this, &Array3D::on2DDelete);
    ui->table2D->addAction(&_delete2DAction);

    connect(ui->standardButtons->button(QDialogButtonBox::Ok),
            &QPushButton::clicked,
            this,
            &Array3D::onOk);
    connect(ui->standardButtons->button(QDialogButtonBox::Cancel),
            &QPushButton::clicked,
            this,
            &Array3D::onCancel);


    QItemSelectionModel* selectionModel = ui->table3D->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &Array3D::onSelectDepth);
}

bool Array3D::onSplitter(QEvent* e)
{
    Q_UNUSED(e)

    return false;
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
    // table->setEditTriggers(QAbstractItemView::AllEditTriggers);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    setDepthColumnWidth(table);
    setDepthColumnDelegate(table);
    connect(model, &QAbstractItemModel::rowsInserted, this, &Array3D::onRowsInserted);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &Array3D::onRowsRemoved);
    connect(model, &QAbstractItemModel::dataChanged, this, &Array3D::onDataChanged);
}

void Array3D::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    update2DArray();
}

void Array3D::onRowsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    update2DArray();
}

void Array3D::onDataChanged(const QModelIndex& topLeft,
                            const QModelIndex& bottomRight,
                            const QVector<int>& roles)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    _material->setEditStateAlter();
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
    // table->setEditTriggers(QAbstractItemView::AllEditTriggers);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    setColumnWidths(table);
    setColumnDelegates(table);

    if (_value->depth() == 0) {
        table->setEnabled(false);
    }
    connect(model, &QAbstractItemModel::dataChanged, this, &Array3D::onDataChanged);
}

void Array3D::onSelectDepth(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    QModelIndexList indexes = selected.indexes();

    // This should be a list of length 1
    for (auto it = indexes.begin(); it != indexes.end(); it++) {
        _value->setCurrentDepth(it->row());
        break;
    }

    update2DArray();
}

void Array3D::update2DArray()
{
    auto table = ui->table2D;
    auto model = static_cast<Array3DModel*>(table->model());
    model->updateData();
    table->setEnabled(_value->depth() > 0);
}

void Array3D::onDepthContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Context Menu"), this);

    contextMenu.addAction(&_deleteDepthAction);

    contextMenu.exec(ui->table3D->mapToGlobal(pos));
}

bool Array3D::newDepthRow(const QModelIndex& index)
{
    auto model = static_cast<Array3DDepthModel*>(ui->table3D->model());
    return model->newRow(index);
}

void Array3D::onDepthDelete(bool checked)
{
    Q_UNUSED(checked)

    QItemSelectionModel* selectionModel = ui->table3D->selectionModel();
    if (!selectionModel->hasSelection() || newDepthRow(selectionModel->currentIndex())) {
        return;
    }

    int res = confirmDepthDelete();
    if (res == QMessageBox::Cancel) {
        return;
    }
}

int Array3D::confirmDepthDelete()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Confirm Delete"));

    QString prompt = tr("Delete the row?");
    box.setText(prompt);
    box.setInformativeText(tr("Removing this will also remove all 2D contents."));

    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.setEscapeButton(QMessageBox::Cancel);

    int res = QMessageBox::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows
    switch (box.exec()) {
        case QMessageBox::Ok:
            deleteDepthSelected();
            res = QMessageBox::Ok;
            break;
    }

    return res;
}

void Array3D::deleteDepthSelected()
{
    auto model = static_cast<Array3DDepthModel*>(ui->table3D->model());
    QItemSelectionModel* selectionModel = ui->table3D->selectionModel();
    auto index = selectionModel->currentIndex();
    model->deleteRow(index);

    auto depth = _value->currentDepth();
    if (depth >= _value->depth()) {
        depth = depth - 1;
    }
    _value->setCurrentDepth(depth);
    update2DArray();
}

void Array3D::on2DContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Context Menu"), this);

    contextMenu.addAction(&_delete2DAction);

    // contextMenu.exec(mapToGlobal(pos));
    contextMenu.exec(ui->table2D->mapToGlobal(pos));
}

bool Array3D::new2DRow(const QModelIndex& index)
{
    auto model = static_cast<Array3DModel*>(ui->table2D->model());
    return model->newRow(index);
}

void Array3D::on2DDelete(bool checked)
{
    Q_UNUSED(checked)

    QItemSelectionModel* selectionModel = ui->table2D->selectionModel();
    if (!selectionModel->hasSelection() || new2DRow(selectionModel->currentIndex())) {
        return;
    }

    int res = confirm2dDelete();
    if (res == QMessageBox::Cancel) {
        return;
    }
}

int Array3D::confirm2dDelete()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Confirm Delete"));

    QString prompt = tr("Delete the row?");
    box.setText(prompt);

    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.setEscapeButton(QMessageBox::Cancel);

    int res = QMessageBox::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows
    switch (box.exec()) {
        case QMessageBox::Ok:
            delete2DSelected();
            res = QMessageBox::Ok;
            break;
    }

    return res;
}

void Array3D::delete2DSelected()
{
    auto model = static_cast<Array3DModel*>(ui->table2D->model());
    QItemSelectionModel* selectionModel = ui->table2D->selectionModel();
    auto index = selectionModel->currentIndex();
    model->deleteRow(index);

    update2DArray();
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
