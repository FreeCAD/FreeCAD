/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "ModelUuids.h"

using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelUUIDs, Base::BaseClass)

const QString ModelUUIDs::ModelUUID_Legacy_Father =
    QStringLiteral("9cdda8b6-b606-4778-8f13-3934d8668e67");
const QString ModelUUIDs::ModelUUID_Legacy_MaterialStandard =
    QStringLiteral("1e2c0088-904a-4537-925f-64064c07d700");

const QString ModelUUIDs::ModelUUID_Machining_Machinability =
    QStringLiteral("9d81fcb2-bf81-48e3-bb57-d45ecf380096");

const QString ModelUUIDs::ModelUUID_Mechanical_ArrudaBoyce =
    QStringLiteral("e10d00de-c7de-4e59-bcdd-058c2ea19ec6");
const QString ModelUUIDs::ModelUUID_Mechanical_Density =
    QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af");
const QString ModelUUIDs::ModelUUID_Mechanical_Hardness =
    QStringLiteral("3d1a6141-d032-4d82-8bb5-a8f339fff8ad");
const QString ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic =
    QStringLiteral("f6f9e48c-b116-4e82-ad7f-3659a9219c50");
const QString ModelUUIDs::ModelUUID_Mechanical_LinearElastic =
    QStringLiteral("7b561d1d-fb9b-44f6-9da9-56a4f74d7536");
const QString ModelUUIDs::ModelUUID_Mechanical_MooneyRivlin =
    QStringLiteral("beeed169-7770-4da0-ab67-c9172cf7d23d");
const QString ModelUUIDs::ModelUUID_Mechanical_NeoHooke =
    QStringLiteral("569ebc58-ef29-434a-83be-555a0980d505");
const QString ModelUUIDs::ModelUUID_Mechanical_OgdenN1 =
    QStringLiteral("a2634a2c-412f-468d-9bec-74ae5d87a9c0");
const QString ModelUUIDs::ModelUUID_Mechanical_OgdenN2 =
    QStringLiteral("233540bb-7b13-4f49-ac12-126a5c82cedf");
const QString ModelUUIDs::ModelUUID_Mechanical_OgdenN3 =
    QStringLiteral("a917d6b8-209f-429e-9972-fe4bbb97af3f");
const QString ModelUUIDs::ModelUUID_Mechanical_OgdenYld2004p18 =
    QStringLiteral("3ef9e427-cc25-43f7-817f-79ff0d49625f");
const QString ModelUUIDs::ModelUUID_Mechanical_OrthotropicLinearElastic =
    QStringLiteral("b19ccc6b-a431-418e-91c2-0ac8c649d146");
const QString ModelUUIDs::ModelUUID_Mechanical_PolynomialN1 =
    QStringLiteral("285a6042-0f0c-4a36-a898-4afadd6408ce");
const QString ModelUUIDs::ModelUUID_Mechanical_PolynomialN2 =
    QStringLiteral("4c2fb7b2-5121-4d6f-be0d-8c5970c9e682");
const QString ModelUUIDs::ModelUUID_Mechanical_PolynomialN3 =
    QStringLiteral("e83ada22-947e-4beb-91e7-482a16f5ba77");
const QString ModelUUIDs::ModelUUID_Mechanical_ReducedPolynomialN1 =
    QStringLiteral("f8052a3c-db17-42ea-b2be-13aa5ef30730");
const QString ModelUUIDs::ModelUUID_Mechanical_ReducedPolynomialN2 =
    QStringLiteral("c52b5021-4bb8-441c-80d4-855fce9de15e");
const QString ModelUUIDs::ModelUUID_Mechanical_ReducedPolynomialN3 =
    QStringLiteral("fa4e58b4-74c7-4292-8e79-7d5fd232fb55");
const QString ModelUUIDs::ModelUUID_Mechanical_Yeoh =
    QStringLiteral("cd13c492-21a9-4578-8191-deec003e4c01");

const QString ModelUUIDs::ModelUUID_Fluid_Default =
    QStringLiteral("1ae66d8c-1ba1-4211-ad12-b9917573b202");

