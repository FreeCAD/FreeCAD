/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef REEN_APPROXSURFACE_H
#define REEN_APPROXSURFACE_H

#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <Geom_BSplineSurface.hxx>
#include <math_Matrix.hxx>

#include <Base/Vector3D.h>

namespace Base {
class SequencerLauncher;
}

// TODO: Replace OCC stuff with ublas & co

namespace Reen {

class ReenExport SplineBasisfunction
{
public:
    enum ValueT {
        Zero = 0,
        Full,
        Other
    };
    /**
     * Konstruktor
     * @param iSize Length of Knots vector
     */
    SplineBasisfunction(int iSize);

    /**
     * Konstruktor
     * @param vKnots Knotenvektor
     * @param iOrder Ordnung (Grad+1) des Basis-Polynoms
     */
    SplineBasisfunction(TColStd_Array1OfReal& vKnots, int iOrder=1);

    /**
     * Konstruktor
     * @param vKnots Knotenvektor der Form (Wert)
     * @param vMults Knotenvektor der Form (Vielfachheit)
     * @param iSize Laenge des Knotenvektors
     * Die Arrays @a vKnots und @a vMults muessen die gleiche besitzen und die Summe der Werte in @a vMults
     * muss identisch mit @a iSize sein.
     * @param iOrder Ordnung (Grad+1) des Basis-Polynoms
     */
    SplineBasisfunction(TColStd_Array1OfReal& vKnots, TColStd_Array1OfInteger& vMults, int iSize, int iOrder=1);

    virtual ~SplineBasisfunction();

    /**
     * Gibt an, ob der Funktionswert Nik(t) an der Stelle fParam
     * 0, 1 oder ein Wert dazwischen ergibt.
     * Dies dient dazu, um die Berechnung zu u.U. zu beschleunigen.
     *
     * @param iIndex Index
     * @param fParam Parameterwert
     * @return ValueT
     */
    virtual ValueT LocalSupport(int iIndex, double fParam)=0;
    /**
     * Berechnet den Funktionswert Nik(t) an der Stelle fParam
     * (aus: Piegl/Tiller 96 The NURBS-Book)
     *
     * @param iIndex Index
     * @param fParam Parameterwert
     * @return Funktionswert Nik(t)
     */
    virtual double BasisFunction(int iIndex, double fParam)=0;
    /**
     * Berechnet die Funktionswerte der ersten iMaxDer Ableitungen an der
     * Stelle fParam (aus: Piegl/Tiller 96 The NURBS-Book)
     *
     * @param iIndex  Index
     * @param iMaxDer max. Ableitung
     * @param fParam  Parameterwert.
     * @return Derivat Liste der Funktionswerte
     *
     *  Die Liste muss fuer iMaxDer+1 Elemente ausreichen.
     */
    virtual void DerivativesOfBasisFunction(int iIndex, int iMaxDer, double fParam,
                                            TColStd_Array1OfReal& Derivat)=0;

    /**
     * Berechnet die k-te Ableitung an der Stelle fParam
     */
    virtual double DerivativeOfBasisFunction(int iIndex, int k, double fParam)=0;

    /**
     * Setzt den Knotenvektor und die Ordnung fest. Die Groesse des Knotenvektors muss exakt so gross sein,
     * wie im Konstruktor festgelegt.
     */
    virtual void SetKnots(TColStd_Array1OfReal& vKnots, int iOrder=1);

    /**
     * Setzt den Knotenvektor und die Ordnung fest. uebergeben wird der Knotenvektor der Form
     * (Wert, Vielfachheit). Intern wird dieser in einen Knotenvektor der Form (Wert,1)
     * umgerechnet. Die Groesse dieses neuen Vektors muss exakt so gross sein, wie im Konstruktor
     * festgelegt.
     */
    virtual void SetKnots(TColStd_Array1OfReal& vKnots, TColStd_Array1OfInteger& vMults, int iOrder=1);

protected: //Member
    // Knotenvektor
    TColStd_Array1OfReal _vKnotVector;

    // Ordnung (=Grad+1)
    int _iOrder;
};

class ReenExport BSplineBasis : public SplineBasisfunction
{
public:

    /**
     * Konstruktor
     * @param iSize Laenge des Knotenvektors
     */
    BSplineBasis(int iSize);

    /**
     * Konstruktor
     * @param vKnots Knotenvektor
     * @param iOrder Ordnung (Grad+1) des Basis-Polynoms
     */
    BSplineBasis(TColStd_Array1OfReal& vKnots, int iOrder=1);

