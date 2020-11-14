/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/nodes/SoClipPlane.h>
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/sensors/SoTimerSensor.h>
# include <QDockWidget>
# include <QPointer>
# include <cmath>
#endif

// Importing of App classes
#ifdef FC_OS_WIN32
# define PartExport    __declspec(dllimport)
#else // for Linux
# define PartExport
#endif

#include "Clipping.h"
#include "ui_Clipping.h"
#include "Application.h"
#include "Command.h"
#include "DockWindowManager.h"
#include "Document.h"
#include "MainWindow.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include <App/Link.h>
#include <App/Part.h>
#include <Base/UnitsApi.h>
#include <Mod/Part/App/FeatureCompound.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/FeaturePartCommon.h>
#include <Mod/Part/App/FeaturePartCut.h>
#include <Mod/Part/App/FeaturePartFuse.h>
#include <Mod/Part/App/PartFeatures.h>

using namespace Gui::Dialog;

class Clipping::Private {
public:
    Ui_Clipping ui;
    QPointer<Gui::View3DInventor> view;
    SoGroup* node;
    SoClipPlane* clipX;
    SoClipPlane* clipY;
    SoClipPlane* clipZ;
    SoClipPlane* clipView;
    bool flipX;
    bool flipY;
    bool flipZ;
    SoTimerSensor* sensor;
    Private() : flipX(false), flipY(false), flipZ(false)
    {
        clipX = new SoClipPlane();
        clipX->on.setValue(false);
        clipX->plane.setValue(SbPlane(SbVec3f(1,0,0),0));
        clipX->ref();

        clipY = new SoClipPlane();
        clipY->on.setValue(false);
        clipY->plane.setValue(SbPlane(SbVec3f(0,1,0),0));
        clipY->ref();

        clipZ = new SoClipPlane();
        clipZ->on.setValue(false);
        clipZ->plane.setValue(SbPlane(SbVec3f(0,0,1),0));
        clipZ->ref();

        clipView = new SoClipPlane();
        clipView->on.setValue(false);
        clipView->plane.setValue(SbPlane(SbVec3f(0,0,1),0));
        clipView->ref();

        node = 0;
        sensor = new SoTimerSensor(moveCallback, this);
    }
    ~Private()
    {
        clipX->unref();
        clipY->unref();
        clipZ->unref();
        clipView->unref();
        delete sensor;
    }
    static void moveCallback(void * data, SoSensor * sensor)
    {
        Q_UNUSED(sensor); 
        Private* self = reinterpret_cast<Private*>(data);
        if (self->view) {
            Gui::View3DInventorViewer* view = self->view->getViewer();
            SoClipPlane* clip = self->clipView;
            SbPlane pln = clip->plane.getValue();
            clip->plane.setValue(SbPlane(view->getViewDirection(),pln.getDistanceFromOrigin()));
        }
    }
};

/* TRANSLATOR Gui::Dialog::Clipping */

Clipping::Clipping(Gui::View3DInventor* view, QWidget* parent)
  : QDialog(parent)
  , d(new Private)
{
    // create widgets
    d->ui.setupUi(this);
    d->ui.clipView->setRange(-INT_MAX,INT_MAX);
    d->ui.clipView->setSingleStep(0.1f);
    d->ui.clipX->setRange(-INT_MAX,INT_MAX);
    d->ui.clipX->setSingleStep(0.1f);
    d->ui.clipY->setRange(-INT_MAX,INT_MAX);
    d->ui.clipY->setSingleStep(0.1f);
    d->ui.clipZ->setRange(-INT_MAX,INT_MAX);
    d->ui.clipZ->setSingleStep(0.1f);

    d->ui.dirX->setRange(-INT_MAX,INT_MAX);
    d->ui.dirX->setSingleStep(0.1f);
    d->ui.dirY->setRange(-INT_MAX,INT_MAX);
    d->ui.dirY->setSingleStep(0.1f);
    d->ui.dirZ->setRange(-INT_MAX,INT_MAX);
    d->ui.dirZ->setSingleStep(0.1f);
    d->ui.dirZ->setValue(1.0f);

    d->view = view;
    View3DInventorViewer* viewer = view->getViewer();
    d->node = static_cast<SoGroup*>(viewer->getSceneGraph());
    d->node->ref();
    d->node->insertChild(d->clipX, 0);
    d->node->insertChild(d->clipY, 0);
    d->node->insertChild(d->clipZ, 0);
    d->node->insertChild(d->clipView, 0);

    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(viewer->getSceneGraph());
    SbBox3f box = action.getBoundingBox();

    if (!box.isEmpty()) {
        refreshCutRanges(box);
    }

    // get all objects in the document
    auto doc = Application::Instance->activeDocument()->getDocument();
    if (!doc) {
        Base::Console().Error("Clipping error: there is document\n");
        return;
    }
    ObjectsList = doc->getObjects();
    if (ObjectsList.empty()) {
        Base::Console().Error("Clipping error: there are no objects in the document\n");
        return;
    }
    // empty the ObjectsListVisible
    ObjectsListVisible.clear();
    // now store those that are currently visible
    for (auto it = ObjectsList.begin(); it != ObjectsList.end(); ++it) {
        if ((*it)->Visibility.getValue())
            ObjectsListVisible.push_back((*it));
    }

    connect(d->ui.CuttingRB, SIGNAL(clicked()), this, SLOT(onModeGBclicked()));
    connect(d->ui.ClippingRB, SIGNAL(clicked()), this, SLOT(onModeGBclicked()));
    connect(d->ui.clipX, SIGNAL(valueChanged(double)), this, SLOT(on_clipX_valueChanged(double)));
    connect(d->ui.clipY, SIGNAL(valueChanged(double)), this, SLOT(on_clipY_valueChanged(double)));
    connect(d->ui.clipZ, SIGNAL(valueChanged(double)), this, SLOT(on_clipZ_valueChanged(double)));
    connect(d->ui.clipView, SIGNAL(valueChanged(double)), this, SLOT(on_clipView_valueChanged(double)));
    connect(d->ui.clipXHS, SIGNAL(sliderMoved(int)), this, SLOT(on_clipXHS_sliderMoved(double)));
    connect(d->ui.clipYHS, SIGNAL(sliderMoved(int)), this, SLOT(on_clipYHS_sliderMoved(double)));
    connect(d->ui.clipZHS, SIGNAL(sliderMoved(int)), this, SLOT(on_clipZHS_sliderMoved(double)));
    connect(d->ui.flipX, SIGNAL(clicked()), this, SLOT(on_flipClipX_clicked()));
    connect(d->ui.flipY, SIGNAL(clicked()), this, SLOT(on_flipClipY_clicked()));
    connect(d->ui.flipZ, SIGNAL(clicked()), this, SLOT(on_flipClipZ_clicked()));
    connect(d->ui.RefreshCutPB, SIGNAL(clicked()), this, SLOT(on_RefreshCutPB_clicked()));
}

