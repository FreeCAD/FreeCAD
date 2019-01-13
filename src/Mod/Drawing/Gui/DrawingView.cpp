/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is Drawing of the FreeCAD CAx development system.           *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A DrawingICULAR PURPOSE.  See the      *
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
# include <QAction>
# include <QApplication>
# include <QBuffer>
# include <QContextMenuEvent>
# include <QFileInfo>
# include <QFileDialog>
# include <QGLWidget>
# include <QGraphicsRectItem>
# include <QGraphicsSvgItem>
# include <QGridLayout>
# include <QGroupBox>
# include <QListWidget>
# include <QMenu>
# include <QMessageBox>
# include <QMouseEvent>
# include <QPainter>
# include <QPaintEvent>
# include <QPrinter>
# include <QPrintDialog>
# include <QPrintPreviewDialog>
# include <QPrintPreviewWidget>
# include <QScrollArea>
# include <QSlider>
# include <QStatusBar>
# include <QSvgRenderer>
# include <QSvgWidget>
# include <QWheelEvent>
# include <cmath>
#endif

#include "DrawingView.h"
#include <Base/Stream.h>
#include <Base/gzstream.h>
#include <Base/PyObjectBase.h>
#include <App/Document.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/FileDialog.h>
#include <Gui/WaitCursor.h>

using namespace DrawingGui;

SvgView::SvgView(QWidget *parent)
    : QGraphicsView(parent)
    , m_renderer(Native)
    , m_svgItem(0)
    , m_backgroundItem(0)
    , m_outlineItem(0)
    , m_invertZoom(false)
{
    setScene(new QGraphicsScene(this));
    setTransformationAnchor(AnchorUnderMouse);
    setDragMode(ScrollHandDrag);

    // Prepare background check-board pattern
    QPixmap tilePixmap(64, 64);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, 32, 32, color);
    tilePainter.fillRect(32, 32, 32, 32, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);
}

void SvgView::drawBackground(QPainter *p, const QRectF &)
{
    p->save();
    p->resetTransform();
    p->drawTiledPixmap(viewport()->rect(), backgroundBrush().texture());
    p->restore();
}

void SvgView::openFile(const QFile &file)
{
    if (!file.exists())
        return;

    QGraphicsScene *s = scene();

    bool drawBackground = (m_backgroundItem ? m_backgroundItem->isVisible() : true);
    bool drawOutline = (m_outlineItem ? m_outlineItem->isVisible() : false);

    s->clear();
    resetTransform();

    m_svgItem = new QGraphicsSvgItem(file.fileName());
    m_svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
    m_svgItem->setCacheMode(QGraphicsItem::NoCache);
    m_svgItem->setZValue(0);

    m_backgroundItem = new QGraphicsRectItem(m_svgItem->boundingRect());
    m_backgroundItem->setBrush(Qt::white);
    m_backgroundItem->setPen(Qt::NoPen);
    m_backgroundItem->setVisible(drawBackground);
    m_backgroundItem->setZValue(-1);

    m_outlineItem = new QGraphicsRectItem(m_svgItem->boundingRect());
    QPen outline(Qt::black, 2, Qt::DashLine);
    outline.setCosmetic(true);
    m_outlineItem->setPen(outline);
    m_outlineItem->setBrush(Qt::NoBrush);
    m_outlineItem->setVisible(drawOutline);
    m_outlineItem->setZValue(1);

    s->addItem(m_backgroundItem);
    s->addItem(m_svgItem);
    s->addItem(m_outlineItem);

    // use the actual bounding box of the SVG template to avoid any scaling effect
    // when printing the drawing (#0000932)
    s->setSceneRect(m_outlineItem->boundingRect());
}

void SvgView::setRenderer(RendererType type)
{
    m_renderer = type;

    if (m_renderer == OpenGL) {
#ifndef QT_NO_OPENGL
        setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
#endif
    } else {
        setViewport(new QWidget);
    }
}

void SvgView::setHighQualityAntialiasing(bool highQualityAntialiasing)
{
#ifndef QT_NO_OPENGL
    setRenderHint(QPainter::HighQualityAntialiasing, highQualityAntialiasing);
#else
    Q_UNUSED(highQualityAntialiasing);
#endif
}

void SvgView::setViewBackground(bool enable)
{
    if (!m_backgroundItem)
        return;

    m_backgroundItem->setVisible(enable);
}

