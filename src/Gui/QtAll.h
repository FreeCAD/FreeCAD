/***************************************************************************
 *   Copyright (c) 2006 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef __QtAll__
#define __QtAll__

// QtCore
#include <qglobal.h>
#include <QBuffer>
#include <QDebug>
#include <QDirIterator>
#include <QElapsedTimer>
#include <qeventloop.h>
#include <qfile.h>
#include <QLibraryInfo>
#include <QMutex>
#include <qmath.h>
#include <qnamespace.h>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <qrect.h>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QRegularExpressionValidator>
#include <qrunnable.h>
#include <QSet>
#include <QSignalMapper>
#include <QString>
#include <QTemporaryFile>
#include <qtextstream.h>
#include <qthread.h>
#include <qthreadpool.h>
#include <qtimer.h>
#include <qtranslator.h>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QVariantAnimation>
#include <QWaitCondition>

// QtGui
#include <QAbstractEventDispatcher>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QActionGroup>
#include <qapplication.h>
#include <QBitmap>
#include <QButtonGroup>
#include <qcheckbox.h>
#include <qclipboard.h>
#include <qcolordialog.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <QDesktopServices>
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
#include <QGuiApplication>
#include <QHeaderView>
#include <qimage.h>
#include <QImageWriter>
#include <qinputdialog.h>
#include <QItemDelegate>
#include <QItemEditorFactory>
#include <QKeyEvent>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <QListWidget>
#include <QLocale>
#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <qmenubar.h>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QMimeData>
#include <qmovie.h>
#include <QOpenGLContext>
#include <QOpenGLDebugMessage>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPath>
#include <qpalette.h>
#include <qpixmap.h>
#include <QPdfWriter>
#include <QPixmapCache>
#include <QPlainTextEdit>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>
#include <QProgressBar>
#include <QProgressDialog>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <QScreen>
#include <QScrollArea>
#include <qscrollbar.h>
#include <QSessionManager>
#include <QSettings>
#include <QShortcut>
#include <qslider.h>
#include <qspinbox.h>
#include <qsplashscreen.h>
#include <qsplitter.h>
#include <QStatusBar>
#include <qstyle.h>
#include <QStyledItemDelegate>
#include <qstylefactory.h>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QSurfaceFormat>
#include <QSyntaxHighlighter>
#include <qtabbar.h>
#include <QTableView>
#include <qtabwidget.h>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QTextList>
#include <QTextTableCell>
#include <QToolBar>
#include <qtoolbox.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <QTreeView>
#include <QTreeWidget>
#include <qvalidator.h>
#include <QWindow>
#include <QWhatsThis>
#include <QWhatsThisClickedEvent>

// QtNetwork
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>

// QtOpenGL
#include <QGLWidget>

// QtSvg
#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QSvgWidget>

// QtWidgets
#include <QGraphicsColorizeEffect>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QStackedWidget>
#include <QWidget>

// QtXML
#include <QDomDocument>
#include <QDomElement>

#endif
