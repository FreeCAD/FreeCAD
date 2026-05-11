/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include <cmath>
#include <cstdint>
#include <list>
#include <Inventor/SbSphere.h>
#include <Inventor/SbString.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoToVRML2Action.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/fields/SoMFNode.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/VRMLnodes/SoVRMLAppearance.h>
#include <Inventor/VRMLnodes/SoVRMLGroup.h>
#include <Inventor/VRMLnodes/SoVRMLIndexedFaceSet.h>
#include <Inventor/VRMLnodes/SoVRMLMaterial.h>
#include <Inventor/VRMLnodes/SoVRMLNormal.h>
#include <Inventor/VRMLnodes/SoVRMLParent.h>
#include <Inventor/VRMLnodes/SoVRMLShape.h>
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>
#include <sstream>
#include <utility>
#include <vector>


#include <Base/FileInfo.h>
#include <Base/Persistence.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <zipios++/gzipoutputstream.h>

#include "SoFCDB.h"
#include "Camera.h"
#include "Flag.h"
#include "Inventor/Draggers/SoTransformDragger.h"
#include "Inventor/Draggers/SoLinearDragger.h"
#include "Inventor/Draggers/SoLinearDraggerGeometry.h"
#include "Inventor/Draggers/SoRotationDragger.h"
#include "Inventor/Draggers/SoRotationDraggerGeometry.h"
#include "Inventor/Draggers/Gizmo.h"
#include "Navigation/GestureNavigationStyle.h"
#include "Navigation/NavigationStyle.h"
#include "Navigation/SiemensNXNavigationStyle.h"
#include "SelectionObject.h"
#include "SoDevicePixelRatioElement.h"
#include "SoFCColorBar.h"
#include "SoFCColorGradient.h"
#include "SoFCColorLegend.h"
#include "SoFCInteractiveElement.h"
#include "SoFCSelection.h"
#include "SoFCSelectionAction.h"
#include "SoFCUnifiedSelection.h"
#include "SoFCVectorizeSVGAction.h"
#include "SoFCVectorizeU3DAction.h"
#include "SoTextLabel.h"
#include "SoDatumLabel.h"
#include "Inventor/MarkerBitmaps.h"
#include "Inventor/SmSwitchboard.h"
#include "Inventor/So3DAnnotation.h"
#include "Inventor/SoAutoZoomTranslation.h"
#include "Inventor/SoAxisCrossKit.h"
#include "Inventor/SoDrawingGrid.h"
#include "Inventor/SoFCBackgroundGradient.h"
#include "Inventor/SoFCBoundingBox.h"
#include "Inventor/SoNaviCube.h"
#include "Inventor/SoMouseWheelEvent.h"
#include "Inventor/SoFCTransform.h"
#include "Inventor/SoToggleSwitch.h"
#include "propertyeditor/PropertyItem.h"
#include "ArcEngine.h"

#include <Inventor/SoFCPlacementIndicatorKit.h>


using namespace Gui;
using namespace Gui::Inventor;
using namespace Gui::PropertyEditor;

static SbBool init_done = false;
static SoGroup* storage = nullptr;

SbBool Gui::SoFCDB::isInitialized()
{
    return init_done;
}

