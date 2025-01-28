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

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QHash>

namespace QSint
{

const QString SystemPanelScheme::minimumStyle = QString::fromLatin1(
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
);

FreeCADPanelScheme::FreeCADPanelScheme()
    : ActionPanelScheme()
{
    ActionPanelScheme* panelStyle = SystemPanelScheme::defaultScheme();

    actionStyle = panelStyle->actionStyle;
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

    actionStyle = SystemPanelScheme::minimumStyle;
}

void FreeCADPanelScheme::restoreActionStyle()
{
    headerButtonFold = builtinFold;
    headerButtonFoldOver = builtinFoldOver;
    headerButtonUnfold = builtinUnfold;
    headerButtonUnfoldOver = builtinUnfoldOver;

    actionStyle = builtinScheme;
}

SystemPanelScheme::SystemPanelScheme()
{
    headerSize = 25;
    headerAnimation = true;

    QPalette p = QApplication::palette();

    headerButtonFold = drawFoldIcon(p, true, false);
    headerButtonFoldOver = drawFoldIcon(p, true, true);
    headerButtonUnfold = drawFoldIcon(p, false, false);
    headerButtonUnfoldOver = drawFoldIcon(p, false, true);
    headerButtonSize = QSize(17, 17);

    groupFoldSteps = 20;
    groupFoldDelay = 15;
    groupFoldEffect = NoFolding;
    groupFoldThaw = true;

    actionStyle = systemStyle(p);
}

// Draws fold/unfold icons based on the palette
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
        // Upward chevron
        chevron << QPoint(lef_X, top_Y)
                << QPoint(mid_X, bot_Y)
                << QPoint(rig_X, top_Y);
    } else {
        // Downward chevron
        chevron << QPoint(lef_X, bot_Y)
                << QPoint(mid_X, top_Y)
                << QPoint(rig_X, bot_Y);
    }

    painter.drawPolyline(chevron);
    return QPixmap::fromImage(img);
}

QString SystemPanelScheme::systemStyle(const QPalette& p) const
{
    QString headerBackground = p.color(QPalette::Highlight).name();
    QString headerLabelText = p.color(QPalette::HighlightedText).name();
    QString headerLabelTextOver = p.color(QPalette::BrightText).name();
    QString groupBorder = p.color(QPalette::Mid).name();
    QString disabledActionText = p.color(QPalette::Disabled, QPalette::Text).name();
    QString actionSelectedBg = p.color(QPalette::Active, QPalette::Light).name();
    QString actionSelectedText = p.color(QPalette::Active, QPalette::ButtonText).name();
    QString actionSelectedBorder = p.color(QPalette::Active, QPalette::Highlight).name();
    QString panelBackground = p.color(QPalette::Window).name();
    QString groupBackground = p.color(QPalette::Button).name();

    QHash<QString, QString> replacements;
    replacements["headerBackground"] = headerBackground;
    replacements["headerLabelText"] = headerLabelText;
    replacements["headerLabelTextOver"] = headerLabelTextOver;
    replacements["groupBorder"] = groupBorder;
    replacements["disabledActionText"] = disabledActionText;
    replacements["actionSelectedBg"] = actionSelectedBg;
    replacements["actionSelectedText"] = actionSelectedText;
    replacements["actionSelectedBorder"] = actionSelectedBorder;
    replacements["panelBackground"] = panelBackground;
    replacements["groupBackground"] = groupBackground;

    QString style = minimumStyle;

    style += QString::fromLatin1(
        "QFrame[class='panel'] {"
            "background-color: {panelBackground};"
        "}"

        "QSint--ActionGroup QFrame[class='header'] {"
            "border: 1px solid {headerBackground};"
            "background-color: {headerBackground};"
        "}"

        "QSint--ActionGroup QToolButton[class='header'] {"
            "color: {headerLabelText};"
        "}"

        "QSint--ActionGroup QToolButton[class='header']:hover {"
            "color: {headerLabelTextOver};"
        "}"

        "QSint--ActionGroup QFrame[class='content'] {"
            "border: 1px solid {groupBorder};"
            "background-color: {groupBackground};"
        "}"

        "QSint--ActionGroup QFrame[class='content'][header='true'] {"
            "border-top: none;"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:!enabled {"
            "color: {disabledActionText};"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:focus {"
            "color: {actionSelectedText};"
            "border: 1px dotted {actionSelectedBorder};"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:on {"
            "background-color: {actionSelectedBg};"
            "color: {actionSelectedText};"
        "}"
    );

    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
        style.replace("{" + it.key() + "}", it.value());
    }

    return style;
}

} // namespace QSint
