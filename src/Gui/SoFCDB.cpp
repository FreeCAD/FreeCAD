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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/actions/SoToVRML2Action.h>
# include <Inventor/VRMLnodes/SoVRMLGroup.h>
# include <Inventor/VRMLnodes/SoVRMLParent.h>
# include <Inventor/SbString.h>
# include <Inventor/nodes/SoGroup.h>
#endif

#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <zipios++/gzipoutputstream.h>

#include "SoFCDB.h"
#include "SoFCColorBar.h"
#include "SoFCColorLegend.h"
#include "SoFCColorGradient.h"
#include "SoFCSelection.h"
#include "SoFCBackgroundGradient.h"
#include "SoFCBoundingBox.h"
#include "SoFCSelection.h"
#include "SoFCUnifiedSelection.h"
#include "SoFCSelectionAction.h"
#include "SoFCInteractiveElement.h"
#include "SoFCUnifiedSelection.h"
#include "SoFCVectorizeSVGAction.h"
#include "SoFCVectorizeU3DAction.h"
#include "SoAxisCrossKit.h"
#include "SoTextLabel.h"
#include "SoNavigationDragger.h"
#include "Inventor/SoDrawingGrid.h"
#include "Inventor/SoAutoZoomTranslation.h"
#include "Inventor/MarkerBitmaps.h"
#include "Inventor/SmSwitchboard.h"
#include "SoFCCSysDragger.h"

#include "propertyeditor/PropertyItem.h"
#include "NavigationStyle.h"
#include "GestureNavigationStyle.h"
#include "Flag.h"
#include "SelectionObject.h"

using namespace Gui;
using namespace Gui::Inventor;
using namespace Gui::PropertyEditor;

static SbBool init_done = false;
static SoGroup *storage = nullptr;

SbBool Gui::SoFCDB::isInitialized(void)
{
    return init_done;
}

void Gui::SoFCDB::init()
{
    SoInteraction                   ::init();
    RotTransDragger                 ::initClass();
    SoGLRenderActionElement         ::initClass();
    SoFCInteractiveElement          ::initClass();
    SoGLWidgetElement               ::initClass();
    SoFCColorBarBase                ::initClass();
    SoFCColorBar                    ::initClass();
    SoFCColorLegend                 ::initClass();
    SoFCColorGradient               ::initClass();
    SoFCBackgroundGradient          ::initClass();
    SoFCBoundingBox                 ::initClass();
    SoFCSelection                   ::initClass();
    SoFCUnifiedSelection            ::initClass();
    SoFCHighlightAction             ::initClass();
    SoFCSelectionAction             ::initClass();
    SoFCDocumentAction              ::initClass();
    SoGLWidgetNode                  ::initClass();
    SoGLVBOActivatedElement         ::initClass();
    SoFCEnableSelectionAction       ::initClass();
    SoFCEnableHighlightAction       ::initClass();
    SoFCSelectionColorAction        ::initClass();
    SoFCHighlightColorAction        ::initClass();
    SoFCDocumentObjectAction        ::initClass();
    SoGLSelectAction                ::initClass();
    SoVisibleFaceAction             ::initClass();
    SoUpdateVBOAction               ::initClass();
    SoBoxSelectionRenderAction      ::initClass();
    SoFCVectorizeSVGAction          ::initClass();
    SoFCVectorizeU3DAction          ::initClass();
    SoHighlightElementAction        ::initClass();
    SoSelectionElementAction        ::initClass();
    SoVRMLAction                    ::initClass();
    SoSkipBoundingGroup             ::initClass();
    SoTextLabel                     ::initClass();
    SoStringLabel                   ::initClass();
    SoFrameLabel                    ::initClass();
    TranslateManip                  ::initClass();
    SoShapeScale                    ::initClass();
    SoAxisCrossKit                  ::initClass();
    SoRegPoint                      ::initClass();
    SoDrawingGrid                   ::initClass();
    SoAutoZoomTranslation           ::initClass();
    MarkerBitmaps                   ::initClass();
    SoFCCSysDragger                 ::initClass();
    SmSwitchboard                   ::initClass();

    PropertyItem                    ::init();
    PropertySeparatorItem           ::init();
    PropertyStringItem              ::init();
    PropertyFontItem                ::init();
    PropertyIntegerItem             ::init();
    PropertyIntegerConstraintItem   ::init();
    PropertyFloatItem               ::init();
    PropertyUnitItem                ::init();
    PropertyFloatConstraintItem     ::init();
    PropertyPrecisionItem           ::init();
    PropertyUnitConstraintItem      ::init();
    PropertyAngleItem               ::init();
    PropertyBoolItem                ::init();
    PropertyVectorItem              ::init();
    PropertyVectorDistanceItem      ::init();
    PropertyPositionItem            ::init();
    PropertyDirectionItem           ::init();
    PropertyMatrixItem              ::init();
    PropertyPlacementItem           ::init();
    PropertyEnumItem                ::init();
    PropertyStringListItem          ::init();
    PropertyFloatListItem           ::init();
    PropertyIntegerListItem         ::init();
    PropertyColorItem               ::init();
    PropertyMaterialItem            ::init();
    PropertyMaterialListItem        ::init();
    PropertyFileItem                ::init();
    PropertyPathItem                ::init();
    PropertyTransientFileItem       ::init();
    PropertyLinkItem                ::init();
    PropertyLinkListItem            ::init();

    NavigationStyle                 ::init();
    UserNavigationStyle             ::init();
    InventorNavigationStyle         ::init();
    CADNavigationStyle              ::init();
    RevitNavigationStyle            ::init();
    BlenderNavigationStyle          ::init();
    MayaGestureNavigationStyle      ::init();
    TouchpadNavigationStyle         ::init();
    GestureNavigationStyle          ::init();
    OpenCascadeNavigationStyle      ::init();

    GLGraphicsItem                  ::init();
    GLFlagWindow                    ::init();

    SelectionObject                 ::init();

    qRegisterMetaType<Base::Vector3f>("Base::Vector3f");
    qRegisterMetaType<Base::Vector3d>("Base::Vector3d");
    qRegisterMetaType<Base::Quantity>("Base::Quantity");
    qRegisterMetaType<QList<Base::Quantity> >("Base::QuantityList");
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
    SoFCColorBarBase                ::finish();
    SoFCColorBar                    ::finish();
    SoFCColorLegend                 ::finish();
    SoFCColorGradient               ::finish();
    SoFCBackgroundGradient          ::finish();
    SoFCBoundingBox                 ::finish();
    SoFCSelection                   ::finish();
    SoFCHighlightAction             ::finish();
    SoFCSelectionAction             ::finish();
    SoFCDocumentAction              ::finish();
    SoFCDocumentObjectAction        ::finish();
    SoFCEnableSelectionAction       ::finish();
    SoFCEnableHighlightAction       ::finish();
    SoFCSelectionColorAction        ::finish();
    SoUpdateVBOAction               ::finish();
    SoFCHighlightColorAction        ::finish();
    
    storage->unref();
    storage = nullptr;
}

