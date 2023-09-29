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
		//FRowDsptr readRowOfDoubles(std::string& line);
		//FColDsptr readColumnOfDoubles(std::string& line);
		//double readDouble(std::string& line);
		//int readInt(std::string& line);
		//bool readBool(std::string& line);
		//std::string readString(std::string& line);
		//void readName(std::vector<std::string>& lines);
		//virtual std::string fullName(std::string partialName);
		//void readDoublesInto(std::string& str, std::string label, FRowDsptr& row);
		//virtual void deleteMbD();
		//virtual void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits);
		//virtual void updateFromMbD();
		//virtual void compareResults(AnalysisType type);
		//virtual void outputResults(AnalysisType type);
		//std::shared_ptr<Units> mbdUnits();
		//std::shared_ptr<Constant> sptrConstant(double value);

		std::string name;
		MBDynItem* owner;
		std::shared_ptr<Item> mbdObject;


	};
}
