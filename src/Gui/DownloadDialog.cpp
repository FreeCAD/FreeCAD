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
#endif

#include "DownloadDialog.h"

using namespace Gui::Dialog;



DownloadDialog::DownloadDialog( QUrl download_url, QString s, QString p )
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    stopped = false;
    purpose = s;
    url = download_url;
    path = p;
    statusLabel = new QLabel( QLatin1String(""), this );
    progressDialog = new QProgressDialog(this);
    http = new QHttp(this);
    buffer = new QBuffer(&ba, this);
    progressDialog->setLabel(statusLabel);
    QFileInfo fi( url.toString() );

        if ( true != url.isValid() ||
             true == url.host().isEmpty() )
        {
        return;
        }
        else
        {
        statusLabel->setText( fi.fileName() );
        }

    buffer->open(QBuffer::ReadWrite);

        if ( url.port() == -1 )
        {
        http->setHost( url.host(), 80 );
        }
        else
        {
        http->setHost( url.host(), url.port() );
        }

    http_request_id = http->get(url.path(), buffer);

    QObject::connect( http,
                      SIGNAL( requestFinished(int, bool) ),
                      this,
                      SLOT( request_finished(int, bool) ) );
    QObject::connect( http,
                      SIGNAL( dataReadProgress(int, int)),
                      this,
                      SLOT( update_progress(int, int) ) );
    QObject::connect( http,
                      SIGNAL( responseHeaderReceived(const QHttpResponseHeader &) ),
                      this,
                      SLOT( read_response_header(const QHttpResponseHeader &) ) );
    QObject::connect( progressDialog,
                      SIGNAL( canceled() ),
                      this,
                      SLOT( cancel_download() ) );

}

DownloadDialog::~DownloadDialog()
{
}

void DownloadDialog::request_finished( int request_id , bool request_error )
{
    if ( true == stopped ){
        buffer->close();
        progressDialog->hide();
        return;
    }

    if ( http_request_id != request_id ){
        return;
    }

    if ( true == request_error ) {
        stopped = true;
        buffer->close();
        progressDialog->hide();
        download_finished( this, false, purpose, path, http->errorString() );
    }else{
        stopped = true;
        buffer->close();
        progressDialog->hide();
        download_finished( this, true, purpose, path, QLatin1String("") );
    }
}

void DownloadDialog::read_response_header( const QHttpResponseHeader & response_header )
{
    if ( true == stopped ) {
        return;
    }

    if ( response_header.statusCode() != 200 ) {
        stopped = true;
        progressDialog->hide();
        http->abort();
        download_finished( this, false, purpose, path, response_header.reasonPhrase() );
    }
}

QByteArray DownloadDialog::return_data()
{
    return ba;
}

void DownloadDialog::update_progress( int read_bytes, int total_bytes )
{
    if ( true == stopped ){
        return;
    }

    progressDialog->setMaximum(total_bytes);
    progressDialog->setValue(read_bytes);
}

void DownloadDialog::cancel_download()
{
    statusLabel->setText(tr("Canceled."));
    stopped = true;
    http->abort();
    buffer->close();
    close();
}

void DownloadDialog::closeEvent( QCloseEvent * e )
{
    if ( stopped == false ){
        stopped = true;
        http->abort();
        buffer->close();
    }

    e->accept();
}

#include "moc_DownloadDialog.cpp"