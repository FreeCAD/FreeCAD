// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QAction>
# include <QApplication>
# include <QContextMenuEvent>
# include <QClipboard>
# include <QCursor>
# include <QFileInfo>
# include <QImageReader>
# include <QLabel>
# include <QMenu>
# include <QMessageBox>
# include <QMimeData>
# include <QPainter>
# include <QPixmap>
# include <QPrintDialog>
# include <QPrinter>
# include <QScrollArea>
# include <QScrollBar>
#endif

#include "ImageView.h"
#include "BitmapFactory.h"

using namespace Gui;

ImageView::ImageView(QWidget* parent)
    : MDIView(nullptr, parent)
    , imageLabel(new QLabel)
    , scrollArea(new QScrollArea)
    , scaleFactor{1.0}
    , dragging{false}
{
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setVisible(false);
    setCentralWidget(scrollArea);
    setAcceptDrops(true);
    setWindowIcon(Gui::BitmapFactory().pixmap("colors"));
}

bool ImageView::loadFile(const QString& fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::information(this, tr("Failed to load image file"),
                                 tr("Cannot load file %1: %2")
                                 .arg(fileName, reader.errorString()));
        return false;
    }

    setImage(image);
    setWindowFilePath(fileName);

    return true;
}

void ImageView::setImage(const QImage& image)
{
    rawImage = image;
    imageLabel->setPixmap(QPixmap::fromImage(image));
    imageLabel->adjustSize();
    scrollArea->setVisible(true);
    scaleFactor = 1.0;
}

void ImageView::scaleImage(double factor)
{
    scaleFactor *= factor;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    imageLabel->resize(scaleFactor * imageLabel->pixmap(Qt::ReturnByValue).size());
#else
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());
#endif

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);
}

void ImageView::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}

bool ImageView::canZoomIn() const
{
    int maxWidth{10000};
    return !isFitToWindow() && imageLabel->width() < maxWidth;
}

bool ImageView::canZoomOut() const
{
    int minWidth{200};
    return !isFitToWindow() && imageLabel->width() > minWidth;
}

void ImageView::zoomIn()
{
    double scale{1.25};
    scaleImage(scale);
}

void ImageView::zoomOut()
{
    double scale{0.8};
    scaleImage(scale);
}

void ImageView::normalSize()
{
    imageLabel->adjustSize();
    scaleFactor = 1.0;
}

void ImageView::fitToWindow(bool fitView)
{
    scrollArea->setWidgetResizable(fitView);
    if (!fitView) {
        normalSize();
    }
}

bool ImageView::isFitToWindow() const
{
    return scrollArea->widgetResizable();
}

bool ImageView::canDrag() const
{
    return scrollArea->verticalScrollBar()->isVisible() ||
           scrollArea->horizontalScrollBar()->isVisible();
}

void ImageView::startDrag()
{
    dragging = true;
}

void ImageView::stopDrag()
{
    dragging = false;
}

bool ImageView::isDragging() const
{
    return dragging;
}

void ImageView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;
    QAction* fitToWindowAct = menu.addAction(tr("Fit to window"));
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setChecked(isFitToWindow());
    connect(fitToWindowAct, &QAction::toggled, this, &ImageView::fitToWindow);

    QAction* zoomInAct = menu.addAction(tr("Zoom in"), this, &ImageView::zoomIn);
    zoomInAct->setEnabled(canZoomIn());

    QAction* zoomOutAct = menu.addAction(tr("Zoom out"), this, &ImageView::zoomOut);
    zoomOutAct->setEnabled(canZoomOut());

    menu.exec(event->globalPos());
}

void ImageView::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons().testFlag(Qt::MiddleButton)) {
        if (canDrag()) {
            setCursor(QCursor(Qt::ClosedHandCursor));
            startDrag();
            dragPos = event->pos();
        }
    }
}

void ImageView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!event->buttons().testFlag(Qt::MiddleButton)) {
        if (isDragging()) {
            stopDrag();
            unsetCursor();
        }
    }
}

