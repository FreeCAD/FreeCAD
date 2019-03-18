/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <iomanip>
# include <ios>
#endif
#include <Inventor/SbBasic.h>
#include <Inventor/SbBSPTree.h>

#include <qglobal.h>
#include <Base/FileInfo.h>
#include "SoFCVectorizeSVGAction.h"

using namespace Gui;

class SoVectorizeItem {
public:
    SoVectorizeItem() {
        this->type = UNDEFINED;
        this->depth = 0.0f;
    }
    // quick and easy type system
    enum Type {
        UNDEFINED,
        LINE,
        TRIANGLE,
        TEXT,
        POINT,
        IMAGE
    };
    int type;
    float depth; // for depth sorting
};

class SoVectorizePoint : public SoVectorizeItem {
public:
    SoVectorizePoint(void) {
        this->type = POINT;
        this->vidx = 0;
        this->size = 1.0f;
        this->col = 0;
    }
    int vidx;       // index to BSPtree coordinate
    float size;     // Coin size (pixels)
    uint32_t col;
};

class SoVectorizeTriangle : public SoVectorizeItem {
public:
    SoVectorizeTriangle(void) {
        this->type = TRIANGLE;
    }
    int vidx[3];      // indices to BSPtree coordinates
    uint32_t col[3];
};

class SoVectorizeLine : public SoVectorizeItem {
public:
    SoVectorizeLine(void) {
        this->type = LINE;
        vidx[0] = 0;
        vidx[1] = 0;
        col[0] = 0;
        col[1] = 0;
        this->pattern = 0xffff;
        this->width = 1.0f;
    }
    int vidx[2];       // indices to BSPtree coordinates
    uint32_t col[2];
    uint16_t pattern;  // Coin line pattern
    float width;       // Coin line width (pixels)
};

class SoVectorizeText : public SoVectorizeItem {
public:
    SoVectorizeText(void) {
        this->type = TEXT;
        this->fontsize = 10;
        this->col = 0;
        this->justification = LEFT;
    }

    enum Justification {
        LEFT,
        RIGHT,
        CENTER
    };

    SbName fontname;
    float fontsize;    // size in normalized coordinates
    SbString string;
    SbVec2f pos;       // pos in normalized coordinates
    uint32_t col;
    Justification justification;
};

class SoVectorizeImage : public SoVectorizeItem {
public:
    SoVectorizeImage(void) {
        this->type = IMAGE;
        this->image.data = 0;
        this->image.nc = 0;
    }

    SbVec2f pos;        // pos in normalized coordinates
    SbVec2f size;       // size in normalized coordinates

    struct Image {
        const unsigned char * data;
        SbVec2s size;
        int nc;
    } image;
};

// ----------------------------------------------------------------

SoSVGVectorOutput::SoSVGVectorOutput()
{
}

SoSVGVectorOutput::~SoSVGVectorOutput()
{
    closeFile();
}

SbBool SoSVGVectorOutput::openFile (const char *filename)
{
    Base::FileInfo fi(filename);
#ifdef _MSC_VER
    this->file.open(fi.toStdWString().c_str(), std::ios::out | std::ios::binary);
#else
    this->file.open(fi.filePath().c_str(), std::ios::out | std::ios::binary);
#endif

    return this->file.is_open();
}

void SoSVGVectorOutput::closeFile (void)
{
    if (this->file.is_open())
        this->file.close();
}

std::fstream& SoSVGVectorOutput::getFileStream()
{
    return this->file;
}

// ----------------------------------------------------------------

namespace Gui {
class SoFCVectorizeSVGActionP
{
public:
    SoFCVectorizeSVGActionP(SoFCVectorizeSVGAction * p) {
        this->publ = p;
    }

    void printCircle(const SbVec3f & v, const SbColor & c, const float radius) const;
    void printSquare(const SbVec3f & v, const SbColor & c, const float size) const;
    void printTriangle(const SbVec3f * v, const SbColor * c) const;
    void printTriangle(const SoVectorizeTriangle * item) const;
    void printLine(const SoVectorizeLine * item) const;
    void printPoint(const SoVectorizePoint * item) const;
    void printText(const SoVectorizeText * item) const;
    void printImage(const SoVectorizeImage * item) const;

private:
    SoFCVectorizeSVGAction * publ;
};
}

void SoFCVectorizeSVGActionP::printText(const SoVectorizeText * item) const
{
    SbVec2f mul = publ->getRotatedViewportSize();
    SbVec2f add = publ->getRotatedViewportStartpos();
    float posx = item->pos[0]*mul[0]+add[0];
    float posy = item->pos[1]*mul[1]+add[1];

    std::ostream& str = publ->getSVGOutput()->getFileStream();
    str << "<text x=\"" << posx << "\" y=\"" << posy << "\" "
           "font-size=\"" << item->fontsize * mul[1] << "px\">" 
        << item->string.getString() << "</text>" << std::endl;
}

