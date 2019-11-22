/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_FeatureAdditive_H
#define PARTDESIGN_FeatureAdditive_H

#include <App/PropertyStandard.h>
#include <Mod/Part/App/PropertyTopoShape.h>

#include "Feature.h"

/// Base class of all additive features in PartDesign
namespace PartDesign
{

class PartDesignExport FeatureAddSub : public PartDesign::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::FeatureAddSub);

public:
    enum Type {
        Additive = 0,
        Subtractive 
    };
    
    FeatureAddSub();

    Type getAddSubType();

    virtual short mustExecute() const override;

    Part::PropertyPartShape   AddSubShape;
    App::PropertyBool Refine;

protected:
    Type addSubType;

    TopoDS_Shape refineShapeIfActive(const TopoDS_Shape&) const;
};

typedef App::FeaturePythonT<FeatureAddSub> FeatureAddSubPython;

class FeatureAdditivePython : public FeatureAddSubPython
{
    PROPERTY_HEADER(PartDesign::FeatureAdditivePython);

public:
    FeatureAdditivePython();
    ~FeatureAdditivePython();
};

class FeatureSubtractivePython : public FeatureAddSubPython
{
    PROPERTY_HEADER(PartDesign::FeatureSubtractivePython);

public:
    FeatureSubtractivePython();
    ~FeatureSubtractivePython();
};

} //namespace PartDesign


#endif // PARTDESIGN_FeatureAdditive_H
