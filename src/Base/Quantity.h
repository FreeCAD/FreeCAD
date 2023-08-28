/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef BASE_Quantity_H
#define BASE_Quantity_H

#include "Unit.h"
#include <QString>

// NOLINTBEGIN
#ifndef  DOUBLE_MAX
# define DOUBLE_MAX 1.7976931348623157E+308    /* max decimal value of a "double"*/
#endif
#ifndef  DOUBLE_MIN
# define DOUBLE_MIN 2.2250738585072014E-308    /* min decimal value of a "double"*/
#endif
// NOLINTEND

namespace Base {
class UnitsSchema;

struct BaseExport QuantityFormat {
    enum NumberOption {
        None = 0x00,
        OmitGroupSeparator = 0x01,
        RejectGroupSeparator = 0x02
    };
    enum NumberFormat {
        Default = 0,
        Fixed = 1,
        Scientific = 2
    };

    using NumberOptions = int;
    NumberOptions option;
    NumberFormat format;
    int precision;
    int denominator;

    // Default denominator of minimum fractional inch. Only used in certain
    // schemas.
    // NOLINTNEXTLINE
    static int defaultDenominator; // i.e 8 for 1/8"

    static inline int getDefaultDenominator() {
        return defaultDenominator;
    }

    static inline void setDefaultDenominator(int denom) {
        defaultDenominator = denom;
    }

    inline int getDenominator() const {
        return denominator;
    }

    inline void setDenominator(int denom) {
        denominator = denom;
    }
    QuantityFormat();
    explicit QuantityFormat(NumberFormat format, int decimals=-1);
    inline char toFormat() const {
        switch (format) {
        case Fixed:
            return 'f';
        case Scientific:
            return 'e';
        default:
            return 'g';
        }
    }
    static inline NumberFormat toFormat(char ch, bool* ok = nullptr) {
        if (ok) {
            *ok = true;
        }
        switch (ch) {
        case 'f':
            return Fixed;
        case 'e':
            return Scientific;
        case 'g':
            return Default;
        default:
            if (ok) {
                *ok = false;
            }
            return Default;
        }
    }
};

/**
 * The Quantity class.
 */
class BaseExport Quantity
{
public:
    /// default constructor
    Quantity();
    Quantity(const Quantity&) = default;
    Quantity(Quantity&&) = default;
    explicit Quantity(double value, const Unit& unit=Unit());
    explicit Quantity(double value, const QString& unit);
    /// Destruction
    ~Quantity () = default;

    /** Operators. */
    //@{
    Quantity operator *(const Quantity &other) const;
    Quantity operator *(double factor) const;
    Quantity operator +(const Quantity &other) const;
    Quantity& operator +=(const Quantity &other);
    Quantity operator -(const Quantity &other) const;
    Quantity& operator -=(const Quantity &other);
    Quantity operator -() const;
    Quantity operator /(const Quantity &other) const;
    Quantity operator /(double factor) const;
    bool operator ==(const Quantity&) const;
    bool operator !=(const Quantity&) const;
    bool operator < (const Quantity&) const;
    bool operator > (const Quantity&) const;
    bool operator <= (const Quantity&) const;
    bool operator >= (const Quantity&) const;
    Quantity& operator =(const Quantity&) = default;
    Quantity& operator =(Quantity&&) = default;
    Quantity pow(const Quantity&)const;
    Quantity pow(double)const;
    //@}

    const QuantityFormat& getFormat() const {
        return myFormat;
    }
    void setFormat(const QuantityFormat& fmt) {
        myFormat = fmt;
    }
    /// transfer to user preferred unit/potence
    QString getUserString(double &factor, QString &unitString) const;
    QString getUserString() const { // to satisfy GCC
        double  dummy1{};
        QString dummy2{};
        return getUserString(dummy1, dummy2);
    }
    QString getUserString(UnitsSchema* schema, double &factor, QString &unitString) const;
    QString getSafeUserString() const;

    static Quantity parse(const QString &string);

    /// returns the unit of the quantity
    const Unit & getUnit() const{return myUnit;}
    /// set the unit of the quantity
    void setUnit(const Unit &un){myUnit = un;}
    /// get the Value of the quantity
    double getValue() const{return myValue;}
    /// set the value of the quantity
    void setValue(double val){myValue = val;}
    /** get the Value in a special unit given as quantity.
      * One can use one of the predifeined quantity units in this class
      */
    double getValueAs(const Quantity &)const;