void ImageView::mouseMoveEvent(QMouseEvent* event)
{
    if (isDragging()) {
        QScrollBar* hBar = scrollArea->horizontalScrollBar();
        QScrollBar* vBar = scrollArea->verticalScrollBar();
        QPoint delta = event->pos() - dragPos;
        hBar->setValue(hBar->value() + (isRightToLeft() ? delta.x() : -delta.x()));
        vBar->setValue(vBar->value() - delta.y());
        dragPos = event->pos();
    }
}

void ImageView::dropEvent(QDropEvent* event)
{
    const QMimeData* data = event->mimeData();
    if (data->hasUrls()) {
        loadImageFromUrl(data->urls());
    }
    else {
        MDIView::dropEvent(event);
    }
}

void ImageView::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* data = event->mimeData();
    if (data->hasUrls()) {
        event->accept();
    }
    else {
        event->ignore();
    }
}

bool ImageView::isImageFormat(const QFileInfo& fileInfo)
{
    QString ext = fileInfo.suffix().toLower();
    QByteArray suffix = ext.toLatin1();
    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    auto it = std::find_if(supportedFormats.begin(), supportedFormats.end(), [suffix](const QByteArray& image) {
        return (image == suffix);
    });

    return (it != supportedFormats.end());
}

void ImageView::loadImageFromUrl(const QList<QUrl>& urls)
{
    if (urls.isEmpty()) {
        return;
    }

    const QUrl& url = urls.first();
    const QFileInfo info(url.toLocalFile());
    if (info.exists() && info.isFile()) {
        if (isImageFormat(info)) {
            loadFile(info.absoluteFilePath());
        }
    }
}

void ImageView::pasteImage()
{
    QImage image = imageFromClipboard();
    if (!image.isNull()) {
        setImage(image);
    }
}

bool ImageView::canPasteImage() const
{
    return !imageFromClipboard().isNull();
}

QImage ImageView::imageFromClipboard()
{
    QImage image;
    if (const QMimeData *mimeData = QApplication::clipboard()->mimeData()) {
        if (mimeData->hasImage()) {
            image = qvariant_cast<QImage>(mimeData->imageData());
        }
    }

    return image;
}

void ImageView::print(QPrinter* printer)
{
    QPainter painter(printer);
    QPixmap pixmap = QPixmap::fromImage(rawImage);
    QRect rect = painter.viewport();
    QSize size = pixmap.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
    painter.setWindow(pixmap.rect());
    painter.drawPixmap(0, 0, pixmap);
}

bool ImageView::onMsg(const char* pMsg,const char** ppReturn)
{
    Q_UNUSED(ppReturn)
    if (strcmp("ViewFit", pMsg) == 0) {
        fitToWindow(true);
        return true;
    }
    if (strcmp("ZoomIn", pMsg) == 0) {
        zoomIn();
        return true;
    }
    if (strcmp("ZoomOut", pMsg) == 0) {
        zoomOut();
        return true;
    }
    if (strcmp("Paste", pMsg) == 0) {
        pasteImage();
        return true;
    }
    if (strcmp("Print", pMsg) == 0) {
        print();
        return true;
    }
    if (strcmp("PrintPreview", pMsg) == 0) {
        printPreview();
        return true;
    }
    if (strcmp("PrintPdf", pMsg) == 0) {
        printPdf();
        return true;
    }

    return false;
}

bool ImageView::onHasMsg(const char* pMsg) const
{
    if (strcmp("ViewFit", pMsg) == 0) {
        return true;
    }
    if (strcmp("ZoomIn", pMsg) == 0) {
        return canZoomIn();
    }
    if (strcmp("ZoomOut", pMsg) == 0) {
        return canZoomOut();
    }
    if (strcmp("Paste", pMsg) == 0) {
        return canPasteImage();
    }
    if (strcmp("Print", pMsg) == 0) {
        return true;
    }
    if (strcmp("PrintPreview", pMsg) == 0) {
        return true;
    }
    if (strcmp("PrintPdf", pMsg) == 0) {
        return true;
    }

    return false;
}

#include "moc_ImageView.cpp"