void SvgView::setViewOutline(bool enable)
{
    if (!m_outlineItem)
        return;

    m_outlineItem->setVisible(enable);
}

void SvgView::paintEvent(QPaintEvent *event)
{
    if (m_renderer == Image) {
        if (m_image.size() != viewport()->size()) {
            m_image = QImage(viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        }

        QPainter imagePainter(&m_image);
        QGraphicsView::render(&imagePainter);
        imagePainter.end();

        QPainter p(viewport());
        p.drawImage(0, 0, m_image);

    } else {
        QGraphicsView::paintEvent(event);
    }
}

void SvgView::wheelEvent(QWheelEvent *event)
{
    int delta = -event->delta();
    if (m_invertZoom)
        delta = -delta;
    qreal factor = std::pow(1.2, delta / 240.0);
    scale(factor, factor);
    event->accept();
}

// ----------------------------------------------------------------------------

/* TRANSLATOR DrawingGui::DrawingView */

DrawingView::DrawingView(Gui::Document* doc, QWidget* parent)
  : Gui::MDIView(doc, parent), m_view(new SvgView)
{
    m_backgroundAction = new QAction(tr("&Background"), this);
    m_backgroundAction->setEnabled(false);
    m_backgroundAction->setCheckable(true);
    m_backgroundAction->setChecked(true);
    connect(m_backgroundAction, SIGNAL(toggled(bool)), m_view, SLOT(setViewBackground(bool)));

    m_outlineAction = new QAction(tr("&Outline"), this);
    m_outlineAction->setEnabled(false);
    m_outlineAction->setCheckable(true);
    m_outlineAction->setChecked(false);
    connect(m_outlineAction, SIGNAL(toggled(bool)), m_view, SLOT(setViewOutline(bool)));

    m_nativeAction = new QAction(tr("&Native"), this);
    m_nativeAction->setCheckable(true);
    m_nativeAction->setChecked(false);
#ifndef QT_NO_OPENGL
    m_glAction = new QAction(tr("&OpenGL"), this);
    m_glAction->setCheckable(true);
#endif
    m_imageAction = new QAction(tr("&Image"), this);
    m_imageAction->setCheckable(true);

#ifndef QT_NO_OPENGL
    m_highQualityAntialiasingAction = new QAction(tr("&High Quality Antialiasing"), this);
    m_highQualityAntialiasingAction->setEnabled(false);
    m_highQualityAntialiasingAction->setCheckable(true);
    m_highQualityAntialiasingAction->setChecked(false);
    connect(m_highQualityAntialiasingAction, SIGNAL(toggled(bool)),
            m_view, SLOT(setHighQualityAntialiasing(bool)));
#endif

    QActionGroup *rendererGroup = new QActionGroup(this);
    rendererGroup->addAction(m_nativeAction);
#ifndef QT_NO_OPENGL
    rendererGroup->addAction(m_glAction);
#endif
    rendererGroup->addAction(m_imageAction);
    connect(rendererGroup, SIGNAL(triggered(QAction *)),
            this, SLOT(setRenderer(QAction *)));

    setCentralWidget(m_view);
    //setWindowTitle(tr("SVG Viewer"));

    m_orientation = QPrinter::Landscape;
    m_pageSize = QPrinter::A4;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/View");
    bool on = hGrp->GetBool("InvertZoom", false);
    m_view->setZoomInverted(on);
}

DrawingView::~DrawingView()
{
}

void DrawingView::load (const QString & fileName)
{
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.exists()) {
            QMessageBox::critical(this, tr("Open SVG File"),
                           tr("Could not open file '%1'.").arg(fileName));

            m_outlineAction->setEnabled(false);
            m_backgroundAction->setEnabled(false);
            return;
        }

        m_view->openFile(file);

        if (!fileName.startsWith(QLatin1String(":/"))) {
            m_currentPath = fileName;
            //setWindowTitle(tr("%1 - SVG Viewer").arg(m_currentPath));
        }

        m_outlineAction->setEnabled(true);
        m_backgroundAction->setEnabled(true);

        findPrinterSettings(QFileInfo(fileName).baseName());
    }
}

