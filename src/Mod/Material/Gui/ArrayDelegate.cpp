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
#include <QColorDialog>
#include <QDesktopServices>
#include <QIODevice>
#include <QItemSelectionModel>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>
#include <limits>
#endif

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/InputField.h>
#include <Gui/PrefWidgets.h>
#include <Gui/SpinBox.h>
#include <Gui/WaitCursor.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/ModelManager.h>

#include "Array2D.h"
#include "Array3D.h"
#include "ArrayDelegate.h"
#include "MaterialSave.h"


using namespace MatGui;

ArrayDelegate::ArrayDelegate(Materials::MaterialValue::ValueType type,
                             const QString& units,
                             QObject* parent)
    : QStyledItemDelegate(parent)
    , _type(type)
    , _units(units)
{}

void ArrayDelegate::paint(QPainter* painter,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const
{
    if (_type == Materials::MaterialValue::Quantity) {
        auto* tableModel = dynamic_cast<const AbstractArrayModel*>(index.model());
        painter->save();

        QString text;
        if (!tableModel->newRow(index)) {
            QVariant item = tableModel->data(index);
            auto quantity = item.value<Base::Quantity>();
            text = QString::fromStdString(quantity.getUserString());
        }
        painter->drawText(option.rect, 0, text);
        painter->restore();
    }
    else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

void ArrayDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (_type == Materials::MaterialValue::Quantity) {
        auto tableModel = dynamic_cast<const QAbstractItemModel*>(index.model());
        auto item = tableModel->data(index);

        auto input = static_cast<Gui::QuantitySpinBox*>(editor);
        input->setValue(item.value<Base::Quantity>());
    }
    else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

QWidget* ArrayDelegate::createEditor(QWidget* parent,
                                     const QStyleOptionViewItem& styleOption,
                                     const QModelIndex& index) const
{
    Q_UNUSED(styleOption)

    auto tableModel = dynamic_cast<const QAbstractTableModel*>(index.model());
    auto item = tableModel->data(index);

    QWidget* editor = createWidget(parent, item);

    return editor;
}

QWidget* ArrayDelegate::createWidget(QWidget* parent, const QVariant& item) const
{
    QWidget* widget = nullptr;

    if (_type == Materials::MaterialValue::String || _type == Materials::MaterialValue::URL
        || _type == Materials::MaterialValue::List) {
        widget = new Gui::PrefLineEdit(parent);
    }
    else if (_type == Materials::MaterialValue::Integer) {
        Gui::UIntSpinBox* spinner = new Gui::UIntSpinBox(parent);
        spinner->setMinimum(0);
        spinner->setMaximum(std::numeric_limits<unsigned>::max());
        spinner->setValue(item.toUInt());
        widget = spinner;
    }
    else if (_type == Materials::MaterialValue::Float) {
        auto spinner = new Gui::DoubleSpinBox(parent);

        // the magnetic permeability is the parameter for which many decimals matter
        // the most however, even for this, 6 digits are sufficient
        spinner->setDecimals(6);

        // for almost all Float parameters of materials a step of 1 would be too large
        spinner->setSingleStep(0.1);

        spinner->setMinimum(std::numeric_limits<double>::min());
        spinner->setMaximum(std::numeric_limits<double>::max());
        spinner->setValue(item.toDouble());
        widget = spinner;
    }
    else if (_type == Materials::MaterialValue::Boolean) {
        auto combo = new Gui::PrefComboBox(parent);
        combo->insertItem(0, QStringLiteral(""));
        combo->insertItem(1, tr("False"));
        combo->insertItem(2, tr("True"));
        combo->setCurrentText(item.toString());
        widget = combo;
    }
    else if (_type == Materials::MaterialValue::Quantity) {
        auto input = new Gui::QuantitySpinBox();
        input->setMinimum(std::numeric_limits<double>::min());
        input->setMaximum(std::numeric_limits<double>::max());
        input->setUnitText(_units);
        input->setValue(item.value<Base::Quantity>());

        widget = input;
    }
    else if (_type == Materials::MaterialValue::FileList) {
        auto chooser = new Gui::FileChooser();
        auto propertyValue = item.toString();
        if (!propertyValue.isEmpty()) {
            chooser->setFileName(propertyValue);
        }

        widget = chooser;
    }
    else {
        // Default editor
        widget = new QLineEdit(parent);
    }

    widget->setParent(parent);

    return widget;
}
