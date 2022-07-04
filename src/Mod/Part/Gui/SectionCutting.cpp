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

#include "PreCompiled.h"

#ifndef _PreComp_

// to avoid compiler warnings of redefining contents of basic.h
// later by #include <Gui/ViewProviderGeometryObject.h>
# define _USE_MATH_DEFINES
# include <cmath>

# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <QDialog>
# include <QDockWidget>
# include <QDoubleSpinBox>
# include <QSlider>
# include <QToolTip>
#endif

#include <App/Document.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/PrefWidgets.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Mod/Part/App/FeatureCompound.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/FeaturePartCommon.h>
#include <Mod/Part/App/FeaturePartCut.h>
#include <Mod/Part/App/FeaturePartFuse.h>
#include <Mod/Part/App/PartFeatures.h>

#include "SectionCutting.h"
#include "ui_SectionCutting.h"


using namespace PartGui;

enum Refresh : bool
{
    notXValue = false,
    notYValue = false,
    notZValue = false,
    notXRange = false,
    notYRange = false,
    notZRange = false,
    XValue = true,
    YValue = true,
    ZValue = true,
    XRange = true,
    YRange = true,
    ZRange = true
};

SectionCut::SectionCut(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_SectionCut)
{
    // create widgets
    ui->setupUi(this);
    ui->cutX->setRange(-INT_MAX, INT_MAX);
    ui->cutY->setRange(-INT_MAX, INT_MAX);
    ui->cutZ->setRange(-INT_MAX, INT_MAX);

    // get all objects in the document
    auto docGui = Gui::Application::Instance->activeDocument();
    if (!docGui) {
        Base::Console().Error("SectionCut error: there is no document\n");
        return;
    }
    doc = docGui->getDocument();
    if (!doc) {
        Base::Console().Error("SectionCut error: there is no document\n");
        return;
    }

    std::vector<App::DocumentObject*> ObjectsList = doc->getObjects();
    if (ObjectsList.empty()) {
        Base::Console().Error("SectionCut error: there are no objects in the document\n");
        return;
    }
    // now store those that are currently visible
    for (auto it = ObjectsList.begin(); it != ObjectsList.end(); ++it) {
        if ((*it)->Visibility.getValue())
            ObjectsListVisible.push_back(*it);
    }

    // lambda function to set color and transparency
    auto setColorTransparency = [&](Part::Box* pcBox) {
        App::Color cutColor;
        long cutTransparency;
        auto vpBox = dynamic_cast<Gui::ViewProviderGeometryObject*>(
            Gui::Application::Instance->getViewProvider(pcBox));
        if (vpBox) {
            cutColor = vpBox->ShapeColor.getValue();
            cutTransparency = vpBox->Transparency.getValue();
            ui->CutColor->setColor(cutColor.asValue<QColor>());
            ui->CutTransparency->setValue(cutTransparency);
            ui->CutTransparency->setToolTip(QString::number(cutTransparency) + QString::fromLatin1(" %"));
        }
    };

    // if we can have existing cut boxes, take their values
    // to access the flip state we must compare the bounding boxes of the cutbox and the compound
    Base::BoundBox3d BoundCompound;
    Base::BoundBox3d BoundCutBox;
    if (doc->getObject(BoxXName) || doc->getObject(BoxYName) || doc->getObject(BoxZName)) {
        // automatic coloring must be disabled
        ui->AutoCutfaceColor->setChecked(false);
        if (doc->getObject(CompoundName)) {
            auto compoundObject = doc->getObject(CompoundName);
            Part::Compound* pcCompound = dynamic_cast<Part::Compound*>(compoundObject);
            if (!pcCompound) {
                Base::Console().Error("SectionCut error: compound is incorrectly named, cannot proceed\n");
                return;
            }
            BoundCompound = pcCompound->Shape.getBoundingBox();
        }
    }
    if (doc->getObject(BoxZName)) {
        Part::Box* pcBox = dynamic_cast<Part::Box*>(doc->getObject(BoxZName));
        if (!pcBox) {
            Base::Console().Error("SectionCut error: cut box is incorrectly named, cannot proceed\n");
            return;
        }
        hasBoxZ = true;
        ui->groupBoxZ->setChecked(true);
        // if z of cutbox bounding is greater than z of compound bounding
        // we know that the cutbox is in flipped state
        BoundCutBox = pcBox->Shape.getBoundingBox();
        if (BoundCutBox.MinZ > BoundCompound.MinZ){
            ui->cutZ->setValue(pcBox->Placement.getValue().getPosition().z);
            ui->flipZ->setChecked(true);
        }
        else {
            ui->cutZ->setValue(pcBox->Height.getValue() + pcBox->Placement.getValue().getPosition().z);
            ui->flipZ->setChecked(false);
        }
        // set color and transparency
        setColorTransparency(pcBox);
    }
    if (doc->getObject(BoxYName)) {
        Part::Box* pcBox = dynamic_cast<Part::Box*>(doc->getObject(BoxYName));
        if (!pcBox) {
            Base::Console().Error("SectionCut error: cut box is incorrectly named, cannot proceed\n");
            return;
        }
        hasBoxY = true;
        ui->groupBoxY->setChecked(true);
        BoundCutBox = pcBox->Shape.getBoundingBox();
        if (BoundCutBox.MinY > BoundCompound.MinY) {
            ui->cutY->setValue(pcBox->Placement.getValue().getPosition().y);
            ui->flipY->setChecked(true);
        }
        else {
            ui->cutY->setValue(pcBox->Width.getValue() + pcBox->Placement.getValue().getPosition().y);
            ui->flipY->setChecked(false);
        }
        setColorTransparency(pcBox);
    }
    if (doc->getObject(BoxXName)) {
        Part::Box* pcBox = dynamic_cast<Part::Box*>(doc->getObject(BoxXName));
        if (!pcBox) {
            Base::Console().Error("SectionCut error: cut box is incorrectly named, cannot proceed\n");
            return;
        }
        hasBoxX = true;
        ui->groupBoxX->setChecked(true);
        BoundCutBox = pcBox->Shape.getBoundingBox();
        if (BoundCutBox.MinX > BoundCompound.MinX) {
            ui->cutX->setValue(pcBox->Placement.getValue().getPosition().x);
            ui->flipX->setChecked(true);
        }
        else {
            ui->cutX->setValue(pcBox->Length.getValue() + pcBox->Placement.getValue().getPosition().x);
            ui->flipX->setChecked(false);
        }
        setColorTransparency(pcBox);
    }

    // hide existing cuts to check if there are objects to be cut visible
    if (doc->getObject(CutXName))
        doc->getObject(CutXName)->Visibility.setValue(false);
    if (doc->getObject(CutYName))
        doc->getObject(CutYName)->Visibility.setValue(false);
    if (doc->getObject(CutZName))
        doc->getObject(CutZName)->Visibility.setValue(false);

    // get bounding box
    SbBox3f box = getViewBoundingBox();
    if (!box.isEmpty()) {
        // if there is a cut box, perform the cut
        if (hasBoxX || hasBoxY || hasBoxZ) {
            // refresh only the range since we set the values above already
            refreshCutRanges(box, Refresh::notXValue, Refresh::notYValue, Refresh::notZValue,
                Refresh::XRange, Refresh::YRange, Refresh::ZRange);
        }
        else
            refreshCutRanges(box);
    }
    // the case of an empty box and having cuts will be handles later by startCutting(true)

    connect(ui->groupBoxX, &QGroupBox::toggled, this, &SectionCut::onGroupBoxXtoggled);
    connect(ui->groupBoxY, &QGroupBox::toggled, this, &SectionCut::onGroupBoxYtoggled);
    connect(ui->groupBoxZ, &QGroupBox::toggled, this, &SectionCut::onGroupBoxZtoggled);
    connect(ui->cutX, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SectionCut::onCutXvalueChanged);
    connect(ui->cutY, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SectionCut::onCutYvalueChanged);
    connect(ui->cutZ, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SectionCut::onCutZvalueChanged);
    connect(ui->cutXHS, &QSlider::sliderMoved, this, &SectionCut::onCutXHSsliderMoved);
    connect(ui->cutYHS, &QSlider::sliderMoved, this, &SectionCut::onCutYHSsliderMoved);
    connect(ui->cutZHS, &QSlider::sliderMoved, this, &SectionCut::onCutZHSsliderMoved);
    connect(ui->cutXHS, &QSlider::valueChanged, this, &SectionCut::onCutXHSChanged);
    connect(ui->cutYHS, &QSlider::valueChanged, this, &SectionCut::onCutYHSChanged);
    connect(ui->cutZHS, &QSlider::valueChanged, this, &SectionCut::onCutZHSChanged);
    connect(ui->flipX, &QPushButton::clicked, this, &SectionCut::onFlipXclicked);
    connect(ui->flipY, &QPushButton::clicked, this, &SectionCut::onFlipYclicked);
    connect(ui->flipZ, &QPushButton::clicked, this, &SectionCut::onFlipZclicked);
    connect(ui->RefreshCutPB, &QPushButton::clicked, this, &SectionCut::onRefreshCutPBclicked);
    connect(ui->CutColor, &QPushButton::clicked, this, &SectionCut::onCutColorclicked);
    connect(ui->CutTransparency, &QSlider::sliderMoved, this, &SectionCut::onTransparencySliderMoved);
    connect(ui->CutTransparency, &QSlider::valueChanged, this, &SectionCut::onTransparencyChanged);
    
    // if there is a cut, perform it
    if (hasBoxX || hasBoxY || hasBoxZ) {
        ui->RefreshCutPB->setEnabled(false);
        startCutting(true);
    }
}

