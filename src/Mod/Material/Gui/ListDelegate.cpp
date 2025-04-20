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

#include "ListDelegate.h"
#include "ListModel.h"
#include "MaterialSave.h"


using namespace MatGui;

ListDelegate::ListDelegate(Materials::MaterialValue::ValueType type,
                           const QString& units,
                           QObject* parent)
    : BaseDelegate(parent)
    , _type(type)
    , _units(units)
{}

QVariant ListDelegate::getValue(const QModelIndex& index) const
{
    auto model = index.model();
    auto item = model->data(index);

    return item;
}

void ListDelegate::setValue(QAbstractItemModel* model,
                            const QModelIndex& index,
                            const QVariant& value) const
{
    auto matModel = dynamic_cast<ListModel*>(model);
    if (matModel) {
        matModel->setData(index, value);

        notifyChanged(model, index);
    }
}

void ListDelegate::notifyChanged(const QAbstractItemModel* model, const QModelIndex& index) const
{
    Q_UNUSED(model)
    Q_UNUSED(index)
}

void ListDelegate::paint(QPainter* painter,
                         const QStyleOptionViewItem& option,
                         const QModelIndex& index) const
{

    auto type = getType(index);
    if (type == Materials::MaterialValue::Quantity) {
        paintQuantity(painter, option, index);
        return;
    }

    if (type == Materials::MaterialValue::Image || type == Materials::MaterialValue::ImageList) {
        paintImage(painter, option, index);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

#include "moc_ListDelegate.cpp"
