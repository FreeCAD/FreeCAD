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
#include <vector>

#include <QApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QLatin1String>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSignalMapper>
#include <QStatusBar>
#endif

#if defined(QTWEBENGINE)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QWebEngineContextMenuData>
#else
#include <QWebEngineContextMenuRequest>
#endif
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineView>
#elif defined(QTWEBKIT)
#include <QNetworkAccessManager>
#include <QWebFrame>
#include <QWebSettings>
#include <QWebView>
using QWebEngineView = QWebView;
using QWebEnginePage = QWebPage;
#endif

#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/DownloadManager.h>
#include <Gui/MDIViewPy.h>
#include <Gui/MainWindow.h>
#include <Gui/ProgressBar.h>
#include <Gui/TextDocumentEditorView.h>

#include "BrowserView.h"
#include "CookieJar.h"


using namespace WebGui;
using namespace Gui;

namespace WebGui
{
enum WebAction
{
    OpenLink = 0,
    OpenLinkInNewWindow = 1,
    ViewSource = 2  // QWebView doesn't have a ViewSource option
};

#ifdef QTWEBENGINE
class WebEngineUrlRequestInterceptor: public QWebEngineUrlRequestInterceptor
{
public:
    explicit WebEngineUrlRequestInterceptor(BrowserView* parent)
        : QWebEngineUrlRequestInterceptor(parent)
        , m_parent(parent)
    {}

    void interceptRequest(QWebEngineUrlRequestInfo& info) override
    {
        // do something with this resource, click or get img for example
        if (info.navigationType() == QWebEngineUrlRequestInfo::NavigationTypeLink) {
            // wash out windows file:///C:/something ->file://C:/something
            QUrl url = info.requestUrl();
            // match & catch drive letter forward
            QRegularExpression re(QLatin1String("^/([a-zA-Z]\\:.*)"));
            QRegularExpressionMatch match = re.match(url.path());

            if (url.host().isEmpty() && url.isLocalFile() && match.hasMatch()) {
                // clip / in file urs ie /C:/something -> C:/something
                url.setPath(match.captured(1));
            }

            // invoke thread safe.
            QMetaObject::invokeMethod(m_parent, "urlFilter", Q_ARG(QUrl, url));
        }
    }

private:
    BrowserView* m_parent;
};
#endif

// ---------------------------------------------------------------------------------------------
UrlWidget::UrlWidget(BrowserView* view)
    : QLineEdit(view)
    , m_view(view)
{
    setText(QLatin1String("https://"));
    hide();
}

UrlWidget::~UrlWidget() = default;

void UrlWidget::keyPressEvent(QKeyEvent* keyEvt)
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

class BrowserViewPy: public Py::PythonExtension<BrowserViewPy>
{
public:
    using BaseType = Py::PythonExtension<BrowserViewPy>;
    static void init_type();  // announce properties and methods

    explicit BrowserViewPy(BrowserView* view);
    ~BrowserViewPy() override;

    Py::Object repr() override;
    Py::Object getattr(const char*) override;
    Py::Object cast_to_base(const Py::Tuple&);

    Py::Object setHtml(const Py::Tuple&);
    Py::Object load(const Py::Tuple&);
    Py::Object stop(const Py::Tuple&);
    Py::Object url(const Py::Tuple&);

    BrowserView* getBrowserViewPtr();

private:
    Gui::MDIViewPy base;
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

