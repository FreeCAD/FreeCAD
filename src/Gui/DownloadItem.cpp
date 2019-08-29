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


#include "PreCompiled.h"
#include <stdio.h>
#include <cmath>

#include <QAuthenticator>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QMenu>
#include <QNetworkDiskCache>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QSettings>
#include <QMetaObject>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHeaderView>
#include <QDebug>
#include <QKeyEvent>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif
#include <QTextDocument>

#include <App/Document.h>

#include "DownloadItem.h"
#include "DownloadManager.h"
#include "Application.h"
#include "Document.h"
#include "MainWindow.h"
#include "FileDialog.h"
#include "ui_DlgAuthorization.h"

using namespace Gui::Dialog;


EditTableView::EditTableView(QWidget *parent)
    : QTableView(parent)
{
}

void EditTableView::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Delete
        || event->key() == Qt::Key_Backspace)
        && model()) {
        removeOne();
    } else {
        QAbstractItemView::keyPressEvent(event);
    }
}

void EditTableView::removeOne()
{
    if (!model() || !selectionModel())
        return;
    int row = currentIndex().row();
    model()->removeRow(row, rootIndex());
    QModelIndex idx = model()->index(row, 0, rootIndex());
    if (!idx.isValid())
        idx = model()->index(row - 1, 0, rootIndex());
    selectionModel()->select(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

void EditTableView::removeAll()
{
    if (model())
        model()->removeRows(0, model()->rowCount(rootIndex()), rootIndex());
}

// ----------------------------------------------------------------------------

SqueezeLabel::SqueezeLabel(QWidget *parent) : QLabel(parent)
{
}

void SqueezeLabel::paintEvent(QPaintEvent *event)
{
    QFontMetrics fm = fontMetrics();
    if (fm.width(text()) > contentsRect().width()) {
        QString elided = fm.elidedText(text(), Qt::ElideMiddle, width());
        QString oldText = text();
        setText(elided);
        QLabel::paintEvent(event);
        setText(oldText);
    } else {
        QLabel::paintEvent(event);
    }
}

// ----------------------------------------------------------------------------

#define AUTOSAVE_IN  1000 * 3  // seconds
#define MAXWAIT      1000 * 15 // seconds

AutoSaver::AutoSaver(QObject *parent) : QObject(parent)
{
    Q_ASSERT(parent);
}

AutoSaver::~AutoSaver()
{
    if (m_timer.isActive())
        qWarning() << "AutoSaver: still active when destroyed, changes not saved.";
}

void AutoSaver::changeOccurred()
{
    if (m_firstChange.isNull())
        m_firstChange.start();

    if (m_firstChange.elapsed() > MAXWAIT) {
        saveIfNecessary();
    } else {
        m_timer.start(AUTOSAVE_IN, this);
    }
}

void AutoSaver::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timer.timerId()) {
        saveIfNecessary();
    } else {
        QObject::timerEvent(event);
    }
}

void AutoSaver::saveIfNecessary()
{
    if (!m_timer.isActive())
        return;
    m_timer.stop();
    m_firstChange = QTime();
    if (!QMetaObject::invokeMethod(parent(), "save", Qt::DirectConnection)) {
        qWarning() << "AutoSaver: error invoking slot save() on parent";
    }
}

// ----------------------------------------------------------------------------

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
    connect(this, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

    QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
#if QT_VERSION >= 0x050000
    QString location = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    QString location = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif
    diskCache->setCacheDirectory(location);
    setCache(diskCache);
}

void NetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
    QWidget *mainWindow = Gui::getMainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui_DlgAuthorization passwordDialog;
    passwordDialog.setupUi(&dialog);
    dialog.adjustSize();

    QString introMessage = tr("<qt>Enter username and password for \"%1\" at %2</qt>");
#if QT_VERSION >= 0x050000
    introMessage = introMessage.arg(QString(reply->url().toString()).toHtmlEscaped(), QString(reply->url().toString()).toHtmlEscaped());
#else
    introMessage = introMessage.arg(Qt::escape(reply->url().toString()), Qt::escape(reply->url().toString()));
#endif
    passwordDialog.siteDescription->setText(introMessage);
    passwordDialog.siteDescription->setWordWrap(true);

    if (dialog.exec() == QDialog::Accepted) {
        auth->setUser(passwordDialog.username->text());
        auth->setPassword(passwordDialog.password->text());
    }
}

