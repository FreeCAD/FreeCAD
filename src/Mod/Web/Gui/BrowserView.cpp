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
# include <QNetworkRequest>
# include <QPainter>
# include <QPrinter>
# include <QPrintDialog>
# include <QScrollBar>
# include <QMouseEvent>
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
#include <Gui/DownloadManager.h>

#include <Base/Parameter.h>
#include <Base/Exception.h>

using namespace WebGui;
using namespace Gui;

/**
 *  Constructs a WebView widget which can be zoomed with Ctrl+Mousewheel
 *  
 */

WebView::WebView(QWidget *parent)
    : QWebView(parent)
{
}

void WebView::wheelEvent(QWheelEvent *event)
{
  if (QApplication::keyboardModifiers() & Qt::ControlModifier)
  {
      qreal factor = zoomFactor() + (-event->delta() / 800.0);
      setZoomFactor(factor);
      event->accept();
      return;
  }
  QWebView::wheelEvent(event);
}

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
    view = new WebView(this);
    setCentralWidget(view);

    view->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    view->page()->setForwardUnsupportedContent(true);

    // setting background to white
    QPalette palette = view->palette();
    palette.setBrush(QPalette::Base, Qt::white);
    view->page()->setPalette(palette);
    view->setAttribute(Qt::WA_OpaquePaintEvent, true);

    connect(view, SIGNAL(loadStarted()),
            this, SLOT(onLoadStarted()));
    connect(view, SIGNAL(loadProgress(int)),
            this, SLOT(onLoadProgress(int)));
    connect(view, SIGNAL(loadFinished(bool)),
            this, SLOT(onLoadFinished(bool)));
    connect(view, SIGNAL(linkClicked(const QUrl &)),
            this, SLOT(onLinkClicked(const QUrl &)));
    connect(view->page(), SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(onDownloadRequested(const QNetworkRequest &)));
    connect(view->page(), SIGNAL(unsupportedContent(QNetworkReply*)),
            this, SLOT(onUnsupportedContent(QNetworkReply*)));
}

/** Destroys the object and frees any allocated resources */
BrowserView::~BrowserView()
{
    delete view;
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

    if (scheme==QString::fromLatin1("http")) {
        load(url);
    }
    // run scripts if not from somewhere else!
    if ((scheme.size() < 2 || scheme==QString::fromLatin1("file"))&& host.isEmpty()) {
        QFileInfo fi(path);
        if (fi.exists()) {
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

void BrowserView::onDownloadRequested(const QNetworkRequest & request)
{
    Gui::Dialog::DownloadManager::getInstance()->download(request);
}

void BrowserView::onUnsupportedContent(QNetworkReply* reply)
{
    // Do not call handleUnsupportedContent() directly otherwise we won't get
    // the metaDataChanged() signal of the reply.
    Gui::Dialog::DownloadManager::getInstance()->download(reply->url());
    // Due to setting the policy QWebPage::DelegateAllLinks the onLinkClicked()
    // slot is called even when clicking on a downloadable file but the page
    // then fails to load. Thus, we reload the previous url.
    view->reload();
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

    view->load(url);
    view->setUrl(url);
    if (url.scheme().size() < 2) {
        QString path     = url.path();
        QFileInfo fi(path);
        QString name = fi.baseName();

        setWindowTitle(name);
    }
    else {
        setWindowTitle(url.host());
    }

    setWindowIcon(QWebSettings::iconForUrl(url));
}

void BrowserView::setHtml(const QString& HtmlCode,const QUrl & BaseUrl,const QString& TabName)
{
    if (isLoading)
        stop();

    view->setHtml(HtmlCode,BaseUrl);
    setWindowTitle(TabName);
    setWindowIcon(QWebSettings::iconForUrl(BaseUrl));
}

void BrowserView::stop(void)
{
    view->stop();
}

void BrowserView::onLoadStarted()
{
    QProgressBar* bar = Gui::Sequencer::instance()->getProgressBar();
    bar->setRange(0, 100);
    bar->show();
    Gui::getMainWindow()->showMessage(tr("Loading %1...").arg(view->url().toString()));
    isLoading = true;
}

void BrowserView::onLoadProgress(int step)
{
    QProgressBar* bar = Gui::Sequencer::instance()->getProgressBar();
    bar->setValue(step);
}

void BrowserView::onLoadFinished(bool ok)
{
    if (ok) {
        QProgressBar* bar = Sequencer::instance()->getProgressBar();
        bar->setValue(100);
        bar->hide();
        getMainWindow()->showMessage(QString());
    }
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
        view->back();
        return true;
    } else if (strcmp(pMsg,"Next")==0){
        view->forward();
        return true;
    } else if (strcmp(pMsg,"Refresh")==0){
        view->reload();
        return true;
    } else if (strcmp(pMsg,"Stop")==0){
        stop();
        return true;
    } else if (strcmp(pMsg,"ZoomIn")==0){
        textSizeMultiplier += 0.2f;
        view->setTextSizeMultiplier(textSizeMultiplier);
        return true;
    } else if (strcmp(pMsg,"ZoomOut")==0){
        textSizeMultiplier -= 0.2f;
        view->setTextSizeMultiplier(textSizeMultiplier);
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

