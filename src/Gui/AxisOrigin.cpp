/****************************************************************************
 *   Copyright (c) 2019 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoIndexedPointSet.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/SoFullPath.h>
#endif

#include "Inventor/SoAutoZoomTranslation.h"
#include "SoFCSelection.h"
#include "SoFCUnifiedSelection.h"
#include "AxisOrigin.h"

using namespace Gui;

TYPESYSTEM_SOURCE(Gui::AxisOrigin,Base::BaseClass)

AxisOrigin::AxisOrigin() = default;

SoGroup *AxisOrigin::getNode() {
    if(node)
        return node;

    node.reset(new SoGroup);
    auto pMat = new SoMaterial();

    const SbVec3f verts[13] =
    {
        SbVec3f(0,0,0), SbVec3f(size,0,0),
        SbVec3f(0,size,0), SbVec3f(0,0,size),
        SbVec3f(dist,dist,0), SbVec3f(dist,pSize,0), SbVec3f(pSize,dist,0),  // XY Plane
        SbVec3f(dist,0,dist), SbVec3f(dist,0,pSize), SbVec3f(pSize,0,dist),  // XY Plane
        SbVec3f(0,dist,dist), SbVec3f(0,pSize,dist), SbVec3f(0,dist,pSize)  // XY Plane
    };

    // indexes used to create the edges
    const int32_t lines[21] =
    {
        0,1,-1,
        0,2,-1,
        0,3,-1,
        5,4,6,-1,
        8,7,9,-1,
        11,10,12,-1
    };

    pMat->diffuseColor.setNum(3);
    pMat->diffuseColor.set1Value(0, SbColor(1.0f, 0.2f, 0.2f));
    pMat->diffuseColor.set1Value(1, SbColor(0.2f, 0.6f, 0.2f));
    pMat->diffuseColor.set1Value(2, SbColor(0.2f, 0.2f, 1.0f));
    pMat->diffuseColor.set1Value(4, SbColor(0.8f, 0.8f, 0.8f));

    auto pCoords = new SoCoordinate3();
    pCoords->point.setNum(3);
    pCoords->point.setValues(0, 13, verts);

    auto zoom = new SoAutoZoomTranslation;
    zoom->scaleFactor = scale;

    auto style = new SoDrawStyle();
    style->lineWidth = lineSize;
    style->pointSize = pointSize;

    auto matBinding = new SoMaterialBinding;
    matBinding->value = SoMaterialBinding::PER_FACE_INDEXED;

    node->addChild(zoom);
    node->addChild(style);
    node->addChild(matBinding);
    node->addChild(pMat);
    node->addChild(pCoords);

#define CREATE_AXIS(_type,_key,_count,_offset,_mat) do{\
        const char *label=_key;\
        if(labels.size()){\
            auto iter = labels.find(_key);\
            if(iter == labels.end())\
                break;\
            else if(iter->second.size())\
                label = iter->second.c_str();\
        }\
        auto pAxis = new SoFCSelection;\
        pAxis->applySettings();\
        pAxis->style = SoFCSelection::EMISSIVE_DIFFUSE;\
        pAxis->subElementName = label;\
        nodeMap[label].reset(pAxis);\
        node->addChild(pAxis);\
        auto _type  = new SoIndexed##_type##Set;\
        pAxis->addChild(_type);\
        _type->coordIndex.setNum(_count);\
        _type->coordIndex.setValues(0,_count,lines+_offset);\
        _type->materialIndex.setValue(_mat);\
    }while(0)

    CREATE_AXIS(Point,"O",1,0,4);
    CREATE_AXIS(Line,"X",3,0,0);
    CREATE_AXIS(Line,"Y",3,3,1);
    CREATE_AXIS(Line,"Z",3,6,2);
    CREATE_AXIS(Line,"XY",4,9,2);
    CREATE_AXIS(Line,"XZ",4,13,1);
    CREATE_AXIS(Line,"YZ",4,17,0);
    return node;
}

bool AxisOrigin::getElementPicked(const SoPickedPoint *pp, std::string &subname) const {
    SoPath *path = pp->getPath();
    int length = path->getLength();
    for(int i=0;i<length;++i) {
        auto node = path->getNodeFromTail(i);
        if(node->isOfType(SoFCSelection::getClassTypeId())) {
            subname = static_cast<SoFCSelection*>(node)->subElementName.getValue().getString();
            return true;
        } else if(node->isOfType(SoFCSelectionRoot::getClassTypeId()))
            break;
    }
    return false;
}

bool AxisOrigin::getDetailPath(const char *subname, SoFullPath *pPath, SoDetail *&) const {
    if(!node)
        return false;
    if(!subname || !subname[0])
        return true;

    auto it = nodeMap.find(subname);
    if(it == nodeMap.end())
        return false;
    pPath->append(node);
    pPath->append(it->second);
    return true;
}

void AxisOrigin::setLineWidth(float size) {
    if(size!=lineSize) {
        node.reset();
        nodeMap.clear();
        lineSize = size;
    }
}

void AxisOrigin::setPointSize(float size) {
    if(pointSize!=size) {
        pointSize = size;
        node.reset();
        nodeMap.clear();
    }
}

void AxisOrigin::setAxisLength(float size) {
    if(this->size!=size) {
        this->size = size;
        node.reset();
        nodeMap.clear();
    }
}

void AxisOrigin::setPlane(float size, float dist) {
    if(pSize!=size || this->dist!=dist) {
        pSize = size;
        this->dist = dist;
        node.reset();
        nodeMap.clear();
    }
}

void AxisOrigin::setScale(float scale) {
    if(this->scale!=scale) {
        this->scale = scale;
        node.reset();
        nodeMap.clear();
    }
}

void AxisOrigin::setLabels(const std::map<std::string,std::string> &labels) {
    this->labels = labels;
    node.reset();
    nodeMap.clear();
}
