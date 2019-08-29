/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2005     *
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

#ifdef __GNUC__
# include <stdint.h>
#endif

#include <sstream>
#include <iomanip>

namespace App
{

/** Color class
 */
class AppExport Color
{
public:
    /**
     * Defines the color as (R,G,B,A) whereas all values are in the range [0,1].
     * \a A defines the transparency.
     */
    explicit Color(float R=0.0,float G=0.0, float B=0.0, float A=0.0)
      :r(R),g(G),b(B),a(A){}
    /**
     * Does basically the same as the constructor above unless that (R,G,B,A) is
     * encoded as an unsigned int.
     */
    Color(uint32_t rgba)
    { setPackedValue( rgba ); }
    /** Copy constructor. */
    Color(const Color& c)
      :r(c.r),g(c.g),b(c.b),a(c.a){}
    /** Returns true if both colors are equal. Therefore all components must be equal. */
    bool operator==(const Color& c) const
    {
        return getPackedValue() == c.getPackedValue();
        //return (c.r==r && c.g==g && c.b==b && c.a==a);
    }
    bool operator!=(const Color& c) const
    {
        return !operator==(c);
    }
    /**
     * Defines the color as (R,G,B,A) whereas all values are in the range [0,1].
     * \a A defines the transparency, 0 means complete opaque and 1 invisible.
     */
    void set(float R,float G, float B, float A=0.0)
    {
        r=R;g=G;b=B;a=A;
    }
    Color& operator=(const Color& c)
    {
        r=c.r;g=c.g;b=c.b;a=c.a;
        return *this;
    }
    /**
     * Sets the color value as a 32 bit combined red/green/blue/alpha value.
     * Each component is 8 bit wide (i.e. from 0x00 to 0xff), and the red
     * value should be stored leftmost, like this: 0xRRGGBBAA.
     *
     * \sa getPackedValue().
     */
    Color& setPackedValue(uint32_t rgba)
    {
        this->set((rgba >> 24)/255.0f,
                 ((rgba >> 16)&0xff)/255.0f,
                 ((rgba >> 8)&0xff)/255.0f,
                 (rgba&0xff)/255.0f);
        return *this;
    }
    /**
     * Returns color as a 32 bit packed unsigned int in the form 0xRRGGBBAA.
     *
     *  \sa setPackedValue().
     */
    uint32_t getPackedValue() const
    {
        return ((uint32_t)(r*255.0f + 0.5f) << 24 |
                (uint32_t)(g*255.0f + 0.5f) << 16 |
                (uint32_t)(b*255.0f + 0.5f) << 8  |
                (uint32_t)(a*255.0f + 0.5f));
    }
    /**
     * creates FC Color from template type, e.g. Qt QColor
     */
    template <typename T>
    void setValue(const T& q)
    { set(q.redF(),q.greenF(),q.blueF()); }
    /**
     * returns a template type e.g. Qt color equivalent to FC color
     *
     */
    template <typename T>
    inline T asValue(void) const {
        return(T(int(r*255.0),int(g*255.0),int(b*255.0)));
    }
    /**
     * returns color as hex color "#RRGGBB"
     *
     */
    std::string asHexString() const {
        std::stringstream ss;
        ss << "#" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << int(r*255.0)
                                                                     << std::setw(2) << int(g*255.0)
                                                                     << std::setw(2) << int(b*255.0);
        return ss.str();
    }
    /**
     * \deprecated
     */
    std::string asCSSString() const {
        return asHexString();
    }
    /**
     * gets color from hex color "#RRGGBB"
     *
     */
    bool fromHexString(const std::string& hex) {
        if (hex.size() < 7 || hex[0] != '#')
            return false;
        // #RRGGBB
        if (hex.size() == 7) {
            std::stringstream ss(hex);
            unsigned int rgb;
            char c;

            ss >> c >> std::hex >> rgb;
            int rc = (rgb >> 16) & 0xff;
            int gc = (rgb >> 8) & 0xff;
            int bc = rgb & 0xff;

            r = rc / 255.0f;
            g = gc / 255.0f;
            b = bc / 255.0f;

            return true;
        }
        // #RRGGBBAA
        if (hex.size() == 9) {
            std::stringstream ss(hex);
            unsigned int rgba;
            char c;

            ss >> c >> std::hex >> rgba;
            int rc = (rgba >> 24) & 0xff;
            int gc = (rgba >> 16) & 0xff;
            int bc = (rgba >> 8) & 0xff;
            int ac = rgba & 0xff;

            r = rc / 255.0f;
            g = gc / 255.0f;
            b = bc / 255.0f;
            a = ac / 255.0f;

            return true;
        }

        return false;
    }

    /// color values, public accessible
    float r,g,b,a;
};

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
    Material(void);
    /** Defines the colors and shininess for the material \a MatName. If \a MatName isn't defined then USER_DEFINED is
     * set and the user must define the colors itself.
     */
    Material(const char* MatName);
    /** Does basically the same as the constructor above unless that it accepts a MaterialType as argument. */
    Material(const MaterialType MatType);
    //@}
    ~Material();

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
        return _matType!=m._matType || shininess!=m.shininess ||
            transparency!=m.transparency || ambientColor!=m.ambientColor ||
            diffuseColor!=m.diffuseColor || specularColor!=m.specularColor ||
            emissiveColor!=m.emissiveColor;
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
