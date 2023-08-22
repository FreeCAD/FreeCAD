/***************************************************************************
 *   Copyright (c) 2015 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
/**
  *Attacher.h, Attacher.cpp contain the functionality of deriving placement
  *from a set of geometric subelements. Examples are: sketch attachment, datum
  *plane placement.
  */

#ifndef PARTATTACHER_H
#define PARTATTACHER_H

#include <GProp_GProps.hxx>

#include <App/GeoFeature.h>
#include <App/PropertyLinks.h>
#include <Base/Exception.h>
#include <Base/Placement.h>

#include "PartFeature.h"


namespace Attacher
{

class AttachEngine;

//Attention! The numbers associated to the modes are permanent, because they are what get written into files.
enum eMapMode {
    mmDeactivated,
    mmTranslate,
    mmObjectXY,
    mmObjectXZ,
    mmObjectYZ,
    mmFlatFace,
    mmTangentPlane,
    mmNormalToPath,
    mmFrenetNB,
    mmFrenetTN,
    mmFrenetTB,
    mmConcentric,
    mmRevolutionSection,
    mmThreePointsPlane,
    mmThreePointsNormal,
    mmFolding,

    mm1AxisX,
    mm1AxisY,
    mm1AxisZ,
    mm1AxisCurv,
    mm1Directrix1,
    mm1Directrix2,
    mm1Asymptote1,
    mm1Asymptote2,
    mm1Tangent,
    mm1Normal,
    mm1Binormal,
    mm1TangentU,
    mm1TangentV,
    mm1TwoPoints,
    mm1Intersection,
    mm1Proximity,

    mm0Origin,
    mm0Focus1,
    mm0Focus2,
    mm0OnEdge,
    mm0CenterOfCurvature,
    mm0CenterOfMass,
    mm0Intersection,
    mm0Vertex,
    mm0ProximityPoint1,
    mm0ProximityPoint2,

    mm1AxisInertia1,
    mm1AxisInertia2,
    mm1AxisInertia3,

    mmInertialCS,

    mm1FaceNormal,

    mmOZX,
    mmOZY,
    mmOXY,
    mmOXZ,
    mmOYZ,
    mmOYX,

    mmDummy_NumberOfModes//a value useful to check the validity of mode value
};//see also eMapModeStrings[] definition in .cpp


/**
 * @brief The eRefType enum lists the types of references. If adding one, see
 * also AttachEngine::eRefTypeStrings, AttachEngine::getShapeType(),
 * AttachEngine::downgradeType(), AttacherTexts.cpp/getShTypeText()
 */
enum eRefType {
    //topo             //ranks: (number of times the type is downgradable)
    rtAnything,        //0
    rtVertex,          //1
    rtEdge,            //1
    rtFace,            //1
    //edges:
    rtLine,            //2
    rtCurve,           //2
    rtCircle,          //3
    rtConic,           //3
    rtEllipse,         //4
    rtParabola,        //4
    rtHyperbola,       //4
    //faces:
    rtFlatFace,        //2
    rtSphericalFace,   //2//flatface, shericalface are also surfaces of revolution, but the axis isn't defined.
    rtSurfaceRev,      //2
    rtCylindricalFace, //3
    rtToroidalFace,    //3
    rtConicalFace,     //3
    //shapes:
    rtPart,            //1
    rtSolid,           //2
    rtWire,            //2
    rtDummy_numberOfShapeTypes,//a value useful to check the validity of value
    rtFlagHasPlacement = 0x0100 //indicates that the linked shape is a whole FreeCAD object that has placement available.
};


using refTypeString = std::vector<eRefType>; //a sequence of ref types, according to Support contents for example
using refTypeStringList = std::vector<refTypeString>; //a set of type strings, defines which selection sets are supported by a certain mode


/**
 * @brief The SuggestResult struct is a container for output information of AttachEngine mode suggesting routine.
 */
struct SuggestResult{
    /**
     * @brief message contains overall verdict of suggestor on current reference set
     */
    enum eSuggestResult{
        srOK, //references are valid for at least one mode
        srLinkBroken, //failed to resolve out some of current references. Exception info is stored in SuggestResult::error.
        srUnexpectedError,
        srNoModesFit,//none of the available mapping modes accepts the set of topological type
        srIncompatibleGeometry,//there is a mode that could fit, but geometry is wrong (e.g. a line is required, but a curve was passed).
    };
    eSuggestResult message;

    /**
      * @brief allApplicableModes. Vector array that will receive the list of
      * all modes that are applicable to current set of references. It doesn't
      * guarantee that all modes will work, it only checks that subelemnts are
      * of right type.
      */
    std::vector<eMapMode> allApplicableModes;

    /**
     * @brief bestFitMode is the mode that is the most specific to current
     * references. Note that the mode may not be valid for current references;
     * check if it's listed in allApplicableModes, or test if message == srOK.
     */
    eMapMode bestFitMode;

