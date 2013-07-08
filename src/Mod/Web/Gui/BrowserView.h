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

# if QT_VERSION >= 0x040400
#include <QWebView>
#endif

class QWebView;
class QUrl;
class QNetworkRequest;
class QNetworkReply;

namespace WebGui {

class WebGuiExport WebView : public QWebView
{
    Q_OBJECT

public:
    WebView(QWidget *parent = 0);
    void wheelEvent(QWheelEvent *event);
};

/**
 * A special view class which sends the messages from the application to
 * the editor and embeds it in a window.
 */
class WebGuiExport BrowserView : public Gui::MDIView, public Gui::WindowParameter
{
    Q_OBJECT

public:
    BrowserView(QWidget* parent);
    ~BrowserView();

    void load(const char* URL);
    void load(const QUrl & url);
    void setHtml(const QString& HtmlCode,const QUrl & BaseUrl,const QString& TabName=QString::fromAscii("Browser"));
    void stop(void);

    void OnChange(Base::Subject<const char*> &rCaller,const char* rcReason);

    const char *getName(void) const {return "BrowserView";}
    void onUpdate(void){};

    bool onMsg(const char* pMsg,const char** ppReturn);
    bool onHasMsg(const char* pMsg) const;

    bool canClose(void);

    /** @name Standard actions of the editor */
    //@{
    //bool open   (const QString &f);
    //bool saveAs ();
    //void cut    ();
    //void copy   ();
    //void paste  ();
    //void undo   ();
    //void redo   ();
    //void run    ();
    //void print  ();
    //void printPdf();
    //@}


protected Q_SLOTS:
    void onLoadStarted();
    void onLoadProgress(int);
    void onLoadFinished(bool);
    void onLinkClicked (const QUrl& url);
    bool chckHostAllowed(const QString& host);
    void onDownloadRequested(const QNetworkRequest& request);
    void onUnsupportedContent(QNetworkReply* reply);

private:
    WebView* view;
    bool isLoading;
    float textSizeMultiplier;
};

} // namespace WebGui

#endif // WEBGUI_BROWSERVIEW_H
