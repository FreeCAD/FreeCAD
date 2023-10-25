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
#endif

#include <limits>

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
#include "MaterialDelegate.h"
#include "MaterialSave.h"


using namespace MatGui;

MaterialDelegate::MaterialDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

bool MaterialDelegate::editorEvent(QEvent* event,
                                   QAbstractItemModel* model,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index)
{
    if (index.column() == 1) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            const QStandardItemModel* treeModel =
                static_cast<const QStandardItemModel*>(index.model());

            // Check we're not the material model root. This is also used to access the entry
            // columns
            auto item = treeModel->itemFromIndex(index);
            auto group = item->parent();
            if (!group) {
                return QStyledItemDelegate::editorEvent(event, model, option, index);
            }

            int row = index.row();

            QString propertyName = group->child(row, 0)->text();
            QString propertyType = QString::fromStdString("String");
            if (group->child(row, 2)) {
                propertyType = group->child(row, 2)->text();
            }

            std::string type = propertyType.toStdString();
            if (type == "Color") {
                Base::Console().Log("Edit color\n");
                showColorModal(item);
                // Mark as handled
                return true;
            }
            else if (type == "2DArray") {
                Base::Console().Log("Edit 2DArray\n");
                showArray2DModal(propertyName, item);
                // Mark as handled
                return true;
            }
            else if (type == "3DArray") {
                Base::Console().Log("Edit 3DArray\n");
                showArray3DModal(propertyName, item);
                // Mark as handled
                return true;
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void MaterialDelegate::showColorModal(QStandardItem* item)
{
    QColor currentColor;  // = d->col;
    currentColor.setRgba(parseColor(item->text()));
    QColorDialog* dlg = new QColorDialog(currentColor);

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
            }
        }
    });

    dlg->exec();
}

void MaterialDelegate::showArray2DModal(const QString& propertyName, QStandardItem* item)
{
    Materials::Material* material = item->data().value<Materials::Material*>();
    Array2D* dlg = new Array2D(propertyName, material);

    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->adjustSize();

    connect(dlg, &QDialog::finished, this, [&](int result) {
        if (result == QDialog::Accepted) {
            Base::Console().Log("Accepted\n");
        }
    });

    dlg->exec();
}

void MaterialDelegate::showArray3DModal(const QString& propertyName, QStandardItem* item)
{
    Materials::Material* material = item->data().value<Materials::Material*>();
    Array3D* dlg = new Array3D(propertyName, material);

    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->adjustSize();

    connect(dlg, &QDialog::finished, this, [&](int result) {
        if (result == QDialog::Accepted) {
            Base::Console().Log("Accepted\n");
        }
    });

    dlg->exec();
}

void MaterialDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    // Base::Console().Log("MaterialsEditor::paint()\n");
    if (index.column() != 1) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const QStandardItemModel* treeModel = static_cast<const QStandardItemModel*>(index.model());

    // Check we're not the material model root. This is also used to access the entry columns
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    int row = index.row();

    QString propertyName = group->child(row, 0)->text();
    QString propertyType = QString::fromStdString("String");
    if (group->child(row, 2)) {
        propertyType = group->child(row, 2)->text();
    }
    QString propertyValue = QString::fromStdString("");
    if (group->child(row, 1)) {
        propertyValue = group->child(row, 1)->text();
    }

    std::string type = propertyType.toStdString();
    if (type == "Color") {
        painter->save();

        QColor color;
        color.setRgba(parseColor(propertyValue));
        int left = option.rect.left() + 5;
        int width = option.rect.width() - 10;
        if (option.rect.width() > 75) {
            left += (option.rect.width() - 75) / 2;
            width = 65;
        }
        painter->fillRect(left,
                          option.rect.top() + 5,
                          width,
                          option.rect.height() - 10,
                          QBrush(color));

        painter->restore();
        return;
    }
    else if (type == "2DArray" || type == "3DArray") {
        // painter->save();

        QImage table(QString::fromStdString(":/icons/table.svg"));
        QRect target(option.rect);
        if (target.width() > target.height()) {
            target.setWidth(target.height());
        }
        else {
            target.setHeight(target.width());
        }
        painter->drawImage(target, table, table.rect());

        // painter->restore();
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

QSize MaterialDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.column() != 1) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const QStandardItemModel* treeModel = static_cast<const QStandardItemModel*>(index.model());

    // Check we're not the material model root. This is also used to access the entry columns
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    int row = index.row();

    QString propertyType = QString::fromStdString("String");
    if (group->child(row, 2)) {
        propertyType = group->child(row, 2)->text();
    }

    std::string type = propertyType.toStdString();
    if (type == "Color") {
        return QSize(75, 23);  // Standard QPushButton size
    }
    else if (type == "2DArray" || type == "3DArray") {
        return QSize(23, 23);
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

void MaterialDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    Base::Console().Log("MaterialsEditor::setEditorData()\n");
    QVariant propertyType = editor->property("Type");
    const QStandardItemModel* model = static_cast<const QStandardItemModel*>(index.model());
    QStandardItem* item = model->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return;
    }

    int row = index.row();
    QString propertyName = group->child(row, 0)->text();

    std::string type = propertyType.toString().toStdString();
    if (type == "File") {
        Gui::FileChooser* chooser = static_cast<Gui::FileChooser*>(editor);
        item->setText(chooser->fileName());
    }
    else if (type == "Quantity") {
        Gui::InputField* input = static_cast<Gui::InputField*>(editor);
        item->setText(input->getQuantityString());
    }
    else {
        QStyledItemDelegate::setEditorData(editor, index);
    }

    // Q_EMIT const_cast<MaterialDelegate *>(this)->propertyChange(propertyName, item->text());
}