// buffer acrobatics for inventor ****************************************************
static char * buffer;
static size_t buffer_size = 0;
static std::string cReturnString;

static void *
buffer_realloc(void * bufptr, size_t size)
{
    buffer = (char *)realloc(bufptr, size);
    buffer_size = size;
    return buffer;
}

const std::string& Gui::SoFCDB::writeNodesToString(SoNode * root)
{
    SoOutput out;
    buffer = (char *)malloc(1024);
    buffer_size = 1024;
    out.setBuffer(buffer, buffer_size, buffer_realloc);
    if (root && root->getTypeId().isDerivedFrom(SoVRMLParent::getClassTypeId()))
        out.setHeaderString("#VRML V2.0 utf8");

    SoWriteAction wa(&out);
    wa.apply(root);

    cReturnString = buffer;
    free(buffer);
    return cReturnString;
}

SoNode* replaceSwitches(SoNodeList* children, SoGroup* parent)
{
    if (!children) {
        return parent;
    }

    for (int i=0; i<children->getLength(); i++) {
        SoNode* node = (*children)[i];
        if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
            if (node->getTypeId().isDerivedFrom(SoSwitch::getClassTypeId())) {
                SoSwitch* group = static_cast<SoSwitch*>(node);
                int which = group->whichChild.getValue();
                if (which == SO_SWITCH_NONE)
                    continue;
                SoGroup* newParent = new SoGroup();
                SoNodeList c;
                if (which >= 0) {
                    c.append(group->getChild(which));
                }
                else {
                    // SO_SWITCH_INHERIT or SO_SWITCH_ALL
                    for (int i=0; i<group->getNumChildren(); i++)
                        c.append(group->getChild(i));
                }

                replaceSwitches(&c, newParent);
                parent->addChild(newParent);
            }
            else {
                SoGroup* newParent = static_cast<SoGroup*>(node->getTypeId().createInstance());
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

bool Gui::SoFCDB::writeToVRML(SoNode* node, const char* filename, bool binary)
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
    std::string buffer = SoFCDB::writeNodesToString(vrmlRoot);
    vrmlRoot->unref(); // release the memory as soon as possible

    // restore old settings
    vrml2.setOverrideMode(false);
    vrml2.apply(noSwitches);
    noSwitches->unref();

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

bool Gui::SoFCDB::writeToFile(SoNode* node, const char* filename, bool binary)
{
    bool ret = false;
    Base::FileInfo fi(filename);

    // Write VRML V2.0
    if (fi.hasExtension("wrl") || fi.hasExtension("vrml") || fi.hasExtension("wrz")) {
        // If 'wrz' is set then force compression
        if (fi.hasExtension("wrz"))
            binary = true;

        ret = SoFCDB::writeToVRML(node, filename, binary);
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
  assert(storage); //call init first.
  return storage;
}
