#include "ASMTPart.h"
#include "CREATE.h"

using namespace MbD;

void MbD::ASMTPart::parseASMT(std::vector<std::string>& lines)
{
	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	while (!lines.empty()) {
		if (lines[0] == (leadingTabs + "Name")) {
			lines.erase(lines.begin());
			name = lines[0];
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "Position3D")) {
			lines.erase(lines.begin());
			position3D = readColumnOfDoubles(lines[0]);
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "RotationMatrix")) {
			lines.erase(lines.begin());
			rotationMatrix = std::make_shared<FullMatrix<double>>(3);
			for (int i = 0; i < 3; i++)
			{
				auto row = readRowOfDoubles(lines[0]);
				rotationMatrix->atiput(i, row);
				lines.erase(lines.begin());
			}
		}
		else if (lines[0] == (leadingTabs + "Velocity3D")) {
			lines.erase(lines.begin());
			velocity3D = readColumnOfDoubles(lines[0]);
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "Omega3D")) {
			lines.erase(lines.begin());
			omega3D = readColumnOfDoubles(lines[0]);
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "FeatureOrder")) {
			lines.erase(lines.begin());
			//featureOrder = std::make_shared<std::vector<std::shared_ptr<ASMTRefPoint>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs + "PrincipalMassMarker"));
			//std::vector<std::string> featureOrderLines(lines.begin(), it);
			//while (!featureOrderLines.empty()) {
			//	if (featureOrderLines[0] == (leadingTabs + "\tExtrusion")) {
			//		featureOrderLines.erase(featureOrderLines.begin());
			//		auto extrusion = CREATE<ASMTExtrusion>::With();
			//		extrusion->parseASMT(featureOrderLines);
			//		featureOrder->push_back(extrusion);
			//		extrusion->owner = this;
			//	}
			//	else {
			//		assert(false);
			//	}
			//}
			lines.erase(lines.begin(), it);
		}
		else if (lines[0] == (leadingTabs + "PrincipalMassMarker")) {
			lines.erase(lines.begin());
			principalMassMarker = CREATE<ASMTPrincipalMassMarker>::With();
			principalMassMarker->parseASMT(lines);
			principalMassMarker->owner = this;
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
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs.substr(0, leadingTabs.size() - 1) + "Part"));
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
		else {
			return;
		}
	}
}
