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

#include <QMenu>

#include <Gui/MainWindow.h>

#include <Mod/Material/App/Exceptions.h>

#include "Array3D.h"
#include "ArrayDelegate.h"
#include "ArrayModel.h"
#include "ui_Array3D.h"


using namespace MatGui;

Array3D::Array3D(const QString& propertyName,
                 std::shared_ptr<Materials::Material> material,
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
        Base::Console().Log("Property '%s' not found\n", propertyName.toStdString().c_str());
        _property = nullptr;
    }
    if (_property) {
        _value =
            std::static_pointer_cast<Materials::Material3DArray>(_property->getMaterialValue());
    }
    else {
        Base::Console().Log("No value loaded\n");
        _value = nullptr;
    }

    setupDefault();
    setupDepthArray();
    setupArray();

    ui->table3D->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->table3D, &QWidget::customContextMenuRequested, this, &Array3D::onDepthContextMenu);

    ui->table2D->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->table2D, &QWidget::customContextMenuRequested, this, &Array3D::on2DContextMenu);

    _deleteDepthAction.setText(tr("Delete row"));
    // _deleteDepthAction.setShortcut(Qt::Key_Delete);
    connect(&_deleteDepthAction, &QAction::triggered, this, &Array3D::onDepthDelete);
    ui->table3D->addAction(&_deleteDepthAction);

    _delete2DAction.setText(tr("Delete row"));
    // _delete2DAction.setShortcut(Qt::Key_Delete);
    connect(&_delete2DAction, &QAction::triggered, this, &Array3D::on2DDelete);
    ui->table2D->addAction(&_delete2DAction);

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


    QItemSelectionModel* selectionModel = ui->table3D->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &Array3D::onSelectDepth);
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
        QString label = tr("Default ") + column1.getName();
        ui->labelDefault->setText(label);
        if (column1.getPropertyType() == QString::fromStdString("Quantity")) {
            ui->editDefault->setMinimum(std::numeric_limits<double>::min());
            ui->editDefault->setMaximum(std::numeric_limits<double>::max());
            ui->editDefault->setUnitText(_property->getColumnUnits(0));
            if (!_value->defaultSet()) {
                _value->setDefault(_property->getColumnNull(0).value<Base::Quantity>());
            }
            ui->editDefault->setValue(_value->getDefault().value<Base::Quantity>());

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
    _material->setEditStateAlter();
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
    Base::Console().Log("Array3D::onDepthContextMenu(%d,%d)\n", pos.x(), pos.y());
    QModelIndex index = ui->table3D->indexAt(pos);
    Base::Console().Log("\tindex at (%d,%d)\n", index.row(), index.column());


    QMenu contextMenu(tr("Context menu"), this);

    contextMenu.addAction(&_deleteDepthAction);

    contextMenu.exec(ui->table3D->mapToGlobal(pos));
}

bool Array3D::newDepthRow(const QModelIndex& index)
{
    Array3DDepthModel* model = static_cast<Array3DDepthModel*>(ui->table3D->model());
    return model->newRow(index);
}

void Array3D::onDepthDelete(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("Array3D::onDepthDelete()\n");
    QItemSelectionModel* selectionModel = ui->table3D->selectionModel();
    if (!selectionModel->hasSelection() || newDepthRow(selectionModel->currentIndex())) {
        Base::Console().Log("\tNothing selected\n");
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

    QString prompt = tr("Are you sure you want to delete the row?");
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
    Array3DDepthModel* model = static_cast<Array3DDepthModel*>(ui->table3D->model());
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
    Base::Console().Log("Array3D::onDepthContextMenu(%d,%d)\n", pos.x(), pos.y());
    QModelIndex index = ui->table2D->indexAt(pos);
    Base::Console().Log("\tindex at (%d,%d)\n", index.row(), index.column());


    QMenu contextMenu(tr("Context menu"), this);

    contextMenu.addAction(&_delete2DAction);

    // contextMenu.exec(mapToGlobal(pos));
    contextMenu.exec(ui->table2D->mapToGlobal(pos));
}

bool Array3D::new2DRow(const QModelIndex& index)
{
    Array3DModel* model = static_cast<Array3DModel*>(ui->table2D->model());
    return model->newRow(index);
}

void Array3D::on2DDelete(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("Array3D::on2DDelete()\n");
    QItemSelectionModel* selectionModel = ui->table2D->selectionModel();
    if (!selectionModel->hasSelection() || new2DRow(selectionModel->currentIndex())) {
        Base::Console().Log("\tNothing selected\n");
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

    QString prompt = tr("Are you sure you want to delete the row?");
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
    Array3DModel* model = static_cast<Array3DModel*>(ui->table2D->model());
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
