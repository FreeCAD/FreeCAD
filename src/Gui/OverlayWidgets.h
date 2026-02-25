/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#include <QTabWidget>
#include <QTimer>
#include <QTime>
#include <QAction>
#include <QToolButton>
#include <QGraphicsEffect>
#include <QImage>
#include <QDockWidget>
#include <QSplitter>
#include <QMenu>

#include <Base/Parameter.h>

#include "OverlayManager.h"

class QPropertyAnimation;
class QLayoutItem;

namespace Gui
{

class OverlayTabWidget;
class OverlayTitleBar;
class OverlaySplitterHandle;
class OverlaySizeGrip;
class OverlayProxyWidget;
class OverlayGraphicsEffect;
class OverlayDragFrame;

/// Tab widget to contain dock widgets in overlay mode
class OverlayTabWidget: public QTabWidget
{
    Q_OBJECT

    /** @name Graphics effect properties for customization through stylesheet */
    //@{
    Q_PROPERTY(
        QColor effectColor READ effectColor WRITE setEffectColor
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        int effectWidth READ effectWidth WRITE setEffectWidth
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        int effectHeight READ effectHeight WRITE setEffectHeight
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        qreal effectOffsetX READ effectOffsetX WRITE setEffectOffsetX
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        qreal effectOffsetY READ effectOffsetY WRITE setEffectOffsetY
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        qreal effectBlurRadius READ effectBlurRadius WRITE setEffectBlurRadius
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        bool enableEffect READ effectEnabled WRITE setEffectEnabled
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(qreal animation READ animation WRITE setAnimation)  // clazy:exclude=qproperty-without-notify
    //@}

public:
    /** Constructor
     * @param parent: parent widget
     * @param pos: docking position
     */
    OverlayTabWidget(QWidget* parent, Qt::DockWidgetArea pos);

    /// Enable/disable overlay mode for this tab widget
    void setOverlayMode(bool enable);

    /// Update splitter handle according to overlay status
    void updateSplitterHandles();

    /** Add a dock widget
     * @param widget: dock widget to be added
     * @param title: title for the dock widget
     */
    void addWidget(QDockWidget* widget, const QString& title);

    /** Remove a dock widget
     * @param widget: dock widget to be removed
     * @param last: optional non overlaid widget. If given, then the removed
     * dock widget will be tabified together with this one.
     */
    void removeWidget(QDockWidget* widget, QDockWidget* last = nullptr);

    /** Set the current dock widget
     * @param widget: an overlay dock widget
     *
     * All other dock widget in the same tab widget will be collapsed, and the
     * give widget will take the whole space. This is actually handled inside
     * onCurrentChanged().
     */
    void setCurrent(QDockWidget* widget);

    /** Handle ESC key press
     *
     * If the this overlay tab widget is hidden and its hint/tabs are visible,
     * pressing ESC will hide the hint and tabs.
     *
     * If this overlay tab widget is visible and the current mouse cursor is on
     * top of the tab widget title bar or any of its split handler, then
     * pressing ESC will hide the title bar and split handler.
     */
    bool onEscape();

    /// Enable/disable transparent background mode
    void setTransparent(bool enable);

    /// Check if transparent background mode is active
    bool isTransparent() const;

    /// Auto mode to show or hide the tab widget
    enum class AutoMode
    {
        /// No auto show or hide
        NoAutoMode,
        /// Auto hide tab widget on lost of focus
        AutoHide,
        /// Auto show tab widget on any new editing task
        EditShow,
        /// Auto hide tab widget on any new editing task
        EditHide,
        /// Auto show on any task panel e.g. suggestive task panel in PartDesign
        TaskShow,
    };
    /// Set auto mode to show or hide the tab widget
    void setAutoMode(AutoMode mode);
    /// Get current auto mode
    AutoMode getAutoMode() const
    {
        return autoMode;
    }

    /// Touch the tab widget to trigger saving of settings
    void touch()
    {
        touched = true;
    }
    /// Check if the tab widget settings need to be saved
    bool isTouched() const
    {
        return touched;
    }

    /// Set geometry of this tab widget
    void setRect(QRect rect);

    /// Get the geometry of this tab widget
    const QRect& getRect();

    /// Overlay query option
    enum class QueryOption
    {
        /// Report the current overlay status
        QueryOverlay,
        /// Report true if transparency status has been changed
        TransparencyChanged,
        /// Report true if transparency status has not been changed
        TransparencyNotChanged,
    };
    /// Query overlay status
    bool isOverlaid(QueryOption option = QueryOption::QueryOverlay) const;
    /** Check if needs to auto hide this tab widget
     *
     * Besides when auto hide mode is activated by user, the tab widget will
     * also auto hide if the current view does not have panning capability
     * (queried through MdiView::hasMsg("CanPan")). The user can explicitly
     * disable this auto hide if no pan by setting user parameter
     * View/DockOverlayAutoView, or preference option Display -> UI -> Auto
     * hide in non 3D view.
     */
    bool checkAutoHide() const;
    /** Obtain geometry of auto hiding tab widget
     * @param rect: output geometry of the tab widget
     * @return Return true if the tab widget should be auto hiding
     */
    bool getAutoHideRect(QRect& rect) const;
    /// Handler of various actions exposed as buttons on title bar
    void onAction(QAction*);
    /// Sync relevant actions status with the current auto mode
    void syncAutoMode();
    // Establish if stylesheet is dark
    static bool isStyleSheetDark(std::string curStyleSheet);
    // Rotate the AutoHide icon according to the dock area
    static QPixmap rotateAutoHideIcon(QPixmap pxAutoHide, Qt::DockWidgetArea dockArea);

    /** Set tab widget position offset
     * @param ofs: the offset size. Width is the x offset for top and bottom
     * docking tab widget, and y offset for left and right ones. Height is the y
     * offset for top and bottom, and x offset for left and right ones.
     */
    void setOffset(const QSize& ofs);
    /// Get the tab widget position offset
    const QSize& getOffset() const
    {
        return offset;
    }

    /** Set tab widget size delta
     * @param delta: the size delta. For left and right widget, the delta is
     * added to the height of the tab widget. For top and bottom widget, it is
     * added to the width.
     */
    void setSizeDelta(int delta);
    /// Get the tab widget size delta
    int getSizeDelta() const
    {
        return sizeDelta;
    }

    /// Obtain the proxy widget
    OverlayProxyWidget* getProxyWidget()
    {
        return proxyWidget;
    }

    /// Obtain the current dock widget
    QDockWidget* currentDockWidget() const;
    /// Obtain the dock widget by index
    QDockWidget* dockWidget(int index) const;
    /// Obtain the index of a given dock widget
    int dockWidgetIndex(QDockWidget*) const;

    /// Set the title bar for this tab widget
    void setTitleBar(QWidget*);

    /// Get the splitter
    QSplitter* getSplitter() const
    {
        return splitter;
    }
    /// Get the title bar
    QWidget* getTitleBar() const
    {
        return titleBar;
    }

    /// Get the docking position of this tab widget
    Qt::DockWidgetArea getDockArea() const
    {
        return dockArea;
    }

    /// Get delay time for animated reveal
    const QTime& getRevealTime() const
    {
        return revealTime;
    }
    /// Set delay time for animated reveal
    void setRevealTime(const QTime& time);

    /// Restore state
    void restore(ParameterGrp::handle handle);
    /// Save tab orders and positions
    void saveTabs();

    /** @name Graphics effect properties setters and getters */
    //@{
    QColor effectColor() const;
    void setEffectColor(const QColor&);
    int effectWidth() const;
    void setEffectWidth(int);
    int effectHeight() const;
    void setEffectHeight(int);
    qreal effectOffsetX() const;
    void setEffectOffsetX(qreal);
    qreal effectOffsetY() const;
    void setEffectOffsetY(qreal);
    qreal effectBlurRadius() const;
    void setEffectBlurRadius(qreal);
    bool effectEnabled() const;
    void setEffectEnabled(bool);
    qreal animation() const
    {
        return _animation;
    }
    void setAnimation(qreal);
    //@}

    /// Schedule for repaint
    void scheduleRepaint();

    /** Return the pixel alpha value at the give position
     * @param pos: position
     * @param radiusScale: scale of the radius to check for alpha.
     *
     * @return Returns the largest alpha value of a circular area of 'pos' as
     * center and  radius as defined by user parameter
     * View/DockOverlayAlphaRadius. May return -1 if out side of the widget, or
     * zero if transparent.
     */
    int testAlpha(const QPoint& pos, int radiusScale);

    /// Start animated showing
    void startShow();
    /// Start animated hiding
    void startHide();

    /// Internal state of the tab widget
    enum class State
    {
        /// The tab widget is showing
        Showing,
        /// Normal visible state
        Normal,
        /// Visual hint is visible
        Hint,
        /// Hint is hidden by user after pressing ESC
        HintHidden,
        /// The tab widget is explicitly hidden by user
        Hidden,
    };
    /// Set state of the tab widget
    void setState(State);
    /// Get the state of the widget
    State getState() const
    {
        return _state;
    }

    /// Handle splitter resize
    void onSplitterResize(int index);

    /// Check if the tab widget is saving its state
    bool isSaving() const
    {
        return _saving;
    }

    /// Helper function to create title bar for a dock widget
    static QWidget* createTitleButton(QAction* action, int size);
    /// Helper function to prepare a widget as a title widget
    static QLayoutItem* prepareTitleWidget(QWidget* widget, const QList<QAction*>& actions);

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent* ev) override;
#else
    void enterEvent(QEnterEvent* ev) override;
#endif
    void leaveEvent(QEvent* ev) override;
    void changeEvent(QEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;
    bool event(QEvent* ev) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

    void retranslate();
    void refreshIcons();

    /// Overlay mode options
    enum class OverlayOption
    {
        /// Enable overlay
        Enable,
        /// Disable overlay
        Disable,
        /// Enable overlay and show tab bar
        ShowTab,
    };
    /// Toggle overlay mode for a given widget
    void setOverlayMode(QWidget* widget, OverlayOption option);
    /// Helper function to set overlay mode for a give widget
    static void _setOverlayMode(QWidget* widget, OverlayOption option);

protected:
    void onCurrentChanged(int index);
    void onTabMoved(int from, int to);
    void onRepaint();
    void onAnimationStateChanged();
    void setupLayout();
    void onSizeGripMove(const QPoint&);

private:
    friend class OverlayProxyWidget;
    friend class OverlayTitleBar;
    friend class OverlayManager;
    friend class OverlayManager::Private;
    friend class OverlaySplitterHandle;
    friend class OverlaySizeGrip;

    QSize offset;
    int sizeDelta = 0;
    QRect rectOverlay;
    OverlayProxyWidget* proxyWidget;
    QSplitter* splitter = nullptr;
    QWidget* titleBar = nullptr;
    QAction actNoAutoMode;
    QAction actAutoHide;
    QAction actEditHide;
    QAction actEditShow;
    QAction actTaskShow;
    QAction actAutoMode;
    QMenu autoModeMenu;
    QAction actTransparent;
    QAction actIncrease;
    QAction actDecrease;
    QAction actOverlay;
    QTimer timer;
    QTimer repaintTimer;
    AutoMode autoMode = AutoMode::NoAutoMode;
    bool repainting = false;
    bool overlaid = false;
    bool currentTransparent = false;
    bool touched = false;
    bool busy = false;
    Qt::DockWidgetArea dockArea;
    int tabSize = 0;
    QTime revealTime;
    ParameterGrp::handle hGrp;

    OverlayGraphicsEffect* _graphicsEffect = nullptr;
    OverlayGraphicsEffect* _graphicsEffectTab = nullptr;
    bool _effectEnabled = false;

    QImage _image;
    qreal _imageScale;

    qreal _animation = 0;
    QPropertyAnimation* _animator = nullptr;

    State _state = State::Normal;

    std::map<QDockWidget*, int> _sizemap;
    bool _saving = false;

    // NOLINTBEGIN
    static OverlayDragFrame* _DragFrame;
    static QDockWidget* _DragFloating;
    static QWidget* _Dragging;
    static OverlayTabWidget* _LeftOverlay;
    static OverlayTabWidget* _RightOverlay;
    static OverlayTabWidget* _TopOverlay;
    static OverlayTabWidget* _BottomOverlay;
    // NOLINTEND
};

/// A translucent frame as a visual indicator when dragging a dock widget
class OverlayDragFrame: public QWidget
{
    Q_OBJECT
public:
    explicit OverlayDragFrame(QWidget* parent);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* ev) override;
};

/// Title bar for OverlayTabWidget
class OverlayTitleBar: public QWidget
{
    Q_OBJECT
public:
    explicit OverlayTitleBar(QWidget* parent);
    void setTitleItem(QLayoutItem*);
    void endDrag();

protected:
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;
    void keyPressEvent(QKeyEvent* ke) override;
    void timerEvent(QTimerEvent* te) override;

private:
    QPoint dragOffset;
    QSize dragSize;
    QLayoutItem* titleItem = nullptr;
    QColor textcolor;
    int timerId = 0;
    bool blink = false;
    bool mouseMovePending = false;
    bool ignoreMouse = false;
};

/// Size grip for title bar and split handler of OverlayTabWidget
class OverlaySizeGrip: public QWidget
{
    Q_OBJECT
public:
    OverlaySizeGrip(QWidget* parent, bool vertical);

Q_SIGNALS:
    void dragMove(const QPoint& globalPos);

protected:
    void paintEvent(QPaintEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    const QPixmap& pixmap() const;

private:
    bool vertical;
};

/// Splitter for OverlayTabWidget
class OverlaySplitter: public QSplitter
{
    Q_OBJECT
public:
    explicit OverlaySplitter(QWidget* parent);
    void retranslate();

protected:
    QSplitterHandle* createHandle() override;
};


/// Splitter handle for dragging the splitter
class OverlaySplitterHandle: public QSplitterHandle
{
    Q_OBJECT
public:
    friend class OverlaySplitter;

