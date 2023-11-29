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

#include <QMenu>

#include <Gui/MainWindow.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/Materials.h>

#include "ArrayDelegate.h"
#include "ArrayModel.h"
#include "ListEdit.h"
#include "ListModel.h"
#include "ui_ListEdit.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ListEdit */

ListEdit::ListEdit(const QString& propertyName,
                   std::shared_ptr<Materials::Material> material,
                   QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_ListEdit)
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
        Base::Console().Log("Value type %d\n",
                            static_cast<int>(_property->getMaterialValue()->getType()));
        _value = _property->getList();
    }
    else {
        Base::Console().Log("No value loaded\n");
    }
    // if (_value) {
    //     Base::Console().Log("Value type %d\n", static_cast<int>(_value->getType()));
    // }

    setupListView();

    // ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);
    // connect(ui->listView, &QWidget::customContextMenuRequested, this, &ListEdit::onContextMenu);

    // _deleteAction.setText(tr("Delete row"));
    // _deleteAction.setShortcut(Qt::Key_Delete);
    // connect(&_deleteAction, &QAction::triggered, this, &ListEdit::onDelete);
    // ui->listView->addAction(&_deleteAction);

    connect(ui->standardButtons, &QDialogButtonBox::accepted, this, &ListEdit::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected, this, &ListEdit::reject);
}

void ListEdit::setColumnDelegates(QListView* list)
{
    int length = _property->columns();
    for (int i = 0; i < length; i++) {
        const Materials::MaterialProperty& column = _property->getColumn(i);
        list->setItemDelegateForColumn(
            i,
            new ArrayDelegate(column.getType(), column.getUnits(), this));
    }
}

void ListEdit::setupListView()
{
    if (_property == nullptr) {
        return;
    }

    auto list = ui->listView;
    auto model = new ListModel(_property, _value, this);
    list->setModel(model);
    // table->setEditTriggers(QAbstractItemView::AllEditTriggers);
    list->setSelectionMode(QAbstractItemView::SingleSelection);

    // setColumnWidths(list);
    // setColumnDelegates(list);
    connect(model, &QAbstractItemModel::dataChanged, this, &ListEdit::onDataChanged);
}

void ListEdit::onDataChanged(const QModelIndex& topLeft,
                             const QModelIndex& bottomRight,
                             const QVector<int>& roles)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    _material->setEditStateAlter();
}

bool ListEdit::newRow(const QModelIndex& index)
{
    ListModel* model = static_cast<ListModel*>(ui->listView->model());
    return model->newRow(index);
}

void ListEdit::onDelete(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("ListEdit::onDelete()\n");
    QItemSelectionModel* selectionModel = ui->listView->selectionModel();
    if (!selectionModel->hasSelection() || newRow(selectionModel->currentIndex())) {
        Base::Console().Log("\tNothing selected\n");
        return;
    }

    int res = confirmDelete();
    if (res == QMessageBox::Cancel) {
        return;
    }
}

int ListEdit::confirmDelete()
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

void ListEdit::deleteSelected()
{
    ListModel* model = static_cast<ListModel*>(ui->listView->model());
    QItemSelectionModel* selectionModel = ui->listView->selectionModel();
    auto index = selectionModel->currentIndex();
    model->deleteRow(index);
}

void ListEdit::accept()
{
    _property->setList(_value);
    QDialog::accept();
}

void ListEdit::reject()
{
    QDialog::reject();
}

#include "moc_ListEdit.cpp"