    /// true if it has a number without a unit
    bool isDimensionless()const;
    /// true if it has a specific unit or no dimension.
    bool isDimensionlessOrUnit(const Unit& unit)const;
    /// true if it has a number and a valid unit
    bool isQuantity()const;
    /// true if it has a number with or without a unit
    bool isValid()const;
    /// sets the quantity invalid
    void setInvalid();


    /** Predefined Unit types. */
    //@{
    static const Quantity NanoMetre;
    static const Quantity MicroMetre;
    static const Quantity CentiMetre;
    static const Quantity DeciMetre;
    static const Quantity Metre;
    static const Quantity MilliMetre;
    static const Quantity KiloMetre;

    static const Quantity Liter;
    static const Quantity MilliLiter;

    static const Quantity Hertz;
    static const Quantity KiloHertz;
    static const Quantity MegaHertz;
    static const Quantity GigaHertz;
    static const Quantity TeraHertz;

    static const Quantity MicroGram;
    static const Quantity MilliGram;
    static const Quantity Gram;
    static const Quantity KiloGram;
    static const Quantity Ton;

    static const Quantity Second;
    static const Quantity Minute;
    static const Quantity Hour;

    static const Quantity Ampere;
    static const Quantity MilliAmpere;
    static const Quantity KiloAmpere;
    static const Quantity MegaAmpere;

    static const Quantity Kelvin;
    static const Quantity MilliKelvin;
    static const Quantity MicroKelvin;

    static const Quantity MilliMole;
    static const Quantity Mole;

    static const Quantity Candela;

    static const Quantity Inch;
    static const Quantity Foot;
    static const Quantity Thou;
    static const Quantity Yard;

    static const Quantity Pound;
    static const Quantity Ounce;
    static const Quantity Stone;
    static const Quantity Hundredweights;
    static const Quantity Mile;

    static const Quantity MilePerHour;
    static const Quantity SquareFoot;
    static const Quantity CubicFoot;

    static const Quantity PoundForce;

    static const Quantity Newton;
    static const Quantity MilliNewton;
    static const Quantity KiloNewton;
    static const Quantity MegaNewton;

    static const Quantity NewtonPerMeter;
    static const Quantity MilliNewtonPerMeter;
    static const Quantity KiloNewtonPerMeter;
    static const Quantity MegaNewtonPerMeter;

    static const Quantity Pascal;
    static const Quantity KiloPascal;
    static const Quantity MegaPascal;
    static const Quantity GigaPascal;

    static const Quantity Bar;
    static const Quantity MilliBar;

    static const Quantity Torr;
    static const Quantity mTorr;
    static const Quantity yTorr;

    static const Quantity PSI;
    static const Quantity KSI;
    static const Quantity MPSI;

    static const Quantity Watt;
    static const Quantity MilliWatt;
    static const Quantity KiloWatt;
    static const Quantity VoltAmpere;

    static const Quantity Volt;
    static const Quantity MilliVolt;
    static const Quantity KiloVolt;

    static const Quantity MegaSiemens;
    static const Quantity KiloSiemens;
    static const Quantity Siemens;
    static const Quantity MilliSiemens;
    static const Quantity MicroSiemens;

    static const Quantity Ohm;
    static const Quantity KiloOhm;
    static const Quantity MegaOhm;

    static const Quantity Coulomb;

    static const Quantity Tesla;
    static const Quantity Gauss;

    static const Quantity Weber;

    //static const Quantity Oersted;

    static const Quantity Farad;
    static const Quantity MilliFarad;
    static const Quantity MicroFarad;
    static const Quantity NanoFarad;
    static const Quantity PicoFarad;

    static const Quantity Henry;
    static const Quantity MilliHenry;
    static const Quantity MicroHenry;
    static const Quantity NanoHenry;

    static const Quantity Joule;
    static const Quantity MilliJoule;
    static const Quantity KiloJoule;
    static const Quantity NewtonMeter;
    static const Quantity VoltAmpereSecond;
    static const Quantity WattSecond;
    static const Quantity KiloWattHour;
    static const Quantity ElectronVolt;
    static const Quantity KiloElectronVolt;
    static const Quantity MegaElectronVolt;
    static const Quantity Calorie;
    static const Quantity KiloCalorie;

    static const Quantity KMH;
    static const Quantity MPH;

    static const Quantity Degree;
    static const Quantity Radian;
    static const Quantity Gon;
    static const Quantity AngMinute;
    static const Quantity AngSecond;
    //@}


protected:
    double         myValue;
    Unit           myUnit;
    QuantityFormat myFormat;
};

} // namespace Base

#endif // BASE_Quantity_H