    add_varargs_method("setHtml", &BrowserViewPy::setHtml, "setHtml(str)");
    add_varargs_method("load", &BrowserViewPy::load, "load(url)");
    add_varargs_method("stop", &BrowserViewPy::stop, "stop()");
    add_varargs_method("url", &BrowserViewPy::url, "url()");
    add_varargs_method("cast_to_base",
                       &BrowserViewPy::cast_to_base,
                       "cast_to_base() cast to MDIView class");
}

BrowserViewPy::BrowserViewPy(BrowserView* view)
    : base(view)
{}

BrowserViewPy::~BrowserViewPy() = default;

BrowserView* BrowserViewPy::getBrowserViewPtr()
{
    return qobject_cast<BrowserView*>(base.getMDIViewPtr());
}

Py::Object BrowserViewPy::cast_to_base(const Py::Tuple&)
{
    return Gui::MDIViewPy::create(base.getMDIViewPtr());
}

Py::Object BrowserViewPy::repr()
{
    std::stringstream s;
    s << "<BrowserView at " << this << ">";
    return Py::String(s.str());
}

// Since with PyCXX it's not possible to make a sub-class of MDIViewPy
// a trick is to use MDIViewPy as class member and override getattr() to
// join the attributes of both classes. This way all methods of MDIViewPy
// appear for SheetViewPy, too.
Py::Object BrowserViewPy::getattr(const char* attr)
{
    if (!getBrowserViewPtr()) {
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    std::string name(attr);
    if (name == "__dict__" || name == "__class__") {
        Py::Dict dict_self(BaseType::getattr("__dict__"));
        Py::Dict dict_base(base.getattr("__dict__"));
        for (const auto& it : dict_base) {
            dict_self.setItem(it.first, it.second);
        }
        return dict_self;
    }

    try {
        return BaseType::getattr(attr);
    }
    catch (Py::AttributeError& e) {
        e.clear();
        return base.getattr(attr);
    }
}

Py::Object BrowserViewPy::setHtml(const Py::Tuple& args)
{
    char* HtmlCode;
    char* BaseUrl;
    if (!PyArg_ParseTuple(args.ptr(), "et|s", "utf-8", &HtmlCode, &BaseUrl)) {
        throw Py::Exception();
    }

    std::string EncodedHtml = std::string(HtmlCode);
    PyMem_Free(HtmlCode);

    getBrowserViewPtr()->setHtml(QString::fromUtf8(EncodedHtml.c_str()),
                                 QUrl(QString::fromUtf8(BaseUrl)));
    return Py::None();
}

Py::Object BrowserViewPy::load(const Py::Tuple& args)
{
    char* BaseUrl;
    if (!PyArg_ParseTuple(args.ptr(), "s", &BaseUrl)) {
        throw Py::Exception();
    }

    getBrowserViewPtr()->load(BaseUrl);
    return Py::None();
}

Py::Object BrowserViewPy::stop(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    getBrowserViewPtr()->stop();
    return Py::None();
}

Py::Object BrowserViewPy::url(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    QUrl url = getBrowserViewPtr()->url();
    return Py::String(url.toString().toStdString());
}

}  // namespace WebGui

/**
 *  Constructs a WebView widget which can be zoomed with Ctrl+Mousewheel
 *
 */

WebView::WebView(QWidget* parent)
    : QWebEngineView(parent)
{
#ifdef QTWEBKIT
    // Increase html font size for high DPI displays
    QRect mainScreenSize = QApplication::primaryScreen()->geometry();
    if (mainScreenSize.width() > 1920) {
        setTextSizeMultiplier(mainScreenSize.width() / 1920.0);
    }
#endif
}

void WebView::mousePressEvent(QMouseEvent* event)
{
#ifdef QTWEBKIT
    if (event->button() == Qt::MiddleButton) {
        QWebHitTestResult r = page()->mainFrame()->hitTestContent(event->pos());
        if (!r.linkUrl().isEmpty()) {
            openLinkInNewWindow(r.linkUrl());
            return;
        }
    }
#endif
    QWebEngineView::mousePressEvent(event);
}

void WebView::wheelEvent(QWheelEvent* event)
{
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        qreal factor = zoomFactor() + (-event->angleDelta().y() / 800.0);
        setZoomFactor(factor);
        event->accept();
        return;
    }
    QWebEngineView::wheelEvent(event);
}

