/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QFile>
# include <QPrinter>
# include <QPrintDialog>
# include <QPrintPreviewDialog>
# include <QProcess>
# include <QSvgRenderer>
# include <QGraphicsSvgItem>
# include <QMessageBox>
# include <QGraphicsScene>
# include <QGraphicsView>
# include <QThread>
# include <QProcess>
# include <boost/bind.hpp>
#endif
#include "GraphicsViewZoom.h"
#include "FileDialog.h"


#include "GraphvizView.h"
#include "Application.h"
#include "MainWindow.h"
#include <App/Application.h>
#include <App/Document.h>

using namespace Gui;

namespace Gui {

/**
 * @brief The GraphvizWorker class
 *
 * Implements a QThread class that does the actual conversion from dot to
 * svg. All critical communcation is done using queued signals.
 *
 */

class GraphvizWorker : public QThread {
    Q_OBJECT
public:
    GraphvizWorker(QObject * parent = 0)
        : QThread(parent)
    {
        proc.moveToThread(this);
    }

    void setData(const QByteArray & data)
    {
        str = data;
    }

    void run() {
        // Write data to process
        proc.write(str);
        proc.closeWriteChannel();
        if (!proc.waitForFinished()) {
            Q_EMIT error();
            quit();
        }

        // Emit result; it will get queued for processing in the main thread
        Q_EMIT svgFileRead(proc.readAll());
    }

    QProcess * process() {
        return &proc;
    }

Q_SIGNALS:
    void svgFileRead(const QByteArray & data);
    void error();

private:
    QProcess proc;
    QByteArray str;
};

}

GraphvizView::GraphvizView(App::Document & _doc, QWidget* parent)
  : MDIView(0, parent)
  , doc(_doc)
  , nPending(0)
{
    // Create scene
    scene = new QGraphicsScene();

    // Create item to hold the graph
    svgItem = new QGraphicsSvgItem();
    renderer = new QSvgRenderer(this);
    svgItem->setSharedRenderer(renderer);
    scene->addItem(svgItem);

    // Create view and zoomer object
    view = new QGraphicsView(scene, this);
    zoomer = new GraphicsViewZoom(view);
    zoomer->set_modifiers(Qt::NoModifier);
    view->show();

    // Set central widget to view
    setCentralWidget(view);

    // Create worker thread
    thread = new GraphvizWorker(this);
    connect(thread, SIGNAL(finished()), this, SLOT(done()));
    connect(thread, SIGNAL(error()), this, SLOT(error()));
    connect(thread, SIGNAL(svgFileRead(const QByteArray &)), this, SLOT(svgFileRead(const QByteArray &)));

    // Connect signal from document
    recomputeConnection = _doc.signalRecomputed.connect(boost::bind(&GraphvizView::updateSvgItem, this, _1));
    undoConnection = _doc.signalUndo.connect(boost::bind(&GraphvizView::updateSvgItem, this, _1));
    redoConnection = _doc.signalRedo.connect(boost::bind(&GraphvizView::updateSvgItem, this, _1));

    updateSvgItem(_doc);
}

GraphvizView::~GraphvizView()
{
    delete scene;
    delete view;
}