// actions to be done when document was closed
void SectionCut::noDocumentActions()
{
    ui->groupBoxX->blockSignals(true);
    ui->groupBoxY->blockSignals(true);
    ui->groupBoxZ->blockSignals(true);
    doc = nullptr;
    // reset the cut group boxes
    ui->groupBoxX->setChecked(false);
    ui->groupBoxY->setChecked(false);
    ui->groupBoxZ->setChecked(false);
    ui->RefreshCutPB->setEnabled(true);
    ui->groupBoxX->blockSignals(false);
    ui->groupBoxY->blockSignals(false);
    ui->groupBoxZ->blockSignals(false);
}

void SectionCut::startCutting(bool isInitial)
{
    // there might be no document
    if (!Gui::Application::Instance->activeDocument()) {
        noDocumentActions();
        return;
    }
    // the document might have been changed
    if (doc != Gui::Application::Instance->activeDocument()->getDocument())
        // refresh documents list
        onRefreshCutPBclicked();

    App::DocumentObject* anObject = nullptr;
    std::vector<App::DocumentObjectT>::iterator it;

    // lambda function to delete objects
    auto deleteObject = [&](const char* objectName) {
        anObject = doc->getObject(objectName);
        // the deleted object might have been visible before, thus check and delete it from the list
        auto found = std::find_if(ObjectsListVisible.begin(), ObjectsListVisible.end(), [anObject](const App::DocumentObjectT& obj) {
            return (obj.getObject() == anObject);
        });
        if (found != ObjectsListVisible.end())
            ObjectsListVisible.erase(found);
        doc->removeObject(objectName);
    };

    // delete the objects we might have already created to cut
    // we must do this because we support several cuts at once and
    // it is dangerous to deal with the fact that the user is free
    // to uncheck cutting planes and to add/remove objects while this dialog is open
    // We must remove in this order because the tree hierary of the features is Z->Y->X and Cut->Box
    
    if (doc->getObject(CutZName))
        deleteObject(CutZName);
    if (doc->getObject(BoxZName))
        deleteObject(BoxZName);
    if (doc->getObject(CutYName))
        deleteObject(CutYName);
    if (doc->getObject(BoxYName))
        deleteObject(BoxYName);
    if (doc->getObject(CutXName))
        deleteObject(CutXName);
    if (doc->getObject(BoxXName))
        deleteObject(BoxXName);
    if (doc->getObject(CompoundName)) {
        auto compoundObject = doc->getObject(CompoundName);
        Part::Compound* pcCompoundDel = dynamic_cast<Part::Compound*>(compoundObject);
        if (!pcCompoundDel) {
            Base::Console().Error("SectionCut error: compound is incorrectly named, cannot proceed\n");
            return;
        }
        std::vector<App::DocumentObject*> compoundObjects;
        pcCompoundDel->Links.getLinks(compoundObjects);
        // first delete the compound
        auto foundObj = std::find_if(ObjectsListVisible.begin(), ObjectsListVisible.end(), [anObject](const App::DocumentObjectT& obj) {
            return (obj.getObject() == anObject);
        });
        if (foundObj != ObjectsListVisible.end())
            ObjectsListVisible.erase(foundObj);
        doc->removeObject(CompoundName);
        // now delete the objects that have been part of the compound
        for (auto itCompound = compoundObjects.begin(); itCompound != compoundObjects.end(); itCompound++) {
            anObject = doc->getObject((*itCompound)->getNameInDocument());
            auto foundObjInner = std::find_if(ObjectsListVisible.begin(), ObjectsListVisible.end(), [anObject](const App::DocumentObjectT& objInner) {
                return (objInner.getObject() == anObject);
            });
            if (foundObjInner != ObjectsListVisible.end())
                ObjectsListVisible.erase((foundObjInner));
            doc->removeObject((*itCompound)->getNameInDocument());
        }
    }

    // make all objects visible that have been visible when the dialog was called
    // because we made them invisible when we created cuts
    for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
        if (it->getObject()) // a formerly visible object might have been deleted
            it->getObject()->Visibility.setValue(true);
        else {
            // we must refresh the ObjectsListVisible list
            onRefreshCutPBclicked();
        }
    }

    // we enable the sliders because for assemblies we disabled them
    ui->cutXHS->setEnabled(true);
    ui->cutYHS->setEnabled(true);
    ui->cutZHS->setEnabled(true);

    // ObjectsListVisible contains all visible objects of the document, but we can only cut
    // those that have a solid shape
    std::vector<App::DocumentObject*> ObjectsListCut;
    bool isLinkAssembly = false;
    for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
        // we need all Link objects in App::Part for example for Assembly 4
        if (it->getObject()->getTypeId() == Base::Type::fromName("App::Part")) {
            App::Part* pcPart = static_cast<App::Part*>(it->getObject());
           
            // collect all its link objects
            auto groupObjects = pcPart->Group.getValue();
            for (auto itGO = groupObjects.begin(); itGO != groupObjects.end(); ++itGO) {
                if ((*itGO)->getTypeId() == Base::Type::fromName("App::Link")) {
                    ObjectsListCut.push_back((*itGO));
                    // we assume that App::Links inside a App::Part are an assembly
                    isLinkAssembly = true;
                }
            }
        }
        // get all shapes that are also Part::Features
        if (it->getObject()->getPropertyByName("Shape")
            && it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Feature"))) {
            // sort out 2D objects, datums, App:Parts, compounds and objects that are part of a PartDesign body
            if (!it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Part2DObject"))
                && !it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Datum"))
                && !it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Feature"))
                && !it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Compound"))
                && it->getObject()->getTypeId() != Base::Type::fromName("App::Part"))
                ObjectsListCut.push_back(it->getObject());
        }
        // get Links that are derived from Part objects
        if (it->getObject()->getTypeId() == Base::Type::fromName("App::Link")) {
            App::Link* pcLink = static_cast<App::Link*>(it->getObject());
            auto linkedObject = doc->getObject(pcLink->LinkedObject.getObjectName());
            if (linkedObject && linkedObject->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Feature")))
                ObjectsListCut.push_back(it->getObject());
        }
    }

    if (isLinkAssembly) {
        // we disable the sliders because for assemblies it will takes ages to do several dozen recomputes
        QString SliderToolTip = tr("Sliders are disabled for assemblies");
        ui->cutXHS->setEnabled(false);
        ui->cutXHS->setToolTip(SliderToolTip);
        ui->cutYHS->setEnabled(false);
        ui->cutYHS->setToolTip(SliderToolTip);
        ui->cutZHS->setEnabled(false);
        ui->cutZHS->setToolTip(SliderToolTip);
    }
    
    // sort out objects that are part of Part::Boolean, Part::MultiCommon, Part::MultiFuse,
    // Part::Thickness and Part::FilletBase
    std::vector<App::DocumentObject*>::iterator it2;
    std::vector<App::DocumentObject*>::iterator it3;
    // check list of visible objects and not cut list because we want to repove from the cut list
    for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
        if ( it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Boolean"))
            || it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::MultiCommon"))
            || it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::MultiFuse"))
            || it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Thickness"))
            || it->getObject()->getTypeId().isDerivedFrom(Base::Type::fromName("Part::FilletBase")) ) {
            // get possible links
            auto subObjectList = it->getObject()->getOutList();
            // if there are links, delete them
            if (!subObjectList.empty()) {
                for (it2 = subObjectList.begin(); it2 != subObjectList.end(); ++it2) {
                    for (it3 = ObjectsListCut.begin(); it3 != ObjectsListCut.end(); ++it3) {
                        if ((*it2) == (*it3)) {
                            ObjectsListCut.erase(it3);
                            break;
                        }
                    }
                }
            }
        }
    }

    // we might have no objects that can be cut
    if (ObjectsListCut.empty()) {
        if (isInitial)
            Base::Console().Error("SectionCut error: there are no visible objects to be cut\n");
        else
            Base::Console().Error("SectionCut error: there are no objects in the document that can be cut\n");
        // block signals to be able to reset the cut group boxes without calling startCutting again
        ui->groupBoxX->blockSignals(true);
        ui->groupBoxY->blockSignals(true);
        ui->groupBoxZ->blockSignals(true);
        ui->groupBoxX->setChecked(false);
        ui->groupBoxY->setChecked(false);
        ui->groupBoxZ->setChecked(false);
        ui->RefreshCutPB->setEnabled(true);
        ui->groupBoxX->blockSignals(false);
        ui->groupBoxY->blockSignals(false);
        ui->groupBoxZ->blockSignals(false);
        return;
    }
    
    // we cut this way:
    // 1. put all existing objects into a part compound
    // 2. create a box with the size of the bounding box
    // 3. cut the box from the compound
    
    // depending on how many cuts should be performed, we need as many boxes
    // if nothing is yet to be cut, we can return
    if (!ui->groupBoxX->isChecked() && !ui->groupBoxY->isChecked()
        && !ui->groupBoxZ->isChecked()) {
        // there is no active cut, so we can enable refresh button
        ui->RefreshCutPB->setEnabled(true);
        return;
    }

    // disable refresh button
    ui->RefreshCutPB->setEnabled(false);

    // create an empty compound
    auto CutCompound = doc->addObject("Part::Compound", CompoundName);
    if (!CutCompound) {
        Base::Console().Error( (std::string("SectionCut error: ")
            + std::string(CompoundName) + std::string(" could not be added\n")).c_str() );
        return;
    }
    Part::Compound* pcCompound = static_cast<Part::Compound*>(CutCompound);
    // store color and transparency of first object
    App::Color cutColor;
    int cutTransparency;
    bool autoColor = true;
    bool autoTransparency = true;
    auto vpFirstObject = dynamic_cast<Gui::ViewProviderGeometryObject*>(
        Gui::Application::Instance->getViewProvider(*ObjectsListCut.begin()));
    if (vpFirstObject) {
        cutColor = vpFirstObject->ShapeColor.getValue();
        cutTransparency = vpFirstObject->Transparency.getValue();
    }
    // fill it with all found elements with the copies of the elements
    int count = 0;
    for (auto itCuts = ObjectsListCut.begin(); itCuts != ObjectsListCut.end(); ++itCuts, count++) {
        // first create a link with a unique name
        std::string newName;
        // since links to normal Part objects all have the document name "Link", use their label text instead
        if ((*itCuts)->getTypeId() == Base::Type::fromName("App::Link"))
             newName = (*itCuts)->Label.getValue();
        else
            newName = (*itCuts)->getNameInDocument();
        newName = newName + "_CutLink";
        
        auto newObject = doc->addObject("App::Link", newName.c_str());
        if (!newObject) {
            Base::Console().Error("SectionCut error: 'App::Link' could not be added\n");
            return;
        }
        App::Link* pcLink = static_cast<App::Link*>(newObject);
        // set the object to the created empty link object
        pcLink->LinkedObject.setValue((*itCuts));
        // we want to get the link at the same position as the original
        pcLink->LinkTransform.setValue(true); 

        // if the object is part of an App::Part container, the link needs to get the container placement
        auto parents = (*itCuts)->getInList();
        if (!parents.empty()) {
            for (auto itParents = parents.begin(); itParents != parents.end(); ++itParents) {
                if ((*itParents)->getTypeId() == Base::Type::fromName("App::Part")) {
                    App::Part* pcPartParent = static_cast<App::Part*>((*itParents));
                    auto placement = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                                      pcPartParent->getPropertyByName("Placement"));
                    if (placement)
                        pcLink->Placement.setValue(placement->getValue());
                }
            }
        }

        // add the link to the compound
        pcCompound->Links.set1Value(count, newObject);

        // hide the objects since only the cut should later be visible
        (*itCuts)->Visibility.setValue(false);

        // check if all objects have same color and transparency
        if (ui->AutoCutfaceColor->isChecked()) {
            auto vpObject = dynamic_cast<Gui::ViewProviderGeometryObject*>(
                Gui::Application::Instance->getViewProvider(*itCuts));
            if (vpObject) {
                if (cutColor != vpObject->ShapeColor.getValue())
                    autoColor = false;
                if (cutTransparency != vpObject->Transparency.getValue())
                    autoTransparency = false;
            }
        }
    }

    // compute the filled compound
    pcCompound->recomputeFeature();

    // make all objects invisible so that only the compound remains
    for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
        it->getObject()->Visibility.setValue(false);
    }

    // the area in which we can cut is the size of the compound
    // we get its size by its bounding box
    SbBox3f CompoundBoundingBox = getViewBoundingBox();
    if (CompoundBoundingBox.isEmpty()) {
        Base::Console().Error("SectionCut error: the CompoundBoundingBox is empty\n");
        return;
    }

    // store the current cut positions te reset them later if possible
    double CutPosX = ui->cutX->value();
    double CutPosY = ui->cutY->value();
    double CutPosZ = ui->cutZ->value();

    // refresh all cut limits according to the new bounding box
    refreshCutRanges(CompoundBoundingBox);
        
    // prepare the cut box size according to the bounding box size
    std::vector<float> BoundingBoxSize = { 0.0, 0.0, 0.0 };
    CompoundBoundingBox.getSize(BoundingBoxSize[0], BoundingBoxSize[1], BoundingBoxSize[2]);
    // get placement of the bounding box origin
    std::vector<float> BoundingBoxOrigin = { 0.0, 0.0, 0.0 };
    CompoundBoundingBox.getOrigin(BoundingBoxOrigin[0], BoundingBoxOrigin[1], BoundingBoxOrigin[2]);

    // now we can create the cut boxes
    Base::Vector3d BoxOriginSet;
    Base::Placement placement;
    SbBox3f CutBoundingBox;
    hasBoxX = false;
    hasBoxY = false;
    hasBoxZ = false;
    hasBoxCustom = false;

    // if automatic, we take this color for the cut
    if (ui->AutoCutfaceColor->isChecked()) {
        if (autoColor) {
            ui->CutColor->blockSignals(true);
            ui->CutColor->setColor(cutColor.asValue<QColor>());
            ui->CutColor->blockSignals(false);
        }
        if (autoTransparency) {
            ui->CutTransparency->blockSignals(true);
            ui->CutTransparency->setValue(cutTransparency);
            ui->CutTransparency->setToolTip(QString::number(cutTransparency) + QString::fromLatin1(" %"));
            ui->CutTransparency->blockSignals(false);
        }
    }

    // read cutface color for the cut box
    App::Color boxColor;
    boxColor.setValue<QColor>(ui->CutColor->color());
    int boxTransparency = ui->CutTransparency->value();

    if (ui->groupBoxX->isChecked()) {
        // create a box
        auto CutBox = doc->addObject("Part::Box", BoxXName);
        if (!CutBox) {
            Base::Console().Error( (std::string("SectionCut error: ")
                + std::string(BoxXName) + std::string(" could not be added\n")).c_str() );
            return;
        }
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        // it appears that because of internal rounding errors, the bounding box is sometimes
        // a bit too small, for example for epplipsoides, thus make the box a bit larger
        pcBox->Length.setValue(BoundingBoxSize[0] + 1.0);
        pcBox->Width.setValue(BoundingBoxSize[1] + 1.0);
        pcBox->Height.setValue(BoundingBoxSize[2] + 1.0);
        // set the previous cut value because refreshCutRanges changed it
        // in case the there was previously no cut, nothing will actually be changed
        // the previous value might now be outside the current possible range, then reset it
        if (CutPosX >= ui->cutX->maximum()) {
            CutPosX = ui->cutX->maximum() - 0.1; // short below the maximum
        }
        else if (CutPosX <= ui->cutX->minimum()) {
            CutPosX = ui->cutX->minimum() + 0.1; // short above the minimum
        }
        // we don't set the value to ui->cutX because this would refresh the cut
        // which we don't have yet, thus do this later
        //set the box position
        if (!ui->flipX->isChecked())
            BoxOriginSet.x = CutPosX - (BoundingBoxSize[0] + 1.0);
        else //flipped
            BoxOriginSet.x = CutPosX;
        // we made the box 1.0 larger that we can place it 0.5 below the bounding box
        BoxOriginSet.y = BoundingBoxOrigin[1] - 0.5;
        BoxOriginSet.z = BoundingBoxOrigin[2] - 0.5;
        placement.setPosition(BoxOriginSet);
        // set box color
        pcBox->Placement.setValue(placement);
        auto vpBox = dynamic_cast<Gui::ViewProviderGeometryObject*>(
            Gui::Application::Instance->getViewProvider(pcBox));
        if (vpBox) {
            vpBox->ShapeColor.setValue(boxColor);
            vpBox->Transparency.setValue(boxTransparency);
        }

        // create a cut feature
        auto CutFeature = doc->addObject("Part::Cut", CutXName);
        if (!CutFeature) {
            Base::Console().Error( (std::string("SectionCut error: ")
                + std::string(CutXName) + std::string(" could not be added\n")).c_str() );
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutFeature);
        pcCut->Base.setValue(CutCompound);
        pcCut->Tool.setValue(CutBox);

        // set the cut value
        ui->cutX->setValue(CutPosX);
        // recomputing recursively is especially for assemblies very time-consuming
        // however there must be a final recursicve recompute and we do this at the end
        // so only recomute recursively if there are no other cuts
        if (!ui->groupBoxY->isChecked() && !ui->groupBoxZ->isChecked())
            pcCut->recomputeFeature(true);
        else
            pcCut->recomputeFeature(false);
        hasBoxX = true;
    }
    if (ui->groupBoxY->isChecked()) {
        // if there is a X cut, its size defines the possible range for the Y cut
        // the cut box size is not affected, it can be as large as the compound
        if (hasBoxX) {
            CutBoundingBox = getViewBoundingBox();
            // refresh the Y cut limits according to the new bounding box
            refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::notZValue,
                Refresh::notXRange, Refresh::YRange, Refresh::notZRange);
        }
        auto CutBox = doc->addObject("Part::Box", BoxYName);
        if (!CutBox) {
            Base::Console().Error((std::string("SectionCut error: ")
                + std::string(BoxYName) + std::string(" could not be added\n")).c_str() );
            return;
        }
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        pcBox->Length.setValue(BoundingBoxSize[0] + 1.0);
        pcBox->Width.setValue(BoundingBoxSize[1] + 1.0);
        pcBox->Height.setValue(BoundingBoxSize[2] + 1.0);
        // reset previous cut value
        if (CutPosY >= ui->cutY->maximum()) {
            CutPosY = ui->cutY->maximum() - 0.1; // short below the maximum
        }
        else if (CutPosY <= ui->cutY->minimum()) {
            CutPosY = ui->cutY->minimum() + 0.1; // short above the minimum
        }
        //set the box position
        BoxOriginSet.x = BoundingBoxOrigin[0] - 0.5;
        if (!ui->flipY->isChecked())
            BoxOriginSet.y = CutPosY - (BoundingBoxSize[1] + 1.0);
        else //flipped
            BoxOriginSet.y = CutPosY;
        BoxOriginSet.z = BoundingBoxOrigin[2] - 0.5;
        placement.setPosition(BoxOriginSet);
        pcBox->Placement.setValue(placement);
        auto vpBox = dynamic_cast<Gui::ViewProviderGeometryObject*>(
            Gui::Application::Instance->getViewProvider(pcBox));
        if (vpBox) {
            vpBox->ShapeColor.setValue(boxColor);
            vpBox->Transparency.setValue(boxTransparency);
        }
        
        auto CutFeature = doc->addObject("Part::Cut", CutYName);
        if (!CutFeature) {
            Base::Console().Error((std::string("SectionCut error: ")
                + std::string(CutYName) + std::string(" could not be added\n")).c_str() );
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutFeature);
        // if there is already a cut, we must take it as feature to be cut
        if (hasBoxX)
            pcCut->Base.setValue(doc->getObject(CutXName));
        else
            pcCut->Base.setValue(CutCompound);
        pcCut->Tool.setValue(CutBox);
        
        // set the cut value
        ui->cutY->setValue(CutPosY);
        if (!ui->groupBoxZ->isChecked())
            pcCut->recomputeFeature(true);
        else
            pcCut->recomputeFeature(false);
        hasBoxY = true;
    }
    if (ui->groupBoxZ->isChecked()) {
        if (hasBoxX || hasBoxY) {
            CutBoundingBox = getViewBoundingBox();
            refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::notZValue,
                Refresh::notXRange, Refresh::notYRange, Refresh::ZRange);
        }
        auto CutBox = doc->addObject("Part::Box", BoxZName);
        if (!CutBox) {
            Base::Console().Error((std::string("SectionCut error: ")
                + std::string(BoxZName) + std::string(" could not be added\n")).c_str() );
            return;
        }
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        pcBox->Length.setValue(BoundingBoxSize[0] + 1.0);
        pcBox->Width.setValue(BoundingBoxSize[1] + 1.0);
        pcBox->Height.setValue(BoundingBoxSize[2] + 1.0);
        // reset previous cut value
        if (CutPosZ >= ui->cutZ->maximum()) {
            CutPosZ = ui->cutZ->maximum() - 0.1; // short below the maximum
        }
        else if (CutPosZ <= ui->cutZ->minimum()) {
            CutPosZ = ui->cutZ->minimum() + 0.1; // short above the minimum
        }
        //set the box position
        BoxOriginSet.x = BoundingBoxOrigin[0] - 0.5;
        BoxOriginSet.y = BoundingBoxOrigin[1] - 0.5;
        if (!ui->flipY->isChecked())
            BoxOriginSet.z = CutPosZ - (BoundingBoxSize[2] + 1.0);
        else //flipped
            BoxOriginSet.z = CutPosZ;
        placement.setPosition(BoxOriginSet);
        pcBox->Placement.setValue(placement);
        auto vpBox = dynamic_cast<Gui::ViewProviderGeometryObject*>(
            Gui::Application::Instance->getViewProvider(pcBox));
        if (vpBox) {
            vpBox->ShapeColor.setValue(boxColor);
            vpBox->Transparency.setValue(boxTransparency);
        }

        auto CutFeature = doc->addObject("Part::Cut", CutZName);
        if (!CutFeature) {
            Base::Console().Error( (std::string("SectionCut error: ")
                + std::string(CutZName) + std::string(" could not be added\n")).c_str() );
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutFeature);
        // if there is already a cut, we must take it as feature to be cut
        if (hasBoxY) {
            pcCut->Base.setValue(doc->getObject(CutYName));
        }
        else if (hasBoxX && !hasBoxY) {
            pcCut->Base.setValue(doc->getObject(CutXName));
        }
        else {
            pcCut->Base.setValue(CutCompound);
        }
        pcCut->Tool.setValue(CutBox);

        // set the cut value
        ui->cutZ->setValue(CutPosZ);
        pcCut->recomputeFeature(true);
        hasBoxZ = true;
    }
}

