/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <BRepAlgo.hxx>
# include <BRepFilletAPI_MakeChamfer.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <BRep_Tool.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
# include <Standard_Version.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureChamfer.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Chamfer, PartDesign::DressUp)

const char* ChamferTypeEnums[] = {"Equal distance", "Two distances", "Distance and Angle", NULL};
const App::PropertyQuantityConstraint::Constraints floatSize = {0.0,FLT_MAX,0.1};
const App::PropertyAngle::Constraints floatAngle = {0.0,180.0,1.0};

static App::DocumentObjectExecReturn *validateParameters(int chamferType, double size, double size2, double angle);

Chamfer::Chamfer()
{
    ADD_PROPERTY_TYPE(ChamferType, (0L), "Chamfer", App::Prop_None, "Type of chamfer");
    ChamferType.setEnums(ChamferTypeEnums);

    ADD_PROPERTY_TYPE(Size, (1.0), "Chamfer", App::Prop_None, "Size of chamfer");
    Size.setUnit(Base::Unit::Length);
    Size.setConstraints(&floatSize);

    ADD_PROPERTY_TYPE(Size2, (1.0), "Chamfer", App::Prop_None, "Second size of chamfer");
    Size2.setUnit(Base::Unit::Length);
    Size2.setConstraints(&floatSize);

    ADD_PROPERTY_TYPE(Angle, (45.0), "Chamfer", App::Prop_None, "Angle of chamfer");
    Angle.setUnit(Base::Unit::Angle);
    Angle.setConstraints(&floatAngle);

    ADD_PROPERTY_TYPE(FlipDirection, (false), "Chamfer", App::Prop_None, "Flip direction");

    updateProperties();
}

