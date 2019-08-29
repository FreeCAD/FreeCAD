/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
# include <Standard_math.hxx>

# include <QAction>
# include <QApplication>
# include <QDockWidget>
# include <QMenu>
# include <QStackedWidget>

# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoFontStyle.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoRotation.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCylinder.h>
# include <Inventor/nodes/SoCone.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoComplexity.h>
#endif

#include "ViewProviderFemConstraint.h"
#include "TaskFemConstraint.h"

#include "Gui/Control.h"
#include "Gui/MainWindow.h"
#include "Gui/Command.h"
#include "Gui/Application.h"
#include "Gui/Document.h"

#include <Base/Console.h>

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraint, Gui::ViewProviderDocumentObject)


ViewProviderFemConstraint::ViewProviderFemConstraint()
{
    ADD_PROPERTY(TextColor,(0.0f,0.0f,0.0f));
    ADD_PROPERTY(FaceColor,(1.0f,0.0f,0.2f));
    ADD_PROPERTY(ShapeColor,(1.0f,0.0f,0.2f));
    ADD_PROPERTY(FontSize,(18));
    ADD_PROPERTY(DistFactor,(1.0));
    ADD_PROPERTY(Mirror,(false));

    pFont = new SoFontStyle();
    pFont->ref();
    pLabel = new SoText2();
    pLabel->ref();
    pTextColor = new SoBaseColor();
    pTextColor->ref();

    pMaterials = new SoBaseColor();
    pMaterials->ref();
    pMaterials->rgb.setValue(1.0f, 0.0f, 0.2f);

    pShapeSep = new SoSeparator();
    pShapeSep->ref();

    TextColor.touch();
    FontSize.touch();
    FaceColor.touch();

    wizardWidget = NULL;
    wizardSubLayout = NULL;
    constraintDialog = NULL;
}

ViewProviderFemConstraint::~ViewProviderFemConstraint()
{
    pFont->unref();
    pLabel->unref();
    pTextColor->unref();
    pMaterials->unref();
    pShapeSep->unref();
}

void ViewProviderFemConstraint::attach(App::DocumentObject* pcObject)
{
    ViewProviderDocumentObject::attach(pcObject);

    SoPickStyle* ps = new SoPickStyle();
    ps->style = SoPickStyle::UNPICKABLE;

    SoSeparator* sep = new SoSeparator();
    SoShapeHints* hints = new SoShapeHints();
    hints->shapeType.setValue(SoShapeHints::UNKNOWN_SHAPE_TYPE);
    hints->vertexOrdering.setValue(SoShapeHints::COUNTERCLOCKWISE);
    sep->addChild(ps);
    sep->addChild(hints);
    sep->addChild(pMaterials);
    sep->addChild(pShapeSep);
    addDisplayMaskMode(sep, "Base");
}

std::vector<std::string> ViewProviderFemConstraint::getDisplayModes(void) const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.push_back("Base");
    return StrList;
}

void ViewProviderFemConstraint::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<App::DocumentObject*> ViewProviderFemConstraint::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>();
}

void ViewProviderFemConstraint::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit constraint"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
}

void ViewProviderFemConstraint::onChanged(const App::Property* prop)
{
    if (prop == &Mirror || prop == &DistFactor) {
        updateData(prop);
    }
    else if (prop == &TextColor) {
        const App::Color& c = TextColor.getValue();
        pTextColor->rgb.setValue(c.r,c.g,c.b);
    }
    else if (prop == &FaceColor) {
        const App::Color& c = FaceColor.getValue();
        pMaterials->rgb.setValue(c.r,c.g,c.b);
    }
    else if (prop == &FontSize) {
        pFont->size = FontSize.getValue();
    }
    else {
        ViewProviderDocumentObject::onChanged(prop);
    }
}

//OvG: Visibility automation show parts and hide meshes on activation of a constraint
std::string ViewProviderFemConstraint::gethideMeshShowPartStr(const std::string showConstr)
{
    return "for amesh in App.activeDocument().Objects:\n\
    if \""+showConstr+"\" == amesh.Name:\n\
        amesh.ViewObject.Visibility = True\n\
    elif \"Mesh\" in amesh.TypeId:\n\
        amesh.ViewObject.Visibility = False\n";
}

std::string ViewProviderFemConstraint::gethideMeshShowPartStr()
{
    return ViewProviderFemConstraint::gethideMeshShowPartStr("");
}