void NetworkAccessManager::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth)
{
    QWidget *mainWindow = Gui::getMainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui_DlgAuthorization proxyDialog;
    proxyDialog.setupUi(&dialog);
    dialog.adjustSize();

    QString introMessage = tr("<qt>Connect to proxy \"%1\" using:</qt>");
#if QT_VERSION >= 0x050000
    introMessage = introMessage.arg(QString(proxy.hostName()).toHtmlEscaped());
#else
    introMessage = introMessage.arg(Qt::escape(proxy.hostName()));
#endif
    proxyDialog.siteDescription->setText(introMessage);
    proxyDialog.siteDescription->setWordWrap(true);

    if (dialog.exec() == QDialog::Accepted) {
        auth->setUser(proxyDialog.username->text());
        auth->setPassword(proxyDialog.password->text());
    }
}

// ----------------------------------------------------------------------------

DownloadItem::DownloadItem(QNetworkReply *reply, bool requestFileName, QWidget *parent)
    : QWidget(parent)
    , m_reply(reply)
    , m_requestFileName(requestFileName)
    , m_bytesReceived(0)
{
    setupUi(this);
    QPalette p = downloadInfoLabel->palette();
    p.setColor(QPalette::Text, Qt::darkGray);
    downloadInfoLabel->setPalette(p);
    progressBar->setMaximum(0);
    tryAgainButton->hide();
    connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
    connect(openButton, SIGNAL(clicked()), this, SLOT(open()));
    connect(tryAgainButton, SIGNAL(clicked()), this, SLOT(tryAgain()));

    init();
}

void DownloadItem::init()
{
    if (!m_reply)
        return;

    // attach to the m_reply
    m_url = m_reply->url();
    m_reply->setParent(this);
    connect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadReadyRead()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(error(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)),
            this, SLOT(downloadProgress(qint64, qint64)));
    connect(m_reply, SIGNAL(metaDataChanged()),
            this, SLOT(metaDataChanged()));
    connect(m_reply, SIGNAL(finished()),
            this, SLOT(finished()));

    // reset info
    downloadInfoLabel->clear();
    progressBar->setValue(0);
    getFileName();

    // start timer for the download estimation
    m_downloadTime.start();

    if (m_reply->error() != QNetworkReply::NoError) {
        error(m_reply->error());
        finished();
    }
}

QString DownloadItem::getDownloadDirectory() const
{
    QString exe = QString::fromLatin1(App::GetApplication().getExecutableName());
#if QT_VERSION >= 0x050000
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
    QString path = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif
    QString dirPath = QDir(path).filePath(exe);
    Base::Reference<ParameterGrp> hPath = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("Preferences")->GetGroup("General");
    std::string dir = hPath->GetASCII("DownloadPath", "");
    if (!dir.empty()) {
        dirPath = QString::fromUtf8(dir.c_str());
    }

#if QT_VERSION >= 0x050000
    if (QFileInfo::exists(dirPath) || QDir().mkpath(dirPath)) {
#else
    if (QFileInfo(dirPath).exists() || QDir().mkpath(dirPath)) {
#endif
        return dirPath;
    }
    else {
        return path;
    }
}

void DownloadItem::getFileName()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    //QString defaultLocation = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    QString defaultLocation = getDownloadDirectory();
    QString downloadDirectory = settings.value(QLatin1String("downloadDirectory"), defaultLocation).toString();
    if (!downloadDirectory.isEmpty())
        downloadDirectory += QLatin1Char('/');

    QString defaultFileName = saveFileName(downloadDirectory);
    QString fileName = defaultFileName;
    if (m_requestFileName) {
        fileName = QFileDialog::getSaveFileName(this, tr("Save File"), defaultFileName);
        if (fileName.isEmpty()) {
            m_reply->close();
            fileNameLabel->setText(tr("Download canceled: %1").arg(QFileInfo(defaultFileName).fileName()));
            return;
        }
    }
    m_output.setFileName(fileName);
    fileNameLabel->setText(QFileInfo(m_output.fileName()).fileName());
    fileNameLabel->setToolTip(m_output.fileName());
    if (m_requestFileName)
        downloadReadyRead();
}

