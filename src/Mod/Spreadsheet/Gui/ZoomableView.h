// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 xtemp09 <xtemp09@gmail.com>                         *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <QGraphicsView>

#include "SpreadsheetView.h"

namespace SpreadsheetGui
{
class SheetTableView;
}

class ZoomableView: public QGraphicsView
{
    Q_OBJECT
    Q_PROPERTY(int zoomLevel READ zoomLevel() WRITE setZoomLevel NOTIFY zoomLevelChanged)
public:
    /*!
     * \brief A descendant of QGraphicsView to show a SheetTableView object in its viewport,
     * allowing magnification. \param ui \details The object replaces SheetTableView in layout,
     * handling mouse and keyboard events.
     */
    explicit ZoomableView(Ui::Sheet* ui);
    ~ZoomableView() override = default;

    int zoomLevel() const;
    void setZoomLevel(int new_scale);


    static constexpr int min {60}, max {160};
    static void checkLimits(int& zoom_level);

Q_SIGNALS:
    void zoomLevelChanged(int);  /// This signal is emitted whenever zoom level is changed. It is
                                 /// used to show the zoom level in the zoom button.

public Q_SLOTS:
    void zoomIn(void);   /// This function is the slot for the zoomIn button and a keyboard shortcut
    void zoomOut(void);  /// This function is the slot for the zoomOut button and a keyboard shortcut
    void resetZoom(void);  /// This function is the slot for a keyboard shortcut

private:
    void updateView(void);

    QPointer<SpreadsheetGui::SheetTableView> stv;
    QGraphicsScene m_scene;
    QGraphicsProxyWidget* qpw {nullptr};

    int m_zoomLevel {0};

protected:
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    static constexpr int zoom_step_mwheel {5}, zoom_step_kb {10};
};
