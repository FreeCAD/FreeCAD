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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QApplication>
# include <QDir>
# include <QFileInfo>
# include <QMessageBox>
# include <QTimer>
#endif

#include <App/Application.h>
#include <Base/Console.h>

#include "NetworkRetriever.h"
#include "Action.h"
#include "BitmapFactory.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "Dialogs/ui_DlgAuthorization.h"


using namespace Gui;
using namespace Gui::Dialog;

namespace Gui {

struct NetworkRetrieverP
{
    // wget options
    int tries;
    int level;
    QString outputFile;
    QString user;
    QString passwd;
    bool timeStamp;
    bool img;
    bool convert;
    bool recurse;
    bool folRel;
    bool html;
    bool nop;
    // wget argument
    QString startUrl;

    QString proxy;
    QString dir;
    bool fail;
};

} // namespace Gui

/* TRANSLATOR Gui::NetworkRetriever */

NetworkRetriever::NetworkRetriever( QObject * parent )
  : QObject( parent )
{
    d = new NetworkRetrieverP;
    d->tries = 3;
    d->level = 1;
    d->timeStamp = false;
    d->img = false;
    d->html = false;
    d->convert = true;
    d->recurse = false;
    d->folRel = false;
    d->nop = false;

    wget = new QProcess(this);

    // if wgets exits emit signal
    connect(wget, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &NetworkRetriever::wgetFinished);

    // if application quits kill wget immediately to avoid dangling processes
    connect(qApp, &QApplication::lastWindowClosed, wget, &QProcess::kill);
}

NetworkRetriever::~NetworkRetriever()
{
    delete wget;
    delete d;
}

/**
 * This method is connected to QTimer::singleShot() and executed after 5 seconds. If wget then is still running
 * we can assume that everything is fine.
 * \note This test is necessary since \a wget writes all its output on stderr and we cannot determine surely
 * if an error occurred or not.
 *
 * There is still a problem that is not solved so far. If wget requires the proxy settings and if these
 * are not set, wget could take more than 5 seconds without downloading anything.
 */
void NetworkRetriever::testFailure()
{
    if ( wget->state() == QProcess::Running )
    {
        d->fail = false;
        QString msg = tr("Download started...");
        Base::Console().message("%s\n", msg.toUtf8().constData());
    }
}

/**
 * Sets the number of retries to \a tries. If \a tries is 0 the number of tries
 * is unlimited. The default value of the tries property is set to 3.
 */
void NetworkRetriever::setNumberOfTries( int tries )
{
    d->tries = tries;
}

/**
 * Sets output file to \a out where documents are written to.
 */
void NetworkRetriever::setOutputFile( const QString& out )
{
    d->outputFile = out;
}

/**
 * If \a ts is true the timestamping is enabled, otherwise timestamping is
 * disabled. If timestamping is enabled files are no more re-retrieved unless
 * they are newer than the local files. As default the timestamping property is disabled.
 */
void NetworkRetriever::setEnableTimestamp(bool ts)
{
    d->timeStamp = ts;
}

/**
 * If you are behind a proxy server then you have to specify your proxy url with \a proxy.
 * Moreover, if the proxy requires user authentication then you can specify the username
 * with \a user and the password with \a passwd.
 */
void NetworkRetriever::setProxy( const QString& proxy, const QString& user, const QString& passwd )
{
    d->proxy = proxy;
    d->user = user;
    d->passwd = passwd;
}

/**
 * If \a recursive is true all referenced files are downloaded recursively.
 * As default recursion is disabled. \a level specifies the maximum recursion
 * depth. If \a level is 0 the recursion depth is infinite. As default the level
 * property is 1.
 * \note: Use this with care!
 */
void NetworkRetriever::setEnableRecursive( bool recursive, int level )
{
    d->recurse = recursive;
    d->level = level;
}

/**
 * If \a folRel is true wget follows relative links only. As default
 * the follows relative property is false.
 */
void NetworkRetriever::setFollowRelative( bool folRel )
{
    d->folRel = folRel;
}

/**
 * If \a convert is true all non-relative links are converted to
 * relative links. As default the convert property is true.
 */
void NetworkRetriever::setEnableConvert( bool convert )
{
    d->convert = convert;
}

/**
 * If \a img is true wget tries to get all needed image files
 * to display the HTML page. As default this behaviour is disabled..
 */
void NetworkRetriever::setFetchImages( bool img )
{
    d->img = img;
}

/**
 * Saves all text/html documents with .html extionsion if \a html is true.
 * As default the html property is false.
 */
void NetworkRetriever::setEnableHTMLExtension( bool html )
{
    d->html = html;
}

/**
 * Do not ever ascend to the parent directory when retrieving recursively.
 */
void NetworkRetriever::setNoParent( bool nop )
{
    d->nop = nop;
}

/**
 * Sets the output directory to \a dir where all downloaded are written into.
 */
void NetworkRetriever::setOutputDirectory( const QString& dir )
{
    d->dir = dir;
}

/**
 * wget starts to download \a startUrl and all referenced pages.
 */
