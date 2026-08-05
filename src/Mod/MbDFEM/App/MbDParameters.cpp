// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDParameters.h"

PROPERTY_SOURCE(MbDFEM::MbDGravity, App::DocumentObject)
PROPERTY_SOURCE(MbDFEM::MbDSimulationParameters, App::DocumentObject)
PROPERTY_SOURCE(MbDFEM::MbDAnimationParameters, App::DocumentObject)

const char* MbDFEM::MbDSimulationParameters::SolverTypeEnums[] = {
    "RK4",
    "DASSL",
    "Euler",
    nullptr,
};

MbDFEM::MbDGravity::MbDGravity()
{
    ADD_PROPERTY_TYPE(gravity,
                      (Base::Vector3d(0.0, 0.0, -9.81)),
                      "MbDFEM",
                      App::Prop_None,
                      "Gravity acceleration vector for this assembly");
}

MbDFEM::MbDSimulationParameters::MbDSimulationParameters()
{
    ADD_PROPERTY_TYPE(startTime, (0.0), "MbDFEM", App::Prop_None, "Simulation start time");
    ADD_PROPERTY_TYPE(endTime, (1.0), "MbDFEM", App::Prop_None, "Simulation end time");
    ADD_PROPERTY_TYPE(stepSize, (0.001), "MbDFEM", App::Prop_None, "Simulation integration step size");
    ADD_PROPERTY_TYPE(solverType, ((long)0), "MbDFEM", App::Prop_None, "Simulation solver type");
    solverType.setEnums(SolverTypeEnums);
    ADD_PROPERTY_TYPE(significantDigits,
                      (6),
                      "MbDFEM",
                      App::Prop_None,
                      "Number of significant digits used for simulation accuracy");
    ADD_PROPERTY_TYPE(maxIterations, (100), "MbDFEM", App::Prop_None, "Maximum solver iterations");
    ADD_PROPERTY_TYPE(outputInterval, (0.01), "MbDFEM", App::Prop_None, "Simulation output interval");
}

MbDFEM::MbDAnimationParameters::MbDAnimationParameters()
{
    ADD_PROPERTY_TYPE(frameRate, (30), "MbDFEM", App::Prop_None, "Animation frame rate");
    ADD_PROPERTY_TYPE(playbackSpeed, (1.0), "MbDFEM", App::Prop_None, "Animation playback speed");
    ADD_PROPERTY_TYPE(loop, (true), "MbDFEM", App::Prop_None, "Loop animation playback");
    ADD_PROPERTY_TYPE(showTrails, (false), "MbDFEM", App::Prop_None, "Show animation trails");
    ADD_PROPERTY_TYPE(trailLength, (60), "MbDFEM", App::Prop_None, "Number of trail frames to display");
    ADD_PROPERTY_TYPE(interpolateFrames,
                      (true),
                      "MbDFEM",
                      App::Prop_None,
                      "Interpolate animation frames");
}
