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


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

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

void Array2D::setupArray()
{
    if (_property == nullptr)
        return;

    auto table = ui->tableView;
    auto model = new QStandardItemModel();
    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    setHeaders(model);
    setColumnWidths(table);

    Materials::Material2DArray *value = static_cast<Materials::Material2DArray *>(_property->getValue());
    int length = _property->columns().size();
    for (int i = 0; i <= value->rows(); i++)
    {
        QList<QStandardItem*> items;

        for (int j = 0; j < length; j++)
        {
            auto item = new QStandardItem();
            items.append(item);
        }

        model->appendRow(items);
    }
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
