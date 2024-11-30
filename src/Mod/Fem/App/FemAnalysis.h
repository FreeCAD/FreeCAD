/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#ifndef Fem_FemAnalysis_H
#define Fem_FemAnalysis_H

#include <App/DocumentObjectGroup.h>
#include <App/FeaturePython.h>
#include <App/PropertyStandard.h>
#include <Mod/Fem/FemGlobal.h>


namespace Fem
{

/**
 * @brief Container of objects relevant to one simulation.
 *
 * @details
 *  A Analysis contains all objects necessary for a complete specification
 *  of a simulation. After computing it also contains the result of the
 *  simulation. The Analysis object is just a container. It is not responsible
 *  for anything else like executing the simulation.
 *
 *  The Analysis class is essentially a App::DocumentObjectGroup. It handles
 *  all the container stuff. The difference is that the Analysis document
 *  object uses a different ViewProvider, has a Uid property and does some
 *  compatibility handling via handleChangedPropertyName.
 *
 *  This implies that it is not checked which objects are put into the
 *  Analysis object. Every document object of FreeCAD can be part of a
 *  Analysis.
 */
class FemExport FemAnalysis: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemAnalysis);

public:
    /**
     * Uses Base::Uuid to initialize the Uid property to a newly generated
     * unique id because PropertyUUID doesn't initialize itself.
     */
    FemAnalysis();
    ~FemAnalysis() override;

    /**
     * A unique identifier for each Analysis object. Useful when doing
     * filesystem operations because it can provide a unique file or
     * directory name for an analysis. Retains its value across save/load
     * cycles.
     */
    App::PropertyUUID Uid;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemAnalysis";
    }

protected:
    /**
     * @brief Retain compatibility with old "Member" property.
     *
     * @details
     *  In an older version of FreeCAD FemAnalysis handles it's member itself
     *  in a property called "Member". Now this is handled in the "Group"
     *  property of DocumentObjectGroup. This methods translates old files
     *  still using the "Member" property.
     */
    void handleChangedPropertyName(Base::XMLReader& reader,
                                   const char* TypeName,
                                   const char* PropName) override;
};

class FemExport DocumentObject: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::DocumentObject);
};

using FemAnalysisPython = App::FeaturePythonT<FemAnalysis>;
using FeaturePython = App::FeaturePythonT<DocumentObject>;


}  // namespace Fem


#endif  // Fem_FemAnalysis_H
