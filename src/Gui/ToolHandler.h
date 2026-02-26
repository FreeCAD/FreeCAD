/***************************************************************************
 *   Copyright (c) 2024 Pierre-Louis Boyer <development@ondsel.com>        *
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

#pragma once

#include <QCursor>
#include <QPixmap>

#include <Base/Parameter.h>
#include <Base/Tools2D.h>

#include "Selection/Selection.h"


namespace Gui
{
class View3DInventorViewer;
struct InputHint;

class GuiExport ToolHandler
{
public:
    ToolHandler() = default;
    virtual ~ToolHandler() = default;

    bool activate();
    virtual void deactivate();

    virtual void quit()
    {}

    /// updates the actCursor with the icon by calling getCrosshairCursorSVGName(),
    /// enabling to set data member dependent icons (i.e. for different construction methods)
    void updateCursor();

    virtual std::list<InputHint> getToolHints() const;
    void updateHint() const;

private:  // NVI
    virtual void preActivated()
    {}
    virtual void activated()
    {}
    virtual void deactivated()
    {}
    virtual void postDeactivated()
    {}

protected:  // NVI requiring base implementation
    virtual QString getCrosshairCursorSVGName() const;


protected:
    // helpers
    /**
     * Sets a cursor for 3D inventor view.
     * pixmap as a cursor image in device independent pixels.
     *
     * \param autoScale - set this to false if pixmap already scaled for HiDPI
     **/

    /** @name Icon helpers */
    //@{
    void setCursor(const QPixmap& pixmap, int x, int y, bool autoScale = true);


    void unsetCursor();

    /// restitutes the DSH cached cursor (without any tail due to autoconstraints, ...)
    void applyCursor();

    void addCursorTail(std::vector<QPixmap>& pixmaps);

    /// returns the color to be used for the crosshair (configurable as a parameter)
    unsigned long getCrosshairColor();

    /// functions to set the cursor to a given svgName (to be migrated to NVI style)

    qreal devicePixelRatio();
    //@}

    View3DInventorViewer* getViewer();

    virtual QWidget* getCursorWidget();

    virtual void setWidgetCursor(QCursor cursor);

private:
    void setSvgCursor(
        const QString& svgName,
        int x,
        int y,
        const std::map<unsigned long, unsigned long>& colorMapping
        = std::map<unsigned long, unsigned long>()
    );


    void applyCursor(QCursor& newCursor);

    void setCrosshairCursor(const QString& svgName);
    void setCrosshairCursor(const char* svgName);

protected:
    QCursor oldCursor;
    QCursor actCursor;
    QPixmap actCursorPixmap;
};


}  // namespace Gui
