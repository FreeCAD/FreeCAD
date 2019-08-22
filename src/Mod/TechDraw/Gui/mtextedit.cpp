/*
** Copyright (C) 2013 Jiří Procházka (Hobrasoft)
** Contact: http://www.hobrasoft.cz/
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file is under the terms of the GNU Lesser General Public License
** version 2.1 as published by the Free Software Foundation and appearing
** in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the
** GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
*/

#include "mtextedit.h"
#include <QTextDocument>
#include <QTextCursor>
#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <stdlib.h>


MTextEdit::MTextEdit(QWidget *parent) : QTextEdit(parent) {
}


bool MTextEdit::canInsertFromMimeData(const QMimeData *source) const {
    return source->hasImage() || QTextEdit::canInsertFromMimeData(source);
}


void MTextEdit::insertFromMimeData(const QMimeData *source) {
    if (source->hasImage()) {
        QStringList formats = source->formats();
        QString format;
        for (int i=0; i<formats.size(); i++) {
            if (formats[i] == "image/bmp")  { format = "BMP";  break; }
            if (formats[i] == "image/jpeg") { format = "JPG";  break; }
            if (formats[i] == "image/jpg")  { format = "JPG";  break; }
            if (formats[i] == "image/gif")  { format = "GIF";  break; }
            if (formats[i] == "image/png")  { format = "PNG";  break; }
            if (formats[i] == "image/pbm")  { format = "PBM";  break; }
            if (formats[i] == "image/pgm")  { format = "PGM";  break; }
            if (formats[i] == "image/ppm")  { format = "PPM";  break; }
            if (formats[i] == "image/tiff") { format = "TIFF"; break; }
            if (formats[i] == "image/xbm")  { format = "XBM";  break; }
            if (formats[i] == "image/xpm")  { format = "XPM";  break; }
            }
        if (!format.isEmpty()) {
//          dropImage(qvariant_cast<QImage>(source->imageData()), format);
            dropImage(qvariant_cast<QImage>(source->imageData()), "JPG"); // Sorry, ale cokoli jiného dlouho trvá
            return;
            }
        }
    QTextEdit::insertFromMimeData(source);
}


QMimeData *MTextEdit::createMimeDataFromSelection() const {
    return QTextEdit::createMimeDataFromSelection();
}


void MTextEdit::dropImage(const QImage& image, const QString& format) {
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, format.toLocal8Bit().data());
    buffer.close();
    QByteArray base64 = bytes.toBase64();
    QByteArray base64l;
    for (int i=0; i<base64.size(); i++) {
        base64l.append(base64[i]);
        if (i%80 == 0) {
            base64l.append("\n");
            }
        }

    QTextCursor cursor = textCursor();
    QTextImageFormat imageFormat;
    imageFormat.setWidth  ( image.width() );
    imageFormat.setHeight ( image.height() );
    imageFormat.setName   ( QString("data:image/%1;base64,%2")
                                .arg(QString("%1.%2").arg(rand()).arg(format))
                                .arg(base64l.data())
                                );
    cursor.insertImage    ( imageFormat );
}

#include <Mod/TechDraw/Gui/moc_mtextedit.cpp>

