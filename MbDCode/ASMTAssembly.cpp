#include <string>
#include <cassert>
#include <fstream>	
#include <algorithm>
#include <numeric>

#include "ASMTAssembly.h"
#include "CREATE.h"
#include "ASMTRevoluteJoint.h"
#include "ASMTCylindricalJoint.h"
#include "ASMTRotationalMotion.h"
#include "ASMTTranslationalMotion.h"
#include "ASMTMarker.h"
#include "Part.h"

using namespace MbD;

void MbD::ASMTAssembly::runFile(const char* chars)
{
	std::ifstream stream(chars);
	std::string line;
	std::vector<std::string> lines;
	while (std::getline(stream, line)) {
		lines.push_back(line);
	}
	assert(lines[0] == "freeCAD: 3D CAD with Motion Simulation  by  askoh.com");
	lines.erase(lines.begin());

	if (lines[0] == "Assembly") {
		lines.erase(lines.begin());
		auto assembly = CREATE<ASMTAssembly>::With();
		assembly->parseASMT(lines);
		assembly->runKINEMATIC();
	}

}

ASMTAssembly* MbD::ASMTAssembly::root()
{
	return this;
}

void MbD::ASMTAssembly::parseASMT(std::vector<std::string>& lines)
{
	readNotes(lines);
	readName(lines);
	readPosition3D(lines);
	readRotationMatrix(lines);
	readVelocity3D(lines);
	readOmega3D(lines);
	initprincipalMassMarker();
	readRefPoints(lines);
	readRefCurves(lines);
	readRefSurfaces(lines);
	readParts(lines);
	readKinematicIJs(lines);
	readConstraintSets(lines);
	readForcesTorques(lines);
	readConstantGravity(lines);
	readSimulationParameters(lines);
	readAnimationParameters(lines);
	readTimeSeries(lines);
	readAssemblySeries(lines);
	readPartSeriesMany(lines);
	readJointSeriesMany(lines);
	readMotionSeriesMany(lines);
}