QString DownloadItem::saveFileName(const QString &directory) const
{
    // Move this function into QNetworkReply to also get file name sent from the server
    QString path = m_url.path();
    if (!m_fileName.isEmpty())
        path = m_fileName;
    QFileInfo info(path);
    QString baseName = info.completeBaseName();
    QString endName = info.suffix();

    if (baseName.isEmpty()) {
        baseName = QLatin1String("unnamed_download");
        qDebug() << "DownloadManager:: downloading unknown file:" << m_url;
    }
    QString name = directory + baseName + QLatin1Char('.') + endName;
    if (QFile::exists(name)) {
        // already exists, don't overwrite
        int i = 1;
        do {
            name = directory + baseName + QLatin1Char('-') + QString::number(i++) + QLatin1Char('.') + endName;
        } while (QFile::exists(name));
    }
    return name;
}


void DownloadItem::stop()
{
    setUpdatesEnabled(false);
    stopButton->setEnabled(false);
    stopButton->hide();
    tryAgainButton->setEnabled(true);
    tryAgainButton->show();
    setUpdatesEnabled(true);
    m_reply->abort();
}

void DownloadItem::open()
{
    QFileInfo info(m_output);
    QString selectedFilter;
    QStringList fileList;
    fileList << info.absoluteFilePath();
    SelectModule::Dict dict = SelectModule::importHandler(fileList, selectedFilter);

    // load the files with the associated modules
    if (!dict.isEmpty()) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
                Gui::Application::Instance->importFrom(it.key().toUtf8(),
                    doc->getDocument()->getName(), it.value().toLatin1());
            }
        }
        else {
            for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
                Gui::Application::Instance->open(it.key().toUtf8(), it.value().toLatin1());
            }
        }
    }
    else {
        QUrl url = QUrl::fromLocalFile(info.absolutePath());
        QDesktopServices::openUrl(url);
    }
}

void DownloadItem::openFolder()
{
    QFileInfo info(m_output);
    QUrl url = QUrl::fromLocalFile(info.absolutePath());
    QDesktopServices::openUrl(url);
}

void DownloadItem::tryAgain()
{
    if (!tryAgainButton->isEnabled())
        return;

    tryAgainButton->setEnabled(false);
    tryAgainButton->setVisible(false);
    stopButton->setEnabled(true);
    stopButton->setVisible(true);
    progressBar->setVisible(true);

    QNetworkReply *r = DownloadManager::getInstance()->networkAccessManager()->get(QNetworkRequest(m_url));
    if (m_reply)
        m_reply->deleteLater();
    if (m_output.exists())
        m_output.remove();
    m_reply = r;
    init();
    /*emit*/ statusChanged();
}

void DownloadItem::contextMenuEvent (QContextMenuEvent * e)
{
    QMenu menu;
    QAction* a = menu.addAction(tr("Open containing folder"), this, SLOT(openFolder()));
    a->setEnabled(m_output.exists());
    menu.exec(e->globalPos());
}

void DownloadItem::downloadReadyRead()
{
    if (m_requestFileName && m_output.fileName().isEmpty())
        return;
    if (!m_output.isOpen()) {
        // in case someone else has already put a file there
        if (!m_requestFileName)
            getFileName();
        if (!m_output.open(QIODevice::WriteOnly)) {
            downloadInfoLabel->setText(tr("Error opening saved file: %1")
                    .arg(m_output.errorString()));
            stopButton->click();
            /*emit*/ statusChanged();
            return;
        }
        downloadInfoLabel->setToolTip(m_url.toString());
        /*emit*/ statusChanged();
    }
    if (-1 == m_output.write(m_reply->readAll())) {
        downloadInfoLabel->setText(tr("Error saving: %1")
                .arg(m_output.errorString()));
        stopButton->click();
    }
}

void DownloadItem::error(QNetworkReply::NetworkError)
{
    qDebug() << "DownloadItem::error" << m_reply->errorString() << m_url;
    downloadInfoLabel->setText(tr("Network Error: %1").arg(m_reply->errorString()));
    tryAgainButton->setEnabled(true);
    tryAgainButton->setVisible(true);
}

