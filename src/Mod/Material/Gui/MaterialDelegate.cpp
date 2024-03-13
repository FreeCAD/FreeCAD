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
#include <QSvgRenderer>
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
#include "ImageEdit.h"
#include "ListEdit.h"
#include "MaterialDelegate.h"
#include "MaterialSave.h"
#include "TextEdit.h"


using namespace MatGui;

MaterialDelegate::MaterialDelegate(QObject* parent)
    : BaseDelegate(parent)
{}

Materials::MaterialValue::ValueType MaterialDelegate::getType(const QModelIndex& index) const
{
    auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return {};
    }

    int row = index.row();
    QString propertyType;
    if (group->child(row, 1)) {
        propertyType = group->child(row, 2)->text();
    }

    return Materials::MaterialValue::mapType(propertyType);
}

QString MaterialDelegate::getUnits(const QModelIndex& index) const
{
    auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return {};
    }

    int row = index.row();
    QString propertyUnits;
    if (group->child(row, 1)) {
        propertyUnits = group->child(row, 3)->text();
    }
    return propertyUnits;
}

QVariant MaterialDelegate::getValue(const QModelIndex& index) const
{
    auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return {};
    }

    int row = index.row();
    QVariant propertyValue;
    if (group->child(row, 1)) {
        auto material = group->child(row, 1)->data().value<std::shared_ptr<Materials::Material>>();
        // auto propertyName = group->child(row, 0)->text();
        auto propertyName = group->child(row, 0)->data().toString();
        propertyValue = material->getProperty(propertyName)->getValue();
    }
    return propertyValue;
}

void MaterialDelegate::setValue(QAbstractItemModel* model,
                                const QModelIndex& index,
                                const QVariant& value) const
{
    auto matModel = dynamic_cast<const QStandardItemModel*>(model);
    auto item = matModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return;
    }

    int row = index.row();
    if (group->child(row, 1)) {
        auto material = group->child(row, 1)->data().value<std::shared_ptr<Materials::Material>>();
        // auto propertyName = group->child(row, 0)->text();
        auto propertyName = group->child(row, 0)->data().toString();
        material->getProperty(propertyName)->setValue(value);
        group->child(row, 1)->setText(value.toString());
    }

    notifyChanged(model, index);
}

void MaterialDelegate::notifyChanged(const QAbstractItemModel* model,
                                     const QModelIndex& index) const
{
    Base::Console().Log("MaterialDelegate::notifyChanged()\n");
    auto treeModel = dynamic_cast<const QStandardItemModel*>(model);
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return;
    }

    int row = index.row();
    if (group->child(row, 1)) {
        auto material = group->child(row, 1)->data().value<std::shared_ptr<Materials::Material>>();
        // auto propertyName = group->child(row, 0)->text();
        auto propertyName = group->child(row, 0)->data().toString();
        auto propertyValue = material->getProperty(propertyName)->getValue();
        material->setEditStateAlter();
        Base::Console().Log("MaterialDelegate::notifyChanged() - marked altered\n");

        Q_EMIT const_cast<MaterialDelegate*>(this)->propertyChange(propertyName,
                                                                   propertyValue.toString());
    }
}

