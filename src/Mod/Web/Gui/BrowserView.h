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

#ifndef WEBGUI_BROWSERVIEW_H
#define WEBGUI_BROWSERVIEW_H


#include <Gui/MDIView.h>
#include <Gui/Window.h>
#include <Mod/Web/WebGlobal.h>
#include <QLineEdit>
#include <QPointer>

#if defined(QTWEBENGINE)
#include <QWebEngineView>

namespace WebGui {
class WebEngineUrlRequestInterceptor;
}
#elif defined(QTWEBKIT)
#include <QWebView>
#endif

class QUrl;
class QNetworkRequest;
class QNetworkReply;

namespace WebGui {
class UrlWidget;

#ifdef QTWEBENGINE
class WebGuiExport WebView : public QWebEngineView
#else
class WebGuiExport WebView : public QWebView
#endif
{
    Q_OBJECT

public:
    WebView(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

private Q_SLOTS:
    void triggerContextMenuAction(int);

Q_SIGNALS:
    void openLinkInExternalBrowser(const QUrl&);
    void openLinkInNewWindow(const QUrl&);
    void viewSource(const QUrl&);
};

/**
 * A special view class which sends the messages from the application to
 * the editor and embeds it in a window.
 */
class WebGuiExport BrowserView : public Gui::MDIView,
                                 public Gui::WindowParameter
{
    Q_OBJECT

    TYPESYSTEM_HEADER();

public:
    BrowserView(QWidget* parent);
    ~BrowserView();

    void load(const char* URL);
    void load(const QUrl & url);
    void setHtml(const QString& HtmlCode,const QUrl & BaseUrl);
    void stop(void);
    QUrl url() const;

    void OnChange(Base::Subject<const char*> &rCaller,const char* rcReason);

    const char *getName(void) const {return "BrowserView";}
    virtual PyObject *getPyObject(void);

    bool onMsg(const char* pMsg,const char** ppReturn);
    bool onHasMsg(const char* pMsg) const;

    bool canClose (void);

#ifdef QTWEBENGINE
public Q_SLOTS:
    void setWindowIcon(const QIcon &icon);
#endif

protected Q_SLOTS:
    void onLoadStarted();
    void onLoadProgress(int);
    void onLoadFinished(bool);
    bool chckHostAllowed(const QString& host);
    void urlFilter(const QUrl &url);
#ifdef QTWEBENGINE
    void onDownloadRequested(QWebEngineDownloadItem *request);
    void onLinkHovered(const QString& url);
#else
    void onDownloadRequested(const QNetworkRequest& request);
    void onUnsupportedContent(QNetworkReply* reply);
    void onLinkHovered(const QString& link, const QString& title, const QString& textContent);
#endif
    void onViewSource(const QUrl &url);
    void onOpenLinkInExternalBrowser(const QUrl& url);
    void onOpenLinkInNewWindow(const QUrl&);
    void onUpdateBrowserActions();

private:
    QPointer<WebView> view;
    bool isLoading;
    UrlWidget *urlWgt;
#ifdef QTWEBENGINE
    WebEngineUrlRequestInterceptor *interceptLinks;
#else
    float textSizeMultiplier;
#endif
};

// the URL ardressbar lineedit
class UrlWidget : public QLineEdit
{
    Q_OBJECT
    BrowserView *m_view;
public:
    explicit UrlWidget(BrowserView *view);
    ~UrlWidget();
    void display();
protected:
    void keyPressEvent(QKeyEvent *keyEvt);
};

} // namespace WebGui

#endif // WEBGUI_BROWSERVIEW_H
