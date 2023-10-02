/***************************************************************************
 *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

#ifndef PARTGUI_SECTIONCUTTING_H
#define PARTGUI_SECTIONCUTTING_H

#include <Inventor/SbBox3f.h>
#include <QDialog>
#include <App/DocumentObserver.h>

class QDoubleSpinBox;
class QSlider;

namespace PartGui {

class Ui_SectionCut;

class SectionCut : public QDialog
{
    Q_OBJECT

public:
    static SectionCut* makeDockWidget(QWidget* parent = nullptr);
    explicit SectionCut(QWidget* parent = nullptr);
    ~SectionCut() override;

protected Q_SLOTS:
    void onGroupBoxXtoggled();
    void onGroupBoxYtoggled();
    void onGroupBoxZtoggled();
    void onCutXvalueChanged(double);
    void onCutYvalueChanged(double);
    void onCutZvalueChanged(double);
    void onCutXHSsliderMoved(int);
    void onCutYHSsliderMoved(int);
    void onCutZHSsliderMoved(int);
    void onCutXHSChanged(int);
    void onCutYHSChanged(int);
    void onCutZHSChanged(int);
    void onFlipXclicked();
    void onFlipYclicked();
    void onFlipZclicked();
    void onRefreshCutPBclicked();
    void onCutColorclicked();
    void onTransparencyHSMoved(int);
    void onTransparencyHSChanged(int);
    void onGroupBoxIntersectingToggled();
    void onBFragColorclicked();
    void onBFragTransparencyHSMoved(int);
    void onBFragTransparencyHSChanged(int);

public:
    void reject() override;

private:
    std::unique_ptr<Ui_SectionCut> ui;
    std::vector<App::DocumentObjectT> ObjectsListVisible;
    App::Document* doc = nullptr; // pointer to active document
    bool hasBoxX = false;
    bool hasBoxY = false;
    bool hasBoxZ = false;
    bool hasBoxCustom = false;
    void noDocumentActions();
    void startCutting(bool isInitial = false);
    SbBox3f getViewBoundingBox();
    void refreshCutRanges(SbBox3f, bool forXValue = true, bool forYValue = true, bool forZValue = true,
        bool forXRange = true, bool forYRange = true, bool forZRange = true);
    void CutValueHelper(double val, QDoubleSpinBox* SpinBox, QSlider* Slider);
    void FlipClickedHelper(const char* BoxName);
    const char* CompoundName = "SectionCutCompound";
    const char* BoxXName = "SectionCutBoxX";
    const char* BoxYName = "SectionCutBoxY";
    const char* BoxZName = "SectionCutBoxZ";
    const char* CutXName = "SectionCutX";
    const char* CutYName = "SectionCutY";
    const char* CutZName = "SectionCutZ";
    void changeCutBoxColors();
    App::DocumentObject* CreateBooleanFragments(App::Document* doc);
    void setBooleanFragmentsColor();
};

} // namespace PartGui

#endif // PARTGUI_SECTIONCUTTING_H
