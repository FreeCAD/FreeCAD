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


#ifndef GUI_DOCKWND_HELP_VIEW_H
#define GUI_DOCKWND_HELP_VIEW_H

#include <QTextBrowser>
#include "DockWindow.h"
#include "Window.h"

class QUrl;
class QLabel;
class QHttpResponseHeader;

namespace Gui {
namespace DockWnd {

/**
 * The TextBrowser class is an extension of Qt's QTextBrowser providing
 * a context menu and the possibility to render files via drag and drop.
 * @author Werner Mayer
 */
class TextBrowserPrivate;
class TextBrowser : public QTextBrowser
{
  Q_OBJECT

public:
  TextBrowser(QWidget * parent=0);
  virtual ~TextBrowser();

  void setSource (const QUrl& url);
  QVariant loadResource (int type, const QUrl& name);

  void backward();
  void forward();

Q_SIGNALS:
  /// start an external browser to display complex web sites
  void startExternalBrowser( const QString& );
  void stateChanged(const QString&);

protected:
  void dropEvent       (QDropEvent  * e);
  void dragEnterEvent  (QDragEnterEvent * e);
  void dragMoveEvent   (QDragMoveEvent  * e );
  void contextMenuEvent(QContextMenuEvent * );
  void timerEvent      (QTimerEvent * e );

private Q_SLOTS:
  void setBackwardAvailable( bool b);
  void setForwardAvailable( bool b);
  void done( bool );
  void onStateChanged ( int state );
  void onResponseHeaderReceived(const QHttpResponseHeader &);
  void onHighlighted(const QString&);

private:
  QString findUrl(const QUrl &name) const;
  QVariant loadFileResource(int type, const QUrl& name);
  QVariant loadHttpResource(int type, const QUrl& name);

private:
  TextBrowserPrivate* d;
};

/**
 * The help viewer class embeds a textbrowser to render help files.
 * @author Wenrer Mayer
 */
class HelpViewViewPrivate;
class GuiExport HelpView : public QWidget
{
  Q_OBJECT

public:
  HelpView( const QString& home_, QWidget* parent = 0 );
  ~HelpView();

  void setFileSource( const QString& );
  /**
   * Filters events if this object has been installed as an event filter for the watched object.
   */
  bool eventFilter ( QObject* o, QEvent* e );

Q_SIGNALS:
  void setSource( const QUrl& );

private Q_SLOTS:
  void openHelpFile();
  void startExternalBrowser( const QString& );
  void onStateChanged(const QString& state);

private:
  QLabel* label;
};

} // namespace DockWnd
} // namespace Gui

#endif // GUI_DOCKWND_HELP_VIEW_H
