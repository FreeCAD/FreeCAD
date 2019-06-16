/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)                                 *
 *                                           (vv.titov@gmail.com) 2015     *
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
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
# include <gp_Pnt.hxx>
# include <gp_Dir.hxx>
# include <gp_Elips.hxx>
# include <gp_Parab.hxx>
# include <gp_Hypr.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <Geom_Plane.hxx>
# include <Geom2d_Curve.hxx>
# include <Geom2dAPI_InterCurveCurve.hxx>
# include <Geom2dAPI_ProjectPointOnCurve.hxx>
# include <GeomAPI.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepIntCurveSurface_Inter.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <ShapeExtend_Explorer.hxx>
# include <GProp_GProps.hxx>
# include <GProp_PGProps.hxx>
# include <GProp_PrincipalProps.hxx>
# include <BRepGProp.hxx>
# include <GeomLib_IsPlanarSurface.hxx>
# include <BRepLProp_SLProps.hxx>
# include <GeomAPI_ProjectPointOnCurve.hxx>
#endif

#include "Attacher.h"
#include "AttachExtension.h"
#include <Base/Console.h>
#include <App/OriginFeature.h>
#include <App/Application.h>
#include <App/Document.h>

using namespace Part;
using namespace Attacher;

//These strings are for mode list enum property.
const char* AttachEngine::eMapModeStrings[]= {
    "Deactivated",
    "Translate",
    "ObjectXY",
    "ObjectXZ",
    "ObjectYZ",
    "FlatFace",
    "TangentPlane",
    "NormalToEdge",
    "FrenetNB",
    "FrenetTN",
    "FrenetTB",
    "Concentric",
    "SectionOfRevolution",
    "ThreePointsPlane",
    "ThreePointsNormal",
    "Folding",

    "ObjectX",
    "ObjectY",
    "ObjectZ",
    "AxisOfCurvature",
    "Directrix1",
    "Directrix2",
    "Asymptote1",
    "Asymptote2",
    "Tangent",
    "Normal",
    "Binormal",
    "TangentU",
    "TangentV",
    "TwoPointLine",
    "IntersectionLine",
    "ProximityLine",

    "ObjectOrigin",
    "Focus1",
    "Focus2",
    "OnEdge",
    "CenterOfCurvature",
    "CenterOfMass",
    "IntersectionPoint",
    "Vertex",
    "ProximityPoint1",
    "ProximityPoint2",

    "AxisOfInertia1",
    "AxisOfInertia2",
    "AxisOfInertia3",

    "InertialCS",

    "FaceNormal",

    "OZX",
    "OZY",
    "OXY",
    "OXZ",
    "OYZ",
    "OYX",

    NULL};

//this list must be in sync with eRefType enum.
//These strings are used only by Py interface of Attacher. Strings for use in Gui are in Mod/Part/Gui/AttacherTexts.cpp
const char* AttachEngine::eRefTypeStrings[]= {
    "Any",
    "Vertex",
    "Edge",
    "Face",

    "Line",
    "Curve",
    "Circle",
    "Conic",
    "Ellipse",
    "Parabola",
    "Hyperbola",

    "Plane",
    "Sphere",
    "Revolve",
    "Cylinder",
    "Torus",
    "Cone",

    "Object",
    "Solid",
    "Wire",
    NULL
};





TYPESYSTEM_SOURCE_ABSTRACT(Attacher::AttachEngine, Base::BaseClass)

AttachEngine::AttachEngine()
 : mapMode(mmDeactivated), mapReverse(false), attachParameter(0.0),
   surfU(0.0), surfV(0.0)
{
}

void AttachEngine::setUp(const App::PropertyLinkSubList &references,
                         eMapMode mapMode, bool mapReverse,
                         double attachParameter,
                         double surfU, double surfV,
                         const Base::Placement &attachmentOffset)
{
    this->references.Paste(references);
    this->mapMode = mapMode;
    this->mapReverse = mapReverse;
    this->attachParameter = attachParameter;
    this->surfU = surfU;
    this->surfV = surfV;
    this->attachmentOffset = attachmentOffset;
}

void AttachEngine::setUp(const AttachEngine &another)
{
    setUp(another.references,
          another.mapMode,
          another.mapReverse,
          another.attachParameter,
          another.surfU,
          another.surfV,
          another.attachmentOffset);
}

Base::Placement AttachEngine::placementFactory(const gp_Dir &ZAxis,
                                        gp_Vec XAxis,
                                        gp_Pnt Origin,
                                        gp_Pnt refOrg,
                                        bool useRefOrg_Line,
                                        bool useRefOrg_Plane,
                                        bool makeYVertical,
                                        bool makeLegacyFlatFaceOrientation,
                                        Base::Placement* placeOfRef) const
{
    if(useRefOrg_Line){
        //move Origin to projection of refOrg onto ZAxis
        gp_Vec refOrgV = gp_Vec(refOrg.XYZ());
        gp_Vec OriginV = gp_Vec(Origin.XYZ());
        gp_Vec ZAxisV = gp_Vec(ZAxis);
        Origin = gp_Pnt((
         OriginV + ZAxisV*ZAxisV.Dot(refOrgV-OriginV)
          ).XYZ());
    }
    if(useRefOrg_Plane){
        //move Origin to projection of refOrg onto plane (ZAxis, Origin)
        gp_Vec refOrgV = gp_Vec(refOrg.XYZ());
        gp_Vec OriginV = gp_Vec(Origin.XYZ());
        gp_Vec ZAxisV = gp_Vec(ZAxis);
        Origin = gp_Pnt((
         refOrgV + ZAxisV*ZAxisV.Dot(OriginV-refOrgV)
          ).XYZ());
    }

    if (XAxis.Magnitude() < Precision::Confusion())
        makeYVertical = true;

    gp_Ax3 ax3;//OCC representation of the final placement
    if (!makeYVertical) {
        ax3 = gp_Ax3(Origin, ZAxis, XAxis);
    } else if (!makeLegacyFlatFaceOrientation) {
        //align Y along Z, if possible
        gp_Vec YAxis(0.0,0.0,1.0);
        XAxis = YAxis.Crossed(gp_Vec(ZAxis));
        if (XAxis.Magnitude() < Precision::Confusion()){
            //ZAxis is along true ZAxis
            XAxis = (gp_Vec(1,0,0)*ZAxis.Z()).Normalized();
        }
        ax3 = gp_Ax3(Origin, ZAxis, XAxis);
    } else if (makeLegacyFlatFaceOrientation) {
        //find out, to which axis of support Normal is closest to.
        //The result will be written into pos variable (0..2 = X..Z)
        if (!placeOfRef)
            throw AttachEngineException("AttachEngine::placementFactory: for Legacy mode, placement of the reference must be supplied. Got null instead!");
        Base::Placement &Place = *placeOfRef;
        Base::Vector3d dX,dY,dZ;//internal axes of support object, as they are in global space
        Place.getRotation().multVec(Base::Vector3d(1,0,0),dX);
        Place.getRotation().multVec(Base::Vector3d(0,1,0),dY);
        Place.getRotation().multVec(Base::Vector3d(0,0,1),dZ);
        gp_Dir dirX(dX.x, dX.y, dX.z);
        gp_Dir dirY(dY.x, dY.y, dY.z);
        gp_Dir dirZ(dZ.x, dZ.y, dZ.z);
        double cosNX = ZAxis.Dot(dirX);
        double cosNY = ZAxis.Dot(dirY);
        double cosNZ = ZAxis.Dot(dirZ);
        std::vector<double> cosXYZ;
        cosXYZ.push_back(fabs(cosNX));
        cosXYZ.push_back(fabs(cosNY));
        cosXYZ.push_back(fabs(cosNZ));

        int pos = std::max_element(cosXYZ.begin(), cosXYZ.end()) - cosXYZ.begin();

        // +X/-X
        if (pos == 0) {
            if (cosNX > 0)
                ax3 = gp_Ax3(Origin, ZAxis, dirY);
            else
                ax3 = gp_Ax3(Origin, ZAxis, -dirY);
        }
        // +Y/-Y
        else if (pos == 1) {
            if (cosNY > 0)
                ax3 = gp_Ax3(Origin, ZAxis, -dirX);
            else
                ax3 = gp_Ax3(Origin, ZAxis, dirX);
        }
        // +Z/-Z
        else {
            ax3 = gp_Ax3(Origin, ZAxis, dirX);
        }
    }

    if(this->mapReverse){
        ax3.ZReverse();
        ax3.XReverse();
    }

    //convert ax3 into Base::Placement
    gp_Trsf Trf;
    Trf.SetTransformation(ax3);
    Trf.Invert();
    Trf.SetScaleFactor(Standard_Real(1.0));

    Base::Matrix4D mtrx;
    TopoShape::convertToMatrix(Trf,mtrx);

    return Base::Placement(mtrx);

}

