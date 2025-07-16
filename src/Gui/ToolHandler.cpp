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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <cmath>

#include <QGuiApplication>
#include <QPainter>

#include <Inventor/events/SoKeyboardEvent.h>
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>

#include "Application.h"
#include "BitmapFactory.h"
#include "CommandT.h"
#include "MainWindow.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

#include "ToolHandler.h"
#include "InputHint.h"

using namespace Gui;

/**************************** ToolHandler *******************************************/

QString ToolHandler::getCrosshairCursorSVGName() const
{
    return QStringLiteral("None");
}

bool ToolHandler::activate()
{
    // save the cursor at the time the DSH is activated
    QWidget* cw = getCursorWidget();
    if (cw) {
        oldCursor = cw->cursor();

        updateCursor();
        updateHint();

        this->preActivated();
        this->activated();
        return true;
    }

    return false;
}

void ToolHandler::deactivate()
{
    this->deactivated();
    this->postDeactivated();

    unsetCursor();

    Gui::MainWindow::getInstance()->hideHints();
}

//**************************************************************************
// Helpers

unsigned long ToolHandler::getCrosshairColor()
{
    unsigned long color = 0xFFFFFFFF;  // white
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    color = hGrp->GetUnsigned("CursorCrosshairColor", color);
    // from rgba to rgb
    color = (color >> 8) & 0xFFFFFF;
    return color;
}

void ToolHandler::setCrosshairCursor(const QString& svgName)
{
    const unsigned long defaultCrosshairColor = 0xFFFFFF;
    unsigned long color = getCrosshairColor();
    auto colorMapping = std::map<unsigned long, unsigned long>();
    colorMapping[defaultCrosshairColor] = color;
    // hot spot of all SVG icons should be 8,8 for 32x32 size (16x16 for 64x64)
    int hotX = 8;
    int hotY = 8;
    setSvgCursor(svgName, hotX, hotY, colorMapping);
}

void ToolHandler::setCrosshairCursor(const char* svgName)
{
    QString cursorName = QString::fromLatin1(svgName);
    setCrosshairCursor(cursorName);
}

void ToolHandler::setSvgCursor(const QString& cursorName,
                                     int x,
                                     int y,
                                     const std::map<unsigned long, unsigned long>& colorMapping)
{
    // The TechDraw_Pointer_*.svg icons have a default size of 64x64. When directly creating
    // them with a size of 32x32 they look very bad.
    // As a workaround the icons are created with 64x64 and afterwards the pixmap is scaled to
    // 32x32. This workaround is only needed if pRatio is equal to 1.0
    //
    qreal pRatio = devicePixelRatio();
    bool isRatioOne = (pRatio == 1.0);
    qreal cursorSize = isRatioOne ? 64 : 32;
    qreal hotX = x;
    qreal hotY = y;
#if !defined(Q_OS_WIN32) && !defined(Q_OS_MACOS)
    if (qGuiApp->platformName() == QLatin1String("xcb")) {
        hotX *= pRatio;
        hotY *= pRatio;
    }
#endif

    QPixmap pointer = Gui::BitmapFactory().pixmapFromSvg(cursorName.toStdString().c_str(),
                                                         QSizeF{cursorSize, cursorSize},
                                                         colorMapping);
    if (isRatioOne) {
        pointer = pointer.scaled(32, 32);
    }
    setCursor(pointer, hotX, hotY, false);
}