bool ViewProviderFemConstraint::setEdit(int ModNum)
{
    Gui::Command::doCommand(Gui::Command::Doc,"%s",ViewProviderFemConstraint::gethideMeshShowPartStr().c_str());
    return Gui::ViewProviderGeometryObject::setEdit(ModNum);
}

void ViewProviderFemConstraint::unsetEdit(int ModNum)
{
    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    if ((wizardWidget != NULL) && (wizardSubLayout != NULL) && (constraintDialog != NULL)) {
        wizardWidget = NULL;
        wizardSubLayout = NULL;
        delete constraintDialog;
        constraintDialog = NULL;

        // Notify the Shaft Wizard that we have finished editing
        // See WizardShaft.py on why we do it this way
        Gui::Command::runCommand(Gui::Command::Doc, "Gui.runCommand('PartDesign_WizardShaftCallBack')");

    } else {
        if (ModNum == ViewProvider::Default) {
            // when pressing ESC make sure to close the dialog
            Gui::Control().closeDialog();
        }
        else {
            ViewProviderDocumentObject::unsetEdit(ModNum);
        }
    }
}
/*
// Create a local coordinate system with the z-axis given in dir
void getLocalCoordinateSystem(const SbVec3f& z, SbVec3f& y, SbVec3f& x)
{
    // Find the y axis in an arbitrary direction, normal to z
    // Conditions:
    // y1 * z1 + y2 * z2 + y3 * z3 = |y| |z| cos(90°) = 0
    // |y| = sqrt(y1^2 + y2^2 + y3^2) = 1
    float z1, z2, z3;
    z.getValue(z1, z2, z3);
    float y1, y2, y3;
    if (fabs(z1) > Precision::Confusion()) {
        // Choose: y3 = 0
        // Solution:
        // y1 * z1 + y2 * z2 = 0
        // y1 = - z2/z1 y2
        // sqrt(z2^2/z1^2 y2^2 + y2^2) = 1
        // y2^2 ( 1 + z2^2/z1^2)) = +-1 -> choose +1 otherwise no solution
        // y2 = +- sqrt(1 / (1 + z2^2/z1^2))
        y3 = 0;
        y2 = sqrt(1 / (1 + z2*z2 / (z1*z1)));
        y1 = -z2/z1 * y2;
        // Note: result might be (0, 1, 0)
    } else if (fabs(z2) > Precision::Confusion()) {
        // Given: z1 = 0
        // Choose: y1 = 0
        // Solution:
        // y2 * z2 + y3 * z3 = 0
        // y2 = - z3/z2 y3
        // sqrt(z3^2/z2^2 y3^3 + y3^2) = 1
        // y3^2 (1 + z3^2/z2^2)) = +1
        // y3 = +- sqrt(1 / (1 + z3^2/z2^2))
        y1 = 0;
        y3 = sqrt(1 / (1 + z3*z3 / (z2*z2)));
        y2 = -z3/z2 * y3;
        // Note: result might be (0, 0, 1)
    } else if  (fabs(z3) > Precision::Confusion()) {
        // Given: z1 = z2 = 0
        // Choose the remaining possible axis
        y1 = 1;
        y2 = 0;
        y3 = 0;
    }

    y = SbVec3f(y1, y2, y3);
    x = y.cross(z);
}
*/
#define PLACEMENT_CHILDREN 2

void ViewProviderFemConstraint::createPlacement(SoSeparator* sep, const SbVec3f &base, const SbRotation &r)
{
    SoTranslation* trans = new SoTranslation();
    trans->translation.setValue(base);
    sep->addChild(trans);
    SoRotation* rot = new SoRotation();
    rot->rotation.setValue(r);
    sep->addChild(rot);
}

void ViewProviderFemConstraint::updatePlacement(const SoSeparator* sep, const int idx, const SbVec3f &base, const SbRotation &r)
{
    SoTranslation* trans = static_cast<SoTranslation*>(sep->getChild(idx));
    trans->translation.setValue(base);
    SoRotation* rot = static_cast<SoRotation*>(sep->getChild(idx+1));
    rot->rotation.setValue(r);
}

#define CONE_CHILDREN 2

void ViewProviderFemConstraint::createCone(SoSeparator* sep, const double height, const double radius)
{
    // Adjust cone so that the tip is on base
    SoTranslation* trans = new SoTranslation();
    trans->translation.setValue(SbVec3f(0,-height/2,0));
    sep->addChild(trans);
    SoCone* cone = new SoCone();
    cone->height.setValue(height);
    cone->bottomRadius.setValue(radius);
    sep->addChild(cone);
}