void Clipping::onModeGBclicked()
{
    auto doc = Application::Instance->activeDocument()->getDocument();
    if (!doc)
        return;

    // we will reuse it several times
    std::vector<App::DocumentObject*>::iterator it;

    // delete the objects we might have already created to cut
    // we must do this because we support several cuts at once and
    // it is dangerous to deal with the fact that the user is free
    // to uncheck cutting planes and to add/remove objects while this dialog is open
    // We must remove in this order because the tree hierary of the feautures is Z->Y->X and Cut->Box
    if (doc->getObject("CutViewCutZ")) {
        // the deleted object might have been visible before, thus check and delete it from the list
        for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
            if ((*it) == doc->getObject("CutViewCutZ")) {
                ObjectsListVisible.erase((it));
                break;
            }
        }
        doc->removeObject("CutViewCutZ");
    }
    if (doc->getObject("CutViewBoxZ")) {
        for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
            if ((*it) == doc->getObject("CutViewBoxZ")) {
                ObjectsListVisible.erase((it));
                break;
            }
        }
        doc->removeObject("CutViewBoxZ");
    }
    if (doc->getObject("CutViewCutY")) {
        for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
            if ((*it) == doc->getObject("CutViewCutY")) {
                ObjectsListVisible.erase((it));
                break;
            }
        }
        doc->removeObject("CutViewCutY");
    }
    if (doc->getObject("CutViewBoxY")) {
        for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
            if ((*it) == doc->getObject("CutViewBoxY")) {
                ObjectsListVisible.erase((it));
                break;
            }
        }
        doc->removeObject("CutViewBoxY");
    }
    if (doc->getObject("CutViewCutX")) {
        for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
            if ((*it) == doc->getObject("CutViewCutX")) {
                ObjectsListVisible.erase((it));
                break;
            }
        }
        doc->removeObject("CutViewCutX");
    }
    if (doc->getObject("CutViewBoxX")) {
        for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
            if ((*it) == doc->getObject("CutViewBoxX")) {
                ObjectsListVisible.erase((it));
                break;
            }
        }
        doc->removeObject("CutViewBoxX");
    }
    if (doc->getObject("CutViewCompound")) {
        auto compoundObject = doc->getObject("CutViewCompound");
        Part::Compound* pcCompoundDel = static_cast<Part::Compound*>(compoundObject);
        std::vector<App::DocumentObject*> compoundObjects;
        pcCompoundDel->Links.getLinks(compoundObjects);
        // first delete the compound
        for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
            if ((*it) == doc->getObject("CutViewCompound")) {
                ObjectsListVisible.erase((it));
                break;
            }
        }
        doc->removeObject("CutViewCompound");
        // now delete the objects that have been part of the compound
        for (it = compoundObjects.begin(); it != compoundObjects.end(); it++) {
            for (auto itOV = ObjectsListVisible.begin(); itOV != ObjectsListVisible.end(); ++itOV) {
                if ((*itOV) == doc->getObject((*it)->getNameInDocument())) {
                    ObjectsListVisible.erase((itOV));
                    break;
                }
            }
            doc->removeObject((*it)->getNameInDocument());
        }
    }

    // make all objects visible that have been visible when the clipping dialog was called
    // because we made them invisible when we created cuts
    for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
        (*it)->Visibility.setValue(true);
    }

    // we enable the sliders because for assemblies we disabled them
    d->ui.clipXHS->setEnabled(true);
    d->ui.clipYHS->setEnabled(true);
    d->ui.clipZHS->setEnabled(true);

    // if we have a clip
    if (d->ui.ClippingRB->isChecked()) {
        // enable refresh button
        d->ui.RefreshCutPB->setEnabled(true);
        // rename the checkboxes
        QString label = d->ui.ClippingRB->text();
        d->ui.groupBoxX->setTitle(label + QString::fromLatin1(" X"));
        d->ui.groupBoxY->setTitle(label + QString::fromLatin1(" Y"));
        d->ui.groupBoxZ->setTitle(label + QString::fromLatin1(" Z"));
        d->ui.groupBoxView->setTitle(label + QString::fromLatin1(" custom direction"));
        // turn on clipping
        if (d->ui.groupBoxX->isChecked())
            d->clipX->on.setValue(true);
        if (d->ui.groupBoxY->isChecked())
            d->clipY->on.setValue(true);
        if (d->ui.groupBoxZ->isChecked())
            d->clipZ->on.setValue(true);
        // reactivate groupBoxView
        d->ui.groupBoxView->setEnabled(true);
        return;
    }

    // ObjectsListVisible contains all visible objects of the document, but we can only cut
    // those that have a solid shape
    std::vector<App::DocumentObject*> ObjectsListCut;
    for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
        // we need all Link objects in App::Parts for example for Assembly 4
        if ((*it)->getTypeId() == Base::Type::fromName("App::Part")) {
            App::Part* pcPart = static_cast<App::Part*>((*it));
            auto TypeString = std::string(pcPart->Type.getValue());
            // since the TypeString begins with control characters, we need to check it from the end
            if (!TypeString.empty() && TypeString.substr(TypeString.length() - 15) == "Assembly4 Model") {
                // collect now all its link objects
                auto groupObjects = pcPart->Group.getValue();
                for (auto itGO = groupObjects.begin(); itGO != groupObjects.end(); ++itGO) {
                    if ((*itGO)->getTypeId() == Base::Type::fromName("App::Link"))
                        ObjectsListCut.push_back((*itGO));
                }
                // we disable the sliders because for assemblies it will takes ages to do several dozen recomputes
                d->ui.clipXHS->setEnabled(false);
                d->ui.clipYHS->setEnabled(false);
                d->ui.clipZHS->setEnabled(false);
            }
        }
        // get all shapes
        if ((*it)->getPropertyByName("Shape")) {
            // sort out 2D objets, datums, App:Parts, compounds and objects that are part of a PartDesign body
            if (!(*it)->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Part2DObject"))
                && !(*it)->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Datum"))
                && !(*it)->getTypeId().isDerivedFrom(Base::Type::fromName("PartDesign::Feature"))
                && !(*it)->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Compound"))
                && (*it)->getTypeId() != Base::Type::fromName("App::Part"))
                ObjectsListCut.push_back((*it));
        }
    }
    
    // sort out objects that are part of Part::Boolean, Part::MultiCommon, Part::MultiFuse,
    // Part::Thickness and Part::FilletBase
    std::vector<App::DocumentObject*>::iterator it2;
    for (it = ObjectsListVisible.begin(); it != ObjectsListVisible.end(); ++it) {
        // Part::Boolean objects
        if ((*it)->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Boolean"))) {
            auto pcBool = static_cast<Part::Boolean*>((*it));
            // Booleans have a base and a tool, both must removed from the collected list
            auto base = pcBool->Base.getValue();
            for (it2 = ObjectsListCut.begin(); it2 != ObjectsListCut.end(); ++it2) {
                if ((*it2) == base) {
                    ObjectsListCut.erase(it2);
                    break;
                }
            }
            auto tool = pcBool->Tool.getValue();
            for (it2 = ObjectsListCut.begin(); it2 != ObjectsListCut.end(); ++it2) {
                if ((*it2) == tool) {
                    ObjectsListCut.erase(it2);
                    break;
                }
            }
        }
        // Part::MultiCommon objects
        else if ((*it)->getTypeId().isDerivedFrom(Base::Type::fromName("Part::MultiCommon"))) {
            auto pcMultiCommon = static_cast<Part::MultiCommon*>((*it));
            // MultiCommons have a Shapes list, these objects must be removed from the collected list
            auto ShapeList = pcMultiCommon->Shapes.getValue();
            for (auto itSL = ShapeList.begin(); itSL != ShapeList.end(); ++itSL) {
                for (it2 = ObjectsListCut.begin(); it2 != ObjectsListCut.end(); ++it2) {
                    if ((*it2) == (*itSL)) {
                        ObjectsListCut.erase(it2);
                        break;
                    }
                }
            }
        }
        // Part::MultiFuse objects
        else if ((*it)->getTypeId().isDerivedFrom(Base::Type::fromName("Part::MultiFuse"))) {
            auto pcMultiFuse = static_cast<Part::MultiFuse*>((*it));
            // MultiFuse have a Shapes list, these objects must be removed from the collected list
            auto ShapeList = pcMultiFuse->Shapes.getValue();
            for (auto itSL = ShapeList.begin(); itSL != ShapeList.end(); ++itSL) {
                for (it2 = ObjectsListCut.begin(); it2 != ObjectsListCut.end(); ++it2) {
                    if ((*it2) == (*itSL)) {
                        ObjectsListCut.erase(it2);
                        break;
                    }
                }
            }
        }
        // Part::Thickness objects
        else if ((*it)->getTypeId() == Base::Type::fromName("Part::Thickness")) {
            auto pcThickness = static_cast<Part::Thickness*>((*it));
            // get the object of the selected faces (there can only be one object)
              auto faces = pcThickness->Faces.getValue();
            for (it2 = ObjectsListCut.begin(); it2 != ObjectsListCut.end(); ++it2) {
                if ((*it2) == faces) {
                    ObjectsListCut.erase(it2);
                    break;
                }
            }
        }
        // Part::FilletBase objects
        else if ((*it)->getTypeId().isDerivedFrom(Base::Type::fromName("Part::FilletBase"))) {
            auto pcFilletBase = static_cast<Part::FilletBase*>((*it));
            // get the object of the selected faces (there can only be one object)
            auto base = pcFilletBase->Base.getValue();
            for (it2 = ObjectsListCut.begin(); it2 != ObjectsListCut.end(); ++it2) {
                if ((*it2) == base) {
                    ObjectsListCut.erase(it2);
                    break;
                }
            }
        }
    }

    // we might have no objects that can be cut
    if (ObjectsListCut.empty()) {
        Base::Console().Error("CutView error: there are no objects in the document that can be cut\n");
        // reset the cut group boxes
        d->ui.groupBoxX->setChecked(false);
        d->ui.groupBoxY->setChecked(false);
        d->ui.groupBoxZ->setChecked(false);
        // set back to clipping
        d->ui.ClippingRB->setChecked(true);
        return;
    }
    
    // we cut this way:
    // 1. we put all existing objects into a part compound
    // 2. we create a box with the size of the bounding box
    // 3. we cut the box from the compound
    
    // first rename the checkboxes
    QString label = d->ui.CuttingRB->text();
    d->ui.groupBoxX->setTitle(label + QString::fromLatin1(" X"));
    d->ui.groupBoxY->setTitle(label + QString::fromLatin1(" Y"));
    d->ui.groupBoxZ->setTitle(label + QString::fromLatin1(" Z"));
    d->ui.groupBoxView->setTitle(label + QString::fromLatin1(" custom direction"));

    // turn off clipping
    d->clipX->on.setValue(false);
    d->clipY->on.setValue(false);
    d->clipZ->on.setValue(false);

    // disable refresh button
    d->ui.RefreshCutPB->setEnabled(false);
    
    // depending on how many cuts should be peformed, we need as many boxes
    // if nothing is yet to be cut, we can return
    if (!d->ui.groupBoxX->isChecked() && !d->ui.groupBoxY->isChecked()
        && !d->ui.groupBoxZ->isChecked() && !d->ui.groupBoxView->isChecked()) {
        return;
    }

    // for now disable groupBoxView
    d->ui.groupBoxView->setEnabled(false);

    // create an empty compound
    auto CutCompound = doc->addObject("Part::Compound", "CutViewCompound");
    if (!CutCompound) {
        Base::Console().Error("CutView error: 'CutCompound' could not be added\n");
        return;
    }
    Part::Compound* pcCompound = static_cast<Part::Compound*>(CutCompound);
    // fill it with all found elements with the copies of the elements
    int i = 0;
    for (it = ObjectsListCut.begin(), i = 0; it != ObjectsListCut.end(); ++it, i++) {
        // first create a link with a unique name
        std::string newName = (*it)->getNameInDocument();
        newName = newName + "_CutLink";
        
        auto newObject = doc->addObject("App::Link", newName.c_str());
        if (!newObject) {
            Base::Console().Error("CutView error: 'App::Link' could not be added\n");
            return;
        }
        App::Link* pcLink = static_cast<App::Link*>(newObject);
        // set the object to the created empty link object
        pcLink->LinkedObject.setValue((*it));
        // we want to get the link at the same position as the original
        pcLink->LinkTransform.setValue(true); 
        // add the link to the compound
        pcCompound->Links.set1Value(i, newObject);

        // hide the objects since only the cut should later be visible
        (*it)->Visibility.setValue(false);
    }

    // compute the filled compound
    pcCompound->recomputeFeature();

    // make all objects invisible so that only the compound remains
    for (it = ObjectsList.begin(); it != ObjectsList.end(); ++it) {
        (*it)->Visibility.setValue(false);
    }

    // the area in which we can cut is the size of the compound
    // we get its size by its bounding box
    SbBox3f CompoundBoundingBox = getViewBoundingBox();
    if (CompoundBoundingBox.isEmpty()) {
        Base::Console().Error("CutView error: the CompoundBoundingBox is empty\n");        
        return;
    }

    // store the current cut positions te reset them later if possible
    double CutPosX = d->ui.clipX->value();
    double CutPosY = d->ui.clipY->value();
    double CutPosZ = d->ui.clipZ->value();

    // refresh all cut limits according to the new bounding box
    refreshCutRanges(CompoundBoundingBox, false, false, false, true, true, true, false);
        
    // prepare the cut box size according to the bounding box size
    std::vector<float> BoundingBoxSize = { 0.0, 0.0, 0.0 };
    CompoundBoundingBox.getSize(BoundingBoxSize[0], BoundingBoxSize[1], BoundingBoxSize[2]);
    // get placement of the bunding box origin
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

    if (d->ui.groupBoxX->isChecked()) {
        // create a box
        auto CutBox = doc->addObject("Part::Box", "CutViewBoxX");
        if (!CutBox) {
            Base::Console().Error("CutView error: 'CutViewBoxX' could not be added\n");
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
        if (CutPosX >= d->ui.clipX->maximum()) {
            CutPosX = d->ui.clipX->maximum() - 0.1; // short below the maximum
        }
        else if (CutPosX <= d->ui.clipX->minimum()) {
            CutPosX = d->ui.clipX->minimum() + 0.1; // short above the minimum
        }
        // we don't set the value to d->ui.clipX because this would refresh the cut
        // which we don't have yet, thus do this later
        //set the box position
        if (!d->ui.flipX->isChecked())
            BoxOriginSet.x = CutPosX - (BoundingBoxSize[0] + 1.0);
        else //flipped
            BoxOriginSet.x = CutPosX;
        // we made the box 1.0 larger that we can place it 0.5 below the boundig box
        BoxOriginSet.y = BoundingBoxOrigin[1] - 0.5;
        BoxOriginSet.z = BoundingBoxOrigin[2] - 0.5;
        placement.setPosition(BoxOriginSet);
        pcBox->Placement.setValue(placement);

        // create a cut feature
        auto CutFeature = doc->addObject("Part::Cut", "CutViewCutX");
        if (!CutFeature) {
            Base::Console().Error("CutView error: 'CutViewCutX' could not be added\n");
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutFeature);
        pcCut->Base.setValue(CutCompound);
        pcCut->Tool.setValue(CutBox);

        // set the cut value
        d->ui.clipX->setValue(CutPosX);
        // recomputing recursively is especially for assemblies very time-consuming
        // however there must be a final recursicve recompute and we do this at the end
        // so only recomute recursively if there are no other cuts
        if (!d->ui.groupBoxY->isChecked() && !d->ui.groupBoxZ->isChecked())
            pcCut->recomputeFeature(true);
        else
            pcCut->recomputeFeature(false);
        hasBoxX = true;
    }
    if (d->ui.groupBoxY->isChecked()) {
        // if there is a X cut, its size defines the possible range for the Y cut
        // the cut box size is not affected, it can be as large as the compound
        if (hasBoxX) {
            CutBoundingBox = getViewBoundingBox();
            // refresh the Y cut limits according to the new bounding box
            refreshCutRanges(CutBoundingBox, false, false, false, false, true, false, false);
        }
        auto CutBox = doc->addObject("Part::Box", "CutViewBoxY");
        if (!CutBox) {
            Base::Console().Error("CutView error: 'CutViewBoxY' could not be added\n");
            return;
        }
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        pcBox->Length.setValue(BoundingBoxSize[0] + 1.0);
        pcBox->Width.setValue(BoundingBoxSize[1] + 1.0);
        pcBox->Height.setValue(BoundingBoxSize[2] + 1.0);
        // reset previous cut value
        if (CutPosY >= d->ui.clipY->maximum()) {
            CutPosY = d->ui.clipY->maximum() - 0.1; // short below the maximum
        }
        else if (CutPosY <= d->ui.clipY->minimum()) {
            CutPosY = d->ui.clipY->minimum() + 0.1; // short above the minimum
        }
        //set the box position
        BoxOriginSet.x = BoundingBoxOrigin[0] - 0.5;
        if (!d->ui.flipY->isChecked())
            BoxOriginSet.y = CutPosY - (BoundingBoxSize[1] + 1.0);
        else //flipped
            BoxOriginSet.y = CutPosY;
        BoxOriginSet.z = BoundingBoxOrigin[2] - 0.5;
        placement.setPosition(BoxOriginSet);
        pcBox->Placement.setValue(placement);
        
        auto CutFeature = doc->addObject("Part::Cut", "CutViewCutY");
        if (!CutFeature) {
            Base::Console().Error("CutView error: 'CutViewCutY' could not be added\n");
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutFeature);
        // if there is already a cut, we must take it as feature to be cut
        if (hasBoxX)
            pcCut->Base.setValue(doc->getObject("CutViewCutX"));
        else
            pcCut->Base.setValue(CutCompound);
        pcCut->Tool.setValue(CutBox);
        
        // set the cut value
        d->ui.clipY->setValue(CutPosY);
        if (!d->ui.groupBoxZ->isChecked())
            pcCut->recomputeFeature(true);
        else
            pcCut->recomputeFeature(false);
        hasBoxY = true;
    }
    if (d->ui.groupBoxZ->isChecked()) {
        if (hasBoxX || hasBoxY) {
            CutBoundingBox = getViewBoundingBox();
            refreshCutRanges(CutBoundingBox, false, false, false, false, false, true, false);
        }
        auto CutBox = doc->addObject("Part::Box", "CutViewBoxZ");
        if (!CutBox) {
            Base::Console().Error("CutView error: 'CutViewBoxZ' could not be added\n");
            return;
        }
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        pcBox->Length.setValue(BoundingBoxSize[0] + 1.0);
        pcBox->Width.setValue(BoundingBoxSize[1] + 1.0);
        pcBox->Height.setValue(BoundingBoxSize[2] + 1.0);
        // reset previous cut value
        if (CutPosZ >= d->ui.clipZ->maximum()) {
            CutPosZ = d->ui.clipZ->maximum() - 0.1; // short below the maximum
        }
        else if (CutPosZ <= d->ui.clipZ->minimum()) {
            CutPosZ = d->ui.clipZ->minimum() + 0.1; // short above the minimum
        }
        //set the box position
        BoxOriginSet.x = BoundingBoxOrigin[0] - 0.5;
        BoxOriginSet.y = BoundingBoxOrigin[1] - 0.5;
        if (!d->ui.flipY->isChecked())
            BoxOriginSet.z = CutPosZ - (BoundingBoxSize[2] + 1.0);
        else //flipped
            BoxOriginSet.z = CutPosZ;
        placement.setPosition(BoxOriginSet);
        pcBox->Placement.setValue(placement);

        auto CutFeature = doc->addObject("Part::Cut", "CutViewCutZ");
        if (!CutFeature) {
            Base::Console().Error("CutView error: 'CutViewCutZ' could not be added\n");
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutFeature);
        // if there is already a cut, we must take it as feature to be cut
        if (hasBoxY) {
            pcCut->Base.setValue(doc->getObject("CutViewCutY"));
        }
        else if (hasBoxX && !hasBoxY) {
            pcCut->Base.setValue(doc->getObject("CutViewCutX"));
        }
        else {
            pcCut->Base.setValue(CutCompound);
        }
        pcCut->Tool.setValue(CutBox);

        // set the cut value
        d->ui.clipZ->setValue(CutPosZ);
        pcCut->recomputeFeature(true);
        hasBoxZ = true;
    }
}

