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
#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QFont>
#include <QGraphicsItem>
#include <QPen>
#include <QStyleOptionGraphicsItem>

#include "QGIView.h"
#include "QGIUserTypes.h"


namespace TechDraw
{
class DrawRichAnno;
class DrawLeaderLine;
}  // namespace TechDraw

namespace TechDrawGui
{
class QGIPrimPath;
class QGIArrow;
class QGEPath;
class QGMText;
class QGCustomText;
class QGCustomRect;


//*******************************************************************

class TechDrawGuiExport QGIRichAnno: public QGIView
{
    Q_OBJECT

public:
    enum {Type = UserType::QGIRichAnno};

    enum class ResizeHandle
    {
        NoHandle,
        LeftHandle,
        RightHandle
        // Future: TopHandle, BottomHandle, CornerHandles
    };

    explicit QGIRichAnno();
    ~QGIRichAnno() override = default;

    int type() const override
    {
        return Type;
    }
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;
    QRectF boundingRect() const override;

    void updateView(bool update = false) override;

    void setTextItem();

    virtual TechDraw::DrawRichAnno* getFeature();
    QPen rectPen() const;

    void setExportingPdf(bool b)
    {
        m_isExportingPdf = b;
    }
    bool getExportingPdf() const
    {
        return m_isExportingPdf;
    }
    void setExportingSvg(bool b)
    {
        m_isExportingSvg = b;
    }
    bool getExportingSvg() const
    {
        return m_isExportingSvg;
    }

    void setEditMode(bool enable);
    QTextDocument* document() const;
    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor& cursor);
    void updateLayout();

    void refocusAnnotation();

    Q_SIGNALS:
    void widthChanged();
    void textChanged();
    void selectionChanged();
    void positionChanged(const QPointF& scenePos);

protected:
    void draw() override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void setLineSpacing(int lineSpacing);
    QFont prefFont(void);

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    bool m_isExportingPdf;
    bool m_isExportingSvg;
    QGCustomText* m_text;
    QGCustomRect* m_rect;

    // For resizing
    ResizeHandle m_currentResizeHandle;
    bool m_isResizing;
    bool m_isDraggingMidResize;  // True if mouse has moved significantly after press during resize
    bool m_transactionOpen;      // True if a Gui::Command transaction is open
    QPointF m_dragStartMouseScenePos;
    QPointF m_initialItemScenePos;   // Scene pos of QGIRichAnno item (center)
    double m_initialTextWidthScene;  // Scene units, from MaxWidth property

    static const double HandleInteractionMargin;  // Margin for grabbing handles (scene units)
    static const double MinTextWidthDocument;     // Minimum resizable width (document units)

    bool m_isEditing;
    double m_textScaleFactor;
    double m_lastGoodWidthScene;

private Q_SLOTS:
    void onContentsChanged();
};

}