// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObjectGroup.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDFEMExport MbDAssembliesFolder: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDAssembliesFolder);

public:
    MbDAssembliesFolder() = default;
    ~MbDAssembliesFolder() override = default;

    bool allowObject(App::DocumentObject* object) override;

    bool allowDuplicateLabel() const override
    {
        return true;
    }
};

class MbDFEMExport MbDPartsFolder: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDPartsFolder);

public:
    MbDPartsFolder() = default;
    ~MbDPartsFolder() override = default;

    bool allowObject(App::DocumentObject* object) override;

    bool allowDuplicateLabel() const override
    {
        return true;
    }
};

class MbDFEMExport MbDMarkersFolder: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDMarkersFolder);

public:
    MbDMarkersFolder() = default;
    ~MbDMarkersFolder() override = default;

    bool allowObject(App::DocumentObject* object) override;

    bool allowDuplicateLabel() const override
    {
        return true;
    }
};

class MbDFEMExport MbDJointsFolder: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDJointsFolder);

public:
    MbDJointsFolder() = default;
    ~MbDJointsFolder() override = default;

    bool allowObject(App::DocumentObject* object) override;

    bool allowDuplicateLabel() const override
    {
        return true;
    }
};

class MbDFEMExport MbDMotionsFolder: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDMotionsFolder);

public:
    MbDMotionsFolder() = default;
    ~MbDMotionsFolder() override = default;

    bool allowObject(App::DocumentObject* object) override;

    bool allowDuplicateLabel() const override
    {
        return true;
    }
};

class MbDFEMExport MbDActionsFolder: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDActionsFolder);

public:
    MbDActionsFolder() = default;
    ~MbDActionsFolder() override = default;

    bool allowObject(App::DocumentObject* object) override;

    bool allowDuplicateLabel() const override
    {
        return true;
    }
};

}  // namespace MbDFEM
