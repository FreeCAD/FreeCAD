/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "MBDynBlock.h"

namespace MbD {
	class MBDynControlData : public MBDynBlock
	{
	public:
		void initialize() override;
		void parseMBDyn(std::vector<std::string>& lines) override;
		void readStructuralNodes(std::vector<std::string>& lines);
		void readRigidBodies(std::vector<std::string>& lines);
		void readJoints(std::vector<std::string>& lines);

		int structuralNodes, rigidBodies, joints;
	};
}