SoSeparator* ViewProviderFemConstraint::createCone(const double height, const double radius)
{
    // Create a new cone node
    SoSeparator* sep = new SoSeparator();
    createCone(sep, height, radius);
    return sep;
}

void ViewProviderFemConstraint::updateCone(const SoNode* node, const int idx, const double height, const double radius)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    SoTranslation* trans = static_cast<SoTranslation*>(sep->getChild(idx));
    trans->translation.setValue(SbVec3f(0,-height/2,0));
    SoCone* cone = static_cast<SoCone*>(sep->getChild(idx+1));
    cone->height.setValue(height);
    cone->bottomRadius.setValue(radius);
}

#define CYLINDER_CHILDREN 1

void ViewProviderFemConstraint::createCylinder(SoSeparator* sep, const double height, const double radius)
{
    SoCylinder* cyl = new SoCylinder();
    cyl->height.setValue(height);
    cyl->radius.setValue(radius);
    sep->addChild(cyl);
}

SoSeparator* ViewProviderFemConstraint::createCylinder(const double height, const double radius)
{
    // Create a new cylinder node
    SoSeparator* sep = new SoSeparator();
    createCylinder(sep, height, radius);
    return sep;
}

void ViewProviderFemConstraint::updateCylinder(const SoNode* node, const int idx, const double height, const double radius)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    SoCylinder* cyl = static_cast<SoCylinder*>(sep->getChild(idx));
    cyl->height.setValue(height);
    cyl->radius.setValue(radius);
}

#define CUBE_CHILDREN 1

void ViewProviderFemConstraint::createCube(SoSeparator* sep, const double width, const double length, const double height)
{
    SoCube* cube = new SoCube();
    cube->width.setValue(width);
    cube->depth.setValue(length);
    cube->height.setValue(height);
    sep->addChild(cube);
}

SoSeparator* ViewProviderFemConstraint::createCube(const double width, const double length, const double height)
{
    SoSeparator* sep = new SoSeparator();
    createCube(sep, width, length, height);
    return sep;
}

void ViewProviderFemConstraint::updateCube(const SoNode* node, const int idx, const double width, const double length, const double height)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    SoCube* cube = static_cast<SoCube*>(sep->getChild(idx));
    cube->width.setValue(width);
    cube->depth.setValue(length);
    cube->height.setValue(height);
}

#define ARROW_CHILDREN (CONE_CHILDREN + PLACEMENT_CHILDREN + CYLINDER_CHILDREN)

void ViewProviderFemConstraint::createArrow(SoSeparator* sep, const double length, const double radius)
{
    createCone(sep, radius, radius/2);
    createPlacement(sep, SbVec3f(0, -radius/2-(length-radius)/2, 0), SbRotation());
    createCylinder(sep, length-radius, radius/5);
}

SoSeparator* ViewProviderFemConstraint::createArrow(const double length, const double radius)
{
    SoSeparator* sep = new SoSeparator();
    createArrow(sep, length, radius);
    return sep;
}

void ViewProviderFemConstraint::updateArrow(const SoNode* node, const int idx, const double length, const double radius)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    updateCone(sep, idx, radius, radius/2);
    updatePlacement(sep, idx+CONE_CHILDREN, SbVec3f(0, -radius/2-(length-radius)/2, 0), SbRotation());
    updateCylinder(sep, idx+CONE_CHILDREN+PLACEMENT_CHILDREN, length-radius, radius/5);
}

#define FIXED_CHILDREN (CONE_CHILDREN + PLACEMENT_CHILDREN + CUBE_CHILDREN)

void ViewProviderFemConstraint::createFixed(SoSeparator* sep, const double height, const double width, const bool gap)
{
    createCone(sep, height-width/4, height-width/4);
    createPlacement(sep, SbVec3f(0, -(height-width/4)/2-width/8 - (gap ? 1.0 : 0.1) * width/8, 0), SbRotation());
    createCube(sep, width, width, width/4);
}

SoSeparator* ViewProviderFemConstraint::createFixed(const double height, const double width, const bool gap)
{
    SoSeparator* sep = new SoSeparator();
    createFixed(sep, height, width, gap);
    return sep;
}

