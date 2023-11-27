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
		void readMaxIterations(std::vector<std::string>& lines);
		void readDefaultOrientation(std::vector<std::string>& lines);
		void readOmegaRotates(std::vector<std::string>& lines);
		void readPrint(std::vector<std::string>& lines);
		void readInitialStiffness(std::vector<std::string>& lines);
		void readStructuralNodes(std::vector<std::string>& lines);
		void readRigidBodies(std::vector<std::string>& lines);
		void readJoints(std::vector<std::string>& lines);
		void readGravity(std::vector<std::string>& lines);

		int maxIterations = 1000;
		std::string defaultOrientation = "euler321";
		std::string omegaRotates = "no";
		std::string print = "none";
		std::string initialStiffness = "1.0, 1.0";
		int structuralNodes, rigidBodies, joints;
	};
}
