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


#include <QApplication>
#include <QFile>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>
#include <QGraphicsView>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QProcess>
#include <QSvgRenderer>
#include <QScrollBar>
#include <QThread>

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>

#include "GraphvizView.h"
#include "GraphicsViewZoom.h"
#include "FileDialog.h"
#include "MainWindow.h"

#define USER_PREF "User parameter:BaseApp/Preferences/"
using namespace Gui;
namespace sp = std::placeholders;
enum class GraphvizView::ProcessType : uint8_t
{
    None,
    View,
    Export,
};
enum class GraphvizView::PathType : uint8_t
{
    Stored,     // Read from properties
    AppPath,    // The dir containing the running executable
    Default,    // Read via PATH environmnet variable
    Selection,  // Select path with file dialog
};

namespace Gui
{
// Simple wrapper around QGraphicsView to make panning possible
class GraphvizGraphicsView final: public QGraphicsView
{
public:
    GraphvizGraphicsView(QGraphicsScene* scene, QWidget* parent);
    ~GraphvizGraphicsView() override = default;

    GraphvizGraphicsView(const GraphvizGraphicsView&) = delete;
    GraphvizGraphicsView(GraphvizGraphicsView&&) = delete;
    GraphvizGraphicsView& operator=(const GraphvizGraphicsView&) = delete;
    GraphvizGraphicsView& operator=(GraphvizGraphicsView&&) = delete;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool isPanning {false};
    QPoint panStart;
};

GraphvizGraphicsView::GraphvizGraphicsView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
{}

void GraphvizGraphicsView::mousePressEvent(QMouseEvent* e)
{
    if (e && e->button() == Qt::LeftButton) {
        isPanning = true;
        panStart = e->pos();
        e->accept();
        QApplication::setOverrideCursor(Qt::ClosedHandCursor);
    }

    QGraphicsView::mousePressEvent(e);

    return;
}

void GraphvizGraphicsView::mouseMoveEvent(QMouseEvent* e)
{
    if (!e) {
        return;
    }

    if (isPanning) {
        auto* horizontalScrollbar = horizontalScrollBar();
        auto* verticalScrollbar = verticalScrollBar();
        if (!horizontalScrollbar || !verticalScrollbar) {
            return;
        }

        auto direction = e->pos() - panStart;
        horizontalScrollbar->setValue(horizontalScrollbar->value() - direction.x());
        verticalScrollbar->setValue(verticalScrollbar->value() - direction.y());

        panStart = e->pos();
        e->accept();
    }

    QGraphicsView::mouseMoveEvent(e);

    return;
}

void GraphvizGraphicsView::mouseReleaseEvent(QMouseEvent* e)
{
    if (e && e->button() & Qt::LeftButton) {
        isPanning = false;
        QApplication::restoreOverrideCursor();
        e->accept();
    }

    QGraphicsView::mouseReleaseEvent(e);

    return;
}

}  // namespace Gui

/* TRANSLATOR Gui::GraphvizView */

TYPESYSTEM_SOURCE_ABSTRACT(Gui::GraphvizView, Gui::MDIView)  // NOLINT

GraphvizView::GraphvizView(App::Document& _doc, QWidget* parent)
    : MDIView(nullptr, parent)
    , doc(_doc)
{
    // Create scene
    scene = new QGraphicsScene();

    // Create item to hold the graph
    svgItem = new QGraphicsSvgItem();
    renderer = new QSvgRenderer(this);
    svgItem->setSharedRenderer(renderer);
    scene->addItem(svgItem);

    // Create view and zoomer object
    view = new GraphvizGraphicsView(scene, this);
    zoomer = new GraphicsViewZoom(view);
    zoomer->set_modifiers(Qt::NoModifier);
    view->show();

    auto hGrp = App::GetApplication().GetParameterGroupByPath(USER_PREF "View");
    bool on = hGrp->GetBool("InvertZoom", true);
    zoomer->set_zoom_inverted(on);
    setCentralWidget(view);

    dotProc = new QProcess(this);
    unflattenProc = new QProcess(this);
    connect(dotProc, &QProcess::started, this, &GraphvizView::convertDotStarted);
    connect(dotProc, &QProcess::errorOccurred, this, &GraphvizView::convertDotError);
    connect(dotProc, &QProcess::finished, this, &GraphvizView::convertDotFinished);
    connect(unflattenProc, &QProcess::started, this, &GraphvizView::convertUnflattenStarted);
    connect(unflattenProc, &QProcess::errorOccurred, this, &GraphvizView::convertUnflattenError);
    connect(unflattenProc, &QProcess::finished, this, &GraphvizView::convertUnflattenFinished);

    // NOLINTBEGIN
    //  Connect signal from document
    recomputeConnection = _doc.signalRecomputed.connect(std::bind(&GraphvizView::updateSvgItem, this));
    undoConnection = _doc.signalUndo.connect(std::bind(&GraphvizView::updateSvgItem, this));
    redoConnection = _doc.signalRedo.connect(std::bind(&GraphvizView::updateSvgItem, this));
    // NOLINTEND

    updateSvgItem();
}