bool MaterialDelegate::editorEvent(QEvent* event,
                                   QAbstractItemModel* model,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index)
{
    if (index.column() == 1) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());

            // Check we're not the material model root. This is also used to access the entry
            // columns
            auto item = treeModel->itemFromIndex(index);
            auto group = item->parent();
            if (!group) {
                return QStyledItemDelegate::editorEvent(event, model, option, index);
            }

            int row = index.row();

            // QString propertyName = group->child(row, 0)->text();
            QString propertyName = group->child(row, 0)->data().toString();

            auto type = getType(index);
            if (type == Materials::MaterialValue::Color) {
                showColorModal(propertyName, item);
                // Mark as handled
                return true;
            }
            if (type == Materials::MaterialValue::MultiLineString) {
                showMultiLineStringModal(propertyName, item);
                // Mark as handled
                return true;
            }
            if (type == Materials::MaterialValue::List || type == Materials::MaterialValue::FileList
                || type == Materials::MaterialValue::ImageList) {
                showListModal(propertyName, item);
                // Mark as handled
                return true;
            }
            if (type == Materials::MaterialValue::Array2D) {
                showArray2DModal(propertyName, item);
                // Mark as handled
                return true;
            }
            if (type == Materials::MaterialValue::Array3D) {
                showArray3DModal(propertyName, item);
                // Mark as handled
                return true;
            }
            if (type == Materials::MaterialValue::Image || type == Materials::MaterialValue::SVG) {
                showImageModal(propertyName, item);
                // Mark as handled
                return true;
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void MaterialDelegate::showColorModal(const QString& propertyName, QStandardItem* item)
{
    QColor currentColor;  // = d->col;
    currentColor.setRgba(parseColor(item->text()));
    auto dlg = new QColorDialog(currentColor);

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if (Gui::DialogOptions::dontUseNativeColorDialog()) {
        dlg->setOptions(QColorDialog::DontUseNativeDialog);
    }
    dlg->setOption(QColorDialog::ColorDialogOption::ShowAlphaChannel, false);

    dlg->setCurrentColor(currentColor);
    dlg->adjustSize();

    connect(dlg, &QColorDialog::finished, this, [&](int result) {
        if (result == QDialog::Accepted) {
            QColor color = dlg->selectedColor();
            if (color.isValid()) {
                QString colorText = QString(QString::fromStdString("(%1,%2,%3,%4)"))
                                        .arg(color.red() / 255.0)
                                        .arg(color.green() / 255.0)
                                        .arg(color.blue() / 255.0)
                                        .arg(color.alpha() / 255.0);
                item->setText(colorText);
                Q_EMIT const_cast<MaterialDelegate*>(this)->propertyChange(propertyName,
                                                                           item->text());
            }
        }
    });

    dlg->exec();
}

void MaterialDelegate::showImageModal(const QString& propertyName, QStandardItem* item)
{
    auto material = item->data().value<std::shared_ptr<Materials::Material>>();
    auto dlg = new ImageEdit(propertyName, material);

    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->adjustSize();

    //connect(dlg, &QDialog::finished, this, [&](int result) {});

    dlg->exec();
}

void MaterialDelegate::showListModal(const QString& propertyName, QStandardItem* item)
{
    auto material = item->data().value<std::shared_ptr<Materials::Material>>();
    auto dlg = new ListEdit(propertyName, material);

    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->adjustSize();

    //connect(dlg, &QDialog::finished, this, [&](int result) {});

    dlg->exec();
}

void MaterialDelegate::showMultiLineStringModal(const QString& propertyName, QStandardItem* item)
{
    auto material = item->data().value<std::shared_ptr<Materials::Material>>();
    auto dlg = new TextEdit(propertyName, material);

    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->adjustSize();

    //connect(dlg, &QDialog::finished, this, [&](int result) {});

    dlg->exec();
}


void MaterialDelegate::showArray2DModal(const QString& propertyName, QStandardItem* item)
{
    auto material = item->data().value<std::shared_ptr<Materials::Material>>();
    auto dlg = new Array2D(propertyName, material);

    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->adjustSize();

    //connect(dlg, &QDialog::finished, this, [&](int result) {});

    dlg->exec();
}

void MaterialDelegate::showArray3DModal(const QString& propertyName, QStandardItem* item)
{
    auto material = item->data().value<std::shared_ptr<Materials::Material>>();
    auto dlg = new Array3D(propertyName, material);

    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->adjustSize();

    //connect(dlg, &QDialog::finished, this, [&](int result) {});

    dlg->exec();
}

void MaterialDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    if (index.column() != 1) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());

    // Check we're not the material model root. This is also used to access the entry columns
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    auto type = getType(index);
    if (type == Materials::MaterialValue::Quantity) {
        paintQuantity(painter, option, index);
        return;
    }
    if (type == Materials::MaterialValue::Image) {
        paintImage(painter, option, index);
        return;
    }
    if (type == Materials::MaterialValue::SVG) {
        paintSVG(painter, option, index);
        return;
    }
    if (type == Materials::MaterialValue::Color) {
        paintColor(painter, option, index);
        return;
    }
    if (type == Materials::MaterialValue::List || type == Materials::MaterialValue::FileList
        || type == Materials::MaterialValue::ImageList) {
        paintList(painter, option, index);
        return;
    }
    if (type == Materials::MaterialValue::MultiLineString) {
        paintMultiLineString(painter, option, index);
        return;
    }
    if (type == Materials::MaterialValue::Array2D || type == Materials::MaterialValue::Array3D) {
        paintArray(painter, option, index);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

QWidget* MaterialDelegate::createEditor(QWidget* parent,
                                        const QStyleOptionViewItem& styleOption,
                                        const QModelIndex& index) const
{
    Q_UNUSED(styleOption)

    if (index.column() != 1) {
        return nullptr;
    }

    auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());

    // Check we're not the material model root. This is also used to access the entry columns
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return nullptr;
    }

    QVariant value = treeModel->data(index);
    QWidget* editor = createWidget(parent, value, index);

    return editor;
}

QWidget* MaterialDelegate::createWidget(QWidget* parent,
                                        const QVariant& item,
                                        const QModelIndex& index) const
{
    QWidget* widget = nullptr;

    auto type = getType(index);
    if (type == Materials::MaterialValue::Integer) {
        auto spinner = new Gui::IntSpinBox(parent);
        spinner->setMinimum(0);
        spinner->setMaximum(INT_MAX);
        spinner->setValue(item.toInt());
        widget = spinner;
    }
    else if (type == Materials::MaterialValue::Float) {
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
    else if (type == Materials::MaterialValue::Boolean) {
        auto combo = new Gui::PrefComboBox(parent);
        combo->insertItem(0, QString::fromStdString(""));
        combo->insertItem(1, tr("False"));
        combo->insertItem(2, tr("True"));
        combo->setCurrentText(item.toString());
        widget = combo;
    }
    else if (type == Materials::MaterialValue::Quantity) {
        auto input = new Gui::InputField(parent);
        input->setMinimum(std::numeric_limits<double>::min());
        input->setMaximum(std::numeric_limits<double>::max());
        input->setUnitText(getUnits(index));
        input->setPrecision(6);
        input->setValue(item.value<Base::Quantity>());

        widget = input;
    }
    else if (type == Materials::MaterialValue::File) {
        auto chooser = new Gui::FileChooser(parent);
        if (!item.toString().isEmpty()) {
            chooser->setFileName(item.toString());
        }

        widget = chooser;
    }
    else {
        // Default editor
        auto lineEdit = new Gui::PrefLineEdit(parent);
        lineEdit->setText(item.toString());
        widget = lineEdit;
    }

    return widget;
}

#include "moc_MaterialDelegate.cpp"
