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

#ifndef GUI_IMAGE_VIEW_H
#define GUI_IMAGE_VIEW_H

#include <Gui/MDIView.h>

class QFileInfo;
class QLabel;
class QScrollArea;
class QScrollBar;
class QUrl;

namespace Gui {

class GuiExport ImageView : public MDIView
{
    Q_OBJECT

public:
    explicit ImageView(QWidget* parent);
    bool loadFile(const QString &);

    const char *getName() const override {
        return "ImageView";
    }

    /// Message handler
    bool onMsg(const char* pMsg, const char** ppReturn) override;
    /// Message handler test
    bool onHasMsg(const char* pMsg) const override;

    /** @name Printing */
    //@{
    using MDIView::print;
    void print(QPrinter* printer) override;
    //@}

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;

private:
    void setImage(const QImage& image);
    void scaleImage(double factor);
    static void adjustScrollBar(QScrollBar *scrollBar, double factor);
    bool canZoomIn() const;
    bool canZoomOut() const;
    void zoomIn();
    void zoomOut();
    void normalSize();
    void fitToWindow(bool fitView);
    bool isFitToWindow() const;
    bool canDrag() const;
    void startDrag();
    void stopDrag();
    bool isDragging() const;
    void pasteImage();
    bool canPasteImage() const;
    static QImage imageFromClipboard();
    static bool isImageFormat(const QFileInfo&);
    void loadImageFromUrl(const QList<QUrl>&);

private:
    QImage rawImage;
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    double scaleFactor;
    bool dragging;
    QPoint dragPos;
};

}

#endif //GUI_IMAGE_VIEW_H