    OverlaySplitterHandle(Qt::Orientation, QSplitter* parent);
    void setTitleItem(QLayoutItem*);
    void retranslate();
    void refreshIcons();
    QDockWidget* dockWidget();

    void showTitle(bool enable);
    bool isShowing() const
    {
        return _showTitle;
    }
    void endDrag();

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent* ev) override;
#else
    void enterEvent(QEnterEvent* ev) override;
#endif
    void showEvent(QShowEvent* ev) override;
    void leaveEvent(QEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;
    void changeEvent(QEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
    QSize sizeHint() const override;

protected:
    void onAction();
    void onTimer();

private:
    QLayoutItem* titleItem = nullptr;
    int idx = -1;
    QAction actFloat;
    bool _showTitle = true;
    int dragging = 0;
    QPoint dragOffset;
    QSize dragSize;
    QTimer timer;
};


/// Tool button for the title bar of the OverlayTabWidget
class OverlayToolButton: public QToolButton
{
    Q_OBJECT
public:
    explicit OverlayToolButton(QWidget* parent);
};

/** Class for handling visual hint for bringing back hidden overlay dock widget
 *
 * The proxy widget is transparent except a customizable rectangle area with a
 * selectable color shown as the visual hint. The hint is normally hidden, and
 * is shown only if the mouse hovers within the widget. When the hint area is
 * clicked, it will bring back hidden overlay dock panel. Note that the proxy
 * widget itself is mouse transparent as well, meaning that it will not receive
 * any mouse event. It is handled in the OverlayManager event filter.
 */
class OverlayProxyWidget: public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QBrush hintColor READ hintColor WRITE setHintColor)  // clazy:exclude=qproperty-without-notify

public:
    explicit OverlayProxyWidget(OverlayTabWidget*);