void SoFCVectorizeSVGActionP::printTriangle(const SoVectorizeTriangle * item) const
{
    SbVec2f mul = publ->getRotatedViewportSize();
    SbVec2f add = publ->getRotatedViewportStartpos();

    const SbBSPTree & bsp = publ->getBSPTree();

    SbVec3f v[3];
    SbColor c[3];
    float t[3];

    for (int i = 0; i < 3; i++) {
        v[i] = bsp.getPoint(item->vidx[i]);
        v[i][0] = (v[i][0] * mul[0]) + add[0];
        v[i][1] = ((1.0f-v[i][1]) * mul[1]) + add[1];
        c[i].setPackedValue(item->col[i], t[i]);
    }
    this->printTriangle((SbVec3f*)v, (SbColor*)c);
}
/* TODO: Support gradients, e.g.
  <defs>
    <linearGradient id="verlauf" x1="0%" y1="100%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="black" stop-opacity="100%" />
      <stop offset="100%" stop-color="white" stop-opacity="50%" />
    </linearGradient>
  </defs>
*/
/* Example: color per vertex
#Inventor V2.1 ascii
Separator {

  Coordinate3 {
    point [ 0.000000  0.000000  0.000000,
        100.000000  50.000000  0.000000,
        0.000000  100.000000  0.000000 ]

  }
  Material {
    diffuseColor [ 1 1 0, 0 0 1, 1 0 0 ]
  }
  
  MaterialBinding {
    value PER_VERTEX
  }
  IndexedFaceSet {
    coordIndex [ 0, 1, 2, -1 ]
  }
}

<?xml version="1.0"?><!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.0//EN"
	"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd">
<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
	<defs>
		<linearGradient id="red" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="35" y2="80">
			<stop offset="0" stop-color="rgb(255,0,0)" stop-opacity="1"/>
			<stop offset="1" stop-color="rgb(255,0,0)" stop-opacity="0"/>
		</linearGradient>
		<linearGradient id="blue" gradientUnits="userSpaceOnUse" x1="100" y1="50" x2="0" y2="50">
			<stop offset="0" stop-color="rgb(0,0,255)" stop-opacity="1"/>
			<stop offset="1" stop-color="rgb(0,0,255)" stop-opacity="0"/>
		</linearGradient>
		<linearGradient id="yellow" gradientUnits="userSpaceOnUse" x1="0" y1="100" x2="40" y2="20">
			<stop offset="0" stop-color="rgb(255,255,0)" stop-opacity="1"/>
			<stop offset="1" stop-color="rgb(255,255,0)" stop-opacity="0"/>
		</linearGradient>
		<path id="triangle1" d="M0 0 L100 50 L0 100 z"/>
<filter id="colorAdd">
	<feComposite in="SourceGraphic" in2="BackgroundImage" operator="arithmetic" k2="1" k3="1"/>
</filter>
<filter id="Matrix1">
	<feColorMatrix type="matrix" values="
	1 0 0 0 0
	0 1 0 0 0
	0 0 1 0 0
	1 1 1 1 0
	0 0 0 0 1
	"/>
</filter>
</defs>
<g filter="url(#Matrix1)">
	<use xlink:href="#triangle1" fill="url(#blue)"/>
	<use xlink:href="#triangle1" fill="url(#yellow)" filter="url(#colorAdd)"/>
	<use xlink:href="#triangle1" fill="url(#red)" filter="url(#colorAdd)"/>
</g>
</svg>
*/
void SoFCVectorizeSVGActionP::printTriangle(const SbVec3f * v, const SbColor * c) const
{
    if (v[0] == v[1] || v[1] == v[2] || v[0] == v[2]) return;
    uint32_t cc = c->getPackedValue();

    std::ostream& str = publ->getSVGOutput()->getFileStream();
    str << "<path d=\"M "
        << v[2][0] << "," << v[2][1] << " L "
        << v[1][0] << "," << v[1][1] << " "
        << v[0][0] << "," << v[0][1] << " z\"" << std::endl
        << "    style=\"fill:#"
        << std::hex << std::setw(6) << std::setfill('0') << (cc >> 8)
        << "; stroke:#"
        << std::hex << std::setw(6) << std::setfill('0') << (cc >> 8)
        << ";" << std::endl
        << "    stroke-width:1.0;" << std::endl
        << "    stroke-linecap:round;stroke-linejoin:round\"/>" << std::endl;
}

void SoFCVectorizeSVGActionP::printCircle(const SbVec3f & v, const SbColor & c, const float radius) const
{
    // todo
    Q_UNUSED(v); 
    Q_UNUSED(c); 
    Q_UNUSED(radius); 
}

void SoFCVectorizeSVGActionP::printSquare(const SbVec3f & v, const SbColor & c, const float size) const
{
    // todo
    Q_UNUSED(v); 
    Q_UNUSED(c); 
    Q_UNUSED(size); 
}

