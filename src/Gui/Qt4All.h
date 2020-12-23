/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2006     *
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


#ifndef __Qt4All__
#define __Qt4All__

// Qt Toolkit
// QtCore
#include <qglobal.h>
#include <QBuffer>
#include <qeventloop.h>
#include <qfile.h>
#include <QLibraryInfo>
#include <QMutex>
#include <qnamespace.h>
#include <QPointer>
#include <QProcess>
#include <qrect.h>
#include <qregexp.h>
#include <qrunnable.h>
#include <QSet>
#include <QSignalMapper>
#include <QTemporaryFile>
#include <qtextcodec.h>
#include <qtextstream.h>
#include <qthread.h>
#include <qthreadpool.h>
#include <qtimer.h>
#include <qtranslator.h>
#include <QUrl>
#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif
#include <qvariant.h>
#include <QWaitCondition>
// QtGui
#include <QAbstractEventDispatcher>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QAbstractTextDocumentLayout>
#include <qaction.h>
#include <QActionGroup>
#include <qapplication.h>
#include <QBitmap>
#include <QButtonGroup>
#include <qcheckbox.h>
#include <qclipboard.h>
#include <qcolordialog.h>
#include <qcombobox.h>
#include <qcursor.h>
#if QT_VERSION >= 0x040200
#include <QDesktopServices>
#endif
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDrag>
#include <qdrawutil.h>
#include <qevent.h>
#include <QFileDialog>
#include <QFileIconProvider>
#include <qfontdatabase.h>
#include <qfontdialog.h>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGroupBox>
#include <QHeaderView>
#include <qimage.h>
#include <QImageWriter>
#include <qinputdialog.h>
#include <QItemDelegate>
#include <QStyledItemDelegate>
#include <QItemEditorFactory>
#include <QKeyEvent>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <QListWidget>
#include <QLocale>
#include <QMainWindow>
#include <qmenubar.h>
#include <qmessagebox.h>
#if QT_VERSION >= 0x050000
#include <QMessageLogContext>
#endif
#include <QMimeData>
#include <qmovie.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <QPixmapCache>
#include <QPlainTextEdit>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>
#include <QProgressBar>
#include <QProgressDialog>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <QScrollArea>
#include <qscrollbar.h>
#include <QSessionManager>
#include <QSettings>
#include <QShortcut>
#include <qslider.h>
#include <qspinbox.h>
#include <qsplashscreen.h>
#include <qsplitter.h>
#include <qstatusbar.h>
#include <qstyle.h>
#include <qstylefactory.h>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QSyntaxHighlighter>
#include <qtabbar.h>
#include <QTableView>
#include <qtabwidget.h>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QToolBar>
#include <qtoolbox.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <QTreeView>
#include <QTreeWidget>
#include <qvalidator.h>
#include <QWhatsThis>
#include <QWhatsThisClickedEvent>
#include <qwidget.h>
#include <qobject.h>
#include <QMdiArea>
#include <QMdiSubWindow>
// QtNetwork
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
// QtSvg
#include <QSvgRenderer>
#include <QSvgWidget>
// QtUiTools
#include <QUiLoader>
#include <QtDesigner/QFormBuilder>

// QtWebKit
#if QT_VERSION >= 0x040400
// Only needed in Web module
//#include <QWebFrame>
//#include <QWebView>
//#include <QWebSettings>
#endif

#include "qmath.h"
#include <QGraphicsView>
#include <QGraphicsScene>

#endif
