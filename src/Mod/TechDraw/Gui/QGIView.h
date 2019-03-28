/***************************************************************************
 *   Copyright (c) 2012-2013 Luke Parry <l.parry@warwick.ac.uk>            *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEW_H
#define DRAWINGGUI_QGRAPHICSITEMVIEW_H

#include <QGraphicsItemGroup>
#include <QPen>
#include <QFont>

#include <App/DocumentObject.h>
#include <Base/Parameter.h>
#include <Gui/ViewProvider.h>

#include <Mod/TechDraw/App/DrawView.h>


QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace TechDrawGui
{
class QGCustomBorder;
class QGCustomLabel;
class QGCustomText;
class QGICaption;
class MDIViewPage;
class QGIViewClip;
class QGCustomImage;

class TechDrawGuiExport  QGIView : public QGraphicsItemGroup
{
public:
    QGIView();
    virtual ~QGIView() = default;

    enum {Type = QGraphicsItem::UserType + 101};
    int type() const override { return Type;}
    virtual QRectF boundingRect() const override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr ) override;

    const char *      getViewName() const;
    const std::string getViewNameAsString() const;
    void setViewFeature(TechDraw::DrawView *obj);
    TechDraw::DrawView * getViewObject() const;

    virtual void toggleBorder(bool state = true);
    virtual void toggleCache(bool state);
    virtual void updateView(bool update = false);
    virtual void drawBorder(void);
    virtual void isVisible(bool state) { m_visibility = state; }
    virtual bool isVisible(void) {return m_visibility;}
    virtual void draw(void);
    virtual void drawCaption(void);
    virtual void rotateView(void);
    void makeMark(double x, double y);
    void makeMark(Base::Vector3d v);


    /** Methods to ensure that Y-Coordinates are orientated correctly.
     * @{ */
    void setPosition(qreal x, qreal y);
    inline qreal getY() { return y() * -1; }
    bool isInnerView() { return m_innerView; }
    void isInnerView(bool state) { m_innerView = state; }
    double getYInClip(double y);
    /** @} */
    QGIViewClip* getClipGroup(void);


    void alignTo(QGraphicsItem*, const QString &alignment);
    void setLocked(bool b) { m_locked = b; }

    virtual QColor getNormalColor(void);
    virtual QColor getPreColor(void);
    virtual QColor getSelectColor(void);
    
    static Gui::ViewProvider* getViewProvider(App::DocumentObject* obj);
    MDIViewPage* getMDIViewPage(void) const;

protected:
    QGIView* getQGIVByName(std::string name);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    // Mouse handling
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    // Preselection events:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    virtual QRectF customChildrenBoundingRect(void) const;
    void dumpRect(char* text, QRectF r);

    QString getPrefFont(void);
    double getPrefFontSize(void);
    Base::Reference<ParameterGrp> getParmGroupCol(void);

    TechDraw::DrawView *viewObj;
    std::string viewName;

    QHash<QString, QGraphicsItem*> alignHash;
    //std::string alignMode;
    //QGIView* alignAnchor;
    bool m_locked;
    bool borderVisible;
    bool m_visibility;
    bool m_innerView;                                                  //View is inside another View

    QPen m_pen;
    QBrush m_brush;
    QColor m_colCurrent;
    QColor m_colNormal;
    QColor m_colPre;
    QColor m_colSel;
    QFont m_font;
    QGCustomLabel* m_label;
    QGCustomBorder* m_border;
    QGICaption* m_caption;
    QGCustomImage* m_lock;
    QPen m_decorPen;
    double m_lockWidth;
    double m_lockHeight;

};

} // namespace

#endif // DRAWINGGUI_QGRAPHICSITEMVIEW_H