    OverlayTabWidget* getOwner() const
    {
        return owner;
    }

    /// For representing hit region
    enum class HitTest
    {
        /// Not hitting
        HitNone = 0,
        /// Hitting the proxy widget size but not within the visible hint area.
        HitOuter = 1,
        /// Hitting the visible hint area.
        HitInner = 2,
    };
    /** Mouse cursor hit test
     * @param pos: cursor position
     * @param delay: Whether to delay showing hint on mouse hit
     */
    HitTest hitTest(const QPoint& pos, bool delay = true);

    /// Check if the visual hint is showing
    bool isActivated() const;

    QBrush hintColor() const;
    void setHintColor(const QBrush&);

    QRect getRect() const;

    void onMousePress();

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent* ev) override;
#else
    void enterEvent(QEnterEvent* ev) override;
#endif
    void hideEvent(QHideEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;

protected:
    void onTimer();

private:
    OverlayTabWidget* owner;
    bool drawLine = false;
    int dockArea;
    QTimer timer;
    QBrush _hintColor;
};

/** Graphic effects for drawing shadow and outline of text on transparent background
 *
 * Modified from https://stackoverflow.com/a/23752747
 */
class OverlayGraphicsEffect: public QGraphicsEffect
{
    Q_OBJECT
public:
    explicit OverlayGraphicsEffect(QObject* parent);

    void draw(QPainter* painter) override;
    QRectF boundingRectFor(const QRectF& rect) const override;

    inline void setSize(const QSize& size)
    {
        if (_size != size) {
            _size = size;
            updateBoundingRect();
        }
    }

    inline QSize size() const
    {
        return _size;
    }

    inline void setOffset(const QPointF& offset)
    {
        if (_offset != offset) {
            _offset = offset;
            updateBoundingRect();
        }
    }

    inline QPointF offset() const
    {
        return _offset;
    }

    inline void setBlurRadius(qreal blurRadius)
    {
        if (_blurRadius != blurRadius) {
            _blurRadius = blurRadius;
            updateBoundingRect();
        }
    }

    inline qreal blurRadius() const
    {
        return _blurRadius;
    }

    inline void setColor(const QColor& color)
    {
        _color = color;
    }
    inline QColor color() const
    {
        return _color;
    }

    inline bool enabled() const
    {
        return _enabled;
    }
    inline void setEnabled(bool enabled)
    {
        if (_enabled != enabled) {
            _enabled = enabled;
            updateBoundingRect();
        }
    }

private:
    bool _enabled;
    QSize _size;
    qreal _blurRadius;
    QColor _color;
    QPointF _offset;
};

}  // namespace Gui