SectionCut* SectionCut::makeDockWidget(QWidget* parent)
{
    // embed this dialog into a QDockWidget
    SectionCut* sectionCut = new SectionCut(parent);
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    // the dialog is designed that you can see the tree, thus put it to the right side
    QDockWidget* dw = pDockMgr->addDockWindow("Section Cutting", sectionCut, Qt::RightDockWidgetArea);
    dw->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    //dw->setFloating(true);
    dw->show();

    return sectionCut;
}

/** Destroys the object and frees any allocated resources */
SectionCut::~SectionCut()
{
    // there might be no document
    if (!Gui::Application::Instance->activeDocument()) {
        noDocumentActions();
        return;
    }
    if (!ui->keepOnlyCutCB->isChecked()) {
        // make all objects visible that have been visible when the dialog was called
        // because we made them invisible when we created cuts
        for (auto it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
            if (it->getObject()) // a formerly visible object might have been deleted
                it->getObject()->Visibility.setValue(true);
        }
    }
}

void SectionCut::reject()
{
    QDialog::reject();
    QDockWidget* dw = qobject_cast<QDockWidget*>(parent());
    if (dw) {
        dw->deleteLater();
    }
}

void SectionCut::onGroupBoxXtoggled()
{
    // reset the cut
    startCutting();
}

