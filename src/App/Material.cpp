/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#include <cstring>
#include <random>
#endif

#include "Application.h"
#include "Material.h"

#include <Base/Tools.h>

using namespace App;


// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
//===========================================================================
// Material
//===========================================================================
Material::Material()
    : shininess {0.9000F}
    , transparency {}
    , _matType {}
{
    setType(DEFAULT);
}

Material::Material(const char* MatName)
    : shininess {0.9000F}
    , transparency {}
    , _matType {}
{
    set(MatName);
}

Material::Material(MaterialType MatType)
    : shininess {0.9000F}
    , transparency {}
    , _matType {}
{
    setType(MatType);
}

void Material::set(const char* MatName)
{
    if (strcmp("Brass", MatName) == 0) {
        setType(BRASS);
    }
    else if (strcmp("Bronze", MatName) == 0) {
        setType(BRONZE);
    }
    else if (strcmp("Copper", MatName) == 0) {
        setType(COPPER);
    }
    else if (strcmp("Gold", MatName) == 0) {
        setType(GOLD);
    }
    else if (strcmp("Pewter", MatName) == 0) {
        setType(PEWTER);
    }
    else if (strcmp("Plaster", MatName) == 0) {
        setType(PLASTER);
    }
    else if (strcmp("Plastic", MatName) == 0) {
        setType(PLASTIC);
    }
    else if (strcmp("Silver", MatName) == 0) {
        setType(SILVER);
    }
    else if (strcmp("Steel", MatName) == 0) {
        setType(STEEL);
    }
    else if (strcmp("Stone", MatName) == 0) {
        setType(STONE);
    }
    else if (strcmp("Shiny plastic", MatName) == 0) {
        setType(SHINY_PLASTIC);
    }
    else if (strcmp("Satin", MatName) == 0) {
        setType(SATIN);
    }
    else if (strcmp("Metalized", MatName) == 0) {
        setType(METALIZED);
    }
    else if (strcmp("Neon GNC", MatName) == 0) {
        setType(NEON_GNC);
    }
    else if (strcmp("Chrome", MatName) == 0) {
        setType(CHROME);
    }
    else if (strcmp("Aluminium", MatName) == 0) {
        setType(ALUMINIUM);
    }
    else if (strcmp("Obsidian", MatName) == 0) {
        setType(OBSIDIAN);
    }
    else if (strcmp("Neon PHC", MatName) == 0) {
        setType(NEON_PHC);
    }
    else if (strcmp("Jade", MatName) == 0) {
        setType(JADE);
    }
    else if (strcmp("Ruby", MatName) == 0) {
        setType(RUBY);
    }
    else if (strcmp("Emerald", MatName) == 0) {
        setType(EMERALD);
    }
    else if (strcmp("Default", MatName) == 0) {
        setType(DEFAULT);
    }
    else {
        setType(USER_DEFINED);
    }
}

