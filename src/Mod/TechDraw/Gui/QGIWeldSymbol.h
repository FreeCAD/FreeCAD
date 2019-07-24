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

#ifndef DRAWINGGUI_QGRAPHICSITEMWELDSYMBOL_H
#define DRAWINGGUI_QGRAPHICSITEMWELDSYMBOL_H

#include <QObject>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QPainterPath>
#include <QColor>
#include <QFont>
#include <QPointF>

#include <Base/Vector3D.h>
#include "QGIView.h"

namespace TechDraw {
class DrawWeldSymbol;
class DrawWeldSymbol;
class DrawView;
}

namespace TechDrawGui
{
class QGIPrimPath;
class QGITile;
class QGIVertex;
class QGCustomText;

//*******************************************************************

class TechDrawGuiExport QGIWeldSymbol : public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 340};

    explicit QGIWeldSymbol(QGILeaderLine* myParent = nullptr,
                           TechDraw::DrawWeldSymbol* lead = nullptr);
    ~QGIWeldSymbol() = default;

    int type() const override { return Type;}
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = 0 ) override;
    virtual QRectF boundingRect() const override;
    virtual QPainterPath shape(void) const override;
    double getEdgeFuzz(void) const;

    virtual void drawBorder() override;
    virtual void updateView(bool update = false) override;

    virtual TechDraw::DrawWeldSymbol* getFeature(void);
    QPointF getTileOrigin(void);
    QPointF getKinkPoint(void);
    QPointF getTailPoint(void);
    bool isTextRightSide(void);

    virtual void setPrettyNormal();
    virtual void setPrettySel();
    virtual void setPrettyPre();

protected:
    virtual QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    virtual void draw() override;
    void drawTile(TechDraw::DrawTileWeld* dtw,
                  QGITile* tile);
    void drawAllAround(void);
    void drawProcessText(void);
    void drawFieldFlag();
    void removeDecorations();

protected:
    virtual QColor getNormalColor() override;
    double getPrefArrowSize();
    
    TechDraw::DrawWeldSymbol* m_weldFeat;
    TechDraw::DrawLeaderLine* m_leadFeat;

    QGILeaderLine* m_qgLead;
    std::vector<QGITile*> m_tiles;
    QGCustomText* m_tailText;
    QGIPrimPath* m_fieldFlag;
    QGIVertex* m_allAround;

    QFont m_font;

    bool m_blockDraw;    //prevent redraws while updating.
};

}

#endif // DRAWINGGUI_QGRAPHICSITEMWELDSYMBOL_H