void AttachEngine::suggestMapModes(SuggestResult &result) const
{
    std::vector<eMapMode> &mlist = result.allApplicableModes;
    mlist.clear();
    mlist.reserve(mmDummy_NumberOfModes);

    std::set<eRefType> &hints = result.nextRefTypeHint;
    hints.clear();

    std::map<eMapMode,refTypeStringList> &mlist_reachable = result.reachableModes;
    mlist_reachable.clear();

    result.message = SuggestResult::srLinkBroken;
    result.bestFitMode = mmDeactivated;


    std::vector<App::GeoFeature*> parts;
    std::vector<const TopoDS_Shape*> shapes;
    std::vector<TopoDS_Shape> shapeStorage;
    std::vector<eRefType> typeStr;
    try{
        readLinks(this->references, parts, shapes, shapeStorage, typeStr);
    } catch (Base::Exception &err) {
        result.references_Types = typeStr;
        result.message = SuggestResult::srLinkBroken;
        result.error.Exception::operator = (err);
        return;
    }

    result.references_Types = typeStr;

    //search valid modes.
    int bestMatchScore = -1;
    result.message = SuggestResult::srNoModesFit;
    for (std::size_t iMode = 0; iMode < this->modeRefTypes.size(); ++iMode) {
        if (! this->modeEnabled[iMode])
            continue;
        const refTypeStringList &listStrings = modeRefTypes[iMode];
        for (std::size_t iStr = 0; iStr < listStrings.size(); ++iStr) {
            int score = 1; //-1 = topo incompatible, 0 = topo compatible, geom incompatible; 1+ = compatible (the higher - the more specific is the mode for the support)
            const refTypeString &str = listStrings[iStr];
            for (std::size_t iChr = 0; iChr < str.size() && iChr < typeStr.size(); ++iChr) {
                int match = AttachEngine::isShapeOfType(typeStr[iChr], str[iChr]);
                switch(match){
                case -1:
                    score = -1;
                break;
                case 0:
                    score = 0;
                break;
                case 1:
                    //keep score
                break;
                default: //2 and above
                    if (score > 0)
                        score += match;
                break;
                }
            }

            if (score > 0  &&  str.size() > typeStr.size()){
                //mode does not fit, but adding more references will make this mode fit.
                hints.insert(str[typeStr.size()]);

                //build string of references to be added to fit this mode
                refTypeString extraRefs;
                extraRefs.resize(str.size() - typeStr.size());
                for (std::size_t iChr = typeStr.size(); iChr < str.size(); iChr++) {
                    extraRefs[iChr - typeStr.size()] = str[iChr];
                }

                //add reachable mode
                auto it_r = mlist_reachable.find(eMapMode(iMode));
                if (it_r == mlist_reachable.end()){
                    it_r = mlist_reachable.insert(std::pair<eMapMode,refTypeStringList>(eMapMode(iMode),refTypeStringList())).first;
                }
                refTypeStringList &list = it_r->second;
                list.push_back(extraRefs);
            }

            //size check is last, because we needed to collect hints
            if (str.size() != typeStr.size())
                score = -1;

            if (score > -1){//still output a best match, even if it is not completely compatible
                if (score > bestMatchScore){
                    bestMatchScore = score;
                    result.bestFitMode = eMapMode(iMode);
                    result.message = score > 0 ? SuggestResult::srOK : SuggestResult::srIncompatibleGeometry;
                }
            }
            if (score > 0){
                if(mlist.size() == 0)
                    mlist.push_back(eMapMode(iMode));
                else if (mlist.back() != eMapMode(iMode))
                    mlist.push_back(eMapMode(iMode));
            }
        }
    }

}

void AttachEngine::EnableAllSupportedModes()
{
    this->modeEnabled.resize(mmDummy_NumberOfModes,false);
    assert(modeRefTypes.size() > 0);
    for (std::size_t i = 0; i < this->modeEnabled.size(); i++) {
        modeEnabled[i] = modeRefTypes[i].size() > 0;
    }
}

eRefType AttachEngine::getShapeType(const TopoDS_Shape& sh)
{
    if(sh.IsNull())
        return rtAnything;

    switch (sh.ShapeType()){
    case TopAbs_SHAPE:
        return rtAnything; //note: there's no rtPart detection here - not enough data!
    break;
    case TopAbs_SOLID:
        return rtSolid;
    break;
    case TopAbs_COMPOUND:{
        const TopoDS_Compound &cmpd = TopoDS::Compound(sh);
        TopoDS_Iterator it (cmpd, Standard_False, Standard_False);//don't mess with placements, to hopefully increase speed
        if (! it.More()) return rtAnything;//empty compound
        const TopoDS_Shape &sh1 = it.Value();
        it.Next();
        if (it.More()){
            //more than one object, a true compound
            return rtAnything;
        } else {
            //just one object, let's take a look inside
            return getShapeType(sh1);
        }
    }break;
    case TopAbs_COMPSOLID:
    case TopAbs_SHELL:
        return rtAnything;
    break;
    case TopAbs_FACE:{
        const TopoDS_Face &f = TopoDS::Face(sh);
        BRepAdaptor_Surface surf(f, /*restriction=*/Standard_False);
        switch(surf.GetType()) {
        case GeomAbs_Plane:
            return rtFlatFace;
        case GeomAbs_Cylinder:
            return rtCylindricalFace;
        case GeomAbs_Cone:
            return rtConicalFace;
        case GeomAbs_Sphere:
            return rtSphericalFace;
        case GeomAbs_Torus:
            return rtToroidalFace;
        case GeomAbs_BezierSurface:
            break;
        case GeomAbs_BSplineSurface:
            break;
        case GeomAbs_SurfaceOfRevolution:
            return rtSurfaceRev;
        case GeomAbs_SurfaceOfExtrusion:
            break;
        case GeomAbs_OffsetSurface:
            break;
        case GeomAbs_OtherSurface:
            break;
        }
        return rtFace;
    }break;
    case TopAbs_EDGE:{
        const TopoDS_Edge &e = TopoDS::Edge(sh);
        BRepAdaptor_Curve crv(e);
        switch (crv.GetType()){
        case GeomAbs_Line:
            return rtLine;
        case GeomAbs_Circle:
            return rtCircle;
        case GeomAbs_Ellipse:
            return rtEllipse;
        case GeomAbs_Hyperbola:
            return rtHyperbola;
        case GeomAbs_Parabola:
            return rtParabola;
        case GeomAbs_BezierCurve:
        case GeomAbs_BSplineCurve:
        case GeomAbs_OtherCurve:
#if OCC_VERSION_HEX >= 0x070000
        case GeomAbs_OffsetCurve:
#endif
            return rtCurve;
        }
    }break;
    case TopAbs_WIRE:
        return rtWire;
    case TopAbs_VERTEX:
        return rtVertex;
    default:
        throw AttachEngineException("AttachEngine::getShapeType: unexpected TopoDS_Shape::ShapeType");
    }//switch shapetype
    return rtAnything;//shouldn't happen, it's here to shut up compiler warning
}

eRefType AttachEngine::getShapeType(const App::DocumentObject *obj, const std::string &subshape)
{
    App::PropertyLinkSubList tmpLink;
    //const_cast is worth here, to keep obj argument const. We are not going to write anything to obj through this temporary link.
    tmpLink.setValue(const_cast<App::DocumentObject*>(obj), subshape.c_str());

    std::vector<App::GeoFeature*> parts;
    std::vector<const TopoDS_Shape*> shapes;
    std::vector<TopoDS_Shape> copiedShapeStorage;
    std::vector<eRefType> types;
    readLinks(tmpLink, parts, shapes, copiedShapeStorage, types);

    assert(types.size() == 1);
    return types[0];
}

eRefType AttachEngine::downgradeType(eRefType type)
{
    //get rid of hasplacement flags, to simplify the rest
    type = eRefType(type & (rtFlagHasPlacement - 1));
    //FIXME: reintroduce the flag when returning a value.

    switch(type){
    case rtVertex:
    case rtEdge:
    case rtFace:
        return rtAnything;
    case rtAnything:
        return rtAnything;
    case rtLine:
    case rtCurve:
        return rtEdge;
    case rtConic:
    case rtCircle:
        return rtCurve;
    case rtEllipse:
    case rtParabola:
    case rtHyperbola:
        return rtConic;
    case rtFlatFace:
    case rtSphericalFace:
    case rtSurfaceRev:
        return rtFace;
    case rtCylindricalFace:
    case rtToroidalFace:
    case rtConicalFace:
        return rtSurfaceRev;
    case rtSolid:
    case rtWire:
        return rtPart;
    case rtPart:
        return rtAnything;
    default:
        throw AttachEngineException("AttachEngine::downgradeType: unknown type");
    }
}

int AttachEngine::getTypeRank(eRefType type)
{
    //get rid of hasplacement flags, to simplify the rest
    type = eRefType(type & (rtFlagHasPlacement - 1));

    int rank = 0;
    while (type != rtAnything) {
        type = downgradeType(type);
        rank++;
        assert(rank<8);//downgrading never yields rtAnything, something's wrong with downgrader.
    }
    return rank;
}

int AttachEngine::isShapeOfType(eRefType shapeType, eRefType requirement)
{
    //first up, check for hasplacement flag
    if (requirement & rtFlagHasPlacement) {
        if(! (shapeType & rtFlagHasPlacement))
            return -1;
    }

    //get rid of hasplacement flags, to simplify the rest
    shapeType = eRefType(shapeType & (rtFlagHasPlacement - 1));
    requirement = eRefType(requirement & (rtFlagHasPlacement - 1));

    if (requirement == rtAnything)
        return 1;

    int reqRank = getTypeRank(requirement);

    //test for valid match
    eRefType shDeg = shapeType;
    while(shDeg != rtAnything){
        if (shDeg == requirement)
            return reqRank;
        shDeg = downgradeType(shDeg);
    }

    //test for slightly invalid match (e.g. requirement==line, shapeType == curve)
    requirement = downgradeType(requirement);
    if (requirement != rtAnything) {
        eRefType shDeg = shapeType;
        while(shDeg != rtAnything){
            if (shDeg == requirement)
                return 0;
            shDeg = downgradeType(shDeg);
        }
    }

    //complete mismatch!
    return -1;
}

std::string AttachEngine::getModeName(eMapMode mmode)
{
    if(mmode < 0 || mmode >= mmDummy_NumberOfModes)
        throw AttachEngineException("AttachEngine::getModeName: Attachment Mode index is out of range");
    return std::string(AttachEngine::eMapModeStrings[mmode]);
}

eMapMode AttachEngine::getModeByName(const std::string &modeName)
{
    for (int mmode = 0   ;   mmode < mmDummy_NumberOfModes   ;   mmode++){
        if (strcmp(eMapModeStrings[mmode],modeName.c_str())==0) {
            return eMapMode(mmode);
        }
    }
    std::stringstream errMsg;
    errMsg << "AttachEngine::getModeByName: mode with this name doesn't exist: " << modeName;
    throw AttachEngineException(errMsg.str());
}

std::string AttachEngine::getRefTypeName(eRefType shapeType)
{
    eRefType flagless = eRefType(shapeType & 0xFF);
    if(flagless < 0 || flagless >= rtDummy_numberOfShapeTypes)
        throw AttachEngineException("eRefType value is out of range");
    std::string result = std::string(eRefTypeStrings[flagless]);
    if (shapeType & rtFlagHasPlacement){
        result.append("|Placement");
    }
    return result;
}

eRefType AttachEngine::getRefTypeByName(const std::string& typeName)
{
    std::string flagless;
    std::string flags;
    size_t seppos = typeName.find('|');
    flagless = typeName.substr(0, seppos);
    if(seppos != std::string::npos ){
        flags = typeName.substr(seppos+1);
    }
    for(int irt = 0   ;   irt < rtDummy_numberOfShapeTypes   ;   irt++){
        if(strcmp(flagless.c_str(),eRefTypeStrings[irt]) == 0){
            if(strcmp("Placement",flags.c_str()) == 0){
                return eRefType(irt | rtFlagHasPlacement);
            } else if (flags.length() == 0){
                return eRefType(irt);
            } else {
                std::stringstream errmsg;
                errmsg << "RefType flag not recognized: " << flags;
                throw AttachEngineException(errmsg.str());
            }
        }
    }
    std::stringstream errmsg;
    errmsg << "RefType not recognized: " << typeName;
    throw AttachEngineException(errmsg.str());
}