void Material::setType(MaterialType MatType)
{
    _matType = MatType;
    switch (MatType) {
        case BRASS:
            ambientColor.set(0.3294F, 0.2235F, 0.0275F);
            diffuseColor.set(0.7804F, 0.5686F, 0.1137F);
            specularColor.set(0.9922F, 0.9412F, 0.8078F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.2179F;
            transparency = 0.0000F;
            break;
        case BRONZE:
            ambientColor.set(0.2125F, 0.1275F, 0.0540F);
            diffuseColor.set(0.7140F, 0.4284F, 0.1814F);
            specularColor.set(0.3935F, 0.2719F, 0.1667F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.2000F;
            transparency = 0.0000F;
            break;
        case COPPER:
            ambientColor.set(0.3300F, 0.2600F, 0.2300F);
            diffuseColor.set(0.5000F, 0.1100F, 0.0000F);
            specularColor.set(0.9500F, 0.7300F, 0.0000F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.9300F;
            transparency = 0.0000F;
            break;
        case GOLD:
            ambientColor.set(0.3000F, 0.2306F, 0.0953F);
            diffuseColor.set(0.4000F, 0.2760F, 0.0000F);
            specularColor.set(0.9000F, 0.8820F, 0.7020F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.0625F;
            transparency = 0.0000F;
            break;
        case PEWTER:
            ambientColor.set(0.1059F, 0.0588F, 0.1137F);
            diffuseColor.set(0.4275F, 0.4706F, 0.5412F);
            specularColor.set(0.3333F, 0.3333F, 0.5216F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.0769F;
            transparency = 0.0000F;
            break;
        case PLASTER:
            ambientColor.set(0.0500F, 0.0500F, 0.0500F);
            diffuseColor.set(0.1167F, 0.1167F, 0.1167F);
            specularColor.set(0.0305F, 0.0305F, 0.0305F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.0078F;
            transparency = 0.0000F;
            break;
        case PLASTIC:
            ambientColor.set(0.1000F, 0.1000F, 0.1000F);
            diffuseColor.set(0.0000F, 0.0000F, 0.0000F);
            specularColor.set(0.0600F, 0.0600F, 0.0600F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.0078F;
            transparency = 0.0000F;
            break;
        case SILVER:
            ambientColor.set(0.1922F, 0.1922F, 0.1922F);
            diffuseColor.set(0.5075F, 0.5075F, 0.5075F);
            specularColor.set(0.5083F, 0.5083F, 0.5083F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.2000F;
            transparency = 0.0000F;
            break;
        case STEEL:
            ambientColor.set(0.0020F, 0.0020F, 0.0020F);
            diffuseColor.set(0.0000F, 0.0000F, 0.0000F);
            specularColor.set(0.9800F, 0.9800F, 0.9800F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.0600F;
            transparency = 0.0000F;
            break;
        case STONE:
            ambientColor.set(0.1900F, 0.1520F, 0.1178F);
            diffuseColor.set(0.7500F, 0.6000F, 0.4650F);
            specularColor.set(0.0784F, 0.0800F, 0.0480F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.1700F;
            transparency = 0.0000F;
            break;
        case SHINY_PLASTIC:
            ambientColor.set(0.0880F, 0.0880F, 0.0880F);
            diffuseColor.set(0.0000F, 0.0000F, 0.0000F);
            specularColor.set(1.0000F, 1.0000F, 1.0000F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 1.0000F;
            transparency = 0.0000F;
            break;
        case SATIN:
            ambientColor.set(0.0660F, 0.0660F, 0.0660F);
            diffuseColor.set(0.0000F, 0.0000F, 0.0000F);
            specularColor.set(0.4400F, 0.4400F, 0.4400F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.0938F;
            transparency = 0.0000F;
            break;
        case METALIZED:
            ambientColor.set(0.1800F, 0.1800F, 0.1800F);
            diffuseColor.set(0.0000F, 0.0000F, 0.0000F);
            specularColor.set(0.4500F, 0.4500F, 0.4500F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.1300F;
            transparency = 0.0000F;
            break;
        case NEON_GNC:
            ambientColor.set(0.2000F, 0.2000F, 0.2000F);
            diffuseColor.set(0.0000F, 0.0000F, 0.0000F);
            specularColor.set(0.6200F, 0.6200F, 0.6200F);
            emissiveColor.set(1.0000F, 1.0000F, 0.0000F);
            shininess = 0.0500F;
            transparency = 0.0000F;
            break;
        case CHROME:
            ambientColor.set(0.3500F, 0.3500F, 0.3500F);
            diffuseColor.set(0.9176F, 0.9176F, 0.9176F);
            specularColor.set(0.9746F, 0.9746F, 0.9746F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.1000F;
            transparency = 0.0000F;
            break;
        case ALUMINIUM:
            ambientColor.set(0.3000F, 0.3000F, 0.3000F);
            diffuseColor.set(0.3000F, 0.3000F, 0.3000F);
            specularColor.set(0.7000F, 0.7000F, 0.8000F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.0900F;
            transparency = 0.0000F;
            break;
        case OBSIDIAN:
            ambientColor.set(0.0538F, 0.0500F, 0.0662F);
            diffuseColor.set(0.1828F, 0.1700F, 0.2253F);
            specularColor.set(0.3327F, 0.3286F, 0.3464F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.3000F;
            transparency = 0.0000F;
            break;
        case NEON_PHC:
            ambientColor.set(1.0000F, 1.0000F, 1.0000F);
            diffuseColor.set(1.0000F, 1.0000F, 1.0000F);
            specularColor.set(0.6200F, 0.6200F, 0.6200F);
            emissiveColor.set(0.0000F, 0.9000F, 0.4140F);
            shininess = 0.0500F;
            transparency = 0.0000F;
            break;
        case JADE:
            ambientColor.set(0.1350F, 0.2225F, 0.1575F);
            diffuseColor.set(0.5400F, 0.8900F, 0.6300F);
            specularColor.set(0.3162F, 0.3162F, 0.3162F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.1000F;
            transparency = 0.0000F;
            break;
        case RUBY:
            ambientColor.set(0.1745F, 0.0118F, 0.0118F);
            diffuseColor.set(0.6142F, 0.0414F, 0.0414F);
            specularColor.set(0.7278F, 0.6279F, 0.6267F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.6000F;
            transparency = 0.0000F;
            break;
        case EMERALD:
            ambientColor.set(0.0215F, 0.1745F, 0.0215F);
            diffuseColor.set(0.0757F, 0.6142F, 0.0757F);
            specularColor.set(0.6330F, 0.7278F, 0.6330F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.6000F;
            transparency = 0.0000F;
            break;
        case USER_DEFINED:
            break;
        default:
            ambientColor.set(0.3333F, 0.3333F, 0.3333F);
            diffuseColor.set(0.8000F, 0.8000F, 0.9000F);
            specularColor.set(0.5333F, 0.5333F, 0.5333F);
            emissiveColor.set(0.0000F, 0.0000F, 0.0000F);
            shininess = 0.9000F;
            transparency = 0.0000F;
            break;
    }
}

App::Material Material::getDefaultAppearance()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    auto getColor = [hGrp](const char* parameter, Base::Color& color) {
        uint32_t packed = color.getPackedRGB();
        packed = hGrp->GetUnsigned(parameter, packed);
        color.setPackedRGB(packed);
    };
    auto intRandom = [](int min, int max) -> int {
        static std::mt19937 generator;
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(generator);
    };

    App::Material mat(App::Material::DEFAULT);
    mat.transparency = Base::fromPercent(hGrp->GetInt("DefaultShapeTransparency", 0));
    long shininess = Base::toPercent(mat.shininess);
    mat.shininess = Base::fromPercent(hGrp->GetInt("DefaultShapeShininess", shininess));

    // This is handled in the material code when using the object appearance
    bool randomColor = hGrp->GetBool("RandomColor", false);

    // diffuse color
    if (randomColor) {
        float red = static_cast<float>(intRandom(0, 255)) / 255.0F;
        float green = static_cast<float>(intRandom(0, 255)) / 255.0F;
        float blue = static_cast<float>(intRandom(0, 255)) / 255.0F;
        mat.diffuseColor = Base::Color(red, green, blue);
    }
    else {
        // Base::Color = (204, 204, 230) = 3435980543UL
        getColor("DefaultShapeColor", mat.diffuseColor);
    }

    getColor("DefaultAmbientColor", mat.ambientColor);
    getColor("DefaultEmissiveColor", mat.emissiveColor);
    getColor("DefaultSpecularColor", mat.specularColor);

    return mat;
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
