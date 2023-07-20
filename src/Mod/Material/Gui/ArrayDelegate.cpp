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
#include <QColorDialog>
#include <QPainter>
#endif

#include <limits>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QIODevice>
#include <QDesktopServices>
#include <QVariant>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Gui/Command.h>
#include <Gui/InputField.h>
#include <Gui/WaitCursor.h>
#include <Gui/Application.h>
#include <Gui/PrefWidgets.h>
#include <Gui/SpinBox.h>
// #include <Gui/FileDialog.h>

#include <QItemSelectionModel>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/ModelManager.h>
#include "ArrayDelegate.h"
#include "MaterialSave.h"
#include "Array2D.h"
#include "Array3D.h"


using namespace MatGui;

ArrayDelegate::ArrayDelegate(Materials::MaterialValue::ValueType type, QString units, QObject* parent) :
    QStyledItemDelegate(parent),
    _type(type),
    _units(units)
{
}

void ArrayDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, 
        const QModelIndex &index) const
{

    if (_type == Materials::MaterialValue::Quantity)
    {
        QAbstractItemModel *tableModel = const_cast<QAbstractItemModel *>(index.model());

        painter->save();

        QVariant item = tableModel->data(index);
        Base::Quantity quantity = item.value<Base::Quantity>();
        double factor;
        // QString text = quantity.getUserString(factor, _units);
        QString text = quantity.getUserString();
        painter->drawText(option.rect, 0, text);

        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

void ArrayDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    Base::Console().Log("ArrayDelegate::setEditorData()\n");

    if (_type == Materials::MaterialValue::Quantity)
    {
        QAbstractItemModel *tableModel = const_cast<QAbstractItemModel *>(index.model());
        auto item = tableModel->data(index);

        // Gui::InputField* input = static_cast<Gui::InputField*>(editor);
        // input->setText(item.toString());
        Gui::QuantitySpinBox *input = static_cast<Gui::QuantitySpinBox*>(editor);
        input->setValue(item.value<Base::Quantity>());
    } else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

QWidget* ArrayDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    Base::Console().Log("ArrayDelegate::createEditor()\n");

    const QAbstractTableModel *tableModel = static_cast<const QAbstractTableModel *>(index.model());
    auto item = tableModel->data(index);

    QWidget* editor = createWidget(parent, item);

    return editor;
}

QWidget* ArrayDelegate::createWidget(QWidget* parent, const QVariant &item) const
{
    QWidget* widget = nullptr;

    if (_type == Materials::MaterialValue::String || _type == Materials::MaterialValue::URL || _type == Materials::MaterialValue::List)
    {
        widget = new Gui::PrefLineEdit(parent);

    } else if (_type == Materials::MaterialValue::Integer)
    {
        Gui::UIntSpinBox *spinner = new Gui::UIntSpinBox(parent);
        spinner->setMinimum(0);
        spinner->setMaximum(UINT_MAX);
        spinner->setValue(item.toUInt());
        widget = spinner;
    } else if (_type == Materials::MaterialValue::Float)
    {
        Gui::DoubleSpinBox *spinner = new Gui::DoubleSpinBox(parent);
        
        // the magnetic permeability is the parameter for which many decimals matter
        // the most however, even for this, 6 digits are sufficient
        spinner->setDecimals(6);

        // for almost all Float parameters of materials a step of 1 would be too large
        spinner->setSingleStep(0.1);

        spinner->setMinimum(std::numeric_limits<double>::min());
        spinner->setMaximum(std::numeric_limits<double>::max());
        spinner->setValue(item.toDouble());
        widget = spinner;
    } else if (_type == Materials::MaterialValue::Boolean)
    {
        Gui::PrefComboBox *combo = new Gui::PrefComboBox(parent);
        combo->insertItem(0, QString::fromStdString(""));
        combo->insertItem(1, QString::fromStdString("False"));
        combo->insertItem(2, QString::fromStdString("True"));
        combo->setCurrentText(item.toString());
        widget = combo;
    } else if (_type == Materials::MaterialValue::Quantity)
    {
        // Gui::InputField *input = new Gui::InputField();
        // input->setMinimum(std::numeric_limits<double>::min());
        // input->setMaximum(std::numeric_limits<double>::max());
        // input->setUnitText(_units);
        // input->setPrecision(6);
        // input->setQuantityString(item.toString());
        Gui::QuantitySpinBox *input = new Gui::QuantitySpinBox();
        input->setMinimum(std::numeric_limits<double>::min());
        input->setMaximum(std::numeric_limits<double>::max());
        input->setUnitText(_units);
        // input->setPrecision(6);
        input->setValue(item.value<Base::Quantity>());

        widget = input;
    } else
    {
        // Default editor
        widget = new QLineEdit(parent);
    }

    widget->setParent(parent);

    return widget;
}

#include "moc_ArrayDelegate.cpp"