void Gui::SoFCDB::init()
{
    SoInteraction ::init();
    SoDevicePixelRatioElement ::initClass();
    SoGLRenderActionElement ::initClass();
    SoFCInteractiveElement ::initClass();
    SoGLWidgetElement ::initClass();
    SoFCColorBarBase ::initClass();
    SoFCColorBar ::initClass();
    SoFCColorLegend ::initClass();
    SoFCColorGradient ::initClass();
    SoFCBackgroundGradient ::initClass();
    SoFCBoundingBox ::initClass();
    SoFCSelection ::initClass();
    SoFCUnifiedSelection ::initClass();
    SoFCPreselectionAction ::initClass();
    SoFCSelectionAction ::initClass();
    SoFCDocumentAction ::initClass();
    SoGLWidgetNode ::initClass();
    SoGLVBOActivatedElement ::initClass();
    SoFCEnableSelectionAction ::initClass();
    SoFCEnablePreselectionAction ::initClass();
    SoFCSelectionColorAction ::initClass();
    SoFCHighlightColorAction ::initClass();
    SoFCDocumentObjectAction ::initClass();
    SoGLSelectAction ::initClass();
    SoVisibleFaceAction ::initClass();
    SoUpdateVBOAction ::initClass();
    SoBoxSelectionRenderAction ::initClass();
    SoFCVectorizeSVGAction ::initClass();
    SoFCVectorizeU3DAction ::initClass();
    SoHighlightElementAction ::initClass();
    SoSelectionElementAction ::initClass();
    SoVRMLAction ::initClass();
    SoSkipBoundingGroup ::initClass();
    SoTextLabel ::initClass();
    SoDatumLabel ::initClass();
    SoColorBarLabel ::initClass();
    SoStringLabel ::initClass();
    SoFrameLabel ::initClass();
    TranslateManip ::initClass();
    SoShapeScale ::initClass();
    SoAxisCrossKit ::initClass();
    SoRegPoint ::initClass();
    SoDrawingGrid ::initClass();
    SoNaviCube ::initClass();
    SoFCTransform ::initClass();
    SoAutoZoomTranslation ::initClass();
    MarkerBitmaps ::initClass();
    SoTransformDragger ::initClass();
    SoLinearGeometryKit ::initClass();
    SoArrowGeometry ::initClass();
    SoLinearGeometryBaseKit ::initClass();
    SoArrowBase ::initClass();
    SoRotatorGeometryKit ::initClass();
    SoRotatorGeometry ::initClass();
    SoRotatorGeometry2 ::initClass();
    SoRotatorArrow ::initClass();
    SoRotatorGeometryBaseKit ::initClass();
    SoRotatorBase ::initClass();
    SoToggleSwitch ::initClass();
    GizmoContainer ::initClass();
    SmSwitchboard ::initClass();
    SoFCSeparator ::initClass();
    SoFCSelectionRoot ::initClass();
    SoFCPathAnnotation ::initClass();
    SoMouseWheelEvent ::initClass();
    So3DAnnotation ::initClass();
    SoDelayedAnnotationsElement ::initClass();
    SoFCPlacementIndicatorKit ::initClass();

    PropertyItem ::init();
    PropertySeparatorItem ::init();
    PropertyStringItem ::init();
    PropertyFontItem ::init();
    PropertyIntegerItem ::init();
    PropertyIntegerConstraintItem ::init();
    PropertyFloatItem ::init();
    PropertyUnitItem ::init();
    PropertyFloatConstraintItem ::init();
    PropertyPrecisionItem ::init();
    PropertyUnitConstraintItem ::init();
    PropertyAngleItem ::init();
    PropertyBoolItem ::init();
    PropertyVectorItem ::init();
    PropertyVectorListItem ::init();
    PropertyVectorDistanceItem ::init();
    PropertyPositionItem ::init();
    PropertyDirectionItem ::init();
    PropertyMatrixItem ::init();
    PropertyPlacementItem ::init();
    PropertyRotationItem ::init();
    PropertyEnumItem ::init();
    PropertyStringListItem ::init();
    PropertyFloatListItem ::init();
    PropertyIntegerListItem ::init();
    PropertyColorItem ::init();
    PropertyMaterialItem ::init();
    PropertyMaterialListItem ::init();
    PropertyFileItem ::init();
    PropertyPathItem ::init();
    PropertyTransientFileItem ::init();
    PropertyLinkItem ::init();
    PropertyLinkListItem ::init();

    NavigationStyle ::init();
    UserNavigationStyle ::init();
    NavigationStateChart ::init();
    BlenderNavigationStyle ::init();
    CADNavigationStyle ::init();
    GestureNavigationStyle ::init();
    MayaGestureNavigationStyle ::init();
    OpenCascadeNavigationStyle ::init();
    InventorNavigationStyle ::init();
    OpenSCADNavigationStyle ::init();
    RevitNavigationStyle ::init();
    SiemensNXNavigationStyle ::init();
    SolidWorksNavigationStyle ::init();
    TinkerCADNavigationStyle ::init();
    TouchpadNavigationStyle ::init();

    GLGraphicsItem ::init();
    GLFlagWindow ::init();

    SelectionObject ::init();

    ArcEngine ::initClass();

    qRegisterMetaType<Base::Vector3f>("Base::Vector3f");
    qRegisterMetaType<Base::Vector3d>("Base::Vector3d");
    qRegisterMetaType<Base::Quantity>("Base::Quantity");
    qRegisterMetaType<QList<Base::Quantity>>("Base::QuantityList");
    init_done = true;

    assert(!storage);
    storage = new SoGroup();
    storage->ref();
}

