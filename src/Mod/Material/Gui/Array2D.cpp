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

Array2D::Array2D(const QString& propertyName,
                 const std::shared_ptr<Materials::Material>& material,
                 QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_Array2D)
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
            std::static_pointer_cast<Materials::Material2DArray>(_property->getMaterialValue());
        setWindowTitle(_property->getDisplayName());
    }
    else {
        _value = nullptr;
    }

    setupArray();

    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QWidget::customContextMenuRequested, this, &Array2D::onContextMenu);

    _deleteAction.setText(tr("Delete row"));
    _deleteAction.setShortcut(Qt::Key_Delete);
    connect(&_deleteAction, &QAction::triggered, this, &Array2D::onDelete);
    ui->tableView->addAction(&_deleteAction);

    connect(ui->standardButtons, &QDialogButtonBox::accepted, this, &Array2D::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected, this, &Array2D::reject);
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
    // table->setEditTriggers(QAbstractItemView::AllEditTriggers);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    setColumnWidths(table);
    setColumnDelegates(table);
    connect(model, &QAbstractItemModel::dataChanged, this, &Array2D::onDataChanged);
}

void Array2D::onDataChanged(const QModelIndex& topLeft,
                            const QModelIndex& bottomRight,
                            const QVector<int>& roles)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    _material->setEditStateAlter();
}

void Array2D::onContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    contextMenu.addAction(&_deleteAction);

    contextMenu.exec(ui->tableView->mapToGlobal(pos));
}

bool Array2D::newRow(const QModelIndex& index)
{
    auto model = static_cast<Array2DModel*>(ui->tableView->model());
    return model->newRow(index);
}

void Array2D::onDelete(bool checked)
{
    Q_UNUSED(checked)

    QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
    if (!selectionModel->hasSelection() || newRow(selectionModel->currentIndex())) {
        return;
    }

    int res = confirmDelete();
    if (res == QMessageBox::Cancel) {
        return;
    }
}

int Array2D::confirmDelete()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Confirm Delete"));

    QString prompt = QObject::tr("Are you sure you want to delete the row?");
    box.setText(prompt);

    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.setEscapeButton(QMessageBox::Cancel);

    int res = QMessageBox::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows
    switch (box.exec()) {
        case QMessageBox::Ok:
            deleteSelected();
            res = QMessageBox::Ok;
            break;
    }

    return res;
}

void Array2D::deleteSelected()
{
    auto model = static_cast<Array2DModel*>(ui->tableView->model());
    QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
    auto index = selectionModel->currentIndex();
    model->deleteRow(index);
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
