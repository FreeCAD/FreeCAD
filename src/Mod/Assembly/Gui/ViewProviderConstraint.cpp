/***************************************************************************
 *   Copyright (c) 2013 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include "ViewProviderConstraint.h"
//#include "TaskAssemblyConstraints.h"
//#include "TaskDlgAssemblyConstraints.h"
#include "Mod/Assembly/App/Constraint.h"
#include "Mod/Assembly/App/PartRef.h"
#include <Mod/Part/App/PartFeature.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <Gui/Control.h>
#include <BRep_Builder.hxx>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoSwitch.h>

using namespace AssemblyGui;

PROPERTY_SOURCE(AssemblyGui::ViewProviderConstraintInternal, PartGui::ViewProviderPart)

ViewProviderConstraintInternal::ViewProviderConstraintInternal()
{
    //constraint entity color
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    unsigned long scol = hGrp->GetUnsigned("ConstructionColor", 56319UL);
    float r, g, b;
    r = ((scol >> 24) & 0xff) / 255.0;
    g = ((scol >> 16) & 0xff) / 255.0;
    b = ((scol >> 8) & 0xff) / 255.0;

    long unsigned ccol = hGrp->GetUnsigned("FullyConstrainedColor", 16711935UL);
    float r2, g2, b2;
    r2 = ((ccol >> 24) & 0xff) / 255.0;
    g2 = ((ccol >> 16) & 0xff) / 255.0;
    b2 = ((ccol >> 8) & 0xff) / 255.0;


    int lwidth = hGrp->GetInt("DefaultShapeLineWidth", 2) + 1;
    App::Material mat;
    mat.ambientColor.set(0.2f, 0.2f, 0.2f);
    mat.diffuseColor.set(r2, g2, b2);
    mat.specularColor.set(0.0f, 0.0f, 0.0f);
    mat.emissiveColor.set(0.0f, 0.0f, 0.0f);
    mat.shininess = 1.0f;
    mat.transparency = 0.5f;
    LineMaterial.setValue(mat);
    PointMaterial.setValue(mat);
    LineColor.setValue(mat.diffuseColor);
    PointColor.setValue(mat.diffuseColor);
    mat.diffuseColor.set(r, g, b);
    DiffuseColor.setValue(mat.diffuseColor);
    LineWidth.setValue(lwidth);
    PointSize.setValue(lwidth);

    Transparency.setValue(50);
    
};

void ViewProviderConstraintInternal::updateVis(const TopoDS_Shape& shape)
{
    updateVisual(shape);
};

void ViewProviderConstraintInternal::updatePlacement(const Base::Placement& p)
{
    float q0 = (float)p.getRotation().getValue()[0];
    float q1 = (float)p.getRotation().getValue()[1];
    float q2 = (float)p.getRotation().getValue()[2];
    float q3 = (float)p.getRotation().getValue()[3];
    float px = (float)p.getPosition().x;
    float py = (float)p.getPosition().y;
    float pz = (float)p.getPosition().z;
    pcTransform->rotation.setValue(q0, q1, q2, q3);
    pcTransform->translation.setValue(px, py, pz);
    pcTransform->center.setValue(0.0f, 0.0f, 0.0f);
}

void ViewProviderConstraintInternal::switch_node(bool onoff)
{
    if(onoff)
        pcModeSwitch->whichChild = 0;
    else
        pcModeSwitch->whichChild = -1;
}

void ViewProviderConstraint::setDisplayMode(const char* ModeName)
{
    setDisplayMaskMode("Flat Lines");

}

std::vector<std::string> ViewProviderConstraint::getDisplayModes(void) const
{
    std::vector<std::string> StrList;

    // add your own mode
    StrList.push_back("Flat Lines");
    return StrList;
}


PROPERTY_SOURCE(AssemblyGui::ViewProviderConstraint, PartGui::ViewProviderPart)

ViewProviderConstraint::ViewProviderConstraint() : m_selected(false)
{
    Selectable.setValue(false);

    //constraint entity color
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    unsigned long scol = hGrp->GetUnsigned("ConstructionColor", 56319UL);
    float r, g, b;
    r = ((scol >> 24) & 0xff) / 255.0;
    g = ((scol >> 16) & 0xff) / 255.0;
    b = ((scol >> 8) & 0xff) / 255.0;

    long unsigned ccol = hGrp->GetUnsigned("FullyConstrainedColor", 16711935UL);
    float r2, g2, b2;
    r2 = ((ccol >> 24) & 0xff) / 255.0;
    g2 = ((ccol >> 16) & 0xff) / 255.0;
    b2 = ((ccol >> 8) & 0xff) / 255.0;


    int lwidth = hGrp->GetInt("DefaultShapeLineWidth", 2) + 1;
    App::Material mat;
    mat.ambientColor.set(0.2f, 0.2f, 0.2f);
    mat.diffuseColor.set(r2, g2, b2);
    mat.specularColor.set(0.0f, 0.0f, 0.0f);
    mat.emissiveColor.set(0.0f, 0.0f, 0.0f);
    mat.shininess = 1.0f;
    mat.transparency = 0.5f;
    LineMaterial.setValue(mat);
    PointMaterial.setValue(mat);
    LineColor.setValue(mat.diffuseColor);
    PointColor.setValue(mat.diffuseColor);
    mat.diffuseColor.set(r, g, b);
    DiffuseColor.setValue(mat.diffuseColor);
    LineWidth.setValue(lwidth);
    PointSize.setValue(lwidth);

    Transparency.setValue(50);
}

bool ViewProviderConstraint::isShow() const
{
    return Visibility.getValue();
}


void ViewProviderConstraint::attach(App::DocumentObject* pcFeat)
{
    SoAnnotation* m_anno1 = new SoAnnotation;
    SoAnnotation* m_anno2 = new SoAnnotation;

    //call parent attach method for normal processing of one visual path
    ViewProviderPart::attach(pcFeat);
    //bring a annotation node before the normal view mode (others are not used)
    m_anno1->addChild(pcModeSwitch->getChild(0));
    pcModeSwitch->replaceChild(0, m_anno1);

    //now also attach a second visual path to the root for our second constraint element
    internal_vp.attach(pcFeat);
    pcRoot->addChild(m_anno2);
    m_anno2->addChild(internal_vp.getRoot());

    internal_vp.setDisplayMM("Flat Lines");
}

void ViewProviderConstraint::update(const App::Property* prop) {

    if(Visibility.getValue() && m_selected) {

        draw();
    }
    ViewProviderPart::update(prop);
}


void ViewProviderConstraint::updateData(const App::Property* prop) {
  
    //set the icon
    int v = dynamic_cast<Assembly::Constraint*>(getObject())->Type.getValue();
    if(v == 4)
      sPixmap = "constraints/Assembly_ConstraintAlignment";
    if(v == 3)
      sPixmap = "constraints/Assembly_ConstraintAngle";
    if(v == 5)
      sPixmap = "constraints/Assembly_ConstraintCoincidence";
    if(v == 1)
      sPixmap = "constraints/Assembly_ConstraintDistance";
    if(v == 0)
      sPixmap = "constraints/Assembly_ConstraintLock";
    if(v == 2)
      sPixmap = "constraints/Assembly_ConstraintOrientation";
    if(v==6)
      sPixmap = "constraints/Assembly_ConstraintGeneral";
    
    if(Visibility.getValue() && m_selected) {

        draw();
    }

    Gui::ViewProviderGeometryObject::updateData(prop);
    internal_vp.Gui::ViewProviderGeometryObject::updateData(prop);
}

void ViewProviderConstraint::onChanged(const App::Property* prop)
{   
    // parent expects the app object to be part::feature, but it isn't. so we have to avoid
    // the visibility prop as this results in accessing of the part::feature and would crash
    if(prop == &Visibility) {
        if(Visibility.getValue() && m_selected) {
            internal_vp.show();
            draw();
        }
        else
            internal_vp.hide();

        ViewProviderGeometryObject::onChanged(prop);
        internal_vp.onChGO(prop);
    }
    else {
        ViewProviderPart::onChanged(prop);
        internal_vp.onChPa(prop);
    }
}

void ViewProviderConstraint::draw()
{

    TopoDS_Shape s1 = getConstraintShape(1);
    updateVisual(s1);

    TopoDS_Shape s2 = getConstraintShape(2);
    internal_vp.updateVis(s2);

    App::DocumentObject* obj1 = dynamic_cast<Assembly::Constraint*>(pcObject)->First.getValue();

    if(!obj1)
        return;

    Assembly::PartRef* part1 = static_cast<Assembly::PartRef*>(obj1);

    if(!part1)
        return;

    //the internal draw algorithm removes all locations. but we have this subshape extracted
    //from a complex one, therefore it's translation is not respected in the parts rotation
    //and if it gets cut away the geometry will be at wrong position
    TopLoc_Location l1 = s1.Location();
    gp_XYZ tr1 = l1.Transformation().TranslationPart();
    Base::Placement p1(Base::Vector3d(tr1.X(), tr1.Y(), tr1.Z()), Base::Rotation());
   // upstream_placement(p1, part1);
    //p1 = part1->m_part->getGlobal<Base::Placement>() * p1;

    float q0 = (float)p1.getRotation().getValue()[0];
    float q1 = (float)p1.getRotation().getValue()[1];
    float q2 = (float)p1.getRotation().getValue()[2];
    float q3 = (float)p1.getRotation().getValue()[3];
    float px = (float)p1.getPosition().x;
    float py = (float)p1.getPosition().y;
    float pz = (float)p1.getPosition().z;
    pcTransform->rotation.setValue(q0, q1, q2, q3);
    pcTransform->translation.setValue(px, py, pz);
    pcTransform->center.setValue(0.0f, 0.0f, 0.0f);

    //Second part
    //***********
    App::DocumentObject* obj2 = dynamic_cast<Assembly::Constraint*>(pcObject)->Second.getValue();

    if(!obj2)
        return;

    //here it's a bit more involved, as the coind tree structure let's the first transform node
    //transform the second part too.
    Assembly::PartRef* part2 = static_cast<Assembly::PartRef*>(obj2);

    if(!part2)
        return;

    //the internal draw algorithm removes all locations. but we have this subshape extracted
    //from a complex one, therefore it's shape internal translation is not respected in the parts rotation
    //and if it gets cut away the geometry will be at wrong position
    TopLoc_Location l2 = s2.Location();
    gp_XYZ tr2 = l2.Transformation().TranslationPart();
    Base::Placement p2(Base::Vector3d(tr2.X(), tr2.Y(), tr2.Z()), Base::Rotation());
    //upstream_placement(p2, part2);

    p2 = p1.inverse()*p2;
    //p2 = p1.inverse() * (part2->m_part->getGlobal<Base::Placement>() * p2);
    internal_vp.updatePlacement(p2);
}

void ViewProviderConstraint::upstream_placement(Base::Placement& p, Assembly::Item* item) {

    //successive transformation for this item
    p = item->Placement.getValue() * p;

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    //get all links to this item and see if we have more Products
    const std::vector<App::DocumentObject*>& vector = item->getInList();
    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::Product::getClassTypeId()) {

            upstream_placement(p, static_cast<Assembly::Product*>(*it));
            return;
        }
    };
};

void ViewProviderConstraint::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if(Gui::Selection().isSelected(pcObject) && Visibility.getValue()) {
        m_selected = true;

        internal_vp.switch_node(true);
        pcModeSwitch->whichChild = 0;
        draw();
    }
    else if(!isEditing()) {
        internal_vp.switch_node(false);
        pcModeSwitch->whichChild = -1;
        m_selected = false;
    }
}


TopoDS_Shape ViewProviderConstraint::getConstraintShape(int link)
{

    //if(link == 1) {
    //    //subshape of first link
    //    //**********************
    //    App::DocumentObject* obj1 = dynamic_cast<Assembly::Constraint*>(pcObject)->First.getValue();

    //    if(!obj1)
    //        return TopoDS_Shape();

    //    Assembly::PartRef* part1 = static_cast<Assembly::PartRef*>(obj1);

    //    if(!part1)
    //        return TopoDS_Shape();

    //    Part::TopoShape ts;
    //    App::DocumentObject* feature1 = part1->Model.getValue();

    //    if(feature1->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
    //        ts = static_cast<Part::Feature*>(feature1)->Shape.getShape();
    //    }
    //    else
    //        return TopoDS_Shape();

    //    TopoDS_Shape s1 = ts.getSubShape(dynamic_cast<Assembly::Constraint*>(pcObject)->First.getSubValues()[0].c_str());

    //    return s1;
    //}
    //else {
    //    //subshape of second link
    //    //**********************
    //    App::DocumentObject* obj2 = dynamic_cast<Assembly::Constraint*>(pcObject)->Second.getValue();

    //    if(!obj2)
    //        return TopoDS_Shape();

    //    Assembly::PartRef* part2 = static_cast<Assembly::PartRef*>(obj2);

    //    if(!part2)
    //        return TopoDS_Shape();

    //    Part::TopoShape ts2;
    //    App::DocumentObject* feature2 = part2->Model.getValue();

    //    if(feature2->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
    //        ts2 = static_cast<Part::Feature*>(feature2)->Shape.getShape();
    //    }
    //    else
    //        return TopoDS_Shape();

    //    TopoDS_Shape s2 = ts2.getSubShape(dynamic_cast<Assembly::Constraint*>(pcObject)->Second.getSubValues()[0].c_str());

    //    return s2;
    //};
	return TopoDS_Shape();
}

void ViewProviderConstraint::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
}

bool ViewProviderConstraint::setEdit(int ModNum)
{

    //// When double-clicking on the item for this sketch the
    //// object unsets and sets its edit mode without closing
    //// the task panel
    //Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    //TaskDlgAssemblyConstraints* ConstraintsDlg = qobject_cast<TaskDlgAssemblyConstraints*>(dlg);

    //// start the edit dialog
    //if(ConstraintsDlg)
    //    Gui::Control().showDialog(ConstraintsDlg);
    //else
    //    Gui::Control().showDialog(new TaskDlgAssemblyConstraints(this));

    ////show the constraint geometries
    //internal_vp.switch_node(true);
    //pcModeSwitch->whichChild = 0;
    //draw();

    //return true;
	return false;
}

void ViewProviderConstraint::unsetEdit(int ModNum)
{
    if(!Gui::Selection().isSelected(pcObject) || !Visibility.getValue()) {
        internal_vp.switch_node(false);
        pcModeSwitch->whichChild = -1;
        m_selected = false;
    }
}