    /**
     * @brief nextRefTypeHint: a hint of what can be added to references to
     * achieve other modes.
     */
    std::set<eRefType> nextRefTypeHint;

    /**
     * @brief reachableModes. List of modes that can be reached by selecting
     * more references. Is a map, where key is the mode that can be reached,
     * and value is a list of reference sequences that can be added to reach
     * the mode (stuff already linked is omitted from these lists; only extra
     * links needed are listed)
     */
    std::map<eMapMode, refTypeStringList> reachableModes;

    /**
     * @brief references_Types: list of types of references, as queried when
     * running suggesting routine.
     */
    refTypeString references_Types;

    Base::RuntimeError error;
};


/**
 * @brief The AttachEngine class is the placement calculation routine, modes,
 * hints and so on. It can be used separately, without deriving from
 * AttachableObject.
 */
class PartExport AttachEngine : public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //methods
    AttachEngine();
    virtual void setUp(const App::PropertyLinkSubList &references,
                      eMapMode mapMode = mmDeactivated,
                      bool mapReverse = false,
                      double attachParameter = 0.0,
                      double surfU = 0.0, double surfV = 0.0,
                      const Base::Placement &attachmentOffset = Base::Placement());
    virtual void setUp(const AttachEngine &another);
    virtual AttachEngine* copy() const = 0;
    virtual Base::Placement calculateAttachedPlacement(const Base::Placement& origPlacement) const = 0;

    /**
     * @brief placementFactory calculates placement from Z axis direction,
     * optional X axis direction, and origin point.
     *
     * @param ZAxis (input) mandatory. Z axis of the returned placement will
     * strictly coincide with ZAxis.
     *
     * @param XAxis (input) optional (i.e., can be zero). Sets the preferred X
     * axis orientation. If it is not perpendicular to ZAxis, it will be forced
     * to be. If XAxis is zero, the effect is equivalent to setting
     * makeYVertical to true.
     *
     * @param Origin (input) mandatory.
     *
     * @param refOrg (input). The point that will be used in case any of
     * useRefOrg_XX parameters is true.
     *
     * @param useRefOrg_Line (input). If true, Origin will be moved along ZAxis
     * to be as close as possible to refOrg.
     *
     * @param useRefOrg_Plane (input). If true, Origin will be moved in
     * XAxis-YAxis plane to be as close as possible to refOrg.
     *
     * @param makeYVertical (input). If true, XAxis is ignored, and X and Y
     * axes are defined in order to make Y axis go as upwards as possible. If
     * ZAxis is strictly upwards, XY will match global XY. If ZAxis is strictly
     * downwards, XAxis will be the reversed global X axis.
     *
     * @param makeLegacyFlatFaceOrientation (input). Modifies the behavior of
     * makeYVertical to match the logic that was used in mapping of sketches to
     * flat faces in FreeCAD prior to introduction of Attacher. Set
     * makeYVertical to true if using this.
     *
     * @return the resulting placement. ReverseXY property of Attacher will be automatically applied.
     */
     Base::Placement placementFactory(const gp_Dir &ZAxis,
                                      gp_Vec XAxis,
                                      gp_Pnt Origin,
                                      gp_Pnt refOrg = gp_Pnt(),
                                      bool useRefOrg_Line = false,
                                      bool useRefOrg_Plane = false,
                                      bool makeYVertical = false,
                                      bool makeLegacyFlatFaceOrientation = false,
                                      Base::Placement* placeOfRef = nullptr) const;

    /**
     * @brief suggestMapModes is the procedure that knows everything about
     * mapping modes. It returns the most appropriate mapping mode, as well as
     * list of all modes that will accept the set of references. In case no modes apply,
     * extra information regarding reasons is returned in msg.
     *
     * @param result (output). Returns results of suggestion, such as best fit
     * mode, list of all modes that apply, hints, etc.
     */
    virtual void suggestMapModes(SuggestResult &result) const;

    /**
     * @brief EnableAllModes enables all modes that have shape type lists filled. The function acts on modeEnabled array.
     */
    void EnableAllSupportedModes();

    ~AttachEngine() override = default;

public://helper functions that may be useful outside of the class
    /**
     * @brief getShapeType by shape. Will never set rtFlagHasPlacement.
     * @param sh
     * @return
     */
    static eRefType getShapeType(const TopoDS_Shape &sh);

    /**
     * @brief getShapeType by link content. Will include rtFlagHasPlacement, if applies.
     * @param obj
     * @param subshape (input). Can be empty string (then, whole object will be used for shape type testing)
     * @return
     */
    static eRefType getShapeType(const App::DocumentObject* obj,
                                 const std::string &subshape);

    /**
     * @brief downgradeType converts a more-specific type into a less-specific
     * type (e.g. rtCircle->rtCurve, rtCurve->rtEdge, rtEdge->rtAnything)
     * @param type
     * @return the downgraded type.
     */
    static eRefType downgradeType(eRefType type);

