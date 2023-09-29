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
	class ASMTSpatialItem : public ASMTItem
	{
		//
	public:
		void setPosition3D(FColDsptr position3D);
		void setRotationMatrix(FMatDsptr rotationMatrix);
		void setVelocity3D(FColDsptr velocity3D);
		void setOmega3D(FColDsptr omega3D);
		void readPosition3D(std::vector<std::string>& lines);
		void readRotationMatrix(std::vector<std::string>& lines);
		void readVelocity3D(std::vector<std::string>& lines);
		void readOmega3D(std::vector<std::string>& lines);

		FColDsptr position3D = std::make_shared<FullColumn<double>>(3);
		FColDsptr velocity3D = std::make_shared<FullColumn<double>>(3);
		FColDsptr omega3D = std::make_shared<FullColumn<double>>(3);
		FMatDsptr rotationMatrix = std::make_shared<FullMatrix<double>>(ListListD{
				{ 1, 0, 0 },
				{ 0, 1, 0 },
				{ 0, 0, 1 }
			});

		// Overloads to simplify syntax.
		void setPosition3D(double a, double b, double c);
		void setVelocity3D(double a, double b, double c);
		void setOmega3D(double a, double b, double c);
		void setRotationMatrix(double v11, double v12, double v13,
			double v21, double v22, double v23,
			double v31, double v32, double v33);
	};
}