void MaterialDelegate::setModelData(QWidget* editor,
                                    QAbstractItemModel* model,
                                    const QModelIndex& index) const
{
    QStyledItemDelegate::setModelData(editor, model, index);

    QStandardItem* item = static_cast<const QStandardItemModel*>(model)->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return;
    }

    int row = index.row();
    QString propertyName = group->child(row, 0)->text();
    Q_EMIT const_cast<MaterialDelegate*>(this)->propertyChange(propertyName, item->text());
}

QWidget* MaterialDelegate::createEditor(QWidget* parent,
                                        const QStyleOptionViewItem&,
                                        const QModelIndex& index) const
{
    Base::Console().Log("MaterialsEditor::createEditor()\n");
    if (index.column() != 1) {
        return nullptr;
    }

    const QStandardItemModel* treeModel = static_cast<const QStandardItemModel*>(index.model());

    // Check we're not the material model root. This is also used to access the entry columns
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group) {
        return nullptr;
    }

    int row = index.row();

    QString propertyName = group->child(row, 0)->text();
    QString propertyType = QString::fromStdString("String");
    if (group->child(row, 2)) {
        propertyType = group->child(row, 2)->text();
    }

    QString propertyValue = QString::fromStdString("");
    if (group->child(row, 1)) {
        propertyValue = group->child(row, 1)->text();
    }

    QString propertyUnits = QString::fromStdString("");
    if (group->child(row, 1)) {
        propertyUnits = group->child(row, 3)->text();
    }

    QWidget* editor =
        createWidget(parent, propertyName, propertyType, propertyValue, propertyUnits);

    return editor;
}

QWidget* MaterialDelegate::createWidget(QWidget* parent,
                                        const QString& propertyName,
                                        const QString& propertyType,
                                        const QString& propertyValue,
                                        const QString& propertyUnits) const
{
    Q_UNUSED(propertyName);

    QWidget* widget = nullptr;

    std::string type = propertyType.toStdString();
    if (type == "String" || type == "URL" || type == "Vector") {
        widget = new Gui::PrefLineEdit(parent);
    }
    else if ((type == "Integer") || (type == "Int")) {
        Gui::UIntSpinBox* spinner = new Gui::UIntSpinBox(parent);
        spinner->setMinimum(0);
        spinner->setMaximum(UINT_MAX);
        spinner->setValue(propertyValue.toUInt());
        widget = spinner;
    }
    else if (type == "Float") {
        Gui::DoubleSpinBox* spinner = new Gui::DoubleSpinBox(parent);

        // the magnetic permeability is the parameter for which many decimals matter
        // the most however, even for this, 6 digits are sufficient
        spinner->setDecimals(6);

        // for almost all Float parameters of materials a step of 1 would be too large
        spinner->setSingleStep(0.1);

        spinner->setMinimum(std::numeric_limits<double>::min());
        spinner->setMaximum(std::numeric_limits<double>::max());
        spinner->setValue(propertyValue.toDouble());
        widget = spinner;
    }
    else if (type == "Boolean") {
        Gui::PrefComboBox* combo = new Gui::PrefComboBox(parent);
        combo->insertItem(0, QString::fromStdString(""));
        combo->insertItem(1, QString::fromStdString("False"));
        combo->insertItem(2, QString::fromStdString("True"));
        combo->setCurrentText(propertyValue);
        widget = combo;
    }
    else if (type == "Quantity") {
        Gui::InputField* input = new Gui::InputField();
        input->setMinimum(std::numeric_limits<double>::min());
        input->setMaximum(std::numeric_limits<double>::max());
        input->setUnitText(propertyUnits);  // TODO: Ensure this exists
        input->setPrecision(6);
        input->setQuantityString(propertyValue);

        widget = input;
    }
    else if (type == "File") {
        Gui::FileChooser* chooser = new Gui::FileChooser();
        if (propertyValue.length() > 0) {
            chooser->setFileName(propertyValue);
        }

        widget = chooser;
    }
    else {
        // Default editor
        widget = new QLineEdit(parent);
    }

    widget->setProperty("Type", propertyType);
    widget->setParent(parent);

    return widget;
}

QRgb MaterialDelegate::parseColor(const QString& color) const
{
    QString trimmed = color;
    trimmed.replace(QRegularExpression(QString::fromStdString("\\(([^<]*)\\)")),
                    QString::fromStdString("\\1"));
    QStringList parts = trimmed.split(QString::fromStdString(","));
    if (parts.length() < 4) {
        return qRgba(0, 0, 0, 1);
    }
    int red = parts.at(0).toDouble() * 255;
    int green = parts.at(1).toDouble() * 255;
    int blue = parts.at(2).toDouble() * 255;
    int alpha = parts.at(3).toDouble() * 255;

    return qRgba(red, green, blue, alpha);
}

#include "moc_MaterialDelegate.cpp"