void ViewProviderFemConstraint::updateFixed(const SoNode* node, const int idx, const double height, const double width, const bool gap)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    updateCone(sep, idx, height-width/4, height-width/4);
    updatePlacement(sep, idx+CONE_CHILDREN, SbVec3f(0, -(height-width/4)/2-width/8 - (gap ? 1.0 : 0.0) * width/8, 0), SbRotation());
    updateCube(sep, idx+CONE_CHILDREN+PLACEMENT_CHILDREN, width, width, width/4);
}

void ViewProviderFemConstraint::createDisplacement(SoSeparator* sep, const double height, const double width, const bool gap)
{
    createCone(sep, height, width);
    createPlacement(sep, SbVec3f(0, -(height)/2-width/8 - (gap ? 1.0 : 0.1) * width/8, 0), SbRotation());
}

SoSeparator* ViewProviderFemConstraint::createDisplacement(const double height, const double width, const bool gap)
{
    SoSeparator* sep = new SoSeparator();
    createDisplacement(sep, height, width, gap);
    return sep;
}

void ViewProviderFemConstraint::updateDisplacement(const SoNode* node, const int idx, const double height, const double width, const bool gap)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    updateCone(sep, idx, height, width);
    updatePlacement(sep, idx+CONE_CHILDREN, SbVec3f(0, -(height)/2-width/8 - (gap ? 1.0 : 0.0) * width/8, 0), SbRotation());
}

void ViewProviderFemConstraint::createRotation(SoSeparator* sep, const double height, const double width, const bool gap)
{
    createCylinder(sep, width/2, height/2);
    createPlacement(sep, SbVec3f(0, -(height)*2-width/8 - (gap ? 1.0 : 0.1) * width/8, 0), SbRotation());
}

SoSeparator* ViewProviderFemConstraint::createRotation(const double height, const double width, const bool gap)
{
    SoSeparator* sep = new SoSeparator();
    createRotation(sep, height, width, gap);
    return sep;
}

void ViewProviderFemConstraint::updateRotation(const SoNode* node, const int idx, const double height, const double width, const bool gap)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    updateCylinder(sep, idx, height/2, width/2);
    updatePlacement(sep, idx+CYLINDER_CHILDREN, SbVec3f(0, -(height)*2-width/8 - (gap ? 1.0 : 0.0) * width/8, 0), SbRotation());
}

QObject* ViewProviderFemConstraint::findChildByName(const QObject* parent, const QString& name)
{
    for (QObjectList::const_iterator o = parent->children().begin(); o != parent->children().end(); o++) {
        if ((*o)->objectName() == name)
            return *o;
        if (!(*o)->children().empty()) {
            QObject* result = findChildByName(*o, name);
            if (result != NULL)
                return result;
        }
    }

    return NULL;
}

void ViewProviderFemConstraint::checkForWizard()
{
    wizardWidget= NULL;
    wizardSubLayout = NULL;
    Gui::MainWindow* mw = Gui::getMainWindow();
    if (mw == NULL) return;
    QDockWidget* dw = mw->findChild<QDockWidget*>(QString::fromLatin1("Combo View"));
    if (dw == NULL) return;
    QWidget* cw = dw->findChild<QWidget*>(QString::fromLatin1("Combo View"));
    if (cw == NULL) return;
    QTabWidget* tw = cw->findChild<QTabWidget*>(QString::fromLatin1("combiTab"));
    if (tw == NULL) return;
    QStackedWidget* sw = tw->findChild<QStackedWidget*>(QString::fromLatin1("qt_tabwidget_stackedwidget"));
    if (sw == NULL) return;
    QScrollArea* sa = sw->findChild<QScrollArea*>();
    if (sa== NULL) return;
    QWidget* wd = sa->widget(); // This is the reason why we cannot use findChildByName() right away!!!
    if (wd == NULL) return;
    QObject* wiz = findChildByName(wd, QString::fromLatin1("ShaftWizard"));
    if (wiz != NULL) {
        wizardWidget = static_cast<QVBoxLayout*>(wiz);
        wizardSubLayout = wiz->findChild<QVBoxLayout*>(QString::fromLatin1("ShaftWizardLayout"));
    }
}


// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(FemGui::ViewProviderFemConstraintPython, FemGui::ViewProviderFemConstraint)
/// @endcond

// explicit template instantiation
template class FemGuiExport ViewProviderPythonFeatureT<ViewProviderFemConstraint>;
}
