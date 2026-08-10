// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDFEMExport MbDGravity: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDGravity);

public:
    MbDGravity();
    ~MbDGravity() override = default;

    App::PropertyVector gravity;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDGravity";
    }
};

class MbDFEMExport MbDSimulationParameters: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDSimulationParameters);

public:
    MbDSimulationParameters();
    ~MbDSimulationParameters() override = default;

    App::PropertyFloat startTime;
    App::PropertyFloat endTime;
    App::PropertyFloat maxStepSize;
    App::PropertyFloat minStepSize;
    App::PropertyEnumeration solverType;
    App::PropertyInteger significantDigits;
    App::PropertyInteger maxIterations;
    App::PropertyFloat outputInterval;

    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderDocumentObject";
    }

private:
    static const char* SolverTypeEnums[];
};

class MbDFEMExport MbDAnimationParameters: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDAnimationParameters);

public:
    MbDAnimationParameters();
    ~MbDAnimationParameters() override = default;

    App::PropertyInteger frameRate;
    App::PropertyFloat playbackSpeed;
    App::PropertyBool loop;
    App::PropertyBool showTrails;
    App::PropertyInteger trailLength;
    App::PropertyBool interpolateFrames;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDAnimationParameters";
    }
};

}  // namespace MbDFEM
