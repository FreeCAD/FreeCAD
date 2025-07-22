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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QFile>
#include <QFileIconProvider>
#include <QImageReader>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QLabel>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QApplication>
#include <QPushButton>
#include <QString>
#endif

#include "FileCardDelegate.h"
#include "../App/DisplayedFilesModel.h"
#include "App/Application.h"
#include <Base/Color.h>

using namespace Start;

FileCardDelegate::FileCardDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
    _parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
    setObjectName(QStringLiteral("thumbnailWidget"));
}

void FileCardDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();
    // Step 1: Styling
    QStyleOptionButton buttonOption;
    buttonOption.initFrom(option.widget);
    buttonOption.rect = option.rect;
    buttonOption.state = QStyle::State_Enabled;

    if ((option.state & QStyle::State_MouseOver) != 0) {
        buttonOption.state |= QStyle::State_MouseOver;
    }
    if ((option.state & QStyle::State_Selected) != 0) {
        buttonOption.state |= QStyle::State_On;
    }
    if ((option.state & QStyle::State_Sunken) != 0) {
        buttonOption.state |= QStyle::State_Sunken;
    }
    qApp->style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter, &styleButton);

    // Step 2: Fetch required data
    auto thumbnailSize =
        static_cast<int>(_parameterGroup->GetInt("FileThumbnailIconsSize", 128));  // NOLINT
    auto baseName = index.data(static_cast<int>(DisplayedFilesModelRoles::baseName)).toString();
    auto elidedName = painter->fontMetrics().elidedText(baseName, Qt::ElideRight, thumbnailSize);
    auto size = index.data(static_cast<int>(DisplayedFilesModelRoles::size)).toString();
    auto image = index.data(static_cast<int>(DisplayedFilesModelRoles::image)).toByteArray();
    auto path = index.data(static_cast<int>(DisplayedFilesModelRoles::path)).toString();

    QPixmap pixmap;
    if (!image.isEmpty()) {
        pixmap.loadFromData(image);
    }
    else {
        pixmap = generateThumbnail(path);
    }

    QPixmap scaledPixmap = pixmap.scaled(QSize(thumbnailSize, thumbnailSize),
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);

    // Step 4: Positioning
    QRect thumbnailRect(option.rect.x() + margin,
                        option.rect.y() + margin,
                        thumbnailSize,
                        thumbnailSize);
    QRect textRect(option.rect.x() + margin,
                   thumbnailRect.bottom() + margin,
                   thumbnailSize,
                   painter->fontMetrics().lineSpacing());

    QRect sizeRect(option.rect.x() + margin,
                   textRect.bottom() + textspacing,
                   thumbnailSize,
                   painter->fontMetrics().lineSpacing() + margin);

    // Step 5: Draw
    QRect pixmapRect(thumbnailRect.topLeft(), scaledPixmap.size());
    pixmapRect.moveCenter(thumbnailRect.center());
    painter->drawPixmap(pixmapRect.topLeft(), scaledPixmap);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);
    painter->drawText(sizeRect, Qt::AlignLeft | Qt::AlignTop, size);
    painter->restore();
}


QSize FileCardDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto thumbnailSize = _parameterGroup->GetInt("FileThumbnailIconsSize", 128);  // NOLINT

    QFontMetrics qfm(QGuiApplication::font());
    int textHeight = textspacing + qfm.lineSpacing() * 2;  // name + size
    int cardWidth = static_cast<int>(thumbnailSize) + 2 * margin;
    int cardHeight = static_cast<int>(thumbnailSize) + textHeight + 3 * margin;

    return {cardWidth, cardHeight};
}

namespace
{
QPixmap pixmapToSizedQImage(const QImage& pixmap, int size)
{
    return QPixmap::fromImage(pixmap).scaled(size,
                                             size,
                                             Qt::AspectRatioMode::KeepAspectRatio,
                                             Qt::TransformationMode::SmoothTransformation);
}
}  // namespace

QPixmap FileCardDelegate::generateThumbnail(const QString& path) const
{
    auto thumbnailSize =
        static_cast<int>(_parameterGroup->GetInt("FileThumbnailIconsSize", 128));  // NOLINT
    if (path.endsWith(QLatin1String(".fcstd"), Qt::CaseSensitivity::CaseInsensitive)) {
        // This is a fallback, the model will have pulled the thumbnail out of the FCStd file if it
        // existed.
        QImageReader reader(QLatin1String(":/icons/freecad-doc.svg"));
        reader.setScaledSize({thumbnailSize, thumbnailSize});
        return QPixmap::fromImage(reader.read());
    }
    if (path.endsWith(QLatin1String(".fcmacro"), Qt::CaseSensitivity::CaseInsensitive)) {
        QImageReader reader(QLatin1String(":/icons/MacroEditor.svg"));
        reader.setScaledSize({thumbnailSize, thumbnailSize});
        return QPixmap::fromImage(reader.read());
    }
    if (!QImageReader::imageFormat(path).isEmpty()) {
        // It is an image: it can be its own thumbnail
        QImageReader reader(path);
        auto image = reader.read();
        if (!image.isNull()) {
            return pixmapToSizedQImage(image, thumbnailSize);
        }
    }
    QIcon icon = QFileIconProvider().icon(QFileInfo(path));
    if (!icon.isNull()) {
        QPixmap pixmap = icon.pixmap(thumbnailSize);
        if (!pixmap.isNull()) {
            return pixmap;
        }
    }
    QPixmap pixmap = QPixmap(thumbnailSize, thumbnailSize);
    pixmap.fill();
    return pixmap;
}