void Gui::SoFCDB::finish()
{
    // Coin doesn't provide a mechanism to free static members of own data types.
    // Hence, we need to define a static method e.g. 'finish()' for all new types
    // to invoke the private member function 'atexit_cleanup()'.
    SoFCColorBarBase ::finish();
    SoFCColorBar ::finish();
    SoFCColorLegend ::finish();
    SoFCColorGradient ::finish();
    SoFCBackgroundGradient ::finish();
    SoFCBoundingBox ::finish();
    SoFCSelection ::finish();
    SoFCPreselectionAction ::finish();
    SoFCSelectionAction ::finish();
    SoFCDocumentAction ::finish();
    SoFCDocumentObjectAction ::finish();
    SoFCEnableSelectionAction ::finish();
    SoFCEnablePreselectionAction ::finish();
    SoFCSelectionColorAction ::finish();
    SoUpdateVBOAction ::finish();
    SoFCHighlightColorAction ::finish();
    SoFCSeparator ::finish();
    SoFCSelectionRoot ::finish();
    SoFCPathAnnotation ::finish();

    storage->unref();
    storage = nullptr;
}

// buffer acrobatics for inventor ****************************************************

namespace
{
static std::vector<char> static_buffer;

static void* buffer_realloc(void* /*bufptr*/, std::size_t size)
{
    static_buffer.resize(size);
    return static_buffer.data();
}
}  // namespace

const std::string& Gui::SoFCDB::writeNodesToString(SoNode* root)
{
    SoOutput out;
    static_buffer.resize(1024);
    out.setBuffer(static_buffer.data(), static_buffer.size(), buffer_realloc);
    if (root && root->getTypeId().isDerivedFrom(SoVRMLParent::getClassTypeId())) {
        out.setHeaderString("#VRML V2.0 utf8");
    }

    SoWriteAction wa(&out);
    wa.apply(root);

    static std::string cReturnString;
    cReturnString = static_buffer.data();
    return cReturnString;
}

struct IndexRange
{
    int start;
    int end;
};

struct FaceSubset
{
    float transparency;
    std::vector<int> faces;
};

struct ShapeReplacement
{
    SoVRMLParent* parent;
    SoVRMLShape* shape;
    SoVRMLGroup* group;
};

bool isSameTransparency(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 1e-6F;
}

std::vector<IndexRange> getIndexFaceRanges(const SoMFInt32& index)
{
    std::vector<IndexRange> ranges;
    int start = 0;
    int count = index.getNum();

    for (int i = 0; i < count; i++) {
        if (index[i] < 0) {
            if (i > start) {
                ranges.push_back({start, i});
            }
            start = i + 1;
        }
    }
    if (start < count) {
        ranges.push_back({start, count});
    }

    return ranges;
}

void setInt32Values(SoMFInt32& field, const std::vector<int32_t>& values)
{
    field.setNum(static_cast<int>(values.size()));
    if (!values.empty()) {
        field.setValues(0, static_cast<int>(values.size()), values.data());
    }
}

void copyPerVertexIndexField(
    const SoMFInt32& source,
    SoMFInt32& target,
    const std::vector<int>& faces
)
{
    const auto ranges = getIndexFaceRanges(source);
    if (ranges.empty()) {
        target.setNum(0);
        return;
    }

    std::vector<int32_t> values;
    for (int face : faces) {
        if (face < 0 || face >= static_cast<int>(ranges.size())) {
            continue;
        }
        const auto& range = ranges[face];
        for (int i = range.start; i < range.end; i++) {
            values.push_back(source[i]);
        }
        values.push_back(-1);
    }

    setInt32Values(target, values);
}

void copyPerFaceIndexField(
    const SoMFInt32& source,
    SoMFInt32& target,
    const std::vector<int>& faces
)
{
    std::vector<int32_t> values;
    values.reserve(faces.size());
    for (int face : faces) {
        if (face >= 0 && face < source.getNum()) {
            values.push_back(source[face]);
        }
    }

    setInt32Values(target, values);
}

void setImplicitPerFaceIndexField(SoMFInt32& target, const std::vector<int>& faces)
{
    std::vector<int32_t> values;
    values.reserve(faces.size());
    for (int face : faces) {
        values.push_back(face);
    }

    setInt32Values(target, values);
}