GraphvizView::~GraphvizView()
{
    unflattenProc->disconnect();
    dotProc->disconnect();
    delete scene;
    delete view;
}

QString GraphvizView::getDirPath()
{
    if (pathType == PathType::Stored) {
        auto hGrp = App::GetApplication().GetParameterGroupByPath(USER_PREF "Paths");
        if (auto path = QString::fromUtf8(hGrp->GetASCII("Graphviz").c_str()); !path.isEmpty()) {
            return path;
        }
        pathType = PathType::AppPath;
    }
    if (pathType == PathType::AppPath) {
        return QCoreApplication::applicationDirPath();
    }
    if (pathType == PathType::Default) {
        return "";  // Empty, non-null, string
    }
    auto msg = QStringLiteral(
                   "<html><head/><body>%1 "
                   "<a href=\"https://www.freecad.org/wiki/Std_DependencyGraph\">%2</a>"
                   "<p>%3</p></body></html>"
    )
                   .arg(
                       tr("Graphviz couldn't be found on your system."),
                       tr("Read more about it here."),
                       tr("Do you want to specify its installation path if it's already installed?")
                   );
    using MB = QMessageBox;
    if (MB::warning(Gui::getMainWindow(), tr("Graphviz not found"), msg, MB::Yes, MB::No) == MB::Yes) {
        return QFileDialog::getExistingDirectory(Gui::getMainWindow(), tr("Graphviz installation path"));
    }
    return QString();  // Null string
}

QString joinPath(const QString& path, const QString& file)
{
    return path.isEmpty() ? file : QDir(path).filePath(file);
}

void GraphvizView::convert(ProcessType type, const QString& exportType, const QString& exportPath)
{
    using enum ProcessType;
    pendingMask |= 1 << (int)type;
    pendingExportType = exportType;
    pendingExportPath = exportPath;
    if (running != None) {
        return;
    }
    for (int t = (int)Export; running == None && t >= (int)View; t--) {
        if ((pendingMask & (1 << t)) != 0) {
            pendingMask ^= (1 << t);
            running = (ProcessType)t;
            if (running == ProcessType::Export) {
                runningExportType = pendingExportType;
                runningExportPath = pendingExportPath;
            }
        }
    }
    if (running == None) {
        return;
    }

    // Create graph in dot format
    std::stringstream stream;
    doc.exportGraphviz(stream);
    graphCode = QByteArray::fromStdString(stream.str());

    convertDotStart();
}
void GraphvizView::convertDotStart()
{
    QStringList dotArgs;
    if (running == ProcessType::Export) {
        dotArgs << QStringLiteral("-T%1").arg(runningExportType);
    }
    else {
        // TODO: Make -Granksep flag value variable depending on number of edges,
        // the downside is that the value affects all subgraphs
        dotArgs << QLatin1String("-Granksep=2") << QLatin1String("-Tsvg");
        if (!App::GetApplication().isFineGrainedRecomputeEnabled()) {
            dotArgs << QLatin1String("-Gsplines=ortho") << QLatin1String("-Goutputorder=edgesfirst");
        }
    }
    if (path.isNull()) {
        path = getDirPath();
        if (path.isNull()) {
            disconnectSignals();
            return;
        }
    }
    dotProc->setEnvironment(QProcess::systemEnvironment());
    dotProc->start(joinPath(path, QStringLiteral("dot")), dotArgs);
}
void GraphvizView::convertDotStarted()
{
    if (pathType == PathType::Selection) {
        auto hGrp = App::GetApplication().GetParameterGroupByPath(USER_PREF "Paths");
        hGrp->SetASCII("Graphviz", (const char*)path.toUtf8());
    }
    auto depGrp = App::GetApplication().GetParameterGroupByPath(USER_PREF "DependencyGraph");
    if (depGrp->GetBool("Unflatten", true)) {
        convertUnflattenStart();
    }
    else {
        convertDotWrite();
    }
}
void GraphvizView::convertDotError()
{
    if (dotProc->error() == QProcess::FailedToStart) {
        path = QString();
        pathType = (PathType)((int)pathType + 1);
        convertDotStart();
        return;
    }
    disconnectSignals();
}
void GraphvizView::convertUnflattenStart()
{
    unflattenProc->setEnvironment(QProcess::systemEnvironment());
    unflattenProc->start(joinPath(path, QStringLiteral("unflatten")), {QLatin1String("-c2 -l2")});
}
void GraphvizView::convertUnflattenStarted()
{
    unflattenProc->write(graphCode);
    unflattenProc->closeWriteChannel();
}
void GraphvizView::convertUnflattenError()
{
    // no error handling: unflatten is optional
    if (dotProc->error() == QProcess::FailedToStart) {
        convertDotWrite();
    }
}
void GraphvizView::convertUnflattenFinished()
{
    auto unflattened = unflattenProc->readAll();
    if (!unflattened.isEmpty()) {
        graphCode = unflattened;
    }
    convertDotWrite();
}
void GraphvizView::convertDotWrite()
{
    dotProc->write(graphCode);
    dotProc->closeWriteChannel();
}
void GraphvizView::convertDotFinished()
{
    auto data = dotProc->readAll();
    if (running == ProcessType::View) {
        if (!data.isEmpty() && renderer->load(data)) {
            svgItem->setSharedRenderer(renderer);
            running = ProcessType::None;
        }
    }
    else if (running == ProcessType::Export) {
        if (!data.isEmpty()) {
            if (QFile file(runningExportPath); file.open(QFile::WriteOnly)) {
                file.write(data);
                file.close();
                running = ProcessType::None;
            }
        }
    }
    if (running != ProcessType::None) {
        auto win = getMainWindow();
        QMessageBox::warning(win, tr("Graphviz failed"), tr("Graphviz failed to create an image file"));
        disconnectSignals();
        running = ProcessType::None;
    }
    convert(ProcessType::None);
}

