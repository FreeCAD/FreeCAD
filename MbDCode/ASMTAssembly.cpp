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
	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	while (!lines.empty()) {
		if (lines[0] == (leadingTabs + "Notes")) {
			lines.erase(lines.begin());
			notes = lines[0];
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "Name")) {
			lines.erase(lines.begin());
			name = lines[0];
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "Position3D")) {
			lines.erase(lines.begin());
			std::istringstream iss(lines[0]);
			position3D = std::make_shared<FullColumn<double>>();
			double d;
			while (iss >> d) {
				position3D->push_back(d);
			}
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "RotationMatrix")) {
			lines.erase(lines.begin());
			rotationMatrix = std::make_shared<FullMatrix<double>>(3, 0);
			for (int i = 0; i < 3; i++)
			{
				auto& row = rotationMatrix->at(i);
				std::istringstream iss(lines[0]);
				double d;
				while (iss >> d) {
					row->push_back(d);
				}
				lines.erase(lines.begin());
			}
		}
		else if (lines[0] == (leadingTabs + "Velocity3D")) {
			lines.erase(lines.begin());
			std::istringstream iss(lines[0]);
			velocity3D = std::make_shared<FullColumn<double>>();
			double d;
			while (iss >> d) {
				velocity3D->push_back(d);
			}
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "Omega3D")) {
			lines.erase(lines.begin());
			std::istringstream iss(lines[0]);
			omega3D = std::make_shared<FullColumn<double>>();
			double d;
			while (iss >> d) {
				omega3D->push_back(d);
			}
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "RefPoints")) {
			lines.erase(lines.begin());
			refPoints = std::make_shared<std::vector<std::shared_ptr<ASMTRefPoint>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs + "RefCurves"));
			std::vector<std::string> refPointsLines(lines.begin(), it);
			while (!refPointsLines.empty()) {
				if (refPointsLines[0] == (leadingTabs + "\tRefPoint")) {
					refPointsLines.erase(refPointsLines.begin());
					auto refPoint = CREATE<ASMTRefPoint>::With();
					refPoint->parseASMT(refPointsLines);
					refPoints->push_back(refPoint);
					refPoint->owner = this;
				}
				else {
					assert(false);
				}
			}
			lines.erase(lines.begin(), it);
		}
		else if (lines[0] == (leadingTabs + "RefCurves")) {
			lines.erase(lines.begin());
			refCurves = std::make_shared<std::vector<std::shared_ptr<ASMTRefCurve>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs + "RefSurfaces"));
			std::vector<std::string> refCurvesLines(lines.begin(), it);
			while (!refCurvesLines.empty()) {
				if (refCurvesLines[0] == (leadingTabs + "\tRefCurve")) {
					refCurvesLines.erase(refCurvesLines.begin());
					auto refCurve = CREATE<ASMTRefCurve>::With();
					refCurve->parseASMT(refCurvesLines);
					refCurves->push_back(refCurve);
					refCurve->owner = this;
				}
				else {
					assert(false);
				}
			}
			lines.erase(lines.begin(), it);
		}
		else if (lines[0] == (leadingTabs + "RefSurfaces")) {
			lines.erase(lines.begin());
			refSurfaces = std::make_shared<std::vector<std::shared_ptr<ASMTRefSurface>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs + "Parts"));
			std::vector<std::string> refSurfacesLines(lines.begin(), it);
			while (!refSurfacesLines.empty()) {
				if (refSurfacesLines[0] == (leadingTabs + "\tRefSurface")) {
					refSurfacesLines.erase(refSurfacesLines.begin());
					auto refSurface = CREATE<ASMTRefSurface>::With();
					refSurface->parseASMT(refSurfacesLines);
					refSurfaces->push_back(refSurface);
					refSurface->owner = this;
				}
				else {
					assert(false);
				}
			}
			lines.erase(lines.begin(), it);
		}
		else if (lines[0] == (leadingTabs + "Parts")) {
			lines.erase(lines.begin());
			parts = std::make_shared<std::vector<std::shared_ptr<ASMTPart>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs + "KinematicIJs"));
			std::vector<std::string> partsLines(lines.begin(), it);
			while (!partsLines.empty()) {
				if (partsLines[0] == (leadingTabs + "\tPart")) {
					partsLines.erase(partsLines.begin());
					auto part = CREATE<ASMTPart>::With();
					part->parseASMT(partsLines);
					parts->push_back(part);
					part->owner = this;
				}
				else {
					assert(false);
				}
			}
			lines.erase(lines.begin(), it);
		}
		else if (lines[0] == (leadingTabs + "KinematicIJs")) {
			lines.erase(lines.begin());
			kinematicIJs = std::make_shared<std::vector<std::shared_ptr<ASMTKinematicIJ>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs + "ConstraintSets"));
			std::vector<std::string> kinematicIJsLines(lines.begin(), it);
			while (!kinematicIJsLines.empty()) {
				if (kinematicIJsLines[0] == (leadingTabs + "\tKinematicIJ")) {
					kinematicIJsLines.erase(kinematicIJsLines.begin());
					auto kinematicIJ = CREATE<ASMTKinematicIJ>::With();
					kinematicIJ->parseASMT(kinematicIJsLines);
					kinematicIJs->push_back(kinematicIJ);
					kinematicIJ->owner = this;
				}
				else {
					assert(false);
				}
			}
			lines.erase(lines.begin(), it);
		}
		else if (lines[0] == (leadingTabs + "ConstraintSets")) {
			lines.erase(lines.begin());
			assert(lines[0] == (leadingTabs + "\tJoints"));
			lines.erase(lines.begin());
			joints = std::make_shared<std::vector<std::shared_ptr<ASMTJoint>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs + "\tMotions"));
			std::vector<std::string> jointsLines(lines.begin(), it);
			while (!jointsLines.empty()) {
				if (jointsLines[0] == (leadingTabs + "\t\tRevoluteJoint")) {
					jointsLines.erase(jointsLines.begin());
					auto joint = CREATE<ASMTRevoluteJoint>::With();
					joint->parseASMT(jointsLines);
					joints->push_back(joint);
					joint->owner = this;
				}
				else if (jointsLines[0] == (leadingTabs + "\t\tCylindricalJoint")) {
					jointsLines.erase(jointsLines.begin());
					auto joint = CREATE<ASMTCylindricalJoint>::With();
					joint->parseASMT(jointsLines);
					joints->push_back(joint);
					joint->owner = this;
				}
				else {
					assert(false);
				}
			}
			lines.erase(lines.begin(), it);
			assert(lines[0] == (leadingTabs + "\tMotions"));
			lines.erase(lines.begin());
			motions = std::make_shared<std::vector<std::shared_ptr<ASMTMotion>>>();
			it = std::find(lines.begin(), lines.end(), (leadingTabs + "\tGeneralConstraintSets"));
			std::vector<std::string> motionsLines(lines.begin(), it);
			while (!motionsLines.empty()) {
				if (motionsLines[0] == (leadingTabs + "\t\tRotationalMotion")) {
					motionsLines.erase(motionsLines.begin());
					auto motion = CREATE<ASMTRotationalMotion>::With();
					motion->parseASMT(motionsLines);
					motions->push_back(motion);
					motion->owner = this;
				}
				else if (motionsLines[0] == (leadingTabs + "\t\tTranslationalMotion")) {
					motionsLines.erase(motionsLines.begin());
					auto motion = CREATE<ASMTTranslationalMotion>::With();
					motion->parseASMT(motionsLines);
					motions->push_back(motion);
					motion->owner = this;
				}
				else {
					assert(false);
				}
			}
			lines.erase(lines.begin(), it);
			assert(lines[0] == (leadingTabs + "\tGeneralConstraintSets"));
			lines.erase(lines.begin());
			constraintSets = std::make_shared<std::vector<std::shared_ptr<ASMTConstraintSet>>>();
			it = std::find(lines.begin(), lines.end(), (leadingTabs + "ForceTorques"));
			std::vector<std::string> generalConstraintSetsLines(lines.begin(), it);
			while (!generalConstraintSetsLines.empty()) {
				assert(false);
			}
			lines.erase(lines.begin(), it);
		}
		else if (lines[0] == (leadingTabs + "ForceTorques")) {
			lines.erase(lines.begin());
			forceTorques = std::make_shared<std::vector<std::shared_ptr<ASMTForceTorque>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs + "ConstantGravity"));
			std::vector<std::string> forceTorquesLines(lines.begin(), it);
			while (!forceTorquesLines.empty()) {
				if (forceTorquesLines[0] == (leadingTabs + "\tForceTorque")) {
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
		else if (lines[0] == (leadingTabs + "ConstantGravity")) {
			lines.erase(lines.begin());
			constantGravity = CREATE<ASMTConstantGravity>::With();
			constantGravity->parseASMT(lines);
			constantGravity->owner = this;
			}
		else if (lines[0] == (leadingTabs + "SimulationParameters")) {
			lines.erase(lines.begin());
			simulationParameters = CREATE<ASMTSimulationParameters>::With();
			simulationParameters->parseASMT(lines);
			simulationParameters->owner = this;
		}
		else if (lines[0] == (leadingTabs + "AnimationParameters")) {
			lines.erase(lines.begin());
			animationParameters = CREATE<ASMTAnimationParameters>::With();
			animationParameters->parseASMT(lines);
			animationParameters->owner = this;
		}
		else {
			assert(false);
		}
	}
}
