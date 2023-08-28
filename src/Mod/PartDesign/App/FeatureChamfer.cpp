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
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
# include <Standard_Version.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureChamfer.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Chamfer, PartDesign::DressUp)

const char* ChamferTypeEnums[] = {"Equal distance", "Two distances", "Distance and Angle", nullptr};
const App::PropertyQuantityConstraint::Constraints Chamfer::floatSize = {0.0, FLT_MAX, 0.1};
const App::PropertyAngle::Constraints Chamfer::floatAngle = {0.0, 180.0, 1.0};

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
    ADD_PROPERTY_TYPE(UseAllEdges, (false), "Chamfer", App::Prop_None,
             "Chamfer all edges if true, else use only those edges in Base property.\n"
             "If true, then this overrides any edge changes made to the Base property or in the dialog.\n");

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

App::DocumentObjectExecReturn *Chamfer::execute()
{
    // NOTE: Normally the Base property and the BaseFeature property should point to the same object.
    // The only difference is that the Base property also stores the edges that are to be chamfered
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    std::vector<std::string> SubNames = std::vector<std::string>(Base.getSubValues());

    if (UseAllEdges.getValue()){
        SubNames.clear();
        std::string edgeTypeName = Part::TopoShape::shapeName(TopAbs_EDGE); //"Edge"
        int count = TopShape.countSubElements(edgeTypeName.c_str());
        for (int ii = 0; ii < count; ii++){
            std::ostringstream edgeName;
            edgeName << edgeTypeName << ii+1;
            SubNames.push_back(edgeName.str());
        }
    }

    std::vector<std::string> FaceNames;

    getContinuousEdges(TopShape, SubNames, FaceNames);

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

    //If no element is selected, then we use a copy of previous feature.
    if (SubNames.empty()) {
        this->Shape.setValue(TopShape);
        return App::DocumentObject::StdReturn;
    }

    // create an untransformed copy of the basefeature shape
    Part::TopoShape baseShape(TopShape);
    baseShape.setTransform(Base::Matrix4D());
    try {
        BRepFilletAPI_MakeChamfer mkChamfer(baseShape.getShape());

        TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
        TopExp::MapShapesAndAncestors(baseShape.getShape(), TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);

        for (const auto &itSN : SubNames) {
            TopoDS_Edge edge = TopoDS::Edge(baseShape.getSubShape(itSN.c_str()));

            const TopoDS_Shape& faceLast = mapEdgeFace.FindFromKey(edge).Last();
            const TopoDS_Shape& faceFirst = mapEdgeFace.FindFromKey(edge).First();

            // Set the face based on flipDirection for all edges by default. Note for chamferType==0 it does not matter which face is used.
            TopoDS_Face face = TopoDS::Face( flipDirection ? faceLast : faceFirst );

            // for chamfer types otherthan Equal (type = 0) check if one of the faces associated with the edge
            // is one of the originally selected faces. If so use the other face by default or the selected face if "flipDirection" is set
            if (chamferType != 0) {

                // for each selected face
                for (const auto &itFN : FaceNames) {
                    const TopoDS_Shape selFace = baseShape.getSubShape(itFN.c_str());

                    if ( faceLast.IsEqual(selFace) )
                        face = TopoDS::Face( flipDirection ? faceFirst : faceLast );

                    else if ( faceFirst.IsEqual(selFace) )
                        face = TopoDS::Face( flipDirection ? faceLast : faceFirst );
                }

            }

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
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Failed to create chamfer"));

        TopoDS_Shape shape = mkChamfer.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is null"));

        TopTools_ListOfShape aLarg;
        aLarg.Append(baseShape.getShape());
        if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
            ShapeFix_ShapeTolerance aSFT;
            aSFT.LimitTolerance(shape, Precision::Confusion(), Precision::Confusion(), TopAbs_SHAPE);
            Handle(ShapeFix_Shape) aSfs = new ShapeFix_Shape(shape);
            aSfs->Perform();
            shape = aSfs->Shape();
            if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is invalid"));
            }
        }
        int solidCount = countSolids(shape);
        if (solidCount > 1) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
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
    DressUp::Restore(reader);
}

void Chamfer::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop)
{
    if (prop && strcmp(TypeName,"App::PropertyFloatConstraint") == 0 &&
        strcmp(prop->getTypeId().getName(), "App::PropertyQuantityConstraint") == 0) {
        App::PropertyFloatConstraint p;
        p.Restore(reader);
        static_cast<App::PropertyQuantityConstraint*>(prop)->setValue(p.getValue());
    }
    else {
        DressUp::handleChangedPropertyType(reader, TypeName, prop);
    }
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
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Size must be greater than zero"));
    }

    switch (chamferType) {
        case 0: // Equal distance
            // Nothing to do.
            break;
        case 1: // Two distances
            if (size2 <= 0) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Size2 must be greater than zero"));
            }
            break;
        case 2: // Distance and angle
            if (angle <= 0 || angle >= 180.0) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Angle must be greater than 0 and less than 180"));
            }
            break;
    }

    return App::DocumentObject::StdReturn;
}


