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
#include <cstdio>
#include <iostream>

#include <QByteArray>
#include <QDockWidget>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMetaEnum>
#include <QSettings>
#include <QFileIconProvider>
#include <QUrlQuery>

#include "DownloadManager.h"
#include "ui_DownloadManager.h"
#include "DockWindowManager.h"
#include "DownloadItem.h"
#include "MainWindow.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DownloadManager */

DownloadManager* DownloadManager::self = nullptr;

DownloadManager* DownloadManager::getInstance()
{
    if (!self)
        self = new DownloadManager(Gui::getMainWindow());
    return self;
}

DownloadManager::DownloadManager(QWidget *parent)
    : QDialog(parent)
    , m_autoSaver(new AutoSaver(this))
    , m_manager(new NetworkAccessManager(this))
    , m_iconProvider(nullptr)
    , m_removePolicy(Never)
    , ui(new Ui_DownloadManager())
{
    ui->setupUi(this);
    ui->downloadsView->setShowGrid(false);
    ui->downloadsView->verticalHeader()->hide();
    ui->downloadsView->horizontalHeader()->hide();
    ui->downloadsView->setAlternatingRowColors(true);
    ui->downloadsView->horizontalHeader()->setStretchLastSection(true);
    m_model = new DownloadModel(this);
    ui->downloadsView->setModel(m_model);
    connect(ui->cleanupButton, &QPushButton::clicked, this, &DownloadManager::cleanup);
    load();

    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    QDockWidget* dw = pDockMgr->addDockWindow(QT_TR_NOOP("Download Manager"),
        this, Qt::BottomDockWidgetArea);
    dw->setFeatures(QDockWidget::DockWidgetMovable|
                    QDockWidget::DockWidgetFloatable|
                    QDockWidget::DockWidgetClosable);
    dw->setAttribute(Qt::WA_DeleteOnClose);
    dw->show();
}

DownloadManager::~DownloadManager()
{
    m_autoSaver->changeOccurred();
    m_autoSaver->saveIfNecessary();
    if (m_iconProvider)
        delete m_iconProvider;
    delete ui;
    self = nullptr;
}

void DownloadManager::closeEvent(QCloseEvent* e)
{
    QDialog::closeEvent(e);
}

int DownloadManager::activeDownloads() const
{
    int count = 0;
    for (int i = 0; i < m_downloads.count(); ++i) {
        if (m_downloads.at(i)->stopButton->isEnabled())
            ++count;
    }
    return count;
}

QUrl DownloadManager::redirectUrl(const QUrl& url) const
{
    QUrl redirectUrl = url;
    if (url.host() == QLatin1String("www.dropbox.com")) {
        QUrlQuery urlQuery(url);
        QList< QPair<QString, QString> > query = urlQuery.queryItems();
        for (const auto & it : query) {
            if (it.first == QLatin1String("dl")) {
                if (it.second == QLatin1String("0\r\n")) {
                    urlQuery.removeQueryItem(QLatin1String("dl"));
                    urlQuery.addQueryItem(QLatin1String("dl"), QLatin1String("1\r\n"));
                }
                else if (it.second == QLatin1String("0")) {
                    urlQuery.removeQueryItem(QLatin1String("dl"));
                    urlQuery.addQueryItem(QLatin1String("dl"), QLatin1String("1"));
                }
                break;
            }
        }
        redirectUrl.setQuery(urlQuery);
    }
    else {
        // When the url comes from drag and drop it may end with CR+LF. This may cause problems
        // and thus should be removed.
        QString str = redirectUrl.toString();
        if (str.endsWith(QLatin1String("\r\n"))) {
            str.chop(2);
            redirectUrl.setUrl(str);
        }
    }

    return redirectUrl;
}

void DownloadManager::download(const QNetworkRequest &request, bool requestFileName)
{
    if (request.url().isEmpty())
        return;

    std::cout << request.url().toString().toStdString() << std::endl;
    handleUnsupportedContent(m_manager->get(request), requestFileName);
}

void DownloadManager::handleUnsupportedContent(QNetworkReply *reply, bool requestFileName)
{
    if (!reply || reply->url().isEmpty())
        return;
    QVariant header = reply->header(QNetworkRequest::ContentLengthHeader);
    bool ok;
    int size = header.toInt(&ok);
    if (ok && size == 0)
        return;

    auto item = new DownloadItem(reply, requestFileName, this);
    addItem(item);
}

void DownloadManager::addItem(DownloadItem *item)
{
    connect(item, &DownloadItem::statusChanged, this, &DownloadManager::updateRow);
    int row = m_downloads.count();
    m_model->beginInsertRows(QModelIndex(), row, row);
    m_downloads.append(item);
    m_model->endInsertRows();
    updateItemCount();
    show();
    ui->downloadsView->setIndexWidget(m_model->index(row, 0), item);
    QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
    item->fileIcon->setPixmap(icon.pixmap(48, 48));
    ui->downloadsView->setRowHeight(row, item->sizeHint().height());
}

