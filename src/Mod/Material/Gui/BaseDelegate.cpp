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
// #include <Gui/FileDialog.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/ModelManager.h>

#include "BaseDelegate.h"
#include "ListModel.h"
#include "MaterialSave.h"


using namespace MatGui;

BaseDelegate::BaseDelegate(Materials::MaterialValue::ValueType type,
                           const QString& units,
                           QObject* parent)
    : QStyledItemDelegate(parent)
    , _type(type)
    , _units(units)
{}

bool BaseDelegate::newRow(const QAbstractItemModel* model, const QModelIndex& index) const
{
    // The model always includes an empty row to allow for additions
    return (index.row() == (model->rowCount() - 1));
}

QString BaseDelegate::getStringValue(const QModelIndex& index) const
{
    auto model = index.model();
    QVariant item = model->data(index);
    auto propertyValue = item.value<QString>();

    return propertyValue;
}

QRgb BaseDelegate::parseColor(const QString& color) const
{
    QString trimmed = color;
    trimmed.replace(QRegularExpression(QString::fromStdString("\\(([^<]*)\\)")),
                    QString::fromStdString("\\1"));
    QStringList parts = trimmed.split(QString::fromStdString(","));
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
        QVariant item = model->data(index);
        auto quantity = item.value<Base::Quantity>();
        QString text = quantity.getUserString();
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
        Base::Console().Log("Loading image\n");
        QByteArray by = QByteArray::fromBase64(propertyValue.toUtf8());
        img = QImage::fromData(by, "PNG").scaled(64, 64, Qt::KeepAspectRatio);
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
    painter->save();

    QImage list(QString::fromStdString(":/icons/list.svg"));
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
    painter->save();

    QImage table(QString::fromStdString(":/icons/multiline.svg"));
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
    painter->save();

    QImage table(QString::fromStdString(":/icons/table.svg"));
    QRect target(option.rect);
    if (target.width() > target.height()) {
        target.setWidth(target.height());
    }
    else {
        target.setHeight(target.width());
    }
    painter->drawImage(target, table, table.rect());

    painter->restore();
    return;
}

void BaseDelegate::paint(QPainter* painter,
                         const QStyleOptionViewItem& option,
                         const QModelIndex& index) const
{

    if (_type == Materials::MaterialValue::Quantity) {
        paintQuantity(painter, option, index);
        return;
    }
    if (_type == Materials::MaterialValue::Image) {
        paintImage(painter, option, index);
        return;
    }
    if (_type == Materials::MaterialValue::Color) {
        paintColor(painter, option, index);
        return;
    }
    if (_type == Materials::MaterialValue::List || _type == Materials::MaterialValue::FileList
        || _type == Materials::MaterialValue::ImageList) {
        paintList(painter, option, index);
        return;
    }
    if (_type == Materials::MaterialValue::MultiLineString) {
        paintMultiLineString(painter, option, index);
        return;
    }
    if (_type == Materials::MaterialValue::Array2D || _type == Materials::MaterialValue::Array3D) {
        paintArray(painter, option, index);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

QSize BaseDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    if (_type == Materials::MaterialValue::Color) {
        return {75, 23};  // Standard QPushButton size
    }
    if (_type == Materials::MaterialValue::Image || _type == Materials::MaterialValue::ImageList) {
        return {64, 64};
    }
    if (_type == Materials::MaterialValue::Array2D || _type == Materials::MaterialValue::Array3D
        || _type == Materials::MaterialValue::MultiLineString
        || _type == Materials::MaterialValue::List || _type == Materials::MaterialValue::FileList
        || _type == Materials::MaterialValue::ImageList) {
        return {23, 23};
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

void BaseDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto model = index.model();
    auto item = model->data(index);

    if (_type == Materials::MaterialValue::List) {
        auto input = dynamic_cast<Gui::PrefLineEdit*>(editor);
        item = input->text();
        return;
    }
    if (_type == Materials::MaterialValue::FileList || _type == Materials::MaterialValue::File) {
        auto chooser = dynamic_cast<Gui::FileChooser*>(editor);
        chooser->setFileName(item.toString());
        return;
    }
    if (_type == Materials::MaterialValue::Quantity) {
        auto input = dynamic_cast<Gui::InputField*>(editor);
        input->setQuantityString(item.toString());
        return;
    }

    QStyledItemDelegate::setEditorData(editor, index);
}

void BaseDelegate::setModelData(QWidget* editor,
                                QAbstractItemModel* model,
                                const QModelIndex& index) const
{
    if (_type == Materials::MaterialValue::FileList) {
        auto chooser = dynamic_cast<Gui::FileChooser*>(editor);
        model->setData(index, chooser->fileName());
    }
    else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
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
    auto item = model->data(index);

    QWidget* editor = createWidget(parent, item, index);

    return editor;
}

QWidget*
BaseDelegate::createWidget(QWidget* parent, const QVariant& item, const QModelIndex& index) const
{
    QWidget* widget = nullptr;

    if (_type == Materials::MaterialValue::String || _type == Materials::MaterialValue::URL
        || _type == Materials::MaterialValue::List) {
        auto lineEdit = new Gui::PrefLineEdit(parent);
        lineEdit->setText(item.toString());
        widget = lineEdit;
    }
    else if (_type == Materials::MaterialValue::Integer) {
        auto spinner = new Gui::UIntSpinBox(parent);
        spinner->setMinimum(0);
        spinner->setMaximum(UINT_MAX);
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
        combo->insertItem(0, QString::fromStdString(""));
        combo->insertItem(1, tr("False"));
        combo->insertItem(2, tr("True"));
        combo->setCurrentText(item.toString());
        widget = combo;
    }
    else if (_type == Materials::MaterialValue::Quantity) {
        auto input = new Gui::QuantitySpinBox(parent);
        input->setMinimum(std::numeric_limits<double>::min());
        input->setMaximum(std::numeric_limits<double>::max());
        input->setUnitText(_units);
        input->setValue(item.value<Base::Quantity>());

        widget = input;
    }
    else if (_type == Materials::MaterialValue::FileList) {
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
        widget = new QLineEdit(parent);
    }

    return widget;
}

#include "moc_BaseDelegate.cpp"
