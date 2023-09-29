#include "MBDynControlData.h"

using namespace MbD;

void MbD::MBDynControlData::initialize()
{
}

void MbD::MBDynControlData::parseMBDyn(std::vector<std::string>& lines)
{
	readStructuralNodes(lines);
	readRigidBodies(lines);
	readJoints(lines);
}

void MbD::MBDynControlData::readStructuralNodes(std::vector<std::string>& lines)
{	
	//structural nodes: 4;
	std::vector<std::string> tokens{"structural", "nodes:"};
	auto it = findLineWith(lines, tokens);
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> structuralNodes;
}

void MbD::MBDynControlData::readRigidBodies(std::vector<std::string>& lines)
{
	//rigid bodies:     3;
	std::vector<std::string> tokens{"rigid", "bodies:"};
	auto it = findLineWith(lines, tokens);
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> rigidBodies;
}

void MbD::MBDynControlData::readJoints(std::vector<std::string>& lines)
{
	//joints:           6;
	std::vector<std::string> tokens{"joints:"};
	auto it = findLineWith(lines, tokens);
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> joints;
}
