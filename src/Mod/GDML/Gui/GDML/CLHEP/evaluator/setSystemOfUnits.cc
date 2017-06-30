// -*- C++ -*-
// $Id: setSystemOfUnits.cc,v 1.2 2003/08/13 20:00:10 garren Exp $
// ----------------------------------------------------------------------

///#include "CLHEP/Evaluator/defs.h"
///#include "CLHEP/Evaluator/Evaluator.h"
#include "defs.h"
#include "Evaluator.h"

namespace HepTool {

void Evaluator::setSystemOfUnits(double meter,
				 double kilogram,
				 double second,
				 double ampere,
				 double kelvin,
				 double mole,
				 double candela)
{			    
  const double kilo_  = 1.e+03; // chilioi (Greek) "thousand"
  const double mega_  = 1.e+06; // megas (Greek) "large"
  const double giga_  = 1.e+09; // gigas (Greek) "giant"
  const double tera_  = 1.e+12; // teras (Greek) "monster"
  const double peta_  = 1.e+15; // pente (Greek) "five"

  const double deci_  = 1.e-01; // decimus (Latin) "tenth"
  const double centi_ = 1.e-02; // centum  (Latin) "hundred"
  const double milli_ = 1.e-03; // mille   (Latin) "thousand"
  const double micro_ = 1.e-06; // micro (Latin) or mikros (Greek) "small"
  const double nano_  = 1.e-09; // nanus (Latin) or nanos  (Greek) "dwarf"
  const double pico_  = 1.e-12; // pico (Spanish) "bit"

  // ======================================================================
  //
  // Base (default) SI units
  // for the basic measurable quantities (dimensions):
  //
  // ======================================================================
  
  // Length
  // metrum (Latin) and metron (Greek) "measure"
  const double m = meter;
  setVariable("meter", m);
  setVariable("metre", m);
  setVariable("m",     m);
  
  // Mass
  const double kg = kilogram;
  setVariable("kilogram", kg);
  setVariable("kg",       kg);
  
  // Time
  // minuta secundam (Latin) "second small one"
  const double s = second;
  setVariable("second", s);
  setVariable("s",      s);
  
  // Current
  // ---  honors Andre-Marie Ampere (1775-1836) of France
  const double A = ampere;
  setVariable("ampere", A);
  setVariable("amp",    A);
  setVariable("A",      A);
  
  // Temperature
  // ---  honors William Thomson, 1st Baron Lord Kelvin (1824-1907) of England
  const double K = kelvin;
  setVariable("kelvin", K);
  setVariable("K",      K);
  
  // Amount of substance
  const double mol = mole;
  setVariable("mole", mol);
  setVariable("mol",  mol);
  
  // Luminous intensity
  const double cd  = candela;
  setVariable("candela", cd);
  setVariable("cd",      cd);

  // ======================================================================
  //
  // Supplementary SI units having special symbols:
  //
  // ======================================================================

  // Plane angle 
  const double rad = 1.;
  setVariable("radian", rad);
  setVariable("rad",    rad);
  setVariable("milliradian", milli_ * rad);
  setVariable("mrad",        milli_ * rad);

  const double pi  = 3.14159265358979323846;
  const double deg = rad*pi/180.;
  setVariable("degree", deg);
  setVariable("deg",    deg);

  // Solid angle
  const double sr  = 1.;
  setVariable("steradian", sr);
  setVariable("sr",        sr);

  // ======================================================================
  //
  // Derived SI units having special symbols:
  //
  // ======================================================================

  // Frequency
  // ---  honors Heinrich Rudolf Hertz (1857-1894) of Germany
  const double Hz = 1./s;
  setVariable("hertz", Hz);
  setVariable("Hz",    Hz);

  // Force
  // ---  honors Sir Isaac Newton (1642-1727) of England
  const double N = m * kg / (s*s);
  setVariable("newton", N);
  setVariable("N",      N);

  // Pressure
  // ---  honors Blaise Pascal (1623-1662) of France
  const double Pa = N / (m*m);
  setVariable("pascal", Pa);
  setVariable("Pa",     Pa);

  const double atm = 101325. * Pa;
  setVariable("atmosphere", atm);
  setVariable("atm",        atm);

  const double bar = 100000*Pa;
  setVariable("bar", bar);

  // Energy
  // ---  honors James Prescott Joule (1818-1889) of England
  const double J = N * m;
  setVariable("joule", J);
  setVariable("J",     J);

  // Power
  // ---  honors James Watt (1736-1819) of Scotland
  const double W = J / s;
  setVariable("watt", W);
  setVariable("W",    W);

  // Electric charge
  // ---  honors Charles-Augustin de Coulomb (1736-1806) of France
  const double C = A * s;
  setVariable("coulomb", C);
  setVariable("C",       C);

  // Electric potential  
  // ---  honors Count Alessandro Volta (1745-1827) of Italy
  const double V = J / C;
  setVariable("volt", V);
  setVariable("V",    V);

  // Electric resistance
  // ---  honors Georg Simon Ohm (1787-1854) of Germany
  const double ohm = V / A;
  setVariable("ohm", ohm);

  // Electric conductance
  // ---  honors Ernst Werner von Siemens (1816-1892) or
  //      his brother Sir William (Karl Wilhelm von) Siemens (1823-1883)
  //      of Germany (England)
  const double S = 1./ ohm;
  setVariable("siemens", S);
  setVariable("S",       S);

  // Electric capacitance
  // ---  honors Michael Faraday (1791-1867) of England
  const double F = C / V;
  setVariable("farad", F);
  setVariable("F",     F);

  // Magnetic flux density
  // ---  honors Nikola Tesla (1856-1943) of Croatia (United States)
  const double T = V * s / (m*m);
  setVariable("tesla", T);
  setVariable("T",     T);

  // ---  honors Karl Friedrich Gauss (1777-1855) of Germany
  const double Gs = 1.e-4*T;
  setVariable("gauss", Gs);
  setVariable("Gs",    Gs);

  // Magnetic flux
  // ---  honors Wilhelm Eduard Weber (1804-1891) of Germany
  const double Wb = V * s;
  setVariable("weber", Wb);
  setVariable("Wb",    Wb);

  // Inductance
  // ---  honors Joseph Henry (1797-1878) of the United States
  const double H = Wb / A;
  setVariable("henry", H);
  setVariable("H",     H);

  // Luminous flux
  const double lm = cd * sr;
  setVariable("lumen", lm);
  setVariable("lm",    lm);

  // Illuminace
  const double lx = lm / (m*m);
  setVariable("lux", lx);
  setVariable("lx",  lx);

  // Radioactivity
  // ---  honors Antoine-Henri Becquerel (1852-1908) of France
  const double Bq = 1./s;
  setVariable("becquerel", Bq);
  setVariable("Bq",        Bq);

  // ---  honors Pierre Curie (1859-1906) of France
  //      and Marie Sklodowska Curie (1867-1934) of Poland
  setVariable("curie", 3.7e+10 * Bq);
  setVariable("Ci",    3.7e+10 * Bq);

  // Specific energy
  // ---  honors Louis Harold Gray, F.R.S. (1905-1965) of England
  const double Gy = J / kg;
  setVariable("gray", Gy);
  setVariable("Gy",   Gy);

  // Dose equivalent
  const double Sv = J / kg;
  setVariable("sievert", Sv);
  setVariable("Sv",      Sv);

  // ======================================================================
  //
  // Selected units:
  //
  // ======================================================================

  // Length

  const double mm = milli_ * m;
  setVariable("millimeter", mm);
  setVariable("mm",         mm);

  const double cm = centi_ * m;
  setVariable("centimeter", cm);
  setVariable("cm",         cm);

  setVariable("decimeter",  deci_ * m);

  const double km = kilo_ * m; 
  setVariable("kilometer",  km);
  setVariable("km",         km);

  setVariable("micrometer", micro_ * m);
  setVariable("micron",     micro_ * m);
  setVariable("nanometer",  nano_  * m);

  // ---  honors Anders Jonas Angstrom (1814-1874) of Sweden
  setVariable("angstrom",   1.e-10 * m);

  // ---  honors Enrico Fermi (1901-1954) of Italy
  setVariable("fermi",      1.e-15 * m);

  // Length^2

  setVariable("m2",  m*m);
  setVariable("mm2", mm*mm);
  setVariable("cm2", cm*cm);
  setVariable("km2", km*km);

  const double barn = 1.e-28 * m*m; 
  setVariable("barn",      barn);
  setVariable("millibarn", milli_ * barn);
  setVariable("mbarn",     milli_ * barn);
  setVariable("microbarn", micro_ * barn);
  setVariable("nanobarn",  nano_  * barn);
  setVariable("picobarn",  pico_  * barn);

  // LengthL^3

  setVariable("m3",  m*m*m);
  setVariable("mm3", mm*mm*mm);
  setVariable("cm3", cm*cm*cm);
  setVariable("cc",  cm*cm*cm);
  setVariable("km3", km*km*km);

  const double L = 1.e-3*m*m*m;
  setVariable("liter", L);  
  setVariable("litre", L);  
  setVariable("L",     L);  
  setVariable("centiliter",  centi_ * L);
  setVariable("cL",          centi_ * L);
  setVariable("milliliter",  milli_ * L);
  setVariable("mL",          milli_ * L);

  // Length^-1

  const double dpt = 1./m;
  setVariable("diopter", dpt);
  setVariable("dioptre", dpt);
  setVariable("dpt",     dpt);

  // Mass

  const double g = 0.001*kg;
  setVariable("gram", g);
  setVariable("g",    g);
  setVariable("milligram",   milli_ * g);
  setVariable("mg",          milli_ * g);
  
  // Time

  setVariable("millisecond", milli_ * s);
  setVariable("ms",          milli_ * s);
  setVariable("microsecond", micro_ * s);
  setVariable("nanosecond",  nano_  * s);
  setVariable("ns",          nano_  * s);
  setVariable("picosecond",  pico_  * s);

  // Current

  setVariable("milliampere", milli_ * A);
  setVariable("mA",          milli_ * A);
  setVariable("microampere", micro_ * A);
  setVariable("nanoampere",  nano_  * A);

  // Frequency

  setVariable("kilohertz",   kilo_ * Hz);
  setVariable("kHz",         kilo_ * Hz);
  setVariable("megahertz",   mega_ * Hz);
  setVariable("MHz",         mega_ * Hz);

  // Force
  setVariable("kilonewton",  kilo_ * N);
  setVariable("kN",          kilo_ * N);

  // Pressure
  setVariable("kilobar",     kilo_ * bar);
  setVariable("kbar",        kilo_ * bar);
  setVariable("millibar",    milli_ * bar);
  setVariable("mbar",        milli_ * bar);

  // Energy
  setVariable("kilojoule",   kilo_ * J);
  setVariable("kJ",          kilo_ * J);
  setVariable("megajoule",   mega_ * J);
  setVariable("MJ",          mega_ * J);
  setVariable("gigajoule",   giga_ * J);
  setVariable("GJ",          giga_ * J);

  const double e_SI  = 1.60217733e-19;  // positron charge in coulomb
  const double ePlus = e_SI * C;        // positron charge
  const double eV    = ePlus * V;
  setVariable("electronvolt", eV);
  setVariable("eV",           eV);
  setVariable("kiloelectronvolt", kilo_ * eV);
  setVariable("keV",              kilo_ * eV);
  setVariable("megaelectronvolt", mega_ * eV);
  setVariable("MeV",              mega_ * eV);
  setVariable("gigaelectronvolt", giga_ * eV);
  setVariable("GeV",              giga_ * eV);
  setVariable("teraelectronvolt", tera_ * eV);
  setVariable("TeV",              tera_ * eV);
  setVariable("petaelectronvolt", peta_ * eV);
  setVariable("PeV",              peta_ * eV);

  // Power
  setVariable("kilowatt",    kilo_ * W);
  setVariable("kW",          kilo_ * W);
  setVariable("megawatt",    mega_ * W);
  setVariable("MW",          mega_ * W);
  setVariable("gigawatt",    giga_ * W);
  setVariable("GW",          giga_ * W);

  // Electric potential  
  setVariable("kilovolt",    kilo_ * V);
  setVariable("kV",          kilo_ * V);
  setVariable("megavolt",    mega_ * V);
  setVariable("MV",          mega_ * V);

  // Electric capacitance
  setVariable("millifarad",  milli_ * F);
  setVariable("mF",          milli_ * F);
  setVariable("microfarad",  micro_ * F);
  setVariable("uF",          micro_ * F);
  setVariable("nanofarad",   nano_  * F);
  setVariable("nF",          nano_  * F);
  setVariable("picofarad",   pico_  * F);
  setVariable("pF",          pico_  * F);

  // Magnetic flux density
  setVariable("kilogauss",   kilo_ * Gs);
  setVariable("kGs",         kilo_ * Gs);
}

} // namespace HepTool
