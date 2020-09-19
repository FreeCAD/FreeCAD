/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DOCKWINDOWMANAGER_H
#define GUI_DOCKWINDOWMANAGER_H

#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <QTime>
#include <QAction>
#include <QToolButton>
#include <QGraphicsEffect>
#include <QImage>

#include <Base/Parameter.h>

#if QT_VERSION  >= 0x050000
#   define FC_HAS_DOCK_OVERLAY
#endif

class QDockWidget;
class QSplitter;
class QPropertyAnimation;

namespace Gui {

struct DockWindowItem {
    QString name;
    Qt::DockWidgetArea pos;
    bool visibility;
    bool tabbed;
};

class GuiExport DockWindowItems
{
public:
    DockWindowItems();
    ~DockWindowItems();

    void addDockWidget(const char* name, Qt::DockWidgetArea pos, bool visibility, bool tabbed);
    void setDockingArea(const char* name, Qt::DockWidgetArea pos);
    void setVisibility(const char* name, bool v);
    void setVisibility(bool v);
    const QList<DockWindowItem>& dockWidgets() const;

private:
    QList<DockWindowItem> _items;
};

/**
 * Class that manages the widgets inside a QDockWidget.
 * \author Werner Mayer
 */
class GuiExport DockWindowManager : public QObject
{
    Q_OBJECT

public:
    /** Creates the only instance of the DockWindowManager. */
    static DockWindowManager* instance();
    static void destruct();

    bool registerDockWindow(const char* name, QWidget* widget);
    QWidget* unregisterDockWindow(const char* name);
    QWidget* findRegisteredDockWindow(const char* name);
    void setup(DockWindowItems*);

    /// Adds a QDockWidget to the main window and sets \a widget as its widget
    QDockWidget* addDockWindow(const char* name, QWidget* widget,  
                 Qt::DockWidgetArea pos = Qt::AllDockWidgetAreas);
    /// Removes and destroys the QDockWidget and returns the widget
    /// with name \a name added with @ref addDockWindow.
    QWidget* removeDockWindow(const char* name);
    /// Removes and destroys the QDockWidget that contains \a dock. \a dock
    /// does not get destroyed.
    void removeDockWindow(QWidget* dock);
    /// Returns the widget with name \a name added with @ref addDockWindow.
    /// @note The returned widget is not the QDockWidget instance
    /// returned from @ref addDockWindow. If you want to access the QDockWidget
    /// you get it with parentWidget() of the returned widget.
    QWidget* getDockWindow(const char* name) const;
    /// Returns a list of all widgets which set to a QDockWidget.
    QList<QWidget*> getDockWindows() const;

    void restoreOverlay();
    void saveOverlay();

    void saveState();
    void retranslate();

    void refreshOverlay(QWidget *widget=nullptr, bool refreshStyle=false);

    void setupTitleBar(QDockWidget *);

    enum OverlayMode {
        ToggleActive,
        ToggleAutoHide,
        ToggleTransparent,
        EnableActive,
        DisableActive,
        EnableAll,
        DisableAll,
        ToggleAll,
        AutoHideAll,
        AutoHideNone,
        ToggleAutoHideAll,
        TransparentAll,
        TransparentNone,
        ToggleTransparentAll,
    };
    void setOverlayMode(OverlayMode mode);

    void changeOverlaySize(int changes);

    void setOverlayMouseTransparent(bool enabled);
    bool isOverlayMouseTransparent() const;

    bool isUnderOverlay() const;

protected:
    bool eventFilter(QObject *, QEvent *ev);

private Q_SLOTS:
   /**
    * \internal
    */
    void onDockWidgetDestroyed(QObject*);
   /**
    * \internal
    */
    void onWidgetDestroyed(QObject*);

    void onToggleDockWidget(bool checked);

    void onDockWidgetTitleChange(const QString &);

    void onTaskViewUpdate();

    void onTimer();

    void onFocusChanged(QWidget *, QWidget *);

    void onAction();

private:
    QDockWidget* findDockWidget(const QList<QDockWidget*>&, const QString&) const;
    
    DockWindowManager();
    ~DockWindowManager();
    static DockWindowManager* _instance;
    struct DockWindowManagerP* d;
};

#ifdef FC_HAS_DOCK_OVERLAY

class OverlayProxyWidget;
class OverlayGraphicsEffect;

class OverlayTabWidget: public QTabWidget
{
    Q_OBJECT

