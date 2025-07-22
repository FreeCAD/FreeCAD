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

#include <Inventor/annex/HardCopy/SoVectorizeAction.h>
#include <Inventor/annex/HardCopy/SoVectorOutput.h>

#include <fstream>
#include <FCGlobal.h>


namespace Gui {

class GuiExport SoSVGVectorOutput : public SoVectorOutput {
public:
    SoSVGVectorOutput();
    ~SoSVGVectorOutput() override;

    SbBool openFile (const char *filename) override;
    void closeFile () override;
    std::fstream& getFileStream();

private:
    std::fstream file;
};

/**
 * @author Werner Mayer
 */
class SoFCVectorizeSVGActionP;
class GuiExport SoFCVectorizeSVGAction : public SoVectorizeAction {
    using inherited = SoReplacedElement;

    SO_ACTION_HEADER(SoFCVectorizeSVGAction);

public:
    SoFCVectorizeSVGAction();
    ~SoFCVectorizeSVGAction() override;

    static void initClass();
    SoSVGVectorOutput * getSVGOutput() const;

    virtual void setBackgroundState(bool b)  { m_backgroundState = b; }
    virtual bool getBackgroundState() const { return m_backgroundState; }
    virtual void setLineWidth(double w) { m_lineWidth = w; }
    virtual double getLineWidth() const { return m_lineWidth; }
    virtual void setUseMM(bool b) { m_usemm = b; }
    virtual bool getUseMM() const { return m_usemm; }

protected:
    void printHeader() const override;
    void printFooter() const override;
    void printBackground() const override;
    void printItem(const SoVectorizeItem * item) const override;
    void printViewport() const override;

private:
    SoFCVectorizeSVGActionP* p;
    friend class SoFCVectorizeSVGActionP;
    bool m_backgroundState{true};
    double m_lineWidth{1.0};
    bool m_usemm{false};
};

} // namespace Gui

#endif // GUI_SOFCVECTORIZESVGACTION_H