void copyIndexedFaceSetSubset(
    const SoVRMLIndexedFaceSet* source,
    SoVRMLIndexedFaceSet* target,
    const std::vector<int>& faces
)
{
    copyPerVertexIndexField(source->coordIndex, target->coordIndex, faces);

    if (source->colorIndex.getNum() > 0) {
        if (source->colorPerVertex.getValue()) {
            copyPerVertexIndexField(source->colorIndex, target->colorIndex, faces);
        }
        else {
            copyPerFaceIndexField(source->colorIndex, target->colorIndex, faces);
        }
    }
    else if (!source->colorPerVertex.getValue() && source->color.getValue()) {
        setImplicitPerFaceIndexField(target->colorIndex, faces);
    }
    else {
        target->colorIndex.setNum(0);
    }

    if (source->normalIndex.getNum() > 0) {
        if (source->normalPerVertex.getValue()) {
            copyPerVertexIndexField(source->normalIndex, target->normalIndex, faces);
        }
        else {
            copyPerFaceIndexField(source->normalIndex, target->normalIndex, faces);
        }
    }
    else if (!source->normalPerVertex.getValue() && source->normal.getValue()) {
        setImplicitPerFaceIndexField(target->normalIndex, faces);
    }
    else {
        target->normalIndex.setNum(0);
    }

    if (source->texCoordIndex.getNum() > 0) {
        copyPerVertexIndexField(source->texCoordIndex, target->texCoordIndex, faces);
    }
    else {
        target->texCoordIndex.setNum(0);
    }
}

SoVRMLAppearance* copyAppearanceWithTransparency(SoNode* sourceAppearance, float transparency)
{
    SoVRMLAppearance* appearance = nullptr;
    if (sourceAppearance
        && sourceAppearance->getTypeId().isDerivedFrom(SoVRMLAppearance::getClassTypeId())) {
        appearance = static_cast<SoVRMLAppearance*>(sourceAppearance->copy(FALSE));
    }
    else {
        appearance = new SoVRMLAppearance;
    }

    SoNode* sourceMaterial = appearance->material.getValue();
    SoVRMLMaterial* material = nullptr;
    if (sourceMaterial
        && sourceMaterial->getTypeId().isDerivedFrom(SoVRMLMaterial::getClassTypeId())) {
        material = static_cast<SoVRMLMaterial*>(sourceMaterial->copy(FALSE));
    }
    else {
        material = new SoVRMLMaterial;
    }

    material->transparency = transparency;
    appearance->material.setValue(material);
    return appearance;
}

std::vector<FaceSubset> buildTransparencySubsets(const std::vector<float>& transparencies)
{
    std::vector<FaceSubset> subsets;
    for (int face = 0; face < static_cast<int>(transparencies.size()); face++) {
        float transparency = transparencies[face];
        if (subsets.empty() || !isSameTransparency(subsets.back().transparency, transparency)) {
            subsets.push_back({transparency, {}});
        }
        subsets.back().faces.push_back(face);
    }

    return subsets;
}

SoVRMLShape* copyShapeSubset(
    const SoVRMLShape* sourceShape,
    const SoVRMLIndexedFaceSet* sourceGeometry,
    const FaceSubset& subset
)
{
    auto shape = new SoVRMLShape;
    shape->copyFieldValues(sourceShape);
    shape->appearance.setValue(
        copyAppearanceWithTransparency(sourceShape->appearance.getValue(), subset.transparency)
    );

    auto geometry = static_cast<SoVRMLIndexedFaceSet*>(sourceGeometry->copy(FALSE));
    copyIndexedFaceSetSubset(sourceGeometry, geometry, subset.faces);
    shape->geometry.setValue(geometry);
    return shape;
}

SoVRMLGroup* splitShapeByTransparency(
    const SoVRMLShape* shape,
    const SoVRMLIndexedFaceSet* geometry,
    const std::vector<float>& transparencies
)
{
    const auto subsets = buildTransparencySubsets(transparencies);
    if (subsets.size() < 2) {
        return nullptr;
    }

    auto group = new SoVRMLGroup;
    for (const auto& subset : subsets) {
        group->addChild(copyShapeSubset(shape, geometry, subset));
    }

    return group;
}