void WebView::contextMenuEvent(QContextMenuEvent* event)
{
#ifdef QTWEBENGINE
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QWebEngineContextMenuData r = page()->contextMenuData();
    QUrl linkUrl = r.linkUrl();
#else
    const QWebEngineContextMenuRequest* r = this->lastContextMenuRequest();
    QUrl linkUrl = r->linkUrl();
#endif
#else
    QWebHitTestResult r = page()->mainFrame()->hitTestContent(event->pos());
    QUrl linkUrl = r.linkUrl();
#endif
    if (!linkUrl.isEmpty()) {
        QMenu menu(this);

        // building a custom signal for external browser action
        QSignalMapper* signalMapper = new QSignalMapper(&menu);
        signalMapper->setProperty("url", QVariant(linkUrl));

        QAction* extAction = menu.addAction(tr("Open in External Browser"));
        signalMapper->setMapping(extAction, WebAction::OpenLink);

        QAction* newAction = menu.addAction(tr("Open in new window"));
        signalMapper->setMapping(newAction, WebAction::OpenLinkInNewWindow);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        connect(signalMapper,
                qOverload<int>(&QSignalMapper::mapped),
                this,
                &WebView::triggerContextMenuAction);
#else
        connect(signalMapper, &QSignalMapper::mappedInt, this, &WebView::triggerContextMenuAction);
#endif
        connect(extAction, &QAction::triggered, signalMapper, qOverload<>(&QSignalMapper::map));
        connect(newAction, &QAction::triggered, signalMapper, qOverload<>(&QSignalMapper::map));

        menu.addAction(pageAction(QWebEnginePage::DownloadLinkToDisk));
        menu.addAction(pageAction(QWebEnginePage::CopyLinkToClipboard));
        menu.exec(mapToGlobal(event->pos()));
        return;
    }
#if defined(QTWEBENGINE)
    else {  // for view source
        // QWebEngine caches standardContextMenu, guard so we only add signalmapper once
        static bool firstRun = true;
        if (firstRun) {
            firstRun = false;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QMenu* menu = page()->createStandardContextMenu();
#else
            QMenu* menu = this->createStandardContextMenu();
#endif
            QList<QAction*> actions = menu->actions();
            for (QAction* ac : actions) {
                if (ac->data().toInt() == WebAction::ViewSource) {
                    QSignalMapper* signalMapper = new QSignalMapper(this);
                    signalMapper->setProperty("url", QVariant(linkUrl));
                    signalMapper->setMapping(ac, WebAction::ViewSource);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                    connect(signalMapper,
                            qOverload<int>(&QSignalMapper::mapped),
                            this,
                            &WebView::triggerContextMenuAction);
#else
                    connect(signalMapper,
                            &QSignalMapper::mappedInt,
                            this,
                            &WebView::triggerContextMenuAction);
#endif
                    connect(ac,
                            &QAction::triggered,
                            signalMapper,
                            qOverload<>(&QSignalMapper::map));
                }
            }
        }
    }
#else
    else {
        QMenu* menu = page()->createStandardContextMenu();
        QAction* ac = menu->addAction(tr("View source"));
        ac->setData(WebAction::ViewSource);
        QSignalMapper* signalMapper = new QSignalMapper(this);
        signalMapper->setProperty("url", QVariant(linkUrl));
        signalMapper->setMapping(ac, WebAction::ViewSource);
        connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(triggerContextMenuAction(int)));
        connect(ac, SIGNAL(triggered()), signalMapper, SLOT(map()));
        menu->exec(event->globalPos());
    }
#endif
    QWebEngineView::contextMenuEvent(event);
}

void WebView::triggerContextMenuAction(int id)
{
    QObject* s = sender();
    QUrl url = s->property("url").toUrl();

    switch (id) {
        case WebAction::OpenLink:
            Q_EMIT openLinkInExternalBrowser(url);
            break;
        case WebAction::OpenLinkInNewWindow:
            Q_EMIT openLinkInNewWindow(url);
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

TYPESYSTEM_SOURCE_ABSTRACT(WebGui::BrowserView, Gui::MDIView)

/**
 *  Constructs a BrowserView which is a child of 'parent', with the
 *  name 'name'.
 */
BrowserView::BrowserView(QWidget* parent)
    : MDIView(nullptr, parent, Qt::WindowFlags())
    , WindowParameter("Browser")
    , isLoading(false)
{
#if defined(QTWEBENGINE)
    // Otherwise cause crash on exit, probably due to double deletion
    setAttribute(Qt::WA_DeleteOnClose, false);
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
    settings->setLocalStoragePath(
        QString::fromUtf8((App::Application::getUserAppDataDir() + "webdata").c_str()));

    // setting background to white
    QPalette palette = view->palette();
    palette.setBrush(QPalette::Base, Qt::white);
    view->page()->setPalette(palette);

    connect(view->page(),
            SIGNAL(linkHovered(const QString&, const QString&, const QString&)),
            this,
            SLOT(onLinkHovered(const QString&, const QString&, const QString&)));
    connect(view, SIGNAL(linkClicked(const QUrl&)), this, SLOT(urlFilter(const QUrl&)));
    connect(view->page(),
            SIGNAL(downloadRequested(const QNetworkRequest&)),
            this,
            SLOT(onDownloadRequested(const QNetworkRequest&)));
    connect(view->page(),
            SIGNAL(unsupportedContent(QNetworkReply*)),
            this,
            SLOT(onUnsupportedContent(QNetworkReply*)));

#else  // QTWEBENGINE
    // QWebEngine doesn't support direct access to network
    // nor rendering access
    QWebEngineProfile* profile = view->page()->profile();
    QString basePath =
        QString::fromStdString(App::Application::getUserAppDataDir()) + QLatin1String("webdata/");
    profile->setPersistentStoragePath(basePath + QLatin1String("persistent"));
    profile->setCachePath(basePath + QLatin1String("cache"));

    interceptLinks = new WebEngineUrlRequestInterceptor(this);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    profile->setUrlRequestInterceptor(interceptLinks);
#else
    profile->setRequestInterceptor(interceptLinks);
#endif

    view->settings()->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, true);
    view->settings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);

    connect(view->page()->profile(),
            &QWebEngineProfile::downloadRequested,
            this,
            &BrowserView::onDownloadRequested);
    connect(view->page(), &QWebEnginePage::iconChanged, this, &BrowserView::setWindowIcon);
    connect(view->page(), &QWebEnginePage::linkHovered, this, &BrowserView::onLinkHovered);
#endif
    connect(view, &WebView::viewSource, this, &BrowserView::onViewSource);
    connect(view, &WebView::loadStarted, this, &BrowserView::onLoadStarted);
    connect(view, &WebView::loadProgress, this, &BrowserView::onLoadProgress);
    connect(view, &WebView::loadFinished, this, &BrowserView::onLoadFinished);
    connect(view,
            &WebView::openLinkInExternalBrowser,
            this,
            &BrowserView::onOpenLinkInExternalBrowser);
    connect(view, &WebView::openLinkInNewWindow, this, &BrowserView::onOpenLinkInNewWindow);
    connect(view, &WebView::loadStarted, this, &BrowserView::onUpdateBrowserActions);
    connect(view, &WebView::loadFinished, this, &BrowserView::onUpdateBrowserActions);
}

