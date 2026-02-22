// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
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
#include <QAbstractItemView>

#include "FileCardDelegate.h"
#include "../App/DisplayedFilesModel.h"
#include "App/Application.h"
#include <Base/Color.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>

using namespace Start;

QCache<QString, QPixmap> FileCardDelegate::_thumbnailCache;

FileCardDelegate::FileCardDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
    _parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start"
    );
    setObjectName(QStringLiteral("thumbnailWidget"));

    // Initialize cache size based on thumbnail size (only once)
    if (_thumbnailCache.maxCost() == 0) {
        int thumbnailSize = static_cast<int>(_parameterGroup->GetInt("FileThumbnailIconsSize", 128));
        int thumbnailMemory = thumbnailSize * thumbnailSize * 4;  // rgba
        int maxCacheItems = (CACHE_SIZE_MB * 1024 * 1024) / thumbnailMemory;
        _thumbnailCache.setMaxCost(maxCacheItems);
        Base::Console().log(
            "FileCardDelegate: Initialized thumbnail cache for %d items (%d MB)\n",
            maxCacheItems,
            CACHE_SIZE_MB
        );
    }
}

void FileCardDelegate::paint(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const
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
    auto thumbnailSize = static_cast<int>(_parameterGroup->GetInt("FileThumbnailIconsSize", 128));  // NOLINT
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

    QPixmap scaledPixmap = pixmap.scaled(
        QSize(thumbnailSize, thumbnailSize),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    // Step 4: Positioning
    QRect thumbnailRect(option.rect.x() + margin, option.rect.y() + margin, thumbnailSize, thumbnailSize);
    QRect textRect(
        option.rect.x() + margin,
        thumbnailRect.bottom() + margin,
        thumbnailSize,
        painter->fontMetrics().lineSpacing()
    );

    QRect sizeRect(
        option.rect.x() + margin,
        textRect.bottom() + textspacing,
        thumbnailSize,
        painter->fontMetrics().lineSpacing() + margin
    );

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

QPixmap FileCardDelegate::generateThumbnail(const QString& path) const
{
    auto thumbnailSize = static_cast<int>(_parameterGroup->GetInt("FileThumbnailIconsSize", 128));  // NOLINT

    // check if we have this thumbnail already inside cache, don't load it once again
    QString cacheKey = getCacheKey(path, thumbnailSize);
    if (!cacheKey.isEmpty()) {
        if (QPixmap* cachedThumbnail = _thumbnailCache.object(cacheKey)) {
            return *cachedThumbnail;  // cache hit - we bail out
        }
    }

    // cache miss - go and load the thumbnail as it could be changed
    return loadAndCacheThumbnail(path, thumbnailSize);
}

QString FileCardDelegate::getCacheKey(const QString& path, int thumbnailSize) const
{
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        return {};
    }

    // create cache key: path:modtime:size
    QString modTime = QString::number(fileInfo.lastModified().toSecsSinceEpoch());
    return QStringLiteral("%1:%2:%3").arg(path, modTime, QString::number(thumbnailSize));
}

QPixmap FileCardDelegate::loadAndCacheThumbnail(const QString& path, int thumbnailSize) const
{
    QPixmap thumbnail;

    if (path.endsWith(QLatin1String(".fcstd"), Qt::CaseSensitivity::CaseInsensitive)) {
        // This is a fallback, the model will have pulled the thumbnail out of the FCStd file if it
        // existed.
        QImageReader reader(QLatin1String(":/icons/freecad-doc.svg"));
        reader.setScaledSize(QSize(thumbnailSize, thumbnailSize));
        thumbnail = QPixmap::fromImage(reader.read());
    }
    else if (path.endsWith(QLatin1String(".fcmacro"), Qt::CaseSensitivity::CaseInsensitive)) {
        QImageReader reader(QLatin1String(":/icons/MacroEditor.svg"));
        reader.setScaledSize(QSize(thumbnailSize, thumbnailSize));
        thumbnail = QPixmap::fromImage(reader.read());
    }
    else if (!QImageReader::imageFormat(path).isEmpty()) {
        // It is an image: it can be its own thumbnail
        QImageReader reader(path);

        // get original size to calculate proper aspect-preserving scaled size
        QSize originalSize = reader.size();
        if (originalSize.isValid()) {
            QSize scaledSize = originalSize.scaled(thumbnailSize, thumbnailSize, Qt::KeepAspectRatio);
            reader.setScaledSize(scaledSize);
        }

        auto image = reader.read();
        if (!image.isNull()) {
            thumbnail = QPixmap::fromImage(image);
        }
        else {
            Base::Console().log(
                "FileCardDelegate: Failed to load image %s: %s\n",
                path.toStdString().c_str(),
                reader.errorString().toStdString().c_str()
            );
        }
    }

    // fallback to system icon if no thumbnail was generated
    if (thumbnail.isNull()) {
        QIcon icon = QFileIconProvider().icon(QFileInfo(path));
        if (!icon.isNull()) {
            thumbnail = icon.pixmap(thumbnailSize);
        }
        else {
            thumbnail = QPixmap(thumbnailSize, thumbnailSize);
            thumbnail.fill();
        }
    }

    // cache the thumbnail if valid
    if (!thumbnail.isNull()) {
        QString cacheKey = getCacheKey(path, thumbnailSize);
        if (!cacheKey.isEmpty()) {
            _thumbnailCache.insert(cacheKey, new QPixmap(thumbnail), 1);
        }
    }

    return thumbnail;
}
