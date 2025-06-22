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

#include "ArrayModel.h"
#include "ListDelegate.h"
#include "ListEdit.h"
#include "ListModel.h"
#include "ui_ListEdit.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ListEdit */

ListEdit::ListEdit(const QString& propertyName,
                   const std::shared_ptr<Materials::Material>& material,
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
        Base::Console().log("Property '%s' not found\n", propertyName.toStdString().c_str());
        _property = nullptr;
    }
    if (_property) {
        _value = _property->getList();
    }
    else {
        Base::Console().log("No value loaded\n");
    }

    setupListView();
    setDelegates(ui->listView);

    ui->buttonDeleteRow->setVisible(false);
    // connect(ui->buttonDeleteRow, &QPushButton::clicked, this, &ListEdit::onDelete);
    connect(ui->standardButtons, &QDialogButtonBox::accepted, this, &ListEdit::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected, this, &ListEdit::reject);

    QItemSelectionModel* selectionModel = ui->listView->selectionModel();
    connect(selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &ListEdit::onSelectionChanged);
}

void ListEdit::setDelegates(QListView* list)
{
    list->setItemDelegate(new ListDelegate(_property->getType(), _property->getUnits(), this));
}

void ListEdit::setupListView()
{
    if (_property == nullptr) {
        return;
    }

    auto list = ui->listView;
    auto model = new ListModel(_property, _value, this);
    list->setModel(model);
    list->setEditTriggers(QAbstractItemView::AllEditTriggers);
    list->setSelectionMode(QAbstractItemView::SingleSelection);

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
    update();
}

bool ListEdit::newRow(const QModelIndex& index)
{
    auto model = dynamic_cast<ListModel*>(ui->listView->model());
    return model->newRow(index);
}

void ListEdit::onDelete(bool checked)
{
    Q_UNUSED(checked)

    QItemSelectionModel* selectionModel = ui->listView->selectionModel();
    if (!selectionModel->hasSelection() || newRow(selectionModel->currentIndex())) {
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

    QString prompt = QObject::tr("Delete the row?");
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
    auto model = dynamic_cast<ListModel*>(ui->listView->model());
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

void ListEdit::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)

    ui->buttonDeleteRow->setEnabled(!selected.isEmpty());
}

#include "moc_ListEdit.cpp"