void applyFaceTransparencyToVRML(
    SoVRMLGroup* vrmlRoot,
    const std::list<std::vector<float>>& transparencySets
)
{
    if (transparencySets.empty()) {
        return;
    }

    SoSearchAction sa;
    sa.setType(SoVRMLShape::getClassTypeId());
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(true);
    sa.apply(vrmlRoot);

    auto transparencySet = transparencySets.begin();
    std::vector<ShapeReplacement> replacements;
    SoPathList& paths = sa.getPaths();
    for (int i = 0; i < paths.getLength() && transparencySet != transparencySets.end(); i++) {
        auto path = static_cast<SoFullPath*>(paths[i]);
        if (path->getLength() < 2) {
            continue;
        }

        auto shape = static_cast<SoVRMLShape*>(path->getTail());
        SoNode* geometryNode = shape->geometry.getValue();
        if (!geometryNode
            || !geometryNode->getTypeId().isDerivedFrom(SoVRMLIndexedFaceSet::getClassTypeId())) {
            continue;
        }

        auto geometry = static_cast<SoVRMLIndexedFaceSet*>(geometryNode);
        int faceCount = static_cast<int>(getIndexFaceRanges(geometry->coordIndex).size());
        if (faceCount != static_cast<int>(transparencySet->size())) {
            continue;
        }

        SoNode* parentNode = path->getNodeFromTail(1);
        if (!parentNode
            || !parentNode->getTypeId().isDerivedFrom(SoVRMLParent::getClassTypeId())) {
            ++transparencySet;
            continue;
        }

        SoVRMLGroup* group = splitShapeByTransparency(shape, geometry, *transparencySet);
        if (group) {
            replacements.push_back({static_cast<SoVRMLParent*>(parentNode), shape, group});
        }
        ++transparencySet;
    }
    sa.reset();

    for (const auto& replacement : replacements) {
        replacement.parent->replaceChild(replacement.shape, replacement.group);
    }
}

SoNode* replaceSwitches(SoNodeList* children, SoGroup* parent)
{
    if (!children) {
        return parent;
    }

    for (int i = 0; i < children->getLength(); i++) {
        SoNode* node = (*children)[i];
        if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
            if (node->getTypeId().isDerivedFrom(SoSwitch::getClassTypeId())) {
                auto group = static_cast<SoSwitch*>(node);
                int which = group->whichChild.getValue();
                if (which == SO_SWITCH_NONE) {
                    continue;
                }
                auto newParent = new SoGroup();
                SoNodeList c;
                if (which >= 0) {
                    c.append(group->getChild(which));
                }
                else {
                    // SO_SWITCH_INHERIT or SO_SWITCH_ALL
                    for (int i = 0; i < group->getNumChildren(); i++) {
                        c.append(group->getChild(i));
                    }
                }

                replaceSwitches(&c, newParent);
                parent->addChild(newParent);
            }
            else {
                auto newParent = static_cast<SoGroup*>(node->getTypeId().createInstance());
                replaceSwitches(node->getChildren(), newParent);
                parent->addChild(newParent);
            }
        }
        else {
            parent->addChild(node);
        }
    }

    return parent;
}

SoNode* replaceSwitchesInSceneGraph(SoNode* node)
{
    if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        return replaceSwitches(node->getChildren(), new SoSeparator);
    }

    return node;
}

SoNode* Gui::SoFCDB::replaceSwitches(SoNode* node)
{
    return replaceSwitchesInSceneGraph(node);
}

void Gui::SoFCDB::writeToVRML(SoNode* node, std::string& buffer)
{
    SoNode* noSwitches = replaceSwitchesInSceneGraph(node);
    noSwitches->ref();
    SoVRMLAction vrml2;
    vrml2.setOverrideMode(true);
    vrml2.apply(noSwitches);
    SoToVRML2Action tovrml2;
    tovrml2.apply(noSwitches);
    SoVRMLGroup* vrmlRoot = tovrml2.getVRML2SceneGraph();

    applyFaceTransparencyToVRML(vrmlRoot, vrml2.getFaceTransparencySets());
    vrmlRoot->setInstancePrefix(SbString("o"));
    vrmlRoot->ref();
    buffer = SoFCDB::writeNodesToString(vrmlRoot);
    vrmlRoot->unref();  // release the memory as soon as possible

    // restore old settings
    vrml2.setOverrideMode(false);
    vrml2.apply(noSwitches);
    noSwitches->unref();
}

bool Gui::SoFCDB::writeToVRML(SoNode* node, const char* filename, bool binary)
{
    std::string buffer;
    writeToVRML(node, buffer);

    Base::FileInfo fi(filename);
    if (binary) {
        // We want to write compressed VRML but Coin 2.4.3 doesn't do it even though
        // SoOutput::getAvailableCompressionMethods() delivers a string list that
        // contains 'GZIP'. setCompression() was called directly after opening the file,
        // returned true and no error message appeared but anyway it didn't work.
        // Strange is that reading GZIPped VRML files works.
        // So, we do the compression on our own.
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        zipios::GZIPOutputStream gzip(str);

        if (gzip) {
            gzip << buffer;
            gzip.close();
            return true;
        }
    }
    else {
        Base::ofstream str(fi, std::ios::out);

        if (str) {
            str << buffer;
            str.close();
            return true;
        }
    }

    return false;
}