void DownloadItem::metaDataChanged()
{
    // https://tools.ietf.org/html/rfc6266
    if (m_reply->hasRawHeader(QByteArray("Content-Disposition"))) {
        QByteArray header = m_reply->rawHeader(QByteArray("Content-Disposition"));
        int index = header.indexOf("filename=");
        if (index >= 0) {
            header = header.mid(index+9);
            if (header.startsWith("\"") || header.startsWith("'"))
                header = header.mid(1);
            if ((index = header.lastIndexOf("\"")) > 0)
                header = header.left(index);
            else if ((index = header.lastIndexOf("'")) > 0)
                header = header.left(index);
            m_fileName = QUrl::fromPercentEncoding(header);
        }
        // Sometimes "filename=" and "filename*=UTF-8''" is set.
        // So, search for this too.
        index = header.indexOf("filename*=UTF-8''");
        if (index >= 0) {
            header = header.mid(index+17);
            if (header.startsWith("\"") || header.startsWith("'"))
                header = header.mid(1);
            if ((index = header.lastIndexOf("\"")) > 0)
                header = header.left(index);
            else if ((index = header.lastIndexOf("'")) > 0)
                header = header.left(index);
            m_fileName = QUrl::fromPercentEncoding(header);
        }
    }

    QUrl url = m_reply->url();

    // If this is a redirected url use this instead
    QUrl redirectUrl = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (!redirectUrl.isEmpty()) {
        QString s = redirectUrl.toString();
        std::cout << "Redirected to " << s.toStdString() << std::endl;

        QVariant header = m_reply->header(QNetworkRequest::LocationHeader);
        QString loc = header.toString();
        Q_UNUSED(loc);

        if (url != redirectUrl) {
            url = redirectUrl;

            disconnect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadReadyRead()));
            disconnect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
                       this, SLOT(error(QNetworkReply::NetworkError)));
            disconnect(m_reply, SIGNAL(downloadProgress(qint64, qint64)),
                       this, SLOT(downloadProgress(qint64, qint64)));
            disconnect(m_reply, SIGNAL(metaDataChanged()),
                       this, SLOT(metaDataChanged()));
            disconnect(m_reply, SIGNAL(finished()),
                       this, SLOT(finished()));
            m_reply->close();
            m_reply->deleteLater();

            m_reply = DownloadManager::getInstance()->networkAccessManager()->get(QNetworkRequest(url));
            init();
        }
    }
}

void DownloadItem::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_bytesReceived = bytesReceived;
    if (bytesTotal == -1) {
        progressBar->setValue(0);
        progressBar->setMaximum(0);
    } else {
        progressBar->setValue(bytesReceived);
        progressBar->setMaximum(bytesTotal);
    }
    updateInfoLabel();
}

void DownloadItem::updateInfoLabel()
{
    //if (m_reply->error() == QNetworkReply::NoError)
    //    return;

    qint64 bytesTotal = progressBar->maximum();
    bool running = !downloadedSuccessfully();

    // update info label
    double speed = m_bytesReceived * 1000.0 / m_downloadTime.elapsed();
    double timeRemaining = ((double)(bytesTotal - m_bytesReceived)) / speed;
    QString timeRemainingString = tr("seconds");
    if (timeRemaining > 60) {
        timeRemaining = timeRemaining / 60;
        timeRemainingString = tr("minutes");
    }
    timeRemaining = floor(timeRemaining);

    // When downloading the eta should never be 0
    if (timeRemaining == 0)
        timeRemaining = 1;

    QString info;
    if (running) {
        QString remaining;
        if (bytesTotal != 0)
            remaining = tr("- %4 %5 remaining")
            .arg(timeRemaining)
            .arg(timeRemainingString);
        info = QString(tr("%1 of %2 (%3/sec) %4"))
            .arg(dataString(m_bytesReceived),
                 bytesTotal == 0 ? tr("?") : dataString(bytesTotal),
                 dataString((int)speed),
                 remaining);
    } else {
        if (m_bytesReceived == bytesTotal)
            info = dataString(m_output.size());
        else
            info = tr("%1 of %2 - Stopped")
                .arg(dataString(m_bytesReceived),
                     dataString(bytesTotal));
    }
    downloadInfoLabel->setText(info);
}

QString DownloadItem::dataString(int size) const
{
    QString unit;
    if (size < 1024) {
        unit = tr("bytes");
    } else if (size < 1024*1024) {
        size /= 1024;
        unit = tr("kB");
    } else {
        size /= 1024*1024;
        unit = tr("MB");
    }
    return QString(QLatin1String("%1 %2")).arg(size).arg(unit);
}

bool DownloadItem::downloading() const
{
    return (progressBar->isVisible());
}

bool DownloadItem::downloadedSuccessfully() const
{
    return (stopButton->isHidden() && tryAgainButton->isHidden());
}

void DownloadItem::finished()
{
    progressBar->hide();
    stopButton->setEnabled(false);
    stopButton->hide();
    m_output.close();
    updateInfoLabel();
    /*emit*/ statusChanged();
}

#include "moc_DownloadItem.cpp"