void SectionCut::onGroupBoxYtoggled()
{
    startCutting();
}

void SectionCut::onGroupBoxZtoggled()
{
    startCutting();
}

// helper function for the onFlip_clicked signal
void SectionCut::CutValueHelper(double value, QDoubleSpinBox* SpinBox, QSlider* Slider)
{
    // there might be no document
    if (!Gui::Application::Instance->activeDocument()) {
        noDocumentActions();
        return;
    }
    // refresh objects and return in case the document was changed
    if (doc != Gui::Application::Instance->activeDocument()->getDocument()) {
        onRefreshCutPBclicked();
        return;
    }
    // update slider position and tooltip
    // the slider value is % of the cut range
    if (Slider->isEnabled()) {
        Slider->blockSignals(true);
        Slider->setValue(
            int((value - SpinBox->minimum())
                / (SpinBox->maximum() - SpinBox->minimum()) * 100.0));
        Slider->setToolTip(QString::number(value, 'g', Base::UnitsApi::getDecimals()));
        Slider->blockSignals(false);
    }

    // we cannot cut to the edge because then the result is an empty shape
        // we chose purposely not to simply set the range for cutX previously
        // because everything is allowed just not the min/max
    if (SpinBox->value() == SpinBox->maximum()) {
        SpinBox->setValue(SpinBox->maximum() - 0.1);
        return;
    }
    if (SpinBox->value() == SpinBox->minimum()) {
        SpinBox->setValue(SpinBox->minimum() + 0.1);
        return;
    }
}

