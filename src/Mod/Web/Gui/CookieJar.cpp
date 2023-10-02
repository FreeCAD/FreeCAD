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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QNetworkCookie>
#include <QTextStream>
#endif

#include <App/Application.h>
#include <Base/FileInfo.h>

#include "CookieJar.h"


using namespace WebGui;

/**
 *  The default cookiejar of qt does not implement saving and restoring of
 *  cookies to disk. So we extend it here, and make it save and restore cookies
 *  to a simple text file saved in the FreeCAD user folder.
 *  adapted from https://github.com/adobe/webkit/blob/master/Tools/QtTestBrowser
 */

FcCookieJar::FcCookieJar(QObject* parent)
    : QNetworkCookieJar(parent)
{
    // We use a timer for the real disk write to avoid multiple IO
    // syscalls in sequence (when loading pages which set multiple cookies).
    m_timer.setInterval(10000);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &FcCookieJar::saveToDisk);
    Base::FileInfo cookiefile(App::Application::getUserAppDataDir() + "cookies");
    m_file.setFileName(QString::fromUtf8(cookiefile.filePath().c_str()));
    if (allCookies().isEmpty()) {
        loadFromDisk();
    }
}

FcCookieJar::~FcCookieJar()
{
    extractRawCookies();
    saveToDisk();
}

bool FcCookieJar::setCookiesFromUrl(const QList<QNetworkCookie>& cookieList, const QUrl& url)
{
    bool status = QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
    if (status) {
        scheduleSaveToDisk();
    }
    return status;
}

void FcCookieJar::scheduleSaveToDisk()
{
    // We extract the raw cookies here because the user may
    // enable/disable/clear cookies while the timer is running.
    extractRawCookies();
    m_timer.start();
}

void FcCookieJar::extractRawCookies()
{
    QList<QNetworkCookie> cookies = allCookies();
    m_rawCookies.clear();

    for (const auto& it : cookies) {
        if (!it.isSessionCookie()) {
            m_rawCookies.append(it.toRawForm());
        }
    }
}

void FcCookieJar::saveToDisk()
{
    m_timer.stop();

    if (m_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&m_file);
        for (const auto& it : m_rawCookies) {
            out << it + "\n";
        }
        m_file.close();
    }
    else {
        qWarning("IO error handling cookiejar file");
    }
}

void FcCookieJar::loadFromDisk()
{
    if (!m_file.exists()) {
        return;
    }

    QList<QNetworkCookie> cookies;

    if (m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&m_file);
        while (!in.atEnd()) {
            cookies.append(QNetworkCookie::parseCookies(in.readLine().toUtf8()));
        }
        m_file.close();
    }
    else {
        qWarning("IO error handling cookiejar file");
    }

    setAllCookies(cookies);
}

void FcCookieJar::reset()
{
    setAllCookies(QList<QNetworkCookie>());
    scheduleSaveToDisk();
}

#include "moc_CookieJar.cpp"