void DrawingView::findPrinterSettings(const QString& fileName)
{
    if (fileName.indexOf(QLatin1String("Portrait"), Qt::CaseInsensitive) >= 0) {
        m_orientation = QPrinter::Portrait;
    }
    else {
        m_orientation = QPrinter::Landscape;
    }

    QMap<QPrinter::PageSize, QString> pageSizes;
    pageSizes[QPrinter::A0] = QString::fromLatin1("A0");
    pageSizes[QPrinter::A1] = QString::fromLatin1("A1");
    pageSizes[QPrinter::A2] = QString::fromLatin1("A2");
    pageSizes[QPrinter::A3] = QString::fromLatin1("A3");
    pageSizes[QPrinter::A4] = QString::fromLatin1("A4");
    pageSizes[QPrinter::A5] = QString::fromLatin1("A5");
    pageSizes[QPrinter::A6] = QString::fromLatin1("A6");
    pageSizes[QPrinter::A7] = QString::fromLatin1("A7");
    pageSizes[QPrinter::A8] = QString::fromLatin1("A8");
    pageSizes[QPrinter::A9] = QString::fromLatin1("A9");
    pageSizes[QPrinter::B0] = QString::fromLatin1("B0");
    pageSizes[QPrinter::B1] = QString::fromLatin1("B1");
    pageSizes[QPrinter::B2] = QString::fromLatin1("B2");
    pageSizes[QPrinter::B3] = QString::fromLatin1("B3");
    pageSizes[QPrinter::B4] = QString::fromLatin1("B4");
    pageSizes[QPrinter::B5] = QString::fromLatin1("B5");
    pageSizes[QPrinter::B6] = QString::fromLatin1("B6");
    pageSizes[QPrinter::B7] = QString::fromLatin1("B7");
    pageSizes[QPrinter::B8] = QString::fromLatin1("B8");
    pageSizes[QPrinter::B9] = QString::fromLatin1("B9");
    for (QMap<QPrinter::PageSize, QString>::iterator it = pageSizes.begin(); it != pageSizes.end(); ++it) {
        if (fileName.startsWith(it.value(), Qt::CaseInsensitive)) {
            m_pageSize = it.key();
            break;
        }
    }
}

void DrawingView::setDocumentObject(const std::string& name)
{
    m_objectName = name;
}

void DrawingView::closeEvent(QCloseEvent* ev)
{
    MDIView::closeEvent(ev);
    if (!ev->isAccepted())
        return;

    // when closing the view from GUI notify the view provider to mark it invisible
    if (_pcDocument && !m_objectName.empty()) {
        App::Document* doc = _pcDocument->getDocument();
        if (doc) {
            App::DocumentObject* obj = doc->getObject(m_objectName.c_str());
            Gui::ViewProvider* vp = _pcDocument->getViewProvider(obj);
            if (vp)
                vp->hide();
        }
    }
}

void DrawingView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    menu.addAction(this->m_backgroundAction);
    menu.addAction(this->m_outlineAction);
    QMenu* submenu = menu.addMenu(tr("&Renderer"));
    submenu->addAction(this->m_nativeAction);
    submenu->addAction(this->m_glAction);
    submenu->addAction(this->m_imageAction);
    submenu->addSeparator();
    submenu->addAction(this->m_highQualityAntialiasingAction);
    menu.exec(event->globalPos());
}

void DrawingView::setRenderer(QAction *action)
{
#ifndef QT_NO_OPENGL
    m_highQualityAntialiasingAction->setEnabled(false);
#endif

    if (action == m_nativeAction)
        m_view->setRenderer(SvgView::Native);
#ifndef QT_NO_OPENGL
    else if (action == m_glAction) {
        m_highQualityAntialiasingAction->setEnabled(true);
        m_view->setRenderer(SvgView::OpenGL);
    }
#endif
    else if (action == m_imageAction) {
        m_view->setRenderer(SvgView::Image);
    }
}

bool DrawingView::onMsg(const char* pMsg, const char** )
{
    if (strcmp("ViewFit",pMsg) == 0) {
        viewAll();
        return true;
    }
    else if (strcmp("Save",pMsg) == 0) {
        Gui::Document *doc = getGuiDocument();
        if (doc) {
            doc->save();
            return true;
        }
    }
    else if (strcmp("SaveAs",pMsg) == 0) {
        Gui::Document *doc = getGuiDocument();
        if (doc) {
            doc->saveAs();
            return true;
        }
    }
    else  if(strcmp("Undo",pMsg) == 0 ) {
        Gui::Document *doc = getGuiDocument();
        if (doc) {
            doc->undo(1);
            return true;
        }
    }
    else  if(strcmp("Redo",pMsg) == 0 ) {
        Gui::Document *doc = getGuiDocument();
        if (doc) {
            doc->redo(1);
            return true;
        }
    }
    return false;
}