/** Destroys the object and frees any allocated resources */
BrowserView::~BrowserView()
{
#ifdef QTWEBENGINE
    delete interceptLinks;  // cleanup not handled implicitly
#endif
    delete view;
}

void BrowserView::urlFilter(const QUrl& url)
{
    QString scheme = url.scheme();
    QString host = url.host();
    // QString username = url.userName();

    // path handling
    QString path = url.path();
    QUrl exturl(url);

    // query
    QString q;
    if (url.hasQuery()) {
        q = url.query();
    }

    // QString fragment = url.	fragment();

#ifdef QTWEBKIT
    if (scheme == QString::fromLatin1("http") || scheme == QString::fromLatin1("https")) {
        load(url);
    }
#endif
    // Small trick to force opening a link in an external browser: use exthttp or exthttps
    // Write your URL as exthttp://www.example.com
    else if (scheme == QString::fromLatin1("exthttp")) {
        exturl.setScheme(QString::fromLatin1("http"));
        QDesktopServices::openUrl(exturl);
        stop();  // stop qwebengine, should do nothing in qwebkit at this point
    }
    else if (scheme == QString::fromLatin1("exthttps")) {
        exturl.setScheme(QString::fromLatin1("https"));
        QDesktopServices::openUrl(exturl);
        stop();  // stop qwebengine, should do nothing in qwebkit at this point
    }
    // run scripts if not from somewhere else!
    if ((scheme.size() < 2 || scheme == QString::fromLatin1("file")) && host.isEmpty()) {
        QFileInfo fi(path);
        if (fi.exists()) {
            QString ext = fi.completeSuffix();
            if (ext == QString::fromLatin1("py")) {
                stop();  // stop qwebengine, should do nothing in qwebkit at this point

                try {
                    if (!q.isEmpty()) {
                        // encapsulate the value in quotes
                        q = q.replace(QString::fromLatin1("="), QString::fromLatin1("=\""))
                            + QString::fromLatin1("\"");
                        q = q.replace(QString::fromLatin1("%"), QString::fromLatin1("%%"));
                        // url queries in the form of somescript.py?key=value, the first key=value
                        // will be printed in the py console as key="value"
                        Gui::Command::doCommand(Gui::Command::Gui, q.toStdString().c_str());
                    }
                    // Gui::Command::doCommand(Gui::Command::Gui,"execfile('%s')",(const char*)
                    // fi.absoluteFilePath().	toLocal8Bit());
                    QString filename = Base::Tools::escapeEncodeFilename(fi.absoluteFilePath());
                    // Set flag indicating that this load/restore has been initiated by the user
                    // (not by a macro)
                    Gui::Application::Instance->setStatus(
                        Gui::Application::UserInitiatedOpenDocument,
                        true);
                    Gui::Command::doCommand(Gui::Command::Gui,
                                            "with open('%s') as file:\n\texec(file.read())",
                                            (const char*)filename.toUtf8());
                    Gui::Application::Instance->setStatus(
                        Gui::Application::UserInitiatedOpenDocument,
                        false);
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(this, tr("Error"), QString::fromUtf8(e.what()));
                }

                App::Document* doc = BaseView::getAppDocument();
                if (doc && doc->testStatus(App::Document::PartialRestore)) {
                    QMessageBox::critical(
                        this,
                        tr("Error"),
                        tr("There were errors while loading the file. Some data might have been "
                           "modified or not recovered at all. Look in the report view for more "
                           "specific information about the objects involved."));
                }

                if (doc && doc->testStatus(App::Document::RestoreError)) {
                    QMessageBox::critical(
                        this,
                        tr("Error"),
                        tr("There were serious errors while loading the file. Some data might have "
                           "been modified or not recovered at all. Saving the project will most "
                           "likely result in loss of data."));
                }
            }
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("File does not exist!"),
                                 fi.absoluteFilePath());
        }
    }
}

