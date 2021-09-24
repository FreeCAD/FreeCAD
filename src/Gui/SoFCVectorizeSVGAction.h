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


#ifndef GUI_SOFCVECTORIZESVGACTION_H
#define GUI_SOFCVECTORIZESVGACTION_H

#ifndef __InventorAll__
# include "InventorAll.h"
#endif

#include <fstream>


namespace Gui {

class GuiExport SoSVGVectorOutput : public SoVectorOutput {
public:
    SoSVGVectorOutput();
    virtual ~SoSVGVectorOutput();

    virtual SbBool openFile (const char *filename);
    virtual void closeFile (void);
    std::fstream& getFileStream();

private:
    std::fstream file;
};

/**
 * @author Werner Mayer
 */
class SoFCVectorizeSVGActionP;
class GuiExport SoFCVectorizeSVGAction : public SoVectorizeAction {
    typedef SoReplacedElement inherited;

    SO_ACTION_HEADER(SoFCVectorizeSVGAction);

public:
    SoFCVectorizeSVGAction(void);
    virtual ~SoFCVectorizeSVGAction();

    static void initClass(void);
    SoSVGVectorOutput * getSVGOutput(void) const;

    virtual void setBackgroundState(bool b) { m_backgroundState = b; }
    virtual bool getBackgroundState(void) const { return m_backgroundState; }
    virtual void setLineWidth(double w) { m_lineWidth = w; }
    virtual double getLineWidth(void) const { return m_lineWidth; }
    virtual void setUseMM(bool b) { m_usemm = b; }
    virtual bool getUseMM(void) const { return m_usemm; }

protected:
    virtual void printHeader(void) const;
    virtual void printFooter(void) const;
    virtual void printBackground(void) const;
    virtual void printItem(const SoVectorizeItem * item) const;
    virtual void printViewport(void) const;

private:
    SoFCVectorizeSVGActionP* p;
    friend class SoFCVectorizeSVGActionP;
    bool m_backgroundState;
    double m_lineWidth;
    bool m_usemm;
};

} // namespace Gui

#endif // GUI_SOFCVECTORIZESVGACTION_H