    /**
     * Konstruktor
     * @param vKnots Knotenvektor der Form (Wert)
     * @param vMults Knotenvektor der Form (Vielfachheit)
     * @param iSize Laenge des Knotenvektors
     * Die Arrays @a vKnots und @a vMults muessen die gleiche besitzen und die Summe der Werte in @a vMults
     * muss identisch mit @a iSize sein.
     * @param iOrder Ordnung (Grad+1) des Basis-Polynoms
     */
    BSplineBasis(TColStd_Array1OfReal& vKnots, TColStd_Array1OfInteger& vMults, int iSize, int iOrder=1);

    /**
     * Bestimmt den Knotenindex zum Parameterwert (aus: Piegl/Tiller 96 The NURBS-Book)
     * @param fParam Parameterwert
     * @return Knotenindex
     */
    virtual int FindSpan(double fParam);

    /**
     * Berechnet die Funktionswerte der an der Stelle fParam
     * nicht verschwindenden Basisfunktionen. Es muss darauf geachtet werden, dass
     * die Liste fuer d(=Grad des B-Splines) Elemente (0,...,d-1) ausreicht.
     * (aus: Piegl/Tiller 96 The NURBS-Book)
     * @param fParam Parameter
     * @param vFuncVals Liste der Funktionswerte
     * Index, Parameterwert
     */
    virtual void AllBasisFunctions(double fParam, TColStd_Array1OfReal& vFuncVals);

    /**
     * Gibt an, ob der Funktionswert Nik(t) an der Stelle fParam
     * 0, 1 oder ein Wert dazwischen ergibt.
     * Dies dient dazu, um die Berechnung zu u.U. zu beschleunigen.
     *
     * @param iIndex Index
     * @param fParam Parameterwert
     * @return ValueT
     */
    virtual ValueT LocalSupport(int iIndex, double fParam);

    /**
     * Berechnet den Funktionswert Nik(t) an der Stelle fParam
     * (aus: Piegl/Tiller 96 The NURBS-Book)
     * @param iIndex Index
     * @param fParam Parameterwert
     * @return Funktionswert Nik(t)
     */
    virtual double BasisFunction(int iIndex, double fParam);

    /**
     * Berechnet die Funktionswerte der ersten iMaxDer Ableitungen an der Stelle fParam
     * (aus: Piegl/Tiller 96 The NURBS-Book)
     * @param iIndex Index
     * @param iMaxDer max. Ableitung
     * @param fParam Parameterwert.
     * @param Derivat
     * Die Liste muss fuer iMaxDer+1 Elemente ausreichen.
     * @return Liste der Funktionswerte
     */
    virtual void DerivativesOfBasisFunction(int iIndex, int iMaxDer, double fParam,
                                            TColStd_Array1OfReal& Derivat);

    /**
     * Berechnet die k-te Ableitung an der Stelle fParam
     */
    virtual double DerivativeOfBasisFunction(int iIndex, int k, double fParam);

    /**
     * Berechnet das Integral des Produkts zweier B-Splines bzw. deren Ableitungen.
     * Der Integrationsbereich erstreckt sich ueber den ganzen Definitionsbereich.
     * Berechnet wird das Integral mittels der Gauss'schen Quadraturformeln.
     */
    virtual double GetIntegralOfProductOfBSplines(int i, int j, int r, int s);

    /**
     * Destruktor
     */
    virtual~ BSplineBasis();

protected:

    /**
     * Berechnet die Nullstellen der Legendre-Polynome und die
     * zugehoerigen Gewichte
     */
    virtual void GenerateRootsAndWeights(TColStd_Array1OfReal& vAbscissas, TColStd_Array1OfReal& vWeights);

    /**
     * Berechnet die Integrationsgrenzen (Indexe der Knoten)
     */
    virtual void FindIntegrationArea(int iIdx1, int iIdx2, int& iBegin, int& iEnd);

    /**
     * Berechnet in Abhaengigkeit vom Grad die Anzahl der zu verwendenden Nullstellen/Gewichte
     * der Legendre-Polynome
     */
    int CalcSize(int r, int s);
};

class ReenExport ParameterCorrection
{

public:
    // Konstruktor
    ParameterCorrection(unsigned usUOrder=4,               //Ordnung in u-Richtung (Ordnung=Grad+1)
                        unsigned usVOrder=4,               //Ordnung in v-Richtung
                        unsigned usUCtrlpoints=6,          //Anz. der Kontrollpunkte in u-Richtung
                        unsigned usVCtrlpoints=6);         //Anz. der Kontrollpunkte in v-Richtung