void GraphvizView::updateSvgItem()
{
    convert(ProcessType::View);
}

void GraphvizView::disconnectSignals()
{
    recomputeConnection.disconnect();
    undoConnection.disconnect();
    redoConnection.disconnect();
}

bool GraphvizView::onMsg(const char* pMsg)
{
    if (strcmp("Save", pMsg) == 0 || strcmp("SaveAs", pMsg) == 0) {
        QList<QPair<FileDialog::Filter, QString>> formatMap {
            {{QStringLiteral("Graphviz"), {"*.gv"}}, QStringLiteral("gv")},
            {{QStringLiteral("PNG"), {"*.png"}}, QStringLiteral("png")},
            {{tr("Bitmap"), {"*.bmp"}}, QStringLiteral("bmp")},
            {{QStringLiteral("GIF"), {"*.gif"}}, QStringLiteral("gif")},
            {{QStringLiteral("JPG"), {"*.jpg"}}, QStringLiteral("jpg")},
            {{QStringLiteral("SVG"), {"*.svg"}}, QStringLiteral("svg")},
            {{QStringLiteral("PDF"), {"*.pdf"}}, QStringLiteral("pdf")},
        };

        FileDialog::FilterList filterList;
        for (const auto& it : std::as_const(formatMap)) {
            filterList.append(it.first);
        }

        qsizetype selectedIdx = -1;
        auto fn = FileDialog::getSaveFileName(this, tr("Export Graph"), "", filterList, &selectedIdx);
        if (!fn.isEmpty()) {
            if (formatMap[selectedIdx].second == QStringLiteral("gv")) {
                std::stringstream str;
                doc.exportGraphviz(str);
                auto buffer = QByteArray::fromStdString(str.str());
                if (!buffer.isEmpty()) {
                    if (QFile file(fn); file.open(QFile::WriteOnly)) {
                        file.write(buffer);
                        file.close();
                    }
                }
            }
            else {
                convert(ProcessType::Export, formatMap[selectedIdx].second, fn);
            }
        }
        return true;
    }
    else if (strcmp("Print", pMsg) == 0) {
        print();
        return true;
    }
    else if (strcmp("PrintPreview", pMsg) == 0) {
        printPreview();
        return true;
    }
    else if (strcmp("PrintPdf", pMsg) == 0) {
        printPdf();
        return true;
    }

    return false;
}

bool GraphvizView::onHasMsg(const char* pMsg) const
{
    std::array msgs {"Save", "SaveAs", "Print", "PrintPreview", "PrintPdf", "AllowsOverlayOnHover"};
    return std::ranges::any_of(msgs, [&](auto s) { return strcmp(s, pMsg) == 0; });
}

void GraphvizView::print(QPrinter* printer)
{
    QPainter p(printer);
    QRect rect = printer->pageLayout().paintRectPixels(printer->resolution());
    view->scene()->render(&p, rect);
    p.end();
}

void GraphvizView::print()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setPageOrientation(QPageLayout::Landscape);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        print(&printer);
    }
}

void GraphvizView::printPdf()
{
    FileDialog::FilterList filterList {{QStringLiteral("PDF"), {"*.pdf"}}};
    auto fn = FileDialog::getSaveFileName(this, tr("Export graph"), "", filterList);
    if (!fn.isEmpty()) {
        convert(ProcessType::Export, "pdf", fn);
    }
}

void GraphvizView::printPreview()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setPageOrientation(QPageLayout::Landscape);

    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, &QPrintPreviewDialog::paintRequested, this, qOverload<QPrinter*>(&GraphvizView::print));
    dlg.exec();
}
