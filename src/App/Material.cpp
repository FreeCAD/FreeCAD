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
# include <cstring>
#endif

#include "Material.h"

using namespace App;


//===========================================================================
// Material
//===========================================================================
Material::Material()
  : shininess{0.2000f}
  , transparency{}
{
    setType(STEEL);
    setType(USER_DEFINED);
}

Material::Material(const char* MatName)
  : shininess{0.2000f}
  , transparency{}
{
    set(MatName);
}

Material::Material(const MaterialType MatType)
  : shininess{0.2000f}
  , transparency{}
{
    setType(MatType);
}

void Material::set(const char* MatName)
{
    if (strcmp("Brass",MatName) == 0 ) {
        setType(BRASS);
    }
    else if (strcmp("Bronze",MatName) == 0 ) {
        setType(BRONZE);
    }
    else if (strcmp("Copper",MatName) == 0 ) {
        setType(COPPER);
    }
    else if (strcmp("Gold",MatName) == 0 ) {
//      ambientColor.set(0.3f,0.1f,0.1f);
//    diffuseColor.set(0.8f,0.7f,0.2f);
//    specularColor.set(0.4f,0.3f,0.1f);
//    shininess = .4f;
//    transparency = .0f;
////    ambientColor.set(0.3f,0.1f,0.1f);
////    diffuseColor.set(0.22f,0.15f,0.00f);
////    specularColor.set(0.71f,0.70f,0.56f);
////    shininess = .16f;
////    transparency = .0f;
////    ambientColor.set(0.24725f, 0.1995f, 0.0745f);
////    diffuseColor.set(0.75164f, 0.60648f, 0.22648f);
////    specularColor.set(0.628281f, 0.555802f, 0.366065f);
////    shininess = .16f;
////    transparency = .0f;
        setType(GOLD);
    }
    else if (strcmp("Pewter",MatName) == 0 ) {
        setType(PEWTER);
    }
    else if (strcmp("Plaster",MatName) == 0 ) {
        setType(PLASTER);
    }
    else if (strcmp("Plastic",MatName) == 0 ) {
        setType(PLASTIC);
    }
    else if (strcmp("Silver",MatName) == 0 ) {
        setType(SILVER);
    }
    else if (strcmp("Steel",MatName) == 0 ) {
        setType(STEEL);
    }
    else if (strcmp("Stone",MatName) == 0 ) {
//    ambientColor.set(0.0f,0.0f,0.0f);
//    diffuseColor.set(0.0f,0.0f,0.0f);
//    specularColor.set(0.4f,0.3f,0.1f);
//    shininess = .4f;
//    transparency = .0f;
        setType(STONE);
    }
    else if (strcmp("Shiny plastic",MatName) == 0 ) {
        setType(SHINY_PLASTIC);
    }
    else if (strcmp("Satin",MatName) == 0 ) {
        setType(SATIN);
    }
    else if (strcmp("Metalized",MatName) == 0 ) {
        setType(METALIZED);
    }
    else if (strcmp("Neon GNC",MatName) == 0 ) {
        setType(NEON_GNC);
    }
    else if (strcmp("Chrome",MatName) == 0 ) {
        setType(CHROME);
    }
    else if (strcmp("Aluminium",MatName) == 0 ) {
        setType(ALUMINIUM);
    }
    else if (strcmp("Obsidian",MatName) == 0 ) {
        setType(OBSIDIAN);
    }
    else if (strcmp("Neon PHC",MatName) == 0 ) {
        setType(NEON_PHC);
    }
    else if (strcmp("Jade",MatName) == 0 ) {
        setType(JADE);
    }
    else if (strcmp("Ruby",MatName) == 0 ) {
        setType(RUBY);
    }
    else if (strcmp("Emerald",MatName) == 0 ) {
        setType(EMERALD);
    }
    else if (strcmp("Default",MatName) == 0 ) {
        setType(DEFAULT);
    }
    else {
        setType(USER_DEFINED);
    }
}

