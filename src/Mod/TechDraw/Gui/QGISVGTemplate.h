/***************************************************************************
 *   Copyright (c) 2012-2014 Luke Parry <l.parry@warwick.ac.uk>            *
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

class QGraphicsScene;
class QGraphicsSvgItem;
class QSvgRenderer;
class QFile;
class QString;

#include "QGITemplate.h"
#include "QGIUserTypes.h"

namespace TechDraw
{
class DrawSVGTemplate;
}

namespace TechDrawGui
{
class QGSPage;

class TechDrawGuiExport QGISVGTemplate : public TechDrawGui::QGITemplate
{
    Q_OBJECT

public:
    explicit QGISVGTemplate(QGSPage* scene);
    ~QGISVGTemplate() override;

    enum {Type = UserType::QGISVGTemplate};
    int type() const override { return Type; }

    void draw() override;
    void drawPageRectangle();

    void updateView(bool update = false) override;

    TechDraw::DrawSVGTemplate* getSVGTemplate() const;
    std::vector<TemplateTextField*> getTextFields() override;

protected:
    void openFile(const QFile& file);
    void load(QByteArray svgCode);

    void createClickHandles();
    void clearClickHandles();

private:
    QGraphicsSvgItem* m_svgItem;
    QSvgRenderer* m_svgRender;
    QGraphicsRectItem* m_pageRectangle;

};// class QGISVGTemplate

}// namespace TechDrawGui