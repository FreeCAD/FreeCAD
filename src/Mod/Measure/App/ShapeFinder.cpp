// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 wandererfan <wandererfan@gmail.com>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

//! ShapeFinder is a class to obtain the located shape pointed at by a DocumentObject and a
//! "new-style" long subelement name. It hides the complexities of obtaining the correct object
//! and its placement.


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <boost_regex.hpp>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <TopLoc_Location.hxx>

#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/Link.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Part.h>
#include <Base/Tools.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/AttachExtension.h>
#include <Mod/Part/App/Attacher.h>

#include "ShapeFinder.h"


using namespace Measure;

//! ResolveResult is a class to hold the result of resolving a selection into the actual target
//! object and traditional subElement name (Vertex1).

ResolveResult::ResolveResult(const App::DocumentObject* realTarget,
                             const std::string& shortSubName,
                             const App::DocumentObject* targetParent)
    : m_target(App::SubObjectT(realTarget, shortSubName.c_str()))
    , m_targetParent(App::DocumentObjectT(targetParent))
{}

App::DocumentObject& ResolveResult::getTarget() const
{
    return *(m_target.getObject());
}

std::string ResolveResult::getShortSub() const
{
    return m_target.getSubName();
}

App::DocumentObject& ResolveResult::getTargetParent() const
{
    return *(m_targetParent.getObject());
}


//! returns the actual target object and subname pointed to by selectObj and selectLongSub (which
//! is likely a result from getSelection or getSelectionEx)
ResolveResult ShapeFinder::resolveSelection(const App::DocumentObject& selectObj,
                                            const std::string& selectLongSub)
{
    App::DocumentObject* targetParent {nullptr};
    std::string childName {};
    const char* subElement {nullptr};
    App::DocumentObject* realTarget =
        selectObj.resolve(selectLongSub.c_str(), &targetParent, &childName, &subElement);
    auto shortSub = getLastTerm(selectLongSub);
    return {realTarget, shortSub, targetParent};
}


