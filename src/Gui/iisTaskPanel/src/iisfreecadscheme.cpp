/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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


#include "iisfreecadscheme.h"
#include <QApplication>
#include <QStyle>


iisFreeCADTaskPanelScheme* iisFreeCADTaskPanelScheme::myDefaultXPScheme = 0;

iisFreeCADTaskPanelScheme::iisFreeCADTaskPanelScheme(QObject *parent)
	: iisTaskPanelScheme(parent)
{
#ifdef Q_OS_WIN32
    QLinearGradient panelBackgroundGrd(0,0, 0,300);
    panelBackgroundGrd.setColorAt(1, QColor(51,51,101));
    panelBackgroundGrd.setColorAt(0, QColor(171,171,193));
    panelBackground = panelBackgroundGrd;

    QLinearGradient headerBackgroundGrd(0,0, 300,0);
    headerBackgroundGrd.setColorAt(0, 0xffffff);
    headerBackgroundGrd.setColorAt(1, 0xc6d3f7);
    headerBackground = headerBackgroundGrd;

    headerBorder = QPen(Qt::NoPen);
    headerSize = 25;
    headerAnimation = false;

    headerLabelScheme.text = QColor(0x215dc6);
    headerLabelScheme.textOver = QColor(0x428eff);
    headerLabelScheme.iconSize = 22;

    headerButtonFold = QPixmap(":/Resources/headerButtonFold_XPBlue2.png");
    headerButtonFoldOver = QPixmap(":/Resources/headerButtonFoldOver_XPBlue2.png");
    headerButtonUnfold = QPixmap(":/Resources/headerButtonUnfold_XPBlue2.png");
    headerButtonUnfoldOver = QPixmap(":/Resources/headerButtonUnfoldOver_XPBlue2.png");
    headerButtonSize = QSize(17,17);

    groupBackground = QBrush(0xd6dff7);
    groupBorder = QColor(0xffffff);

    taskLabelScheme.text = QColor(0x215dc6);
    taskLabelScheme.textOver = QColor(0x428eff);
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

    headerBorder = QPen(Qt::NoPen);
    headerSize = 25;
    headerAnimation = false;

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

iisFreeCADTaskPanelScheme::~iisFreeCADTaskPanelScheme()
{

}

iisTaskPanelScheme* iisFreeCADTaskPanelScheme::defaultScheme()	
{ 
	if (!myDefaultXPScheme)
		myDefaultXPScheme = new iisFreeCADTaskPanelScheme();

	return myDefaultXPScheme; 
}

QPixmap iisFreeCADTaskPanelScheme::drawFoldIcon(const QPalette& p) const
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
