/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEWDIMENSION_H
#define DRAWINGGUI_QGRAPHICSITEMVIEWDIMENSION_H

#include <QObject>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QColor>
#include <QFont>
#include <Base/Vector3D.h>
#include "Rez.h"
#include "QGIView.h"
#include "QGCustomText.h"

namespace TechDraw {
class DrawViewDimension;
}

namespace TechDraw {
class BaseGeom;
class AOC;
}

namespace TechDrawGui
{
class QGIArrow;
class QGIDimLines;
class QGIViewDimension;
class ViewProviderDimension;

class QGIDatumLabel : public QGraphicsObject
{
Q_OBJECT

public:
    QGIDatumLabel();
    virtual ~QGIDatumLabel() = default;

    enum {Type = QGraphicsItem::UserType + 107};
    int type() const override { return Type;}

    virtual QRectF boundingRect() const override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr ) override;
    void setLabelCenter();
    void setPosFromCenter(const double &xCenter, const double &yCenter);
    double X() const { return posX; }
    double Y() const { return posY; }              //minus posY?
    
    void setFont(QFont f);
    QFont getFont(void) { return m_dimText->font(); }
    void setDimString(QString t);
    void setDimString(QString t, qreal maxWidth);
    void setUnitString(QString t);
    void setTolString();
    void setPrettySel(void);
    void setPrettyPre(void);
    void setPrettyNormal(void);
    void setColor(QColor c);

    bool verticalSep;
    std::vector<int> seps;

    QGCustomText* getDimText(void) { return m_dimText; }
    void setDimText(QGCustomText* newText) { m_dimText = newText; }
    QGCustomText* getTolTextOver(void) { return m_tolTextOver; }
    void setTolTextOver(QGCustomText* newTol) { m_tolTextOver = newTol; }
    QGCustomText* getTolTextUnder(void) { return m_tolTextUnder; }
    void setTolTextUnder(QGCustomText* newTol) { m_tolTextOver = newTol; }

    double getTolAdjust(void);
/*    bool hasHover;*/

    bool isFramed(void) { return m_isFramed; }
    void setFramed(bool framed) { m_isFramed = framed; }

    double getLineWidth(void) { return m_lineWidth; }
    void setLineWidth(double lineWidth) { m_lineWidth = lineWidth; }

Q_SIGNALS:
    void setPretty(int state);
    void dragging(bool);
    void hover(bool state);
    void selected(bool state);
    void dragFinished();

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
//    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event) override;

    QGCustomText* m_dimText;
    QGCustomText* m_tolTextOver;
    QGCustomText* m_tolTextUnder;
    QGCustomText* m_unitText;
    int getPrecision(void);
    QColor m_colNormal;
    bool m_ctrl;

    double posX;
    double posY;

    bool m_isFramed;
    double m_lineWidth;
    
private:
};

//*******************************************************************

class TechDrawGuiExport QGIViewDimension : public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 106};

    explicit QGIViewDimension();
    ~QGIViewDimension() = default;

    void setViewPartFeature(TechDraw::DrawViewDimension *obj);
    int type() const override { return Type;}
    virtual QRectF boundingRect() const override;
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = 0 ) override;

    virtual void drawBorder() override;
    virtual void updateView(bool update = false) override;
    virtual QColor prefNormalColor(void);
    QString getLabelText(void);
    void setPrettyPre(void);
    void setPrettySel(void);
    void setPrettyNormal(void);

    virtual void setGroupSelection(bool b) override;
    virtual QGIDatumLabel* getDatumLabel(void) { return datumLabel; }

    void setNormalColorAll(void);

public Q_SLOTS:
    void onPrettyChanged(int state);
    void datumLabelDragged(bool ctrl);
    void datumLabelDragFinished(void);
    void select(bool state);
    void hover(bool state);
    void updateDim();

