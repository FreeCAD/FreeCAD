#include <string>
#include <cassert>
#include <fstream>	

#include "ASMTAssembly.h"
#include "CREATE.h"
#include "ASMTRevoluteJoint.h"
#include "ASMTCylindricalJoint.h"
#include "ASMTRotationalMotion.h"
#include "ASMTTranslationalMotion.h"

using namespace MbD;

void MbD::ASMTAssembly::runFile(const char* chars)
{
	std::string str;
	std::ifstream in(chars);
	std::string line;
	std::vector<std::string> lines;
	while (std::getline(in, line)) {
		lines.push_back(line);
	}
	assert(lines[0] == "freeCAD: 3D CAD with Motion Simulation  by  askoh.com");
	lines.erase(lines.begin());

	if (lines[0] == "Assembly") {
		lines.erase(lines.begin());
		auto assembly = CREATE<ASMTAssembly>::With();
		assembly->parseASMT(lines);
	}

}

void MbD::ASMTAssembly::parseASMT(std::vector<std::string>& lines)
{
	readNotes(lines);
	readName(lines);
	readPosition3D(lines);
	readRotationMatrix(lines);
	readVelocity3D(lines);
	readOmega3D(lines);
	readRefPoints(lines);
	readRefCurves(lines);
	readRefSurfaces(lines);
	readParts(lines);
	readKinematicIJs(lines);
	readConstraintSets(lines);
	readForceTorques(lines);
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

void MbD::ASMTAssembly::readForceTorques(std::vector<std::string>& lines)
{
	assert(lines[0] == "\tForceTorques");
	lines.erase(lines.begin());
	forceTorques = std::make_shared<std::vector<std::shared_ptr<ASMTForceTorque>>>();
	auto it = std::find(lines.begin(), lines.end(), "\tConstantGravity");
	std::vector<std::string> forceTorquesLines(lines.begin(), it);
	while (!forceTorquesLines.empty()) {
		if (forceTorquesLines[0] == "\t\tForceTorque") {
			forceTorquesLines.erase(forceTorquesLines.begin());
			auto forceTorque = CREATE<ASMTForceTorque>::With();
			forceTorque->parseASMT(forceTorquesLines);
			forceTorques->push_back(forceTorque);
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
	auto part = *it;
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
	auto joint = *it;
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
	auto motion = *it;
	motion->readMotionSeries(lines);
}


