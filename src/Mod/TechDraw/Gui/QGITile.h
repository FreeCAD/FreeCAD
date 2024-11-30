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

#ifndef TECHDRAWGUI_QGITILE_H
#define TECHDRAWGUI_QGITILE_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>
#include <QFont>
#include <QGraphicsColorizeEffect>
#include <QPointF>

#include "QGIDecoration.h"

namespace TechDraw {
class DrawTile;
class DrawTileWeld;
}

namespace TechDrawGui
{
class QGCustomSvg;
class QGCustomText;
class QGIWeldSymbol;

class TechDrawGuiExport QGITile : public QGIDecoration
{
public:
    explicit QGITile(TechDraw::DrawTileWeld*);
    ~QGITile() override;

    enum {Type = QGraphicsItem::UserType + 325};
    int type(void) const override { return Type;}

    QRectF boundingRect() const override;

    void setTileTextLeft(std::string s);
    void setTileTextRight(std::string s);
    void setTileTextCenter(std::string s);
    void setFont(QFont f, double fSizePx);
    void setFont(std::string fName, double fSizePx);
    void setSymbolFile(std::string s);
    void setTilePosition(QPointF org, int r, int c);
    void setTileScale(double s);
    void setTailRight(bool b) { m_tailRight = b; }
    void setAltWeld(bool b) { m_altWeld = b; }
    bool isTailRight();
    void draw() override;

protected:
    QColor getTileColor(void) const;
    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();

    double getSymbolWidth(void) const;
    double getSymbolHeight(void) const;
    double getSymbolFactor(void) const;
    QByteArray getSvgString(QString svgPath);

    QString prefTextFont(void) const;
    double prefFontSize(void) const;
    void makeSymbol(void);
    void makeText(void);

    bool getAltWeld(void);
    bool isReadable(QString filePath);
    std::string getStringFromFile(std::string inSpec);


private:
    QGCustomText*      m_qgTextL;
    QGCustomText*      m_qgTextR;
    QGCustomText*      m_qgTextC;
    QGCustomSvg*       m_qgSvg;
  //QGraphicsColorizeEffect* m_effect;
    QString            m_svgPath;
    QString            m_textL;
    QString            m_textR;
    QString            m_textC;
    QString            m_fontName;
    QFont              m_font;
    QPointF            m_origin;
    double             m_wide;
    double             m_high;
    double             m_scale;
    int                m_row;
    int                m_col;
    bool               m_tailRight;
    bool               m_altWeld;
    TechDraw::DrawTileWeld* m_tileFeat;
};

}

#endif // TECHDRAWGUI_QGITILE_H