void Material::setType(const MaterialType MatType)
{
    _matType = MatType;
    switch (MatType)
    {
    case BRASS:
        ambientColor .set(0.3294f,0.2235f,0.0275f);
        diffuseColor .set(0.7804f,0.5686f,0.1137f);
        specularColor.set(0.9922f,0.9412f,0.8078f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.2179f;
        transparency = 0.0000f;
        break;
    case BRONZE:
        ambientColor .set(0.2125f,0.1275f,0.0540f);
        diffuseColor .set(0.7140f,0.4284f,0.1814f);
        specularColor.set(0.3935f,0.2719f,0.1667f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.2000f;
        transparency = 0.0000f;
        break;
    case COPPER:
        ambientColor .set(0.3300f,0.2600f,0.2300f);
        diffuseColor .set(0.5000f,0.1100f,0.0000f);
        specularColor.set(0.9500f,0.7300f,0.0000f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.9300f;
        transparency = 0.0000f;
        break;
    case GOLD:
        ambientColor .set(0.3000f,0.2306f,0.0953f);
        diffuseColor .set(0.4000f,0.2760f,0.0000f);
        specularColor.set(0.9000f,0.8820f,0.7020f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.0625f;
        transparency = 0.0000f;
        break;
    case PEWTER:
        ambientColor .set(0.1059f,0.0588f,0.1137f);
        diffuseColor .set(0.4275f,0.4706f,0.5412f);
        specularColor.set(0.3333f,0.3333f,0.5216f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.0769f;
        transparency = 0.0000f;
        break;
    case PLASTER:
        ambientColor .set(0.0500f,0.0500f,0.0500f);
        diffuseColor .set(0.1167f,0.1167f,0.1167f);
        specularColor.set(0.0305f,0.0305f,0.0305f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.0078f;
        transparency = 0.0000f;
        break;
    case PLASTIC:
        ambientColor .set(0.1000f,0.1000f,0.1000f);
        diffuseColor .set(0.0000f,0.0000f,0.0000f);
        specularColor.set(0.0600f,0.0600f,0.0600f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.0078f;
        transparency = 0.0000f;
        break;
    case SILVER:
        ambientColor .set(0.1922f,0.1922f,0.1922f);
        diffuseColor .set(0.5075f,0.5075f,0.5075f);
        specularColor.set(0.5083f,0.5083f,0.5083f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.2000f;
        transparency = 0.0000f;
        break;
    case STEEL:
        ambientColor .set(0.0020f,0.0020f,0.0020f);
        diffuseColor .set(0.0000f,0.0000f,0.0000f);
        specularColor.set(0.9800f,0.9800f,0.9800f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.0600f;
        transparency = 0.0000f;
        break;
    case STONE:
        ambientColor .set(0.1900f,0.1520f,0.1178f);
        diffuseColor .set(0.7500f,0.6000f,0.4650f);
        specularColor.set(0.0784f,0.0800f,0.0480f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.1700f;
        transparency = 0.0000f;
        break;
    case SHINY_PLASTIC:
        ambientColor .set(0.0880f,0.0880f,0.0880f);
        diffuseColor .set(0.0000f,0.0000f,0.0000f);
        specularColor.set(1.0000f,1.0000f,1.0000f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 1.0000f;
        transparency = 0.0000f;
        break;
    case SATIN:
        ambientColor .set(0.0660f,0.0660f,0.0660f);
        diffuseColor .set(0.0000f,0.0000f,0.0000f);
        specularColor.set(0.4400f,0.4400f,0.4400f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.0938f;
        transparency = 0.0000f;
        break;
    case METALIZED:
        ambientColor .set(0.1800f,0.1800f,0.1800f);
        diffuseColor .set(0.0000f,0.0000f,0.0000f);
        specularColor.set(0.4500f,0.4500f,0.4500f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.1300f;
        transparency = 0.0000f;
        break;
    case NEON_GNC:
        ambientColor .set(0.2000f,0.2000f,0.2000f);
        diffuseColor .set(0.0000f,0.0000f,0.0000f);
        specularColor.set(0.6200f,0.6200f,0.6200f);
        emissiveColor.set(1.0000f,1.0000f,0.0000f);
        shininess    = 0.0500f;
        transparency = 0.0000f;
        break;
    case CHROME:
        ambientColor .set(0.3500f,0.3500f,0.3500f);
        diffuseColor .set(0.9176f,0.9176f,0.9176f);
        specularColor.set(0.9746f,0.9746f,0.9746f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.1000f;
        transparency = 0.0000f;
        break;
    case ALUMINIUM:
        ambientColor .set(0.3000f,0.3000f,0.3000f);
        diffuseColor .set(0.3000f,0.3000f,0.3000f);
        specularColor.set(0.7000f,0.7000f,0.8000f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.0900f;
        transparency = 0.0000f;
        break;
    case OBSIDIAN:
        ambientColor .set(0.0538f,0.0500f,0.0662f);
        diffuseColor .set(0.1828f,0.1700f,0.2253f);
        specularColor.set(0.3327f,0.3286f,0.3464f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.3000f;
        transparency = 0.0000f;
        break;
    case NEON_PHC:
        ambientColor .set(1.0000f,1.0000f,1.0000f);
        diffuseColor .set(1.0000f,1.0000f,1.0000f);
        specularColor.set(0.6200f,0.6200f,0.6200f);
        emissiveColor.set(0.0000f,0.9000f,0.4140f);
        shininess    = 0.0500f;
        transparency = 0.0000f;
        break;
    case JADE:
        ambientColor .set(0.1350f,0.2225f,0.1575f);
        diffuseColor .set(0.5400f,0.8900f,0.6300f);
        specularColor.set(0.3162f,0.3162f,0.3162f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.1000f;
        transparency = 0.0000f;
        break;
    case RUBY:
        ambientColor .set(0.1745f,0.0118f,0.0118f);
        diffuseColor .set(0.6142f,0.0414f,0.0414f);
        specularColor.set(0.7278f,0.6279f,0.6267f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.6000f;
        transparency = 0.0000f;
        break;
    case EMERALD:
        ambientColor .set(0.0215f,0.1745f,0.0215f);
        diffuseColor .set(0.0757f,0.6142f,0.0757f);
        specularColor.set(0.6330f,0.7278f,0.6330f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.6000f;
        transparency = 0.0000f;
        break;
    case USER_DEFINED:
        break;
    default:
        ambientColor .set(0.2000f,0.2000f,0.2000f);
        diffuseColor .set(0.8000f,0.8000f,0.8000f);
        specularColor.set(0.0000f,0.0000f,0.0000f);
        emissiveColor.set(0.0000f,0.0000f,0.0000f);
        shininess    = 0.2000f;
        transparency = 0.0000f;
        break;
    }
}
