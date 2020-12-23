/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>

#include "FeaturePad.h"
#include "FeatureSolid.h"
#include "FeaturePocket.h"
#include "FeatureFillet.h"
#include "FeatureSketchBased.h"
#include "FeatureRevolution.h"
#include "FeatureGroove.h"
#include "Body.h"
#include "FeatureDressUp.h"
#include "FeatureChamfer.h"
#include "FeatureDraft.h"
#include "FeatureTransformed.h"
#include "FeatureMirrored.h"
#include "FeatureLinearPattern.h"
#include "FeaturePolarPattern.h"
#include "FeatureScaled.h"
#include "FeatureMultiTransform.h"
#include "FeatureHole.h"
#include "DatumPlane.h"
#include "DatumLine.h"
#include "DatumPoint.h"
#include "FeatureBoolean.h"
#include "FeaturePrimitive.h"
#include "DatumCS.h"
#include "FeatureThickness.h"
#include "FeaturePipe.h"
#include "FeatureLoft.h"
#include "ShapeBinder.h"
#include "FeatureBase.h"

namespace PartDesign {
extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(_PartDesign)
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import Sketcher");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(0);
    }

    PyObject* mod = PartDesign::initModule();
    Base::Console().Log("Loading PartDesign module... done\n");


    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.

    PartDesign::Feature                     ::init();
    PartDesign::FeaturePython               ::init();
    PartDesign::Solid                       ::init();
    PartDesign::DressUp                     ::init();
    PartDesign::FeatureAddSub               ::init();
    PartDesign::FeatureAddSubPython         ::init();
    PartDesign::FeatureAdditivePython       ::init();
    PartDesign::FeatureSubtractivePython    ::init();
    PartDesign::ProfileBased                ::init();
    PartDesign::Transformed                 ::init();
    PartDesign::Mirrored                    ::init();
    PartDesign::LinearPattern               ::init();
    PartDesign::PolarPattern                ::init();
    PartDesign::Scaled                      ::init();
    PartDesign::MultiTransform              ::init();
    PartDesign::Hole                        ::init();
    PartDesign::Body                        ::init();
    PartDesign::Pad                         ::init();
    PartDesign::Pocket                      ::init();
    PartDesign::Fillet                      ::init();
    PartDesign::Revolution                  ::init();
    PartDesign::Groove                      ::init();
    PartDesign::Chamfer                     ::init();
    PartDesign::Draft                       ::init();
    PartDesign::Thickness                   ::init();
    PartDesign::Pipe                        ::init();
    PartDesign::AdditivePipe                ::init();
    PartDesign::SubtractivePipe             ::init();
    PartDesign::Loft                        ::init();
    PartDesign::AdditiveLoft                ::init();
    PartDesign::SubtractiveLoft             ::init();
    PartDesign::ShapeBinder                 ::init();
    PartDesign::SubShapeBinder              ::init();
    PartDesign::Plane                       ::init();
    PartDesign::Line                        ::init();
    PartDesign::Point                       ::init();
    PartDesign::CoordinateSystem            ::init();
    PartDesign::Boolean                     ::init();
    PartDesign::FeaturePrimitive            ::init();
    PartDesign::Box                         ::init();
    PartDesign::AdditiveBox                 ::init();
    PartDesign::SubtractiveBox              ::init();
    PartDesign::Cylinder                    ::init();
    PartDesign::AdditiveCylinder            ::init();
    PartDesign::SubtractiveCylinder         ::init();
    PartDesign::Sphere                      ::init();
    PartDesign::AdditiveSphere              ::init();
    PartDesign::SubtractiveSphere           ::init();
    PartDesign::Cone                        ::init();
    PartDesign::AdditiveCone                ::init();
    PartDesign::SubtractiveCone             ::init();
    PartDesign::Ellipsoid                   ::init();
    PartDesign::AdditiveEllipsoid           ::init();
    PartDesign::SubtractiveEllipsoid        ::init();
    PartDesign::Torus                       ::init();
    PartDesign::AdditiveTorus               ::init();
    PartDesign::SubtractiveTorus            ::init();
    PartDesign::Prism                       ::init();
    PartDesign::AdditivePrism               ::init();
    PartDesign::SubtractivePrism            ::init();
    PartDesign::Wedge                       ::init();
    PartDesign::AdditiveWedge               ::init();
    PartDesign::SubtractiveWedge            ::init();
    PartDesign::FeatureBase                 ::init();

    PyMOD_Return(mod);
}