void GraphvizView::updateSvgItem(const App::Document &doc)
{
    nPending++;

    // Skip if thread is working now
    if (nPending > 1)
        return;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Paths");
    QProcess * proc = thread->process();
    QStringList args;
    args << QLatin1String("-Tsvg");
#ifdef FC_OS_LINUX
    QString path = QString::fromUtf8(hGrp->GetASCII("Graphviz", "/usr/bin").c_str());
#else
    QString path = QString::fromUtf8(hGrp->GetASCII("Graphviz").c_str());
#endif
    bool pathChanged = false;
#ifdef FC_OS_WIN32
    QString exe = QString::fromLatin1("\"%1/dot\"").arg(path);
#else
    QString exe = QString::fromLatin1("%1/dot").arg(path);
#endif
    proc->setEnvironment(QProcess::systemEnvironment());
    do {
        proc->start(exe, args);
        if (!proc->waitForStarted()) {
            int ret = QMessageBox::warning(Gui::getMainWindow(),
                                           qApp->translate("Std_ExportGraphviz","Graphviz not found"),
                                           qApp->translate("Std_ExportGraphviz","Graphviz couldn't be found on your system.\n"
                                                           "Do you want to specify its installation path if it's already installed?"),
                                           QMessageBox::Yes, QMessageBox::No);
            if (ret == QMessageBox::No) {
                disconnectSignals();
                return;
            }
            path = QFileDialog::getExistingDirectory(Gui::getMainWindow(),
                                                     qApp->translate("Std_ExportGraphviz","Graphviz installation path"));
            if (path.isEmpty()) {
                disconnectSignals();
                return;
            }
            pathChanged = true;
#ifdef FC_OS_WIN32
            exe = QString::fromLatin1("\"%1/dot\"").arg(path);
#else
            exe = QString::fromLatin1("%1/dot").arg(path);
#endif
        }
        else {
            if (pathChanged)
                hGrp->SetASCII("Graphviz", (const char*)path.toUtf8());
            break;
        }
    }
    while(true);

    // Create graph in dot format
    std::stringstream stream;
    doc.exportGraphviz(stream);
    graphCode = stream.str();

    // Update worker thread, and start it
    thread->setData(QByteArray(graphCode.c_str(), graphCode.size()));
    thread->start();
}

void GraphvizView::svgFileRead(const QByteArray & data)
{
    // Update renderer with new SVG file, and give message if something went wrong
    if (renderer->load(data))
        svgItem->setSharedRenderer(renderer);
    else {
        QMessageBox::warning(getMainWindow(),
                             qApp->translate("Std_ExportGraphviz","Graphviz failed"),
                             qApp->translate("Std_ExportGraphviz","Graphviz failed to create an image file"));
        disconnectSignals();
    }
}

void GraphvizView::error()
{
    // If the worker fails for some reason, stop giving it more data later
    disconnectSignals();
}

void GraphvizView::done()
{
    nPending--;
    if (nPending > 0) {
        nPending = 0;
        updateSvgItem(doc);
        thread->start();
    }
}

void GraphvizView::disconnectSignals()
{
    recomputeConnection.disconnect();
    undoConnection.disconnect();
    redoConnection.disconnect();
}

#include <QObject>
#include <QGraphicsView>

QByteArray GraphvizView::exportGraph(const QString& format)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Paths");
    QProcess proc;
    QStringList args;
    args << QString::fromLatin1("-T%1").arg(format);

#ifdef FC_OS_LINUX
    QString path = QString::fromUtf8(hGrp->GetASCII("Graphviz", "/usr/bin").c_str());
#else
    QString path = QString::fromUtf8(hGrp->GetASCII("Graphviz").c_str());
#endif

#ifdef FC_OS_WIN32
    QString exe = QString::fromLatin1("\"%1/dot\"").arg(path);
#else
    QString exe = QString::fromLatin1("%1/dot").arg(path);
#endif
    proc.setEnvironment(QProcess::systemEnvironment());
    proc.start(exe, args);
    if (!proc.waitForStarted()) {
        return QByteArray();
    }

    proc.write(graphCode.c_str(), graphCode.size());
    proc.closeWriteChannel();
    if (!proc.waitForFinished())
        return QByteArray();

    return proc.readAll();
}

