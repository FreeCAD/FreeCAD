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
# include <QDir>
# include <QProcess>
# include <QTemporaryFile>
# include <sstream>
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
#include "SoMouseWheelEvent.h"

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
    SoFCSeparator                   ::initClass();
    SoFCSelectionRoot               ::initClass();
    SoFCPathAnnotation              ::initClass();
    SoMouseWheelEvent               ::initClass();

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
    PropertyVectorListItem          ::init();
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
    SoFCSeparator                   ::finish();
    SoFCSelectionRoot               ::finish();
    SoFCPathAnnotation              ::finish();
    
    storage->unref();
    storage = nullptr;
}

// buffer acrobatics for inventor ****************************************************
static char * static_buffer;
static size_t static_buffer_size = 0;
static std::string cReturnString;

static void *
buffer_realloc(void * bufptr, size_t size)
{
    static_buffer = (char *)realloc(bufptr, size);
    static_buffer_size = size;
    return static_buffer;
}

const std::string& Gui::SoFCDB::writeNodesToString(SoNode * root)
{
    SoOutput out;
    static_buffer = (char *)malloc(1024);
    static_buffer_size = 1024;
    out.setBuffer(static_buffer, static_buffer_size, buffer_realloc);
    if (root && root->getTypeId().isDerivedFrom(SoVRMLParent::getClassTypeId()))
        out.setHeaderString("#VRML V2.0 utf8");

    SoWriteAction wa(&out);
    wa.apply(root);

    cReturnString = static_buffer;
    free(static_buffer);
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
    vrmlRoot->setInstancePrefix(SbString("o"));
    vrmlRoot->ref();
    buffer = SoFCDB::writeNodesToString(vrmlRoot);
    vrmlRoot->unref(); // release the memory as soon as possible

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
    writeToX3D(node, buffer);

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

bool Gui::SoFCDB::writeToX3D(SoNode* node, std::string& buffer)
{
    writeToVRML(node, buffer);
    if (buffer.empty())
        return false;

    QString filename = QDir::tempPath();
    filename += QLatin1String("/sceneXXXXXX.wrl");
    QTemporaryFile wrlFile(filename);
    if (wrlFile.open()) {
        filename = wrlFile.fileName();
        wrlFile.write(buffer.c_str(), buffer.size());
        wrlFile.close();

        QString exe(QLatin1String("tovrmlx3d"));
        QStringList args;
        args << filename << QLatin1String("--encoding") << QLatin1String("xml");
        QProcess proc;
        proc.setEnvironment(QProcess::systemEnvironment());
        proc.start(exe, args);
        if (proc.waitForStarted() && proc.waitForFinished()) {
            QByteArray x3d = proc.readAll();
            if (x3d.isEmpty())
                return false;

            x3d.replace('\t', "  ");

            // compute a sensible view point
            SoGetBoundingBoxAction bboxAction(SbViewportRegion(1280, 1024));
            bboxAction.apply(node);
            SbBox3f bbox = bboxAction.getBoundingBox();
            SbSphere bs;
            bs.circumscribe(bbox);
            const SbVec3f& cnt = bs.getCenter();
            float dist = bs.getRadius();

            QString vp = QString::fromLatin1("  <Viewpoint id=\"Top\" centerOfRotation=\"%1 %2 %3\" "
                                             "position=\"%1 %2 %4\" orientation=\"0.000000 0.000000 1.000000 0.000000\" "
                                             "description=\"camera\" fieldOfView=\"0.9\"></Viewpoint>\n")
                         .arg(cnt[0]).arg(cnt[1]).arg(cnt[2]).arg(cnt[2] + 2.0f * dist);
            int index = x3d.indexOf("<Scene>\n");
            if (index >= 0) {
                x3d.insert(index + 8, vp);
            }

            buffer = x3d.data();
            return true;
        }
    }

    return false;
}

bool Gui::SoFCDB::writeToX3DOM(SoNode* node, std::string& buffer)
{
    std::string x3d;
    if (!writeToX3D(node, x3d))
        return false;

    // remove the first two lines from the x3d output as this duplicates
    // the xml and doctype header
    std::size_t pos = x3d.find('\n');
    pos = x3d.find('\n', pos + 1);
    x3d = x3d.erase(0, pos + 1);

    std::stringstream out;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n";
    out << "<html xmlns='http://www.w3.org/1999/xhtml'>\n"
        << "  <head>\n"
        << "    <script type='text/javascript' src='http://www.x3dom.org/download/x3dom.js'> </script>\n"
        << "    <link rel='stylesheet' type='text/css' href='http://www.x3dom.org/download/x3dom.css'></link>\n"
        << "  </head>\n";
    out << x3d;
    out << "</html>\n";

    buffer = out.str();

    return true;
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
    else if (fi.hasExtension("x3d") || fi.hasExtension("x3dz")) {
        // If 'x3dz' is set then force compression
        if (fi.hasExtension("x3dz"))
            binary = true;

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
  assert(storage); //call init first.
  return storage;
}
