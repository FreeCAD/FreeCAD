/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
 *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMultipleCopy.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTransform.h>
#include <QAction>
#include <QMenu>
#endif

#include "App/Application.h"
#include "Gui/Command.h"
#include "Gui/Control.h"
#include "Gui/Document.h"
#include "Gui/Selection/Selection.h"
#include "Mod/Fem/App/FemConstraint.h"

#include "ViewProviderFemConstraint.h"
#include "ViewProviderFemConstraintPy.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraint, Gui::ViewProviderGeometryObject)


ViewProviderFemConstraint::ViewProviderFemConstraint()
    : rotateSymbol(true)
    , pSymbol(nullptr)
    , pExtraSymbol(nullptr)
    , pExtraTrans(nullptr)
    , ivFile(nullptr)
{
    pShapeSep = new SoSeparator();
    pShapeSep->ref();
    pMultCopy = new SoMultipleCopy();
    pMultCopy->ref();
    pExtraTrans = new SoTransform();
    pExtraTrans->ref();

    ShapeAppearance.setDiffuseColor(1.0f, 0.0f, 0.2f);
    ShapeAppearance.setSpecularColor(0.0f, 0.0f, 0.0f);

    Gui::ViewProviderSuppressibleExtension::initExtension(this);
}

ViewProviderFemConstraint::~ViewProviderFemConstraint()
{
    pMultCopy->unref();
    pExtraTrans->unref();
    pShapeSep->unref();
}

void ViewProviderFemConstraint::attach(App::DocumentObject* pcObject)
{
    ViewProviderGeometryObject::attach(pcObject);

    SoPickStyle* ps = new SoPickStyle();
    ps->style = SoPickStyle::UNPICKABLE;

    SoSeparator* sep = new SoSeparator();
    SoShapeHints* hints = new SoShapeHints();
    hints->shapeType.setValue(SoShapeHints::UNKNOWN_SHAPE_TYPE);
    hints->vertexOrdering.setValue(SoShapeHints::COUNTERCLOCKWISE);
    sep->addChild(ps);
    sep->addChild(hints);
    sep->addChild(pcShapeMaterial);
    sep->addChild(pShapeSep);
    addDisplayMaskMode(sep, "Base");
}

std::string ViewProviderFemConstraint::resourceSymbolDir =
    App::Application::getResourceDir() + "Mod/Fem/Resources/symbols/";

void ViewProviderFemConstraint::loadSymbol(const char* fileName)
{
    ivFile = fileName;
    SoInput in;
    if (!in.openFile(ivFile)) {
        std::stringstream str;
        str << "Error opening symbol file " << fileName;
        throw Base::ImportError(str.str());
    }
    SoSeparator* nodes = SoDB::readAll(&in);
    if (!nodes) {
        std::stringstream str;
        str << "Error reading symbol file " << fileName;
        throw Base::ImportError(str.str());
    }

    nodes->ref();
    pSymbol = dynamic_cast<SoSeparator*>(nodes->getChild(0));
    pShapeSep->addChild(pMultCopy);
    if (pSymbol) {
        pMultCopy->addChild(pSymbol);
    }
    if (nodes->getNumChildren() == 2) {
        pExtraSymbol = dynamic_cast<SoSeparator*>(nodes->getChild(1));
        if (pExtraSymbol) {
            pShapeSep->addChild(pExtraTrans);
            pShapeSep->addChild(pExtraSymbol);
        }
    }
    pMultCopy->matrix.setNum(0);
    nodes->unref();
}

std::vector<std::string> ViewProviderFemConstraint::getDisplayModes() const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.emplace_back("Base");
    return StrList;
}

void ViewProviderFemConstraint::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0) {
        setDisplayMaskMode("Base");
    }
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<App::DocumentObject*> ViewProviderFemConstraint::claimChildren() const
{
    return {};
}

void ViewProviderFemConstraint::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit Analysis Feature"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    ViewProviderGeometryObject::setupContextMenu(menu,
                                                 receiver,
                                                 member);  // clazy:exclude=skipped-base-method
}

void ViewProviderFemConstraint::onChanged(const App::Property* prop)
{
    ViewProviderGeometryObject::onChanged(prop);
}

