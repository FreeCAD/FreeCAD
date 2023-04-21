/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_NETWORKRETRIEVER_H
#define GUI_NETWORKRETRIEVER_H

#include <QProcess>

#include "Command.h"


namespace Gui {

/**
 * The NetworkRetriever class encapsulates the GNU tool \a wget.
 * wget is a non-interactive network retriever to download e.g. the whole
 * file structure from a server.
 * \author Werner Mayer
 */
class NetworkRetriever : public QObject
{
    Q_OBJECT

public:
    NetworkRetriever( QObject * parent = nullptr );
    ~NetworkRetriever() override;

    void setNumberOfTries( int );
    void setOutputFile( const QString& );
    void setEnableTimestamp(bool);
    void setProxy( const QString&, const QString& = QString(), const QString& = QString() );
    void setEnableRecursive( bool, int = 0 );
    void setFollowRelative( bool );
    void setEnableConvert( bool );
    void setFetchImages( bool );
    void setEnableHTMLExtension( bool );
    void setNoParent( bool );

    void setOutputDirectory( const QString& );
    bool startDownload( const QString& );
    bool isDownloading() const;
    void abort();

    static bool testWget();

Q_SIGNALS:
    void wgetExited();

private Q_SLOTS:
    void testFailure();
    void wgetFinished(int, QProcess::ExitStatus);

private:
    QProcess* wget;
    struct NetworkRetrieverP* d;
};

// --------------------------------------------------------------------

class StdCmdDownloadOnlineHelp : public QObject, public Command
{
    Q_OBJECT

public:
    StdCmdDownloadOnlineHelp(QObject * parent = nullptr);
    ~StdCmdDownloadOnlineHelp() override;
    /** i18n stuff of the command. */
    void languageChange() override;

protected:
    void activated(int iMsg) override;

    /** Creates the action object. */
    Action* createAction() override;

private Q_SLOTS:
    void wgetFinished();

private:
    NetworkRetriever* wget;
};

} // namespace Gui

#endif // GUI_NETWORKRETRIEVER_H
