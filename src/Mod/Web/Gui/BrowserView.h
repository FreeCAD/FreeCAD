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

#if QT_VERSION >= 0x050700 and defined(QTWEBENGINE)
#include <QWebEngineView>
namespace WebGui {
class WebEngineUrlRequestInterceptor;
};
#elif QT_VERSION >= 0x040400 and defined(QTWEBKIT)
#include <QWebView>
#endif

class QUrl;
class QNetworkRequest;
class QNetworkReply;

namespace WebGui {

#ifdef QTWEBENGINE
class WebGuiExport WebView : public QWebEngineView
#else
class WebGuiExport WebView : public QWebView
#endif
{
    Q_OBJECT

public:
    WebView(QWidget *parent = 0);
#ifdef QTWEBENGINE
    // reimplement setTextSizeMultiplier
    void setTextSizeMultiplier(qreal factor);
#endif

protected:
#ifdef QTWEBKIT
    void mousePressEvent(QMouseEvent *event);
#endif
    void wheelEvent(QWheelEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

private Q_SLOTS:
    void triggerContextMenuAction(int);

Q_SIGNALS:
    void openLinkInExternalBrowser(const QUrl&);
    void openLinkInNewWindow(const QUrl&);
#ifdef QTWEBENGINE
    void viewSource(const QUrl&);
#endif
};

/**
 * A special view class which sends the messages from the application to
 * the editor and embeds it in a window.
 */
class WebGuiExport BrowserView : public Gui::MDIView,
                                 public Gui::WindowParameter
{
    Q_OBJECT

public:
    BrowserView(QWidget* parent);
    ~BrowserView();

    void load(const char* URL);
    void load(const QUrl & url);
    void setHtml(const QString& HtmlCode,const QUrl & BaseUrl);
    void stop(void);

    void OnChange(Base::Subject<const char*> &rCaller,const char* rcReason);

    const char *getName(void) const {return "BrowserView";}
    virtual PyObject *getPyObject(void);

    bool onMsg(const char* pMsg,const char** ppReturn);
    bool onHasMsg(const char* pMsg) const;

    bool canClose(void);

protected Q_SLOTS:
    void onLoadStarted();
    void onLoadProgress(int);
    void onLoadFinished(bool);
    bool chckHostAllowed(const QString& host);
#ifdef QTWEBENGINE
    void onDownloadRequested(QWebEngineDownloadItem *request);
    void setWindowIcon(const QIcon &icon);
    void urlFilter(const QUrl &url);
    void onViewSource(const QUrl &url);
#else
    void onDownloadRequested(const QNetworkRequest& request);
    void onUnsupportedContent(QNetworkReply* reply);
    void onLinkClicked (const QUrl& url);
#endif
    void onOpenLinkInExternalBrowser(const QUrl& url);
    void onOpenLinkInNewWindow(const QUrl&);

private:
    WebView* view;
    bool isLoading;
#ifdef QTWEBENGINE
    WebEngineUrlRequestInterceptor *interceptLinks;
#else
    float textSizeMultiplier;
#endif
};

} // namespace WebGui

#endif // WEBGUI_BROWSERVIEW_H
