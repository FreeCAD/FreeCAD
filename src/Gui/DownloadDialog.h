/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef GUI_DOWNLOADDIALOG_H
#define GUI_DOWNLOADDIALOG_H

#include <QtCore>
#include <QDialog>
#include <QUrl>
#include <QMessageBox>
#include <QBuffer>
#include <QLabel>
#include <QProgressDialog>
#include <QHttp>
#include <QFileInfo>
#include <QCloseEvent>

namespace Gui {
namespace Dialog {

 /** Download a resource (file) from the web to a location on the disk
 * 
 */

class GuiExport DownloadDialog : public QDialog
{
    Q_OBJECT

public:

    DownloadDialog( QUrl url, QString s, QString p = QString() );
    ~DownloadDialog();
    QUrl url;
    QString purpose;
    QString path;
    QByteArray ba;
    bool stopped;
    QByteArray return_data();

public Q_SLOTS:

    void request_finished( int, bool );
    void cancel_download();
    void update_progress( int read_bytes, int total_bytes );
    void read_response_header(const QHttpResponseHeader & response_header);

Q_SIGNALS:

    void download_finished( DownloadDialog * d,
                            bool ok,
                            QString s,
                            QString p,
                            QString e );


protected:

    QHttp * http;
    int http_request_id;
    QBuffer * buffer;
    void closeEvent( QCloseEvent * e );

private:

    bool url_is_valid;
    QProgressDialog * progressDialog;
    QLabel * statusLabel;

};

} // namespace Dialog
} // namespace Gui


#endif // GUI_DOWNLOADDIALOG_H