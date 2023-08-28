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
#include <QAbstractEventDispatcher>
#include <QAbstractItemModel>
#include <QBuffer>
#include <QDebug>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QLibraryInfo>
#include <QLocale>
#include <QMutex>
#include <qmath.h>
#include <QMessageLogContext>
#include <QMimeData>
#include <qnamespace.h>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QRect>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QRegularExpressionValidator>
#include <QRunnable>
#include <QSet>
#include <QSettings>
#include <QSignalMapper>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QVariantAnimation>
#include <QWaitCondition>

// QtGui
#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QActionGroup>
#include <QBitmap>
#include <QClipboard>
#include <QCursor>
#include <QDesktopServices>
#include <QDrag>
#include <qdrawutil.h>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QImageWriter>
#include <QKeyEvent>
#include <QMovie>
#include <QOpenGLContext>
#include <QOpenGLDebugMessage>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPdfWriter>
#include <QPixmap>
#include <QPixmapCache>
#include <QScreen>
#include <QSessionManager>
#include <QShortcut>
#include <QStandardItemModel>
#include <QSurfaceFormat>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextList>
#include <QTextTableCell>
#include <QValidator>
#include <QWindow>
#include <QWhatsThisClickedEvent>

// QtNetwork
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>

// QtPrintSupport
#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>

// QtSvg
#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QSvgWidget>

// QtWidgets
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFontDialog>
#include <QGraphicsColorizeEffect>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemDelegate>
#include <QItemEditorFactory>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QProgressDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QSlider>
#include <QSpinBox>
#include <QSplashScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleFactory>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QTabBar>
#include <QTableView>
#include <QTabWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTextEdit>
#include <QToolBar>
#include <QToolBox>
#include <QToolButton>
#include <QToolTip>
#include <QTreeView>
#include <QTreeWidget>
#include <QWhatsThis>
#include <QWidget>
#include <QWidgetAction>

// QtXML
#include <QDomDocument>
#include <QDomElement>

#endif
