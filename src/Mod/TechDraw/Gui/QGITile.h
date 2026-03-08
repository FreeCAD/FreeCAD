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

#include <QColor>
#include <QFont>
#include <QGraphicsColorizeEffect>
#include <QPointF>

#include "QGIDecoration.h"
#include "QGIUserTypes.h"

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
    ~QGITile() override = default;

    enum {Type = UserType::QGITile};
    int type() const override { return Type;}

    QRectF boundingRect() const override;

    void setTileTextLeft(const std::string& text);
    void setTileTextRight(const std::string& text);
    void setTileTextCenter(const std::string& text);
    void setFont(const QFont& font, double fSizePx);
    void setFont(const std::string& fName, double fSizePx);
    void setSymbolFile(const std::string& fileSpec);
    void setTilePosition(QPointF org, int row, int col);
    void setTileScale(double scale);
    void setTailRight(bool state) { m_tailRight = state; }
    void setAltWeld(bool state) { m_altWeld = state; }
    bool isTailRight() const;
    void draw() override;

    void setLocalAxes(Base::Vector3d xdir, Base::Vector3d ydir);
    QPointF mapPointToRotation(Base::Vector3d pointIn);
    QPointF calcTilePosition();

protected:
    QColor getTileColor() const;
    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();

    double getSymbolWidth() const;
    double getSymbolHeight() const;
    double getSymbolFactor() const;
    QByteArray getSvgString(QString svgPath);

    QString prefTextFont() const;
    double prefFontSize() const;
    void makeSymbol();
    void makeText();

    bool getAltWeld() const;
    bool isReadable(QString filePath);
    std::string getStringFromFile(const std::string &inSpec);


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

    Base::Vector3d     m_leaderXDirection;
    Base::Vector3d     m_leaderYDirection;
};

}