GProp_GProps AttachEngine::getInertialPropsOfShape(const std::vector<const TopoDS_Shape*> &shapes)
{
    //explode compounds
    TopTools_HSequenceOfShape totalSeq;
    for (const TopoDS_Shape* pSh: shapes){
        ShapeExtend_Explorer xp;
        totalSeq.Append( xp.SeqFromCompound(*pSh, /*recursive=*/true));
    }
    if (totalSeq.Length() == 0)
        throw AttachEngineException("AttachEngine::getInertialPropsOfShape: no geometry provided");
    const TopoDS_Shape &sh0 = totalSeq.Value(1);
    switch (sh0.ShapeType()){
    case TopAbs_VERTEX:{
        GProp_PGProps gpr;
        for (int i = 0   ;   i < totalSeq.Length()   ;   i++){
            const TopoDS_Shape &sh = totalSeq.Value(i+1);
            if (sh.ShapeType() != TopAbs_VERTEX)
                throw AttachEngineException("AttachEngine::getInertialPropsOfShape: provided shapes are incompatible (not only vertices)");
            gpr.AddPoint(BRep_Tool::Pnt(TopoDS::Vertex(sh)));
        }
        return gpr;
    } break;
    case TopAbs_EDGE:
    case TopAbs_WIRE:{
        GProp_GProps gpr_acc;
        GProp_GProps gpr;
        for (int i = 0   ;   i < totalSeq.Length()   ;   i++){
            const TopoDS_Shape &sh = totalSeq.Value(i+1);
            if (sh.ShapeType() != TopAbs_EDGE && sh.ShapeType() != TopAbs_WIRE)
                throw AttachEngineException("AttachEngine::getInertialPropsOfShape: provided shapes are incompatible (not only edges/wires)");
            if (sh.Infinite())
                throw AttachEngineException("AttachEngine::getInertialPropsOfShape: infinite shape provided");
            BRepGProp::LinearProperties(sh,gpr);
            gpr_acc.Add(gpr);
        }
        return gpr_acc;
    } break;
    case TopAbs_FACE:
    case TopAbs_SHELL:{
        GProp_GProps gpr_acc;
        GProp_GProps gpr;
        for (int i = 0   ;   i < totalSeq.Length()   ;   i++){
            const TopoDS_Shape &sh = totalSeq.Value(i+1);
            if (sh.ShapeType() != TopAbs_FACE && sh.ShapeType() != TopAbs_SHELL)
                throw AttachEngineException("AttachEngine::getInertialPropsOfShape: provided shapes are incompatible (not only faces/shells)");
            if (sh.Infinite())
                throw AttachEngineException("AttachEngine::getInertialPropsOfShape: infinite shape provided");
            BRepGProp::SurfaceProperties(sh,gpr);
            gpr_acc.Add(gpr);
        }
        return gpr_acc;
    } break;
    case TopAbs_SOLID:
    case TopAbs_COMPSOLID:{
        GProp_GProps gpr_acc;
        GProp_GProps gpr;
        for (int i = 0   ;   i < totalSeq.Length()   ;   i++){
            const TopoDS_Shape &sh = totalSeq.Value(i+1);
            if (sh.ShapeType() != TopAbs_SOLID && sh.ShapeType() != TopAbs_COMPSOLID)
                throw AttachEngineException("AttachEngine::getInertialPropsOfShape: provided shapes are incompatible (not only solids/compsolids)");
            if (sh.Infinite())
                throw AttachEngineException("AttachEngine::getInertialPropsOfShape: infinite shape provided");
            BRepGProp::VolumeProperties(sh,gpr);
            gpr_acc.Add(gpr);
        }
        return gpr_acc;
    } break;
    default:
        throw AttachEngineException("AttachEngine::getInertialPropsOfShape: unexpected shape type");
    }
}

/*!
 * \brief AttachEngine3D::readLinks
 * \param parts
 * \param shapes
 * \param storage is a buffer storing what some of the pointers in shapes point to. It is needed, since
 * subshapes are copied in the process (but copying a whole shape of an object can potentially be slow).
 */
void AttachEngine::readLinks(const App::PropertyLinkSubList &references,
                             std::vector<App::GeoFeature*> &geofs,
                             std::vector<const TopoDS_Shape*> &shapes,
                             std::vector<TopoDS_Shape> &storage,
                             std::vector<eRefType> &types)
{
    verifyReferencesAreSafe(references);
    const std::vector<App::DocumentObject*> &objs = references.getValues();
    const std::vector<std::string> &sub = references.getSubValues();
    geofs.resize(objs.size());
    storage.reserve(objs.size());
    shapes.resize(objs.size());
    types.resize(objs.size());
    for (std::size_t i = 0; i < objs.size(); i++) {
        if (!objs[i]->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            throw AttachEngineException("AttachEngine3D: link points to something that is not App::GeoFeature");
        }
        App::GeoFeature* geof = static_cast<App::GeoFeature*>(objs[i]);
        geofs[i] = geof;
        const Part::TopoShape* shape;
        if (geof->isDerivedFrom(Part::Feature::getClassTypeId())){
            shape = &(static_cast<Part::Feature*>(geof)->Shape.getShape());
            if (shape->isNull()){
                throw AttachEngineException("AttachEngine3D: Part has null shape");
            }
            if (sub[i].length()>0){
                try{
                    storage.push_back(shape->getSubShape(sub[i].c_str()));
                } catch (Standard_Failure&){
                    throw AttachEngineException("AttachEngine3D: subshape not found");
                }
                if(storage[storage.size()-1].IsNull())
                    throw AttachEngineException("AttachEngine3D: null subshape");
                shapes[i] = &(storage[storage.size()-1]);
            } else {
                shapes[i] = &(shape->getShape());
            }
        } else if (  geof->isDerivedFrom(App::Plane::getClassTypeId())  ){
            //obtain Z axis and origin of placement
            Base::Vector3d norm;
            geof->Placement.getValue().getRotation().multVec(Base::Vector3d(0.0,0.0,1.0),norm);
            Base::Vector3d org;
            geof->Placement.getValue().multVec(Base::Vector3d(),org);
            //make shape - an local-XY plane infinite face
            gp_Pln pl = gp_Pln(gp_Pnt(org.x, org.y, org.z), gp_Dir(norm.x, norm.y, norm.z));
            TopoDS_Shape myShape = BRepBuilderAPI_MakeFace(pl).Shape();
            myShape.Infinite(true);
            storage.push_back(myShape);
            shapes[i] = &(storage[storage.size()-1]);
        } else if (  geof->isDerivedFrom(App::Line::getClassTypeId())  ){
            //obtain X axis and origin of placement
            //note an inconsistency: App::Line is along local X, PartDesign::DatumLine is along local Z.
            Base::Vector3d dir;
            geof->Placement.getValue().getRotation().multVec(Base::Vector3d(1.0,0.0,0.0),dir);
            Base::Vector3d org;
            geof->Placement.getValue().multVec(Base::Vector3d(),org);
            //make shape - an infinite line along local X axis
            gp_Lin l = gp_Lin(gp_Pnt(org.x, org.y, org.z), gp_Dir(dir.x, dir.y, dir.z));
            TopoDS_Shape myShape = BRepBuilderAPI_MakeEdge(l).Shape();
            myShape.Infinite(true);
            storage.push_back(myShape);
            shapes[i] = &(storage[storage.size()-1]);
        } else {
            Base::Console().Warning("Attacher: linked object %s is unexpected, assuming it has no shape.\n",geof->getNameInDocument());
            storage.push_back(TopoDS_Shape());
            shapes[i] = &(storage[storage.size()-1]);
        }

        //FIXME: unpack single-child compounds here? Compounds are not used so far, so it should be considered later, when the need arises.
        types[i] = getShapeType(*(shapes[i]));
        if (sub[i].length() == 0)
            types[i] = eRefType(types[i] | rtFlagHasPlacement);
    }
}

void AttachEngine::throwWrongMode(eMapMode mmode)
{
    std::stringstream errmsg;
    if (mmode >= 0 && mmode<mmDummy_NumberOfModes) {
        if (AttachEngine::eMapModeStrings[mmode]) {
            errmsg << "Attachment mode " << AttachEngine::eMapModeStrings[mmode] << " is not implemented." ;
        } else {
            errmsg << "Attachment mode " << int(mmode) << " is undefined." ;
        }
    } else {
        errmsg << "Attachment mode index (" << int(mmode) << ") is out of range." ;
    }
    throw Base::ValueError(errmsg.str().c_str());
}

void AttachEngine::verifyReferencesAreSafe(const App::PropertyLinkSubList &references)
{
    const std::vector<App::DocumentObject*> links =  references.getValues();
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for(App::DocumentObject* lnk : links){
        bool found = false;
        for(App::Document* doc : docs){
            if(doc->isIn(lnk)){
                found = true;
            }
        }
        if (!found){
            throw AttachEngineException("AttachEngine: verifyReferencesAreSafe: references point to deleted object.");
        }
    }
}


//=================================================================================

TYPESYSTEM_SOURCE(Attacher::AttachEngine3D, Attacher::AttachEngine)