    virtual ~ParameterCorrection()
    {
        delete _pvcPoints;
        delete _pvcUVParam;
    }

protected:
    /**
     * Berechnet die Eigenvektoren der Kovarianzmatrix
     */
    virtual void CalcEigenvectors();

    /**
     * Projiziert die Kontrollpunkte auf die Fit-Ebene
     */
    void ProjectControlPointsOnPlane();

    /**
     * Berechnet eine initiale Flaeche zu Beginn des Algorithmus. Dazu wird die Ausgleichsebene zu der
     * Punktwolke berechnet.
     * Die Punkte werden bzgl. der Basis bestehend aus den Eigenvektoren der Kovarianzmatrix berechnet und
     * auf die Ausgleichsebene projiziert. Von diesen Punkten wird die Boundingbox berechnet, dann werden
     * die u/v-Parameter fuer die Punkte berechnet.
     */
    virtual bool DoInitialParameterCorrection(double fSizeFactor=0.0f);

    /**
     * Berechnet die u.v-Werte der Punkte
     */
    virtual bool GetUVParameters(double fSizeFactor);

    /**
     * Fuehrt eine Parameterkorrektur durch.
     */
    virtual void DoParameterCorrection(int iIter)=0;

    /**
     * Loest Gleichungssystem
     */
    virtual bool SolveWithoutSmoothing()=0;

    /**
     * Loest ein regulaeres Gleichungssystem
     */
    virtual bool SolveWithSmoothing(double fWeight)=0;

public:
    /**
     * Berechnet eine B-Spline-Flaeche.aus den geg. Punkten
     */
    virtual Handle(Geom_BSplineSurface) CreateSurface(const TColgp_Array1OfPnt& points,
                                                     int iIter,
                                                     bool bParaCor,
                                                     double fSizeFactor=0.0f);
    /**
     * Setzen der u/v-Richtungen
     * Dritter Parameter gibt an, ob die Richtungen tatsaechlich verwendet werden sollen.
     */
    virtual void SetUV(const Base::Vector3d& clU, const Base::Vector3d& clV, bool bUseDir=true);

    /**
     * Gibt die u/v/w-Richtungen zurueck
     */
    virtual void GetUVW(Base::Vector3d& clU, Base::Vector3d& clV, Base::Vector3d& clW) const;

    /**
     *
     */
    virtual Base::Vector3d GetGravityPoint() const;

    /**
     * Verwende Glaettungsterme
     */
    virtual void EnableSmoothing(bool bSmooth=true, double fSmoothInfl=1.0f);

protected:
    bool                    _bGetUVDir;        //! Stellt fest, ob u/v-Richtung vorgegeben wird
    bool                    _bSmoothing;       //! Glaettung verwenden
    double                  _fSmoothInfluence; //! Einfluss der Glaettung
    unsigned                _usUOrder;         //! Ordnung in u-Richtung
    unsigned                _usVOrder;         //! Ordnung in v-Richtung
    unsigned                _usUCtrlpoints;    //! Anzahl der Kontrollpunkte in u-Richtung
    unsigned                _usVCtrlpoints;    //! Anzahl der Kontrollpunkte in v-Richtung
    Base::Vector3d          _clU;              //! u-Richtung
    Base::Vector3d          _clV;              //! v-Richtung
    Base::Vector3d          _clW;              //! w-Richtung (senkrecht zu u-und v-Richtung)
    TColgp_Array1OfPnt*     _pvcPoints;        //! Punktliste der Rohdaten
    TColgp_Array1OfPnt2d*   _pvcUVParam;       //! Parameterwerte zu den Punkten aus der Liste
    TColgp_Array2OfPnt      _vCtrlPntsOfSurf;  //! Array von Kontrollpunkten
    TColStd_Array1OfReal    _vUKnots;          //! Knotenvektor der B-Spline-Flaeche in u-Richtung
    TColStd_Array1OfReal    _vVKnots;          //! Knotenvektor der B-Spline-Flaeche in v-Richtung
    TColStd_Array1OfInteger _vUMults;          //! Vielfachheit der Knoten im Knotenvektor
    TColStd_Array1OfInteger _vVMults;          //! Vielfachheit der Knoten im Knotenvektor
};

///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Diese Klasse berechnet auf einer beliebigen Punktwolke (auch scattered data) eine
 * B-Spline-Flaeche. Die Flaeche wird iterativ mit Hilfe einer Parameterkorrektur erzeugt.
 * Siehe dazu Hoschek/Lasser 2. Auflage (1992).
 * Erweitert wird die Approximation um Glaettungsterme, so dass glatte Flaechen erzeugt werden
 * koennen.
 */

class ReenExport BSplineParameterCorrection : public ParameterCorrection
{
public:
    // Konstruktor
    BSplineParameterCorrection(unsigned usUOrder=4,               //Ordnung in u-Richtung (Ordnung=Grad+1)
                               unsigned usVOrder=4,               //Ordnung in v-Richtung
                               unsigned usUCtrlpoints=6,          //Anz. der Kontrollpunkte in u-Richtung
                               unsigned usVCtrlpoints=6);         //Anz. der Kontrollpunkte in v-Richtung

