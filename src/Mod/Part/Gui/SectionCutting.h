// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <functional>
#include <Inventor/SbBox3f.h>
#include <QDialog>
#include <Base/BoundBox.h>
#include <App/DocumentObserver.h>

class QDoubleSpinBox;
class QSlider;

namespace Gui
{
class ViewProviderGeometryObject;
}

namespace Part
{
class Box;
class Cut;
class Compound;
}  // namespace Part

namespace PartGui
{

class Ui_SectionCut;

class SectionCut: public QDialog
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
    struct Args
    {
        Base::Vector3f origin;
        Base::Vector3f size;
        std::size_t numObjects;
        App::DocumentObject* partCompound;
        App::DocumentObject* boolFragment;
        std::function<void(Part::Box*)> boxFunc;
        std::function<void(Part::Cut*)> cutFunc;
    };

    void processXBoxAndCut(const Args& args);
    void processYBoxAndCut(const Args& args);
    void processZBoxAndCut(const Args& args);

    void initSpinBoxes();
    void initControls(const Base::BoundBox3d&);
    void initXControls(const Base::BoundBox3d&, const std::function<void(Part::Box*)>&);
    void initYControls(const Base::BoundBox3d&, const std::function<void(Part::Box*)>&);
    void initZControls(const Base::BoundBox3d&, const std::function<void(Part::Box*)>&);
    void initCutRanges();
    void setupConnections();
    void tryStartCutting();
    void setAutoColoringChecked(bool on);
    void setSlidersEnabled(bool on);
    void setSlidersToolTip(const QString& text);
    void setGroupsDisabled();
    void setAutoColor(const QColor& color);
    void setAutoTransparency(int value);
    void initBooleanFragmentControls(Gui::ViewProviderGeometryObject* compoundBF);
    Base::BoundBox3d collectObjects();
    void collectAndShowLinks(const std::vector<App::DocumentObject*>& objects);
    void noDocumentActions();
    void startCutting(bool isInitial = false);
    void startObjectCutting(bool isInitial);
    bool findObjects(std::vector<App::DocumentObject*>& objects);
    void filterObjects(std::vector<App::DocumentObject*>& objects);
    void throwMissingObjectsError(bool isInitial);
    bool isCuttingEnabled() const;
    void setObjectsVisible(bool value);
    int getCompoundTransparency() const;
    static SbBox3f getViewBoundingBox();
    void refreshCutRanges(
        SbBox3f,
        bool forXValue,
        bool forYValue,
        bool forZValue,
        bool forXRange,
        bool forYRange,
        bool forZRange
    );
    void adjustYZRanges(SbBox3f);
    void CutValueHelper(double val, QDoubleSpinBox* SpinBox, QSlider* Slider);
    void FlipClickedHelper(const char* BoxName);
    void changeCutBoxColors();
    void createAllObjects(const std::vector<App::DocumentObject*>& ObjectsListCut);
    App::DocumentObject* CreateBooleanFragments(App::Document* doc);
    App::DocumentObject* createBooleanFragments(
        const std::vector<App::DocumentObject*>& links,
        int transparency
    );
    Part::Compound* createCompound(const std::vector<App::DocumentObject*>& links, int transparency);
    void setBooleanFragmentsColor();
    Part::Box* findCutBox(const char* name) const;
    App::DocumentObject* findObject(const char* objName) const;
    App::DocumentObject* findOrCreateObject(const char* objName);
    void hideCutObjects();
    void deleteObejcts();
    void deleteCompound();
    void restoreVisibility();
    std::tuple<Base::Vector3f, Base::Vector3f> adjustRanges();
    void adjustYRange();
    void adjustZRange();
    void resetHasBoxes();
    App::DocumentObject* getCutXBase(size_t num, App::DocumentObject* comp, App::DocumentObject* frag) const;
    App::DocumentObject* getCutYBase(size_t num, App::DocumentObject* comp, App::DocumentObject* frag) const;
    App::DocumentObject* getCutZBase(size_t num, App::DocumentObject* comp, App::DocumentObject* frag) const;
    Part::Cut* createCut(const char* name);
    Part::Cut* tryCreateCut(const char* name);
    Part::Box* createBox(const char* name, const Base::Vector3f& size);
    std::tuple<Part::Box*, Part::Cut*> tryCreateXBoxAndCut(
        const Base::Vector3f& pos,
        const Base::Vector3f& size
    );
    Part::Box* tryCreateXBox(const Base::Vector3f& pos, const Base::Vector3f& size);
    Part::Box* createXBox(const Base::Vector3f& pos, const Base::Vector3f& size);
    std::tuple<Part::Box*, Part::Cut*> tryCreateYBoxAndCut(
        const Base::Vector3f& pos,
        const Base::Vector3f& size
    );
    Part::Box* tryCreateYBox(const Base::Vector3f& pos, const Base::Vector3f& size);
    Part::Box* createYBox(const Base::Vector3f& pos, const Base::Vector3f& size);
    std::tuple<Part::Box*, Part::Cut*> tryCreateZBoxAndCut(
        const Base::Vector3f& pos,
        const Base::Vector3f& size
    );
    Part::Box* tryCreateZBox(const Base::Vector3f& pos, const Base::Vector3f& size);
    Part::Box* createZBox(const Base::Vector3f& pos, const Base::Vector3f& size);

    double getPosX(Part::Box* box) const;
    double getPosY(Part::Box* box) const;
    double getPosZ(Part::Box* box) const;

private:
    std::unique_ptr<Ui_SectionCut> ui;
    std::vector<App::DocumentObjectT> ObjectsListVisible;
    App::Document* doc = nullptr;  // pointer to active document
    bool hasBoxX = false;
    bool hasBoxY = false;
    bool hasBoxZ = false;
    bool hasBoxCustom = false;
    const char* CompoundName = "SectionCutCompound";
    const char* BoxXName = "SectionCutBoxX";
    const char* BoxYName = "SectionCutBoxY";
    const char* BoxZName = "SectionCutBoxZ";
    const char* CutXName = "SectionCutX";
    const char* CutYName = "SectionCutY";
    const char* CutZName = "SectionCutZ";
};

}  // namespace PartGui