AttachEngine3D::AttachEngine3D()
{
    //fill type lists for modes
    modeRefTypes.resize(mmDummy_NumberOfModes);
    refTypeString s;
    refTypeStringList ss;

    modeRefTypes[mmTranslate].push_back(cat(rtVertex));

    ss.clear();
    ss.push_back(cat(eRefType(rtAnything | rtFlagHasPlacement)));
    ss.push_back(cat(rtConic));
    modeRefTypes[mmObjectXY] = ss;
    modeRefTypes[mmObjectXZ] = ss;
    modeRefTypes[mmObjectYZ] = ss;

    modeRefTypes[mmInertialCS].push_back(cat(rtAnything));
    modeRefTypes[mmInertialCS].push_back(cat(rtAnything,rtAnything));
    modeRefTypes[mmInertialCS].push_back(cat(rtAnything,rtAnything,rtAnything));
    modeRefTypes[mmInertialCS].push_back(cat(rtAnything,rtAnything,rtAnything,rtAnything));

    modeRefTypes[mmFlatFace].push_back(cat(rtFlatFace));

    modeRefTypes[mmTangentPlane].push_back(cat(rtFace, rtVertex));
    modeRefTypes[mmTangentPlane].push_back(cat(rtVertex, rtFace));

    //---------Edge-driven

    s=cat(rtEdge);
    modeRefTypes[mmNormalToPath].push_back(s);

    s = cat(rtCurve);
    modeRefTypes[mmFrenetNB].push_back(s);
    modeRefTypes[mmFrenetTN].push_back(s);
    modeRefTypes[mmFrenetTB].push_back(s);
    modeRefTypes[mmRevolutionSection].push_back(s);
    modeRefTypes[mmConcentric].push_back(s);
    s = cat(rtCircle);
    modeRefTypes[mmRevolutionSection].push_back(s);//for this mode to get best score on circles
    modeRefTypes[mmConcentric].push_back(s);

    //-----------Edge-driven at vertex

    s=cat(rtEdge, rtVertex);
    modeRefTypes[mmNormalToPath].push_back(s);
    s=cat(rtVertex, rtEdge);
    modeRefTypes[mmNormalToPath].push_back(s);

    s=cat(rtCurve, rtVertex);
    modeRefTypes[mmFrenetNB].push_back(s);
    modeRefTypes[mmFrenetTN].push_back(s);
    modeRefTypes[mmFrenetTB].push_back(s);
    modeRefTypes[mmRevolutionSection].push_back(s);
    modeRefTypes[mmConcentric].push_back(s);
    s = cat(rtCircle, rtVertex);
    modeRefTypes[mmRevolutionSection].push_back(s);//for this mode to get best score on circles
    modeRefTypes[mmConcentric].push_back(s);

    s=cat(rtVertex, rtCurve);
    modeRefTypes[mmFrenetNB].push_back(s);
    modeRefTypes[mmFrenetTN].push_back(s);
    modeRefTypes[mmFrenetTB].push_back(s);
    modeRefTypes[mmRevolutionSection].push_back(s);
    modeRefTypes[mmConcentric].push_back(s);
    s = cat(rtVertex, rtCircle);
    modeRefTypes[mmRevolutionSection].push_back(s);//for this mode to get best score on circles
    modeRefTypes[mmConcentric].push_back(s);

    //------------ThreePoints

    s = cat(rtVertex, rtVertex, rtVertex);
    modeRefTypes[mmThreePointsPlane].push_back(s);
    modeRefTypes[mmThreePointsNormal].push_back(s);

    s = cat(rtLine, rtVertex);
    modeRefTypes[mmThreePointsPlane].push_back(s);
    modeRefTypes[mmThreePointsNormal].push_back(s);

    s = cat(rtVertex, rtLine);
    modeRefTypes[mmThreePointsPlane].push_back(s);
    modeRefTypes[mmThreePointsNormal].push_back(s);

    s = cat(rtLine, rtLine);
    modeRefTypes[mmThreePointsPlane].push_back(s);
    modeRefTypes[mmThreePointsNormal].push_back(s);

    //------------origin-axis-axis modes
    for (int mmode = mmOZX; mmode <= mmOYX; ++mmode){
        modeRefTypes[mmode].push_back(cat(rtVertex, rtVertex, rtVertex));
        modeRefTypes[mmode].push_back(cat(rtVertex, rtVertex, rtLine));
        modeRefTypes[mmode].push_back(cat(rtVertex, rtLine, rtVertex));
        modeRefTypes[mmode].push_back(cat(rtVertex, rtLine, rtLine));
        modeRefTypes[mmode].push_back(cat(rtVertex, rtVertex));
        modeRefTypes[mmode].push_back(cat(rtVertex, rtLine));
    }


    modeRefTypes[mmFolding].push_back(cat(rtLine, rtLine, rtLine, rtLine));

    this->EnableAllSupportedModes();
}

AttachEngine3D* AttachEngine3D::copy() const
{
    AttachEngine3D* p = new AttachEngine3D;
    p->setUp(*this);
    return p;
}