bool NetworkRetriever::startDownload( const QString& startUrl )
{
    if ( !testWget() )
        return false;

    d->startUrl = startUrl;

    // proxy as environment variable
    if ( !d->proxy.isEmpty() )
    {
        QStringList env = wget->environment();
        env << QStringLiteral("http_proxy=%1").arg(d->proxy);
        env << QStringLiteral("ftp_proxy=%1").arg(d->proxy);
        wget->setEnvironment(env);
    }
    else
    {
        QStringList env = wget->environment();
        env.removeAll(QStringLiteral("http_proxy=%1").arg(d->proxy));
        env.removeAll(QStringLiteral("ftp_proxy=%1").arg(d->proxy));
        wget->setEnvironment(env);
    }

    QStringList wgetArguments;

    // since the wget option '--directory-prefix' seems not to work as expected
    // and QProcess::setWorkingDirectory() fails if the 'doc' directory doesn't
    // exist we must check for this and create it if needed.
    if ( !d->dir.isEmpty() )
    {
        QDir dir(d->dir);
        if (!dir.exists(d->dir))
        {
            if (!dir.mkdir(d->dir))
            {
                Base::Console().error("Directory '%s' could not be created.", (const char*)d->dir.toLatin1());
                return true; // please, no error message
            }
        }

        wget->setWorkingDirectory( dir.path() );
    }

    // user authentication
    if ( !d->proxy.isEmpty() )
    {
        if ( !d->user.isEmpty() )
        {
            wgetArguments << QStringLiteral("--proxy-user=%1").arg( d->user );
            if ( !d->passwd.isEmpty() )
            {
                wgetArguments << QStringLiteral("--proxy-passwd=%1").arg( d->passwd );
            }
        }
    }

    // output file
    if ( !d->outputFile.isEmpty() )
        wgetArguments << QStringLiteral("--output-document=%1").arg( d->outputFile );
    // timestamping enabled -> update newer files only
    if ( d->timeStamp )
        wgetArguments << QStringLiteral("-N");
    // get all needed image files
    if ( d->img )
        wgetArguments << QStringLiteral("-p");
    // follow relative links only
    if ( d->folRel )
        wgetArguments<< QStringLiteral("-L");
    if ( d->recurse )
    {
        wgetArguments << QStringLiteral("-r");
        wgetArguments << QStringLiteral("--level=%1").arg( d->level );
    }

    if ( d->nop )
        wgetArguments << QStringLiteral("-np");

    // convert absolute links in to relative
    if ( d->convert )
        wgetArguments << QStringLiteral("-k");
    // number of tries
    wgetArguments << QStringLiteral("--tries=%1").arg( d->tries );
    // use HTML file extension
    if ( d->html )
        wgetArguments << QStringLiteral("-E");

    // start URL
    wgetArguments << startUrl;

#ifdef FC_OS_LINUX
    // on Linux it seems that we have to change cwd
    QString cwd = QDir::currentPath ();
    if ( !d->dir.isEmpty() )
    {
        QDir::setCurrent(d->dir);
    }

    wget->start(QStringLiteral("wget"), wgetArguments);
    QDir::setCurrent( cwd );
#else
    wget->start(QStringLiteral("wget"), wgetArguments);
#endif

    return wget->state() == QProcess::Running;
}

/**
 * Returns true if wget is still downloading, otherwise returns false.
 */
bool NetworkRetriever::isDownloading() const
{
    return wget->state() == QProcess::Running;
}

/**
 * Kills wget if it is still running.
 */
void NetworkRetriever::abort()
{
    if ( wget->state() == QProcess::Running)
    {
        QTimer::singleShot( 2000, wget, &QProcess::kill);
    }
}

void NetworkRetriever::wgetFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    wget->setReadChannel(QProcess::StandardError);
    if (wget->canReadLine()) {
        QByteArray data = wget->readAll();
        Base::Console().warning(data);
    }
    Q_EMIT wgetExited();
}

/**
 * This is a test if wget is in PATH environment or not.
 * If the test succeeds true is returned, false otherwise.
 */
bool NetworkRetriever::testWget()
{
    QProcess proc;
    proc.setProgram(QStringLiteral("wget"));
    proc.start();
    bool ok = proc.state() == QProcess::Running;
    proc.kill();
    proc.waitForFinished();
    return ok;
}

// --------------------------------------------------------------------

StdCmdDownloadOnlineHelp::StdCmdDownloadOnlineHelp( QObject * parent)
  : QObject(parent), Command("Std_DownloadOnlineHelp")
{
    sGroup        ="Help";
    sMenuText     = QT_TR_NOOP("Download online help");
    sToolTipText  = QT_TR_NOOP("Download %1's online help");
    sWhatsThis    = "Std_DownloadOnlineHelp";
    sStatusTip    = QT_TR_NOOP("Download %1's online help");
    sPixmap       = "help";

    wget = new NetworkRetriever( this );
    // downloading recursively and depth 5
    wget->setEnableRecursive( true, 5 );
    wget->setNumberOfTries( 3 );
    wget->setEnableHTMLExtension( true );
    wget->setEnableConvert( true );

    wget->setEnableTimestamp( true );
    wget->setFetchImages( true );
    wget->setFollowRelative( false );
    wget->setNoParent( true );

    connect(wget, &NetworkRetriever::wgetExited, this, &StdCmdDownloadOnlineHelp::wgetFinished);
}

