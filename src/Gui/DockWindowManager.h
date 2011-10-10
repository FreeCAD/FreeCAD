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
    void setup(DockWindowItems*);

    QDockWidget* addDockWindow(const char* name, QWidget* widget,  
                 Qt::DockWidgetArea pos = Qt::AllDockWidgetAreas);
    QWidget* removeDockWindow(const char* name);
    void removeDockWindow(QWidget* dock);
    QWidget* getDockWindow(const char* name) const;
    QList<QWidget*> getDockWindows() const;

    void saveState();
    void retranslate();

private Q_SLOTS:
   /**
    * \internal
    */
    void onDockWidgetDestroyed(QObject*);
   /**
    * \internal
    */
    void onWidgetDestroyed(QObject*);

private:
    QDockWidget* findDockWidget(const QList<QDockWidget*>&, const QString&) const;
    
    DockWindowManager();
    ~DockWindowManager();
    static DockWindowManager* _instance;
    struct DockWindowManagerP* d;
};

} // namespace Gui

#endif // GUI_DOCKWINDOWMANAGER_H 
