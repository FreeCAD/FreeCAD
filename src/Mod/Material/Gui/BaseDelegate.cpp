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

#include "BaseDelegate.h"
#include "ListModel.h"
#include "MaterialSave.h"


using namespace MatGui;

BaseDelegate::BaseDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

bool BaseDelegate::newRow(const QAbstractItemModel* model, const QModelIndex& index) const
{
    // The model always includes an empty row to allow for additions
    return (index.row() == (model->rowCount() - 1));
}

QString BaseDelegate::getStringValue(const QModelIndex& index) const
{
    QVariant item = getValue(index);
    auto propertyValue = item.value<QString>();

    return propertyValue;
}

QRgb BaseDelegate::parseColor(const QString& color) const
{
    QString trimmed = color;
    trimmed.replace(QRegularExpression(QStringLiteral("\\(([^<]*)\\)")),
                    QStringLiteral("\\1"));
    QStringList parts = trimmed.split(QStringLiteral(","));
    if (parts.length() < 3) {
        return qRgba(0, 0, 0, 255);
    }
    int red = parts.at(0).toDouble() * 255;
    int green = parts.at(1).toDouble() * 255;
    int blue = parts.at(2).toDouble() * 255;
    int alpha = 255;
    if (parts.length() > 3) {
        alpha = parts.at(3).toDouble() * 255;
    }

    return qRgba(red, green, blue, alpha);
}

void BaseDelegate::paintQuantity(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    auto model = index.model();
    painter->save();

    if (newRow(model, index)) {
        painter->drawText(option.rect, 0, QString());
    }
    else {
        QString text;
        QVariant item = getValue(index);
        auto quantity = item.value<Base::Quantity>();
        if (quantity.isValid()) {
            text = QString::fromStdString(quantity.getUserString());
        }
        painter->drawText(option.rect, 0, text);
    }

    painter->restore();
}