Base::Placement AttachEngine3D::calculateAttachedPlacement(Base::Placement origPlacement) const
{
    const eMapMode mmode = this->mapMode;
    if (mmode == mmDeactivated)
        throw ExceptionCancel();//to be handled in positionBySupport, to not do anything if disabled
    std::vector<App::GeoFeature*> parts;
    std::vector<const TopoDS_Shape*> shapes;
    std::vector<TopoDS_Shape> copiedShapeStorage;
    std::vector<eRefType> types;
    readLinks(this->references, parts, shapes, copiedShapeStorage, types);

    if (parts.size() == 0)
        throw ExceptionCancel();


    //common stuff for all map modes
    gp_Pnt refOrg (0.0,0.0,0.0);//origin of linked object
    Base::Placement Place = parts[0]->Placement.getValue();
    refOrg = gp_Pnt(Place.getPosition().x, Place.getPosition().y, Place.getPosition().z);

    //variables to derive the actual placement.
    //They are to be set, depending on the mode:
     //to the sketch
    gp_Dir SketchNormal;//points at the user
    gp_Vec SketchXAxis; //if left zero, a guess will be made
    gp_Pnt SketchBasePoint; //where to put the origin of the sketch


    switch (mmode) {
    case mmDeactivated:
        //should have been filtered out already!
        break;
    case mmTranslate:{
        if (shapes.size() < 1)
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: no subobjects specified (need one vertex).");
        const TopoDS_Shape &sh = *shapes[0];
        if (sh.IsNull())
            throw Base::ValueError("Null face in AttachEngine3D::calculateAttachedPlacement()!");
        if (sh.ShapeType() != TopAbs_VERTEX)
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: no subobjects specified (need one vertex).");
        gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(sh));
        Base::Placement plm = Base::Placement();
        plm.setPosition(Base::Vector3d(p.X(), p.Y(), p.Z()));
        plm.setPosition(plm.getPosition() + this->attachmentOffset.getPosition());
        plm.setRotation(origPlacement.getRotation());
        return plm;
    } break;
    case mmObjectXY:
    case mmObjectXZ:
    case mmObjectYZ:{
        //DeepSOIC: could have been done much more efficiently, but I'm lazy...
        gp_Dir dirX, dirY, dirZ;
        if (types[0] & rtFlagHasPlacement) {
            Base::Vector3d dX,dY,dZ;//internal axes of support object, as they are in global space
            Place.getRotation().multVec(Base::Vector3d(1,0,0),dX);
            Place.getRotation().multVec(Base::Vector3d(0,1,0),dY);
            Place.getRotation().multVec(Base::Vector3d(0,0,1),dZ);
            dirX = gp_Dir(dX.x, dX.y, dX.z);
            dirY = gp_Dir(dY.x, dY.y, dY.z);
            dirZ = gp_Dir(dZ.x, dZ.y, dZ.z);
            SketchBasePoint = gp_Pnt(Place.getPosition().x,Place.getPosition().y,Place.getPosition().z);
        } else if (isShapeOfType(types[0],rtConic) > 0) {
            const TopoDS_Edge &e = TopoDS::Edge(*shapes[0]);
            BRepAdaptor_Curve adapt(e);
            gp_Ax3 pos;
            switch(adapt.GetType()){
            case GeomAbs_Ellipse:{
                gp_Elips cc = adapt.Ellipse();
                pos = gp_Ax3(cc.Position());
            }break;
            case GeomAbs_Hyperbola:{
                gp_Hypr cc = adapt.Hyperbola();
                pos = gp_Ax3(cc.Position());
            }break;
            case GeomAbs_Parabola:{
                gp_Parab cc = adapt.Parabola();
                pos = gp_Ax3(cc.Position());
            }break;
            default:
                assert(0);//conics should have been filtered out by testing shape type in the above if.
            }
            dirX = pos.XDirection();
            dirY = pos.YDirection();
            dirZ = pos.Axis().Direction();
            SketchBasePoint = pos.Location();
        } else {
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: need either a conic section edge, or a whole object for ObjectXY-like modes.");
        }

        switch (mmode){
        case mmObjectXY:
            SketchNormal = dirZ;
            SketchXAxis = gp_Vec(dirX);
            break;
        case mmObjectXZ:
            SketchNormal = dirY.Reversed();
            SketchXAxis = gp_Vec(dirX);
            break;
        case mmObjectYZ:
            SketchNormal = dirX;
            SketchXAxis = gp_Vec(dirY);
            break;
        default:
            break;
        }

    } break;
    case mmInertialCS:{
        GProp_GProps gpr = AttachEngine::getInertialPropsOfShape(shapes);
        GProp_PrincipalProps pr = gpr.PrincipalProperties();
        if (pr.HasSymmetryPoint())
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement:InertialCS: inertia tensor is trivial, principal axes are undefined.");
        if (pr.HasSymmetryAxis()){
            Base::Console().Warning("AttachEngine3D::calculateAttachedPlacement:InertialCS: inertia tensor has axis of symmetry. Second and third axes of inertia are undefined.\n");
            //find defined axis, and use it as Z axis
            //situation: we have two moments that are almost equal, and one
            //that is substantially different. The one that is different
            //corresponds to a defined axis. We'll identify the different one by
            //comparing differences.
            Standard_Real I1, I2, I3;
            pr.Moments(I1,I2,I3);
            Standard_Real d12, d23, d31;
            d12 = fabs(I1-I2);
            d23 = fabs(I2-I3);
            d31 = fabs(I3-I1);
            if(d12 < d23 && d12 < d31){
                SketchNormal = pr.ThirdAxisOfInertia();
            } else if (d23 < d31 && d23 < d12){
                SketchNormal = pr.FirstAxisOfInertia();
            } else {
                SketchNormal = pr.SecondAxisOfInertia();
            }
        } else {
            SketchNormal = pr.FirstAxisOfInertia();
            SketchXAxis = pr.SecondAxisOfInertia();
        }
        SketchBasePoint = gpr.CentreOfMass();
    }break;
    case mmFlatFace:{
        if (shapes.size() < 1)
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: no subobjects specified (needed one planar face).");

        const TopoDS_Face &face = TopoDS::Face(*(shapes[0]));
        if (face.IsNull())
            throw Base::ValueError("Null face in AttachEngine3D::calculateAttachedPlacement()!");

        gp_Pln plane;
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() == GeomAbs_Plane) {
            plane = adapt.Plane();
        }
        else {
            TopLoc_Location loc;
            Handle(Geom_Surface) surf = BRep_Tool::Surface(face, loc);
            GeomLib_IsPlanarSurface check(surf);
            if (check.IsPlanar())
                plane = check.Plan();
            else
                throw Base::ValueError("No planar face in AttachEngine3D::calculateAttachedPlacement()!");
        }

        bool Reverse = false;
        if (face.Orientation() == TopAbs_REVERSED)
            Reverse = true;

        Standard_Boolean ok = plane.Direct();
        if (!ok) {
            // toggle if plane has a left-handed coordinate system
            plane.UReverse();
            Reverse = !Reverse;
        }
        gp_Ax1 Normal = plane.Axis();
        if (Reverse)
            Normal.Reverse();
        SketchNormal = Normal.Direction();

        Handle (Geom_Plane) gPlane = new Geom_Plane(plane);
        GeomAPI_ProjectPointOnSurf projector(refOrg,gPlane);
        SketchBasePoint = projector.NearestPoint();

    } break;
    case mmTangentPlane: {
        if (shapes.size() < 2)
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: not enough subshapes (need one false and one vertex).");

        bool bThruVertex = false;
        if (shapes[0]->ShapeType() == TopAbs_VERTEX) {
            std::swap(shapes[0],shapes[1]);
            bThruVertex = true;
        }

        const TopoDS_Face &face = TopoDS::Face(*(shapes[0]));
        if (face.IsNull())
            throw Base::ValueError("Null face in AttachEngine3D::calculateAttachedPlacement()!");

        const TopoDS_Vertex &vertex = TopoDS::Vertex(*(shapes[1]));
        if (vertex.IsNull())
            throw Base::ValueError("Null vertex in AttachEngine3D::calculateAttachedPlacement()!");

        BRepAdaptor_Surface surf (face);
        Handle (Geom_Surface) hSurf = BRep_Tool::Surface(face);
        gp_Pnt p = BRep_Tool::Pnt(vertex);

        GeomAPI_ProjectPointOnSurf projector(p, hSurf);
        double u, v;
        if (projector.NbPoints()==0)
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: projecting point onto surface failed.");
        projector.LowerDistanceParameters(u, v);

        BRepLProp_SLProps prop(surf,u,v,1, Precision::Confusion());
        SketchNormal = prop.Normal();

        gp_Dir dirX;
        prop.TangentU(dirX); //if normal is defined, this should be defined too
        SketchXAxis = gp_Vec(dirX).Reversed();//yields upside-down sketches less often.

        if (face.Orientation() == TopAbs_REVERSED) {
            SketchNormal.Reverse();
            SketchXAxis.Reverse();
        }
        if (bThruVertex) {
            SketchBasePoint = p;
        } else {
            SketchBasePoint = projector.NearestPoint();
        }
    } break;
    case mmNormalToPath:
    case mmFrenetNB:
    case mmFrenetTN:
    case mmFrenetTB:
    case mmRevolutionSection:
    case mmConcentric: {//all alignments to point on curve
        if (shapes.size() < 1)
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: no subshapes specified (need one edge, and an optional vertex).");

        bool bThruVertex = false;
        if (shapes[0]->ShapeType() == TopAbs_VERTEX && shapes.size()>=2) {
            std::swap(shapes[0],shapes[1]);
            bThruVertex = true;
        }

        const TopoDS_Edge &path = TopoDS::Edge(*(shapes[0]));
        if (path.IsNull())
            throw Base::ValueError("Null path in AttachEngine3D::calculateAttachedPlacement()!");

        BRepAdaptor_Curve adapt(path);

        double u = 0.0;
        double u1 = adapt.FirstParameter();
        double u2 = adapt.LastParameter();
        if(Precision::IsInfinite(u1) || Precision::IsInfinite(u2)){
            //prevent attachment to infinities in case of infinite shape.
            //example of an infinite shape is a datum line.
            u1 = 0.0;
            u2 = 1.0;
        }

        //if a point is specified, use the point as a point of mapping, otherwise use parameter value from properties
        gp_Pnt p_in;
        if (shapes.size() >= 2) {
            TopoDS_Vertex vertex = TopoDS::Vertex(*(shapes[1]));
            if (vertex.IsNull())
                throw Base::ValueError("Null vertex in AttachEngine3D::calculateAttachedPlacement()!");
            p_in = BRep_Tool::Pnt(vertex);

            Handle (Geom_Curve) hCurve = BRep_Tool::Curve(path, u1, u2);

            GeomAPI_ProjectPointOnCurve projector (p_in, hCurve);
            u = projector.LowerDistanceParameter();
        } else {
            u = u1  +  this->attachParameter * (u2 - u1);
        }
        gp_Pnt p;  gp_Vec d; //point and derivative
        adapt.D1(u,p,d);

        if (d.Magnitude()<Precision::Confusion())
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: path curve derivative is below 1e-7, too low, can't align");

        //Set origin. Note that it will be overridden later for mmConcentric and mmRevolutionSection
        if (bThruVertex) {
            SketchBasePoint = p_in;
        } else {
            SketchBasePoint = p;
        }

        if (mmode == mmRevolutionSection
                || mmode == mmConcentric
                || mmode == mmFrenetNB
                || mmode == mmFrenetTN
                || mmode == mmFrenetTB){
            gp_Vec dd;//second derivative
            try{
                adapt.D2(u,p,d,dd);
            } catch (Standard_Failure &e){
                //ignore. This is brobably due to insufficient continuity.
                dd = gp_Vec(0., 0., 0.);
                Base::Console().Warning("AttachEngine3D::calculateAttachedPlacement: can't calculate second derivative of curve. OCC error: %s\n", e.GetMessageString());
            }

            gp_Vec T,N,B;//Frenet?Serret axes: tangent, normal, binormal
            T = d.Normalized();
            N = dd.Subtracted(T.Multiplied(dd.Dot(T)));//take away the portion of dd that is along tangent
            if (N.Magnitude() > Precision::SquareConfusion()) {
                N.Normalize();
                B = T.Crossed(N);
            } else {
                Base::Console().Warning("AttachEngine3D::calculateAttachedPlacement: path curve second derivative is below 1e-14, can't align x axis.\n");
                N = gp_Vec(0.,0.,0.);
                B = gp_Vec(0.,0.,0.);//redundant, just for consistency
            }


            switch (mmode){
            case mmFrenetNB:
            case mmRevolutionSection:
                SketchNormal = T.Reversed();//to avoid sketches upside-down for regular curves like circles
                SketchXAxis = N.Reversed();
                break;
            case mmFrenetTN:
            case mmConcentric:
                if (N.Magnitude() == 0.0)
                    throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: Frenet-Serret normal is undefined. Can't align to TN plane.");
                SketchNormal = B;
                SketchXAxis = T;
                break;
            case mmFrenetTB:
                if (N.Magnitude() == 0.0)
                    throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: Frenet-Serret normal is undefined. Can't align to TB plane.");
                SketchNormal = N.Reversed();//it is more convenient to sketch on something looking at it so it is convex.
                SketchXAxis = T;
                break;
            default:
                assert(0);//mode forgotten?
            }
            if (mmode == mmRevolutionSection || mmode == mmConcentric) {
                //make sketch origin be at center of osculating circle
                if (N.Magnitude() == 0.0)
                    throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: path has infinite radius of curvature at the point. Can't align for revolving.");
                double curvature = dd.Dot(N) / pow(d.Magnitude(), 2);
                gp_Vec pv (p.XYZ());
                pv.Add(N.Multiplied(1/curvature));//shift the point along curvature by radius of curvature
                SketchBasePoint = gp_Pnt(pv.XYZ());
                //it would have been cool to have the curve attachment point available inside sketch... Leave for future.
            }
        } else if (mmode == mmNormalToPath){//mmNormalToPath
            //align sketch origin to the origin of support
            SketchNormal = gp_Dir(d.Reversed());//sketch normal looks at user. It is natural to have the curve directed away from user, so reversed.
        }

    } break;
    case mmThreePointsPlane:
    case mmThreePointsNormal: {

        std::vector<gp_Pnt> points;

        for (std::size_t i = 0; i < shapes.size(); i++) {
            const TopoDS_Shape &sh = *shapes[i];
            if (sh.IsNull())
                throw Base::ValueError("Null shape in AttachEngine3D::calculateAttachedPlacement()!");
            if (sh.ShapeType() == TopAbs_VERTEX){
                const TopoDS_Vertex &v = TopoDS::Vertex(sh);
                points.push_back(BRep_Tool::Pnt(v));
            } else if (sh.ShapeType() == TopAbs_EDGE) {
                const TopoDS_Edge &e = TopoDS::Edge(sh);
                BRepAdaptor_Curve crv(e);
                double u1 = crv.FirstParameter();
                double u2 = crv.LastParameter();
                if ( Precision::IsInfinite(u1)
                     || Precision::IsInfinite(u2) ){
                    u1 = 0.0;
                    u2 = 1.0;
                }
                points.push_back(crv.Value(u1));
                points.push_back(crv.Value(u2));
            }
            if (points.size() >= 3)
                break;
        }

        if(points.size()<3)
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: less than 3 points are specified, cannot derive the plane.");

        gp_Pnt p0 = points[0];
        gp_Pnt p1 = points[1];
        gp_Pnt p2 = points[2];

        gp_Vec vec01 (p0,p1);
        gp_Vec vec02 (p0,p2);
        if (vec01.Magnitude() < Precision::Confusion() || vec02.Magnitude() < Precision::Confusion())
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: some of 3 points are coincident. Can't make a plane");
        vec01.Normalize();
        vec02.Normalize();

        gp_Vec norm ;
        if (mmode == mmThreePointsPlane) {
            norm = vec01.Crossed(vec02);
            if (norm.Magnitude() < Precision::Confusion())
                throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: points are collinear. Can't make a plane");
            //SketchBasePoint = (p0+p1+p2)/3.0
            SketchBasePoint = gp_Pnt(gp_Vec(p0.XYZ()).Added(p1.XYZ()).Added(p2.XYZ()).Multiplied(1.0/3.0).XYZ());
        } else if (mmode == mmThreePointsNormal) {
            norm = vec02.Subtracted(vec01.Multiplied(vec02.Dot(vec01))).Reversed();//norm = vec02 forced perpendicular to vec01.
            if (norm.Magnitude() < Precision::Confusion())
                throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: points are collinear. Can't make a plane");
            //SketchBasePoint = (p0+p1)/2.0

            Handle (Geom_Plane) gPlane = new Geom_Plane(p0, gp_Dir(norm));
            GeomAPI_ProjectPointOnSurf projector(p2,gPlane);
            SketchBasePoint = projector.NearestPoint();

        }

        norm.Normalize();
        SketchNormal = gp_Dir(norm);

    } break;
    case mmFolding: {

        // Expected selection: four edges in order: edgeA, fold axis A,
        // fold axis B, edgeB. The sketch will be placed angled so as to join
        // edgeA to edgeB by folding the sheet along axes. All edges are
        // expected to be in one plane.

        if (shapes.size()<4)
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: not enough shapes (need 4 lines: edgeA, axisA, axisB, edgeB).");

        //extract the four lines
        const TopoDS_Edge* edges[4];
        BRepAdaptor_Curve adapts[4];
        gp_Lin lines[4];
        for(int i=0  ;  i<4  ;  i++){
            edges[i] = &TopoDS::Edge(*(shapes[i]));
            if (edges[i]->IsNull())
                throw Base::ValueError("Null edge in AttachEngine3D::calculateAttachedPlacement()!");

            adapts[i] = BRepAdaptor_Curve(*(edges[i]));
            if (adapts[i].GetType() != GeomAbs_Line)
                throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: Folding - non-straight edge.");
            lines[i] = adapts[i].Line();
        }

        //figure out the common starting point (variable p)
        gp_Pnt p, p1, p2, p3, p4;
        double signs[4] = {0,0,0,0};//flags whether to reverse line directions, for all directions to point away from the common vertex
        p1 = adapts[0].Value(adapts[0].FirstParameter());
        p2 = adapts[0].Value(adapts[0].LastParameter());
        p3 = adapts[1].Value(adapts[1].FirstParameter());
        p4 = adapts[1].Value(adapts[1].LastParameter());
        p = p1;
        if (p1.Distance(p3) < Precision::Confusion()){
            p = p3;
            signs[0] = +1.0;
            signs[1] = +1.0;
        } else if (p1.Distance(p4) < Precision::Confusion()){
            p = p4;
            signs[0] = +1.0;
            signs[1] = -1.0;
        } else if (p2.Distance(p3) < Precision::Confusion()){
            p = p3;
            signs[0] = -1.0;
            signs[1] = +1.0;
        } else if (p2.Distance(p4) < Precision::Confusion()){
            p = p4;
            signs[0] = -1.0;
            signs[1] = -1.0;
        } else {
            throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: Folding - edges to not share a vertex.");
        }
        for (int i = 2  ;  i<4  ;  i++){
            p1 = adapts[i].Value(adapts[i].FirstParameter());
            p2 = adapts[i].Value(adapts[i].LastParameter());
            if (p.Distance(p1) < Precision::Confusion())
                signs[i] = +1.0;
            else if (p.Distance(p2) < Precision::Confusion())
                signs[i] = -1.0;
            else
                throw Base::ValueError("AttachEngine3D::calculateAttachedPlacement: Folding - edges to not share a vertex.");
        }

        gp_Vec dirs[4];
        for(int i=0  ;  i<4  ;  i++){
            assert(fabs(signs[i]) == 1.0);
            dirs[i] = gp_Vec(lines[i].Direction()).Multiplied(signs[i]);
        }

        double ang = this->calculateFoldAngle(
                    dirs[1],
                    dirs[2],
                    dirs[0],
                    dirs[3]
                );

        gp_Vec norm = dirs[1].Crossed(dirs[2]);
        //rotation direction: when angle is positive, rotation is CCW when observing the vector so
        //that the axis is pointing at you. Hence angle is negated here.
        norm.Rotate(gp_Ax1(gp_Pnt(),gp_Dir(dirs[1])),-ang);
        SketchNormal = norm.Reversed();

        SketchXAxis = dirs[1];

        SketchBasePoint = p;

    } break;
    case mmOZX:
    case mmOZY:
    case mmOXY:
    case mmOXZ:
    case mmOYZ:
    case mmOYX: {
        const char orderStrings[6][4] = {
            "ZXY",
            "ZYX",
            "XYZ",
            "XZY",
            "YZX",
            "YXZ",
        };
        const char* orderString = orderStrings[mmode - mmOZX];

        enum dirIndex {
            X,
            Y,
            Z
        };
        int order[3];
        for(int i = 0; i < 3; ++i){
            order[i] = orderString[i] - 'X';
        }

        if (shapes.size() < 2)
            THROWM(Base::ValueError, "AttachEngine3D::calculateAttachedPlacement: not enough shapes linked (at least two are required).");

        gp_Vec dirs[3];

        //read out origin
        if (shapes[0]->IsNull())
            THROWM(Base::TypeError, "AttachEngine3D::calculateAttachedPlacement: null shape!")
        if (shapes[0]->ShapeType() != TopAbs_VERTEX)
            THROWM(Base::TypeError, "AttachEngine3D::calculateAttachedPlacement: first reference must be a vertex, it's not")
        SketchBasePoint = BRep_Tool::Pnt(TopoDS::Vertex(*(shapes[0])));

        //read out axes directions
        for(size_t i = 1; i < 3 && i < shapes.size(); ++i){
            if (shapes[i]->IsNull())
                THROWM(Base::TypeError, "AttachEngine3D::calculateAttachedPlacement: null shape!")
            if (shapes[i]->ShapeType() == TopAbs_VERTEX){
                gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(*(shapes[i])));
                dirs[order[i-1]] = gp_Vec(SketchBasePoint, p);
            } else if (shapes[i]->ShapeType() == TopAbs_EDGE){
                const TopoDS_Edge &e = TopoDS::Edge(*(shapes[i]));
                BRepAdaptor_Curve crv(e);
                double u1 = crv.FirstParameter();
                double u2 = crv.LastParameter();
                if ( Precision::IsInfinite(u1)
                     || Precision::IsInfinite(u2) ){
                    u1 = 0.0;
                    u2 = 1.0;
                }
                gp_Pnt p1 = crv.Value(u1);
                gp_Pnt p2 = crv.Value(u2);
                dirs[order[i-1]] = gp_Vec(p1,p2);
            }
        }

        //make the placement
        Base::Rotation rot =
                Base::Rotation::makeRotationByAxes(
                    Base::Vector3d(dirs[0].X(), dirs[0].Y(), dirs[0].Z()),
                    Base::Vector3d(dirs[1].X(), dirs[1].Y(), dirs[1].Z()),
                    Base::Vector3d(dirs[2].X(), dirs[2].Y(), dirs[2].Z()),
                    orderString
                );
        if(this->mapReverse){
            rot = rot * Base::Rotation(Base::Vector3d(0,1,0),D_PI);
        }

        Base::Placement plm =
                Base::Placement(Base::Vector3d(SketchBasePoint.X(), SketchBasePoint.Y(), SketchBasePoint.Z()), rot);
        plm *= this->attachmentOffset;
        return plm;
    } break;
    default:
        throwWrongMode(mmode);
    }//switch (MapMode)

    //----------calculate placement, based on point and vector

    Base::Placement plm =
            this->placementFactory(SketchNormal, SketchXAxis, SketchBasePoint, gp_Pnt(),
                                   /*useRefOrg_Line = */ false,
                                   /*useRefOrg_Plane = */ false,
                                   /*makeYVertical = */ false,
                                   /*makeLegacyFlatFaceOrientation = */ mmode == mmFlatFace,
                                   &Place);
    plm *= this->attachmentOffset;
    return plm;
}

