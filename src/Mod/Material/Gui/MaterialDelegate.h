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

#ifndef MATGUI_MATERIALDELEGATE_H
#define MATGUI_MATERIALDELEGATE_H

#include <boost/filesystem.hpp>

#include <QDialog>
#include <QDir>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QSvgWidget>
#include <QTreeView>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelManager.h>

namespace fs = boost::filesystem;

namespace MatGui
{

class MaterialDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MaterialDelegate(QObject* parent = nullptr);
    ~MaterialDelegate() override = default;

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem&,
                          const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override;

protected:
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

Q_SIGNALS:
    /** Emits this signal when a property has changed */
    void propertyChange(const QString& property, const QString value);

private:
    QWidget* createWidget(QWidget* parent,
                          const QString& propertyName,
                          const QString& propertyType,
                          const QString& propertyValue,
                          const QString& propertyUnits) const;
    QRgb parseColor(const QString& color) const;
    void showColorModal(QStandardItem* item);
    void showArray2DModal(const QString& propertyName, QStandardItem* item);
    void showArray3DModal(const QString& propertyName, QStandardItem* item);
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALDELEGATE_H