void ViewProviderFemConstraint::updateData(const App::Property* prop)
{
    auto pcConstraint = this->getObject<const Fem::Constraint>();

    if (prop == &pcConstraint->Points || prop == &pcConstraint->Normals
        || prop == &pcConstraint->Scale) {
        updateSymbol();
    }
    else {
        ViewProviderGeometryObject::updateData(prop);
    }
}

void ViewProviderFemConstraint::handleChangedPropertyName(Base::XMLReader& reader,
                                                          const char* typeName,
                                                          const char* propName)
{
    if (strcmp(propName, "FaceColor") == 0
        && Base::Type::fromName(typeName) == App::PropertyColor::getClassTypeId()) {
        App::PropertyColor color;
        color.Restore(reader);
        ShapeAppearance.setDiffuseColor(color.getValue());
    }
    else if (strcmp(propName, "ShapeMaterial") == 0
             && Base::Type::fromName(typeName) == App::PropertyMaterial::getClassTypeId()) {
        // nothing
    }
    else {
        ViewProviderGeometryObject::handleChangedPropertyName(reader, typeName, propName);
    }
}

void ViewProviderFemConstraint::setRotateSymbol(bool rotate)
{
    rotateSymbol = rotate;
    updateSymbol();
}

void ViewProviderFemConstraint::updateSymbol()
{
    auto obj = this->getObject<const Fem::Constraint>();
    if (!obj) {
        return;
    }

    const std::vector<Base::Vector3d>& points = obj->Points.getValue();
    const std::vector<Base::Vector3d>& normals = obj->Normals.getValue();
    if (points.size() != normals.size()) {
        return;
    }

    pMultCopy->matrix.setNum(points.size());
    SbMatrix* mat = pMultCopy->matrix.startEditing();

    for (size_t i = 0; i < points.size(); ++i) {
        transformSymbol(points[i], normals[i], mat[i]);
    }

    pMultCopy->matrix.finishEditing();

    transformExtraSymbol();
}

void ViewProviderFemConstraint::transformSymbol(const Base::Vector3d& point,
                                                const Base::Vector3d& normal,
                                                SbMatrix& mat) const
{
    auto obj = this->getObject<const Fem::Constraint>();
    SbVec3f axisY(0, 1, 0);
    float s = obj->getScaleFactor();
    SbVec3f scale(s, s, s);
    SbVec3f norm = rotateSymbol ? SbVec3f(normal.x, normal.y, normal.z) : axisY;
    SbRotation rot(axisY, norm);
    SbVec3f tra(static_cast<float>(point.x),
                static_cast<float>(point.y),
                static_cast<float>(point.z));
    mat.setTransform(tra, rot, scale);
}

void ViewProviderFemConstraint::transformExtraSymbol() const
{
    if (pExtraTrans) {
        auto obj = this->getObject<const Fem::Constraint>();
        float s = obj->getScaleFactor();
        SbMatrix mat;
        mat.setScale(s);
        pExtraTrans->setMatrix(mat);
    }
}


// OvG: Visibility automation show parts and hide meshes on activation of a constraint
std::string ViewProviderFemConstraint::gethideMeshShowPartStr(const std::string showConstr)
{
    return "for amesh in App.activeDocument().Objects:\n\
    if \""
        + showConstr + "\" == amesh.Name:\n\
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
    Gui::Command::doCommand(Gui::Command::Doc,
                            "%s",
                            ViewProviderFemConstraint::gethideMeshShowPartStr().c_str());
    return Gui::ViewProviderGeometryObject::setEdit(ModNum);
}

void ViewProviderFemConstraint::unsetEdit(int ModNum)
{
    // clear the selection (convenience)
    Gui::Selection().clearSelection();
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderGeometryObject::unsetEdit(ModNum);
    }
}

PyObject* ViewProviderFemConstraint::getPyObject()
{
    if (!pyViewObject) {
        pyViewObject = new ViewProviderFemConstraintPy(this);
    }
    pyViewObject->IncRef();
    return pyViewObject;
}


// Python feature -----------------------------------------------------------------------

namespace Gui
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(FemGui::ViewProviderFemConstraintPython, FemGui::ViewProviderFemConstraint)
/// @endcond

// explicit template instantiation
template class FemGuiExport ViewProviderFeaturePythonT<ViewProviderFemConstraint>;
}  // namespace Gui