void SoFCVectorizeSVGActionP::printLine(const SoVectorizeLine * item) const
{
    SbVec2f mul = publ->getRotatedViewportSize();
    SbVec2f add = publ->getRotatedViewportStartpos();

    const SbBSPTree & bsp = publ->getBSPTree();

    SbVec3f v[2];
    SbColor c[2];
    float t[2];

    for (int i = 0; i < 2; i++) {
        v[i] = bsp.getPoint(item->vidx[i]);
        v[i][0] = (v[i][0] * mul[0]) + add[0];
        v[i][1] = ((1.0f-v[i][1]) * mul[1]) + add[1];
        c[i].setPackedValue(item->col[i], t[i]);
    }
    uint32_t cc = c[0].getPackedValue();

    std::ostream& str = publ->getSVGOutput()->getFileStream();
    str << "<line "
        << "x1=\"" << v[0][0] << "\" y1=\"" << v[0][1] << "\" "
        << "x2=\"" << v[1][0] << "\" y2=\"" << v[1][1] << "\" "
        << "stroke=\"#"
        << std::hex << std::setw(6) << std::setfill('0') << (cc >> 8)
        << "\" stroke-width=\"1px\" />\n";
}

void SoFCVectorizeSVGActionP::printPoint(const SoVectorizePoint * item) const
{
    // todo
    Q_UNUSED(item); 
}

void SoFCVectorizeSVGActionP::printImage(const SoVectorizeImage * item) const
{
    // todo
    Q_UNUSED(item); 
}

// -------------------------------------------------------

SO_ACTION_SOURCE(SoFCVectorizeSVGAction);

void SoFCVectorizeSVGAction::initClass(void)
{
    SO_ACTION_INIT_CLASS(SoFCVectorizeSVGAction, SoVectorizeAction);
}

SoFCVectorizeSVGAction::SoFCVectorizeSVGAction()
{
    SO_ACTION_CONSTRUCTOR(SoFCVectorizeSVGAction);
    this->setOutput(new SoSVGVectorOutput);
    this->p = new SoFCVectorizeSVGActionP(this);
}

SoFCVectorizeSVGAction::~SoFCVectorizeSVGAction()
{
    delete this->p;
}

SoSVGVectorOutput *
SoFCVectorizeSVGAction::getSVGOutput(void) const
{
    return static_cast<SoSVGVectorOutput*>(SoVectorizeAction::getOutput());
}

void SoFCVectorizeSVGAction::printHeader(void) const
{
    std::ostream& str = this->getSVGOutput()->getFileStream();
    str << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
    str << "<!-- Created with FreeCAD (http://www.freecadweb.org) -->" << std::endl;
    str << "<svg xmlns=\"http://www.w3.org/2000/svg\"" << std::endl;
    str << "     xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:ev=\"http://www.w3.org/2001/xml-events\"" << std::endl;
    str << "     version=\"1.1\" baseProfile=\"full\"" << std::endl;

    SbVec2f size = getPageSize();
    if (this->getOrientation() == LANDSCAPE)
        SbSwap<float>(size[0], size[1]);
    str << "     width=\"" << size[0] << "\" height=\"" << size[1] << "\">" << std::endl;
    str << "<g>" << std::endl;
}

void SoFCVectorizeSVGAction::printFooter(void) const
{
    std::ostream& str = this->getSVGOutput()->getFileStream();
    str << "</g>" << std::endl;
    str << "</svg>";
}

void SoFCVectorizeSVGAction::printViewport(void) const
{
}

void SoFCVectorizeSVGAction::printBackground(void) const
{
    SbVec2f mul = getRotatedViewportSize();
    SbVec2f add = getRotatedViewportStartpos();

    float x[2],y[2];
    x[0] = add[0];
    x[1] = mul[0] - add[0];
    y[0] = add[1];
    y[1] = mul[1] - add[1];

    SbColor bg;
    (void)this->getBackgroundColor(bg);
    uint32_t cc = bg.getPackedValue();

    std::ostream& str = this->getSVGOutput()->getFileStream();
    str << "</g>" << std::endl;
    str << "<path" << std::endl;
    str << "   d=\"M "
        << x[0] << "," << y[0] << " L "
        << x[1] << "," << y[0] << " L "
        << x[1] << "," << y[1] << " L "
        << x[0] << "," << y[1] << " L "
        << x[0] << "," << y[0] << " z \"" << std::endl;
    str << "   style=\"fill:#"
        << std::hex << std::setw(6) << std::setfill('0') << (cc >> 8)
        << ";fill-opacity:1;fill-rule:evenodd;stroke:none;"
           "stroke-width:1px;stroke-linecap:butt;stroke-linejoin:"
           "miter;stroke-opacity:1\" />\n";
    str << "<g>" << std::endl;
}

void SoFCVectorizeSVGAction::printItem(const SoVectorizeItem * item) const
{
    switch (item->type) {
    case SoVectorizeItem::TRIANGLE:
        this->p->printTriangle(static_cast<const SoVectorizeTriangle*>(item));
        break;
    case SoVectorizeItem::LINE:
        this->p->printLine(static_cast<const SoVectorizeLine*>(item));
        break;
    case SoVectorizeItem::POINT:
        this->p->printPoint(static_cast<const SoVectorizePoint*>(item));
        break;
    case SoVectorizeItem::TEXT:
        this->p->printText(static_cast<const SoVectorizeText*>(item));
        break;
    case SoVectorizeItem::IMAGE:
        this->p->printImage(static_cast<const SoVectorizeImage*>(item));
        break;
    default:
        assert(0 && "unsupported item");
        break;
    }
}
