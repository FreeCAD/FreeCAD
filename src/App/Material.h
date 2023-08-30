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


#ifndef APP_MATERIAL_H
#define APP_MATERIAL_H

#include <App/Color.h>

namespace App
{

/** Material class
 */
class AppExport Material
{
public:
    enum MaterialType {
        BRASS,
        BRONZE,
        COPPER,
        GOLD,
        PEWTER,
        PLASTER,
        PLASTIC,
        SILVER,
        STEEL,
        STONE,
        SHINY_PLASTIC,
        SATIN,
        METALIZED,
        NEON_GNC,
        CHROME,
        ALUMINIUM,
        OBSIDIAN,
        NEON_PHC,
        JADE,
        RUBY,
        EMERALD,
        DEFAULT,
        USER_DEFINED
    };

public:
    /** @name Constructors
     */
    //@{
    /** Sets the USER_DEFINED material type. The user must set the colors afterwards. */
    Material();
    /** Defines the colors and shininess for the material \a MatName. If \a MatName isn't defined then USER_DEFINED is
     * set and the user must define the colors itself.
     */
    explicit Material(const char* MatName);
    /** Does basically the same as the constructor above unless that it accepts a MaterialType as argument. */
    explicit Material(const MaterialType MatType);
    //@}

    /** Set a material by name
     *  There are some standard materials defined which are:
     *  \li Brass
     *  \li Bronze
     *  \li Copper
     *  \li Gold
     *  \li Pewter
     *  \li Plaster
     *  \li Plastic
     *  \li Silver
     *  \li Steel
     *  \li Stone
     *  \li Shiny plastic
     *  \li Satin
     *  \li Metalized
     *  \li Neon GNC
     *  \li Chrome
     *  \li Aluminium
     *  \li Obsidian
     *  \li Neon PHC
     *  \li Jade
     *  \li Ruby
     *  \li Emerald
     * Furthermore there two additional modes \a Default which defines a kind of grey metallic and user defined that
     * does nothing.
     * The Color and the other properties of the material are defined in the range [0-1].
     * If \a MatName is an unknown material name then the type USER_DEFINED is set and the material doesn't get changed.
     */
    void set(const char* MatName);
    /**
     * This method is provided for convenience which does basically the same as the method above unless that it accepts a MaterialType
     * as argument.
     */
    void setType(const MaterialType MatType);
    /**
     * Returns the currently set material type.
     */
    MaterialType getType() const
    { return _matType; }

    /** @name Properties */
    //@{
    Color ambientColor;  /**< Defines the ambient color. */
    Color diffuseColor;  /**< Defines the diffuse color. */
    Color specularColor; /**< Defines the specular color. */
    Color emissiveColor; /**< Defines the emissive color. */
    float shininess;
    float transparency;
    //@}

    bool operator==(const Material& m) const
    {
        return _matType==m._matType && shininess==m.shininess &&
            transparency==m.transparency && ambientColor==m.ambientColor &&
            diffuseColor==m.diffuseColor && specularColor==m.specularColor &&
            emissiveColor==m.emissiveColor;
    }
    bool operator!=(const Material& m) const
    {
        return !operator==(m);
    }

private:
    MaterialType _matType;
};

} //namespace App

#endif // APP_MATERIAL_H
