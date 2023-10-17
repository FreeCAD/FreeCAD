#include "MBDynInitialValue.h"
#include "ASMTSimulationParameters.h"
#include "ASMTAssembly.h"

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
	readDerivativesTolerance(lines);
	readDerivativesMaxIterations(lines);
	readDerivativesCoefficient(lines);
	assert(lines.size() == 2);
}

void MbD::MBDynInitialValue::readInitialTime(std::vector<std::string>& lines)
{
	//initial time:   0.;
	std::vector<std::string> tokens{ "initial", "time:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> initialTime;
	lines.erase(it);
}

void MbD::MBDynInitialValue::readFinalTime(std::vector<std::string>& lines)
{
	//final time:     5.;
	std::vector<std::string> tokens{ "final", "time:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> finalTime;
	lines.erase(it);
}

void MbD::MBDynInitialValue::readTimeStep(std::vector<std::string>& lines)
{
	//time step:      1.e-2;
	std::vector<std::string> tokens{ "time", "step:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> timeStep;
	lines.erase(it);
}

void MbD::MBDynInitialValue::readMaxIterations(std::vector<std::string>& lines)
{
	//max iterations: 10;
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

void MbD::MBDynInitialValue::readTolerance(std::vector<std::string>& lines)
{
	//tolerance:      1.e-6;
	std::vector<std::string> tokens{ "tolerance:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> tolerance;
	lines.erase(it);
}

void MbD::MBDynInitialValue::readDerivativesTolerance(std::vector<std::string>& lines)
{
	//derivatives tolerance: 0.0001;
	std::vector<std::string> tokens{ "derivatives", "tolerance:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> derivativesTolerance;
	lines.erase(it);
}

void MbD::MBDynInitialValue::readDerivativesMaxIterations(std::vector<std::string>& lines)
{
	//derivatives max iterations: 100;
	std::vector<std::string> tokens{ "derivatives", "max", "iterations:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> str;
	iss >> derivativesMaxIterations;
	lines.erase(it);
}

void MbD::MBDynInitialValue::readDerivativesCoefficient(std::vector<std::string>& lines)
{
	//derivatives coefficient: auto;
	std::vector<std::string> tokens{ "derivatives", "coefficient:" };
	auto it = findLineWith(lines, tokens);
	if (it == lines.end()) return;
	std::istringstream iss(*it);
	std::string str;
	iss >> str;
	iss >> str;
	iss >> derivativesCoefficient;
	lines.erase(it);
}

void MbD::MBDynInitialValue::createASMT()
{
	auto simulationParameters = std::make_shared<ASMTSimulationParameters>();
	asmtItem = simulationParameters;
	simulationParameters->settstart(initialTime);
	simulationParameters->settend(finalTime);	//tstart == tend Initial Conditions only.
	simulationParameters->sethmin(1.0e-9);
	simulationParameters->sethmax(1.0);
	simulationParameters->sethout(timeStep);
	simulationParameters->seterrorTol(tolerance);
	simulationParameters->setmaxIter(maxIterations);
	asmtAssembly()->setSimulationParameters(simulationParameters);
}
