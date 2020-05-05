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

#include <Base/Parameter.h>

#if QT_VERSION  >= 0x050000
#   define FC_HAS_DOCK_OVERLAY
#endif

class QDockWidget;
class QSplitter;

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

class OverlayTabWidget: public QTabWidget
{
    Q_OBJECT
public:
    OverlayTabWidget(QWidget *parent, Qt::DockWidgetArea pos);

    static void setOverlayMode(QWidget *widget, int enable);
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
    void setAutoHideOffset(int offset);

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

    Qt::DockWidgetArea getDockArea() const {return dockArea;}

    const QTime &getRevealTime() const {return revealTime;}
    void setRevealTime(const QTime &time);

    void restore(ParameterGrp::handle handle);
    void saveTabs();

protected:
    void leaveEvent(QEvent*);
    void enterEvent(QEvent*);
    void changeEvent(QEvent*);
    void resizeEvent(QResizeEvent*);
    bool eventFilter(QObject *, QEvent *ev);

    static void _setOverlayMode(QWidget *widget, int enable);
    void retranslate();

protected Q_SLOTS:
    void onCurrentChanged(int index);
    void onTabMoved(int from, int to);
    void onSplitterMoved();
    void setupLayout();

private:
    QSize offset;
    int sizeDelta = 0;
    QRect rectOverlay;
    int autoHideOffset = 0;
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
    bool overlayed = false;
    bool touched = false;
    Qt::DockWidgetArea dockArea;
    int tabSize = 0;
    QTime revealTime;
    ParameterGrp::handle hGrp;
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
public:
    OverlayProxyWidget(OverlayTabWidget *);

    OverlayTabWidget *getOwner() const {return owner;}

protected:
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void hideEvent(QHideEvent*);
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent *);

private:
    OverlayTabWidget* owner;
    bool drawLine = false;
    int pos;
};

#endif // FC_HAS_DOCK_OVERLAY

} // namespace Gui

#endif // GUI_DOCKWINDOWMANAGER_H 