void ToolHandler::setCursor(const QPixmap& p, int x, int y, bool autoScale)
{

    QWidget* cw = getCursorWidget();
    if (cw) {
        QCursor cursor;
        QPixmap p1(p);
        // TODO remove autoScale after all cursors are SVG-based
        if (autoScale) {
            qreal pRatio = devicePixelRatio();
            int newWidth = p.width() * pRatio;
            int newHeight = p.height() * pRatio;
            p1 = p1.scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            p1.setDevicePixelRatio(pRatio);
            qreal hotX = x;
            qreal hotY = y;
#if !defined(Q_OS_WIN32) && !defined(Q_OS_MACOS)
            if (qGuiApp->platformName() == QLatin1String("xcb")) {
                hotX *= pRatio;
                hotY *= pRatio;
            }
#endif
            cursor = QCursor(p1, hotX, hotY);
        }
        else {
            // already scaled
            cursor = QCursor(p1, x, y);
        }

        actCursor = cursor;
        actCursorPixmap = p1;

        setWidgetCursor(cursor);
    }
}

void ToolHandler::addCursorTail(std::vector<QPixmap>& pixmaps)
{
    // Create a pixmap that will contain icon and each autoconstraint icon
    QPixmap baseIcon = QPixmap(actCursorPixmap);
    baseIcon.setDevicePixelRatio(actCursorPixmap.devicePixelRatio());
    qreal pixelRatio = baseIcon.devicePixelRatio();
    // cursor size in device independent pixels
    qreal baseCursorWidth = baseIcon.width();
    qreal baseCursorHeight = baseIcon.height();

    int tailWidth = 0;
    for (auto const& p : pixmaps) {
        tailWidth += p.width();
    }

    int newIconWidth = baseCursorWidth + tailWidth;
    int newIconHeight = baseCursorHeight;

    QPixmap newIcon(newIconWidth, newIconHeight);
    newIcon.fill(Qt::transparent);

    QPainter qp;
    qp.begin(&newIcon);

    qp.drawPixmap(QPointF(0, 0),
                    baseIcon.scaled(baseCursorWidth * pixelRatio,
                                    baseCursorHeight * pixelRatio,
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation));

    // Iterate through pixmaps and them to the cursor pixmap
    qreal currentIconX = baseCursorWidth;
    qreal currentIconY;

    for (auto& icon : pixmaps) {
        currentIconY = baseCursorHeight - icon.height();
        qp.drawPixmap(QPointF(currentIconX, currentIconY), icon);
        currentIconX += icon.width();
    }

    qp.end();  // Finish painting

    // Create the new cursor with the icon.
    QPoint p = actCursor.hotSpot();
    newIcon.setDevicePixelRatio(pixelRatio);
    QCursor newCursor(newIcon, p.x(), p.y());
    applyCursor(newCursor);
}

void ToolHandler::updateCursor()
{
    auto cursorstring = getCrosshairCursorSVGName();

    if (cursorstring != QStringLiteral("None")) {
        setCrosshairCursor(cursorstring);
    }
}

std::list<InputHint> ToolHandler::getToolHints() const
{
    return {};
}

void ToolHandler::updateHint() const
{
    Gui::getMainWindow()->showHints(getToolHints());
}

void ToolHandler::applyCursor()
{
    applyCursor(actCursor);
}

void ToolHandler::applyCursor(QCursor& newCursor)
{
    setWidgetCursor(newCursor);
}

void ToolHandler::unsetCursor()
{
    setWidgetCursor(oldCursor);
}

qreal ToolHandler::devicePixelRatio()
{
    qreal pixelRatio = 1;

    QWidget* cw = getCursorWidget();
    if (cw) {
        pixelRatio = cw->devicePixelRatio();
    }
    return pixelRatio;
}

QWidget* ToolHandler::getCursorWidget()
{
    Gui::View3DInventorViewer* viewer = getViewer();
    if (viewer) {
        return viewer->getWidget();
    }
    return nullptr;
}

void ToolHandler::setWidgetCursor(QCursor cursor)
{
    QWidget* cw = getCursorWidget();
    if (cw) {
        cw->setCursor(cursor);
    }
}

Gui::View3DInventorViewer* ToolHandler::getViewer()
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom<Gui::View3DInventor>()) {
        return static_cast<Gui::View3DInventor*>(view)->getViewer();
    }
    return nullptr;
}
