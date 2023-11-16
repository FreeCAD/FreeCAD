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
	class MBDynVariable;
	class MBDynReference;
	class MBDynNode;
	class ASMTItem;
	class MBDynBody;
	class MBDynJoint;
	class MBDynDrive;
	class ASMTAssembly;

	class MBDynItem
	{
		//
	public:
		virtual ~MBDynItem() {}
		virtual MBDynSystem* root();

		virtual void initialize();
		void noop();
		//void setName(std::string str);
		virtual void parseMBDyn(std::vector<std::string>& lines);
		static std::vector<std::string> collectArgumentsFor(std::string title, std::string& statement);
		std::vector<std::string>::iterator findLineWith(std::vector<std::string>& lines, std::vector<std::string>& tokens);
		bool lineHasTokens(const std::string& line, std::vector<std::string>& tokens);
		virtual std::shared_ptr<std::vector<std::shared_ptr<MBDynNode>>> mbdynNodes();
		virtual std::shared_ptr<std::vector<std::shared_ptr<MBDynBody>>> mbdynBodies();
		virtual std::shared_ptr<std::vector<std::shared_ptr<MBDynJoint>>> mbdynJoints();
		virtual std::shared_ptr<std::vector<std::shared_ptr<MBDynDrive>>> mbdynDrives();
		virtual std::vector<std::string> nodeNames();
		virtual std::shared_ptr<std::map<std::string, Symsptr>> mbdynVariables();
		virtual std::shared_ptr<std::map<std::string, std::shared_ptr<MBDynReference>>> mbdynReferences();
		virtual void createASMT();
		virtual std::shared_ptr<MBDynNode> nodeAt(std::string nodeName);
		virtual int nodeidAt(std::string nodeName);
		virtual std::shared_ptr<MBDynBody> bodyWithNode(std::string nodeName);
		virtual std::shared_ptr<ASMTAssembly> asmtAssembly();
		virtual std::string formulaFromDrive(std::string driveName, std::string varName);

		FColDsptr readVector3(std::vector<std::string>& args);
		FColDsptr readPosition(std::vector<std::string>& args);
		FColDsptr readBasicPosition(std::vector<std::string>& args);
		FMatDsptr readOrientation(std::vector<std::string>& args);
		FMatDsptr readBasicOrientation(std::vector<std::string>& args);
		std::string popOffTop(std::vector<std::string>& args);
		std::string readStringOffTop(std::vector<std::string>& args);
		std::string readToken(std::string& line);

		std::string name;
		MBDynItem* owner;
		std::shared_ptr<ASMTItem> asmtItem;


	};
}