short Chamfer::mustExecute() const
{
    bool touched = false;

    auto chamferType = ChamferType.getValue();

    switch (chamferType) {
        case 0: // "Equal distance"
            touched = Size.isTouched() || ChamferType.isTouched();
            break;
        case 1: // "Two distances"
            touched = Size.isTouched() || ChamferType.isTouched() || Size2.isTouched();
            break;
        case 2: // "Distance and Angle"
            touched = Size.isTouched() || ChamferType.isTouched() || Angle.isTouched();
            break;
    }

    if (Placement.isTouched() || touched)
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Chamfer::execute(void)
{
    // NOTE: Normally the Base property and the BaseFeature property should point to the same object.
    // The only difference is that the Base property also stores the edges that are to be chamfered
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseShape();
    } catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    std::vector<std::string> SubNames = std::vector<std::string>(Base.getSubValues());
    getContiniusEdges(TopShape, SubNames);

    if (SubNames.size() == 0)
        return new App::DocumentObjectExecReturn("No edges specified");

    const int chamferType = ChamferType.getValue();
    const double size = Size.getValue();
    const double size2 = Size2.getValue();
    const double angle = Angle.getValue();
    const bool flipDirection = FlipDirection.getValue();

    auto res = validateParameters(chamferType, size, size2, angle);
    if (res != App::DocumentObject::StdReturn) {
        return res;
    }

    this->positionByBaseFeature();
    // create an untransformed copy of the basefeature shape
    Part::TopoShape baseShape(TopShape);
    baseShape.setTransform(Base::Matrix4D());
    try {
        BRepFilletAPI_MakeChamfer mkChamfer(baseShape.getShape());

        TopTools_IndexedMapOfShape mapOfEdges;
        TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
        TopExp::MapShapesAndAncestors(baseShape.getShape(), TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
        TopExp::MapShapes(baseShape.getShape(), TopAbs_EDGE, mapOfEdges);

        for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
            TopoDS_Edge edge = TopoDS::Edge(baseShape.getSubShape(it->c_str()));
            const TopoDS_Face& face = (chamferType != 0 && flipDirection) ?
                TopoDS::Face(mapEdgeFace.FindFromKey(edge).Last()) :
                TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
            switch (chamferType) {
                case 0: // Equal distance
                    mkChamfer.Add(size, size, edge, face);
                    break;
                case 1: // Two distances
                    mkChamfer.Add(size, size2, edge, face);
                    break;
                case 2: // Distance and angle
                    mkChamfer.AddDA(size, Base::toRadians(angle), edge, face);
                    break;
            }
        }

        mkChamfer.Build();
        if (!mkChamfer.IsDone())
            return new App::DocumentObjectExecReturn("Failed to create chamfer");

        TopoDS_Shape shape = mkChamfer.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        TopTools_ListOfShape aLarg;
        aLarg.Append(baseShape.getShape());
        if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
            ShapeFix_ShapeTolerance aSFT;
            aSFT.LimitTolerance(shape, Precision::Confusion(), Precision::Confusion(), TopAbs_SHAPE);
            Handle(ShapeFix_Shape) aSfs = new ShapeFix_Shape(shape);
            aSfs->Perform();
            shape = aSfs->Shape();
            if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
                return new App::DocumentObjectExecReturn("Resulting shape is invalid");
            }
        }
        int solidCount = countSolids(shape);
        if (solidCount > 1) {
            return new App::DocumentObjectExecReturn("Chamfer: Result has multiple solids. This is not supported at this time.");
        }
        shape = refineShapeIfActive(shape);
        this->Shape.setValue(getSolid(shape));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void Chamfer::Restore(Base::XMLReader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        const char* PropName = reader.getAttribute("name");
        const char* TypeName = reader.getAttribute("type");
        App::Property* prop = getPropertyByName(PropName);

        try {
            if (prop && strcmp(prop->getTypeId().getName(), TypeName) == 0) {
                prop->Restore(reader);
            }
            else if (prop && strcmp(TypeName,"App::PropertyFloatConstraint") == 0 &&
                     strcmp(prop->getTypeId().getName(), "App::PropertyQuantityConstraint") == 0) {
                App::PropertyFloatConstraint p;
                p.Restore(reader);
                static_cast<App::PropertyQuantityConstraint*>(prop)->setValue(p.getValue());
            }
        }
        catch (const Base::XMLParseException&) {
            throw; // re-throw
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        reader.readEndElement("Property");
    }
    reader.readEndElement("Properties");
}

void Chamfer::onChanged(const App::Property* prop)
{
    if (prop == &ChamferType) {
        updateProperties();
    }

    DressUp::onChanged(prop);
}

void Chamfer::updateProperties()
{
    auto chamferType = ChamferType.getValue();

    auto disableproperty = [](App::Property * prop, bool on) {
        prop->setStatus(App::Property::ReadOnly, on);
    };

    switch (chamferType) {
    case 0: // "Equal distance"
        disableproperty(&this->Angle, true);
        disableproperty(&this->Size2, true);
        break;
    case 1: // "Two distances"
        disableproperty(&this->Angle, true);
        disableproperty(&this->Size2, false);
        break;
    case 2: // "Distance and Angle"
        disableproperty(&this->Angle, false);
        disableproperty(&this->Size2, true);
        break;
    }
}

static App::DocumentObjectExecReturn *validateParameters(int chamferType, double size, double size2, double angle)
{
    // Size is common to all chamfer types.
    if (size <= 0) {
        return new App::DocumentObjectExecReturn("Size must be greater than zero");
    }

    switch (chamferType) {
        case 0: // Equal distance
            // Nothing to do.
            break;
        case 1: // Two distances
            if (size2 <= 0) {
                return new App::DocumentObjectExecReturn("Size2 must be greater than zero");
            }
            break;
        case 2: // Distance and angle
            if (angle <= 0 || angle >= 180.0) {
                return new App::DocumentObjectExecReturn("Angle must be greater than 0 and less than 180");
            }
            break;
    }

    return App::DocumentObject::StdReturn;
}