void MbD::ASMTAssembly::readNotes(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tNotes");
	lines.erase(lines.begin());
	notes = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTAssembly::readParts(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tParts");
	lines.erase(lines.begin());
	parts = std::make_shared<std::vector<std::shared_ptr<ASMTPart>>>();
	auto it = std::find(lines.begin(), lines.end(), "\tKinematicIJs");
	std::vector<std::string> partsLines(lines.begin(), it);
	while (!partsLines.empty()) {
		readPart(partsLines);
	}
	lines.erase(lines.begin(), it);

}

void MbD::ASMTAssembly::readPart(std::vector<std::string>& lines)
{
	assert(lines[0] == "\t\tPart");
	lines.erase(lines.begin());
	auto part = CREATE<ASMTPart>::With();
	part->parseASMT(lines);
	parts->push_back(part);
	part->owner = this;
}

void MbD::ASMTAssembly::readKinematicIJs(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tKinematicIJs");
	lines.erase(lines.begin());
	kinematicIJs = std::make_shared<std::vector<std::shared_ptr<ASMTKinematicIJ>>>();
	auto it = std::find(lines.begin(), lines.end(), "\tConstraintSets");
	std::vector<std::string> kinematicIJsLines(lines.begin(), it);
	while (!kinematicIJsLines.empty()) {
		readKinematicIJ(kinematicIJsLines);
	}
	lines.erase(lines.begin(), it);

}

void MbD::ASMTAssembly::readKinematicIJ(std::vector<std::string>& lines)
{
	assert(false);
}

void MbD::ASMTAssembly::readConstraintSets(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tConstraintSets");
	lines.erase(lines.begin());
	readJoints(lines);
	readMotions(lines);
	readGeneralConstraintSets(lines);
}

void MbD::ASMTAssembly::readJoints(std::vector<std::string>& lines)
{
	assert(lines[0] == "\t\tJoints");
	lines.erase(lines.begin());
	joints = std::make_shared<std::vector<std::shared_ptr<ASMTJoint>>>();
	auto it = std::find(lines.begin(), lines.end(), "\t\tMotions");
	std::vector<std::string> jointsLines(lines.begin(), it);
	std::shared_ptr<ASMTJoint> joint;
	while (!jointsLines.empty()) {
		if (jointsLines[0] == "\t\t\tRevoluteJoint") {
			joint = CREATE<ASMTRevoluteJoint>::With();
		}
		else if (jointsLines[0] == "\t\t\tCylindricalJoint") {
			joint = CREATE<ASMTCylindricalJoint>::With();
		}
		else {
			assert(false);
		}
		jointsLines.erase(jointsLines.begin());
		joint->parseASMT(jointsLines);
		joints->push_back(joint);
		joint->owner = this;
	}
	lines.erase(lines.begin(), it);

}

void MbD::ASMTAssembly::readMotions(std::vector<std::string>& lines)
{
	assert(lines[0] == "\t\tMotions");
	lines.erase(lines.begin());
	motions = std::make_shared<std::vector<std::shared_ptr<ASMTMotion>>>();
	auto it = std::find(lines.begin(), lines.end(), "\t\tGeneralConstraintSets");
	std::vector<std::string> motionsLines(lines.begin(), it);
	std::shared_ptr<ASMTMotion> motion;
	while (!motionsLines.empty()) {
		if (motionsLines[0] == "\t\t\tRotationalMotion") {
			motion = CREATE<ASMTRotationalMotion>::With();
		}
		else if (motionsLines[0] == "\t\t\tTranslationalMotion") {
			motion = CREATE<ASMTTranslationalMotion>::With();
		}
		else {
			assert(false);
		}
		motionsLines.erase(motionsLines.begin());
		motion->parseASMT(motionsLines);
		motions->push_back(motion);
		motion->owner = this;
		motion->initMarkers();
	}
	lines.erase(lines.begin(), it);

}

void MbD::ASMTAssembly::readGeneralConstraintSets(std::vector<std::string>& lines)
{
	assert(lines[0] == "\t\tGeneralConstraintSets");
	lines.erase(lines.begin());
	constraintSets = std::make_shared<std::vector<std::shared_ptr<ASMTConstraintSet>>>();
	auto it = std::find(lines.begin(), lines.end(), "\tForceTorques");
	std::vector<std::string> generalConstraintSetsLines(lines.begin(), it);
	while (!generalConstraintSetsLines.empty()) {
		assert(false);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTAssembly::readForcesTorques(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tForceTorques");	//Spelling is not consistent in asmt file.
	lines.erase(lines.begin());
	forcesTorques = std::make_shared<std::vector<std::shared_ptr<ASMTForceTorque>>>();
	auto it = std::find(lines.begin(), lines.end(), "\tConstantGravity");
	std::vector<std::string> forcesTorquesLines(lines.begin(), it);
	while (!forcesTorquesLines.empty()) {
		if (forcesTorquesLines[0] == "\t\tForceTorque") {
			forcesTorquesLines.erase(forcesTorquesLines.begin());
			auto forceTorque = CREATE<ASMTForceTorque>::With();
			forceTorque->parseASMT(forcesTorquesLines);
			forcesTorques->push_back(forceTorque);
			forceTorque->owner = this;
		}
		else {
			assert(false);
		}
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTAssembly::readConstantGravity(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tConstantGravity");
	lines.erase(lines.begin());
	constantGravity = CREATE<ASMTConstantGravity>::With();
	constantGravity->parseASMT(lines);
	constantGravity->owner = this;
}

void MbD::ASMTAssembly::readSimulationParameters(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tSimulationParameters");
	lines.erase(lines.begin());
	simulationParameters = CREATE<ASMTSimulationParameters>::With();
	simulationParameters->parseASMT(lines);
	simulationParameters->owner = this;
}

void MbD::ASMTAssembly::readAnimationParameters(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tAnimationParameters");
	lines.erase(lines.begin());
	animationParameters = CREATE<ASMTAnimationParameters>::With();
	animationParameters->parseASMT(lines);
	animationParameters->owner = this;
}

void MbD::ASMTAssembly::readTimeSeries(std::vector<std::string>& lines)
{
	assert(lines[0] == "TimeSeries");
	lines.erase(lines.begin());
	assert(lines[0].find("Number\tInput") != std::string::npos);
	lines.erase(lines.begin());
	readTimes(lines);
}

void MbD::ASMTAssembly::readTimes(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	std::string substr = "Time\tInput";
	auto pos = str.find(substr);
	assert(pos != std::string::npos);
	str.erase(0, pos + substr.length());
	times = readRowOfDoubles(str);
	times->insert(times->begin(), times->at(0));	//The first element is the input state.
	lines.erase(lines.begin());
}

void MbD::ASMTAssembly::readPartSeriesMany(std::vector<std::string>& lines)
{
	assert(lines[0].find("PartSeries") != std::string::npos);
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("JointSeries") != std::string::npos;
		});
	std::vector<std::string> partSeriesLines(lines.begin(), it);
	while (!partSeriesLines.empty()) {
		readPartSeries(partSeriesLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTAssembly::readJointSeriesMany(std::vector<std::string>& lines)
{
	assert(lines[0].find("JointSeries") != std::string::npos);
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("MotionSeries") != std::string::npos;
		});
	std::vector<std::string> jointSeriesLines(lines.begin(), it);
	while (!jointSeriesLines.empty()) {
		readJointSeries(jointSeriesLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTAssembly::readAssemblySeries(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	std::string substr = "AssemblySeries";
	auto pos = str.find(substr);
	assert(pos != std::string::npos);
	str.erase(0, pos + substr.length());
	auto seriesName = readString(str);
	assert(fullName("") == seriesName);
	lines.erase(lines.begin());
	//xs, ys, zs, bryxs, bryys, bryzs
	readXs(lines);
	readYs(lines);
	readZs(lines);
	readBryantxs(lines);
	readBryantys(lines);
	readBryantzs(lines);
	readVXs(lines);
	readVYs(lines);
	readVZs(lines);
	readOmegaXs(lines);
	readOmegaYs(lines);
	readOmegaZs(lines);
	readAXs(lines);
	readAYs(lines);
	readAZs(lines);
	readAlphaXs(lines);
	readAlphaYs(lines);
	readAlphaZs(lines);
}

void MbD::ASMTAssembly::readPartSeries(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	std::string substr = "PartSeries";
	auto pos = str.find(substr);
	assert(pos != std::string::npos);
	str.erase(0, pos + substr.length());
	auto seriesName = readString(str);
	auto it = std::find_if(parts->begin(), parts->end(), [&](const std::shared_ptr<ASMTPart>& prt) {
		return prt->fullName("") == seriesName;
		});
	auto& part = *it;
	part->readPartSeries(lines);
}

void MbD::ASMTAssembly::readJointSeries(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	std::string substr = "JointSeries";
	auto pos = str.find(substr);
	assert(pos != std::string::npos);
	str.erase(0, pos + substr.length());
	auto seriesName = readString(str);
	auto it = std::find_if(joints->begin(), joints->end(), [&](const std::shared_ptr<ASMTJoint>& jt) {
		return jt->fullName("") == seriesName;
		});
	auto& joint = *it;
	joint->readJointSeries(lines);
}

void MbD::ASMTAssembly::readMotionSeriesMany(std::vector<std::string>& lines)
{
	assert(lines[0].find("MotionSeries") != std::string::npos);
	while (!lines.empty()) {
		readMotionSeries(lines);
	}
}

void MbD::ASMTAssembly::readMotionSeries(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	std::string substr = "MotionSeries";
	auto pos = str.find(substr);
	assert(pos != std::string::npos);
	str.erase(0, pos + substr.length());
	auto seriesName = readString(str);
	auto it = std::find_if(motions->begin(), motions->end(), [&](const std::shared_ptr<ASMTMotion>& jt) {
		return jt->fullName("") == seriesName;
		});
	auto& motion = *it;
	motion->readMotionSeries(lines);
}

void MbD::ASMTAssembly::outputFor(AnalysisType type)
{
	assert(false);
}

void MbD::ASMTAssembly::logString(std::string& str)
{
	assert(false);
}

void MbD::ASMTAssembly::logString(double value)
{
	assert(false);
}

void MbD::ASMTAssembly::preMbDrun(std::shared_ptr<System> mbdSys)
{
	calcCharacteristicDimensions();
	deleteMbD();
	createMbD(mbdSys, mbdUnits);
	std::static_pointer_cast<Part>(mbdObject)->asFixed();
}

void MbD::ASMTAssembly::postMbDrun()
{
	assert(false);
}

void MbD::ASMTAssembly::calcCharacteristicDimensions()
{
	auto unitTime = this->calcCharacteristicTime();
	auto unitMass = this->calcCharacteristicMass();
	auto unitLength = this->calcCharacteristicLength();
	auto unitAngle = 1.0;
	this->mbdUnits = std::make_shared<Units>(unitTime, unitMass, unitLength, unitAngle);
}

double MbD::ASMTAssembly::calcCharacteristicTime()
{
	return std::abs(simulationParameters->hout);
}

double MbD::ASMTAssembly::calcCharacteristicMass()
{
	auto n = parts->size();
	double sumOfSquares = 0.0;
	for (int i = 0; i < n; i++)
	{
		auto mass = parts->at(i)->principalMassMarker->mass;
		sumOfSquares += mass * mass;
	}
	auto unitMass = std::sqrt(sumOfSquares / n);
	if (unitMass <= 0) unitMass = 1.0;
	return unitMass;
}

double MbD::ASMTAssembly::calcCharacteristicLength()
{
	auto markerMap = this->markerMap();
	auto lengths = std::make_shared<std::vector<double>>();
	auto connectorList = this->connectorList();
	for (auto& connector : *connectorList) {
		auto& mkrI = markerMap->at(connector->markerI);
		lengths->push_back(mkrI->rpmp()->length());
		auto& mkrJ = markerMap->at(connector->markerJ);
		lengths->push_back(mkrJ->rpmp()->length());
	}
	auto n = lengths->size();
	double sumOfSquares = std::accumulate(lengths->begin(), lengths->end(), 0.0, [](double sum, double l) { return sum + l * l; });
	auto unitLength = std::sqrt(sumOfSquares / std::max((int)n, 1));
	if (unitLength <= 0) unitLength = 1.0;
	return unitLength;
}

std::shared_ptr<std::vector<std::shared_ptr<ASMTItemIJ>>> MbD::ASMTAssembly::connectorList()
{
	auto list = std::make_shared<std::vector<std::shared_ptr<ASMTItemIJ>>>();
	list->insert(list->end(), joints->begin(), joints->end());
	list->insert(list->end(), motions->begin(), motions->end());
	list->insert(list->end(), kinematicIJs->begin(), kinematicIJs->end());
	list->insert(list->end(), forcesTorques->begin(), forcesTorques->end());
	return list;
}

std::shared_ptr<std::map<std::string, std::shared_ptr<ASMTMarker>>> MbD::ASMTAssembly::markerMap()
{
	auto answer = std::make_shared<std::map<std::string, std::shared_ptr<ASMTMarker>>>();
	for (auto& refPoint : *refPoints) {
		for (auto& marker : *refPoint->markers) {
			answer->insert(std::make_pair(marker->fullName(""), marker));
		}
	}
	for (auto& part : *parts) {
		for (auto& refPoint : *part->refPoints) {
			for (auto& marker : *refPoint->markers) {
				answer->insert(std::make_pair(marker->fullName(""), marker));
			}
		}
	}
	return answer;
}

void MbD::ASMTAssembly::deleteMbD()
{
	ASMTSpatialContainer::deleteMbD();
	constantGravity->deleteMbD();
	asmtTime->deleteMbD();
	for (auto& part : *parts) { part->deleteMbD(); }
	for (auto& joint : *joints) { joint->deleteMbD(); }
	for (auto& motion : *motions) { motion->deleteMbD(); }
	for (auto& forceTorque : *forcesTorques) { forceTorque->deleteMbD(); }


}

void MbD::ASMTAssembly::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTSpatialContainer::createMbD(mbdSys, mbdUnits);
	constantGravity->createMbD(mbdSys, mbdUnits);
	asmtTime->createMbD(mbdSys, mbdUnits);
	for (auto& part : *parts) { part->createMbD(mbdSys, mbdUnits); }
	for (auto& joint : *joints) { joint->createMbD(mbdSys, mbdUnits); }
	for (auto& motion : *motions) { motion->createMbD(mbdSys, mbdUnits); }
	for (auto& forceTorque : *forcesTorques) { forceTorque->createMbD(mbdSys, mbdUnits); }

	auto mbdSysSolver = mbdSys->systemSolver;
	mbdSysSolver->errorTolPosKine = simulationParameters->errorTolPosKine;
	mbdSysSolver->errorTolAccKine = simulationParameters->errorTolAccKine;
	mbdSysSolver->iterMaxPosKine = simulationParameters->iterMaxPosKine;
	mbdSysSolver->iterMaxAccKine = simulationParameters->iterMaxAccKine;
	mbdSysSolver->tstart = simulationParameters->tstart / mbdUnits->time;
	mbdSysSolver->tend = simulationParameters->tend / mbdUnits->time;
	mbdSysSolver->hmin = simulationParameters->hmin / mbdUnits->time;
	mbdSysSolver->hmax = simulationParameters->hmax / mbdUnits->time;
	mbdSysSolver->hout = simulationParameters->hout / mbdUnits->time;
	mbdSysSolver->corAbsTol = simulationParameters->corAbsTol;
	mbdSysSolver->corRelTol = simulationParameters->corRelTol;
	mbdSysSolver->intAbsTol = simulationParameters->intAbsTol;
	mbdSysSolver->intRelTol = simulationParameters->intRelTol;
	mbdSysSolver->iterMaxDyn = simulationParameters->iterMaxDyn;
	mbdSysSolver->orderMax = simulationParameters->orderMax;
	mbdSysSolver->translationLimit = simulationParameters->translationLimit / mbdUnits->length;
	mbdSysSolver->rotationLimit = simulationParameters->rotationLimit;
	animationParameters = nullptr;
}

void MbD::ASMTAssembly::runKINEMATIC()
{
	auto mbdSystem = CREATE<System>::With();
	mbdObject = mbdSystem;
	mbdSystem->externalSystem->asmtAssembly = this;
	mbdSystem->runKINEMATIC(mbdSystem);
}
//
//void MbD::ASMTAssembly::initprincipalMassMarker()
//{
//	//Choose first refPoint as center of mass
//	assert(!refPoints->empty());
//	auto refPoint = refPoints->at(0);
//	principalMassMarker = CREATE<ASMTPrincipalMassMarker>::With();
//	principalMassMarker->position3D = refPoint->position3D;
//	principalMassMarker->rotationMatrix = refPoint->rotationMatrix;
//}

void MbD::ASMTAssembly::initprincipalMassMarker()
{
	principalMassMarker = CREATE<ASMTPrincipalMassMarker>::With();
	principalMassMarker->mass = 0.0;
	principalMassMarker->density = 0.0;
	principalMassMarker->momentOfInertias = std::make_shared<DiagonalMatrix<double>>(3, 0);
	principalMassMarker->position3D = std::make_shared<FullColumn<double>>(3, 0);
	principalMassMarker->rotationMatrix = std::make_shared<FullMatrix<double>>(3, 3);
	principalMassMarker->rotationMatrix->identity();
}

std::shared_ptr<ASMTSpatialContainer> MbD::ASMTAssembly::spatialContainerAt(std::shared_ptr<ASMTAssembly> self, std::string& longname)
{
	if ((self->fullName("")) == longname) return self;
	auto it = std::find_if(parts->begin(), parts->end(), [&](const std::shared_ptr<ASMTPart>& prt) {
		return prt->fullName("") == longname;
		});
	auto& part = *it;
	return part;
}

std::shared_ptr<ASMTMarker> MbD::ASMTAssembly::markerAt(std::string& longname)
{
	for (auto& refPoint : *refPoints) {
		for (auto& marker : *refPoint->markers) {
			if (marker->fullName("") == longname) return marker;
		}
	}
	for (auto& part : *parts) {
		for (auto& refPoint : *part->refPoints) {
			for (auto& marker : *refPoint->markers) {
				if (marker->fullName("") == longname) return marker;
			}
		}
	}
	return nullptr;
}

std::shared_ptr<ASMTJoint> MbD::ASMTAssembly::jointAt(std::string& longname)
{
	auto it = std::find_if(joints->begin(), joints->end(), [&](const std::shared_ptr<ASMTJoint>& jt) {
		return jt->fullName("") == longname;
		});
	auto& joint = *it;
	return joint;
}

std::shared_ptr<ASMTMotion> MbD::ASMTAssembly::motionAt(std::string& longname)
{
	auto it = std::find_if(motions->begin(), motions->end(), [&](const std::shared_ptr<ASMTMotion>& mt) {
		return mt->fullName("") == longname;
		});
	auto& motion = *it;
	return motion;
}

std::shared_ptr<ASMTForceTorque> MbD::ASMTAssembly::forceTorqueAt(std::string& longname)
{
	auto it = std::find_if(forcesTorques->begin(), forcesTorques->end(), [&](const std::shared_ptr<ASMTForceTorque>& mt) {
		return mt->fullName("") == longname;
		});
	auto& forceTorque = *it;
	return forceTorque;
}

FColDsptr MbD::ASMTAssembly::vOcmO()
{
	return std::make_shared<FullColumn<double>>(3, 0.0);
}

FColDsptr MbD::ASMTAssembly::omeOpO()
{
	return std::make_shared<FullColumn<double>>(3, 0.0);
}

std::shared_ptr<ASMTTime> MbD::ASMTAssembly::geoTime()
{
	return asmtTime;
}

void MbD::ASMTAssembly::updateFromMbD()
{
	ASMTSpatialContainer::updateFromMbD();
	auto geoTime = asmtTime->getValue();
	auto it = std::find_if(times->begin(), times->end(), [&](double t) {
		return Numeric::equaltol(t, geoTime, 1.0e-9);
		});
	assert(it != times->end());
	for (auto& part : *parts) part->updateFromMbD();
	for (auto& joint : *joints) joint->updateFromMbD();
	for (auto& motion : *motions) motion->updateFromMbD();
	for (auto& forceTorque : *forcesTorques) forceTorque->updateFromMbD();
}

void MbD::ASMTAssembly::compareResults(AnalysisType type)
{
	ASMTSpatialContainer::compareResults(type);
	for (auto& part : *parts) part->compareResults(type);
	for (auto& joint : *joints) joint->compareResults(type);
	for (auto& motion : *motions) motion->compareResults(type);
	for (auto& forceTorque : *forcesTorques) forceTorque->compareResults(type);
}


