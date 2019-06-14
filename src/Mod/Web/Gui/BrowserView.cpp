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
# include <QStatusBar>
# include <QTextBlock>
# include <QTextCodec>
# include <QTextStream>
# include <QTimer>
# include <QFileInfo>
# include <QDesktopServices>
# include <QMenu>
# include <QDesktopWidget>
# include <QSignalMapper>
# include <QPointer>
# include <QDir>
# include <QLineEdit>
#endif


#if QT_VERSION >= 0x050700 && defined(QTWEBENGINE)
# include <QWebEnginePage>
# include <QWebEngineView>
# include <QWebEngineSettings>
# include <QWebEngineProfile>
# include <QWebEngineContextMenuData>
# include <QWebEngineUrlRequestInterceptor>
# include <QWebEngineUrlRequestInfo>
# define QWEBVIEW QWebEngineView
# define QWEBPAGE QWebEnginePage
#elif QT_VERSION >= 0x040400 && defined(QTWEBKIT)
# include <QWebFrame>
# include <QWebView>
# include <QWebSettings>
# include <QNetworkAccessManager>
# define QWEBVIEW QWebView
# define QWEBPAGE QWebPage
#endif

#include <QLatin1String>
#include <QRegExp>
#include "BrowserView.h"
#include "CookieJar.h"
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/ProgressBar.h>
#include <Gui/Command.h>
#include <Gui/OnlineDocumentation.h>
#include <Gui/DownloadManager.h>
#include <Gui/TextDocumentEditorView.h>

#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <CXX/Extensions.hxx>

FC_LOG_LEVEL_INIT("Web",true,true)

using namespace WebGui;
using namespace Gui;

namespace WebGui {
enum WebAction {
    OpenLink = 0xff,
#ifdef QTWEBENGINE
    ViewSource = QWebEnginePage::ViewSource
#else
    ViewSource = 200 // QWebView doesn't have a ViewSource option
#endif
};

#ifdef QTWEBENGINE
class WebEngineUrlRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
public:
    WebEngineUrlRequestInterceptor(BrowserView *parent) :
        QWebEngineUrlRequestInterceptor(parent),
        m_parent(parent)
    {
    }

    void interceptRequest(QWebEngineUrlRequestInfo &info)
    {
        // do something with this resource, click or get img for example
        if (info.navigationType() == QWebEngineUrlRequestInfo::NavigationTypeLink) {
            // wash out windows file:///C:/something ->file://C:/something
            QUrl url = info.requestUrl();
            QRegExp re(QLatin1String("^/([a-zA-Z]\\:.*)")); // match & catch drive letter forward

            if (url.host().isEmpty() && url.isLocalFile() && re.exactMatch(url.path()))
                // clip / in file urs ie /C:/something -> C:/something
                url.setPath(re.cap(1));

            // invoke thread safe.
            QMetaObject::invokeMethod(m_parent, "urlFilter", Q_ARG(QUrl, url));
        }
    }

private:
    BrowserView *m_parent;
};
#endif

// ---------------------------------------------------------------------------------------------
UrlWidget::UrlWidget(BrowserView *view) :
    QLineEdit(view), m_view(view)
{
    setText(QLatin1String("https://"));
    hide();
}
UrlWidget::~UrlWidget()
{
}

void UrlWidget::keyPressEvent(QKeyEvent *keyEvt)
{
    switch (keyEvt->key()) {
    case Qt::Key_Escape:
        hide();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_view->load(text().toLatin1());
        hide();
        break;
    default:
        QLineEdit::keyPressEvent(keyEvt);
    }
}

void UrlWidget::display()
{
    setFixedWidth(m_view->size().width());
    setText(m_view->url().toString());
    show();
    setFocus(Qt::ActiveWindowFocusReason);
}

// ---------------------------------------------------------------------------------------------

class BrowserViewPy : public Py::PythonExtension<BrowserViewPy>
{
public:
    static void init_type(void);    // announce properties and methods

    BrowserViewPy(BrowserView* view);
    ~BrowserViewPy();

    Py::Object repr();

    Py::Object setHtml(const Py::Tuple&);

private:
    QPointer<BrowserView> myWebView;
};

void BrowserViewPy::init_type()
{
    behaviors().name("BrowserView");
    behaviors().doc("Python interface class to BrowserView");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    behaviors().readyType();

    add_varargs_method("setHtml",&BrowserViewPy::setHtml,"setHtml(str)");
}