double AttachEngine3D::calculateFoldAngle(gp_Vec axA, gp_Vec axB, gp_Vec edA, gp_Vec edB) const
{
    //DeepSOIC: this hardcore math can probably be replaced with a couple of
    //clever OCC calls... See forum thread "Sketch mapping enhancement" for a
    //picture on how this math was derived.
    //http://forum.freecadweb.org/viewtopic.php?f=8&t=10511&sid=007946a934530ff2a6c9259fb32624ec&start=40#p87584
    axA.Normalize();
    axB.Normalize();
    edA.Normalize();
    edB.Normalize();
    gp_Vec norm = axA.Crossed(axB);
    if (norm.Magnitude() < Precision::Confusion())
        throw AttachEngineException("calculateFoldAngle: Folding axes are parallel, folding angle cannot be computed.");
    norm.Normalize();
    double a = edA.Dot(axA);
    double ra = edA.Crossed(axA).Magnitude();
    if (fabs(ra) < Precision::Confusion())
        throw AttachEngineException("calculateFoldAngle: axisA and edgeA are parallel, folding can't be computed.");
    double b = edB.Dot(axB);
    double costheta = axB.Dot(axA);
    double sintheta = axA.Crossed(axB).Dot(norm);
    double singama = -costheta;
    double cosgama = sintheta;
    double k = b*cosgama;
    double l = a + b*singama;
    double xa = k + l*singama/cosgama;
    double cos_unfold = -xa/ra;
    if (fabs(cos_unfold)>0.999)
        throw AttachEngineException("calculateFoldAngle: cosine of folding angle is too close to or above 1.");
    return acos(cos_unfold);
}


//=================================================================================

TYPESYSTEM_SOURCE(Attacher::AttachEnginePlane, Attacher::AttachEngine);

AttachEnginePlane::AttachEnginePlane()
{
    //re-used 3d modes: all of Attacher3d
    AttachEngine3D attacher3D;
    this->modeRefTypes = attacher3D.modeRefTypes;
    this->EnableAllSupportedModes();
}

AttachEnginePlane *AttachEnginePlane::copy() const
{
    AttachEnginePlane* p = new AttachEnginePlane;
    p->setUp(*this);
    return p;
}

Base::Placement AttachEnginePlane::calculateAttachedPlacement(Base::Placement origPlacement) const
{
    //re-use Attacher3d
    Base::Placement plm;
    AttachEngine3D attacher3D;
    attacher3D.setUp(*this);
    plm = attacher3D.calculateAttachedPlacement(origPlacement);
    return plm;
}

//=================================================================================

TYPESYSTEM_SOURCE(Attacher::AttachEngineLine, Attacher::AttachEngine);