Clipping* Clipping::makeDockWidget(Gui::View3DInventor* view)
{
    // embed this dialog into a QDockWidget
    Clipping* clipping = new Clipping(view);
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    // the dialog is designed that you can see the tree, thus put it to the right side
    QDockWidget* dw = pDockMgr->addDockWindow("Clipping", clipping, Qt::RightDockWidgetArea);
    dw->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    //dw->setFloating(true);
    dw->show();

    return clipping;
}

/** Destroys the object and frees any allocated resources */
Clipping::~Clipping()
{
    d->node->removeChild(d->clipX);
    d->node->removeChild(d->clipY);
    d->node->removeChild(d->clipZ);
    d->node->removeChild(d->clipView);
    d->node->unref();
    delete d;
}

void Clipping::reject()
{
    QDialog::reject();
    QDockWidget* dw = qobject_cast<QDockWidget*>(parent());
    if (dw) {
        dw->deleteLater();
    }
}

void Clipping::on_groupBoxX_toggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }
    if (d->ui.ClippingRB->isChecked()) {
        d->clipX->on.setValue(on);
    }
    if (d->ui.CuttingRB->isChecked()) {
        // setting the Cutting RB will invoke onModeGBclicked resets the cut
        d->ui.CuttingRB->clicked(true);
    }
}