bool DrawingView::onHasMsg(const char* pMsg) const
{
    if (strcmp("ViewFit",pMsg) == 0)
        return true;
    else if (strcmp("Save",pMsg) == 0)
        return getGuiDocument() != 0;
    else if (strcmp("SaveAs",pMsg) == 0)
        return getGuiDocument() != 0;
    else if (strcmp("Undo",pMsg) == 0) {
        App::Document* doc = getAppDocument();
        return doc && doc->getAvailableUndos() > 0;
    }
    else if (strcmp("Redo",pMsg) == 0) {
        App::Document* doc = getAppDocument();
        return doc && doc->getAvailableRedos() > 0;
    }
    else if (strcmp("Print",pMsg) == 0)
        return true;
    else if (strcmp("PrintPreview",pMsg) == 0)
        return true;
    else if (strcmp("PrintPdf",pMsg) == 0)
        return true;
    return false;
}

void DrawingView::onRelabel(Gui::Document *pDoc)
{
    if (!bIsPassive && pDoc) {
        QString cap = QString::fromLatin1("%1 : %2[*]")
            .arg(QString::fromUtf8(pDoc->getDocument()->Label.getValue()))
            .arg(objectName());
        setWindowTitle(cap);
    }
}

void DrawingView::printPdf()
{
    Gui::FileOptionsDialog dlg(this, 0);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setWindowTitle(tr("Export PDF"));
    dlg.setNameFilters(QStringList() << QString::fromLatin1("%1 (*.pdf)").arg(tr("PDF file")));

    QGridLayout *gridLayout;
    QGridLayout *formLayout;
    QGroupBox *groupBox;
    QListWidget *listWidget;
    QListWidgetItem* item;
    QWidget *form = new QWidget(&dlg);
    form->resize(40, 300);
    formLayout = new QGridLayout(form);
    groupBox = new QGroupBox(form);
    gridLayout = new QGridLayout(groupBox);
    listWidget = new QListWidget(groupBox);
    gridLayout->addWidget(listWidget, 0, 0, 1, 1);
    formLayout->addWidget(groupBox, 0, 0, 1, 1);

    groupBox->setTitle(tr("Page sizes"));
    item = new QListWidgetItem(tr("A0"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A0));
    item = new QListWidgetItem(tr("A1"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A1));
    item = new QListWidgetItem(tr("A2"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A2));
    item = new QListWidgetItem(tr("A3"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A3));
    item = new QListWidgetItem(tr("A4"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A4));
    item = new QListWidgetItem(tr("A5"), listWidget);
    item->setData(Qt::UserRole, QVariant(QPrinter::A5));
    int index = 4; // by default A4
    for (int i=0; i<listWidget->count(); i++) {
        if (listWidget->item(i)->data(Qt::UserRole).toInt() == m_pageSize) {
            index = i;
            break;
        }
    }
    listWidget->item(index)->setSelected(true);
    dlg.setOptionsWidget(Gui::FileOptionsDialog::ExtensionRight, form, false);

    if (dlg.exec() == QDialog::Accepted) {
        Gui::WaitCursor wc;
        QString filename = dlg.selectedFiles().front();
        QPrinter printer(QPrinter::HighResolution);
        printer.setFullPage(true);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        printer.setOrientation(m_orientation);
        QList<QListWidgetItem*> items = listWidget->selectedItems();
        if (items.size() == 1) {
            int AX = items.front()->data(Qt::UserRole).toInt();
            printer.setPaperSize(QPrinter::PageSize(AX));
        }

        print(&printer);
    }
}

void DrawingView::print()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setPageSize(m_pageSize);
    printer.setOrientation(m_orientation);

    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        print(&printer);
    }
}

void DrawingView::printPreview()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    printer.setPageSize(m_pageSize);
    printer.setOrientation(m_orientation);

    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, SIGNAL(paintRequested (QPrinter *)),
            this, SLOT(print(QPrinter *)));
    dlg.exec();
}

