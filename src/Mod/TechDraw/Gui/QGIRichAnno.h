/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAWGUI_QGIRICHANNO_H
#define TECHDRAWGUI_QGIRICHANNO_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QFont>
#include <QGraphicsItem>
#include <QPen>
#include <QStyleOptionGraphicsItem>

#include "QGIView.h"


namespace TechDraw {
class DrawRichAnno;
class DrawLeaderLine;
}

namespace TechDrawGui
{
class QGIPrimPath;
class QGIArrow;
class QGEPath;
class QGMText;
class QGCustomText;
class QGCustomRect;


//*******************************************************************

class TechDrawGuiExport QGIRichAnno : public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 233};

    explicit QGIRichAnno();
    ~QGIRichAnno() override = default;

    int type() const override { return Type;}
    void paint( QPainter * painter,
                const QStyleOptionGraphicsItem * option,
                QWidget * widget = nullptr ) override;
    QRectF boundingRect() const override;

    void drawBorder() override;
    void updateView(bool update = false) override;

    void setTextItem();

    virtual TechDraw::DrawRichAnno* getFeature();
    QPen rectPen() const;

    void setExportingPdf(bool b) { m_isExportingPdf = b; }
    bool getExportingPdf() { return m_isExportingPdf; }
    void setExportingSvg(bool b) { m_isExportingSvg = b; }
    bool getExportingSvg() { return m_isExportingSvg; }

protected:
    void draw() override;
    void setLineSpacing(int lineSpacing);
    QFont prefFont(void);

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    bool m_isExportingPdf;
    bool m_isExportingSvg;
    QGCustomText* m_text;
    bool m_hasHover;
    QGCustomRect* m_rect;

};

}

#endif // TECHDRAWGUI_QGIRICHANNO_H
