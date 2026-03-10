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

#include "IconManager.h"

#include <QDomDocument>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
#include <fmt/format.h>

using namespace Gui;

IconManager& IconManager::instance()
{
    static IconManager s_instance;
    return s_instance;
}

void IconManager::registerPixmap(const QPixmap& pixmap, const IconMeta& meta)
{
    if (pixmap.isNull() || meta.iconId.isEmpty() || meta.svgPath.isEmpty()) {
        return;
    }

    QMutexLocker lock(&m_mutex);
    m_metaByPixmapKey.insert(pixmap.cacheKey(), meta);
}

const IconManager::IconMeta* IconManager::metaForPixmap(const QPixmap& pixmap) const
{
    if (pixmap.isNull()) {
        return nullptr;
    }

    QMutexLocker lock(&m_mutex);
    auto it = m_metaByPixmapKey.constFind(pixmap.cacheKey());
    if (it == m_metaByPixmapKey.constEnd()) {
        return nullptr;
    }
    return &it.value();
}
QPixmap IconManager::pixmap(const QString& path)
{
    IconManager::IconMeta meta;
    meta.iconId = path;
    meta.svgPath = path;
    meta.themed = true;

    RenderRequest req;
    req.size = QSize(16, 16);
    req.dpr = 1.0;
    req.color = qApp->palette().text().color();

    QPixmap px = render(meta, req);
    registerPixmap(px, meta);
    return px;
}

QIcon IconManager::icon(const QString& path)
{
    QPixmap px = pixmap(path);
    return QIcon(px);
}

QPixmap IconManager::render(const IconMeta& meta, const RenderRequest& request) const
{
    if (meta.svgPath.isEmpty() || !request.size.isValid()) {
        return {};
    }

    const IconRenderCacheKey cacheKey {
        .iconId = meta.iconId,
        .size = request.size,
        .dpr = request.dpr,
        .color = request.color.rgba(),
        .mode = static_cast<int>(request.mode),
        .state = static_cast<int>(request.state)
    };

    {
        QMutexLocker lock(&m_mutex);
        auto it = m_renderCache.constFind(cacheKey);
        if (it != m_renderCache.constEnd()) {
            return it.value();
        }
    }

    QByteArray rawSvg;
    {
        QMutexLocker lock(&m_mutex);
        auto it = m_svgCacheByPath.constFind(meta.svgPath);
        if (it == m_svgCacheByPath.constEnd()) {
            SvgSource src;
            src.rawSvg = loadSvg(meta.svgPath);
            if (src.rawSvg.isEmpty()) {
                return {};
            }
            it = m_svgCacheByPath.insert(meta.svgPath, src);
        }
        rawSvg = it->rawSvg;
    }

    QByteArray finalSvg = rawSvg;
    if (meta.themed) {
        finalSvg = materializeSvgDom(rawSvg, request.color);
        if (finalSvg.isEmpty()) {
            return {};
        }
    }

    QPixmap pixmap = renderSvg(finalSvg, request.size, request.dpr);
    if (pixmap.isNull()) {
        return {};
    }

    {
        QMutexLocker lock(&m_mutex);
        m_renderCache.insert(cacheKey, pixmap);
    }

    return pixmap;
}

void IconManager::clear()
{
    QMutexLocker lock(&m_mutex);
    m_metaByPixmapKey.clear();
    m_svgCacheByPath.clear();
    m_renderCache.clear();
}

QByteArray IconManager::loadSvg(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

QByteArray IconManager::materializeSvgDom(const QByteArray& rawSvg, const QColor& color) const
{
    QDomDocument doc;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const auto parseResult = doc.setContent(rawSvg);
    if (!parseResult) {
        return {};
    }
#else
    QString errorMsg;
    int errorLine = 0;
    int errorColumn = 0;
    if (!doc.setContent(rawSvg, &errorMsg, &errorLine, &errorColumn)) {
        return {};
    }
#endif

    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        return {};
    }
    // root.setAttribute("shape-rendering", "geometricPrecision");
    root.setAttribute(
        QStringLiteral("stroke-width"),
        QString::fromStdString(fmt::format("{:.2f}", 1.66))
    );
    recolorCurrentStrokeAttributes(root, color.name(QColor::HexRgb));
    return doc.toByteArray(0);
}

void IconManager::recolorCurrentStrokeAttributes(QDomElement& element, const QString& colorValue)
{
    if (element.hasAttribute(QStringLiteral("stroke"))) {
        const QString stroke = element.attribute(QStringLiteral("stroke"));
        if (stroke.compare(QStringLiteral("currentColor"), Qt::CaseInsensitive) == 0) {
            element.setAttribute(QStringLiteral("stroke"), colorValue);
        }
    }

    for (QDomNode child = element.firstChild(); !child.isNull(); child = child.nextSibling()) {
        if (!child.isElement()) {
            continue;
        }

        QDomElement childElement = child.toElement();
        recolorCurrentStrokeAttributes(childElement, colorValue);
    }
}

QPixmap IconManager::renderSvg(const QByteArray& svg, const QSize& size, qreal dpr) const
{
    QSvgRenderer renderer(svg);
    if (!renderer.isValid()) {
        return {};
    }

    QPixmap pixmap(size * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    auto offset = size.width() / 24.0f;

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter, QRectF(QPointF(offset, offset), QSizeF(size)));

    return pixmap;
}
