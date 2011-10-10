/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QAbstractTextDocumentLayout>
# include <QApplication>
# include <QClipboard>
# include <QDateTime>
# include <QHBoxLayout>
# include <QMessageBox>
# include <QPainter>
# include <QPrinter>
# include <QPrintDialog>
# include <QScrollBar>
# if QT_VERSION >= 0x040400
# include <QWebFrame>
# include <QWebView>
# include <QWebSettings>
# endif
# include <QStatusBar>
# include <QTextBlock>
# include <QTextCodec>
# include <QTextStream>
# include <QTimer>
# include <QFileInfo>
#endif

#include "BrowserView.h"
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/ProgressBar.h>
#include <Gui/DownloadDialog.h>
#include <Gui/Command.h>
#include <Gui/OnlineDocumentation.h>

#include <Base/Parameter.h>
#include <Base/Exception.h>

using namespace WebGui;
using namespace Gui;


/* TRANSLATOR Gui::BrowserView */

/**
 *  Constructs a BrowserView which is a child of 'parent', with the
 *  name 'name'.
 */
BrowserView::BrowserView(QWidget* parent)
    : MDIView(0,parent,0),
      WindowParameter( "Browser" ),
      isLoading(false),
      textSizeMultiplier(1.0)
{
    WebView = new QWebView(this);
    setCentralWidget(WebView);

    WebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    connect(WebView, SIGNAL(loadStarted()),
            this, SLOT(onLoadStarted()));
    connect(WebView, SIGNAL(loadProgress(int)),
            this, SLOT(onLoadProgress(int)));
    connect(WebView, SIGNAL(loadFinished(bool)),
            this, SLOT(onLoadFinished()));
    connect(WebView, SIGNAL(linkClicked(const QUrl &)),
            this, SLOT(onLinkClicked(const QUrl &)));
}

/** Destroys the object and frees any allocated resources */
BrowserView::~BrowserView()
{
    delete WebView;
}

void BrowserView::onLinkClicked (const QUrl & url) 
{
    QString scheme   = url.scheme();
    QString host     = url.host();

    // path handling 
    QString path     = url.path();
    QFileInfo fi(path);
    QString ext = fi.completeSuffix();

    //QString fragment = url.	fragment();

    if(scheme==QString::fromLatin1("http")){
/*         Dialog::DownloadDialog Dlg (url,QString::fromLatin1("c:/temp/test.fcstd"));
        int result = Dlg.exec();
       if(ext ==QString::fromLatin1("fcstd") )
             Gui::Command::doCommand(Gui::Command::Gui,"Gui.open('%s')",);

        load(url);*/
        OpenURLInBrowser(url.toString().toLatin1());
    }
    // run scripts if not from somewhere else!
    if((scheme.size() < 2 || scheme==QString::fromLatin1("file"))&& host.isEmpty()){
        QFileInfo fi(path);
        if(fi.exists()){
            QString ext = fi.completeSuffix();
            if (ext == QString::fromLatin1("py")) {
                try {
                    Gui::Command::doCommand(Gui::Command::Gui,"execfile('%s')",(const char*) fi.absoluteFilePath().	toLocal8Bit());
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(this, tr("Error"), QString::fromUtf8(e.what()));
                }
            }
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("File does not exist!"),
            fi.absoluteFilePath ());
        }
    }
}

bool BrowserView::chckHostAllowed(const QString& host)
{
    // only check if a local file, later we can do here a dialog to ask the user if 
    return host.isEmpty();
}


void BrowserView::load(const char* URL)
{
    QUrl url = QUrl(QString::fromUtf8(URL));
    load(url);
}

void BrowserView::load(const QUrl & url)
{
    if(isLoading)
        stop();

    WebView->load(url);
    WebView->setUrl(url);
    if(url.scheme().size() < 2){
        QString path     = url.path();
        QFileInfo fi(path);
        QString name = fi.baseName();

        setWindowTitle(name);
    }else 
        setWindowTitle(url.host());

    setWindowIcon(QWebSettings::iconForUrl(url));
}

void BrowserView::setHtml(const QString& HtmlCode,const QUrl & BaseUrl,const QString& TabName)
{
    if(isLoading)
        stop();

    WebView->setHtml(HtmlCode,BaseUrl);
    setWindowTitle(TabName);
    setWindowIcon(QWebSettings::iconForUrl(BaseUrl));
}

void BrowserView::stop(void)
{
    WebView->stop();
}

void BrowserView::onLoadStarted()
{
    QProgressBar* bar = Gui::Sequencer::instance()->getProgressBar();
    bar->setRange(0, 100);
    bar->show();
    Gui::getMainWindow()->statusBar()->showMessage(tr("Loading %1...").arg(WebView->url().toString()));
    isLoading = true;
}

void BrowserView::onLoadProgress(int step)
{
    QProgressBar* bar = Gui::Sequencer::instance()->getProgressBar();
    bar->setValue(step);
}

void BrowserView::onLoadFinished()
{
    QProgressBar* bar = Sequencer::instance()->getProgressBar();
    bar->setValue(100);
    bar->hide();
    getMainWindow()->statusBar()->showMessage(QString());
    isLoading = false;
}

void BrowserView::OnChange(Base::Subject<const char*> &rCaller,const char* rcReason)
{
}

/**
 * Runs the action specified by \a pMsg.
 */
bool BrowserView::onMsg(const char* pMsg,const char** ppReturn)
{
    if (strcmp(pMsg,"Back")==0){
        WebView->back();
        return true;
    } else if (strcmp(pMsg,"Next")==0){
        WebView->forward();
        return true;
    } else if (strcmp(pMsg,"Refresh")==0){
        WebView->reload();
        return true;
    } else if (strcmp(pMsg,"Stop")==0){
        stop();
        return true;
    } else if (strcmp(pMsg,"ZoomIn")==0){
        textSizeMultiplier += 0.2f;
        WebView->setTextSizeMultiplier(textSizeMultiplier);
        return true;
    } else if (strcmp(pMsg,"ZoomOut")==0){
        textSizeMultiplier -= 0.2f;
        WebView->setTextSizeMultiplier(textSizeMultiplier);
        return true;
    }

    return false;
}

/**
 * Checks if the action \a pMsg is available. This is for enabling/disabling
 * the corresponding buttons or menu items for this action.
 */
bool BrowserView::onHasMsg(const char* pMsg) const
{
    if (strcmp(pMsg,"Back")==0)  return true;
    if (strcmp(pMsg,"Next")==0)  return true;
    if (strcmp(pMsg,"Refresh")==0) return !isLoading;
    if (strcmp(pMsg,"Stop")==0) return isLoading;
    if (strcmp(pMsg,"ZoomIn")==0) return true;
    if (strcmp(pMsg,"ZoomOut")==0) return true;

    return false;
}

/** Checking on close state. */
bool BrowserView::canClose(void)
{
   return true;
}

#include "moc_BrowserView.cpp"

