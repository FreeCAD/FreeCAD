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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <boost_signals2.hpp>

#include <QColor>
#include <QFont>
#include <QGraphicsItemGroup>
#include <QObject>
#include <QPen>
#include <QPointF>

#include <Base/Parameter.h>
#include <Base/Vector3D.h>

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace App
{
class DocumentObject;
}

namespace Gui
{
class ViewProvider;
}

namespace TechDraw
{
class DrawView;
}

namespace TechDrawGui
{
class QGSPage;
class QGVPage;
class ViewProviderPage;
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

    enum {Type = QGraphicsItem::UserType + 101};
    int type() const override { return Type;}
    QRectF boundingRect() const override;
    void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr ) override;

    const char *      getViewName() const;
    const std::string getViewNameAsString() const;
    void setViewFeature(TechDraw::DrawView *obj);
    TechDraw::DrawView * getViewObject() const;
    MDIViewPage* getMDIViewPage() const;

    double getScale();

    void hideFrame();               //used by derived classes that don't display a frame

    virtual bool getFrameState();
    virtual void toggleCache(bool state);
    virtual void updateView(bool update = false);
    virtual void drawBorder();
    virtual void isVisible(bool state);
    virtual bool isVisible();

    virtual bool getGroupSelection();
    virtual void setGroupSelection(bool isSelected);
    virtual void setGroupSelection(bool isSelected, const std::vector<std::string> &subNames);

    virtual void draw();
    virtual void drawCaption();
    virtual void rotateView();
    void makeMark(double xPos, double yPos, QColor color = Qt::red);
    void makeMark(Base::Vector3d pos, QColor color = Qt::red);
    void makeMark(QPointF pos, QColor color = Qt::red);


    /** Methods to ensure that Y-Coordinates are orientated correctly.
     * @{ */
    void setPosition(qreal xPos, qreal yPos);
    inline qreal getY() { return y() * -1; }
    bool isInnerView() const { return m_innerView; }
    void isInnerView(bool state) { m_innerView = state; }
    QGIViewClip* getClipGroup();

    void alignTo(QGraphicsItem*, const QString &alignment);

    QColor prefNormalColor(); //preference
    QColor getNormalColor() { return m_colNormal; }  //current setting
    void setNormalColor(QColor color) { m_colNormal = color; }
    QColor getPreColor();     //preference
    QColor getSelectColor();  //preference
    QColor getCurrentColor() { return m_colCurrent; }
    void setCurrentColor(QColor color)  {m_colCurrent = color; }
    QColor getSettingColor() { return m_colSetting; }
    void   setSettingColor(QColor color) { m_colSetting = color; }

    virtual void setStack(int z);
    virtual void setStackFromVP();

    static Gui::ViewProvider* getViewProvider(App::DocumentObject* obj);
    static ViewProviderPage* getViewProviderPage(TechDraw::DrawView* dView);

    static int calculateFontPixelSize(double sizeInMillimetres);
    static int calculateFontPixelWidth(const QFont &font);
    static const double DefaultFontSizeInMM;

    static QString getPrefFont();
    static double getPrefFontSize();
    static double getDimFontSize();
    QFont getFont() { return m_font; };
    void setFont(QFont font) { m_font = font; }

    static int exactFontSize(std::string fontFamily, double nominalSize);

    virtual void removeChild(QGIView* child);
    virtual void addArbitraryItem(QGraphicsItem* qgi);
    virtual void switchParentItem(QGIView *targetParent);

    // Mouse handling
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

protected:
    QGIView* getQGIVByName(std::string name);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    // Preselection events:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    virtual QRectF customChildrenBoundingRect() const;
    void dumpRect(const char* text, QRectF rect);

    Base::Reference<ParameterGrp> getParmGroupCol();

private:
    TechDraw::DrawView *viewObj;
    std::string viewName;

    QHash<QString, QGraphicsItem*> alignHash;
    bool m_innerView;                                                  //View is inside another View
    bool m_multiselectActivated;

    QPen m_pen;
    QBrush m_brush;
    QColor m_colCurrent;
    QColor m_colNormal;
    QColor m_colPre;
    QColor m_colSel;
    QColor m_colSetting;
    QFont m_font;
    QGCustomLabel* m_label;
    QGCustomBorder* m_border;
    QGICaption* m_caption;
    QGCustomImage* m_lock;
    QPen m_decorPen;
    double m_lockWidth;
    double m_lockHeight;
    int m_zOrder;

};

} // namespace

#endif // DRAWINGGUI_QGRAPHICSITEMVIEW_H