bool Gui::SoFCDB::writeToX3D(SoNode* node, const char* filename, bool binary)
{
    std::string buffer;
    writeToX3D(node, false, buffer);

    Base::FileInfo fi(filename);
    if (binary) {
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        zipios::GZIPOutputStream gzip(str);

        if (gzip) {
            gzip << buffer;
            gzip.close();
            return true;
        }
    }
    else {
        Base::ofstream str(fi, std::ios::out);

        if (str) {
            str << buffer;
            str.close();
            return true;
        }
    }

    return false;
}

bool Gui::SoFCDB::writeToX3D(SoNode* node, bool exportViewpoints, std::string& buffer)
{
    SoNode* noSwitches = replaceSwitchesInSceneGraph(node);
    noSwitches->ref();
    SoVRMLAction vrml2;
    vrml2.setOverrideMode(true);
    vrml2.apply(noSwitches);
    SoToVRML2Action tovrml2;
    tovrml2.apply(noSwitches);
    SoVRMLGroup* vrmlRoot = tovrml2.getVRML2SceneGraph();

    vrmlRoot->setInstancePrefix(SbString("o"));
    vrmlRoot->ref();

    // Search for SoVRMLIndexedFaceSet nodes and set creaseAngle to 0.5
    {
        SoSearchAction sa;
        sa.setType(SoVRMLShape::getClassTypeId());
        sa.setInterest(SoSearchAction::ALL);
        sa.setSearchingAll(true);
        sa.apply(vrmlRoot);
        SoPathList& paths = sa.getPaths();
        for (int i = 0; i < paths.getLength(); i++) {
            SoPath* path = paths[i];
            auto shape = static_cast<SoVRMLShape*>(path->getTail());
            SoNode* geom = shape->geometry.getValue();
            if (geom && geom->getTypeId() == SoVRMLIndexedFaceSet::getClassTypeId()) {
                SoNode* norm = static_cast<SoVRMLIndexedFaceSet*>(geom)->normal.getValue();
                if (norm && norm->getTypeId() == SoVRMLNormal::getClassTypeId()) {
                    // if empty then nullify the normal field node
                    if (static_cast<SoVRMLNormal*>(norm)->vector.getNum() == 0) {
                        static_cast<SoVRMLIndexedFaceSet*>(geom)->normal.setValue(nullptr);
                    }
                }
                else {
                    static_cast<SoVRMLIndexedFaceSet*>(geom)->creaseAngle.setValue(0.5f);
                }
            }
        }
        sa.reset();  // clear the internal cache
    }

    std::stringstream out;
    writeX3D(vrmlRoot, exportViewpoints, out);
    buffer = out.str();

    vrmlRoot->unref();  // release the memory as soon as possible

    // restore old settings
    vrml2.setOverrideMode(false);
    vrml2.apply(noSwitches);
    noSwitches->unref();

    return true;
}