void Clipping::on_groupBoxY_toggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }
    if (d->ui.ClippingRB->isChecked()) {
        d->clipY->on.setValue(on);
    }
    if (d->ui.CuttingRB->isChecked()) {
        d->ui.CuttingRB->clicked(true);
    }
}

void Clipping::on_groupBoxZ_toggled(bool on)
{
    if (on) {
        d->ui.groupBoxView->setChecked(false);
    }
    if (d->ui.ClippingRB->isChecked()) {
        d->clipZ->on.setValue(on);
    }
    if (d->ui.CuttingRB->isChecked()) {
        d->ui.CuttingRB->clicked(true);
    }
}

void Clipping::on_clipX_valueChanged(double val)
{
    // update slider position and tooltip
    // the slider value is % of the cut range
    d->ui.clipXHS->setValue(
        int((val - d->ui.clipX->minimum())
            / (d->ui.clipX->maximum() - d->ui.clipX->minimum()) * 100.0));
    d->ui.clipXHS->setToolTip(QString::number(val, 'g', Base::UnitsApi::getDecimals()));

    if (d->ui.ClippingRB->isChecked()) {
        SbPlane pln = d->clipX->plane.getValue();
        d->clipX->plane.setValue(SbPlane(pln.getNormal(), d->flipX ? -val : val));
    }
    else { // we cut
        // we cannot cut to the edge because then the result is an empty shape
        // we chose purposely not to simply set the range for clipX previously
        // because everything is allowed just not the min/max
        if (d->ui.clipX->value() == d->ui.clipX->maximum()) {
            d->ui.clipX->setValue(d->ui.clipX->maximum() - 0.1);
            return;
        }
        if (d->ui.clipX->value() == d->ui.clipX->minimum()) {
            d->ui.clipX->setValue(d->ui.clipX->minimum() + 0.1);
            return;
        }
        // get the cut box
        auto doc = Application::Instance->activeDocument()->getDocument();
        auto CutBox = doc->getObject("CutViewBoxX");
        // when the value has been set after reseting the compound bounding box
        // there is not yet a cut and we do nothing
        if (!CutBox)
            return;
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        // get its placement and size
        Base::Placement placement = pcBox->Placement.getValue();
        Base::Vector3d BoxPosition = placement.getPosition();
        // change the placement
        if (!d->ui.flipX->isChecked())
            BoxPosition.x = d->ui.clipX->value() - pcBox->Length.getValue();
        else //flipped
            BoxPosition.x = d->ui.clipX->value();
        placement.setPosition(BoxPosition);
        pcBox->Placement.setValue(placement);

        pcBox->recomputeFeature();

        auto CutObject = doc->getObject("CutViewCutX");
        // there should be a box, but maybe the user deleted it meanwhile
        if (!CutObject) {
            Base::Console().Warning("CutView warning: there is no 'CutViewCutX', trying to recreate it\n");
            // setting the Cutting RB will invoke onModeGBclicked that recreates the box
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutObject);
        pcCut->recomputeFeature();

        // if there is another cut, we must recalculate it too
        // we might have cut so that the range for Y and Z is now smaller
        // the hierarchy is always Z->Y->X
        if (hasBoxY && !hasBoxZ) { // only Y
            auto CutFeatureY = doc->getObject("CutViewCutY");
            if (!CutFeatureY) {
                Base::Console().Warning("CutView warning: there is no 'CutViewCutY, trying to recreate it'\n");
                d->ui.CuttingRB->clicked(true);
                return;
            }
            // refresh the Y and Z cut limits according to the new bounding box of the cut result
            // make the CutViewCutY invisible
            CutFeatureY->Visibility.setValue(false);
            // make CutViewCutX visible
            CutObject->Visibility.setValue(true);
            // get new bunding box
            auto CutBoundingBox = getViewBoundingBox();
            // refresh Y limits and Z limits + Z value
            refreshCutRanges(CutBoundingBox, false, false, true, false, true, true, false);
            // the value of Y can now be outside or at the limit, in this case reset the value too
            if ((d->ui.clipY->value() >= d->ui.clipY->maximum())
                || (d->ui.clipY->value() <= d->ui.clipY->minimum()))
                refreshCutRanges(CutBoundingBox, false, true, true, false, true, true, false);
            // make the CutViewCutY visible again
            CutFeatureY->Visibility.setValue(true);
            // make CutViewCutX invisible again
            CutObject->Visibility.setValue(false);
            // recompute the cut
            Part::Cut* pcCutY = static_cast<Part::Cut*>(CutFeatureY);
            pcCutY->recomputeFeature();
        }
        else if (hasBoxZ) { // Z
            // the main cut is Z, no matter if there is a cut in Y
            auto CutFeatureZ = doc->getObject("CutViewCutZ");
            if (!CutFeatureZ) {
                Base::Console().Error("CutView error: there is no 'CutViewCutZ'\n");
                return;
            }
            // refresh the Y and Z cut limits according to the new bounding box of the cut result
            // make the CutViewCutZ invisible
            CutFeatureZ->Visibility.setValue(false);
            // make CutViewCutX visible
            CutObject->Visibility.setValue(true);
            // get new bunding box
            auto CutBoundingBox = getViewBoundingBox();
            // refresh Y and Z limits
            if (hasBoxY) {
                refreshCutRanges(CutBoundingBox, false, false, false, false, true, true, false);
                // the value of Y or Z can now be outside or at the limit, in this case reset the value too
                if ((d->ui.clipY->value() >= d->ui.clipY->maximum())
                    || (d->ui.clipY->value() <= d->ui.clipY->minimum()))
                    refreshCutRanges(CutBoundingBox, false, true, false, false, true, true, false);
                if ((d->ui.clipZ->value() >= d->ui.clipZ->maximum())
                    || (d->ui.clipZ->value() <= d->ui.clipZ->minimum()))
                    refreshCutRanges(CutBoundingBox, false, false, true, false, true, true, false);
            }
            else { // there is no Y cut yet so we can set the Y value too
                refreshCutRanges(CutBoundingBox, false, true, false, false, true, true, false);
                // the value of Z can now be outside or at the limit, in this case reset the value too
                if ((d->ui.clipZ->value() >= d->ui.clipZ->maximum())
                    || (d->ui.clipZ->value() <= d->ui.clipZ->minimum()))
                    refreshCutRanges(CutBoundingBox, false, true, true, false, true, true, false);
            }
            // make the CutViewCutZ visible again
            CutFeatureZ->Visibility.setValue(true);
            // make CutViewCutX invisible again
            CutObject->Visibility.setValue(false);
            // recompute the cut
            Part::Cut* pcCutZ = static_cast<Part::Cut*>(CutFeatureZ);
            pcCutZ->recomputeFeature();
        }
        else { // just X
            // refresh Y and Z limits + values
            auto CutBoundingBox = getViewBoundingBox();
            refreshCutRanges(CutBoundingBox, false, true, true, false, true, true, false);
        }
    }
}