BrowserViewPy::BrowserViewPy(BrowserView* view) : myWebView(view)
{
}

BrowserViewPy::~BrowserViewPy()
{
}

Py::Object BrowserViewPy::repr()
{
    std::stringstream s;
    s << "<BrowserView at " << this << ">";
    return Py::String(s.str());
}

Py::Object BrowserViewPy::setHtml(const Py::Tuple& args)
{
    char* HtmlCode;
    char* BaseUrl;
    if (!PyArg_ParseTuple(args.ptr(), "et|s","utf-8",&HtmlCode,&BaseUrl))
        throw Py::Exception();

    std::string EncodedHtml = std::string(HtmlCode);
    PyMem_Free(HtmlCode);

    if (myWebView)
        myWebView->setHtml(QString::fromUtf8(EncodedHtml.c_str()), QUrl(QString::fromLatin1(BaseUrl)));
    return Py::None();
}
}

/**
 *  Constructs a WebView widget which can be zoomed with Ctrl+Mousewheel
 *
 */

WebView::WebView(QWidget *parent)
    : QWEBVIEW(parent)
{
    // Increase html font size for high DPI displays
    QRect mainScreenSize = QApplication::desktop()->screenGeometry();
    if (mainScreenSize.width() > 1920){
        setTextSizeMultiplier (mainScreenSize.width()/1920.0);
    }
}

#ifdef QTWEBENGINE
// implement a custom method using font minimum size
void WebView::setTextSizeMultiplier(qreal factor)
{
    QWebEngineSettings *sett = settings();
    int fontSize = sett->fontSize(QWebEngineSettings::MinimumFontSize);
    fontSize = static_cast<int>(fontSize * factor);
    sett->setFontSize(QWebEngineSettings::MinimumFontSize, fontSize);
}
#else // QTWEBKIT

void WebView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton) {
        QWebHitTestResult r = page()->mainFrame()->hitTestContent(event->pos());
        if (!r.linkUrl().isEmpty()) {
            openLinkInNewWindow(r.linkUrl());
            return;
        }
    }
    QWEBVIEW::mousePressEvent(event);
}
#endif

void WebView::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        qreal factor = zoomFactor() + (-event->delta() / 800.0);
        setZoomFactor(factor);
        event->accept();
        return;
    }
    QWEBVIEW::wheelEvent(event);
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
#ifdef QTWEBENGINE
    const QWebEngineContextMenuData r = page()->contextMenuData();
#else
    QWebHitTestResult r = page()->mainFrame()->hitTestContent(event->pos());
