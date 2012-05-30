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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <BRepAlgoAPI_BooleanOperation.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include "ViewProviderBoolean.h"
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/FeaturePartBoolean.h>
#include <Mod/Part/App/FeaturePartFuse.h>
#include <Mod/Part/App/FeaturePartCommon.h>

using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderBoolean,PartGui::ViewProviderPart)

ViewProviderBoolean::ViewProviderBoolean()
{
}

ViewProviderBoolean::~ViewProviderBoolean()
{
}

std::vector<App::DocumentObject*> ViewProviderBoolean::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(static_cast<Part::Boolean*>(getObject())->Base.getValue());
    temp.push_back(static_cast<Part::Boolean*>(getObject())->Tool.getValue());

    return temp;
}

QIcon ViewProviderBoolean::getIcon(void) const
{
    App::DocumentObject* obj = getObject();
    if (obj) {
        Base::Type type = obj->getTypeId();
        if (type == Base::Type::fromName("Part::Common"))
            return Gui::BitmapFactory().pixmap("Part_Common");
        else if (type == Base::Type::fromName("Part::Fuse"))
            return Gui::BitmapFactory().pixmap("Part_Fuse");
        else if (type == Base::Type::fromName("Part::Cut"))
            return Gui::BitmapFactory().pixmap("Part_Cut");
        else if (type == Base::Type::fromName("Part::Section"))
            return Gui::BitmapFactory().pixmap("Part_Section");
    }

    return ViewProviderPart::getIcon();
}

void findFaces(BRepAlgoAPI_BooleanOperation* mkBool,
               const TopTools_IndexedMapOfShape& M1,
               const TopTools_IndexedMapOfShape& M3,
               const std::vector<App::Color>& colBase,
               std::vector<App::Color>& colBool)
{
    for (int i=1; i<=M1.Extent(); i++) {
        bool modified=false, generated=false;
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(mkBool->Modified(M1(i))); it.More(); it.Next()) {
            modified = true;
            for (int j=1; j<=M3.Extent(); j++) {
                if (M3(j).IsPartner(it.Value())) {
                    colBool[j-1] = colBase[i-1];
                    break;
                }
            }
        }

        for (it.Initialize(mkBool->Generated(M1(i))); it.More(); it.Next()) {
            generated = true;
            for (int j=1; j<=M3.Extent(); j++) {
                if (M3(j).IsPartner(it.Value())) {
                    colBool[j-1] = colBase[i-1];
                    break;
                }
            }
        }

        if (!modified && !generated && !mkBool->IsDeleted(M1(i))) {
            for (int j=1; j<=M3.Extent(); j++) {
                if (M3(j).IsPartner(M1(i))) {
                    colBool[j-1] = colBase[i-1];
                    break;
                }
            }
        }
    }
}

void ViewProviderBoolean::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
        Part::Boolean* objBool = dynamic_cast<Part::Boolean*>(getObject());
        Part::Feature* objBase = dynamic_cast<Part::Feature*>(objBool->Base.getValue());
        Part::Feature* objTool = dynamic_cast<Part::Feature*>(objBool->Tool.getValue());

        BRepAlgoAPI_BooleanOperation* mkBool = objBool->getBooleanOperation();
        if (mkBool && objBase && objTool) {
            const TopoDS_Shape& baseShape = objBase->Shape.getValue();
            const TopoDS_Shape& toolShape = objTool->Shape.getValue();
            const TopoDS_Shape& boolShape = objBool->Shape.getValue();

            TopTools_IndexedMapOfShape M1, M2, M3;
            TopExp::MapShapes(baseShape, TopAbs_FACE, M1);
            TopExp::MapShapes(toolShape, TopAbs_FACE, M2);
            TopExp::MapShapes(boolShape, TopAbs_FACE, M3);
            Gui::ViewProvider* vpBase = Gui::Application::Instance->getViewProvider(objBase);
            Gui::ViewProvider* vpTool = Gui::Application::Instance->getViewProvider(objTool);
            std::vector<App::Color> colBase = static_cast<PartGui::ViewProviderPart*>(vpBase)->DiffuseColor.getValues();
            std::vector<App::Color> colTool = static_cast<PartGui::ViewProviderPart*>(vpTool)->DiffuseColor.getValues();
            std::vector<App::Color> colBool;
            colBool.resize(M3.Extent(), this->ShapeColor.getValue());
            bool applyColor=false;
            if (colBase.size() == M1.Extent()) {
                findFaces(mkBool, M1, M3, colBase, colBool);
                applyColor = true;
            }
            if (colTool.size() == M2.Extent()) {
                findFaces(mkBool, M2, M3, colTool, colBool);
                applyColor = true;
            }
            if (applyColor)
                this->DiffuseColor.setValues(colBool);
        }
    }
}

PROPERTY_SOURCE(PartGui::ViewProviderMultiFuse,PartGui::ViewProviderPart)

ViewProviderMultiFuse::ViewProviderMultiFuse()
{
}

ViewProviderMultiFuse::~ViewProviderMultiFuse()
{
}

std::vector<App::DocumentObject*> ViewProviderMultiFuse::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>(static_cast<Part::MultiFuse*>(getObject())->Shapes.getValues());
}

QIcon ViewProviderMultiFuse::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("Part_Fuse");
}


PROPERTY_SOURCE(PartGui::ViewProviderMultiCommon,PartGui::ViewProviderPart)

ViewProviderMultiCommon::ViewProviderMultiCommon()
{
}

ViewProviderMultiCommon::~ViewProviderMultiCommon()
{
}

std::vector<App::DocumentObject*> ViewProviderMultiCommon::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>(static_cast<Part::MultiCommon*>(getObject())->Shapes.getValues());
}

QIcon ViewProviderMultiCommon::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("Part_Common");
}