void Clipping::on_clipXHS_sliderMoved(int val)
{
    // we cannot cut to the edge because then the result is an empty shape
    // we chose purposely not to simply set the range for clipXHS previously
    // because everything is allowed just not the min/max
    // we set it one slider step below the min/max
    if (val == d->ui.clipXHS->maximum()) {
        d->ui.clipXHS->setValue(d->ui.clipXHS->maximum() - d->ui.clipXHS->singleStep());
        return;
    }
    if (val == d->ui.clipXHS->minimum()) {
        d->ui.clipXHS->setValue(d->ui.clipXHS->minimum() + d->ui.clipXHS->singleStep());
        return;
    }
    // the slider value is % of the cut range
    double NewCutValue = d->ui.clipX->minimum()
        + val / 100.0 * (d->ui.clipX->maximum() - d->ui.clipX->minimum());
    d->ui.clipXHS->setToolTip(QString::number(NewCutValue, 'g', Base::UnitsApi::getDecimals()));
    d->ui.clipX->setValue(NewCutValue);
}

void Clipping::on_clipY_valueChanged(double val)
{
    // update slider position and tooltip
    // the slider value is % of the cut range
    d->ui.clipYHS->setValue(
        int((val - d->ui.clipY->minimum())
            / (d->ui.clipY->maximum() - d->ui.clipY->minimum()) * 100.0));
    d->ui.clipYHS->setToolTip(QString::number(val, 'g', Base::UnitsApi::getDecimals()));

    if (d->ui.ClippingRB->isChecked()) {
        SbPlane pln = d->clipY->plane.getValue();
        d->clipY->plane.setValue(SbPlane(pln.getNormal(), d->flipY ? -val : val));
    }
    else { // we cut
        // we cannot cut to the edge because then the result is an empty shape
        if (d->ui.clipY->value() == d->ui.clipY->maximum()) {
            d->ui.clipY->setValue(d->ui.clipY->maximum() - 0.1);
            return;
        }
        if (d->ui.clipY->value() == d->ui.clipY->minimum()) {
            d->ui.clipY->setValue(d->ui.clipY->minimum() + 0.1);
            return;
        }
        auto doc = Application::Instance->activeDocument()->getDocument();
        auto CutBox = doc->getObject("CutViewBoxY");
        if (!CutBox)
            return;
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        Base::Placement placement = pcBox->Placement.getValue();
        Base::Vector3d BoxPosition = placement.getPosition();
        if (!d->ui.flipY->isChecked())
            BoxPosition.y = d->ui.clipY->value() - pcBox->Width.getValue();
        else //flipped
            BoxPosition.y = d->ui.clipY->value();
        placement.setPosition(BoxPosition);
        pcBox->Placement.setValue(placement);

        pcBox->recomputeFeature();

        auto CutObject = doc->getObject("CutViewCutY");
        if (!CutObject) {
            Base::Console().Warning("CutView warning: there is no 'CutViewCutY', trying to recreate it\n");
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutObject);
        pcCut->recomputeFeature();

        // if there is another cut, we must recalculate it too
        // we might have cut so that the range for Z is now smaller
        // we only need to check for Z since the hierarchy is always Z->Y->X
        if (hasBoxZ) {
            auto CutFeatureZ = doc->getObject("CutViewCutZ");
            if (!CutFeatureZ) {
                Base::Console().Error("CutView error: there is no 'CutViewCutZ'\n");
                return;
            }
            // refresh the Z cut limits according to the new bounding box of the cut result
            // make the CutViewCutZ invisible
            CutFeatureZ->Visibility.setValue(false);
            // make CutViewCutX visible
            CutObject->Visibility.setValue(true);
            // get new bunding box
            auto CutBoundingBox = getViewBoundingBox();
            // refresh Z limits
            refreshCutRanges(CutBoundingBox, false, false, false, false, false, true, false);
            // the value of Z can now be outside or at the limit, in this case reset the value too
            if ((d->ui.clipZ->value() >= d->ui.clipZ->maximum())
                || (d->ui.clipZ->value() <= d->ui.clipZ->minimum()))
                refreshCutRanges(CutBoundingBox, false, false, true, false, false, true, false);
            // make the CutViewCutZ visible again
            CutFeatureZ->Visibility.setValue(true);
            // make CutViewCutX invisible again
            CutObject->Visibility.setValue(false);
            // recompute the cut
            Part::Cut* pcCutZ = static_cast<Part::Cut*>(CutFeatureZ);
            pcCutZ->recomputeFeature();
        }
        else { // just X
            // refresh Z limits + values
            auto CutBoundingBox = getViewBoundingBox();
            refreshCutRanges(CutBoundingBox, false, false, true, false, false, true, false);
        }
    }
}