void Gui::SoFCDB::writeX3DFields(
    SoNode* node,
    std::map<SoNode*, std::string>& nodeMap,
    bool isRoot,
    int& numDEF,
    int spaces,
    std::ostream& out
)
{
    // remove the VRML prefix from the type name
    std::string type(node->getTypeId().getName().getString());
    type = type.substr(4);

    out << Base::blanks(spaces) << "<" << type;
    if (node->getRefCount() > 1 && !isRoot) {
        SbName name = node->getName();
        std::stringstream str;
        if (name.getLength() == 0) {
            str << "o" << numDEF++;
        }
        else {
            str << name.getString();
        }

        nodeMap[node] = str.str();
        std::string escapedName = Base::Persistence::encodeAttribute(str.str());
        out << " DEF=\"" << escapedName << "\"";
    }

    const SoFieldData* fielddata = node->getFieldData();
    if (fielddata) {
        int numFieldNodes = 0;

        // process non-SoSFNode and non-SoMFNode fields
        for (int i = 0; i < fielddata->getNumFields(); i++) {
            SoField* field = fielddata->getField(node, i);
            if (!field->isDefault()) {
                if (!field->isOfType(SoSFNode::getClassTypeId())
                    && !field->isOfType(SoMFNode::getClassTypeId())) {
                    SbString value;
                    field->get(value);
                    QByteArray ba(value.getString(), value.getLength());
                    ba.replace('\n', ' ');
                    if (field->isOfType(SoMField::getClassTypeId())) {
                        ba.replace('[', ' ');
                        ba.replace(']', ' ');
                        ba = ba.simplified();
                    }

                    // escape XML special characters in attribute value
                    std::string escapedValue = Base::Persistence::encodeAttribute(
                        std::string(ba.data())
                    );

                    out << '\n'
                        << Base::blanks(spaces + 2) << fielddata->getFieldName(i).getString()
                        << "=\"" << escapedValue << "\" ";
                }
                else {
                    numFieldNodes++;
                }
            }
        }

        if (numFieldNodes > 0) {
            out << ">\n";
        }
        else {
            out << "/>\n";
        }

        // process SoSFNode or SoMFNode fields
        for (int i = 0; i < fielddata->getNumFields(); i++) {
            SoField* field = fielddata->getField(node, i);
            if (!field->isDefault()) {
                if (field->isOfType(SoSFNode::getClassTypeId())) {
                    auto sfNode = static_cast<SoSFNode*>(field);
                    writeX3DChild(sfNode->getValue(), nodeMap, numDEF, spaces + 2, out);
                }
                else if (field->isOfType(SoMFNode::getClassTypeId())) {
                    auto mfNode = static_cast<SoMFNode*>(field);
                    for (int j = 0; j < mfNode->getNum(); j++) {
                        writeX3DChild(mfNode->getNode(j), nodeMap, numDEF, spaces + 2, out);
                    }
                }
            }
        }

        if (numFieldNodes > 0) {
            out << Base::blanks(spaces) << "</" << type << ">\n";
        }
    }
}

void Gui::SoFCDB::writeX3DChild(
    SoNode* node,
    std::map<SoNode*, std::string>& nodeMap,
    int& numDEF,
    int spaces,
    std::ostream& out
)
{
    if (!node) {
        return;
    }

    // check if the node is already used
    auto mapIt = nodeMap.find(node);
    if (mapIt == nodeMap.end()) {
        writeX3DFields(node, nodeMap, false, numDEF, spaces, out);
    }
    else {
        // remove the VRML prefix from the type name
        std::string sftype(node->getTypeId().getName().getString());
        sftype = sftype.substr(4);
        std::string escapedRef = Base::Persistence::encodeAttribute(mapIt->second);
        out << Base::blanks(spaces) << "<" << sftype << " USE=\"" << escapedRef << "\" />\n";
    }
}

void Gui::SoFCDB::writeX3D(SoVRMLGroup* node, bool exportViewpoints, std::ostream& out)
{
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D 3.3//EN\" "
           "\"http://www.web3d.org/specifications/x3d-3.3.dtd\">\n";
    out << "<X3D profile=\"Immersive\" version=\"3.3\" "
           "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema-instance\" "
           "xsd:noNamespaceSchemaLocation=\"http://www.web3d.org/specifications/x3d-3.3.xsd\">\n";
    out << "  <head>\n"
           "    <meta name=\"generator\" content=\"FreeCAD\"/>\n"
           "    <meta name=\"author\" content=\"\"/>\n"
           "    <meta name=\"company\" content=\"\"/>\n"
           "  </head>\n";

    std::map<SoNode*, std::string> nodeMap;
    out << "  <Scene>\n";

    // compute a sensible view point
    SoGetBoundingBoxAction bboxAction(SbViewportRegion(1280, 1024));
    bboxAction.apply(node);
    SbBox3f bbox = bboxAction.getBoundingBox();
    SbSphere bs;
    bs.circumscribe(bbox);
    const SbVec3f& cnt = bs.getCenter();
    float dist = 2.4f * bs.getRadius();
    float dist3 = 0.577350f * dist;  // sqrt(1/3) * dist

    if (exportViewpoints) {
        auto viewpoint =
            [&out](const char* text, const SbVec3f& cnt, const SbVec3f& pos, const SbRotation& rot) {
                SbVec3f axis;
                float angle;
                rot.getValue(axis, angle);
                out << "    <Viewpoint id=\"" << text << "\" centerOfRotation=\"" << cnt[0] << " "
                    << cnt[1] << " " << cnt[2] << "\" position=\"" << pos[0] << " " << pos[1] << " "
                    << pos[2] << "\" orientation=\"" << axis[0] << " " << axis[1] << " " << axis[2]
                    << " " << angle << R"(" description="camera" fieldOfView="0.9">)"
                    << "</Viewpoint>\n";
            };

        viewpoint(
            "Iso",
            cnt,
            SbVec3f(cnt[0] + dist3, cnt[1] - dist3, cnt[2] + dist3),
            Camera::rotation(Camera::Isometric)
        );
        viewpoint("Front", cnt, SbVec3f(cnt[0], cnt[1] - dist, cnt[2]), Camera::rotation(Camera::Front));
        viewpoint("Back", cnt, SbVec3f(cnt[0], cnt[1] + dist, cnt[2]), Camera::rotation(Camera::Rear));
        viewpoint("Right", cnt, SbVec3f(cnt[0] + dist, cnt[1], cnt[2]), Camera::rotation(Camera::Right));
        viewpoint("Left", cnt, SbVec3f(cnt[0] - dist, cnt[1], cnt[2]), Camera::rotation(Camera::Left));
        viewpoint("Top", cnt, SbVec3f(cnt[0], cnt[1], cnt[2] + dist), Camera::rotation(Camera::Top));
        viewpoint("Bottom", cnt, SbVec3f(cnt[0], cnt[1], cnt[2] - dist), Camera::rotation(Camera::Bottom));
    }

    int numDEF = 0;
    writeX3DFields(node, nodeMap, true, numDEF, 4, out);
    out << "  </Scene>\n";
    out << "</X3D>\n";
}

