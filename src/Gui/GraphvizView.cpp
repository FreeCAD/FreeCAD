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
# include <QFile>
# include <QPrinter>
# include <QPrintDialog>
# include <QPrintPreviewDialog>
# include <QProcess>
# include <QSvgRenderer>
#endif
# include <QGraphicsScene>
# include <QGraphicsView>
#include "FileDialog.h"


#include "GraphvizView.h"
#include <App/Application.h>

using namespace Gui;


GraphvizView::GraphvizView(const QPixmap& p, QWidget* parent)
  : MDIView(0, parent)
{
    scene = new QGraphicsScene();
    scene->addPixmap(p);
    view = new QGraphicsView(scene, this);
    view->show();
    setCentralWidget(view);
}

GraphvizView::~GraphvizView()
{
    delete scene;
    delete view;
}

void GraphvizView::setDependencyGraph(const std::string& s)
{
    graphCode = s;
}

QByteArray GraphvizView::exportGraph(const QString& filter)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Paths");
    QProcess proc;
    QStringList args;
    if (filter.indexOf(QLatin1String("png")) > 0)
        args << QLatin1String("-Tpng");
    else if (filter.indexOf(QLatin1String("svg")) > 0)
        args << QLatin1String("-Tsvg");
    else if (filter.indexOf(QLatin1String("pdf")) > 0)
        args << QLatin1String("-Tpdf");
    else
        return QByteArray();

#ifdef FC_OS_LINUX
    QString path = QString::fromUtf8(hGrp->GetASCII("Graphviz", "/usr/bin").c_str());
#else
    QString path = QString::fromUtf8(hGrp->GetASCII("Graphviz").c_str());
#endif

#ifdef FC_OS_WIN32
    QString exe = QString::fromAscii("\"%1/dot\"").arg(path);
#else
    QString exe = QString::fromAscii("%1/dot").arg(path);
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
        QStringList filter;
        filter << tr("PNG format (*.png)");
        filter << tr("SVG format (*.svg)");
        filter << tr("PDF format (*.pdf)");

        QString selectedFilter;
        QString fn = Gui::FileDialog::getSaveFileName(this, tr("Export graph"), QString(), filter.join(QLatin1String(";;")), &selectedFilter);
        if (!fn.isEmpty()) {
            QByteArray buffer = exportGraph(selectedFilter);
            if (buffer.isEmpty())
                return false;
            QFile file(fn);
            if (file.open(QFile::WriteOnly)) {
                file.write(buffer);
                file.close();
                return true;
            }
        }
    }
    else if (strcmp("Print",pMsg) == 0) {
        return true;
    }
    else if (strcmp("PrintPreview",pMsg) == 0) {
        return true;
    }
    else if (strcmp("PrintPdf",pMsg) == 0) {
        return true;
    }

    return false;
}

bool GraphvizView::onHasMsg(const char* pMsg) const
{
    if  (strcmp("Save",pMsg) == 0)
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
    //QByteArray buffer = exportGraph(QString::fromLatin1("(*.svg)"));
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
    filter << tr("PDF format (*.pdf)");

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
