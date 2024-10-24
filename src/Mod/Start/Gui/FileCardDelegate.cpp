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
#endif

#include "FileCardDelegate.h"
#include "../App/DisplayedFilesModel.h"
#include "App/Application.h"
#include <App/Color.h>
#include <gsl/pointers>

using namespace Start;

FileCardDelegate::FileCardDelegate(QObject* parent)
    : QAbstractItemDelegate(parent)
{
    _parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
    _widget = std::make_unique<QWidget>();
    _widget->setObjectName(QLatin1String("thumbnailWidget"));
    auto layout = gsl::owner<QVBoxLayout*>(new QVBoxLayout());
    layout->setSpacing(0);
    _widget->setLayout(layout);
}

QColor FileCardDelegate::getBorderColor() const
{
    QColor color(98, 160, 234);  // NOLINT
    uint32_t packed = App::Color::asPackedRGB<QColor>(color);
    packed = _parameterGroup->GetUnsigned("FileThumbnailBorderColor", packed);
    color = App::Color::fromPackedRGB<QColor>(packed);
    return color;
}

QColor FileCardDelegate::getBackgroundColor() const
{
    QColor color(221, 221, 221);  // NOLINT
    uint32_t packed = App::Color::asPackedRGB<QColor>(color);
    packed = _parameterGroup->GetUnsigned("FileThumbnailBackgroundColor", packed);
    color = App::Color::fromPackedRGB<QColor>(packed);
    return color;
}

QColor FileCardDelegate::getSelectionColor() const
{
    QColor color(38, 162, 105);  // NOLINT
    uint32_t packed = App::Color::asPackedRGB<QColor>(color);
    packed = _parameterGroup->GetUnsigned("FileThumbnailSelectionColor", packed);
    color = App::Color::fromPackedRGB<QColor>(packed);
    return color;
}

void FileCardDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    auto thumbnailSize =
        static_cast<int>(_parameterGroup->GetInt("FileThumbnailIconsSize", 128));  // NOLINT
    auto cardWidth = thumbnailSize;
    auto baseName = index.data(static_cast<int>(DisplayedFilesModelRoles::baseName)).toString();
    auto size = index.data(static_cast<int>(DisplayedFilesModelRoles::size)).toString();
    auto image = index.data(static_cast<int>(DisplayedFilesModelRoles::image)).toByteArray();
    auto path = index.data(static_cast<int>(DisplayedFilesModelRoles::path)).toString();
    painter->save();
    auto thumbnail = std::make_unique<QLabel>();
    auto pixmap = std::make_unique<QPixmap>();
    auto layout = qobject_cast<QVBoxLayout*>(_widget->layout());
    if (!image.isEmpty()) {
        pixmap->loadFromData(image);
        if (!pixmap->isNull()) {
            auto scaled = pixmap->scaled(QSize(thumbnailSize, thumbnailSize),
                                         Qt::AspectRatioMode::KeepAspectRatio,
                                         Qt::TransformationMode::SmoothTransformation);
            thumbnail->setPixmap(scaled);
        }
    }
    else {
        thumbnail->setPixmap(generateThumbnail(path));
    }
    thumbnail->setFixedSize(thumbnailSize, thumbnailSize);
    thumbnail->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);

    QString style = QStringLiteral("");

    _widget->setProperty("state", QStringLiteral(""));
    if (option.state & QStyle::State_Selected) {
        _widget->setProperty("state", QStringLiteral("pressed"));
        if (qApp->styleSheet().isEmpty()) {
            QColor color = getSelectionColor();
            style = QString::fromLatin1("QWidget#thumbnailWidget {"
                                        " border: 2px solid rgb(%1, %2, %3);"
                                        " border-radius: 4px;"
                                        " padding: 2px;"
                                        "}")
                        .arg(color.red())
                        .arg(color.green())
                        .arg(color.blue());
        }
    }
    else if (option.state & QStyle::State_MouseOver) {
        _widget->setProperty("state", QStringLiteral("hovered"));
        if (qApp->styleSheet().isEmpty()) {
            QColor color = getBorderColor();
            style = QString::fromLatin1("QWidget#thumbnailWidget {"
                                        " border: 2px solid rgb(%1, %2, %3);"
                                        " border-radius: 4px;"
                                        " padding: 2px;"
                                        "}")
                        .arg(color.red())
                        .arg(color.green())
                        .arg(color.blue());
        }
    }
    else if (qApp->styleSheet().isEmpty()) {
        QColor color = getBackgroundColor();
        style = QString::fromLatin1("QWidget#thumbnailWidget {"
                                    " background-color: rgb(%1, %2, %3);"
                                    " border-radius: 8px;"
                                    "}")
                    .arg(color.red())
                    .arg(color.green())
                    .arg(color.blue());
    }

    _widget->setStyleSheet(style);

    auto elided =
        painter->fontMetrics().elidedText(baseName, Qt::TextElideMode::ElideRight, cardWidth);
    auto name = std::make_unique<QLabel>(elided);
    layout->addWidget(thumbnail.get());  // Temp. ownership transfer
    layout->addWidget(name.get());       // Temp. ownership transfer
    auto sizeLabel = std::make_unique<QLabel>(size);
    layout->addWidget(sizeLabel.get());  // Temp. ownership transfer
    layout->addStretch();
    _widget->resize(option.rect.size());
    painter->translate(option.rect.topLeft());
    _widget->render(painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter->restore();
    layout->removeWidget(sizeLabel.get());
    layout->removeWidget(thumbnail.get());
    layout->removeWidget(name.get());
}


QSize FileCardDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    auto thumbnailSize = _parameterGroup->GetInt("FileThumbnailIconsSize", 128);  // NOLINT
    auto cardMargin = _widget->layout()->contentsMargins();
    auto cardWidth = thumbnailSize + cardMargin.left() + cardMargin.right();
    auto spacing = _widget->layout()->spacing();

    auto font = QGuiApplication::font();
    auto qfm = QFontMetrics(font);
    auto textHeight = 2 * qfm.lineSpacing();
    auto cardHeight =
        thumbnailSize + textHeight + 2 * spacing + cardMargin.top() + cardMargin.bottom();

    return {static_cast<int>(cardWidth), static_cast<int>(cardHeight)};
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
