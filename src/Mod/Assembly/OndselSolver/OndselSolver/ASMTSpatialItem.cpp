/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "ASMTSpatialItem.h"
#include "Units.h"
#include "Part.h"
#include "ASMTSpatialContainer.h"
#include "EulerAngles.h"

using namespace MbD;

MbD::ASMTSpatialItem::ASMTSpatialItem() : ASMTItem()
{
}

void MbD::ASMTSpatialItem::setPosition3D(FColDsptr vec)
{
	position3D = vec;
}

void MbD::ASMTSpatialItem::setQuarternions(double q0, double q1, double q2, double q3)
{
	auto eulerParameters = CREATE<EulerParameters<double>>::With(ListD{ q1, q2, q3, q0 });
	eulerParameters->calc();
	rotationMatrix = eulerParameters->aA;
}

void MbD::ASMTSpatialItem::setRotationMatrix(FMatDsptr mat)
{
	rotationMatrix = mat;
}

void MbD::ASMTSpatialItem::readPosition3D(std::vector<std::string>& lines)
{
	assert(lines[0].find("Position3D") != std::string::npos);
	lines.erase(lines.begin());
	std::istringstream iss(lines[0]);
	position3D = std::make_shared<FullColumn<double>>();
	double d;
	while (iss >> d) {
		position3D->push_back(d);
	}
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialItem::readRotationMatrix(std::vector<std::string>& lines)
{
	assert(lines[0].find("RotationMatrix") != std::string::npos);
	lines.erase(lines.begin());
	rotationMatrix = std::make_shared<FullMatrix<double>>(3, 0);
	for (size_t i = 0; i < 3; i++)
	{
		auto& row = rotationMatrix->at(i);
		std::istringstream iss(lines[0]);
		double d;
		while (iss >> d) {
			row->push_back(d);
		}
		lines.erase(lines.begin());
	}
}

void MbD::ASMTSpatialItem::getPosition3D(double& a, double& b, double& c)
{
	a = position3D->at(0);
	b = position3D->at(1);
	c = position3D->at(2);
}

void MbD::ASMTSpatialItem::getQuarternions(double& q0, double& q1, double& q2, double& q3)
{
	auto eulerParameters = rotationMatrix->asEulerParameters();
	q0 = eulerParameters->at(3);
	q1 = eulerParameters->at(0);
	q2 = eulerParameters->at(1);
	q3 = eulerParameters->at(2);
}

// Overloads to simplify syntax.
void MbD::ASMTSpatialItem::setPosition3D(double a, double b, double c)
{
	position3D = std::make_shared<FullColumn<double>>(ListD{ a, b, c });
}

void MbD::ASMTSpatialItem::setRotationMatrix(double v11, double v12, double v13,
	double v21, double v22, double v23,
	double v31, double v32, double v33)
{
	rotationMatrix = std::make_shared<FullMatrix<double>>(ListListD{
		{ v11, v12, v13 },
		{ v21, v22, v23 },
		{ v31, v32, v33 }
		});
}

void MbD::ASMTSpatialItem::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelPosition(os, level + 1);
	storeOnLevelRotationMatrix(os, level + 1);
}

void MbD::ASMTSpatialItem::storeOnLevelPosition(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "Position3D");
	if (xs == nullptr || xs->empty()) {
		storeOnLevelArray(os, level + 1, *position3D);
	}
	else {
		auto array = getPosition3D(0);
		storeOnLevelArray(os, level + 1, *array);
	}
}

void MbD::ASMTSpatialItem::storeOnLevelPositionRaw(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "Position3D");
	storeOnLevelArray(os, level + 1, *position3D);
}

void MbD::ASMTSpatialItem::storeOnLevelRotationMatrix(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "RotationMatrix");
	if (xs == nullptr || xs->empty()) {
		for (size_t i = 0; i < 3; i++)
		{
			storeOnLevelArray(os, level + 1, *rotationMatrix->at(i));
		}
	}
	else {
		auto rotMat = getRotationMatrix(0);
		for (size_t i = 0; i < 3; i++)
		{
			storeOnLevelArray(os, level + 1, *rotMat->at(i));
		}
	}

}

void MbD::ASMTSpatialItem::storeOnLevelRotationMatrixRaw(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "RotationMatrix");
	for (size_t i = 0; i < 3; i++)
	{
		storeOnLevelArray(os, level + 1, *rotationMatrix->at(i));
	}
}

FColDsptr MbD::ASMTSpatialItem::getPosition3D(size_t i)
{
	auto vec3 = std::make_shared<FullColumn<double>>(3);
	vec3->atiput(0, xs->at(i));
	vec3->atiput(1, ys->at(i));
	vec3->atiput(2, zs->at(i));
	return vec3;
}

FMatDsptr MbD::ASMTSpatialItem::getRotationMatrix(size_t i)
{
	auto bryantAngles = std::make_shared<EulerAngles<double>>();
	bryantAngles->setRotOrder(1, 2, 3);
	bryantAngles->at(0) = bryxs->at(i);
	bryantAngles->at(1) = bryys->at(i);
	bryantAngles->at(2) = bryzs->at(i);
	bryantAngles->calc();
	return bryantAngles->aA;
}

void MbD::ASMTSpatialItem::restorePosRot()
{
	position3D = oldPos3D;
	rotationMatrix = oldRotMat;
}
