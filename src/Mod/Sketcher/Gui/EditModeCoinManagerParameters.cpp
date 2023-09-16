/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "EditModeCoinManagerParameters.h"


using namespace SketcherGui;

SbColor DrawingParameters::InformationColor(0.0f, 1.0f, 0.0f);       // #00FF00 -> (  0,255,  0)
SbColor DrawingParameters::CreateCurveColor(0.8f, 0.8f, 0.8f);       // #CCCCCC -> (204,204,204)
SbColor DrawingParameters::CrossColorH(0.8f, 0.4f, 0.4f);            // #CC6666 -> (204,102,102)
SbColor DrawingParameters::CrossColorV(0.47f, 1.0f, 0.51f);          // #83FF83 -> (120,255,131)
SbColor DrawingParameters::InvalidSketchColor(1.0f, 0.42f, 0.0f);    // #FF6D00 -> (255,109,  0)
SbColor DrawingParameters::FullyConstrainedColor(0.0f, 1.0f, 0.0f);  // #00FF00 -> (  0,255,  0)
SbColor
    DrawingParameters::FullyConstraintInternalAlignmentColor(0.87f,
                                                             0.87f,
                                                             0.78f);   // #DEDEC8 -> (222,222,200)
SbColor DrawingParameters::InternalAlignedGeoColor(0.7f, 0.7f, 0.5f);  // #B2B27F -> (178,178,127)
SbColor
    DrawingParameters::FullyConstraintConstructionPointColor(1.0f,
                                                             0.58f,
                                                             0.50f);  // #FF9580 -> (255,149,128)
SbColor DrawingParameters::VertexColor(1.0f, 0.149f, 0.0f);           // #FF2600 -> (255, 38,  0)
SbColor DrawingParameters::FullyConstraintElementColor(0.50f,
                                                       0.81f,
                                                       0.62f);           // #80D0A0 -> (128,208,160)
SbColor DrawingParameters::CurveColor(1.0f, 1.0f, 1.0f);                 // #FFFFFF -> (255,255,255)
SbColor DrawingParameters::PreselectColor(0.88f, 0.88f, 0.0f);           // #E1E100 -> (225,225,  0)
SbColor DrawingParameters::SelectColor(0.11f, 0.68f, 0.11f);             // #1CAD1C -> ( 28,173, 28)
SbColor DrawingParameters::PreselectSelectedColor(0.36f, 0.48f, 0.11f);  // #5D7B1C -> ( 93,123, 28)
SbColor DrawingParameters::CurveExternalColor(0.8f, 0.2f, 0.6f);         // #CC3399 -> (204, 51,153)
SbColor DrawingParameters::CurveDraftColor(0.0f, 0.0f, 0.86f);           // #0000DC -> (  0,  0,220)
SbColor
    DrawingParameters::FullyConstraintConstructionElementColor(0.56f,
                                                               0.66f,
                                                               0.99f);  // #8FA9FD -> (143,169,253)

SbColor DrawingParameters::ConstrDimColor(1.0f, 0.149f, 0.0f);  // #FF2600 -> (255, 38,  0)
SbColor DrawingParameters::ConstrIcoColor(1.0f, 0.149f, 0.0f);  // #FF2600 -> (255, 38,  0)
SbColor
    DrawingParameters::NonDrivingConstrDimColor(0.0f, 0.149f, 1.0f);     // #0026FF -> (  0, 38,255)
SbColor DrawingParameters::ExprBasedConstrDimColor(1.0f, 0.5f, 0.149f);  // #FF7F26 -> (255, 127,38)
SbColor DrawingParameters::DeactivatedConstrDimColor(0.8f, 0.8f, 0.8f);  // #CCCCCC -> (204,204,204)
SbColor DrawingParameters::CursorTextColor(0.0f, 0.0f, 1.0f);            // #0000FF -> (0,0,255)

const MultiFieldId MultiFieldId::Invalid = MultiFieldId();
