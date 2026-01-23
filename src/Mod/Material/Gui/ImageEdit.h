// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <memory>

#include <QAction>
#include <QDialog>
#include <QLabel>
#include <QList>
#include <QListView>
#include <QPoint>
#include <QResizeEvent>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVariant>

#include <Mod/Material/App/Model.h>

#include "ListModel.h"

namespace MatGui
{

class Ui_ImageEdit;

class ImageLabel: public QLabel
{
    Q_OBJECT

public:
    explicit ImageLabel(QWidget* parent = 0);
    ~ImageLabel() = default;

    void setPixmap(const QPixmap& pixmap);
    void setSVG(const QString& svg);
    void renderSVG();

protected:
    void resizeEvent(QResizeEvent* event);
    void paintEvent(QPaintEvent* event);

private:
    QPixmap _pixmap;
    QString _svg;
};

class ImageEdit: public QDialog
{
    Q_OBJECT

public:
    ImageEdit(const QString& propertyName,
              const std::shared_ptr<Materials::Material>& material,
              QWidget* parent = nullptr);
    ~ImageEdit() override = default;

    void accept() override;
    void reject() override;

private Q_SLOTS:
    void onFileSelect(bool checked);

private:
    std::unique_ptr<Ui_ImageEdit> ui;
    std::shared_ptr<Materials::Material> _material;
    std::shared_ptr<Materials::MaterialProperty> _property;

    QPixmap _pixmap;
    QString _svg;

    void showPixmap();
    void showSVG();

    QString selectFile(const QString& filePatterns);
    void onFileSelectImage();
    void onFileSelectSVG();
};

}  // namespace MatGui