void SectionCut::onCutXvalueChanged(double val)
{
    CutValueHelper(val, ui->cutX, ui->cutXHS);

    // get the cut box
    auto CutBox = doc->getObject(BoxXName);
    // when the value has been set after resetting the compound bounding box
    // there is not yet a cut and we do nothing
    if (!CutBox)
        return;
    Part::Box* pcBox = dynamic_cast<Part::Box*>(CutBox);
    if (!pcBox) {
        Base::Console().Error((std::string("SectionCut error: ") + std::string(BoxXName)
            + std::string(" is no Part::Box object. Cannot proceed.\n")).c_str());
        return;
    }
    // get its placement and size
    Base::Placement placement = pcBox->Placement.getValue();
    Base::Vector3d BoxPosition = placement.getPosition();
    // change the placement
    if (!ui->flipX->isChecked())
        BoxPosition.x = ui->cutX->value() - pcBox->Length.getValue();
    else //flipped
        BoxPosition.x = ui->cutX->value();
    placement.setPosition(BoxPosition);
    pcBox->Placement.setValue(placement);

    auto CutObject = doc->getObject(CutXName);
    // there should be a box, but maybe the user deleted it meanwhile
    if (!CutObject) {
        Base::Console().Warning((std::string("SectionCut warning: there is no ")
            + std::string(CutXName) + std::string(", trying to recreate it\n")).c_str());
        // recreate the box
        startCutting();
        return;
    }

    // if there is another cut, we must recalculate it too
    // we might have cut so that the range for Y and Z is now smaller
    // the hierarchy is always Z->Y->X
    if (hasBoxY && !hasBoxZ) { // only Y
        auto CutFeatureY = doc->getObject(CutYName);
        if (!CutFeatureY) {
            Base::Console().Warning((std::string("SectionCut warning: there is no ")
                + std::string(CutYName) + std::string(", trying to recreate it\n")).c_str());
            startCutting();
            return;
        }
        // refresh the Y and Z cut limits according to the new bounding box of the cut result
        // make the SectionCutY invisible
        CutFeatureY->Visibility.setValue(false);
        // make SectionCutX visible
        CutObject->Visibility.setValue(true);
        // get new bounding box
        auto CutBoundingBox = getViewBoundingBox();
        // refresh Y limits and Z limits + Z value
        refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::ZValue,
            Refresh::notXRange, Refresh::YRange, Refresh::ZRange);
        // the value of Y can now be outside or at the limit, in this case reset the value too
        if ((ui->cutY->value() >= ui->cutY->maximum())
            || (ui->cutY->value() <= ui->cutY->minimum()))
            refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::YValue, Refresh::ZValue,
                Refresh::notXRange, Refresh::YRange, Refresh::ZRange);
        // make the SectionCutY visible again
        CutFeatureY->Visibility.setValue(true);
        // make SectionCutX invisible again
        CutObject->Visibility.setValue(false);
        // recompute the cut
        Part::Cut* pcCutY = static_cast<Part::Cut*>(CutFeatureY);
        pcCutY->recomputeFeature(true);
    }
    else if (hasBoxZ) { // at least Z
        // the main cut is Z, no matter if there is a cut in Y
        auto CutFeatureZ = doc->getObject(CutZName);
        if (!CutFeatureZ) {
            Base::Console().Error((std::string("SectionCut error: there is no ")
                + std::string(CutZName) + std::string("\n")).c_str());
            return;
        }
        // refresh the Y and Z cut limits according to the new bounding box of the cut result
        // make the SectionCutZ invisible
        CutFeatureZ->Visibility.setValue(false);
        // make SectionCutX visible
        CutObject->Visibility.setValue(true);
        // get new bounding box
        auto CutBoundingBox = getViewBoundingBox();
        // refresh Y and Z limits
        if (hasBoxY) {
            refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::notZValue,
                Refresh::notXRange, Refresh::YRange, Refresh::ZRange);
            // the value of Y or Z can now be outside or at the limit, in this case reset the value too
            if ((ui->cutY->value() >= ui->cutY->maximum())
                || (ui->cutY->value() <= ui->cutY->minimum()))
                refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::YValue, Refresh::notZValue,
                    Refresh::notXRange, Refresh::YRange, Refresh::ZRange);
            if ((ui->cutZ->value() >= ui->cutZ->maximum())
                || (ui->cutZ->value() <= ui->cutZ->minimum()))
                refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::ZValue,
                    Refresh::notXRange, Refresh::YRange, Refresh::ZRange);
        }
        else { // there is no Y cut yet so we can set the Y value too
            refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::YValue, Refresh::notZValue,
                Refresh::notXRange, Refresh::YRange, Refresh::ZRange);
            // the value of Z can now be outside or at the limit, in this case reset the value too
            if ((ui->cutZ->value() >= ui->cutZ->maximum())
                || (ui->cutZ->value() <= ui->cutZ->minimum()))
                refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::YValue, Refresh::ZValue,
                    Refresh::notXRange, Refresh::YRange, Refresh::ZRange);
        }
        // make the SectionCutZ visible again
        CutFeatureZ->Visibility.setValue(true);
        // make SectionCutX invisible again
        CutObject->Visibility.setValue(false);
        // recompute the cut
        Part::Cut* pcCutZ = static_cast<Part::Cut*>(CutFeatureZ);
        pcCutZ->recomputeFeature(true);
    }
    else { // just X
        // refresh Y and Z limits + values
        auto CutBoundingBox = getViewBoundingBox();
        refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::YValue, Refresh::ZValue,
            Refresh::notXRange, Refresh::YRange, Refresh::ZRange);
        // recompute the cut
        Part::Cut* pcCut = dynamic_cast<Part::Cut*>(CutObject);
        if (!pcCut) {
            Base::Console().Error((std::string("SectionCut error: ") + std::string(CutZName)
                + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
            return;
        }
        pcCut->recomputeFeature(true);
    }
}

