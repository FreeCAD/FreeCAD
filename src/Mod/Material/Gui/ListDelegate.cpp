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

#include "ListDelegate.h"
#include "ListModel.h"
#include "MaterialSave.h"


using namespace MatGui;

ListDelegate::ListDelegate(Materials::MaterialValue::ValueType type,
                           const QString& units,
                           QObject* parent)
    : BaseDelegate(type, units, parent)
{}

void ListDelegate::paint(QPainter* painter,
                         const QStyleOptionViewItem& option,
                         const QModelIndex& index) const
{

    if (_type == Materials::MaterialValue::Quantity) {
        paintQuantity(painter, option, index);
        return;
    }

    if (_type == Materials::MaterialValue::Image || _type == Materials::MaterialValue::ImageList) {
        paintImage(painter, option, index);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

// bool ListDelegate::editorEvent(QEvent* event,
//                                QAbstractItemModel* model,
//                                const QStyleOptionViewItem& option,
//                                const QModelIndex& index)
// {
//     if (event->type() == QEvent::MouseButtonDblClick) {
//         auto treeModel = index.model();

//         auto item = treeModel->data(index);

//         int row = index.row();

//         QString propertyName = group->child(row, 0)->text();
//         QString propertyType = QString::fromStdString("String");
//         if (group->child(row, 2)) {
//             propertyType = group->child(row, 2)->text();
//         }

//         std::string type = propertyType.toStdString();
//         if (_type == Materials::MaterialValue::Image || _type ==
//         Materials::MaterialValue::ImageList) {
//             showImageModal(propertyName, item);
//             // Mark as handled
//             return true;
//         }
//     }
//     return QStyledItemDelegate::editorEvent(event, model, option, index);
// }

#include "moc_ListDelegate.cpp"