bool GraphvizView::onMsg(const char* pMsg,const char** ppReturn)
{
    if (strcmp("Save",pMsg) == 0 || strcmp("SaveAs",pMsg) == 0) {
        QList< QPair<QString, QString> > formatMap;
        formatMap << qMakePair(QString::fromLatin1("%1 (*.png)").arg(tr("PNG format")), QString::fromLatin1("png"));
        formatMap << qMakePair(QString::fromLatin1("%1 (*.bmp)").arg(tr("Bitmap format")), QString::fromLatin1("bmp"));
        formatMap << qMakePair(QString::fromLatin1("%1 (*.gif)").arg(tr("GIF format")), QString::fromLatin1("gif"));
        formatMap << qMakePair(QString::fromLatin1("%1 (*.jpg)").arg(tr("JPG format")), QString::fromLatin1("jpg"));
        formatMap << qMakePair(QString::fromLatin1("%1 (*.svg)").arg(tr("SVG format")), QString::fromLatin1("svg"));
        formatMap << qMakePair(QString::fromLatin1("%1 (*.pdf)").arg(tr("PDF format")), QString::fromLatin1("pdf"));
      //formatMap << qMakePair(tr("VRML format (*.vrml)"), QString::fromLatin1("vrml"));

        QStringList filter;
        for (QList< QPair<QString, QString> >::iterator it = formatMap.begin(); it != formatMap.end(); ++it)
            filter << it->first;

        QString selectedFilter;
        QString fn = Gui::FileDialog::getSaveFileName(this, tr("Export graph"), QString(), filter.join(QLatin1String(";;")), &selectedFilter);
        if (!fn.isEmpty()) {
            QString format;
            for (QList< QPair<QString, QString> >::iterator it = formatMap.begin(); it != formatMap.end(); ++it) {
                if (selectedFilter == it->first) {
                    format = it->second;
                    break;
                }
            }
            QByteArray buffer = exportGraph(format);
            if (buffer.isEmpty())
                return true;
            QFile file(fn);
            if (file.open(QFile::WriteOnly)) {
                file.write(buffer);
                file.close();
            }
        }
        return true;
    }
    else if (strcmp("Print",pMsg) == 0) {
        print();
        return true;
    }
    else if (strcmp("PrintPreview",pMsg) == 0) {
        printPreview();
        return true;
    }
    else if (strcmp("PrintPdf",pMsg) == 0) {
        printPdf();
        return true;
    }

    return false;
}

bool GraphvizView::onHasMsg(const char* pMsg) const
{
    if (strcmp("Save",pMsg) == 0)
        return true;
    else if (strcmp("SaveAs",pMsg) == 0)
        return true;
    else if (strcmp("Print",pMsg) == 0)
        return true; 
    else if (strcmp("PrintPreview",pMsg) == 0)
        return true; 
    else if (strcmp("PrintPdf",pMsg) == 0)
        return true; 
    return false;
}

void GraphvizView::print(QPrinter* printer)
{
    QPainter p(printer);
    QRect rect = printer->pageRect();
    view->scene()->render(&p, rect);
    //QByteArray buffer = exportGraph(QString::fromLatin1("svg"));
    //QSvgRenderer svg(buffer);
    //svg.render(&p, rect);
    p.end();
}

void GraphvizView::print()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setOrientation(QPrinter::Landscape);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        print(&printer);
    }
}

void GraphvizView::printPdf()
{
    QStringList filter;
    filter << QString::fromLatin1("%1 (*.pdf)").arg(tr("PDF format"));

    QString selectedFilter;
    QString fn = Gui::FileDialog::getSaveFileName(this, tr("Export graph"), QString(), filter.join(QLatin1String(";;")), &selectedFilter);
    if (!fn.isEmpty()) {
        QByteArray buffer = exportGraph(selectedFilter);
        if (buffer.isEmpty())
            return;
        QFile file(fn);
        if (file.open(QFile::WriteOnly)) {
            file.write(buffer);
            file.close();
        }
    }
}

void GraphvizView::printPreview()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setOrientation(QPrinter::Landscape);

    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, SIGNAL(paintRequested (QPrinter *)),
            this, SLOT(print(QPrinter *)));
    dlg.exec();
}

#include "moc_GraphvizView.cpp"
#include "moc_GraphvizView-internal.cpp"