protected:

    static double getAnglePlacementFactor(double testAngle, double endAngle, double startRotation);
    static int compareAngleStraightness(double straightAngle, double leftAngle, double rightAngle,
                                        double leftStrikeFactor, double rightStrikeFactor);

    static double getIsoStandardLinePlacement(double labelAngle);
    Base::Vector2d getIsoRefOutsetPoint(const Base::BoundBox2d &labelRectangle, bool right) const;
    Base::Vector2d getIsoRefJointPoint(const Base::BoundBox2d &labelRectangle, bool right) const;
    Base::Vector2d getAsmeRefOutsetPoint(const Base::BoundBox2d &labelRectangle, bool right) const;
    Base::Vector2d getAsmeRefJointPoint(const Base::BoundBox2d &labelRectangle, bool right) const;

    static Base::Vector2d computePerpendicularIntersection(const Base::Vector2d &linePoint,
                              const Base::Vector2d &perpendicularPoint, double lineAngle);
    static Base::Vector2d computeExtensionLinePoints(const Base::Vector2d &originPoint,
                              const Base::Vector2d &linePoint, double hintAngle,
                              double overhangSize, double gapSize, Base::Vector2d &startPoint);
    static double computeLineAndLabelAngles(const Base::Vector2d &rotationCenter, const Base::Vector2d &labelCenter,
                                            double lineLabelDistance, double &lineAngle, double &labelAngle);
    static double computeLineStrikeFactor(const Base::BoundBox2d &labelRectangle, const Base::Vector2d &lineOrigin,
                                          double lineAngle, const std::vector<std::pair<double, bool>> &drawMarking);
    static double computeArcStrikeFactor(const Base::BoundBox2d &labelRectangle, const Base::Vector2d &arcCenter,
                                         double arcRadius, const std::vector<std::pair<double, bool>> &drawMarking);

    static double normalizeStartPosition(double &startPosition, double &lineAngle);
    static double normalizeStartRotation(double &startRotation);
    bool constructDimensionLine(const Base::Vector2d &targetPoint, double lineAngle,
                                double startPosition, double jointPosition, const Base::BoundBox2d &labelRectangle,
                                int arrowCount, int standardStyle, bool flipArrows,
                                std::vector<std::pair<double, bool>> &outputMarking) const;
    bool constructDimensionArc(const Base::Vector2d &arcCenter, double arcRadius, double endAngle,
                               double startRotation, double handednessFactor, double jointRotation,
                               const Base::BoundBox2d &labelRectangle, int arrowCount, int standardStyle,
                               bool flipArrows, std::vector<std::pair<double, bool>> &outputMarking) const;

    void draw() override;

    void resetArrows(void) const;
    void drawArrows(int count, const Base::Vector2d positions[], double angles[], bool flipped) const;

    void drawSingleLine(QPainterPath &painterPath, const Base::Vector2d &lineOrigin, double lineAngle,
                        double startPosition, double endPosition) const;
    void drawMultiLine(QPainterPath &painterPath, const Base::Vector2d &lineOrigin, double lineAngle,
                       const std::vector<std::pair<double, bool>> &drawMarking) const;
    void drawSingleArc(QPainterPath &painterPath, const Base::Vector2d &arcCenter, double arcRadius,
                       double startAngle, double endAngle) const;
    void drawMultiArc(QPainterPath &painterPath, const Base::Vector2d &arcCenter, double arcRadius,
                      const std::vector<std::pair<double, bool>> &drawMarking) const;

    void drawDimensionLine(QPainterPath &painterPath, const Base::Vector2d &targetPoint, double lineAngle,
                           double startPosition, double jointPosition, const Base::BoundBox2d &labelRectangle,
                           int arrowCount, int standardStyle, bool flipArrows) const;
    void drawDimensionArc(QPainterPath &painterPath, const Base::Vector2d &arcCenter, double arcRadius,
                          double endAngle, double startRotation, double jointAngle,
                          const Base::BoundBox2d &labelRectangle, int arrowCount,
                          int standardStyle, bool flipArrows) const;

    void drawDistanceExecutive(const Base::Vector2d &startPoint, const Base::Vector2d &endPoint, double lineAngle,
             const Base::BoundBox2d &labelRectangle, int standardStyle, int renderExtent, bool flipArrows) const;
    void drawRadiusExecutive(const Base::Vector2d &centerPoint, const Base::Vector2d &midPoint, double radius,
                             double endAngle, double startRotation, const Base::BoundBox2d &labelRectangle,
                             double centerOverhang, int standardStyle, int renderExtent, bool flipArrow) const;

    void drawDistance(TechDraw::DrawViewDimension *dimension, ViewProviderDimension *viewProvider) const;
    void drawRadius(TechDraw::DrawViewDimension *dimension, ViewProviderDimension *viewProvider) const;
    void drawDiameter(TechDraw::DrawViewDimension *dimension, ViewProviderDimension *viewProvider) const;
    void drawAngle(TechDraw::DrawViewDimension *dimension, ViewProviderDimension *viewProvider) const;
    
    virtual QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;
    virtual void setSvgPens(void);
    virtual void setPens(void);
    Base::Vector3d findIsoDir(Base::Vector3d ortho);
    Base::Vector3d findIsoExt(Base::Vector3d isoDir);
    QString getPrecision(void);

    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event) override;

    bool hasHover;
    QGIDatumLabel* datumLabel;                                         //dimension text
    QGIDimLines* dimLines;                                       //dimension lines + extension lines
    QGIArrow* aHead1;
    QGIArrow* aHead2;
    //QGICMark* centerMark
    double m_lineWidth;

    void arrowPositionsToFeature(const Base::Vector2d positions[]) const;

private:
    static inline Base::Vector2d fromQtApp(const Base::Vector3d &v) { return Base::Vector2d(v.x, -v.y); }
    static inline Base::BoundBox2d fromQtGui(const QRectF &r)
                                       { return Base::BoundBox2d(Rez::appX(r.left()), -Rez::appX(r.top()),
                                                                 Rez::appX(r.right()), -Rez::appX(r.bottom())); }

    static inline QPointF toQtGui(const Base::Vector2d &v) { return QPointF(Rez::guiX(v.x), -Rez::guiX(v.y)); }
    static inline QRectF toQtGui(const Base::BoundBox2d &r)
                             { return QRectF(Rez::guiX(r.MinX), -Rez::guiX(r.MaxY),
                                             Rez::guiX(r.Width()), Rez::guiX(r.Height())); }

    static double toDeg(double a);
    static double toQtRad(double a);
    static double toQtDeg(double a);

    double getDefaultExtensionLineOverhang() const;
    double getDefaultArrowTailLength() const;
    double getDefaultIsoDimensionLineSpacing() const;
    double getDefaultIsoReferenceLineOverhang() const;
    double getDefaultAsmeHorizontalLeaderLength() const;
    double getDefaultAsmeExtensionLineGap() const;

/*    QGIView* m_parent;      //for edit dialog set up eventually*/

};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEWDIMENSION_H
