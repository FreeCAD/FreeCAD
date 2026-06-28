// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <Base/Parameter.h>
#include <QCache>
#include <QEvent>
#include <QFileInfo>
#include <QImage>
#include <QPushButton>
#include <QStyledItemDelegate>

class FileCardDelegate: public QStyledItemDelegate
{

public:
    explicit FileCardDelegate(QObject* parent = nullptr);

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected:
    QPixmap generateThumbnail(const QString& path) const;

private:
    QString getCacheKey(const QString& path, int thumbnailSize) const;
    QPixmap loadAndCacheThumbnail(const QString& path, int thumbnailSize) const;

    Base::Reference<ParameterGrp> _parameterGroup;
    const int margin = 11;
    const int textspacing = 2;
    QPushButton styleButton;

    static QCache<QString, QPixmap> _thumbnailCache;  // cache key structure: "path:modtime:size"
    static constexpr const int CACHE_SIZE_MB = 50;    // 50MB cache limit
};