void Clipping::on_clipYHS_sliderMoved(int val)
{
    // we cannot cut to the edge because then the result is an empty shape
    if (val == d->ui.clipYHS->maximum()) {
        d->ui.clipYHS->setValue(d->ui.clipYHS->maximum() - d->ui.clipYHS->singleStep());
        return;
    }
    if (val == d->ui.clipYHS->minimum()) {
        d->ui.clipYHS->setValue(d->ui.clipYHS->minimum() + d->ui.clipYHS->singleStep());
        return;
    }
    // the slider value is % of the cut range
    double NewCutValue = d->ui.clipY->minimum()
        + val / 100.0 * (d->ui.clipY->maximum() - d->ui.clipY->minimum());
    d->ui.clipYHS->setToolTip(QString::number(NewCutValue, 'g', Base::UnitsApi::getDecimals()));
    d->ui.clipY->setValue(NewCutValue);
}

void Clipping::on_clipZ_valueChanged(double val)
{
    // update slider position and tooltip
    // the slider value is % of the cut range
    d->ui.clipZHS->setValue(
        int((val - d->ui.clipZ->minimum())
            / (d->ui.clipZ->maximum() - d->ui.clipZ->minimum()) * 100.0));
    d->ui.clipZHS->setToolTip(QString::number(val, 'g', Base::UnitsApi::getDecimals()));

    if (d->ui.ClippingRB->isChecked()) {
        SbPlane pln = d->clipZ->plane.getValue();
        d->clipZ->plane.setValue(SbPlane(pln.getNormal(), d->flipZ ? -val : val));
    }
    else { // we cut
        // we cannot cut to the edge because then the result is an empty shape
        if (d->ui.clipZ->value() == d->ui.clipZ->maximum()) {
            d->ui.clipZ->setValue(d->ui.clipZ->maximum() - 0.1);
            return;
        }
        if (d->ui.clipZ->value() == d->ui.clipZ->minimum()) {
            d->ui.clipZ->setValue(d->ui.clipZ->minimum() + 0.1);
            return;
        }
        auto doc = Application::Instance->activeDocument()->getDocument();
        auto CutBox = doc->getObject("CutViewBoxZ");
        if (!CutBox)
            return;
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        Base::Placement placement = pcBox->Placement.getValue();
        Base::Vector3d BoxPosition = placement.getPosition();
        if (!d->ui.flipZ->isChecked())
            BoxPosition.z = d->ui.clipZ->value() - pcBox->Height.getValue();
        else //flipped
            BoxPosition.z = d->ui.clipZ->value();
        placement.setPosition(BoxPosition);
        pcBox->Placement.setValue(placement);

        pcBox->recomputeFeature();

        auto CutObject = doc->getObject("CutViewCutZ");
        if (!CutObject) {
            Base::Console().Warning("CutView warning: there is no 'CutViewCutZ', trying to recreate it\n");
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutObject);
        pcCut->recomputeFeature();
    }
}