void DownloadManager::updateRow()
{
    auto item = qobject_cast<DownloadItem*>(sender());
    int row = m_downloads.indexOf(item);
    if (-1 == row)
        return;
    if (!m_iconProvider)
        m_iconProvider = new QFileIconProvider();
    QIcon icon = m_iconProvider->icon(QFileInfo(item->m_output.fileName()));
    if (icon.isNull())
        icon = style()->standardIcon(QStyle::SP_FileIcon);
    item->fileIcon->setPixmap(icon.pixmap(48, 48));
    ui->downloadsView->setRowHeight(row, item->minimumSizeHint().height());

    bool remove = false;
    if (item->downloadedSuccessfully()
        && removePolicy() == DownloadManager::SuccessFullDownload) {
        remove = true;
    }
    if (remove)
        m_model->removeRow(row);

    ui->cleanupButton->setEnabled(m_downloads.count() - activeDownloads() > 0);
}

DownloadManager::RemovePolicy DownloadManager::removePolicy() const
{
    return m_removePolicy;
}

void DownloadManager::setRemovePolicy(RemovePolicy policy)
{
    if (policy == m_removePolicy)
        return;
    m_removePolicy = policy;
    m_autoSaver->changeOccurred();
}

void DownloadManager::save() const
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
    settings.setValue(QLatin1String("removeDownloadsPolicy"), QLatin1String(removePolicyEnum.valueToKey(m_removePolicy)));
    settings.setValue(QLatin1String("size"), size());
    if (m_removePolicy == Exit)
        return;

    for (int i = 0; i < m_downloads.count(); ++i) {
        QString key = QString(QLatin1String("download_%1_")).arg(i);
        settings.setValue(key + QLatin1String("url"), m_downloads[i]->m_url);
        settings.setValue(key + QLatin1String("location"), QFileInfo(m_downloads[i]->m_output).filePath());
        settings.setValue(key + QLatin1String("done"), m_downloads[i]->downloadedSuccessfully());
    }
    int i = m_downloads.count();
    QString key = QString(QLatin1String("download_%1_")).arg(i);
    while (settings.contains(key + QLatin1String("url"))) {
        settings.remove(key + QLatin1String("url"));
        settings.remove(key + QLatin1String("location"));
        settings.remove(key + QLatin1String("done"));
        key = QString(QLatin1String("download_%1_")).arg(++i);
    }
}

void DownloadManager::load()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QSize size = settings.value(QLatin1String("size")).toSize();
    if (size.isValid())
        resize(size);
    QByteArray value = settings.value(QLatin1String("removeDownloadsPolicy"), QLatin1String("Never")).toByteArray();
    QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
    m_removePolicy = removePolicyEnum.keyToValue(value) == -1 ?
                        Never :
                        static_cast<RemovePolicy>(removePolicyEnum.keyToValue(value));

    int i = 0;
    QString key = QString(QLatin1String("download_%1_")).arg(i);
    while (settings.contains(key + QLatin1String("url"))) {
        QUrl url = settings.value(key + QLatin1String("url")).toUrl();
        QString fileName = settings.value(key + QLatin1String("location")).toString();
        bool done = settings.value(key + QLatin1String("done"), true).toBool();
        if (!url.isEmpty() && !fileName.isEmpty()) {
            auto item = new DownloadItem(nullptr, false, this);
            item->m_output.setFileName(fileName);
            item->fileNameLabel->setText(QFileInfo(item->m_output.fileName()).fileName());
            item->m_url = url;
            item->stopButton->setVisible(false);
            item->stopButton->setEnabled(false);
            item->tryAgainButton->setVisible(!done);
            item->tryAgainButton->setEnabled(!done);
            item->progressBar->setVisible(!done);
            addItem(item);
        }
        key = QString(QLatin1String("download_%1_")).arg(++i);
    }
    ui->cleanupButton->setEnabled(m_downloads.count() - activeDownloads() > 0);
}

void DownloadManager::cleanup()
{
    if (m_downloads.isEmpty())
        return;
    m_model->removeRows(0, m_downloads.count());
    updateItemCount();
    if (m_downloads.isEmpty() && m_iconProvider) {
        delete m_iconProvider;
        m_iconProvider = nullptr;
    }
    m_autoSaver->changeOccurred();
}

void DownloadManager::updateItemCount()
{
    int count = m_downloads.count();
    ui->itemCount->setText(count == 1 ? tr("1 Download") : tr("%1 Downloads").arg(count));
}

// ----------------------------------------------------------------------------

DownloadModel::DownloadModel(DownloadManager *downloadManager, QObject *parent)
    : QAbstractListModel(parent)
    , m_downloadManager(downloadManager)
{
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount(index.parent()))
        return {};
    if (role == Qt::ToolTipRole)
        if (!m_downloadManager->m_downloads.at(index.row())->downloadedSuccessfully())
            return m_downloadManager->m_downloads.at(index.row())->downloadInfoLabel->text();
    return {};
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : m_downloadManager->m_downloads.count();
}

bool DownloadModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    int lastRow = row + count - 1;
    for (int i = lastRow; i >= row; --i) {
        if (m_downloadManager->m_downloads.at(i)->downloadedSuccessfully()
            || m_downloadManager->m_downloads.at(i)->tryAgainButton->isEnabled()) {
            beginRemoveRows(parent, i, i);
            m_downloadManager->m_downloads.takeAt(i)->deleteLater();
            endRemoveRows();
        }
    }
    m_downloadManager->m_autoSaver->changeOccurred();
    return true;
}

#include "moc_DownloadManager.cpp"
