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
	class ASMTPrincipalMassMarker : public ASMTSpatialItem
	{
		//
	public:
		ASMTPrincipalMassMarker();
		static std::shared_ptr<ASMTPrincipalMassMarker> With();
		void parseASMT(std::vector<std::string>& lines) override;
		void setMass(double mass);
		void setDensity(double density);
		void setMomentOfInertias(DiagMatDsptr momentOfInertias);

		// Overloads to simplify syntax.
		void setMomentOfInertias(double a, double b, double c);
		void storeOnLevel(std::ofstream& os, size_t level) override;

		double mass = 1.0;
		double density = 10.0;
		DiagMatDsptr momentOfInertias = std::make_shared<DiagonalMatrix<double>>(ListD{ 1.0, 2.0, 3.0 });

	};
}