bool BrowserView::chckHostAllowed(const QString& host)
{
    // only check if a local file, later we can do here a dialog to ask the user if
    return host.isEmpty();
}

#ifdef QTWEBENGINE
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void BrowserView::onDownloadRequested(QWebEngineDownloadItem* request)
#else
void BrowserView::onDownloadRequested(QWebEngineDownloadRequest* request)
#endif
{
    QUrl url = request->url();
    if (!url.isLocalFile()) {
        request->accept();
        Gui::Dialog::DownloadManager::getInstance()->download(request->url());
    }
    else {
        request->cancel();
        Gui::getMainWindow()->loadUrls(App::GetApplication().getActiveDocument(),
                                       QList<QUrl>() << url);
    }
}

void BrowserView::setWindowIcon(const QIcon& icon)
{
    Gui::MDIView::setWindowIcon(icon);
}

void BrowserView::onLinkHovered(const QString& url)
{
    Gui::getMainWindow()->statusBar()->showMessage(url);
}

void BrowserView::onViewSource(const QUrl& url)
{
    Q_UNUSED(url);
    view->page()->toHtml([=](const QString& pageSource) {
        QPlainTextEdit* editorWidget = new QPlainTextEdit {};
        App::TextDocument* txtDoc = new App::TextDocument;
        TextDocumentEditorView* textDocView =
            new TextDocumentEditorView {txtDoc, editorWidget, getMainWindow()};
        editorWidget->setReadOnly(true);
        editorWidget->setPlainText(pageSource);
        getMainWindow()->addWindow(textDocView);
    });
}
#else
void BrowserView::onDownloadRequested(const QNetworkRequest& request)
{
    QUrl url = request.url();
    if (!url.isLocalFile()) {
        Gui::Dialog::DownloadManager::getInstance()->download(request);
    }
    else {
        Gui::getMainWindow()->loadUrls(App::GetApplication().getActiveDocument(),
                                       QList<QUrl>() << url);
    }
}

void BrowserView::onUnsupportedContent(QNetworkReply* reply)
{
    // Do not call handleUnsupportedContent() directly otherwise we won't get
    // the metaDataChanged() signal of the reply.
    Gui::Dialog::DownloadManager::getInstance()->download(reply->url());
    // Due to setting the policy QWebPage::DelegateAllLinks the urlFilter()
    // slot is called even when clicking on a downloadable file but the page
    // then fails to load. Thus, we reload the previous url.
    view->reload();
}

void BrowserView::onLinkHovered(const QString& link,
                                const QString& title,
                                const QString& textContent)
{
    Q_UNUSED(title)
    Q_UNUSED(textContent)
    QUrl url = QUrl::fromEncoded(link.toLatin1());
    QString str = url.isValid() ? url.toString() : link;
    Gui::getMainWindow()->statusBar()->showMessage(str);
}

