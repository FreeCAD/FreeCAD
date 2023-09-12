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

#include <qimageiohandler.h>
#include <qstringlist.h>

#if !defined(QT_NO_IMAGEFORMATPLUGIN) && !defined(QT_NO_SVGRENDERER)

#include "qsvgiohandler.h"

#include <qbytearray.h>
#include <qdebug.h>
#include <qiodevice.h>

QT_BEGIN_NAMESPACE

class QSvgPlugin: public QImageIOPlugin
{
public:
    QStringList keys() const;
    Capabilities capabilities(QIODevice* device, const QByteArray& format) const;
    QImageIOHandler* create(QIODevice* device, const QByteArray& format = QByteArray()) const;
};

QStringList QSvgPlugin::keys() const
{
    return QStringList() << QLatin1String("svg") << QLatin1String("svgz");
}

QImageIOPlugin::Capabilities QSvgPlugin::capabilities(QIODevice* device,
                                                      const QByteArray& format) const
{
    if (format == "svg" || format == "svgz") {
        return Capabilities(CanRead);
    }
    if (!format.isEmpty()) {
        return 0;
    }

    Capabilities cap;
    if (device->isReadable() && QSvgIOHandler::canRead(device)) {
        cap |= CanRead;
    }
    return cap;
}

QImageIOHandler* QSvgPlugin::create(QIODevice* device, const QByteArray& format) const
{
    QSvgIOHandler* hand = new QSvgIOHandler();
    hand->setDevice(device);
    hand->setFormat(format);
    return hand;
}

Q_EXPORT_STATIC_PLUGIN(QSvgPlugin)
Q_EXPORT_PLUGIN2(qsvg, QSvgPlugin)

QT_END_NAMESPACE

#endif  // !QT_NO_IMAGEFORMATPLUGIN