    virtual ~BSplineParameterCorrection(){};

protected:
    /**
     * Initialisierung
     */
    virtual void Init();

    /**
     * Fuehrt eine Parameterkorrektur durch.
     */
    virtual void DoParameterCorrection(int iIter);

    /**
     * Loest ein ueberbestimmtes LGS mit Hilfe der Householder-Transformation
     */
    virtual bool SolveWithoutSmoothing();

    /**
     * Loest ein regulaeres Gleichungssystem durch LU-Zerlegung. Es fliessen je nach Gewichtung
     * Glaettungsterme mit ein
     */
    virtual bool SolveWithSmoothing(double fWeight);

public:
    /**
     * Setzen des Knotenvektors
     */
    void SetUKnots(const std::vector<double>& afKnots);

    /**
     * Setzen des Knotenvektors
     */
    void SetVKnots(const std::vector<double>& afKnots);

    /**
     * Gibt die erste Matrix der Glaettungsterme zurueck, falls berechnet
     */
    virtual const math_Matrix& GetFirstSmoothMatrix() const;

    /**
     * Gibt die zweite Matrix der Glaettungsterme zurueck, falls berechnet
     */
    virtual const math_Matrix& GetSecondSmoothMatrix() const;

    /**
     * Gibt die dritte Matrix der Glaettungsterme zurueck, falls berechnet
     */
    virtual const math_Matrix& GetThirdSmoothMatrix() const;

    /**
     * Setzt die erste Matrix der Glaettungsterme
     */
    virtual void SetFirstSmoothMatrix(const math_Matrix& rclMat);

    /**
     * Setzt die zweite Matrix der Glaettungsterme
     */
    virtual void SetSecondSmoothMatrix(const math_Matrix& rclMat);

    /**
     * Setzt die dritte Matrix der Glaettungsterme
     */
    virtual void SetThirdSmoothMatrix(const math_Matrix& rclMat);

    /**
     * Verwende Glaettungsterme
     */
    virtual void EnableSmoothing(bool bSmooth=true, double fSmoothInfl=1.0f);

    /**
     * Verwende Glaettungsterme
     */
    virtual void EnableSmoothing(bool bSmooth, double fSmoothInfl,
                                 double fFirst, double fSec,  double fThird);

protected:
    /**
     * Berechnet die Matrix zu den Glaettungstermen
     * (siehe Dissertation U.Dietz)
     */
    virtual void CalcSmoothingTerms(bool bRecalc, double fFirst, double fSecond, double fThird);

    /**
     * Berechnet die Matrix zum ersten Glaettungsterm
     * (siehe Diss. U.Dietz)
     */
    virtual void CalcFirstSmoothMatrix(Base::SequencerLauncher&);

    /**
     * Berechnet die Matrix zum zweiten Glaettunsterm
     * (siehe Diss. U.Dietz)
     */
    virtual void CalcSecondSmoothMatrix(Base::SequencerLauncher&);

    /**
     * Berechnet die Matrix zum dritten Glaettungsterm
     */
    virtual void CalcThirdSmoothMatrix(Base::SequencerLauncher&);

protected:
    BSplineBasis           _clUSpline;        //! B-Spline-Basisfunktion in u-Richtung
    BSplineBasis           _clVSpline;        //! B-Spline-Basisfunktion in v-Richtung
    math_Matrix            _clSmoothMatrix;   //! Matrix der Glaettungsfunktionale
    math_Matrix            _clFirstMatrix;    //! Matrix der 1. Glaettungsfunktionale
    math_Matrix            _clSecondMatrix;   //! Matrix der 2. Glaettungsfunktionale
    math_Matrix            _clThirdMatrix;    //! Matrix der 3. Glaettungsfunktionale
};

} // namespace Reen

#endif // REEN_APPROXSURFACE_H
