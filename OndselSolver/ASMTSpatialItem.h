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
	class EXPORT ASMTSpatialItem : public ASMTItem
	{
		//
	public:
		void setPosition3D(FColDsptr position3D);
		void setRotationMatrix(FMatDsptr rotationMatrix);
		void readPosition3D(std::vector<std::string>& lines);
		void readRotationMatrix(std::vector<std::string>& lines);

		// Overloads to simplify syntax.
		void getPosition3D(double& a, double& b, double& c);
		void getQuarternions(double& q0, double& q1, double& q2, double& q3);
		void setPosition3D(double a, double b, double c);
		void setQuarternions(double q0, double q1, double q2, double q3);
		void setRotationMatrix(double v11, double v12, double v13,
			double v21, double v22, double v23,
			double v31, double v32, double v33);
		void storeOnLevel(std::ofstream& os, int level) override;
		void storeOnLevelPosition(std::ofstream& os, int level);
		void storeOnLevelRotationMatrix(std::ofstream& os, int level);
		FColDsptr getPosition3D(int i);
		FMatDsptr getRotationMatrix(int i);

		FColDsptr position3D = std::make_shared<FullColumn<double>>(3);
		FMatDsptr rotationMatrix = std::make_shared<FullMatrixDouble>(ListListD{
				{ 1, 0, 0 },
				{ 0, 1, 0 },
				{ 0, 0, 1 }
			});
		FRowDsptr xs, ys, zs, bryxs, bryys, bryzs;
		FRowDsptr inxs, inys, inzs, inbryxs, inbryys, inbryzs;

	};
}

