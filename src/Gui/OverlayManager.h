/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QDockWidget>
#include <FCGlobal.h>

class QAction;
class QDockWidget;
class QString;
class QPoint;
class QWidget;

namespace Gui
{

class OverlayTabWidget;

/// Class that manages overlay dockable widgets
class GuiExport OverlayManager: public QObject
{
    Q_OBJECT
public:
    OverlayManager();
    ~OverlayManager() override;

    /// restore states
    void restore();
    /// save states
    void save();
    /// Retranslate text on language change
    void retranslate();
    /// Reload icon images
    void refreshIcons();

    /// Reload mode
    enum class ReloadMode
    {
        /// Reload is pending
        ReloadPending = 0,
        /// Reload is paused
        ReloadPause = 1,
        /// Resume state reload
        ReloadResume = 2,
    };
    /** Set reload mode
     *
     * An internal timer is used to batch handle all relevant parameter
     * changes. The function is provided to let other code temporarily disable
     * the timer before changing the parameter, and then resume it after done.
     */
    void reload(ReloadMode mode = ReloadMode::ReloadPending);

    /** Refresh overlay internal layout
     * @param widget: optional source widget that triggers the refresh
     * @param refreshStyle: whether to reload stylesheet
     */
    void refresh(QWidget* widget = nullptr, bool refreshStyle = false);

    /// Setup title bar for a QDockWidget
    void setupTitleBar(QDockWidget*);

    /// Overlay mode
    enum class OverlayMode
    {
        /// Toggle the focused widget between normal and overlay on top of MDI client area
        ToggleActive,
        /// Toggle overlay dock widget background between opaque and transparent
        ToggleTransparent,
        /// Make the focused widget switch to overlay on top of MDI client area
        EnableActive,
        /// Make the focused widget switch back to normal dockable widget
        DisableActive,
        /// Make all docked widget switch to overlay
        EnableAll,
        /// Make all docked widget switch back to normal
        DisableAll,
        /// Toggle all docked widget between normal and overlay
        ToggleAll,
        /// Set all overlay dock widget to transparent background
        TransparentAll,
        /// Set all overlay dock widget to opaque background
        TransparentNone,
        /// Toggle all overlay dock widget background between opaque and transparent
        ToggleTransparentAll,
        /// Toggle show/hide of the left docked widgets
        ToggleLeft,
        /// Toggle show/hide of the right docked widgets
        ToggleRight,
        /// Toggle show/hide of the top docked widgets
        ToggleTop,
        /// Toggle show/hide of the bottom docked widgets
        ToggleBottom,
    };
    /// Set overlay mode
    void setOverlayMode(OverlayMode mode);

    /// Enable/disable mouse transparent mode
    void setMouseTransparent(bool enabled);
    /// Report if mouse transparent mode is active
    bool isMouseTransparent() const;

    /// Check if the cursor is within an overlay docked widget
    bool isUnderOverlay() const;

    /// Initialize a newly created dock widget
    void initDockWidget(QDockWidget*);
    /// Prepare a dock widget for overlay display
    void setupDockWidget(QDockWidget*, int dockArea = Qt::NoDockWidgetArea);
    /// Switch a dock widget back to normal display
    void unsetupDockWidget(QDockWidget*);

    /** Mouse event handler for dragging a dock widget
     * @param pos: mouse cursor position
     * @param widget: dragging widget, can either be a QDockWidget if it is
     *                floating, or a OverlayTabWidget if docked in overlay mode
     * @param offset: offset from the mouse cursor to the widget origin
     * @param size: widget size before dragging start
     * @param drop: whether to drop after drag to the position
     */
    void dragDockWidget(
        const QPoint& pos,
        QWidget* widget,
        const QPoint& offset,
        const QSize& size,
        bool drop = false
    );

    /// Float an overlay docked widget
    void floatDockWidget(QDockWidget*);

    /// Return the last widget whose mouse event got intercepted by the overlay manager for mouse
    /// pass through
    QWidget* getLastMouseInterceptWidget() const;

    /// Return the stylesheet for overlay widgets
    const QString& getStyleSheet() const;

    /// Check whether to hide tab in overlay dock widget
    bool getHideTab() const;

    /// Helper function to set focus when switching active sub window
    static void setFocusView();

    /// Return the singleton instance of the overlay manager
    static OverlayManager* instance();
    /// Destroy the overlay manager
    static void destruct();

    class Private;

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

    /// Register a named docked widget with an overlay tab widget
    void registerDockWidget(const QString& name, OverlayTabWidget*);
    /// Unregister a named docked widget with an overlay tab widget
    void unregisterDockWidget(const QString& name, OverlayTabWidget*);

private:
    void onToggleDockWidget(bool checked);
    void onDockVisibleChange(bool visible);
    void onDockFeaturesChange(QDockWidget::DockWidgetFeatures features);
    void onDockWidgetTitleChange(const QString&);
    void onTaskViewUpdate();
    void onFocusChanged(QWidget*, QWidget*);
    void onAction();

private Q_SLOTS:
    void raiseAll();

private:
    friend class Private;
    friend class OverlayTabWidget;
    Private* d;
};

}  // namespace Gui
