/***************************************************************************
 *   Copyright (c) 2019 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
#pragma once //to make qt creator happy, see QTCREATORBUG-20883

#ifndef FREECAD_CONSTRAINTSOLVER_PARATRANSFORM_H
#define FREECAD_CONSTRAINTSOLVER_PARATRANSFORM_H

#include <Mod/ConstraintSolver/App/ParaGeometry.h>
#include "Placement.h"

namespace FCS {
namespace G2D {

class ParaTransform;
typedef Base::UnsafePyHandle<ParaTransform> HParaTransform;

class ParaPlacement;
typedef Base::UnsafePyHandle<ParaPlacement> HParaPlacement;

class FCSExport ParaTransform : public FCS::ParaObject
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
protected://data
    ///chain of inverse placements
    /// Overall transform = inverse(_revchain[0]*...*_revchain[n]) * chain[0]*...*chain[n]
    std::vector<HParaPlacement> _revchain;
    ///chain of forward placements
    std::vector<HParaPlacement> _fwchain;

public://methods
    virtual PyObject* getPyObject() override;

    Placement value(const ValueSet& vals);
    Placement operator()(const ValueSet& vals) {return value(vals);}

    virtual void update() override;
    virtual HParaObject copy() const override;
    virtual std::vector<ParameterRef> makeParameters(HParameterStore into) override;
    virtual void throwIfIncomplete() const override;
    virtual void initFromDict(Py::Dict dict) override;

    const std::vector<HParaPlacement>& fwchain() const {return _fwchain;}
    const std::vector<HParaPlacement>& revchain() const {return _revchain;}
    void set(const std::vector<HParaPlacement>& fwchain, const std::vector<HParaPlacement>& revchain);

    void add(HParaPlacement plm);
    void add(HParaTransform plm);
    void addInverse(HParaPlacement plm);
    void addInverse(HParaTransform plm);

    /**
     * @brief simplify purges same placements from revchain and fwdchain.
     *
     * example:
     * before:
     *   fwchain = placement1, placement2, placement3
     *   revchain = placement1, placement2, placement4, placement3, placement5
     * after:
     *   fwchain = placement3
     *   revchain = placement4, placement3, placement5
     * returns 2.
     *
     * @return the number of placement repetitions purged
     */
    int simplify();
    ///makes a ParaTransform that transforms an object defined in 'from' coordinate system into 'to' coordinate system.
    /// 'to' and 'from' must have no inverse transforms to them
    static HParaTransform transformFromInto(HParaTransform from, HParaTransform into);

    /**
     * @brief simplifyTransforms Simplifies a list of transforms by removing the common lead-in.
     * @param transforms: list of transforms to simplify. Transforms can have chains of inverses.
     * @param compute_not_change: if true, only returns the number of common placements that can be stripped off; the transforms are not modified then.
     * @return number of placements that were/can be stripped off
     */
    static int simplifyTransforms(std::vector<HParaTransform> transforms, bool compute_not_change = false);
    static int simplifyTransformsOfConstraint(ParaObject& constraint, bool compute_not_change = false);

    ///throws an exception if revchain is not empty.
    void throwIfHasInverse() const;
    ///interface for extracting placement sequence
    /// @{
    ///returns i'th placement of the full chain
    HParaPlacement operator[](int index);
    ///is the i'th placement of the full chain inverse?
    bool isInverse(int index){return index >= _revchain.size();}
    int numInverses() const {return _revchain.size();}
    int size() const {return _revchain.size() + _fwchain.size();}
    /// @}

protected://methods
    virtual void initAttrs() override;

public: //friends
    friend class ParaTransformPy;

};

}} //namespace

#endif
