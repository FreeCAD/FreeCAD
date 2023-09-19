/***************************************************************************
 *   Copyright (c) 2016 Yorik van Havre <yorik@uncreated.net>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef WEBGUI_COOKIEJAR_H
#define WEBGUI_COOKIEJAR_H

#include <Mod/Web/WebGlobal.h>
#include <QFile>
#include <QNetworkCookieJar>
#include <QTimer>

class QNetworkCookieJar;

namespace WebGui
{

class WebGuiExport FcCookieJar: public QNetworkCookieJar
{

    Q_OBJECT

public:
    explicit FcCookieJar(QObject* parent = nullptr);
    ~FcCookieJar() override;
    bool setCookiesFromUrl(const QList<QNetworkCookie>&, const QUrl&) override;

public Q_SLOTS:
    void scheduleSaveToDisk();
    void loadFromDisk();
    void reset();

private Q_SLOTS:
    void saveToDisk();

private:
    void extractRawCookies();
    QList<QByteArray> m_rawCookies;
    QFile m_file;
    QTimer m_timer;
};

}  // namespace WebGui

#endif  // WEBGUI_COOKIEJAR_H