void BrowserView::onViewSource(const QUrl& url)
{
    Q_UNUSED(url);
    if (!view->page() || !view->page()->currentFrame()) {
        return;
    }
    QString pageSource = view->page()->currentFrame()->toHtml();
    QPlainTextEdit* editorWidget = new QPlainTextEdit {};
    App::TextDocument* txtDoc = new App::TextDocument;
    TextDocumentEditorView* textDocView =
        new TextDocumentEditorView {txtDoc, editorWidget, getMainWindow()};
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

void BrowserView::load(const QUrl& url)
{
    if (isLoading) {
        stop();
    }

    urlWgt->setText(url.toString());

    view->load(url);
    view->setUrl(url);
    if (url.scheme().size() < 2) {
        QString path = url.path();
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

void BrowserView::setHtml(const QString& HtmlCode, const QUrl& BaseUrl)
{
    if (isLoading) {
        stop();
    }

    view->setHtml(HtmlCode, BaseUrl);
#ifdef QTWEBKIT
    setWindowIcon(QWebSettings::iconForUrl(BaseUrl));
#endif
}

void BrowserView::stop()
{
    view->stop();
}

QUrl BrowserView::url() const
{
    return view->url();
}

void BrowserView::onLoadStarted()
{
    QProgressBar* bar = Gui::SequencerBar::instance()->getProgressBar();
    bar->setRange(0, 100);
    bar->show();
    Gui::getMainWindow()->showMessage(tr("Loading %1...").arg(view->url().toString()));
    isLoading = true;
}

void BrowserView::onLoadProgress(int step)
{
    QProgressBar* bar = Gui::SequencerBar::instance()->getProgressBar();
    bar->setValue(step);
}

void BrowserView::onLoadFinished(bool ok)
{
    Q_UNUSED(ok)

    QProgressBar* bar = SequencerBar::instance()->getProgressBar();
    bar->setValue(100);
    bar->hide();
    Gui::MainWindow* win = Gui::getMainWindow();
    if (win) {
        win->showMessage(QString());
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

void BrowserView::onUpdateBrowserActions()
{
    CommandManager& mgr = Application::Instance->commandManager();
    std::vector<const char*> cmds = {"Web_BrowserBack",
                                     "Web_BrowserNext",
                                     "Web_BrowserRefresh",
                                     "Web_BrowserStop",
                                     "Web_BrowserZoomIn",
                                     "Web_BrowserZoomOut",
                                     "Web_BrowserSetURL"};
    for (const auto& it : cmds) {
        Gui::Command* cmd = mgr.getCommandByName(it);
        if (cmd) {
            cmd->testActive();
        }
    }
}

void BrowserView::OnChange(Base::Subject<const char*>& rCaller, const char* rcReason)
{
    Q_UNUSED(rCaller);
    Q_UNUSED(rcReason);
}

/**
 * Runs the action specified by \a pMsg.
 */
bool BrowserView::onMsg(const char* pMsg, const char**)
{
    if (strcmp(pMsg, "Back") == 0) {
        view->back();
        return true;
    }
    else if (strcmp(pMsg, "Next") == 0) {
        view->forward();
        return true;
    }
    else if (strcmp(pMsg, "Refresh") == 0) {
        view->reload();
        return true;
    }
    else if (strcmp(pMsg, "Stop") == 0) {
        stop();
        return true;
    }
    else if (strcmp(pMsg, "ZoomIn") == 0) {
        qreal factor = view->zoomFactor();
        view->setZoomFactor(factor + 0.2);
        return true;
    }
    else if (strcmp(pMsg, "ZoomOut") == 0) {
        qreal factor = view->zoomFactor();
        view->setZoomFactor(factor - 0.2);
        return true;
    }
    else if (strcmp(pMsg, "SetURL") == 0) {
        if (urlWgt->isVisible()) {
            urlWgt->hide();
        }
        else {
            urlWgt->display();
        }
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
    if (strcmp(pMsg, "Back") == 0) {
        return view->page()->action(QWebEnginePage::Back)->isEnabled();
    }
    if (strcmp(pMsg, "Next") == 0) {
        return view->page()->action(QWebEnginePage::Forward)->isEnabled();
    }
    if (strcmp(pMsg, "Refresh") == 0) {
        return !isLoading;
    }
    if (strcmp(pMsg, "Stop") == 0) {
        return isLoading;
    }
    if (strcmp(pMsg, "ZoomIn") == 0) {
        return true;
    }
    if (strcmp(pMsg, "ZoomOut") == 0) {
        return true;
    }
    if (strcmp(pMsg, "SetURL") == 0) {
        return true;
    }

    return false;
}

/** Checking on close state. */
bool BrowserView::canClose()
{
    return true;
}

PyObject* BrowserView::getPyObject()
{
    static bool init = false;
    if (!init) {
        init = true;
        BrowserViewPy::init_type();
    }

    return new BrowserViewPy(this);
}
#include "moc_BrowserView.cpp"
