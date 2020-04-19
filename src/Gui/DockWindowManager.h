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

#if QT_VERSION  >= 0x050000
#   define FC_HAS_DOCK_OVERLAY
#endif

class QDockWidget;

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
class GuiExport DockWindowManager : QObject
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

    void refreshOverlay(QWidget *widget=nullptr);

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
    void setCurrent(QWidget *widget);

    void setAutoHide(bool enable);
    bool isAutoHide() const {return autoHide;}

    void setTransparent(bool enable);
    bool isTransparent() const {return transparent;}

    void setRect(QRect rect, bool overlay);
    const QRect &getRect(bool overlay);
    bool isOverlayed() const {return overlayed;}
    bool checkAutoHide() const;
    bool getAutoHideRect(QRect &rect) const;
    void changeSize(int changes);

    OverlayProxyWidget *getProxyWidget() {return proxyWidget;}

protected:
    void leaveEvent(QEvent*);
    void enterEvent(QEvent*);

    static void _setOverlayMode(QWidget *widget, int enable);

private:
    QRect rectActive;
    QRect rectOverlay;
    OverlayProxyWidget *proxyWidget;
    bool overlayed = false;
    bool autoHide = false;
    bool transparent = false;
};

class OverlayProxyWidget: public QWidget
{
    Q_OBJECT
public:
    OverlayProxyWidget(OverlayTabWidget *);

    OverlayTabWidget *getOwner() const {return owner;}

protected:
    void enterEvent(QEvent*);

private:
    OverlayTabWidget* owner;
};

#endif // FC_HAS_DOCK_OVERLAY

} // namespace Gui

#endif // GUI_DOCKWINDOWMANAGER_H 
