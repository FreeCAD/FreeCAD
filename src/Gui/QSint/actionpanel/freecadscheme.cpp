/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "freecadscheme.h"
#include "macpanelscheme.h"
#include "winxppanelscheme.h"
#include "winvistapanelscheme.h"
#include <QImage>
#include <QPainter>
#include <QPalette>


namespace QSint
{


const char* ActionPaneFreeCAD =

    "QFrame[class='panel'] {"
        "background-color:qlineargradient(x1:1, y1:0.3, x2:1, y2:0, stop:0 rgb(51,51,101), stop:1 rgb(171,171,193));"
    "}"

    "QSint--ActionGroup QFrame[class='header'] {"
        "border: 1px solid #ffffff;"
        "border-top-left-radius: 4px;"
        "border-top-right-radius: 4px;"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #ffffff, stop: 1 #c6d3f7);"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "color: #215dc6;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "font-weight: bold;"
    "}"

    "QSint--ActionGroup QToolButton[class='header']:hover {"
        "color: #428eff;"
    "}"

    "QSint--ActionGroup QFrame[class='content'] {"
        "background-color: #d6dff7;"
        "border: 1px solid #ffffff;"
    "}"

    "QSint--ActionGroup QFrame[class='content'][header='true'] {"
        "border-top: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: #215dc6;"
        "text-align: left;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:!enabled {"
        "color: #999999;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "color: #428eff;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"
    ;

FreeCADPanelScheme::FreeCADPanelScheme() : ActionPanelScheme()
{
#if defined(Q_OS_WIN32)
    ActionPanelScheme* panelStyle = WinXPPanelScheme2::defaultScheme();

    actionStyle = QString(ActionPaneFreeCAD);
#elif defined(Q_OS_MAC)
    ActionPanelScheme* panelStyle = MacPanelScheme::defaultScheme();

    actionStyle = panelStyle->actionStyle;
#else
    ActionPanelScheme* panelStyle = ActionPanelScheme::defaultScheme();

    actionStyle = panelStyle->actionStyle;
#endif

    builtinScheme = actionStyle;

    headerSize = panelStyle->headerSize;
    headerAnimation = panelStyle->headerAnimation;
    headerButtonFold = panelStyle->headerButtonFold;
    headerButtonFoldOver = panelStyle->headerButtonFoldOver;
    headerButtonUnfold = panelStyle->headerButtonUnfold;
    headerButtonUnfoldOver = panelStyle->headerButtonUnfoldOver;
    headerButtonSize = panelStyle->headerButtonSize;

    groupFoldSteps = panelStyle->groupFoldSteps;
    groupFoldDelay = panelStyle->groupFoldDelay;
    groupFoldEffect = panelStyle->groupFoldEffect;
    groupFoldThaw = panelStyle->groupFoldThaw;
}

void FreeCADPanelScheme::clearActionStyle()
{
    actionStyle.clear();
}

void FreeCADPanelScheme::restoreActionStyle()
{
    actionStyle = builtinScheme;
}

QPixmap FreeCADPanelScheme::drawFoldIcon(const QPalette& p) const
{
    QImage img(17,17,QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    QPainter painter;
    painter.begin(&img);
    painter.setBrush(p.window());
    painter.drawEllipse(2,2,13,13);
    painter.setPen(p.color(QPalette::Base));
    painter.drawEllipse(2,2,13,13);
    painter.setPen(p.color(QPalette::Highlight));
    painter.drawLine(QLine(5,7,8,4));
    painter.drawLine(QLine(6,7,8,5));
    painter.drawLine(QLine(8,4,11,7));
    painter.drawLine(QLine(8,5,10,7));
    painter.drawLine(QLine(5,11,8,8));
    painter.drawLine(QLine(6,11,8,9));
    painter.drawLine(QLine(8,8,11,11));
    painter.drawLine(QLine(9,8,10,11));
    painter.end();
    return QPixmap::fromImage(img);
}

}
#if 0
iisFreeCADTaskPanelScheme::iisFreeCADTaskPanelScheme(QObject *parent)
	: iisTaskPanelScheme(parent)
{
#ifdef Q_OS_WIN32
#else
    QPalette p = QApplication::palette();
    QLinearGradient panelBackgroundGrd(0,0, 0,300);
    panelBackgroundGrd.setColorAt(1, p.color(QPalette::Dark));
    panelBackgroundGrd.setColorAt(0, p.color(QPalette::Midlight));
    panelBackground = panelBackgroundGrd;

    QLinearGradient headerBackgroundGrd(0,0,0,100);
    headerBackgroundGrd.setColorAt(0, p.color(QPalette::Highlight));
    headerBackgroundGrd.setColorAt(1, p.color(QPalette::Highlight).lighter());
    headerBackground = headerBackgroundGrd;

    headerLabelScheme.text = p.color(QPalette::HighlightedText);
    headerLabelScheme.textOver = p.color(QPalette::BrightText);
    headerLabelScheme.iconSize = 22;

    headerButtonSize = QSize(17,17);
    QPalette p2 = p;
    p2.setColor(QPalette::Highlight,p2.color(QPalette::Highlight).lighter());
    QPixmap px1 = drawFoldIcon(p);
    QPixmap px2 = drawFoldIcon(p2);
    headerButtonFold = px1;
    headerButtonFoldOver = px2;
    QTransform mat;
    mat.rotate(180.0);
    headerButtonUnfold = px1.transformed(mat);
    headerButtonUnfoldOver = px2.transformed(mat);

    groupBackground = p.window();
    groupBorder = p.color(QPalette::Window);

    taskLabelScheme.text = p.color(QPalette::Text);
    taskLabelScheme.textOver = p.color(QPalette::Highlight);
#endif
}
#endif