    /**
     * @brief getTypeRank determines, how specific is the supplied shape type.
     * The ranks are outlined in definition of eRefType. The ranks are defined
     * by implementation of downgradeType().
     * @param type
     * @return number of times the type can be downgradeType() before it
     * becomes rtAnything
     */
    static int getTypeRank(eRefType type);

    /**
     * @brief isShapeOfType tests if a shape fulfills the requirement of a mode, and returns a score of how spot on was the requirement.
     * @param shapeType (use return value of AttachEngine::getShapeType)
     * @param requirement
     * @return : -1 - doesn't fulfill,
     *           0 - compatible topology, but incompatible specific (e.g. rtLine, rtCircle);
     *           1 - valid by generic type (e.g. rtCircle is rtEdge),
     *           2 and up - more and more specific match (according to rank of requirement)
     */
    static int isShapeOfType(eRefType shapeType, eRefType requirement);

    /**
     * @brief getModeName
     * @param mmode
     * @return returns a string that identifies the attachment mode in enum property.
     */
    static std::string getModeName(eMapMode mmode);

    static eMapMode getModeByName(const std::string &modeName);

    static std::string getRefTypeName(eRefType shapeType);

    static eRefType getRefTypeByName(const std::string &typeName);

    static GProp_GProps getInertialPropsOfShape(const std::vector<const TopoDS_Shape*> &shapes);

    /**
     * @brief verifyReferencesAreSafe: checks if pointers in references still
     * point to objects contained in open documents. This guarantees the links
     * are valid. Throws Base::Exception if invalid links are found.
     */
    static void verifyReferencesAreSafe(const App::PropertyLinkSubList& references);

public: //enums
    static const char* eMapModeStrings[];
    static const char* eRefTypeStrings[];


public: //members
    App::PropertyLinkSubList references;

    eMapMode mapMode = mmDeactivated;
    bool mapReverse = false;
    double attachParameter = 0.0;
    double surfU = 0.0, surfV = 0.0;
    Base::Placement attachmentOffset;

    /**
     * @brief modeEnabled is an indicator, whether some mode is ever suggested
     * or not. Set to false to suppress suggesting some mode, like so:
     * modeEnabled[mmModeIDontLike] = false;
     */
    std::vector<bool> modeEnabled;

    std::vector<refTypeStringList> modeRefTypes; //a complete data structure, containing info on which modes support what selection

protected:
    refTypeString cat(eRefType rt1){
        refTypeString ret;
        ret.push_back(rt1);
        return ret;
    }
    refTypeString cat(eRefType rt1, eRefType rt2){
        refTypeString ret;
        ret.push_back(rt1);
        ret.push_back(rt2);
        return ret;
    }
    refTypeString cat(eRefType rt1, eRefType rt2, eRefType rt3){
        refTypeString ret;
        ret.push_back(rt1);
        ret.push_back(rt2);
        ret.push_back(rt3);
        return ret;
    }
    refTypeString cat(eRefType rt1, eRefType rt2, eRefType rt3, eRefType rt4){
        refTypeString ret;
        ret.push_back(rt1);
        ret.push_back(rt2);
        ret.push_back(rt3);
        ret.push_back(rt4);
        return ret;
    }
    static void readLinks(const App::PropertyLinkSubList &references, std::vector<App::GeoFeature *> &geofs,
                          std::vector<const TopoDS_Shape*>& shapes, std::vector<TopoDS_Shape> &storage,
                          std::vector<eRefType> &types);

    static void throwWrongMode(eMapMode mmode);

};


class PartExport AttachEngine3D : public AttachEngine
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    AttachEngine3D();
    AttachEngine3D* copy() const override;
    Base::Placement calculateAttachedPlacement(const Base::Placement& origPlacement) const override;
private:
    double calculateFoldAngle(gp_Vec axA, gp_Vec axB, gp_Vec edA, gp_Vec edB) const;
};

//attacher specialized for datum planes
class PartExport AttachEnginePlane : public AttachEngine
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    AttachEnginePlane();
    AttachEnginePlane* copy() const override;
    Base::Placement calculateAttachedPlacement(const Base::Placement& origPlacement) const override;
};

//attacher specialized for datum lines
class PartExport AttachEngineLine : public AttachEngine
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    AttachEngineLine();
    AttachEngineLine* copy() const override;
    Base::Placement calculateAttachedPlacement(const Base::Placement& origPlacement) const override;
};

//attacher specialized for datum points
class PartExport AttachEnginePoint : public AttachEngine
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    AttachEnginePoint();
    AttachEnginePoint* copy() const override;
    Base::Placement calculateAttachedPlacement(const Base::Placement& origPlacement) const override;

private:
    gp_Pnt getProximityPoint(eMapMode mode, const TopoDS_Shape& s1, const TopoDS_Shape& s2) const;
};

//====================================================================

class ExceptionCancel : public Base::Exception
{
public:
    ExceptionCancel() = default;
    explicit ExceptionCancel(char* msg){this->setMessage(msg);}
    ~ExceptionCancel() noexcept override = default;
};

} // namespace Attacher

#endif // PARTATTACHER_H