AttachEngineLine::AttachEngineLine()
{
    //fill type lists for modes
    modeRefTypes.resize(mmDummy_NumberOfModes);
    refTypeString s;

    //re-used 3d modes
    AttachEngine3D attacher3D;
    modeRefTypes[mm1AxisX] = attacher3D.modeRefTypes[mmObjectYZ];
    modeRefTypes[mm1AxisY] = attacher3D.modeRefTypes[mmObjectXZ];
    modeRefTypes[mm1AxisZ] = attacher3D.modeRefTypes[mmObjectXY];
    modeRefTypes[mm1AxisCurv] = attacher3D.modeRefTypes[mmRevolutionSection];
    modeRefTypes[mm1Binormal] = attacher3D.modeRefTypes[mmFrenetTN];
    modeRefTypes[mm1Normal] = attacher3D.modeRefTypes[mmFrenetTB];
    modeRefTypes[mm1Tangent] = attacher3D.modeRefTypes[mmNormalToPath];

    modeRefTypes[mm1TwoPoints].push_back(cat(rtVertex,rtVertex));
    modeRefTypes[mm1TwoPoints].push_back(cat(rtLine));

    modeRefTypes[mm1Asymptote1].push_back(cat(rtHyperbola));
    modeRefTypes[mm1Asymptote2].push_back(cat(rtHyperbola));

    modeRefTypes[mm1Directrix1].push_back(cat(rtConic));

    modeRefTypes[mm1Directrix2].push_back(cat(rtEllipse));
    modeRefTypes[mm1Directrix2].push_back(cat(rtHyperbola));

    modeRefTypes[mm1Proximity].push_back(cat(rtAnything, rtAnything));

    modeRefTypes[mm1AxisInertia1].push_back(cat(rtAnything));
    modeRefTypes[mm1AxisInertia1].push_back(cat(rtAnything,rtAnything));
    modeRefTypes[mm1AxisInertia1].push_back(cat(rtAnything,rtAnything,rtAnything));
    modeRefTypes[mm1AxisInertia1].push_back(cat(rtAnything,rtAnything,rtAnything,rtAnything));
    modeRefTypes[mm1AxisInertia2] = modeRefTypes[mm1AxisInertia1];
    modeRefTypes[mm1AxisInertia3] = modeRefTypes[mm1AxisInertia1];

    modeRefTypes[mm1FaceNormal] = attacher3D.modeRefTypes[mmTangentPlane];



    this->EnableAllSupportedModes();
}

AttachEngineLine *AttachEngineLine::copy() const
{
    AttachEngineLine* p = new AttachEngineLine;
    p->setUp(*this);
    return p;
}

Base::Placement AttachEngineLine::calculateAttachedPlacement(Base::Placement origPlacement) const
{
    eMapMode mmode = this->mapMode;

    //modes that are mirrors of attacher3D:
    bool bReUsed = true;
    Base::Placement presuperPlacement;
    switch(mmode){
    case mmDeactivated:
        throw ExceptionCancel();//to be handled in positionBySupport, to not do anything if disabled
    case mm1AxisX:
        mmode = mmObjectYZ;
        break;
    case mm1AxisY:
        mmode = mmObjectXZ;
        break;
    case mm1AxisZ:
        mmode = mmObjectXY;
        break;
    case mm1AxisCurv:
        mmode = mmRevolutionSection;
        //the line should go along Y, not Z
        presuperPlacement.setRotation(
                    Base::Rotation(  Base::Vector3d(0.0,0.0,1.0),
                                     Base::Vector3d(0.0,1.0,0.0)  )
                    );
        break;
    case mm1Binormal:
        mmode = mmFrenetTN;
        break;
    case mm1Normal:
        mmode = mmFrenetTB;
        break;
    case mm1Tangent:
        mmode = mmNormalToPath;
        break;
    case mm1FaceNormal:
        mmode = mmTangentPlane;
        break;
    default:
        bReUsed = false;
        break;
    }

    Base::Placement plm;
    if (!bReUsed){
        std::vector<App::GeoFeature*> parts;
        std::vector<const TopoDS_Shape*> shapes;
        std::vector<TopoDS_Shape> copiedShapeStorage;
        std::vector<eRefType> types;
        readLinks(this->references, parts, shapes, copiedShapeStorage, types);

        if (parts.size() == 0)
            throw ExceptionCancel();


        //common stuff for all map modes
        gp_Pnt refOrg (0.0,0.0,0.0);
        Base::Placement Place = parts[0]->Placement.getValue();
        refOrg = gp_Pnt(Place.getPosition().x, Place.getPosition().y, Place.getPosition().z);

        //variables to derive the actual placement.
        //They are to be set, depending on the mode:
        gp_Dir LineDir;
        gp_Pnt LineBasePoint; //the point the line goes through


        switch (mmode) {
        case mm1AxisInertia1:
        case mm1AxisInertia2:
        case mm1AxisInertia3:{
            GProp_GProps gpr = AttachEngine::getInertialPropsOfShape(shapes);
            LineBasePoint = gpr.CentreOfMass();
            GProp_PrincipalProps pr = gpr.PrincipalProperties();
            if (pr.HasSymmetryPoint())
                throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement:AxisOfInertia: inertia tensor is trivial, principal axes are undefined.");

            //query moments, to use them to check if axis is defined
            //See AttachEngine3D::calculateAttachedPlacement:case mmInertial for comment explaining these comparisons
            Standard_Real I1, I2, I3;
            pr.Moments(I1,I2,I3);
            Standard_Real d12, d23, d31;
            d12 = fabs(I1-I2);
            d23 = fabs(I2-I3);
            d31 = fabs(I3-I1);

            if (mmode == mm1AxisInertia1){
                LineDir = pr.FirstAxisOfInertia();
                if (pr.HasSymmetryAxis() && !(d23 < d31 && d23 < d12))
                    throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement:AxisOfInertia: inertia tensor has axis of symmetry; first axis of inertia is undefined.");
            } else if (mmode == mm1AxisInertia2) {
                LineDir = pr.SecondAxisOfInertia();
                if (pr.HasSymmetryAxis() && !(d31 < d12 && d31 < d23))
                    throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement:AxisOfInertia: inertia tensor has axis of symmetry; second axis of inertia is undefined.");
            } else if (mmode == mm1AxisInertia3) {
                LineDir = pr.ThirdAxisOfInertia();
                if (pr.HasSymmetryAxis() && !(d12 < d23 && d12 < d31))
                    throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement:AxisOfInertia: inertia tensor has axis of symmetry; third axis of inertia is undefined.");
            }
        }break;
        case mm1TwoPoints:{
            std::vector<gp_Pnt> points;

            for (std::size_t i = 0; i < shapes.size(); i++) {
                const TopoDS_Shape &sh = *shapes[i];
                if (sh.IsNull())
                    throw Base::ValueError("Null shape in AttachEngineLine::calculateAttachedPlacement()!");
                if (sh.ShapeType() == TopAbs_VERTEX){
                    const TopoDS_Vertex &v = TopoDS::Vertex(sh);
                    points.push_back(BRep_Tool::Pnt(v));
                } else if (sh.ShapeType() == TopAbs_EDGE) {
                    const TopoDS_Edge &e = TopoDS::Edge(sh);
                    BRepAdaptor_Curve crv(e);
                    double u1 = crv.FirstParameter();
                    double u2 = crv.LastParameter();
                    if ( Precision::IsInfinite(u1)
                         || Precision::IsInfinite(u2) ){
                        u1 = 0.0;
                        u2 = 1.0;
                    }
                    points.push_back(crv.Value(u1));
                    points.push_back(crv.Value(u2));
                }
                if (points.size() >= 2)
                    break;
            }

            if(points.size()<2)
                throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement: less than 2 points are specified, cannot derive the line.");

            gp_Pnt p0 = points[0];
            gp_Pnt p1 = points[1];

            LineDir = gp_Dir(gp_Vec(p0,p1));
            LineBasePoint = p0;

        }break;
        case mm1Asymptote1:
        case mm1Asymptote2:{
            if (shapes[0]->IsNull())
                throw Base::ValueError("Null shape in AttachEngineLine::calculateAttachedPlacement()!");
            const TopoDS_Edge &e = TopoDS::Edge(*(shapes[0]));
            BRepAdaptor_Curve adapt (e);
            if (adapt.GetType() != GeomAbs_Hyperbola)
                throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement: Asymptotes are available only for hyperbola-shaped edges, the one supplied is not.");
            gp_Hypr hyp = adapt.Hyperbola();
            if (mmode == mm1Asymptote1)
                LineDir = hyp.Asymptote1().Direction();
            else
                LineDir = hyp.Asymptote2().Direction();
            LineBasePoint = hyp.Location();
        }break;
        case mm1Directrix1:
        case mm1Directrix2:{
            if (shapes[0]->IsNull())
                throw Base::ValueError("Null shape in AttachEngineLine::calculateAttachedPlacement()!");
            const TopoDS_Edge &e = TopoDS::Edge(*(shapes[0]));
            BRepAdaptor_Curve adapt (e);
            gp_Ax1 dx1, dx2;//vars to receive directrices
            switch(adapt.GetType()){
            case GeomAbs_Ellipse:{
                gp_Elips cc = adapt.Ellipse();
                dx1 = cc.Directrix1();
                dx2 = cc.Directrix2();
            }break;
            case GeomAbs_Hyperbola:{
                gp_Hypr cc = adapt.Hyperbola();
                dx1 = cc.Directrix1();
                dx2 = cc.Directrix2();
            }break;
            case GeomAbs_Parabola:{
                gp_Parab cc = adapt.Parabola();
                dx1 = cc.Directrix();
                if (mmode == mm1Directrix2)
                    throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement: Parabola has no second directrix");
            }break;
            default:
                throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement: referenced edge is not a conic section with a directrix");
            }
            if (mmode == mm1Directrix1){
                LineDir = dx1.Direction();
                LineBasePoint = dx1.Location();
            } else {
                LineDir = dx2.Direction();
                LineBasePoint = dx2.Location();
            }
        }break;
        case mm1Proximity:{
            if (shapes.size() < 2)
                throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement: Proximity mode requires two shapes; only one is supplied");
            if (shapes[0]->IsNull())
                throw Base::ValueError("Null shape in AttachEngineLine::calculateAttachedPlacement()!");
            if (shapes[1]->IsNull())
                throw Base::ValueError("Null shape in AttachEngineLine::calculateAttachedPlacement()!");
            BRepExtrema_DistShapeShape distancer (*(shapes[0]), *(shapes[1]));
            if (!distancer.IsDone())
                throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement: proximity calculation failed.");
            if (distancer.NbSolution()>1)
                Base::Console().Warning("AttachEngineLine::calculateAttachedPlacement: proximity calculation gave %i solutions, ambiguous.\n",int(distancer.NbSolution()));
            gp_Pnt p1 = distancer.PointOnShape1(1);
            gp_Pnt p2 = distancer.PointOnShape2(1);
            LineBasePoint = p1;
            gp_Vec dist = gp_Vec(p1,p2);
            if (dist.Magnitude() < Precision::Confusion())
                throw Base::ValueError("AttachEngineLine::calculateAttachedPlacement: can't make proximity line, because shapes touch or intersect");
            LineDir = gp_Dir(dist);
        }break;
        default:
            throwWrongMode(mmode);
        }

        plm = this->placementFactory(LineDir, gp_Vec(), LineBasePoint, refOrg,
                                       /*useRefOrg_Line = */ true);
    } else {//re-use 3d mode
        AttachEngine3D attacher3D;
        attacher3D.setUp(*this);
        attacher3D.mapMode = mmode;
        attacher3D.attachmentOffset = Base::Placement(); //AttachmentOffset is applied separately here, afterwards. So we are resetting it in sub-attacher to avoid applying it twice!
        plm = attacher3D.calculateAttachedPlacement(origPlacement);
        plm *= presuperPlacement;
    }
    plm *= this->attachmentOffset;
    return plm;
}


