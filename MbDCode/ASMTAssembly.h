#pragma once
#include <fstream>	

#include "ASMTItem.h"
#include "ASMTRefPoint.h"
#include "ASMTRefCurve.h"
#include "ASMTRefSurface.h"
#include "ASMTPart.h"
#include "ASMTKinematicIJ.h"
#include "ASMTConstraintSet.h"
#include "ASMTForceTorque.h"
#include "ASMTConstantGravity.h"
#include "ASMTSimulationParameters.h"
#include "ASMTAnimationParameters.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "ASMTJoint.h"
#include "ASMTMotion.h"

namespace MbD {
    class ASMTAssembly : public ASMTItem
    {
        //
    public:
        static void runFile(const char* chars);
        void parseASMT(std::vector<std::string>& lines) override;

        std::string notes, name;
        FColDsptr position3D, velocity3D, omega3D;
        FMatDsptr rotationMatrix;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefPoint>>> refPoints;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefCurve>>> refCurves;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTRefSurface>>> refSurfaces;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTPart>>> parts;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTKinematicIJ>>> kinematicIJs;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTConstraintSet>>> constraintSets;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTJoint>>> joints;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTMotion>>> motions;
        std::shared_ptr<std::vector<std::shared_ptr<ASMTForceTorque>>> forceTorques;
        std::shared_ptr<ASMTConstantGravity> constantGravity;
        std::shared_ptr<ASMTSimulationParameters> simulationParameters;
        std::shared_ptr<ASMTAnimationParameters> animationParameters;

    };
}

