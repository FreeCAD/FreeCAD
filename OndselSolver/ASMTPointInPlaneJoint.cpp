/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTPointInPlaneJoint.h"
#include "PointInPlaneJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTPointInPlaneJoint::mbdClassNew()
{
    return CREATE<PointInPlaneJoint>::With();
}

void MbD::ASMTPointInPlaneJoint::parseASMT(std::vector<std::string>& lines)
{
    ASMTJoint::parseASMT(lines);
    readOffset(lines);
}

void MbD::ASMTPointInPlaneJoint::readOffset(std::vector<std::string>& lines)
{
	assert(lines[0].find("offset") != std::string::npos);
	lines.erase(lines.begin());
	offset = readDouble(lines[0]);
	lines.erase(lines.begin());

}

void MbD::ASMTPointInPlaneJoint::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTJoint::createMbD(mbdSys, mbdUnits);
	auto pointInPlaneJoint = std::static_pointer_cast<PointInPlaneJoint>(mbdObject);
	pointInPlaneJoint->offset = offset;
}

void MbD::ASMTPointInPlaneJoint::storeOnLevel(std::ofstream& os, int level)
{
	storeOnLevelString(os, level, "PointInPlaneJoint");
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
	ASMTItemIJ::storeOnLevel(os, level);
}

void MbD::ASMTPointInPlaneJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "PointInPlaneJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}