const QString ModelUUIDs::ModelUUID_Thermal_Default =
    QStringLiteral("9959d007-a970-4ea7-bae4-3eb1b8b883c7");

const QString ModelUUIDs::ModelUUID_Electromagnetic_Default =
    QStringLiteral("b2eb5f48-74b3-4193-9fbb-948674f427f3");

const QString ModelUUIDs::ModelUUID_Architectural_Default =
    QStringLiteral("32439c3b-262f-4b7b-99a8-f7f44e5894c8");
const QString ModelUUIDs::ModelUUID_Rendering_Architectural =
    QStringLiteral("27e48ac9-54e1-4a1f-aa49-d5d690242705");

const QString ModelUUIDs::ModelUUID_Costs_Default =
    QStringLiteral("881df808-8726-4c2e-be38-688bb6cce466");

const QString ModelUUIDs::ModelUUID_Rendering_Basic =
    QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e");
const QString ModelUUIDs::ModelUUID_Rendering_Texture =
    QStringLiteral("bbdcc65b-67ca-489c-bd5c-a36e33d1c160");
const QString ModelUUIDs::ModelUUID_Rendering_Advanced =
    QStringLiteral("c880f092-cdae-43d6-a24b-55e884aacbbf");
const QString ModelUUIDs::ModelUUID_Rendering_Vector =
    QStringLiteral("fdf5a80e-de50-4157-b2e5-b6e5f88b680e");

const QString ModelUUIDs::ModelUUID_Render_Appleseed =
    QStringLiteral("b0a10f70-13bf-4598-ab63-bcfbbcd813e3");
const QString ModelUUIDs::ModelUUID_Render_Carpaint =
    QStringLiteral("4d2cc163-0707-40e2-a9f7-14288c4b97bd");
const QString ModelUUIDs::ModelUUID_Render_Cycles =
    QStringLiteral("a6da1b66-929c-48bf-ae80-3b0495c7b50b");
const QString ModelUUIDs::ModelUUID_Render_Diffuse =
    QStringLiteral("c19b2d30-c55b-48aa-a938-df9e2f7779cf");
const QString ModelUUIDs::ModelUUID_Render_Disney =
    QStringLiteral("f8723572-4470-4c39-a749-6d3b71358a5b");
const QString ModelUUIDs::ModelUUID_Render_Emission =
    QStringLiteral("9f6cb588-c89d-4a74-9d0f-2786a8568cec");
const QString ModelUUIDs::ModelUUID_Render_Glass =
    QStringLiteral("d76a56f5-7250-4efb-bb89-8ea0a9ccaa6b");
const QString ModelUUIDs::ModelUUID_Render_Luxcore =
    QStringLiteral("6b992304-33e0-490b-a391-e9d0af79bb69");
const QString ModelUUIDs::ModelUUID_Render_Luxrender =
    QStringLiteral("67ac6a63-e173-4e05-898b-af743f1f9563");
const QString ModelUUIDs::ModelUUID_Render_Mixed =
    QStringLiteral("84bab333-984f-47fe-a512-d17c7cb2daa9");
const QString ModelUUIDs::ModelUUID_Render_Ospray =
    QStringLiteral("a4792c23-0be9-47c2-b16d-47b2d2d5efd6");
const QString ModelUUIDs::ModelUUID_Render_Pbrt =
    QStringLiteral("35b34b82-4325-4d27-97bd-d10bb2c56586");
const QString ModelUUIDs::ModelUUID_Render_Povray =
    QStringLiteral("6ec8b415-4c7b-4206-a80b-2ea64101f34b");
const QString ModelUUIDs::ModelUUID_Render_SubstancePBR =
    QStringLiteral("f212b643-db96-452e-8428-376a4534e5ab");
const QString ModelUUIDs::ModelUUID_Render_Texture =  // ???
    QStringLiteral("fc9b6135-95cd-4ba8-ad9a-0972caeebad2");
const QString ModelUUIDs::ModelUUID_RenderWB =
    QStringLiteral("344008be-a837-43af-90bc-f795f277b309");

const QString ModelUUIDs::ModelUUID_Test_Model =
    QStringLiteral("34d0583d-f999-49ba-99e6-aa40bd5c3a6b");