void DrawingView::print(QPrinter* printer)
{
    // As size of the render area paperRect() should be used. When performing a real
    // print pageRect() may also work but the output is cropped at the bottom part.
    // So, independent whether pageRect() or paperRect() is used there is no scaling effect.
    // However, when using a different paper size as set in the drawing template (e.g.
    // DIN A5 instead of DIN A4) then the output is scaled.
    //
    // When creating a PDF file there seems to be no difference between pageRect() and
    // paperRect().
    //
    // When showing the preview of a print paperRect() must be used because with pageRect()
    // a certain scaling effect can be observed and the content becomes smaller.
    QPaintEngine::Type paintType = printer->paintEngine()->type();
    if (printer->outputFormat() == QPrinter::NativeFormat) {
        int w = printer->widthMM();
        int h = printer->heightMM();
        QPrinter::PaperSize realPaperSize = getPageSize(w, h);
        QPrinter::PaperSize curPaperSize = printer->paperSize();

        // for the preview a 'Picture' paint engine is used which we don't
        // care if it uses wrong printer settings
        bool doPrint = paintType != QPaintEngine::Picture;

        if (doPrint && printer->orientation() != this->m_orientation) {
            int ret = QMessageBox::warning(this, tr("Different orientation"),
                tr("The printer uses a different orientation than the drawing.\n"
                   "Do you want to continue?"),
                   QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
        else if (doPrint && realPaperSize != this->m_pageSize) {
            int ret = QMessageBox::warning(this, tr("Different paper size"),
                tr("The printer uses a different paper size than the drawing.\n"
                   "Do you want to continue?"),
                   QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
        else if (doPrint && curPaperSize != this->m_pageSize) {
            int ret = QMessageBox::warning(this, tr("Different paper size"),
                tr("The printer uses a different paper size than the drawing.\n"
                   "Do you want to continue?"),
                   QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
    }

    QPainter p(printer);
    if (!p.isActive() && !printer->outputFileName().isEmpty()) {
        qApp->setOverrideCursor(Qt::ArrowCursor);
        QMessageBox::critical(this, tr("Opening file failed"),
            tr("Can't open file '%1' for writing.").arg(printer->outputFileName()));
        qApp->restoreOverrideCursor();
        return;
    }
    QRect rect = printer->paperRect();
#ifdef Q_OS_WIN32
    // On Windows the preview looks broken when using paperRect as render area.
    // Although the picture is scaled when using pageRect, it looks just fine.
    if (paintType == QPaintEngine::Picture)
        rect = printer->pageRect();
#endif
    this->m_view->scene()->render(&p, rect);
    p.end();
}

QPrinter::PageSize DrawingView::getPageSize(int w, int h) const
{
    static const float paperSizes[][2] = {
        {210, 297}, // A4
        {176, 250}, // B5
        {215.9f, 279.4f}, // Letter
        {215.9f, 355.6f}, // Legal
        {190.5f, 254}, // Executive
        {841, 1189}, // A0
        {594, 841}, // A1
        {420, 594}, // A2
        {297, 420}, // A3
        {148, 210}, // A5
        {105, 148}, // A6
        {74, 105}, // A7
        {52, 74}, // A8
        {37, 52}, // A8
        {1000, 1414}, // B0
        {707, 1000}, // B1
        {31, 44}, // B10
        {500, 707}, // B2
        {353, 500}, // B3
        {250, 353}, // B4
        {125, 176}, // B6
        {88, 125}, // B7
        {62, 88}, // B8
        {33, 62}, // B9
        {163, 229}, // C5E
        {105, 241}, // US Common
        {110, 220}, // DLE
        {210, 330}, // Folio
        {431.8f, 279.4f}, // Ledger
        {279.4f, 431.8f} // Tabloid
    };

    QPrinter::PageSize ps = QPrinter::Custom;
    for (int i=0; i<30; i++) {
        if (std::abs(paperSizes[i][0]-w) <= 1 &&
            std::abs(paperSizes[i][1]-h) <= 1) {
            ps = static_cast<QPrinter::PageSize>(i);
            break;
        }
        else
        if (std::abs(paperSizes[i][0]-h) <= 1 &&
            std::abs(paperSizes[i][1]-w) <= 1) {
            ps = static_cast<QPrinter::PageSize>(i);
            break;
        }
    }

    return ps;
}

void DrawingView::viewAll()
{
    m_view->fitInView(m_view->scene()->sceneRect(), Qt::KeepAspectRatio);
}

PyObject* DrawingView::getPyObject()
{
    Py_Return;
}

#include "moc_DrawingView.cpp"