#endif
    if (!r.linkUrl().isEmpty()) {
        QMenu menu(this);
        QWEBPAGE::WebAction openLink = static_cast<QWEBPAGE::WebAction>(WebAction::OpenLink);

        // building a custom signal for external browser action
        QSignalMapper* signalMapper = new QSignalMapper (&menu);
        signalMapper->setProperty("url", QVariant(r.linkUrl()));
        connect(signalMapper, SIGNAL(mapped(int)),
                this, SLOT(triggerContextMenuAction(int)));

        QAction* extAction = menu.addAction(tr("Open in External Browser"));
        connect (extAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
        signalMapper->setMapping(extAction, openLink);

        QAction* newAction = menu.addAction(tr("Open in new window"));
        connect (newAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
        signalMapper->setMapping(newAction, QWEBPAGE::OpenLinkInNewWindow);

        menu.addAction(pageAction(QWEBPAGE::DownloadLinkToDisk));
        menu.addAction(pageAction(QWEBPAGE::CopyLinkToClipboard));
        menu.exec(mapToGlobal(event->pos()));
        return;
    }
#if QT_VERSION >= 0x050800 && defined(QTWEBENGINE)
    else { // for view source
        // QWebEngine caches standardContextMenu, guard so we only add signalmapper once
        static bool firstRun = true;
        if (firstRun) {
            firstRun = false;
            QMenu *menu = page()->createStandardContextMenu();
            QList<QAction *> actions = menu->actions();
            for(QAction *ac : actions) {
                if (ac->data().toInt() == WebAction::ViewSource) {
                    QSignalMapper* signalMapper = new QSignalMapper (this);
                    signalMapper->setProperty("url", QVariant(r.linkUrl()));
                    signalMapper->setMapping(ac, WebAction::ViewSource);
                    connect(signalMapper, SIGNAL(mapped(int)),
                            this, SLOT(triggerContextMenuAction(int)));
                    connect (ac, SIGNAL(triggered()), signalMapper, SLOT(map()));
                }
            }
        }
    }
#else
    else {
        QMenu *menu = page()->createStandardContextMenu();
        QAction *ac = menu->addAction(tr("View source"));
        ac->setData(WebAction::ViewSource);
        QSignalMapper* signalMapper = new QSignalMapper (this);
        signalMapper->setProperty("url", QVariant(r.linkUrl()));
        signalMapper->setMapping(ac, WebAction::ViewSource);
        connect(signalMapper, SIGNAL(mapped(int)),
                this, SLOT(triggerContextMenuAction(int)));
        connect (ac, SIGNAL(triggered()), signalMapper, SLOT(map()));
        menu->exec(event->globalPos());
    }
#endif
    QWEBVIEW::contextMenuEvent(event);
}

void WebView::triggerContextMenuAction(int id)
{
    QObject* s = sender();
    QUrl url = s->property("url").toUrl();

    switch (id) {
    case WebAction::OpenLink:
        openLinkInExternalBrowser(url);
        break;
    case QWEBPAGE::OpenLinkInNewWindow:
        openLinkInNewWindow(url);
        break;
    case WebAction::ViewSource:
        Q_EMIT viewSource(url);
        break;
    default:
        break;
    }
}

// ------------------------------------------------------------------------

/* TRANSLATOR Gui::BrowserView */

/**
 *  Constructs a BrowserView which is a child of 'parent', with the
 *  name 'name'.
 */
BrowserView::BrowserView(QWidget* parent)
    : MDIView(0,parent,0),
      WindowParameter( "Browser" ),
      isLoading(false)
{
#if defined(QTWEBENGINE) && defined(Q_OS_LINUX)
    // Otherwise cause crash on exit, probably due to double deletion
    // Not sure if this applies to other platform
    setAttribute(Qt::WA_DeleteOnClose,false);
#endif

    view = new WebView(this);
    setCentralWidget(view);
    view->setAttribute(Qt::WA_OpaquePaintEvent, true);

    urlWgt = new UrlWidget(this);

#ifdef QTWEBKIT
    textSizeMultiplier = 1.0;

    view->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    view->page()->setForwardUnsupportedContent(true);

    // set our custom cookie manager
    FcCookieJar* cookiejar = new FcCookieJar(this);
    view->page()->networkAccessManager()->setCookieJar(cookiejar);

    // enable local storage so we can store stuff across sessions (startpage)
    QWebSettings* settings = view->settings();
    settings->setAttribute(QWebSettings::LocalStorageEnabled, true);
    settings->setLocalStoragePath(QString::fromUtf8((App::Application::getUserAppDataDir()+"webdata").c_str()));

    // setting background to white
    QPalette palette = view->palette();
    palette.setBrush(QPalette::Base, Qt::white);
    view->page()->setPalette(palette);

    connect(view, SIGNAL(linkClicked(const QUrl &)),
            this, SLOT(onLinkClicked(const QUrl &)));
    connect(view->page(), SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(onDownloadRequested(const QNetworkRequest &)));
    connect(view->page(), SIGNAL(unsupportedContent(QNetworkReply*)),
            this, SLOT(onUnsupportedContent(QNetworkReply*)));

#else // QTWEBENGINE
    // QWebEngine doesn't support direct access to network
    // nor rendering access
    QWebEngineProfile *profile = view->page()->profile();
    QString basePath = QLatin1String(App::Application::getUserAppDataDir().c_str()) + QLatin1String("webdata");
    profile->setPersistentStoragePath(basePath + QLatin1String("persistent"));
    profile->setCachePath(basePath + QLatin1String("cache"));

    interceptLinks = new WebEngineUrlRequestInterceptor(this);
    profile->setRequestInterceptor(interceptLinks);

    view->settings()->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, true);

    connect(view->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
            this, SLOT(onDownloadRequested(QWebEngineDownloadItem*)));
    connect(view->page(), SIGNAL(iconChanged(const QIcon &)),
            this, SLOT(setWindowIcon(const QIcon &)));
#endif

    connect(view, SIGNAL(viewSource(const QUrl&)),
            this, SLOT(onViewSource(const QUrl&)));
    connect(view, SIGNAL(loadStarted()),
            this, SLOT(onLoadStarted()));
    connect(view, SIGNAL(loadProgress(int)),
            this, SLOT(onLoadProgress(int)));
    connect(view, SIGNAL(loadFinished(bool)),
            this, SLOT(onLoadFinished(bool)));
    connect(view, SIGNAL(openLinkInExternalBrowser(const QUrl &)),
            this, SLOT(onOpenLinkInExternalBrowser(const QUrl &)));
    connect(view, SIGNAL(openLinkInNewWindow(const QUrl &)),
            this, SLOT(onOpenLinkInNewWindow(const QUrl &)));
}

