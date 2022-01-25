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

#ifndef  DOUBLE_MAX
# define DOUBLE_MAX 1.7976931348623157E+308    /* max decimal value of a "double"*/
#endif
#ifndef  DOUBLE_MIN
# define DOUBLE_MIN 2.2250738585072014E-308    /* min decimal value of a "double"*/
#endif

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

    typedef int NumberOptions;
    NumberOptions option;
    NumberFormat format;
    int precision;
    int denominator;

    // Default denominator of minimum fractional inch. Only used in certain
    // schemas.
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
    QuantityFormat(NumberFormat format, int decimals=-1);
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
    static inline NumberFormat toFormat(char c, bool* ok = nullptr) {
        if (ok)
            *ok = true;
        switch (c) {
        case 'f':
            return Fixed;
        case 'e':
            return Scientific;
        case 'g':
            return Default;
        default:
            if (ok)
                *ok = false;
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
    Quantity(const Quantity&);
    explicit Quantity(double value, const Unit& unit=Unit());
    explicit Quantity(double value, const QString& unit);
    /// Destruction
    ~Quantity () {}

    /** Operators. */
    //@{
    Quantity operator *(const Quantity &p) const;
    Quantity operator *(double p) const;
    Quantity operator +(const Quantity &p) const;
    Quantity& operator +=(const Quantity &p);
    Quantity operator -(const Quantity &p) const;
    Quantity& operator -=(const Quantity &p);
    Quantity operator -() const;
    Quantity operator /(const Quantity &p) const;
    Quantity operator /(double p) const;
    bool operator ==(const Quantity&) const;
    bool operator < (const Quantity&) const;
    bool operator > (const Quantity&) const;
    bool operator <= (const Quantity&) const;
    bool operator >= (const Quantity&) const;
    Quantity& operator =(const Quantity&);
    Quantity pow(const Quantity&)const;
    Quantity pow(double)const;
    //@}

    const QuantityFormat& getFormat() const {
        return _Format;
    }
    void setFormat(const QuantityFormat& f) {
        _Format = f;
    }
    /// transfer to user preferred unit/potence
    QString getUserString(double &factor, QString &unitString) const;
    QString getUserString() const { // to satisfy GCC
        double  dummy1;
        QString dummy2;
        return getUserString(dummy1,dummy2);
    }
    QString getUserString(UnitsSchema* schema, double &factor, QString &unitString) const;

    static Quantity parse(const QString &string);

    /// returns the unit of the quantity
    const Unit & getUnit() const{return _Unit;}
    /// set the unit of the quantity
    void setUnit(const Unit &un){_Unit = un;}
    /// get the Value of the quantity
    double getValue() const{return _Value;}
    /// set the value of the quantity
    void setValue(double val){_Value = val;}
    /** get the Value in a special unit given as quantity.
      * One can use one of the predifeined quantity units in this class
      */
    double getValueAs(const Quantity &)const;


    /// true if it has a number without a unit
    bool isDimensionless()const;
    /// true if it has a number and a valid unit
    bool isQuantity()const;
    /// true if it has a number with or without a unit
    bool isValid()const;
    /// sets the quantity invalid
    void setInvalid();


    /** Predefined Unit types. */
    //@{
    static Quantity NanoMetre;
    static Quantity MicroMetre;
    static Quantity CentiMetre;
    static Quantity DeciMetre;
    static Quantity Metre;
    static Quantity MilliMetre;
    static Quantity KiloMetre;

    static Quantity Liter;
    static Quantity MilliLiter;

    static Quantity Hertz;
    static Quantity KiloHertz;
    static Quantity MegaHertz;
    static Quantity GigaHertz;
    static Quantity TeraHertz;

    static Quantity MicroGram;
    static Quantity MilliGram;
    static Quantity Gram;
    static Quantity KiloGram;
    static Quantity Ton;

    static Quantity Second;
    static Quantity Minute;
    static Quantity Hour;

    static Quantity Ampere;
    static Quantity MilliAmpere;
    static Quantity KiloAmpere;
    static Quantity MegaAmpere;

    static Quantity Kelvin;
    static Quantity MilliKelvin;
    static Quantity MicroKelvin;

    static Quantity MilliMole;
    static Quantity Mole;

    static Quantity Candela;

    static Quantity Inch;
    static Quantity Foot;
    static Quantity Thou;
    static Quantity Yard;

    static Quantity Pound;
    static Quantity Ounce;
    static Quantity Stone;
    static Quantity Hundredweights;
    static Quantity Mile;

    static Quantity MilePerHour;
    static Quantity SquareFoot;
    static Quantity CubicFoot;

    static Quantity PoundForce;

    static Quantity Newton;
    static Quantity MilliNewton;
    static Quantity KiloNewton;
    static Quantity MegaNewton;

    static Quantity NewtonPerMeter;
    static Quantity MilliNewtonPerMeter;
    static Quantity KiloNewtonPerMeter;
    static Quantity MegaNewtonPerMeter;
    
    static Quantity Pascal;
    static Quantity KiloPascal;
    static Quantity MegaPascal;
    static Quantity GigaPascal;

    static Quantity Bar;
    static Quantity MilliBar;

    static Quantity Torr;
    static Quantity mTorr;
    static Quantity yTorr;

    static Quantity PSI;
    static Quantity KSI;
    static Quantity MPSI;

    static Quantity Watt;
    static Quantity MilliWatt;
    static Quantity KiloWatt;
    static Quantity VoltAmpere;

    static Quantity Volt;
    static Quantity MilliVolt;
    static Quantity KiloVolt;

    static Quantity MegaSiemens;
    static Quantity KiloSiemens;
    static Quantity Siemens;
    static Quantity MilliSiemens;
    static Quantity MicroSiemens;

    static Quantity Ohm;
    static Quantity KiloOhm;
    static Quantity MegaOhm;

    static Quantity Coulomb;

    static Quantity Tesla;
    static Quantity Gauss;

    static Quantity Weber;

    static Quantity Oersted;

    static Quantity Farad;
    static Quantity MilliFarad;
    static Quantity MicroFarad;
    static Quantity NanoFarad;
    static Quantity PicoFarad;

    static Quantity Henry;
    static Quantity MilliHenry;
    static Quantity MicroHenry;
    static Quantity NanoHenry;

    static Quantity Joule;
    static Quantity MilliJoule;
    static Quantity KiloJoule;
    static Quantity NewtonMeter;
    static Quantity VoltAmpereSecond;
    static Quantity WattSecond;
    static Quantity KiloWattHour;
    static Quantity ElectronVolt;
    static Quantity KiloElectronVolt;
    static Quantity MegaElectronVolt;
    static Quantity Calorie;
    static Quantity KiloCalorie;

    static Quantity KMH;
    static Quantity MPH;

    static Quantity Degree;
    static Quantity Radian;
    static Quantity Gon;
    static Quantity AngMinute;
    static Quantity AngSecond;
    //@}


protected:
    double         _Value;
    Unit           _Unit;
    QuantityFormat _Format;
};

} // namespace Base

#endif // BASE_Quantity_H