void BaseDelegate::paintImage(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const
{
    auto propertyValue = getStringValue(index);

    painter->save();

    QImage img;
    if (!propertyValue.isEmpty()) {
        QByteArray by = QByteArray::fromBase64(propertyValue.toUtf8());
        img = QImage::fromData(by).scaled(64, 64, Qt::KeepAspectRatio);
    }
    QRect target(option.rect);
    if (target.width() > target.height()) {
        target.setWidth(target.height());
    }
    else {
        target.setHeight(target.width());
    }
    painter->drawImage(target, img, img.rect());

    painter->restore();
}

void BaseDelegate::paintSVG(QPainter* painter,
                            const QStyleOptionViewItem& option,
                            const QModelIndex& index) const
{
    auto propertyValue = getStringValue(index);

    painter->save();

    if (!propertyValue.isEmpty()) {
        QSvgRenderer renderer(propertyValue.toUtf8());

        renderer.render(painter, QRectF(option.rect));
    }

    painter->restore();
}

void BaseDelegate::paintColor(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const
{
    auto propertyValue = getStringValue(index);
    painter->save();

    QColor color;
    color.setRgba(qRgba(0, 0, 0, 255));  // Black border
    int left = option.rect.left() + 2;
    int width = option.rect.width() - 4;
    if (option.rect.width() > 75) {
        left += (option.rect.width() - 75) / 2;
        width = 71;
    }
    painter->fillRect(left, option.rect.top() + 2, width, option.rect.height() - 4, QBrush(color));

    color.setRgba(parseColor(propertyValue));
    left = option.rect.left() + 5;
    width = option.rect.width() - 10;
    if (option.rect.width() > 75) {
        left += (option.rect.width() - 75) / 2;
        width = 65;
    }
    painter->fillRect(left, option.rect.top() + 5, width, option.rect.height() - 10, QBrush(color));

    painter->restore();
}

void BaseDelegate::paintList(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    Q_UNUSED(index)

    painter->save();

    QImage list(QStringLiteral(":/icons/list.svg"));
    QRect target(option.rect);
    if (target.width() > target.height()) {
        target.setWidth(target.height());
    }
    else {
        target.setHeight(target.width());
    }
    painter->drawImage(target, list, list.rect());

    painter->restore();
}

void BaseDelegate::paintMultiLineString(QPainter* painter,
                                        const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
    Q_UNUSED(index)

    painter->save();

    QImage table(QStringLiteral(":/icons/multiline.svg"));
    QRect target(option.rect);
    if (target.width() > target.height()) {
        target.setWidth(target.height());
    }
    else {
        target.setHeight(target.width());
    }
    painter->drawImage(target, table, table.rect());

    painter->restore();
}

void BaseDelegate::paintArray(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const
{
    Q_UNUSED(index)

    painter->save();

    QImage table(QStringLiteral(":/icons/table.svg"));
    QRect target(option.rect);
    if (target.width() > target.height()) {
        target.setWidth(target.height());
    }
    else {
        target.setHeight(target.width());
    }
    painter->drawImage(target, table, table.rect());

    painter->restore();
}

void BaseDelegate::paint(QPainter* painter,
                         const QStyleOptionViewItem& option,
                         const QModelIndex& index) const
{
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

QSize BaseDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    auto type = getType(index);
    if (type == Materials::MaterialValue::Color) {
        return {75, 23};  // Standard QPushButton size
    }
    if (type == Materials::MaterialValue::Image || type == Materials::MaterialValue::SVG) {
        return {64, 64};
    }
    if (type == Materials::MaterialValue::Array2D || type == Materials::MaterialValue::Array3D
        || type == Materials::MaterialValue::MultiLineString
        || type == Materials::MaterialValue::List || type == Materials::MaterialValue::FileList
        || type == Materials::MaterialValue::ImageList) {
        return {23, 23};
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

void BaseDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (!editor) {
        return;
    }

    auto item = getValue(index);
    auto type = getType(index);
    if (type == Materials::MaterialValue::List) {
        auto input = dynamic_cast<Gui::PrefLineEdit*>(editor);
        item = input->text();
        return;
    }
    if (type == Materials::MaterialValue::FileList || type == Materials::MaterialValue::File) {
        auto chooser = dynamic_cast<Gui::FileChooser*>(editor);
        chooser->setFileName(item.toString());
        return;
    }
    if (type == Materials::MaterialValue::Quantity) {
        auto input = dynamic_cast<Gui::QuantitySpinBox*>(editor);
        // input->setQuantityString(item.toString());
        input->setValue(item.value<Base::Quantity>());
        return;
    }
    if (type == Materials::MaterialValue::List || type == Materials::MaterialValue::ImageList) {
        // Handled by dialogs
        return;
    }

    QStyledItemDelegate::setEditorData(editor, index);
}

void BaseDelegate::setModelData(QWidget* editor,
                                QAbstractItemModel* model,
                                const QModelIndex& index) const
{
    QVariant value;
    auto type = getType(index);
    if (type == Materials::MaterialValue::FileList || type == Materials::MaterialValue::File) {
        auto chooser = dynamic_cast<Gui::FileChooser*>(editor);
        value = chooser->fileName();
    }
    else if (type == Materials::MaterialValue::Quantity) {
        auto input = dynamic_cast<Gui::QuantitySpinBox*>(editor);
        // value = input->text();
        // return;
        // auto quantity = Base::Quantity::parse(input->text());
        auto quantity = input->value();
        value = QVariant::fromValue(quantity);
    }
    else if (type == Materials::MaterialValue::Integer) {
        auto spinner = dynamic_cast<Gui::IntSpinBox*>(editor);
        value = spinner->value();
    }
    else if (type == Materials::MaterialValue::Float) {
        auto spinner = dynamic_cast<Gui::DoubleSpinBox*>(editor);
        value = spinner->value();
    }
    else if (type == Materials::MaterialValue::Boolean) {
        auto combo = dynamic_cast<Gui::PrefComboBox*>(editor);
        value = combo->currentText();
    }
    else if (type == Materials::MaterialValue::Image || type == Materials::MaterialValue::SVG) {
        // Value was already saved to the property
        notifyChanged(index.model(), index);
        return;
    }
    else {
        auto lineEdit = dynamic_cast<Gui::PrefLineEdit*>(editor);
        value = lineEdit->text();
    }

    setValue(model, index, value);
    // Q_EMIT model->dataChanged(index, index);
}

QWidget* BaseDelegate::createEditor(QWidget* parent,
                                    const QStyleOptionViewItem& styleOption,
                                    const QModelIndex& index) const
{
    Q_UNUSED(styleOption)

    auto model = index.model();

    // If this is a new row then it has to be added before the editor is created. Otherwise
    // Adding the row while the editor is active can change where the editor is placed.
    if (newRow(model, index)) {
        const_cast<QAbstractItemModel*>(model)->insertRows(index.row(), 1);
    }
    auto item = getValue(index);

    QWidget* editor = createWidget(parent, item, index);

    return editor;
}

QWidget*
BaseDelegate::createWidget(QWidget* parent, const QVariant& item, const QModelIndex& index) const
{
    QWidget* widget = nullptr;

    auto type = getType(index);
    if (type == Materials::MaterialValue::Integer) {
        auto spinner = new Gui::UIntSpinBox(parent);
        spinner->setMinimum(0);
        spinner->setMaximum(std::numeric_limits<unsigned>::max());
        spinner->setValue(item.toUInt());
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
        combo->insertItem(0, QStringLiteral(""));
        combo->insertItem(1, tr("False"));
        combo->insertItem(2, tr("True"));
        combo->setCurrentText(item.toString());
        widget = combo;
    }
    else if (type == Materials::MaterialValue::Quantity) {
        auto input = new Gui::QuantitySpinBox(parent);
        input->setMinimum(std::numeric_limits<double>::min());
        input->setMaximum(std::numeric_limits<double>::max());
        input->setUnitText(getUnits(index));
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
    else if (type == Materials::MaterialValue::FileList) {
        auto chooser = new Gui::FileChooser(parent);
        auto propertyValue = item.toString();

        connect(chooser,
                &Gui::FileChooser::fileNameChanged,
                [this, chooser, index](const QString&) {
                    setModelData(chooser, const_cast<QAbstractItemModel*>(index.model()), index);
                });

        connect(chooser,
                &Gui::FileChooser::fileNameSelected,
                [this, chooser, index](const QString&) {
                    setModelData(chooser, const_cast<QAbstractItemModel*>(index.model()), index);
                });
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

#include "moc_BaseDelegate.cpp"
