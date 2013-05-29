/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DOWNLOADITEM_H
#define GUI_DIALOG_DOWNLOADITEM_H

#include <QBasicTimer>
#include <QFile>
#include <QTime>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QLabel>
#include <QTableView>

class AutoSaver;
class QFileIconProvider;

class EditTableView : public QTableView
{
    Q_OBJECT

public:
    EditTableView(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent *event);

public Q_SLOTS:
    void removeOne();
    void removeAll();
};

class SqueezeLabel : public QLabel
{
    Q_OBJECT

public:
    SqueezeLabel(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

};

/*
    This class will call the save() slot on the parent object when the parent changes.
    It will wait several seconds after changed() to combining multiple changes and
    prevent continuous writing to disk.
  */
class AutoSaver : public QObject {

Q_OBJECT

public:
    AutoSaver(QObject *parent);
    ~AutoSaver();
    void saveIfNeccessary();

public Q_SLOTS:
    void changeOccurred();

protected:
    void timerEvent(QTimerEvent *event);

private:
    QBasicTimer m_timer;
    QTime m_firstChange;

};

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    NetworkAccessManager(QObject *parent = 0);

private Q_SLOTS:
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *auth);
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth);
};

#include "ui_DownloadItem.h"

namespace Gui {
namespace Dialog {
class DownloadModel;

class DownloadItem : public QWidget, public Ui_DownloadItem
{
    Q_OBJECT

Q_SIGNALS:
    void statusChanged();

public:
    DownloadItem(QNetworkReply *reply = 0, bool requestFileName = false, QWidget *parent = 0);
    bool downloading() const;
    bool downloadedSuccessfully() const;

    QUrl m_url;
    QString m_fileName;

    QFile m_output;
    QNetworkReply *m_reply;

private Q_SLOTS:
    void stop();
    void tryAgain();
    void open();
    void openFolder();

    void downloadReadyRead();
    void error(QNetworkReply::NetworkError code);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void metaDataChanged();
    void finished();

private:
    void contextMenuEvent(QContextMenuEvent *);
    void getFileName();
    void init();
    void updateInfoLabel();
    QString dataString(int size) const;
    QString getDownloadDirectory() const;
    QString saveFileName(const QString &directory) const;

    bool m_requestFileName;
    qint64 m_bytesReceived;
    QTime m_downloadTime;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DOWNLOADITEM_H
