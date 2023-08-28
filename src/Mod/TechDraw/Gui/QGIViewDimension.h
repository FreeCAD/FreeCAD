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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>
#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>

#include <Base/Vector3D.h>

#include "QGCustomText.h"
#include "QGIView.h"
#include "Rez.h"


namespace TechDraw {
class DrawViewDimension;
}

namespace TechDraw {
class BaseGeom;
class AOC;
}

namespace TechDrawGui
{
class QGCustomText;
class QGIArrow;
class QGIDimLines;
class QGIViewDimension;
class QGCustomSvg;
class ViewProviderDimension;

class QGIDatumLabel : public QGraphicsObject
{
Q_OBJECT

public:
    QGIDatumLabel();
    ~QGIDatumLabel() override = default;

    enum {Type = QGraphicsItem::UserType + 107};
    int type() const override { return Type;}

    QRectF boundingRect() const override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr ) override;
    void setLabelCenter();
    void setPosFromCenter(const double &xCenter, const double &yCenter);
    double X() const { return posX; }
    double Y() const { return posY; }              //minus posY?

    void setFont(QFont font);
    QFont getFont() const { return m_dimText->font(); }
    void setDimString(QString text);
    void setDimString(QString text, qreal maxWidth);
    void setUnitString(QString text);
    void setToleranceString();
    void setPrettySel();
    void setPrettyPre();
    void setPrettyNormal();
    void setColor(QColor color);

    QGCustomText* getDimText() { return m_dimText; }
    void setDimText(QGCustomText* newText) { m_dimText = newText; }
    QGCustomText* getTolTextOver() { return m_tolTextOver; }
    void setTolTextOver(QGCustomText* newTol) { m_tolTextOver = newTol; }
    QGCustomText* getTolTextUnder() { return m_tolTextUnder; }
    void setTolTextUnder(QGCustomText* newTol) { m_tolTextOver = newTol; }

    double getTolAdjust();

    bool isFramed() const { return m_isFramed; }
    void setFramed(bool framed) { m_isFramed = framed; }

    double getLineWidth() const { return m_lineWidth; }
    void setLineWidth(double lineWidth) { m_lineWidth = lineWidth; }
    void setQDim(QGIViewDimension* qDim) { parent = qDim;}

Q_SIGNALS:
    void setPretty(int state);
    void dragging(bool);
    void hover(bool state);
    void selected(bool state);
    void dragFinished();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    int getPrecision();

    bool getVerticalSep() const { return verticalSep; }
    void setVerticalSep(bool sep) { verticalSep = sep; }
    std::vector<int> getSeps() const { return seps; }
    void setSeps(std::vector<int> newSeps) { seps = newSeps; }

private:
    bool verticalSep;
    std::vector<int> seps;

    QGIViewDimension* parent;

    QGCustomText* m_dimText;
    QGCustomText* m_tolTextOver;
    QGCustomText* m_tolTextUnder;
    QGCustomText* m_unitText;
    QColor m_colNormal;
    bool m_ctrl;

    double posX;
    double posY;

    bool m_isFramed;
    double m_lineWidth;

    int m_dragState;

private:
};

//*******************************************************************

class TechDrawGuiExport QGIViewDimension : public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 106};

    QGIViewDimension();
    ~QGIViewDimension() override = default;

    void setViewPartFeature(TechDraw::DrawViewDimension *obj);
    int type() const override { return Type;}
    QRectF boundingRect() const override;
    void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = nullptr ) override;

    void drawBorder() override;
    void updateView(bool update = false) override;
    QColor prefNormalColor();
    QString getLabelText();
    void setPrettyPre();
    void setPrettySel();
    void setPrettyNormal();

    void setGroupSelection(bool isSelected) override;
    virtual QGIDatumLabel* getDatumLabel() const { return datumLabel; }

    void setNormalColorAll();
    TechDraw::DrawViewDimension* getDimFeat() { return dvDimension; }

public Q_SLOTS:
    void onPrettyChanged(int state);
    void datumLabelDragged(bool ctrl);
    void datumLabelDragFinished();
    void select(bool state);
    void hover(bool state);
    void updateDim();

protected:
    void mousePressEvent( QGraphicsSceneMouseEvent * event) override;
    void mouseMoveEvent( QGraphicsSceneMouseEvent * event) override;
    void mouseReleaseEvent( QGraphicsSceneMouseEvent * event) override;

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

    void resetArrows() const;
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
    void drawDistanceOverride(const Base::Vector2d &startPoint, const Base::Vector2d &endPoint,
                              double lineAngle, const Base::BoundBox2d &labelRectangle,
                              int standardStyle, int renderExtent, bool flipArrows, double extensionAngle) const;

    void drawRadiusExecutive(const Base::Vector2d &centerPoint, const Base::Vector2d &midPoint, double radius,
                             double endAngle, double startRotation, const Base::BoundBox2d &labelRectangle,
                             double centerOverhang, int standardStyle, int renderExtent, bool flipArrow) const;

    void drawDistance(TechDraw::DrawViewDimension *dimension, ViewProviderDimension *viewProvider) const;
    void drawRadius(TechDraw::DrawViewDimension *dimension, ViewProviderDimension *viewProvider) const;
    void drawDiameter(TechDraw::DrawViewDimension *dimension, ViewProviderDimension *viewProvider) const;
    void drawAngle(TechDraw::DrawViewDimension *dimension, ViewProviderDimension *viewProvider) const;

    QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;
    virtual void setSvgPens();
    virtual void setPens();
    Base::Vector3d findIsoDir(Base::Vector3d ortho) const;
    Base::Vector3d findIsoExt(Base::Vector3d isoDir) const;
    QString getPrecision();

    void arrowPositionsToFeature(const Base::Vector2d positions[]) const;
    void makeMarkC(double xPos, double yPos, QColor color = Qt::red) const;

private:
    static inline Base::Vector2d fromQtApp(const Base::Vector3d &vec3) { return {vec3.x, -vec3.y}; }
    static inline Base::BoundBox2d fromQtGui(const QRectF &rect)
                                       { return { Rez::appX(rect.left()), -Rez::appX(rect.top()),
                                                  Rez::appX(rect.right()), -Rez::appX(rect.bottom()) }; }

    static inline QPointF toQtGui(const Base::Vector2d &vec2) { return {Rez::guiX(vec2.x), -Rez::guiX(vec2.y)}; }
    static inline QRectF toQtGui(const Base::BoundBox2d &rect)
                             { return {Rez::guiX(rect.MinX), -Rez::guiX(rect.MaxY),
                                       Rez::guiX(rect.Width()), Rez::guiX(rect.Height())}; }

    static double toDeg(double angle);
    static double toQtRad(double angle);
    static double toQtDeg(double angle);

    double getDefaultExtensionLineOverhang() const;
    double getDefaultArrowTailLength() const;
    double getDefaultIsoDimensionLineSpacing() const;
    double getIsoDimensionLineSpacing() const;
    double getDefaultIsoReferenceLineOverhang() const;
    double getDefaultAsmeHorizontalLeaderLength() const;

    TechDraw::DrawViewDimension *dvDimension;
    bool hasHover;
    QGIDatumLabel* datumLabel;                                         //dimension text
    QGIDimLines* dimLines;                                       //dimension lines + extension lines
    QGIArrow* aHead1;
    QGIArrow* aHead2;
    double m_lineWidth;

    QGCustomSvg* m_refFlag;

};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEWDIMENSION_H