    Q_PROPERTY(QColor effectColor READ effectColor WRITE setEffectColor DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(int effectWidth READ effectWidth WRITE setEffectWidth DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(int effectHeight READ effectHeight WRITE setEffectHeight DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(qreal effectOffsetX READ effectOffsetX WRITE setEffectOffsetX DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(qreal effectOffsetY READ effectOffsetY WRITE setEffectOffsetY DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(qreal effectBlurRadius READ effectBlurRadius WRITE setEffectBlurRadius DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(bool enableEffect READ effectEnabled WRITE setEffectEnabled DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(qreal animation READ animation WRITE setAnimation DESIGNABLE true SCRIPTABLE true)

public:
    OverlayTabWidget(QWidget *parent, Qt::DockWidgetArea pos);

    void setOverlayMode(QWidget *widget, int enable);
    void setOverlayMode(bool enable);
    void addWidget(QDockWidget *widget, const QString &title);
    void removeWidget(QDockWidget *widget);
    void setCurrent(QDockWidget *widget);

    void setAutoHide(bool enable);
    bool isAutoHide() const {return actAutoHide.isChecked();}

    void setTransparent(bool enable);
    bool isTransparent() const {return actTransparent.isChecked();}

    void setEditHide(bool enable);
    bool isEditHide() const {return actEditHide.isChecked();}

    void setEditShow(bool enable);
    bool isEditShow() const {return actEditShow.isChecked();}

    void touch() {touched = true;}
    bool isTouched() const {return touched;}

    void setRect(QRect rect);

    const QRect &getRect();
    bool isOverlayed() const {return overlayed;}
    bool checkAutoHide() const;
    bool getAutoHideRect(QRect &rect) const;
    void changeSize(int changes, bool checkModifier=true);
    void onAction(QAction *);

    void setOffset(const QSize &ofs);
    const QSize &getOffset() const {return offset;}

    void setSizeDelta(int delta);
    int getSizeDelta() const {return sizeDelta;}

    OverlayProxyWidget *getProxyWidget() {return proxyWidget;}

    QDockWidget *currentDockWidget() const;
    QDockWidget *dockWidget(int index) const;
    int dockWidgetIndex(QDockWidget *) const;

    void setTitleBar(QWidget *);

    QSplitter *getSplitter() const {return splitter;}
    QWidget *getTitleBar() const {return titleBar;}

    Qt::DockWidgetArea getDockArea() const {return dockArea;}

    const QTime &getRevealTime() const {return revealTime;}
    void setRevealTime(const QTime &time);

    void restore(ParameterGrp::handle handle);
    void saveTabs();

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
    qreal animation() const {return _animation;}
    void setAnimation(qreal);

    void scheduleRepaint();

    int testAlpha(const QPoint &);

    void startShow();
    void startHide();

    enum State {
        State_Normal,
        State_Hint,
        State_HintHidden,
    };
    void setState(State);
    State getState() const {return _state;}

protected:
    void leaveEvent(QEvent*);
    void enterEvent(QEvent*);
    void changeEvent(QEvent*);
    void resizeEvent(QResizeEvent*);
    void paintEvent(QPaintEvent *);
    bool event(QEvent *ev);
    bool eventFilter(QObject *, QEvent *ev);

    static void _setOverlayMode(QWidget *widget, int enable);
    void retranslate();

protected Q_SLOTS:
    void onCurrentChanged(int index);
    void onTabMoved(int from, int to);
    void onSplitterMoved();
    void onRepaint();
    void onAnimationStateChanged();
    void setupLayout();

private:
    friend class OverlayProxyWidget;
    friend struct DockWindowManagerP;

    QSize offset;
    int sizeDelta = 0;
    QRect rectOverlay;
    OverlayProxyWidget *proxyWidget;
    QSplitter *splitter = nullptr;
    QWidget *titleBar = nullptr;
    QAction actAutoHide;
    QAction actEditHide;
    QAction actEditShow;
    QAction actTransparent;
    QAction actIncrease;
    QAction actDecrease;
    QAction actOverlay;
    QTimer timer;
    QTimer repaintTimer;
    bool repainting = false;
    bool overlayed = false;
    bool touched = false;
    Qt::DockWidgetArea dockArea;
    int tabSize = 0;
    QTime revealTime;
    ParameterGrp::handle hGrp;

    OverlayGraphicsEffect *_graphicsEffect = nullptr;
    OverlayGraphicsEffect *_graphicsEffectTab = nullptr;
    bool _effectEnabled = false;

    QImage _image;
    qreal _imageScale;

    qreal _animation = 0;
    QPropertyAnimation *_animator = nullptr;

    State _state = State_Normal;
};

class OverlayToolButton: public QToolButton
{
    Q_OBJECT
public:
    OverlayToolButton(QWidget *parent);
};

class OverlayProxyWidget: public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QBrush hintColor READ hintColor WRITE setHintColor DESIGNABLE true SCRIPTABLE true)

public:
    OverlayProxyWidget(OverlayTabWidget *);

    OverlayTabWidget *getOwner() const {return owner;}
    bool hitTest(QPoint, bool delay=true);
    bool isActivated() const;

    QBrush hintColor() const;
    void setHintColor(const QBrush &);

protected:
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void hideEvent(QHideEvent*);
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent *);

protected Q_SLOTS:
    void onTimer();

private:
    OverlayTabWidget* owner;
    int drawLine = false;
    int dockArea;
    QTimer timer;
    QBrush _hintColor;
};


// modified from https://stackoverflow.com/a/23752747
class OverlayGraphicsEffect: public QGraphicsEffect
{
    Q_OBJECT
public:
    OverlayGraphicsEffect(QObject *parent);

    virtual void draw(QPainter* painter);
    virtual QRectF boundingRectFor(const QRectF& rect) const;

    inline void setSize(const QSize &size)
        { if(_size!=size){_size = size; updateBoundingRect(); } }

    inline QSize size() const { return _size; }

    inline void setOffset(const QPointF &offset)
        { if(_offset!=offset) {_offset = offset; updateBoundingRect(); } }

    inline QPointF offset() const { return _offset; }

    inline void setBlurRadius(qreal blurRadius)
        { if(_blurRadius!=blurRadius) {_blurRadius = blurRadius; updateBoundingRect();} }

    inline qreal blurRadius() const { return _blurRadius; }

    inline void setColor(const QColor& color) { _color = color; }
    inline QColor color() const { return _color; }

    inline bool enabled() const {return _enabled;}
    inline void setEnabled(bool enabled) 
        { if(_enabled!=enabled) {_enabled = enabled; updateBoundingRect();} }

private:
    bool _enabled;
    QSize  _size;
    qreal  _blurRadius;
    QColor _color;
    QPointF _offset;
};

#endif // FC_HAS_DOCK_OVERLAY

} // namespace Gui

#endif // GUI_DOCKWINDOWMANAGER_H 
