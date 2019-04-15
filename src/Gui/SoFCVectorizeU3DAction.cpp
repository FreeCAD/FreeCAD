/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <qglobal.h>
# include <iomanip>
# include <ios>
#endif
#include <Inventor/SbBasic.h>
#include <Inventor/SbBSPTree.h>

#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include "SoFCVectorizeU3DAction.h"

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

SoU3DVectorOutput::SoU3DVectorOutput()
{
}

SoU3DVectorOutput::~SoU3DVectorOutput()
{
    closeFile();
}

SbBool SoU3DVectorOutput::openFile (const char *filename)
{
    Base::FileInfo fi(filename);
#ifdef _MSC_VER
    this->file.open(fi.toStdWString().c_str(), std::ios::out | std::ios::binary);
#else
    this->file.open(fi.filePath().c_str(), std::ios::out | std::ios::binary);
#endif

    return this->file.is_open();
}

void SoU3DVectorOutput::closeFile (void)
{
    if (this->file.is_open())
        this->file.close();
}

std::fstream& SoU3DVectorOutput::getFileStream()
{
    return this->file;
}

// ----------------------------------------------------------------

namespace Gui {
class SoFCVectorizeU3DActionP
{
public:
    SoFCVectorizeU3DActionP(SoFCVectorizeU3DAction * p) {
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
    SoFCVectorizeU3DAction * publ;
};
}

void SoFCVectorizeU3DActionP::printText(const SoVectorizeText * item) const
{
    //SbVec2f mul = publ->getRotatedViewportSize();
    //SbVec2f add = publ->getRotatedViewportStartpos();
    //float posx = item->pos[0]*mul[0]+add[0];
    //float posy = item->pos[1]*mul[1]+add[1];

    //std::ostream& str = publ->getU3DOutput()->getFileStream();
    // todo
    Q_UNUSED(item); 
}

void SoFCVectorizeU3DActionP::printTriangle(const SoVectorizeTriangle * item) const
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

void SoFCVectorizeU3DActionP::printTriangle(const SbVec3f * v, const SbColor * c) const
{
    if (v[0] == v[1] || v[1] == v[2] || v[0] == v[2]) return;
    //uint32_t cc = c->getPackedValue();

    //std::ostream& str = publ->getU3DOutput()->getFileStream();
    // todo
    Q_UNUSED(c); 
}

void SoFCVectorizeU3DActionP::printCircle(const SbVec3f & v, const SbColor & c, const float radius) const
{
    // todo
    Q_UNUSED(v); 
    Q_UNUSED(c); 
    Q_UNUSED(radius); 
}

void SoFCVectorizeU3DActionP::printSquare(const SbVec3f & v, const SbColor & c, const float size) const
{
    // todo
    Q_UNUSED(v); 
    Q_UNUSED(c); 
    Q_UNUSED(size); 
}

void SoFCVectorizeU3DActionP::printLine(const SoVectorizeLine * item) const
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
    //uint32_t cc = c->getPackedValue();

    //std::ostream& str = publ->getU3DOutput()->getFileStream();
    // todo
    Q_UNUSED(item); 
}

void SoFCVectorizeU3DActionP::printPoint(const SoVectorizePoint * item) const
{
    // todo
    Q_UNUSED(item); 
}

void SoFCVectorizeU3DActionP::printImage(const SoVectorizeImage * item) const
{
    // todo
    Q_UNUSED(item); 
}

// -------------------------------------------------------

SO_ACTION_SOURCE(SoFCVectorizeU3DAction);

void SoFCVectorizeU3DAction::initClass(void)
{
    SO_ACTION_INIT_CLASS(SoFCVectorizeU3DAction, SoVectorizeAction);
    //SO_ACTION_ADD_METHOD(SoNode, SoFCVectorizeU3DAction::actionMethod);
}

SoFCVectorizeU3DAction::SoFCVectorizeU3DAction()
{
    SO_ACTION_CONSTRUCTOR(SoFCVectorizeU3DAction);
    this->setOutput(new SoU3DVectorOutput);
    this->p = new SoFCVectorizeU3DActionP(this);
}

SoFCVectorizeU3DAction::~SoFCVectorizeU3DAction()
{
    delete this->p;
}

SoU3DVectorOutput *
SoFCVectorizeU3DAction::getU3DOutput(void) const
{
    return static_cast<SoU3DVectorOutput*>(SoVectorizeAction::getOutput());
}

void
SoFCVectorizeU3DAction::actionMethod(SoAction * a, SoNode * n)
{
    Q_UNUSED(a); 
    Q_UNUSED(n); 
}

void SoFCVectorizeU3DAction::beginTraversal(SoNode * node)
{
    inherited::beginTraversal(node);
}

void SoFCVectorizeU3DAction::endTraversal(SoNode * node)
{
    inherited::endTraversal(node);
}

void SoFCVectorizeU3DAction::printHeader(void) const
{
    std::ostream& str = this->getU3DOutput()->getFileStream();
    str << "FILE_FORMAT \"IDTF\"" << std::endl
        << "FORMAT_VERSION 100" << std::endl;

    str << Base::tabs(0) << "NODE \"MODEL\" {" << std::endl;
    str << Base::tabs(1) << "NODE_NAME \"FreeCAD\"" << std::endl;
    str << Base::tabs(1) << "PARENT_LIST {" << std::endl;
    str << Base::tabs(2) << "PARENT_COUNT 1" << std::endl;
    str << Base::tabs(2) << "PARENT 0 {" << std::endl;
    str << Base::tabs(3) << "PARENT_NAME \"<NULL>\"" << std::endl;
    str << Base::tabs(3) << "PARENT_TM {" << std::endl;
    str << Base::tabs(4) << "1.000000 0.000000 0.000000 0.000000" << std::endl;
    str << Base::tabs(4) << "0.000000 1.000000 0.000000 0.000000" << std::endl;
    str << Base::tabs(4) << "0.000000 0.000000 1.000000 0.000000" << std::endl;
    str << Base::tabs(4) << "0.000000 0.000000 0.000000 1.000000" << std::endl;
    str << Base::tabs(3) << "}" << std::endl;
    str << Base::tabs(2) << "}" << std::endl;
    str << Base::tabs(1) << "}" << std::endl;
    str << Base::tabs(1) << "RESOURCE_NAME \"FreeCAD\"" << std::endl;
    str << Base::tabs(0) << "}" << std::endl;
}

void SoFCVectorizeU3DAction::printFooter(void) const
{
}

void SoFCVectorizeU3DAction::printViewport(void) const
{
}

void SoFCVectorizeU3DAction::printBackground(void) const
{
    //SbVec2f mul = getRotatedViewportSize();
    //SbVec2f add = getRotatedViewportStartpos();

    //float x[2],y[2];
    //x[0] = add[0];
    //x[1] = mul[0] - add[0];
    //y[0] = add[1];
    //y[1] = mul[1] - add[1];

    //SbColor bg;
    //(void)this->getBackgroundColor(bg);
    //uint32_t cc = bg.getPackedValue();

    //std::ostream& str = this->getU3DOutput()->getFileStream();
    // todo
}

void SoFCVectorizeU3DAction::printItem(const SoVectorizeItem * item) const
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
