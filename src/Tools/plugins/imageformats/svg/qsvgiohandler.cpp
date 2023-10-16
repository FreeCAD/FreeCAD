/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Changes: Use Webkit engine instead of SVG renderer

#include "qsvgiohandler.h"

#ifndef QT_NO_SVGRENDERER

#include "qbuffer.h"
#include "qdebug.h"
#include "qimage.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qvariant.h"
#include "qwebframe.h"
#include "qwebview.h"
#include "qxmlstream.h"

QT_BEGIN_NAMESPACE

class QSvgIOHandlerPrivate
{
public:
    QSvgIOHandlerPrivate(QSvgIOHandler* qq)
        : q(qq)
        , loaded(false)
        , readDone(false)
        , backColor(Qt::transparent)
    {
        QPalette pal = webView.palette();
        pal.setColor(QPalette::Background, backColor);
        webView.setPalette(pal);
    }

    bool load(QIODevice* device);

    QSvgIOHandler* q;
    QWebView webView;
    QXmlStreamReader xmlReader;
    QSize defaultSize;
    QRect clipRect;
    QSize scaledSize;
    QRect scaledClipRect;
    bool loaded;
    bool readDone;
    QColor backColor;
};


bool QSvgIOHandlerPrivate::load(QIODevice* device)
{
    if (loaded) {
        return true;
    }
    if (q->format().isEmpty()) {
        q->canRead();
    }

    // # The SVG renderer doesn't handle trailing, unrelated data, so we must
    // assume that all available data in the device is to be read.
    bool res = false;
#if 0
    QBuffer *buf = qobject_cast<QBuffer *>(device);
    if (buf) {
        const QByteArray &ba = buf->data();
        res = r.load(QByteArray::fromRawData(ba.constData() + buf->pos(), ba.size() - buf->pos()));
        buf->seek(ba.size());
    } else if (q->format() == "svgz") {
        res = r.load(device->readAll());
    } else {
        xmlReader.setDevice(device);
        res = r.load(&xmlReader);
    }

    if (res) {
        defaultSize = QSize(r.viewBox().width(), r.viewBox().height());
        loaded = true;
    }
#else
    webView.setContent(device->readAll(), QString::fromLatin1("image/svg+xml"));
    QString node = QString::fromLatin1("document.rootElement.nodeName");
    QString root = webView.page()->mainFrame()->evaluateJavaScript(node).toString();

    if (!root.isEmpty() && root.compare(QLatin1String("svg"), Qt::CaseInsensitive) == 0) {
        QString w = QString::fromLatin1("document.rootElement.width.baseVal.value");
        QString h = QString::fromLatin1("document.rootElement.height.baseVal.value");
        double ww = webView.page()->mainFrame()->evaluateJavaScript(w).toDouble();
        double hh = webView.page()->mainFrame()->evaluateJavaScript(h).toDouble();

        defaultSize = QSize(ww, hh);
        loaded = true;
        res = true;
    }
#endif

    return loaded;
}


QSvgIOHandler::QSvgIOHandler()
    : d(new QSvgIOHandlerPrivate(this))
{}


QSvgIOHandler::~QSvgIOHandler()
{
    delete d;
}


bool QSvgIOHandler::canRead() const
{
    if (!device()) {
        return false;
    }
    if (d->loaded && !d->readDone) {
        return true;  // Will happen if we have been asked for the size
    }

    QByteArray buf = device()->peek(8);
    if (buf.startsWith("\x1f\x8b")) {
        setFormat("svgz");
        return true;
    }
    else if (buf.contains("<?xml") || buf.contains("<svg")) {
        setFormat("svg");
        return true;
    }
    return false;
}


QByteArray QSvgIOHandler::name() const
{
    return "svg";
}


bool QSvgIOHandler::read(QImage* image)
{
    if (!d->readDone && d->load(device())) {
        bool xform =
            (d->clipRect.isValid() || d->scaledSize.isValid() || d->scaledClipRect.isValid());
        QSize finalSize = d->defaultSize;
        QRectF bounds;
        if (xform && !d->defaultSize.isEmpty()) {
            bounds = QRectF(QPointF(0, 0), QSizeF(d->defaultSize));
            QPoint tr1, tr2;
            QSizeF sc(1, 1);
            if (d->clipRect.isValid()) {
                tr1 = -d->clipRect.topLeft();
                finalSize = d->clipRect.size();
            }
            if (d->scaledSize.isValid()) {
                sc = QSizeF(qreal(d->scaledSize.width()) / finalSize.width(),
                            qreal(d->scaledSize.height()) / finalSize.height());
                finalSize = d->scaledSize;
            }
            if (d->scaledClipRect.isValid()) {
                tr2 = -d->scaledClipRect.topLeft();
                finalSize = d->scaledClipRect.size();
            }
            QTransform t;
            t.translate(tr2.x(), tr2.y());
            t.scale(sc.width(), sc.height());
            t.translate(tr1.x(), tr1.y());
            bounds = t.mapRect(bounds);
        }
        *image = QImage(finalSize, QImage::Format_ARGB32_Premultiplied);
        if (!finalSize.isEmpty()) {
            image->fill(d->backColor.rgba());
            QPainter p(image);
#if 0
            d->r.render(&p, bounds);
#else
            // qreal xs = size.isValid() ? size.width() / ww : 1.0;
            // qreal ys = size.isValid() ? size.height() / hh : 1.0;
            // p.scale(xs, ys);

            // the best quality
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            p.setOpacity(0);  // important to keep transparent background
            d->webView.page()->mainFrame()->render(&p);
#endif
            p.end();
        }
        d->readDone = true;
        return true;
    }

    return false;
}


QVariant QSvgIOHandler::option(ImageOption option) const
{
    switch (option) {
        case ImageFormat:
            return QImage::Format_ARGB32_Premultiplied;
            break;
        case Size:
            d->load(device());
            return d->defaultSize;
            break;
        case ClipRect:
            return d->clipRect;
            break;
        case ScaledSize:
            return d->scaledSize;
            break;
        case ScaledClipRect:
            return d->scaledClipRect;
            break;
        case BackgroundColor:
            return d->backColor;
            break;
        default:
            break;
    }
    return QVariant();
}


void QSvgIOHandler::setOption(ImageOption option, const QVariant& value)
{
    switch (option) {
        case ClipRect:
            d->clipRect = value.toRect();
            break;
        case ScaledSize:
            d->scaledSize = value.toSize();
            break;
        case ScaledClipRect:
            d->scaledClipRect = value.toRect();
            break;
        case BackgroundColor:
            d->backColor = value.value<QColor>();
            break;
        default:
            break;
    }
}


bool QSvgIOHandler::supportsOption(ImageOption option) const
{
    switch (option) {
        case ImageFormat:
        case Size:
        case ClipRect:
        case ScaledSize:
        case ScaledClipRect:
        case BackgroundColor:
            return true;
        default:
            break;
    }
    return false;
}


bool QSvgIOHandler::canRead(QIODevice* device)
{
    QByteArray buf = device->peek(8);
    return buf.startsWith("\x1f\x8b") || buf.contains("<?xml") || buf.contains("<svg");
}

QT_END_NAMESPACE

#endif  // QT_NO_SVGRENDERER
