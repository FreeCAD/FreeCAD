/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "CREATE.h"

namespace MbD {
	class MBDynSystem;
	class MBDynVariables;
	class MBDynReferences;

	class MBDynItem
	{
		//
	public:
		virtual MBDynSystem* root();

		virtual void initialize();
		//void setName(std::string str);
		virtual void parseMBDyn(std::vector<std::string>& lines);
		std::vector<std::string>::iterator findLineWith(std::vector<std::string>& lines, std::vector<std::string>& tokens);
		bool lineHasTokens(const std::string& line, std::vector<std::string>& tokens);
		virtual std::shared_ptr<MBDynVariables> mbdynVariables();
		virtual std::shared_ptr<MBDynReferences> mbdynReferences();
		FColDsptr readPosition(std::shared_ptr<std::vector<std::string>>& args);
		FColDsptr readBasicPosition(std::shared_ptr<std::vector<std::string>>& args);
		FMatDsptr readOrientation(std::shared_ptr<std::vector<std::string>>& args);
		FMatDsptr readBasicOrientation(std::shared_ptr<std::vector<std::string>>& args);

		std::string name;
		MBDynItem* owner;
		std::shared_ptr<Item> mbdObject;


	};
}