void SectionCut::onCutXHSsliderMoved(int val)
{
    // we cannot cut to the edge because then the result is an empty shape
    // we chose purposely not to simply set the range for cutXHS previously
    // because everything is allowed just not the min/max
    // we set it one slider step below the min/max
    if (val == ui->cutXHS->maximum()) {
        ui->cutXHS->setValue(ui->cutXHS->maximum() - ui->cutXHS->singleStep());
        return;
    }
    if (val == ui->cutXHS->minimum()) {
        ui->cutXHS->setValue(ui->cutXHS->minimum() + ui->cutXHS->singleStep());
        return;
    }
    // the slider value is % of the cut range
    double NewCutValue = ui->cutX->minimum()
        + val / 100.0 * (ui->cutX->maximum() - ui->cutX->minimum());
    ui->cutXHS->setToolTip(QString::number(NewCutValue, 'g', Base::UnitsApi::getDecimals()));
    ui->cutX->setValue(NewCutValue);
}

void SectionCut::onCutXHSChanged(int val)
{
    onCutXHSsliderMoved(val);
}

void SectionCut::onCutYvalueChanged(double val)
{
    CutValueHelper(val, ui->cutY, ui->cutYHS);

    auto CutBox = doc->getObject(BoxYName);
    if (!CutBox)
        return;
    Part::Box* pcBox = dynamic_cast<Part::Box*>(CutBox);
    if (!pcBox) {
        Base::Console().Error((std::string("SectionCut error: ") + std::string(BoxYName)
            + std::string(" is no Part::Box object. Cannot proceed.\n")).c_str());
        return;
    }
    Base::Placement placement = pcBox->Placement.getValue();
    Base::Vector3d BoxPosition = placement.getPosition();
    if (!ui->flipY->isChecked())
        BoxPosition.y = ui->cutY->value() - pcBox->Width.getValue();
    else //flipped
        BoxPosition.y = ui->cutY->value();
    placement.setPosition(BoxPosition);
    pcBox->Placement.setValue(placement);

    auto CutObject = doc->getObject(CutYName);
    if (!CutObject) {
        Base::Console().Warning((std::string("SectionCut warning: there is no ")
            + std::string(CutYName) + std::string(", trying to recreate it\n")).c_str());
        startCutting();
        return;
    }

    // if there is another cut, we must recalculate it too
    // we might have cut so that the range for Z is now smaller
    // we only need to check for Z since the hierarchy is always Z->Y->X
    if (hasBoxZ) {
        auto CutFeatureZ = doc->getObject(CutZName);
        if (!CutFeatureZ) {
            Base::Console().Error((std::string("SectionCut error: there is no ")
                + std::string(CutZName) + std::string("\n")).c_str());
            return;
        }
        // refresh the Z cut limits according to the new bounding box of the cut result
        // make the SectionCutZ invisible
        CutFeatureZ->Visibility.setValue(false);
        // make SectionCutX visible
        CutObject->Visibility.setValue(true);
        // get new bounding box
        auto CutBoundingBox = getViewBoundingBox();
        // refresh Z limits
        refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::notZValue,
            Refresh::notXRange, Refresh::notYRange, Refresh::ZRange);
        // the value of Z can now be outside or at the limit, in this case reset the value too
        if ((ui->cutZ->value() >= ui->cutZ->maximum())
            || (ui->cutZ->value() <= ui->cutZ->minimum()))
            refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::ZValue,
                Refresh::notXRange, Refresh::notYRange, Refresh::ZRange);
        // make the SectionCutZ visible again
        CutFeatureZ->Visibility.setValue(true);
        // make SectionCutX invisible again
        CutObject->Visibility.setValue(false);
        // recompute the cut
        Part::Cut* pcCutZ = static_cast<Part::Cut*>(CutFeatureZ);
        pcCutZ->recomputeFeature(true);
    }
    else { // just Y
        // refresh Z limits + values
        auto CutBoundingBox = getViewBoundingBox();
        refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::ZValue,
            Refresh::notXRange, Refresh::notYRange, Refresh::ZRange);
        // recompute the cut
        Part::Cut* pcCut = dynamic_cast<Part::Cut*>(CutObject);
        if (!pcCut) {
            Base::Console().Error((std::string("SectionCut error: ") + std::string(CutZName)
                + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
            return;
        }
        pcCut->recomputeFeature(true);
        // refresh X limits
        // this is done by
        // first making the cut X box visible, the setting the limits only for X
        // if x-limit in box direcion is larger than object, reset value to saved limit
        if (hasBoxX) {
            auto CutBoxX = doc->getObject(BoxXName);
            if (!CutBoxX)
                return;
            // first store the values
            double storedX;
            if (!ui->flipX->isChecked())
                storedX = ui->cutX->minimum();
            else
                storedX = ui->cutX->maximum();
            // show the cutting box
            CutBoxX->Visibility.setValue(true);
            // set new XRange
            auto CutBoundingBox = getViewBoundingBox();
            refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::notZValue,
                Refresh::XRange, Refresh::notYRange, Refresh::notZRange);
            // hide cutting box and compare resultwith stored value
            CutBoxX->Visibility.setValue(false);
            if (!ui->flipX->isChecked()) {
                if (storedX > ui->cutX->minimum())
                    ui->cutX->setMinimum(storedX);
            }
            else {
                if (storedX < ui->cutX->maximum())
                    ui->cutX->setMaximum(storedX);
            }
        }
    }
}

void SectionCut::onCutYHSsliderMoved(int val)
{
    // we cannot cut to the edge because then the result is an empty shape
    if (val == ui->cutYHS->maximum()) {
        ui->cutYHS->setValue(ui->cutYHS->maximum() - ui->cutYHS->singleStep());
        return;
    }
    if (val == ui->cutYHS->minimum()) {
        ui->cutYHS->setValue(ui->cutYHS->minimum() + ui->cutYHS->singleStep());
        return;
    }
    // the slider value is % of the cut range
    double NewCutValue = ui->cutY->minimum()
        + val / 100.0 * (ui->cutY->maximum() - ui->cutY->minimum());
    ui->cutYHS->setToolTip(QString::number(NewCutValue, 'g', Base::UnitsApi::getDecimals()));
    ui->cutY->setValue(NewCutValue);
}

void SectionCut::onCutYHSChanged(int val)
{
    onCutYHSsliderMoved(val);
}

