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
#include <QObject>
#include <QPen>
#include <QFont>
#include <QColor>
#include <QCursor>
#include <QPointF>

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
class QGVPage;
class QGCustomBorder;
class QGCustomLabel;
class QGCustomText;
class QGICaption;
class MDIViewPage;
class QGIViewClip;
class QGCustomImage;
class QGTracker;
class QGIVertex;

class TechDrawGuiExport  QGIView : public QObject, public QGraphicsItemGroup
{
    Q_OBJECT
public:
    QGIView();
    virtual ~QGIView();

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
    double getScale(void);

    virtual bool getFrameState(void);
    virtual void toggleCache(bool state);
    virtual void updateView(bool update = false);
    virtual void drawBorder(void);
    virtual void isVisible(bool state);
    virtual bool isVisible(void);
    virtual void draw(void);
    virtual void drawCaption(void);
    virtual void rotateView(void);
    void makeMark(double x, double y, QColor c = Qt::red);
    void makeMark(Base::Vector3d v, QColor c = Qt::red);
    void makeMark(QPointF p, QColor c = Qt::red);


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
    static QGVPage* getGraphicsView(TechDraw::DrawView* dv);
    static double calculateFontPointSizeF(const QGraphicsItem *graphicsItem, double sizeInMillimetres);
    static const double DefaultFontSizeInMM;

    MDIViewPage* getMDIViewPage(void) const;

    // Mouse handling
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    boost::signals2::signal<void (QGIView*, QPointF)> signalSelectPoint;

public Q_SLOTS:
    virtual void onSourceChange(TechDraw::DrawView* newParent);

protected:
    QGIView* getQGIVByName(std::string name);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    // Mouse handling
/*    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;*/
    // Preselection events:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    virtual QRectF customChildrenBoundingRect(void) const;
    void dumpRect(char* text, QRectF r);

    QString getPrefFont(void);
    double getPrefFontSize(void);
    double calculateFontPointSizeF(double sizeInMillimetres) const;

    Base::Reference<ParameterGrp> getParmGroupCol(void);

    TechDraw::DrawView *viewObj;
    std::string viewName;

    QHash<QString, QGraphicsItem*> alignHash;
    //std::string alignMode;
    //QGIView* alignAnchor;
    bool m_locked;
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
