#include "MBDynControlData.h"

using namespace MbD;

void MbD::MBDynControlData::initialize()
{
}

void MbD::MBDynControlData::parseMBDyn(std::vector<std::string>& lines)
{
	readMaxIterations(lines);
	readDefaultOrientation(lines);
	readOmegaRotates(lines);
	readPrint(lines);
	readInitialStiffness(lines);
	readStructuralNodes(lines);
	readRigidBodies(lines);
	readJoints(lines);
	readGravity(lines);
	assert(lines.size() == 2);
}

void MbD::MBDynControlData::readMaxIterations(std::vector<std::string>& lines)
{
	//max iterations: 1000;
	std::vector<std::string> tokens{ "max", "iterations:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> maxIterations;
	lines.erase(it);
}

void MbD::MBDynControlData::readDefaultOrientation(std::vector<std::string>& lines)
{
	//default orientation: euler321;
	std::vector<std::string> tokens{ "default", "orientation:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> defaultOrientation;
	lines.erase(it);
}

void MbD::MBDynControlData::readOmegaRotates(std::vector<std::string>& lines)
{
	//omega rotates: no;
	std::vector<std::string> tokens{ "omega", "rotates:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> omegaRotates;
	lines.erase(it);
}

void MbD::MBDynControlData::readPrint(std::vector<std::string>& lines)
{
	//print: none;
	std::vector<std::string> tokens{ "print:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> print;
	lines.erase(it);
}

void MbD::MBDynControlData::readInitialStiffness(std::vector<std::string>& lines)
{
	//initial stiffness: 1.0, 1.0;
	std::vector<std::string> tokens{ "initial", "stiffness:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> initialStiffness;
	iss >> str;
	initialStiffness.append(str);
	lines.erase(it);
}

void MbD::MBDynControlData::readStructuralNodes(std::vector<std::string>& lines)
{
	//structural nodes: 4;
	std::vector<std::string> tokens{ "structural", "nodes:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> structuralNodes;
	lines.erase(it);
}

void MbD::MBDynControlData::readRigidBodies(std::vector<std::string>& lines)
{
	//rigid bodies:     3;
	std::vector<std::string> tokens{ "rigid", "bodies:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> rigidBodies;
	lines.erase(it);
}

void MbD::MBDynControlData::readJoints(std::vector<std::string>& lines)
{
	//joints:           6;
	std::vector<std::string> tokens{ "joints:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> joints;
	lines.erase(it);
}

void MbD::MBDynControlData::readGravity(std::vector<std::string>& lines)
{
	//gravity;
	std::vector<std::string> tokens{ "gravity" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	lines.erase(it);
}