void SectionCut::onCutZvalueChanged(double val)
{
    CutValueHelper(val, ui->cutZ, ui->cutZHS);

    auto CutBox = doc->getObject(BoxZName);
    if (!CutBox)
        return;
    Part::Box* pcBox = dynamic_cast<Part::Box*>(CutBox);
    if (!pcBox) {
        Base::Console().Error((std::string("SectionCut error: ") + std::string(BoxZName)
            + std::string(" is no Part::Box object. Cannot proceed.\n")).c_str());
        return;
    }
    Base::Placement placement = pcBox->Placement.getValue();
    Base::Vector3d BoxPosition = placement.getPosition();
    if (!ui->flipZ->isChecked())
        BoxPosition.z = ui->cutZ->value() - pcBox->Height.getValue();
    else //flipped
        BoxPosition.z = ui->cutZ->value();
    placement.setPosition(BoxPosition);
    pcBox->Placement.setValue(placement);

    auto CutObject = doc->getObject(CutZName);
    if (!CutObject) {
        Base::Console().Warning((std::string("SectionCut warning: there is no ")
            + std::string(CutZName) + std::string(", trying to recreate it\n")).c_str());
        startCutting();
        return;
    }
    Part::Cut* pcCut = dynamic_cast<Part::Cut*>(CutObject);
    if (!pcCut) {
        Base::Console().Error((std::string("SectionCut error: ") + std::string(CutZName)
            + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
        return;
    }
    pcCut->recomputeFeature(true);
    // refresh X and Y limits
    // this is done e.g. for X by
    // first making the cut X box visible, the setting the limits only for X
    // if x-limit in box direcion is larger than object, reset value to saved limit
    SbBox3f CutBoundingBox;
    if (hasBoxX) {
        auto CutBoxX = doc->getObject(BoxXName);
        if (!CutBoxX)
            return;
        // first store the values
        double storedX;
        if (!ui->flipX->isChecked())
            storedX = ui->cutX->minimum();
        else
            storedX = ui->cutX->maximum();
        // show the cutting box
        CutBoxX->Visibility.setValue(true);
        // set new XRange
        CutBoundingBox = getViewBoundingBox();
        refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::notZValue,
            Refresh::XRange, Refresh::notYRange, Refresh::notZRange);
        // hide cutting box and compare resultwith stored value
        CutBoxX->Visibility.setValue(false);
        if (!ui->flipX->isChecked()) {
            if (storedX > ui->cutX->minimum())
                ui->cutX->setMinimum(storedX);
        }
        else {
            if (storedX < ui->cutX->maximum())
                ui->cutX->setMaximum(storedX);
        }
    }
    if (hasBoxY) {
        auto CutBoxY = doc->getObject(BoxYName);
        if (!CutBoxY)
            return;
        double storedY;
        if (!ui->flipY->isChecked())
            storedY = ui->cutY->minimum();
        else
            storedY = ui->cutY->maximum();
        CutBoxY->Visibility.setValue(true);
        CutBoundingBox = getViewBoundingBox();
        refreshCutRanges(CutBoundingBox, Refresh::notXValue, Refresh::notYValue, Refresh::notZValue,
            Refresh::notXRange, Refresh::YRange, Refresh::notZRange);
        CutBoxY->Visibility.setValue(false);
        if (!ui->flipY->isChecked()) {
            if (storedY > ui->cutY->minimum())
                ui->cutY->setMinimum(storedY);
        }
        else {
            if (storedY < ui->cutY->maximum())
                ui->cutY->setMaximum(storedY);
        }
    }
}

void SectionCut::onCutZHSsliderMoved(int val)
{
    // we cannot cut to the edge because then the result is an empty shape
    if (val == ui->cutZHS->maximum()) {
        ui->cutZHS->setValue(ui->cutZHS->maximum() - ui->cutZHS->singleStep());
        return;
    }
    if (val == ui->cutZHS->minimum()) {
        ui->cutZHS->setValue(ui->cutZHS->minimum() + ui->cutZHS->singleStep());
        return;
    }
    // the slider value is % of the cut range
    double NewCutValue = ui->cutZ->minimum()
        + val / 100.0 * (ui->cutZ->maximum() - ui->cutZ->minimum());
    ui->cutZHS->setToolTip(QString::number(NewCutValue, 'g', Base::UnitsApi::getDecimals()));
    ui->cutZ->setValue(NewCutValue);
}

void SectionCut::onCutZHSChanged(int val)
{
    onCutZHSsliderMoved(val);
}

// helper function for the onFlip_clicked signal
void SectionCut::FlipClickedHelper(const char* BoxName)
{
    // there might be no document
    if (!Gui::Application::Instance->activeDocument()) {
        noDocumentActions();
        return;
    }
    // refresh objects and return in case the document was changed
    if (doc != Gui::Application::Instance->activeDocument()->getDocument()) {
        onRefreshCutPBclicked();
        return;
    }
    // we must move the box e.g. in y-direction by its Width
    auto CutBox = doc->getObject(BoxName);
    // there should be a box, but maybe the user deleted it meanwhile
    if (!CutBox) {
        Base::Console().Warning((std::string("SectionCut warning: there is no ")
            + std::string(BoxName) + std::string(", trying to recreate it\n")).c_str());
        // recreate the box
        startCutting();
        return;
    }
    Part::Box* pcBox = dynamic_cast<Part::Box*>(CutBox);
    if (!pcBox) {
        Base::Console().Error((std::string("SectionCut error: ") + std::string(BoxName)
            + std::string(" is no Part::Box object. Cannot proceed.\n")).c_str());
        return;
    }
    // get its placement and size
    Base::Placement placement = pcBox->Placement.getValue();
    Base::Vector3d BoxPosition = placement.getPosition();
    // flip the box
    switch (std::string(BoxName).back())
    {
    case 'X':
        if (ui->flipX->isChecked())
            BoxPosition.x = BoxPosition.x + pcBox->Length.getValue();
        else
            BoxPosition.x = BoxPosition.x - pcBox->Length.getValue();
        break;
    case 'Y':
        if (ui->flipY->isChecked())
            BoxPosition.y = BoxPosition.y + pcBox->Width.getValue();
        else
            BoxPosition.y = BoxPosition.y - pcBox->Width.getValue();
        break;
    case 'Z':
        if (ui->flipZ->isChecked())
            BoxPosition.z = BoxPosition.z + pcBox->Height.getValue();
        else
            BoxPosition.z = BoxPosition.z - pcBox->Height.getValue();
        break;
    }
    placement.setPosition(BoxPosition);
    pcBox->Placement.setValue(placement);
}

void SectionCut::onFlipXclicked()
{
    FlipClickedHelper(BoxXName);

    auto CutObject = doc->getObject(CutXName);
    // there should be a cut, but maybe the user deleted it meanwhile
    if (!CutObject) {
        Base::Console().Warning((std::string("SectionCut warning: there is no ")
            + std::string(CutXName) + std::string(", trying to recreate it\n")).c_str());
        // recreate the box
        startCutting();
        return;
    }

    // if there is another cut, we must recalculate it too
    // the hierarchy is always Z->Y->X
    if (hasBoxY && !hasBoxZ) { // only Y
        auto CutFeatureY = doc->getObject(CutYName);
        if (!CutFeatureY) {
            Base::Console().Warning(
                (std::string("SectionCut warning: the expected ")
                    + std::string(CutYName) + std::string(" is missing, trying to recreate it\n")).c_str());
            // recreate the box
            startCutting();
            return;
        }
        Part::Cut* pcCutY = dynamic_cast<Part::Cut*>(CutFeatureY);
        if (!pcCutY) {
            Base::Console().Error((std::string("SectionCut error: ") + std::string(CutYName)
                + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
            return;
        }
        pcCutY->recomputeFeature(true);
    }
    else if ((!hasBoxY && hasBoxZ) || (hasBoxY && hasBoxZ)) { // at least Z
        // the main cut is Z, no matter if there is a cut in Y
        auto CutFeatureZ = doc->getObject(CutZName);
        if (!CutFeatureZ) {
            Base::Console().Warning((std::string("SectionCut warning: the expected ")
                + std::string(CutZName) + std::string(" is missing, trying to recreate it\n")).c_str());
            // recreate the box
            startCutting();
            return;
        }
        Part::Cut* pcCutZ = dynamic_cast<Part::Cut*>(CutFeatureZ);
        if (!pcCutZ) {
            Base::Console().Error((std::string("SectionCut error: ") + std::string(CutZName)
                + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
            return;
        }
        pcCutZ->recomputeFeature(true);
    }
    else { // only do this when there is no other box to save recomputes
        Part::Cut* pcCut = dynamic_cast<Part::Cut*>(CutObject);
        if (!pcCut) {
            Base::Console().Error((std::string("SectionCut error: ") + std::string(CutXName)
                + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
            return;
        }
        pcCut->recomputeFeature(true);
    }     
}

void SectionCut::onFlipYclicked()
{
    FlipClickedHelper(BoxYName);

    auto CutObject = doc->getObject(CutYName);
    // there should be a cut, but maybe the user deleted it meanwhile
    if (!CutObject) {
        Base::Console().Warning((std::string("SectionCut warning: there is no ")
            + std::string(CutYName) + std::string(", trying to recreate it\n")).c_str());
        // recreate the box
        startCutting();
        return;
    }

    // if there is another cut, we must recalculate it too
    // we only need to check for Z since the hierarchy is always Z->Y->X
    if (hasBoxZ) {
        auto CutFeatureZ = doc->getObject(CutZName);
        Part::Cut* pcCutZ = dynamic_cast<Part::Cut*>(CutFeatureZ);
        if (!pcCutZ) {
            Base::Console().Error((std::string("SectionCut error: ") + std::string(CutZName)
                + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
            return;
        }
        pcCutZ->recomputeFeature(true);
    }
    else {
        Part::Cut* pcCut = dynamic_cast<Part::Cut*>(CutObject);
        if (!pcCut) {
            Base::Console().Error((std::string("SectionCut error: ") + std::string(CutYName)
                + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
            return;
        }
        pcCut->recomputeFeature(true);
    }
}

void SectionCut::onFlipZclicked()
{
    FlipClickedHelper(BoxZName);

    auto CutObject = doc->getObject(CutZName);
    // there should be a cut, but maybe the user deleted it meanwhile
    if (!CutObject) {
        Base::Console().Warning((std::string("SectionCut warning: there is no ")
            + std::string(CutZName) + std::string(", trying to recreate it\n")).c_str());
        // recreate the box
        startCutting();
        return;
    }
    Part::Cut* pcCut = dynamic_cast<Part::Cut*>(CutObject);
    if (!pcCut) {
        Base::Console().Error((std::string("SectionCut error: ") + std::string(CutZName)
            + std::string(" is no Part::Cut object. Cannot proceed.\n")).c_str());
        return;
    }
    pcCut->recomputeFeature(true);
}

// changes the cutface color
void SectionCut::onCutColorclicked()
{
    // re-cut to change the color of all cut boxes
    if (ui->groupBoxX->isChecked() || ui->groupBoxY->isChecked() || ui->groupBoxZ->isChecked())
        startCutting();
}

void SectionCut::onTransparencySliderMoved(int val)
{
    ui->CutTransparency->setToolTip(QString::number(val) + QString::fromLatin1(" %"));
    // highlight the tooltip
    QToolTip::showText(QCursor::pos(), QString::number(val) + QString::fromLatin1(" %"), nullptr);
    // re-cut to change the color of all cut boxes
    if (ui->groupBoxX->isChecked() || ui->groupBoxY->isChecked() || ui->groupBoxZ->isChecked())
        startCutting();
}

void SectionCut::onTransparencyChanged(int val)
{
    onTransparencySliderMoved(val);
}

// refreshes the list of document objects and the visible objects
void SectionCut::onRefreshCutPBclicked()
{
    // get document
    auto docGui = Gui::Application::Instance->activeDocument();
    if (!docGui) {
        Base::Console().Error("SectionCut error: there is no document\n");
        return;
    }
    doc = docGui->getDocument();
    // get all objects in the document
    std::vector<App::DocumentObject*> ObjectsList = doc->getObjects();
    if (ObjectsList.empty()) {
        Base::Console().Error("SectionCut error: there are no objects in the document\n");
        return;
    }
    // empty the ObjectsListVisible
    ObjectsListVisible.clear();
    // now store those that are currently visible
    for (auto it = ObjectsList.begin(); it != ObjectsList.end(); ++it) {
        if ((*it)->Visibility.getValue()) {
            ObjectsListVisible.push_back(*it);
        }
    }
    // reset defaults
    hasBoxX = false;
    hasBoxY = false;
    hasBoxZ = false;
    // we can have existing cuts
    if (doc->getObject(CutZName)) {
        hasBoxZ = true;
        ui->groupBoxZ->blockSignals(true);
        ui->groupBoxZ->setChecked(true);
        ui->groupBoxZ->blockSignals(false);
    }
    if (doc->getObject(CutYName)) {
        hasBoxY = true;
        ui->groupBoxY->blockSignals(true);
        ui->groupBoxY->setChecked(true);
        ui->groupBoxY->blockSignals(false);
    }
    if (doc->getObject(CutXName)) {
        hasBoxX = true;
        ui->groupBoxX->blockSignals(true);
        ui->groupBoxX->setChecked(true);
        ui->groupBoxX->blockSignals(false);
    }
    // if there is a cut, disable the button
    if (hasBoxX || hasBoxY || hasBoxZ)
        ui->RefreshCutPB->setEnabled(false);
}

SbBox3f SectionCut::getViewBoundingBox()
{
    SbBox3f Box;
    auto docGui = Gui::Application::Instance->activeDocument();
    if (!docGui) {
        Base::Console().Error("SectionCut error: there is no active document\n");
        return Box; // return an empty box
    }
    Gui::View3DInventor* view = dynamic_cast<Gui::View3DInventor*>(docGui->getActiveView());
    if (!view) {
        Base::Console().Error("SectionCut error: could not get the active view\n");
        return Box; // return an empty box
    }
    Gui::View3DInventorViewer* viewer = view->getViewer();
    SoCamera* camera = viewer->getSoRenderManager()->getCamera();
    if (!camera || !camera->isOfType(SoOrthographicCamera::getClassTypeId()))
        return Box; // return an empty box
    // get scene bounding box
    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(viewer->getSceneGraph());
    return action.getBoundingBox();
}

void SectionCut::refreshCutRanges(SbBox3f BoundingBox,
    bool forXValue, bool forYValue, bool forZValue,
    bool forXRange, bool forYRange, bool forZRange)
{
    if (!BoundingBox.isEmpty()) {
        SbVec3f center = BoundingBox.getCenter();
        int minDecimals = Base::UnitsApi::getDecimals();
        float lenx, leny, lenz;
        BoundingBox.getSize(lenx, leny, lenz);
        int steps = 100;

        // set the ranges
        float rangeMin; // to silence a compiler warning we use a float
        float rangeMax;
        if (forXRange) {
            rangeMin = center[0] - (lenx / 2);
            rangeMax = center[0] + (lenx / 2);
            ui->cutX->setRange(rangeMin, rangeMax);
            // determine the single step values
            lenx = lenx / steps;
            int dim = static_cast<int>(log10(lenx));
            double singleStep = pow(10.0, dim);
            ui->cutX->setSingleStep(singleStep);
        }
        if (forYRange) {
            rangeMin = center[1] - (leny / 2);
            rangeMax = center[1] + (leny / 2);
            ui->cutY->setRange(rangeMin, rangeMax);
            leny = leny / steps;
            int dim = static_cast<int>(log10(leny));
            double singleStep = pow(10.0, dim);
            ui->cutY->setSingleStep(singleStep);
        }
        if (forZRange) {
            rangeMin = center[2] - (lenz / 2);
            rangeMax = center[2] + (lenz / 2);
            ui->cutZ->setRange(rangeMin, rangeMax);
            lenz = lenz / steps;
            int dim = static_cast<int>(log10(lenz));
            double singleStep = pow(10.0, dim);
            ui->cutZ->setSingleStep(singleStep);
        }
        if (forXValue) {
            ui->cutX->setValue(center[0]);
            ui->cutXHS->setValue(50);
        }
        if (forYValue) {
            ui->cutY->setValue(center[1]);
            ui->cutYHS->setValue(50);
        }
        if (forZValue) {
            ui->cutZ->setValue(center[2]);
            ui->cutZHS->setValue(50);
        }

        // set decimals
        ui->cutX->setDecimals(minDecimals);
        ui->cutY->setDecimals(minDecimals);
        ui->cutZ->setDecimals(minDecimals);
    }
}

#include "moc_SectionCutting.cpp"