//=================================================================================

TYPESYSTEM_SOURCE(Attacher::AttachEnginePoint, Attacher::AttachEngine)

AttachEnginePoint::AttachEnginePoint()
{
    //fill type lists for modes
    modeRefTypes.resize(mmDummy_NumberOfModes);
    refTypeString s;

    //re-used 3d modes
    AttachEngine3D attacher3D;
    modeRefTypes[mm0Origin] = attacher3D.modeRefTypes[mmObjectXY];
    modeRefTypes[mm0CenterOfCurvature] = attacher3D.modeRefTypes[mmRevolutionSection];
    modeRefTypes[mm0OnEdge] = attacher3D.modeRefTypes[mmNormalToPath];

    modeRefTypes[mm0Vertex].push_back(cat(rtVertex));
    modeRefTypes[mm0Vertex].push_back(cat(rtLine));

    modeRefTypes[mm0Focus1].push_back(cat(rtConic));

    modeRefTypes[mm0Focus2].push_back(cat(rtEllipse));
    modeRefTypes[mm0Focus2].push_back(cat(rtHyperbola));

    s = cat(rtAnything, rtAnything);
    modeRefTypes[mm0ProximityPoint1].push_back(s);
    modeRefTypes[mm0ProximityPoint2].push_back(s);

    modeRefTypes[mm0CenterOfMass].push_back(cat(rtAnything));
    modeRefTypes[mm0CenterOfMass].push_back(cat(rtAnything,rtAnything));
    modeRefTypes[mm0CenterOfMass].push_back(cat(rtAnything,rtAnything,rtAnything));
    modeRefTypes[mm0CenterOfMass].push_back(cat(rtAnything,rtAnything,rtAnything,rtAnything));

    this->EnableAllSupportedModes();
}

AttachEnginePoint *AttachEnginePoint::copy() const
{
    AttachEnginePoint* p = new AttachEnginePoint;
    p->setUp(*this);
    return p;
}

Base::Placement AttachEnginePoint::calculateAttachedPlacement(Base::Placement origPlacement) const
{
    eMapMode mmode = this->mapMode;

    //modes that are mirrors of attacher3D:
    bool bReUsed = true;
    switch(mmode){
    case mmDeactivated:
        throw ExceptionCancel();//to be handled in positionBySupport, to not do anything if disabled
    case mm0Origin:
        mmode = mmObjectXY;
        break;
    case mm0CenterOfCurvature:
        mmode = mmRevolutionSection;
        break;
    case mm0OnEdge:
        //todo: prevent thruPoint
        mmode = mmNormalToPath;
        break;
    default:
        bReUsed = false;
    }

    Base::Placement plm;
    if (!bReUsed){
        std::vector<App::GeoFeature*> parts;
        std::vector<const TopoDS_Shape*> shapes;
        std::vector<TopoDS_Shape> copiedShapeStorage;
        std::vector<eRefType> types;
        readLinks(this->references, parts, shapes, copiedShapeStorage, types);

        if (parts.empty())
            throw ExceptionCancel();


        //variables to derive the actual placement.
        //They are to be set, depending on the mode:
        gp_Pnt BasePoint; //where to put the point


        switch (mmode) {
        case mm0Vertex:{
            std::vector<gp_Pnt> points;
            assert(shapes.size()>0);

            const TopoDS_Shape &sh = *shapes[0];
            if (sh.IsNull())
                throw Base::ValueError("Null shape in AttachEnginePoint::calculateAttachedPlacement()!");
            if (sh.ShapeType() == TopAbs_VERTEX){
                const TopoDS_Vertex &v = TopoDS::Vertex(sh);
                BasePoint = BRep_Tool::Pnt(v);
            } else if (sh.ShapeType() == TopAbs_EDGE) {
                const TopoDS_Edge &e = TopoDS::Edge(sh);
                BRepAdaptor_Curve crv(e);
                double u = crv.FirstParameter();
                if(Precision::IsInfinite(u))
                    throw Base::ValueError("Edge is infinite");
                BasePoint = crv.Value(u);
            }

        }break;
        case mm0Focus1:
        case mm0Focus2:{
            if (shapes[0]->IsNull())
                throw Base::ValueError("Null shape in AttachEnginePoint::calculateAttachedPlacement()!");
            const TopoDS_Edge &e = TopoDS::Edge(*(shapes[0]));
            BRepAdaptor_Curve adapt (e);
            gp_Pnt f1, f2;
            switch(adapt.GetType()){
            case GeomAbs_Ellipse:{
                gp_Elips cc = adapt.Ellipse();
                f1 = cc.Focus1();
                f2 = cc.Focus2();
            }break;
            case GeomAbs_Hyperbola:{
                gp_Hypr cc = adapt.Hyperbola();
                f1 = cc.Focus1();
                f2 = cc.Focus2();
            }break;
            case GeomAbs_Parabola:{
                gp_Parab cc = adapt.Parabola();
                f1 = cc.Focus();
                if (mmode == mm0Focus2)
                    throw Base::ValueError("AttachEnginePoint::calculateAttachedPlacement: Parabola has no second focus");
            }break;
            default:
                throw Base::ValueError("AttachEnginePoint::calculateAttachedPlacement: referenced edge is not a conic section with a directrix");
            }
            if (mmode == mm0Focus1)
                BasePoint = f1;
            else
                BasePoint = f2;
        }break;
        case mm0ProximityPoint1:
        case mm0ProximityPoint2:{
            if (shapes.size() < 2)
                throw Base::ValueError("AttachEnginePoint::calculateAttachedPlacement: Proximity mode requires two shapes; only one is supplied");
            if (shapes[0]->IsNull())
                throw Base::ValueError("Null shape in AttachEnginePoint::calculateAttachedPlacement()!");
            if (shapes[1]->IsNull())
                throw Base::ValueError("Null shape in AttachEnginePoint::calculateAttachedPlacement()!");

            BasePoint = getProximityPoint(mmode, *(shapes[0]), *(shapes[1]));
        }break;
        case mm0CenterOfMass:{
            GProp_GProps gpr =  AttachEngine::getInertialPropsOfShape(shapes);
            BasePoint = gpr.CentreOfMass();
        }break;
        default:
            throwWrongMode(mmode);
        }

        plm = this->placementFactory(gp_Vec(0.0,0.0,1.0), gp_Vec(1.0,0.0,0.0), BasePoint, gp_Pnt());
    } else {//re-use 3d mode
        AttachEngine3D attacher3D;
        attacher3D.setUp(*this);
        attacher3D.mapMode = mmode;
        attacher3D.attachmentOffset = Base::Placement(); //AttachmentOffset is applied separately here, afterwards. So we are resetting it in sub-attacher to avoid applying it twice!
        plm = attacher3D.calculateAttachedPlacement(origPlacement);
    }
    plm *= this->attachmentOffset;
    return plm;
}

gp_Pnt AttachEnginePoint::getProximityPoint(eMapMode mmode, const TopoDS_Shape& s1, const TopoDS_Shape& s2) const
{
    // #0003921: Crash when opening document with datum point intersecting line and plane
    //
    // BRepExtrema_DistanceSS is used inside BRepExtrema_DistShapeShape and can cause
    // a crash if the input shape is an unlimited face.
    // So, when the input is a face and an edge then before checking for minimum distances
    // try to determine intersection points.
    try {
        TopoDS_Shape face, edge;
        if (s1.ShapeType() == TopAbs_FACE &&
            s2.ShapeType() == TopAbs_EDGE) {
            face = s1;
            edge = s2;
        }
        else if (s1.ShapeType() == TopAbs_EDGE &&
                 s2.ShapeType() == TopAbs_FACE) {
            edge = s1;
            face = s2;
        }

        // edge and face
        if (!edge.IsNull() && !face.IsNull()) {
            BRepAdaptor_Curve crv(TopoDS::Edge(edge));
            BRepIntCurveSurface_Inter intCS;
            intCS.Init(face, crv.Curve(), Precision::Confusion());
            std::vector<gp_Pnt> points;
            for (; intCS.More(); intCS.Next()) {
                gp_Pnt pnt = intCS.Pnt();
                points.push_back(pnt);
            }

            if (points.size() > 1)
                Base::Console().Warning("AttachEnginePoint::calculateAttachedPlacement: proximity calculation gave %d solutions, ambiguous.\n", int(points.size()));

            // if an intersection is found return the first hit
            // otherwise continue with BRepExtrema_DistShapeShape
            if (!points.empty())
                return points.front();
        }
    }
    catch (Standard_Failure) {
        // ignore
    }

    BRepExtrema_DistShapeShape distancer (s1, s2);
    if (!distancer.IsDone())
        throw Base::ValueError("AttachEnginePoint::calculateAttachedPlacement: proximity calculation failed.");
    if (distancer.NbSolution() > 1)
        Base::Console().Warning("AttachEnginePoint::calculateAttachedPlacement: proximity calculation gave %i solutions, ambiguous.\n",int(distancer.NbSolution()));

    gp_Pnt p1 = distancer.PointOnShape1(1);
    gp_Pnt p2 = distancer.PointOnShape2(1);
    if (mmode == mm0ProximityPoint1)
        return p1;
    else
        return p2;
}
