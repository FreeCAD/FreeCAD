/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTPrincipalMassMarker : public ASMTItem
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;

		FColDsptr position3D;
		FMatDsptr rotationMatrix;
		double mass, density;
        DiagMatDsptr momentOfInertias;

    };
}

