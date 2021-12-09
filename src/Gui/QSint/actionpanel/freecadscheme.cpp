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
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPalette>


namespace QSint
{


const char* ActionPanelFreeCAD =

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
        "text-decoration: underline;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"
    ;

const char* MinimumActionPanelFreeCAD =

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "font-weight: bold;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "text-align: left;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "text-decoration: underline;"
    "}"
    ;

FreeCADPanelScheme::FreeCADPanelScheme() : ActionPanelScheme()
{
#if defined(Q_OS_WIN32)
    ActionPanelScheme* panelStyle = WinXPPanelScheme2::defaultScheme();

    actionStyle = QString(ActionPanelFreeCAD);
#elif defined(Q_OS_MAC)
    ActionPanelScheme* panelStyle = MacPanelScheme::defaultScheme();

    actionStyle = panelStyle->actionStyle;
#elif defined(Q_OS_LINUX)
    ActionPanelScheme* panelStyle = SystemPanelScheme::defaultScheme();

    actionStyle = panelStyle->actionStyle;
#else
    ActionPanelScheme* panelStyle = ActionPanelScheme::defaultScheme();

    actionStyle = panelStyle->actionStyle;
#endif

    builtinScheme = actionStyle;
    minimumStyle = QString(MinimumActionPanelFreeCAD);

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


    builtinFold = headerButtonFold;
    builtinFoldOver = headerButtonFoldOver;
    builtinUnfold = headerButtonUnfold;
    builtinUnfoldOver = headerButtonUnfoldOver;
}

void FreeCADPanelScheme::clearActionStyle()
{
    headerButtonFold = QPixmap();
    headerButtonFoldOver = QPixmap();
    headerButtonUnfold = QPixmap();
    headerButtonUnfoldOver = QPixmap();

    actionStyle = minimumStyle;
}

void FreeCADPanelScheme::restoreActionStyle()
{
    headerButtonFold = builtinFold;
    headerButtonFoldOver = builtinFoldOver;
    headerButtonUnfold = builtinUnfold;
    headerButtonUnfoldOver = builtinUnfoldOver;

    actionStyle = builtinScheme;
}

// -----------------------------------------------------

SystemPanelScheme::SystemPanelScheme()
{
    headerSize = 25;
    headerAnimation = true;

    QPalette p = QApplication::palette();
    QPalette p2 = p;
    p2.setColor(QPalette::Highlight,p2.color(QPalette::Highlight).lighter());

    headerButtonFold = drawFoldIcon(p, true);
    headerButtonFoldOver = drawFoldIcon(p2, true);
    headerButtonUnfold = drawFoldIcon(p, false);
    headerButtonUnfoldOver = drawFoldIcon(p2, false);
    headerButtonSize = QSize(17,17);

    groupFoldSteps = 20;
    groupFoldDelay = 15;
    groupFoldEffect = NoFolding;
    groupFoldThaw = true;

    actionStyle = systemStyle(QApplication::palette());
}

/*!
  \code
    QPalette p = QApplication::palette();
    QPalette p2 = p;
    p2.setColor(QPalette::Highlight,p2.color(QPalette::Highlight).lighter());
    headerButtonFold = drawFoldIcon(p, true);
    headerButtonFoldOver = drawFoldIcon(p2, true);
    headerButtonUnfold = drawFoldIcon(p, false);
    headerButtonUnfoldOver = drawFoldIcon(p2, false);
  \endcode
 */
QPixmap SystemPanelScheme::drawFoldIcon(const QPalette& p, bool fold) const
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

    if (!fold) {
        QTransform mat;
        mat.rotate(180.0);
        img = img.transformed(mat);
    }
    return QPixmap::fromImage(img);
}

QString SystemPanelScheme::systemStyle(const QPalette& p) const
{
  QColor panelBackground1 = p.color(QPalette::Dark);
  QColor panelBackground2 = p.color(QPalette::Midlight);

  QColor headerBackground1 = p.color(QPalette::Highlight);
  QColor headerBackground2 = p.color(QPalette::Highlight).lighter();

  QColor headerLabelText = p.color(QPalette::HighlightedText);
  QColor headerLabelTextOver = p.color(QPalette::BrightText);

  QColor groupBackground = p.window().color();
  QColor groupBorder = p.color(QPalette::Window);

  QColor taskLabelText = p.color(QPalette::Text);
  QColor taskLabelTextOver = p.color(QPalette::Highlight);

  QString style = QString::fromLatin1(
    "QFrame[class='panel'] {"
        "background-color:qlineargradient(x1:1, y1:0.3, x2:1, y2:0, stop:0 %1, stop:1 %2);"
    "}"

    "QSint--ActionGroup QFrame[class='header'] {"
        "border: 1px solid #ffffff;"                                // todo
        "border-top-left-radius: 4px;"
        "border-top-right-radius: 4px;"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 %3, stop: 1 %4);"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "color: %5;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "font-weight: bold;"
    "}"

    "QSint--ActionGroup QToolButton[class='header']:hover {"
        "color: %6;"
    "}"

    "QSint--ActionGroup QFrame[class='content'] {"
        "background-color: %7;"
        "border: 1px solid %8;"
    "}"

    "QSint--ActionGroup QFrame[class='content'][header='true'] {"
        "border-top: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: %9;"
        "text-align: left;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:!enabled {"
        "color: #999999;"                                           // todo
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "color: %10;"
        "text-decoration: underline;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"                                // todo
        "color: #006600;"                                           // todo
    "}"
  )
          .arg(panelBackground1.name(),
               panelBackground2.name(),
               headerBackground1.name(),
               headerBackground2.name(),
               headerLabelText.name(),
               headerLabelTextOver.name(),
               groupBackground.name(),
               groupBorder.name(),
               taskLabelText.name())
           .arg(taskLabelTextOver.name())
  ;

  return style;
}

}