void Clipping::on_clipZHS_sliderMoved(int val)
{
    // we cannot cut to the edge because then the result is an empty shape
    if (val == d->ui.clipZHS->maximum()) {
        d->ui.clipZHS->setValue(d->ui.clipZHS->maximum() - d->ui.clipZHS->singleStep());
        return;
    }
    if (val == d->ui.clipZHS->minimum()) {
        d->ui.clipZHS->setValue(d->ui.clipZHS->minimum() + d->ui.clipZHS->singleStep());
        return;
    }
    // the slider value is % of the cut range
    double NewCutValue = d->ui.clipZ->minimum()
        + val / 100.0 * (d->ui.clipZ->maximum() - d->ui.clipZ->minimum());
    d->ui.clipZHS->setToolTip(QString::number(NewCutValue, 'g', Base::UnitsApi::getDecimals()));
    d->ui.clipZ->setValue(NewCutValue);
}

void Clipping::on_flipClipX_clicked()
{
    if (d->ui.ClippingRB->isChecked()) {
        d->flipX = !d->flipX;
        SbPlane pln = d->clipX->plane.getValue();
        d->clipX->plane.setValue(SbPlane(-pln.getNormal(), -pln.getDistanceFromOrigin()));
    }
    else { // we cut
        // we must move the box in x-direction by its Lenght
        // get the cut box
        auto doc = Application::Instance->activeDocument()->getDocument();
        auto CutBox = doc->getObject("CutViewBoxX");
        // there should be a box, but maybe the user deleted it meanwhile
        if (!CutBox) {
            Base::Console().Warning("CutView warning: there is no 'CutViewBoxX', trying to recreate it\n");
            // setting the Cutting RB will invoke onModeGBclicked that recreates the box
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        // get its placement and size
        Base::Placement placement = pcBox->Placement.getValue();
        Base::Vector3d BoxPosition = placement.getPosition();
        // flip the box
        if (d->ui.flipX->isChecked())
            BoxPosition.x = BoxPosition.x + pcBox->Length.getValue();
        else
            BoxPosition.x = BoxPosition.x - pcBox->Length.getValue();
        placement.setPosition(BoxPosition);
        pcBox->Placement.setValue(placement);

        pcBox->recomputeFeature();

        auto CutObject = doc->getObject("CutViewCutX");
        // there should be a box, but maybe the user deleted it meanwhile
        if (!CutObject) {
            Base::Console().Warning("CutView warning: there is no 'CutViewCutX', trying to recreate it\n");
            // setting the Cutting RB will invoke onModeGBclicked that recreates the box
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutObject);
        pcCut->recomputeFeature();

        // if there is another cut, we must recalculate it too
        // the hierarchy is always Z->Y->X
        if (hasBoxY && !hasBoxZ) { // only Y
            auto CutFeatureY = doc->getObject("CutViewCutY");
            if (!CutFeatureY) {
                Base::Console().Warning("CutView warning: the expected 'CutViewCutY' is missing, trying to recreate it\n");
                // setting the Cutting RB will invoke onModeGBclicked that recreates the box
                d->ui.CuttingRB->clicked(true);
                return;
            }
            Part::Cut* pcCutY = static_cast<Part::Cut*>(CutFeatureY);
            pcCutY->recomputeFeature();
        }
        else if ((!hasBoxY && hasBoxZ) || (hasBoxY && hasBoxZ)) { // only Z
            // the main cut is Z, no matter if there is a cut in Y
            auto CutFeatureZ = doc->getObject("CutViewCutZ");
            if (!CutFeatureZ) {
                Base::Console().Warning("CutView warning: the expected 'CutViewCutY' is missing, trying to recreate it\n");
                // setting the Cutting RB will invoke onModeGBclicked that recreates the box
                d->ui.CuttingRB->clicked(true);
                return;
            }
            Part::Cut* pcCutZ = static_cast<Part::Cut*>(CutFeatureZ);
            pcCutZ->recomputeFeature();
        }
    }
}

void Clipping::on_flipClipY_clicked()
{
    if (d->ui.ClippingRB->isChecked()) {
        d->flipY = !d->flipY;
        SbPlane pln = d->clipY->plane.getValue();
        d->clipY->plane.setValue(SbPlane(-pln.getNormal(), -pln.getDistanceFromOrigin()));
    }
    else { // we cut
        // we must move the box in y-direction by its Width
        // get the cut box
        auto doc = Application::Instance->activeDocument()->getDocument();
        auto CutBox = doc->getObject("CutViewBoxY");
        // there should be a box, but maybe the user deleted it meanwhile
        if (!CutBox) {
            Base::Console().Warning("CutView warning: there is no 'CutViewBoxY', trying to recreate it\n");
            // setting the Cutting RB will invoke onModeGBclicked that recreates the box
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        // get its placement and size
        Base::Placement placement = pcBox->Placement.getValue();
        Base::Vector3d BoxPosition = placement.getPosition();
        // flip the box
        if (d->ui.flipY->isChecked())
            BoxPosition.y = BoxPosition.y + pcBox->Width.getValue();
        else
            BoxPosition.y = BoxPosition.y - pcBox->Width.getValue();
        placement.setPosition(BoxPosition);
        pcBox->Placement.setValue(placement);

        pcBox->recomputeFeature();

        auto CutObject = doc->getObject("CutViewCutY");
        // there should be a box, but maybe the user deleted it meanwhile
        if (!CutObject) {
            Base::Console().Warning("CutView warning: there is no 'CutViewCutY', trying to recreate it\n");
            // setting the Cutting RB will invoke onModeGBclicked that recreates the box
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutObject);
        pcCut->recomputeFeature();

        // if there is another cut, we must recalculate it too
        // we only need to check for Z since the hierarchy is always Z->Y->X
        if (hasBoxZ) {
            auto CutFeatureZ = doc->getObject("CutViewCutZ");
            Part::Cut* pcCutZ = static_cast<Part::Cut*>(CutFeatureZ);
            pcCutZ->recomputeFeature();
        }
    }
}

void Clipping::on_flipClipZ_clicked()
{
    if (d->ui.ClippingRB->isChecked()) {
        d->flipZ = !d->flipZ;
        SbPlane pln = d->clipZ->plane.getValue();
        d->clipZ->plane.setValue(SbPlane(-pln.getNormal(), -pln.getDistanceFromOrigin()));
    }
    else { // we cut
        // we must move the box in z-direction by its Height
        // get the cut box
        auto doc = Application::Instance->activeDocument()->getDocument();
        auto CutBox = doc->getObject("CutViewBoxZ");
        // there should be a box, but maybe the user deleted it meanwhile
        if (!CutBox) {
            Base::Console().Warning("CutView warning: there is no 'CutViewBoxZ', trying to recreate it\n");
            // setting the Cutting RB will invoke onModeGBclicked that recreates the box
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Box* pcBox = static_cast<Part::Box*>(CutBox);
        // get its placement and size
        Base::Placement placement = pcBox->Placement.getValue();
        Base::Vector3d BoxPosition = placement.getPosition();
        // flip the box
        if (d->ui.flipZ->isChecked())
            BoxPosition.z = BoxPosition.z + pcBox->Height.getValue();
        else
            BoxPosition.z = BoxPosition.z - pcBox->Height.getValue();
        placement.setPosition(BoxPosition);
        pcBox->Placement.setValue(placement);

        pcBox->recomputeFeature();

        auto CutObject = doc->getObject("CutViewCutZ");
        // there should be a box, but maybe the user deleted it meanwhile
        if (!CutObject) {
            Base::Console().Warning("CutView warning: there is no 'CutViewCutZ', trying to recreate it\n");
            // setting the Cutting RB will invoke onModeGBclicked that recreates the box
            d->ui.CuttingRB->clicked(true);
            return;
        }
        Part::Cut* pcCut = static_cast<Part::Cut*>(CutObject);
        pcCut->recomputeFeature();
    }
}

void Clipping::on_RefreshCutPB_clicked()
{
    // refresh the list of document objects and the visible objects
    
    // get all objects in the document
    auto doc = Application::Instance->activeDocument()->getDocument();
    if (!doc) {
        Base::Console().Error("Clipping error: there is document\n");
        return;
    }
    ObjectsList = doc->getObjects();
    if (ObjectsList.empty()) {
        Base::Console().Error("Clipping error: there are no objects in the document\n");
        return;
    }
    // empty the ObjectsListVisible
    ObjectsListVisible.clear();
    // now store those that are currently visible
    for (auto it = ObjectsList.begin(); it != ObjectsList.end(); ++it) {
        if ((*it)->Visibility.getValue())
            ObjectsListVisible.push_back((*it));
    }
}

void Clipping::on_groupBoxView_toggled(bool on)
{
    if (on) {
        d->ui.groupBoxX->setChecked(false);
        d->ui.groupBoxY->setChecked(false);
        d->ui.groupBoxZ->setChecked(false);
    }

    d->clipView->on.setValue(on);
}

void Clipping::on_clipView_valueChanged(double val)
{
    if (d->ui.ClippingRB->isChecked()) {
        SbPlane pln = d->clipView->plane.getValue();
        d->clipView->plane.setValue(SbPlane(pln.getNormal(), val));
    }
    else { // we cut
        ;
    }
}

void Clipping::on_fromView_clicked()
{
    if (d->view) {
        Gui::View3DInventorViewer* view = d->view->getViewer();
        SbVec3f dir = view->getViewDirection();
        SbPlane pln = d->clipView->plane.getValue();
        d->clipView->plane.setValue(SbPlane(dir,pln.getDistanceFromOrigin()));
    }
}

void Clipping::on_adjustViewdirection_toggled(bool on)
{
    d->ui.dirX->setDisabled(on);
    d->ui.dirY->setDisabled(on);
    d->ui.dirZ->setDisabled(on);
    d->ui.fromView->setDisabled(on);

    if (on)
        d->sensor->schedule();
    else
        d->sensor->unschedule();
}

void Clipping::on_dirX_valueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

void Clipping::on_dirY_valueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

void Clipping::on_dirZ_valueChanged(double)
{
    double x = d->ui.dirX->value();
    double y = d->ui.dirY->value();
    double z = d->ui.dirZ->value();

    SbPlane pln = d->clipView->plane.getValue();
    SbVec3f normal(x,y,z);
    if (normal.sqrLength() > 0.0f)
        d->clipView->plane.setValue(SbPlane(normal,pln.getDistanceFromOrigin()));
}

SbBox3f Clipping::getViewBoundingBox()
{
    auto docGui = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(docGui->getActiveView());
    Gui::View3DInventorViewer* viewer = view->getViewer();
    SoCamera* camera = viewer->getSoRenderManager()->getCamera();
    SbBox3f Box;
    if (!camera || !camera->isOfType(SoOrthographicCamera::getClassTypeId()))
        return Box; // return an empty box
    // get scene bounding box
    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(viewer->getSceneGraph());
    return action.getBoundingBox();
}

void Clipping::refreshCutRanges(SbBox3f BoundingBox,
    bool forXValue, bool forYValue, bool forZValue,
    bool forXRange, bool forYRange, bool forZRange, bool forCustom)
{
    if (!BoundingBox.isEmpty()) {
        SbVec3f cnt = BoundingBox.getCenter();
        if (forCustom) {
            d->ui.clipView->setValue(cnt[2]);
        }
        if (forXValue) {
            d->ui.clipX->setValue(cnt[0]);
            d->ui.clipXHS->setValue(50);
        }
        if (forYValue) {
            d->ui.clipY->setValue(cnt[1]);
            d->ui.clipYHS->setValue(50);
        }
        if (forZValue) {
            d->ui.clipZ->setValue(cnt[2]);
            d->ui.clipZHS->setValue(50);
        }
        
        int minDecimals = Base::UnitsApi::getDecimals();
        float lenx, leny, lenz;
        BoundingBox.getSize(lenx, leny, lenz);
        int steps = 100;
        float minlen = std::min<float>(lenx, std::min<float>(leny, lenz));

        // set the min and max values
        if (forXRange)
            d->ui.clipX->setRange(cnt[0] - lenx / 2, cnt[0] + lenx / 2);
        if (forYRange)
            d->ui.clipY->setRange(cnt[1] - leny / 2, cnt[1] + leny / 2);
        if (forZRange)
            d->ui.clipZ->setRange(cnt[2] - lenz / 2, cnt[2] + lenz / 2);
        // determine the single step values
        if (forCustom) {
            minlen = minlen / steps;
            int dim = static_cast<int>(log10(minlen));
            double singleStep = pow(10.0, dim);
            d->ui.clipView->setSingleStep(singleStep);
            minDecimals = std::max(minDecimals, -dim);
        }
        if (forXRange) {
            lenx = lenx / steps;
            int dim = static_cast<int>(log10(lenx));
            double singleStep = pow(10.0, dim);
            d->ui.clipX->setSingleStep(singleStep);
            d->ui.clipXHS->setSingleStep(singleStep);
        }
        if (forYRange) {
            leny = leny / steps;
            int dim = static_cast<int>(log10(leny));
            double singleStep = pow(10.0, dim);
            d->ui.clipY->setSingleStep(singleStep);
            d->ui.clipYHS->setSingleStep(singleStep);
        }
        if (forZRange) {
            lenz = lenz / steps;
            int dim = static_cast<int>(log10(lenz));
            double singleStep = pow(10.0, dim);
            d->ui.clipZ->setSingleStep(singleStep);
            d->ui.clipZHS->setSingleStep(singleStep);
        }

        // set decimals
        d->ui.clipView->setDecimals(minDecimals);
        d->ui.clipX->setDecimals(minDecimals);
        d->ui.clipY->setDecimals(minDecimals);
        d->ui.clipZ->setDecimals(minDecimals);
    }
}

#include "moc_Clipping.cpp"
