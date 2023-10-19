/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTSpatialItem.h"

namespace MbD {
    class EXPORT ASMTPrincipalMassMarker : public ASMTSpatialItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void setMass(double mass);
        void setDensity(double density);
        void setMomentOfInertias(DiagMatDsptr momentOfInertias);

        // Overloads to simplify syntax.
        void setMomentOfInertias(double a, double b, double c);
        void storeOnLevel(std::ofstream& os, int level) override;

        double mass = 0.0;
        double density = 0.0;
        DiagMatDsptr momentOfInertias = std::make_shared<DiagonalMatrix<double>>(ListD{ 0.,0.,0. });

    };
}

