#include "MBDynInitialValue.h"

using namespace MbD;

void MbD::MBDynInitialValue::initialize()
{
}

void MbD::MBDynInitialValue::parseMBDyn(std::vector<std::string>& lines)
{
	readInitialTime(lines);
	readFinalTime(lines);
	readTimeStep(lines);
	readMaxIterations(lines);
	readTolerance(lines);
}

void MbD::MBDynInitialValue::readInitialTime(std::vector<std::string>& lines)
{
	//initial time:   0.;
	std::vector<std::string> tokens{"initial", "time:"};
	auto it = findLineWith(lines, tokens);
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> initialTime;	
}

void MbD::MBDynInitialValue::readFinalTime(std::vector<std::string>& lines)
{
	//final time:     5.;
	std::vector<std::string> tokens{"final", "time:"};
	auto it = findLineWith(lines, tokens);
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> finalTime;
}

void MbD::MBDynInitialValue::readTimeStep(std::vector<std::string>& lines)
{
	//time step:      1.e-2;
	std::vector<std::string> tokens{"time", "step:"};
	auto it = findLineWith(lines, tokens);
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> timeStep;
}

void MbD::MBDynInitialValue::readMaxIterations(std::vector<std::string>& lines)
{
	//max iterations: 10;
	std::vector<std::string> tokens{"max", "iterations:"};
	auto it = findLineWith(lines, tokens);
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> maxIterations;
}

void MbD::MBDynInitialValue::readTolerance(std::vector<std::string>& lines)
{
	//tolerance:      1.e-6;
	std::vector<std::string> tokens{"tolerance:"};
	auto it = findLineWith(lines, tokens);
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> tolerance;
}