/** Destroys the object and frees any allocated resources */
BrowserView::~BrowserView()
{
    FC_LOG("destroying browser view");
#ifdef QTWEBENGINE
    delete interceptLinks; // cleanup not handled implicitly
#endif
    delete view;
}

#ifdef QTWEBENGINE
void BrowserView::urlFilter(const QUrl &url)
#else
void BrowserView::onLinkClicked (const QUrl & url)
#endif
{
    QString scheme   = url.scheme();
    QString host     = url.host();
    //QString username = url.userName();

    // path handling
    QString path     = url.path();
    QFileInfo fi(path);
    QUrl exturl(url);

    // query
    QString q;
    if (url.hasQuery())
#if QT_VERSION >= 0x050000
        q = url.query();
#else
        q = QString::fromAscii(url.encodedQuery().data());
#endif

    //QString fragment = url.	fragment();

#ifdef QTWEBKIT
    if (scheme==QString::fromLatin1("http") || scheme==QString::fromLatin1("https")) {
        load(url);
    }
#endif
    // Small trick to force opening a link in an external browser: use exthttp or exthttps
    // Write your URL as exthttp://www.example.com
    else if (scheme==QString::fromLatin1("exthttp")) {
        exturl.setScheme(QString::fromLatin1("http"));
        QDesktopServices::openUrl(exturl);
        stop();// stop qwebengine, should do nothing in qwebkit at this point
    }
    else if (scheme==QString::fromLatin1("exthttps")) {
        exturl.setScheme(QString::fromLatin1("https"));
        QDesktopServices::openUrl(exturl);
        stop();// stop qwebengine, should do nothing in qwebkit at this point
    }
    // run scripts if not from somewhere else!
    if ((scheme.size() < 2 || scheme==QString::fromLatin1("file"))&& host.isEmpty()) {
        QFileInfo fi(path);
        if (fi.exists()) {
            QString ext = fi.completeSuffix();
            if (ext == QString::fromLatin1("py")) {
                stop(); // stop qwebengine, should do nothing in qwebkit at this point

                try {
                    if (!q.isEmpty()) {
                        // encapsulate the value in quotes
                        q = q.replace(QString::fromLatin1("="),QString::fromLatin1("=\""))+QString::fromLatin1("\"");
                        q = q.replace(QString::fromLatin1("%"),QString::fromLatin1("%%"));
                        // url queries in the form of somescript.py?key=value, the first key=value will be printed in the py console as key="value"
                        Gui::Command::doCommand(Gui::Command::Gui,q.toStdString().c_str());
                    }
                    // Gui::Command::doCommand(Gui::Command::Gui,"execfile('%s')",(const char*) fi.absoluteFilePath().	toLocal8Bit());
                    Gui::Command::doCommand(Gui::Command::Gui,"exec(open('%s').read())",(const char*) fi.absoluteFilePath()	.toLocal8Bit());
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(this, tr("Error"), QString::fromUtf8(e.what()));
                }

                App::Document *doc = BaseView::getAppDocument();
                if(doc && doc->testStatus(App::Document::PartialRestore))
                    QMessageBox::critical(this, tr("Error"), tr("There were errors while loading the file. Some data might have been modified or not recovered at all. Look in the report view for more specific information about the objects involved."));
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

#ifdef QTWEBENGINE
void BrowserView::onDownloadRequested(QWebEngineDownloadItem *request)
{
    Gui::Dialog::DownloadManager::getInstance()->download(request->url());
}

void BrowserView::setWindowIcon(const QIcon &icon)
{
    Gui::MDIView::setWindowIcon(icon);
}

void BrowserView::onViewSource(const QUrl &url)
{
    Q_UNUSED(url);
    view->page()->toHtml([=](const QString &pageSource){
        QPlainTextEdit *editorWidget = new QPlainTextEdit {};
        App::TextDocument *txtDoc = new App::TextDocument;
        TextDocumentEditorView *textDocView = new TextDocumentEditorView {
                txtDoc,
                editorWidget, getMainWindow()};
        editorWidget->setReadOnly(true);
        editorWidget->setPlainText(pageSource);
        getMainWindow()->addWindow(textDocView);
    });
}
#else
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

void BrowserView::onViewSource(const QUrl &url)
{
    Q_UNUSED(url);
    if (!view->page() || !view->page()->currentFrame())
        return;
    QString pageSource = view->page()->currentFrame()->toHtml();
    QPlainTextEdit *editorWidget = new QPlainTextEdit {};
    App::TextDocument *txtDoc = new App::TextDocument;
    TextDocumentEditorView *textDocView = new TextDocumentEditorView {
                txtDoc, editorWidget, getMainWindow()
    };
    editorWidget->setReadOnly(true);
    editorWidget->setPlainText(pageSource);
    getMainWindow()->addWindow(textDocView);
}
#endif

void BrowserView::load(const char* URL)
{
    QUrl url = QUrl::fromUserInput(QString::fromUtf8(URL));
    load(url);
}

void BrowserView::load(const QUrl & url)
{
    if (isLoading)
        stop();

    urlWgt->setText(url.toString());

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

#ifdef QTWEBKIT
    setWindowIcon(QWebSettings::iconForUrl(url));
#endif
}

void BrowserView::setHtml(const QString& HtmlCode,const QUrl & BaseUrl)
{
    if (isLoading)
        stop();

    view->setHtml(HtmlCode, BaseUrl);
#ifdef QTWEBKIT
    setWindowIcon(QWebSettings::iconForUrl(BaseUrl));
#endif
}

void BrowserView::stop(void)
{
    view->stop();
}

QUrl BrowserView::url() const
{
    return view->url();
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

void BrowserView::onOpenLinkInExternalBrowser(const QUrl& url)
{
    QDesktopServices::openUrl(url);
}

void BrowserView::onOpenLinkInNewWindow(const QUrl& url)
{
    BrowserView* view = new WebGui::BrowserView(Gui::getMainWindow());
    view->setWindowTitle(QObject::tr("Browser"));
    view->resize(400, 300);
    view->load(url);
    Gui::getMainWindow()->addWindow(view);
    Gui::getMainWindow()->setActiveWindow(this);
}

void BrowserView::OnChange(Base::Subject<const char*> &rCaller,const char* rcReason)
{
    Q_UNUSED(rCaller);
    Q_UNUSED(rcReason);
}

/**
 * Runs the action specified by \a pMsg.
 */
bool BrowserView::onMsg(const char* pMsg,const char** )
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
        qreal factor = view->zoomFactor();
        view->setZoomFactor(factor + 0.2);
        return true;
    } else if (strcmp(pMsg,"ZoomOut")==0){
        qreal factor = view->zoomFactor();
        view->setZoomFactor(factor - 0.2);
        return true;
    } else if (strcmp(pMsg,"SetURL")==0){
        if (urlWgt->isVisible())
            urlWgt->hide();
        else
            urlWgt->display();
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
    if (strcmp(pMsg,"Back")==0)
        return view->page()->action(QWEBPAGE::Back)->isEnabled();
    if (strcmp(pMsg,"Next")==0)
        return view->page()->action(QWEBPAGE::Forward)->isEnabled();
    if (strcmp(pMsg,"Refresh")==0) return !isLoading;
    if (strcmp(pMsg,"Stop")==0) return isLoading;
    if (strcmp(pMsg,"ZoomIn")==0) return true;
    if (strcmp(pMsg,"ZoomOut")==0) return true;
    if (strcmp(pMsg,"SetURL")==0) return true;

    return false;
}

/** Checking on close state. */
bool BrowserView::canClose(void)
{
    return true;
}

PyObject* BrowserView::getPyObject(void)
{
    static bool init = false;
    if (!init) {
        init = true;
        BrowserViewPy::init_type();
    }

    return new BrowserViewPy(this);
}
#include "moc_BrowserView.cpp"