StdCmdDownloadOnlineHelp::~StdCmdDownloadOnlineHelp()
{
    delete wget;
}

Action * StdCmdDownloadOnlineHelp::createAction()
{
    Action *pcAction;

    QString exe = QString::fromStdString(App::Application::getExecutableName());
    pcAction = new Action(this,getMainWindow());
    pcAction->setText(QCoreApplication::translate(
        this->className(), getMenuText()));
    pcAction->setToolTip(QCoreApplication::translate(
        this->className(), getToolTipText()).arg(exe));
    pcAction->setStatusTip(QCoreApplication::translate(
        this->className(), getStatusTip()).arg(exe));
    pcAction->setWhatsThis(QCoreApplication::translate(
        this->className(), getWhatsThis()).arg(exe));
    pcAction->setIcon(Gui::BitmapFactory().pixmap(getPixmap()));
    pcAction->setShortcut(QString::fromLatin1(getAccel()));

    return pcAction;
}

void StdCmdDownloadOnlineHelp::languageChange()
{
    if (_pcAction) {
        QString exe = QString::fromStdString(App::Application::getExecutableName());
        _pcAction->setText(QCoreApplication::translate(
            this->className(), getMenuText()));
        _pcAction->setToolTip(QCoreApplication::translate(
            this->className(), getToolTipText()).arg(exe));
        _pcAction->setStatusTip(QCoreApplication::translate(
            this->className(), getStatusTip()).arg(exe));
        _pcAction->setWhatsThis(QCoreApplication::translate(
            this->className(), getWhatsThis()).arg(exe));
    }
}

void StdCmdDownloadOnlineHelp::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!wget->isDownloading()) {
        ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp");
        hGrp = hGrp->GetGroup("Preferences")->GetGroup("OnlineHelp");
        std::string url = hGrp->GetASCII("DownloadURL", "www.freecad.org/wiki/");
        std::string prx = hGrp->GetASCII("ProxyText", "");
        bool bUseProxy  = hGrp->GetBool ("UseProxy", false);
        bool bAuthor    = hGrp->GetBool ("Authorize", false);

        if (bUseProxy) {
            QString username;
            QString password;

            if (bAuthor) {
                QDialog dlg(getMainWindow());
                dlg.setModal(true);
                Ui_DlgAuthorization ui;
                ui.setupUi(&dlg);

                if (dlg.exec() == QDialog::Accepted) {
                    username = ui.username->text();
                    password = ui.password->text();
                }
            }

            wget->setProxy(QString::fromLatin1(prx.c_str()), username, password);
        }

        int loop=3;
        bool canStart = false;

        // set output directory
        QString path = QString::fromStdString(App::Application::getHomePath());
        path += QStringLiteral("/doc/");
        ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/OnlineHelp");
        path = QString::fromUtf8(hURLGrp->GetASCII( "DownloadLocation", path.toLatin1() ).c_str());

        while (loop > 0) {
            loop--;
            QFileInfo fi( path);
            if (!fi.exists()) {
                if (QMessageBox::critical(getMainWindow(), tr("Non-existing directory"),
                     tr("The directory '%1' does not exist.\n\n"
                        "Do you want to specify an existing directory?").arg(fi.filePath()),
                     QMessageBox::Yes | QMessageBox::No) !=
                     QMessageBox::Yes)
                {
                    // exit the command
                    return;
                }
                else
                {
                    path = FileDialog::getExistingDirectory();
                    if ( path.isEmpty() )
                        return;
                }
            }

            if (!fi.permission( QFile::WriteUser)) {
                if (QMessageBox::critical(getMainWindow(), tr("Missing permission"),
                     tr("You don't have write permission to '%1'\n\n"
                        "Do you want to specify another directory?").arg(fi.filePath()),
                     QMessageBox::Yes | QMessageBox::No) !=
                     QMessageBox::Yes)
                {
                    // exit the command
                    return;
                }
                else {
                    path = FileDialog::getExistingDirectory();
                    if ( path.isEmpty() )
                        return;
                }
            }
            else {
                wget->setOutputDirectory( path );
                canStart = true;
                break;
            }
        }

        if (canStart) {
            bool ok = wget->startDownload(QString::fromLatin1(url.c_str()));
            if (!ok)
                Base::Console().error("The tool 'wget' couldn't be found. Please check your installation.");
            else if ( wget->isDownloading() && _pcAction )
                _pcAction->setText(tr("Stop downloading"));
        }
    }
    else // kill the process now
    {
        wget->abort();
    }
}

void StdCmdDownloadOnlineHelp::wgetFinished()
{
    if (_pcAction)
        _pcAction->setText(QCoreApplication::translate(
            this->className(), getMenuText()));
}

#include "moc_NetworkRetriever.cpp"

