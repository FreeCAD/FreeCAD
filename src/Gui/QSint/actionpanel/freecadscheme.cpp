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
#include "winvistapanelscheme.h"
#include "winxppanelscheme.h"

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

    // set a QGroupBox to avoid that the OS style is used, see
    // https://github.com/FreeCAD/FreeCAD/issues/6102
    // the px values are taken from Behave-dark.qss, except the padding
    "QSint--ActionGroup QFrame[class='content'] QGroupBox {"
    "border: 1px solid #bbbbbb;"
    "border-radius: 3px;"
    "margin-top: 10px;"
    "padding: 2px;"
    "}"
    // since we set a custom frame we also need to set the title
    "QSint--ActionGroup QFrame[class='content'] QGroupBox::title {"
    "padding-left: 3px;"
    "top: -6px;"
    "left: 12px;"
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
#elif defined(Q_OS_MACOS)
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

    headerButtonFold = drawFoldIcon(p, true, false);
    headerButtonFoldOver = drawFoldIcon(p, true, true);
    headerButtonUnfold = drawFoldIcon(p, false, false);
    headerButtonUnfoldOver = drawFoldIcon(p, false, true);
    headerButtonSize = QSize(17,17);

    groupFoldSteps = 20;
    groupFoldDelay = 15;
    groupFoldEffect = NoFolding;
    groupFoldThaw = true;

    actionStyle = systemStyle(QApplication::palette());
}

QPixmap SystemPanelScheme::drawFoldIcon(const QPalette& palette, bool fold, bool hover) const
{
    QSize bSize = headerButtonSize;
    QImage img(bSize.width(), bSize.height(), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);

    painter.setRenderHint(QPainter::Antialiasing);

    qreal penWidth = bSize.width() / 14.0;
    qreal lef_X = 0.25 * bSize.width();
    qreal mid_X = 0.50 * bSize.width();
    qreal rig_X = 0.75 * bSize.width();
    qreal bot_Y = 0.40 * bSize.height();
    qreal top_Y = 0.64 * bSize.height();

    if (hover) {
        penWidth *= 1.8;
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(palette.color(QPalette::HighlightedText), penWidth));

    QPolygon chevron;
    if (fold) {
        // Upward
        chevron << QPoint(lef_X, top_Y)
                << QPoint(mid_X, bot_Y)
                << QPoint(rig_X, top_Y);
    } else {
        // Downward
        chevron << QPoint(lef_X, bot_Y)
                << QPoint(mid_X, top_Y)
                << QPoint(rig_X, bot_Y);
    }

    painter.drawPolyline(chevron);
    painter.end();
    return QPixmap::fromImage(img);
}


QString SystemPanelScheme::systemStyle(const QPalette& p) const
{
    QColor headerBackground = p.color(QPalette::Highlight);
    QColor headerLabelText = p.color(QPalette::HighlightedText);
    QColor headerLabelTextOver = p.color(QPalette::BrightText);
    QColor groupBorder = p.color(QPalette::Mid);
    QColor disabledActionText = p.color(QPalette::Disabled, QPalette::Text);
    QColor actionSelectedBg = p.color(QPalette::Active, QPalette::Light);
    QColor actionSelectedText = p.color(QPalette::Active, QPalette::ButtonText);
    QColor actionSelectedBorder = p.color(QPalette::Active, QPalette::Highlight);

    QString style = QString::fromLatin1(
        "QSint--ActionGroup QFrame[class='header'] {"
            "border: 1px solid transparent;"
            "background-color: %1;"
        "}"

        "QSint--ActionGroup QToolButton[class='header'] {"
            "text-align: left;"
            "color: %2;"
            "background-color: transparent;"
            "border: 1px solid transparent;"
            "font-weight: bold;"
        "}"

        "QSint--ActionGroup QToolButton[class='header']:hover {"
            "color: %3;"
        "}"

        "QSint--ActionGroup QFrame[class='content'] {"
            "border: 1px solid %4;"
        "}"

        "QSint--ActionGroup QFrame[class='content'][header='true'] {"
            "border-top: none;"
        "}"

        "QSint--ActionGroup QToolButton[class='action'] {"
            "background-color: transparent;"
            "border: 1px solid transparent;"
            "text-align: left;"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:!enabled {"
            "color: %5;"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:hover {"
            "text-decoration: underline;"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:focus {"
            "color: %7;"
            "border: 1px dotted %8;"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:on {"
            "background-color: %6;"
            "color: %7;"
        "}"
    ).arg(
        headerBackground.name(),
        headerLabelText.name(),
        headerLabelTextOver.name(),
        groupBorder.name(),
        disabledActionText.name(),
        actionSelectedBg.name(),
        actionSelectedText.name(),
        actionSelectedBorder.name()
    );

    return style;
}

}
