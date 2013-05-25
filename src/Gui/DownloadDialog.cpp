/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QPushButton>
# include <QHBoxLayout>
#endif

#include <QAuthenticator>
#include "DownloadDialog.h"
#include "ui_DlgAuthorization.h"

using namespace Gui::Dialog;


DownloadDialog::DownloadDialog(const QUrl& url, QWidget *parent)
  : QDialog(parent), url(url)
{
    statusLabel = new QLabel(url.toString());
    progressBar = new QProgressBar(this);
    downloadButton = new QPushButton(tr("Download"));
    downloadButton->setDefault(true);
    cancelButton = new QPushButton(tr("Cancel"));
    closeButton = new QPushButton(tr("Close"));
    closeButton->setAutoDefault(false);


    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(downloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(closeButton, QDialogButtonBox::RejectRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
    cancelButton->hide();

    http = new QHttp(this);

    connect(http, SIGNAL(requestFinished(int, bool)),
            this, SLOT(httpRequestFinished(int, bool)));
    connect(http, SIGNAL(dataReadProgress(int, int)),
            this, SLOT(updateDataReadProgress(int, int)));
    connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
    connect(http, SIGNAL(authenticationRequired(const QString &, quint16, QAuthenticator *)),
            this, SLOT(slotAuthenticationRequired(const QString &, quint16, QAuthenticator *)));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadFile()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelDownload()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(statusLabel);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Download"));
}

DownloadDialog::~DownloadDialog()
{
}

void DownloadDialog::downloadFile()
{
    QFileInfo fileInfo(url.path());
    QString fileName = fileInfo.fileName();
    if (QFile::exists(fileName)) {
        if (QMessageBox::question(this, tr("Download"),
                tr("There already exists a file called %1 in "
                   "the current directory. Overwrite?").arg(fileName),
                QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No)
            return;
        QFile::remove(fileName);
    }

    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("Download"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(fileName).arg(file->errorString()));
        delete file;
        file = 0;
        return;
    }

    QHttp::ConnectionMode mode = url.scheme().toLower() == QLatin1String("https")
        ? QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp;
    http->setHost(url.host(), mode, url.port() == -1 ? 80 : url.port());
    
    if (!url.userName().isEmpty())
        http->setUser(url.userName(), url.password());

    httpRequestAborted = false;
    QByteArray path = QUrl::toPercentEncoding(url.path(), "!$&'()*+,;=:@/");
    if (path.isEmpty())
        path = "/";
    httpGetId = http->get(QString::fromAscii(path), file);

    statusLabel->setText(tr("Downloading %1.").arg(fileName));
    downloadButton->setEnabled(false);
    cancelButton->show();
    closeButton->hide();
}

void DownloadDialog::cancelDownload()
{
    statusLabel->setText(tr("Download canceled."));
    httpRequestAborted = true;
    http->abort();
    close();
}

void DownloadDialog::httpRequestFinished(int requestId, bool error)
{
    if (requestId != httpGetId)
        return;
    if (httpRequestAborted) {
        if (file) {
            file->close();
            file->remove();
            delete file;
            file = 0;
        }

        progressBar->hide();
        return;
    }

    if (requestId != httpGetId)
        return;

    progressBar->hide();
    file->close();

    if (error) {
        file->remove();
        QMessageBox::information(this, tr("Download"),
                                 tr("Download failed: %1.")
                                 .arg(http->errorString()));
    }
    else {
        QString fileName = QFileInfo(url.path()).fileName();
        statusLabel->setText(tr("Downloaded %1 to current directory.").arg(fileName));
    }

    downloadButton->setEnabled(true);
    cancelButton->hide();
    closeButton->show();
    delete file;
    file = 0;
}

void DownloadDialog::readResponseHeader(const QHttpResponseHeader &responseHeader)
{
    switch (responseHeader.statusCode()) {
    case 200:                   // Ok
    case 301:                   // Moved Permanently
    case 302:                   // Found
    case 303:                   // See Other
    case 307:                   // Temporary Redirect
        // these are not error conditions
        break;

    default:
        QMessageBox::information(this, tr("Download"),
                                 tr("Download failed: %1.")
                                 .arg(responseHeader.reasonPhrase()));
        httpRequestAborted = true;
        progressBar->hide();
        http->abort();
    }
}

void DownloadDialog::updateDataReadProgress(int bytesRead, int totalBytes)
{
    if (httpRequestAborted)
        return;

    progressBar->setMaximum(totalBytes);
    progressBar->setValue(bytesRead);
}

void DownloadDialog::slotAuthenticationRequired(const QString &hostName, quint16, QAuthenticator *authenticator)
{
    QDialog dlg;
    Ui_DlgAuthorization ui;
    ui.setupUi(&dlg);
    dlg.adjustSize();
    ui.siteDescription->setText(tr("%1 at %2").arg(authenticator->realm()).arg(hostName));

    if (dlg.exec() == QDialog::Accepted) {
        authenticator->setUser(ui.username->text());
        authenticator->setPassword(ui.password->text());
    }
}


#include "moc_DownloadDialog.cpp"