bool Gui::SoFCDB::writeToX3DOM(SoNode* node, std::string& buffer)
{
    std::string x3d;
    if (!writeToX3D(node, true, x3d)) {
        return false;
    }

    // remove the first two lines from the x3d output as this duplicates
    // the xml and doctype header
    std::size_t pos = x3d.find('\n');
    pos = x3d.find('\n', pos + 1);
    x3d = x3d.erase(0, pos + 1);

    std::stringstream out;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
           "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n";
    out << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
        << "  <head>\n"
        << "    <meta charset=\"utf-8\"/>\n"
        << "    <title>FreeCAD X3DOM Export</title>\n"
        << "    <script src=\"http://www.x3dom.org/download/x3dom.js\"> </script>\n"
        << "    <link rel=\"stylesheet\" type=\"text/css\" "
           "href=\"http://www.x3dom.org/download/x3dom.css\"/>\n"
        << "  </head>\n"
        << "  <body>\n"
        << "    <div>\n";

    auto onclick = [&out](const char* text) {
        out << "      <button onclick=\"document.getElementById('" << text
            << "').setAttribute('set_bind','true');\">" << text << "</button>\n";
    };

    onclick("Iso");
    onclick("Front");
    onclick("Back");
    onclick("Right");
    onclick("Left");
    onclick("Top");
    onclick("Bottom");

    out << "    </div>\n" << x3d;

    out << "  </body>\n"
        << "</html>\n";

    buffer = out.str();

    return true;
}

bool Gui::SoFCDB::writeToFile(SoNode* node, const char* filename, bool binary)
{
    bool ret = false;
    Base::FileInfo fi(filename);

    // Write VRML V2.0
    if (fi.hasExtension({"wrl", "vrml", "wrz"})) {
        // If 'wrz' is set then force compression
        if (fi.hasExtension("wrz")) {
            binary = true;
        }

        ret = SoFCDB::writeToVRML(node, filename, binary);
    }
    else if (fi.hasExtension({"x3d", "x3dz"})) {
        // If 'x3dz' is set then force compression
        if (fi.hasExtension("x3dz")) {
            binary = true;
        }

        ret = SoFCDB::writeToX3D(node, filename, binary);
    }
    else if (fi.hasExtension("xhtml")) {
        std::string buffer;
        if (SoFCDB::writeToX3DOM(node, buffer)) {
            Base::ofstream str(Base::FileInfo(filename), std::ios::out);

            if (str) {
                str << buffer;
                str.close();
                ret = true;
            }
        }
    }
    else if (fi.hasExtension("iv")) {
        // Write Inventor in ASCII
        std::string buffer = SoFCDB::writeNodesToString(node);
        Base::ofstream str(Base::FileInfo(filename), std::ios::out);

        if (str) {
            str << buffer;
            str.close();
            ret = true;
        }
    }

    return ret;
}

SoGroup* Gui::SoFCDB::getStorage()
{
    assert(storage);  // call init first.
    return storage;
}