//! returns the shape of rootObject+leafSub. Any transforms from objects in the path from rootObject
//! to leafSub are applied to the shape.
//! leafSub is typically obtained from Selection as it provides the appropriate longSubname.  The
//! leaf sub string can also be constructed by walking the tree.
// TODO: to truly locate the shape, we need to consider attachments - see
// ShapeExtractor::getShapesFromXRoot()
//       and ShapeFinder::getLinkAttachParent()
TopoDS_Shape ShapeFinder::getLocatedShape(const App::DocumentObject& rootObject,
                                          const std::string& leafSub)
{
    auto resolved = resolveSelection(rootObject, leafSub);
    auto target = &resolved.getTarget();
    auto shortSub = resolved.getShortSub();
    if (!target) {
        return {};
    }

    TopoDS_Shape shape =
        Part::Feature::getShape(target,
                                Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (isShapeReallyNull(shape)) {
        return {};
    }

    auto cleanSub = removeTnpInfo(leafSub);
    auto transform = getGlobalTransform(rootObject, cleanSub);

    shape = transformShape(shape, transform.first, transform.second);
    Part::TopoShape tShape {shape};
    if (!shortSub.empty()) {
        return tShape.getSubTopoShape(shortSub.c_str()).getShape();
    }

    return tShape.getShape();
}


//! convenient version of previous method
Part::TopoShape ShapeFinder::getLocatedTopoShape(const App::DocumentObject& rootObject,
                                                 const std::string& leafSub)
{
    return {getLocatedShape(rootObject, leafSub)};
}


//! traverse the tree from leafSub up to rootObject, obtaining placements along the way.  Note that
//! the placements will need to be applied in the reverse order (ie top down) of what is delivered
//! in plm stack.  leafSub is a dot separated longSubName which DOES NOT include rootObject.  the
//! result does not include rootObject's transform.
void ShapeFinder::crawlPlacementChain(std::vector<Base::Placement>& plmStack,
                                      std::vector<Base::Matrix4D>& scaleStack,
                                      const App::DocumentObject& rootObject,
                                      const std::string& leafSub)
{
    auto currentSub = leafSub;
    std::string previousSub {};
    while (!currentSub.empty() && currentSub != previousSub) {
        auto resolved = resolveSelection(rootObject, currentSub);
        auto target = &resolved.getTarget();
        if (!target) {
            return;
        }
        auto currentPlacement = getPlacement(target);
        auto currentScale = getScale(target);
        if (!currentPlacement.isIdentity() || !currentScale.isUnity()) {
            plmStack.push_back(currentPlacement);
            scaleStack.push_back(currentScale);
        }
        previousSub = currentSub;
        currentSub = pruneLastTerm(currentSub);
    }
}


//! return inShape with placement and scaler applied.  If inShape contains any infinite subshapes
//! (such as Datum planes), the infinite shapes will not be included in the result.
TopoDS_Shape ShapeFinder::transformShape(TopoDS_Shape& inShape,
                                         const Base::Placement& placement,
                                         const Base::Matrix4D& scaler)
{
    if (isShapeReallyNull(inShape)) {
        return {};
    }
    // we modify the parameter shape here.  we don't claim to be const, but may be better to copy
    // the shape?
    Part::TopoShape tshape {inShape};
    if (tshape.isInfinite()) {
        inShape = stripInfiniteShapes(inShape);
    }

    // copying the shape prevents "non-orthogonal GTrsf" errors in some versions
    // of OCC.  Something to do with triangulation of shape??
    // it may be that incremental mesh would work here too.
    BRepBuilderAPI_Copy copier(inShape);
    tshape = Part::TopoShape(copier.Shape());
    if (tshape.isNull()) {
        return {};
    }

    tshape.transformShape(scaler, true, true);
    tshape.setPlacement(placement);

    return tshape.getShape();
}


//! this getter should work for any object, not just links
Base::Placement ShapeFinder::getPlacement(const App::DocumentObject* root)
{
    auto namedProperty = root->getPropertyByName("Placement");
    auto placementProperty = dynamic_cast<App::PropertyPlacement*>(namedProperty);
    if (namedProperty && placementProperty) {
        return placementProperty->getValue();
    }
    return {};
}


//! get root's scale property.  If root is not a Link related object, then the identity matrrix will
//! be returned.
Base::Matrix4D ShapeFinder::getScale(const App::DocumentObject* root)
{
    if (!isLinkLike(root)) {
        return {};
    }

    Base::Matrix4D linkScale;
    auto namedProperty = root->getPropertyByName("ScaleVector");
    auto scaleVectorProperty = dynamic_cast<App::PropertyVector*>(namedProperty);
    if (scaleVectorProperty) {
        linkScale.scale(scaleVectorProperty->getValue());
    }
    return linkScale;
}


//! there isn't convenient common ancestor for the members of the Link family.  We use
//! isLinkLike(obj) instead of obj->isDerivedFrom<ConvenientCommonAncestor>().  Some links have
//! proxy objects and will not be detected by isDerivedFrom().
bool ShapeFinder::isLinkLike(const App::DocumentObject* obj)
{
    if (!obj) {
        return false;
    }

    if (obj->isDerivedFrom<App::Link>() || obj->isDerivedFrom<App::LinkElement>()
        || obj->isDerivedFrom<App::LinkGroup>()) {
        return true;
    }

    auto namedProperty = obj->getPropertyByName("LinkedObject");
    auto linkedObjectProperty = dynamic_cast<App::PropertyLink*>(namedProperty);
    if (linkedObjectProperty) {
        return true;
    }

    namedProperty = obj->getPropertyByName("ElementList");
    auto elementListProperty = dynamic_cast<App::PropertyLinkList*>(namedProperty);
    return elementListProperty != nullptr;
}


//! Infinite shapes can not be projected, so they need to be removed. inShape is usually a compound.
//! Datum features (Axis, Plane and CS) are examples of infinite shapes.
TopoDS_Shape ShapeFinder::stripInfiniteShapes(const TopoDS_Shape& inShape)
{
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    TopoDS_Iterator it(inShape);
    for (; it.More(); it.Next()) {
        TopoDS_Shape shape = it.Value();
        if (shape.ShapeType() < TopAbs_SOLID) {
            // look inside composite shapes
            shape = stripInfiniteShapes(shape);
        }
        else if (Part::TopoShape(shape).isInfinite()) {
            continue;
        }
        // simple shape & finite
        builder.Add(comp, shape);
    }

    return {std::move(comp)};
}


//! check for shape is null or shape has no subshapes(vertex/edge/face/etc)
//! this handles the case of an empty compound which is not IsNull, but has no
//! content.
// Note: the same code exists in TechDraw::ShapeUtils
bool ShapeFinder::isShapeReallyNull(const TopoDS_Shape& shape)
{
    // if the shape is null or it has no subshapes, then it is really null
    return shape.IsNull() || !TopoDS_Iterator(shape).More();
}


//! Returns the net transformation of a path from rootObject to leafSub. rootObject's transform
//! is included in the result.
std::pair<Base::Placement, Base::Matrix4D>
ShapeFinder::getGlobalTransform(const App::DocumentObject& rootObject, const std::string& leafSub)
{
    // we prune the last term if it is a vertex, edge or face
    std::string newSub = removeGeometryTerm(leafSub);

    std::vector<Base::Placement> plmStack;
    std::vector<Base::Matrix4D> scaleStack;
    // get transforms below rootObject
    // Note: root object is provided by the caller and may or may not be a top level object
    crawlPlacementChain(plmStack, scaleStack, rootObject, newSub);

    auto pathTransform = sumTransforms(plmStack, scaleStack);

    // apply the placements in reverse order - top to bottom
    // should this be rootObject's local transform?
    auto rootTransform = getGlobalTransform(&rootObject);

    auto netPlm = rootTransform.first * pathTransform.first;
    auto netScale = rootTransform.second * pathTransform.second;

    return {netPlm, netScale};
}


//! tries to get the global position and scale for a object with no information about the
//! path through the tree from a root to cursor object.
std::pair<Base::Placement, Base::Matrix4D>
ShapeFinder::getGlobalTransform(const App::DocumentObject* cursorObject)
{
    if (!cursorObject) {
        return {};
    }

    Base::Placement netPlm;
    Base::Matrix4D netScale = getScale(cursorObject);

    Base::Placement geoPlm;
    auto geoCursor = dynamic_cast<const App::GeoFeature*>(cursorObject);
    if (!isLinkLike(cursorObject) && geoCursor) {
        netPlm = geoCursor->globalPlacement();
        return {netPlm, netScale};
    }

    netPlm = getPlacement(cursorObject);

    return {netPlm, netScale};
}


//! combine a series of placement & scale transforms.  The input stacks are expected in leaf to root
//! order, but the result is in the expected root to leaf order.
std::pair<Base::Placement, Base::Matrix4D>
ShapeFinder::sumTransforms(const std::vector<Base::Placement>& plmStack,
                           const std::vector<Base::Matrix4D>& scaleStack)
{
    Base::Placement netPlm;
    Base::Matrix4D netScale;

    auto itRevPlm = plmStack.rbegin();
    for (; itRevPlm != plmStack.rend(); itRevPlm++) {
        netPlm *= *itRevPlm;
    }
    auto itRevScale = scaleStack.rbegin();
    for (; itRevScale != scaleStack.rend(); itRevScale++) {
        netScale *= *itRevScale;
    }

    return {netPlm, netScale};
}


//! get the parent to which attachObject is attached via Links (not regular Part::Attacher
//! attachment)
App::DocumentObject* ShapeFinder::getLinkAttachParent(const App::DocumentObject* attachedObject)
{
    auto namedProperty = attachedObject->getPropertyByName("a1AttParent");
    auto attachProperty = dynamic_cast<App::PropertyLink*>(namedProperty);
    if (namedProperty && attachProperty) {
        return attachProperty->getValue();
    }
    return {};
}


//! debugging routine that returns a string representation of a placement.
// TODO: this should be in Base::Placement?
std::string ShapeFinder::PlacementAsString(const Base::Placement& inPlacement)
{
    auto position = inPlacement.getPosition();
    auto rotation = inPlacement.getRotation();
    Base::Vector3d axis;
    double angle {0.0};
    rotation.getValue(axis, angle);
    std::stringstream ss;
    ss << "pos: (" << position.x << ", " << position.y << ", " << position.z << ")  axis: ("
       << axis.x << ", " << axis.y << ", " << axis.z << ")  angle: " << Base::toDegrees(angle);
    return ss.str();
}


//! debug routine. return readable form of TopLoc_Location from OCC
std::string ShapeFinder::LocationAsString(const TopLoc_Location& location)
{
    auto position = Base::Vector3d {location.Transformation().TranslationPart().X(),
                                    location.Transformation().TranslationPart().Y(),
                                    location.Transformation().TranslationPart().Z()};
    gp_XYZ axisDir;
    double angle {0};
    auto isRotation = location.Transformation().GetRotation(axisDir, angle);
    Base::Vector3d axis {axisDir.X(), axisDir.Y(), axisDir.Z()};

    std::stringstream ss;
    ss << "isRotation: " << isRotation << "  pos: (" << position.x << ", " << position.y << ", "
       << position.z << ")  axis: (" << axisDir.X() << ", " << axisDir.Y() << ", " << axisDir.Z()
       << ")  angle: " << Base::toDegrees(angle);
    return ss.str();
}
