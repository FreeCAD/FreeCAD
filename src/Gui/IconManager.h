// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Kacper Donat <kacper@kadet.net>                     *
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

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QIcon>
#include <QMutex>
#include <QPixmap>
#include <QSize>
#include <QString>


class QDomElement;
namespace Gui
{

struct IconRenderCacheKey
{
    QString iconId;
    QSize size;
    qreal dpr = 1.0;
    QRgb color = 0;
    int mode = 0;
    int state = 0;

    bool operator==(const IconRenderCacheKey& other) const noexcept
    {
        return iconId == other.iconId && size == other.size && qFuzzyCompare(dpr, other.dpr)
            && color == other.color && mode == other.mode && state == other.state;
    }
};

class IconManager
{
public:
    struct IconMeta
    {
        QString iconId;
        QString svgPath;
        bool themed = true;
    };

    struct RenderRequest
    {
        QSize size;
        qreal dpr = 1.0;
        QColor color;
        QIcon::Mode mode = QIcon::Normal;
        QIcon::State state = QIcon::Off;
    };

    static IconManager& instance();

    void registerPixmap(const QPixmap& pixmap, const IconMeta& meta);
    const IconMeta* metaForPixmap(const QPixmap& pixmap) const;

    QPixmap pixmap(const QString& path);
    QIcon icon(const QString& path);

    QPixmap render(const IconMeta& meta, const RenderRequest& request) const;
    void clear();

private:
    IconManager() = default;
    IconManager(const IconManager&) = delete;
    IconManager& operator=(const IconManager&) = delete;

    struct SvgSource
    {
        QByteArray rawSvg;
    };

    QByteArray loadSvg(const QString& path) const;
    QByteArray materializeSvgDom(const QByteArray& rawSvg, const QColor& color) const;
    QPixmap renderSvg(const QByteArray& svg, const QSize& size, qreal dpr) const;

    static void recolorCurrentStrokeAttributes(QDomElement& element, const QString& colorValue);

private:
    mutable QMutex m_mutex;
    QHash<qint64, IconMeta> m_metaByPixmapKey;
    mutable QHash<QString, SvgSource> m_svgCacheByPath;
    mutable QHash<IconRenderCacheKey, QPixmap> m_renderCache;
};

}  // namespace Gui

template<>
struct std::hash<Gui::IconRenderCacheKey>
{
    std::size_t operator()(const Gui::IconRenderCacheKey& key) const noexcept
    {
        std::size_t seed = 0;

        auto combine = [&seed](std::size_t value) {
            seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        combine(qHash(key.iconId));
        combine(qHash(key.size.width()));
        combine(qHash(key.size.height()));
        combine(qHash(qRound64(key.dpr * 1000)));
        combine(qHash(static_cast<quint32>(key.color)));
        combine(qHash(key.mode));
        combine(qHash(key.state));

        return seed;
    }
};
