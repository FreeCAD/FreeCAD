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
#include <Handle_Geom_BSplineSurface.hxx>
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
  /**
   * Konstruktor
   * @param iSize Length of Knots vector
   */
  SplineBasisfunction(int iSize);

  /**
   * Konstruktor
   * @param vKnots Knotenvektor
   * @param iOrder
   * \todo undocumented parameter iOrder
   */
  SplineBasisfunction(TColStd_Array1OfReal& vKnots, int iOrder=1);

  /**
   * Konstruktor
   * @param vKnots Knotenvektor der Form (Wert,Vielfachheit)
   * @param vMults
   * @param iSize
   * @param iOrder
   * \todo undocumented parameters vMults, iSize, iOrder
   */
  SplineBasisfunction(TColStd_Array1OfReal& vKnots, TColStd_Array1OfInteger& vMults, int iSize, int iOrder=1);

  virtual ~SplineBasisfunction();

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
   * @param Derivat
   * @return Liste der Funktionswerte
   *
   *  Die Liste muß für iMaxDer+1 Elemente ausreichen.
   * \todo undocumented parameter Derivat
   */
  virtual void DerivativesOfBasisFunction(int iIndex, int iMaxDer, double fParam,
                                  TColStd_Array1OfReal& Derivat)=0;

  /**
   * Berechnet die k-te Ableitung an der Stelle fParam
   */
  virtual double DerivativeOfBasisFunction(int iIndex, int k, double fParam)=0;

  /**
   * Setzt den Knotenvektor und die Ordnung fest. Die Größe des Knotenvektors muß exakt so groß sein,
   * wie im Konstruktor festgelegt.
   */
  virtual void SetKnots(TColStd_Array1OfReal& vKnots, int iOrder=1);

  /**
   * Setzt den Knotenvektor und die Ordnung fest. Übergeben wird der Knotenvektor der Form
   * (Wert, Vielfachheit). Intern wird dieser in einen Knotenvektor der Form (Wert,1) 
   * umgerechnet. Die Größe dieses neuen Vektors muß exakt so groß sein, wie im Konstruktor 
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
   * @param iSize Länge des Knotenvektors
   */
  BSplineBasis(int iSize);

  /**
   * Konstruktor
   * @param vKnots Knotenvektor
   * @param iOrder
   * \todo undocumented parameter iOrder
   */
  BSplineBasis(TColStd_Array1OfReal& vKnots, int iOrder=1);

  /**
   * Konstruktor
   * @param vKnots Knotenvektor der Form (Wert,Vielfachheit)
   * @param vMults
   * @param iSize
   * @param iOrder
   * \todo undocumented parameters
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
   * nicht verschwindenden Basisfunktionen. Es muß darauf geachtet werden, daß
   * die Liste für d(=Grad des B-Splines) Elemente (0,...,d-1) ausreicht.
   * (aus: Piegl/Tiller 96 The NURBS-Book)
   * @param fParam
   * @param vFuncVals
   * Index, Parameterwert
   * @return Liste der Funktionswerte
   * \todo undocumented parameter
   */
  virtual void AllBasisFunctions(double fParam, TColStd_Array1OfReal& vFuncVals);

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
   * Die Liste muß für iMaxDer+1 Elemente ausreichen.
   * @return Liste der Funktionswerte
   * \todo undocumented parameter Derivat
   */
  virtual void DerivativesOfBasisFunction(int iIndex, int iMaxDer, double fParam,
                                  TColStd_Array1OfReal& Derivat);

  /**
   * Berechnet die k-te Ableitung an der Stelle fParam
   */
  virtual double DerivativeOfBasisFunction(int iIndex, int k, double fParam);

  /**
   * Berechnet das Integral des Produkts zweier B-Splines bzw. deren Ableitungen. 
   * Der Integrationsbereich erstreckt sich über den ganzen Definitionsbereich.
   * Berechnet wird das Integral mittels der Gaußschen Quadraturformeln.
   */
  virtual double GetIntegralOfProductOfBSplines(int i, int j, int r, int s);

  /**
   * Destruktor
   */
  virtual~ BSplineBasis();

protected:

  /**
   * Berechnet die Nullstellen der Legendre-Polynome und die
   * zugehörigen Gewichte
   */
  virtual void GenerateRootsAndWeights(TColStd_Array1OfReal& vAbscissas, TColStd_Array1OfReal& vWeights);

  /**
   * Berechnet die Integrationsgrenzen (Indexe der Knoten)
   */
  virtual void FindIntegrationArea(int iIdx1, int iIdx2, int& iBegin, int& iEnd);

  /**
   * Berechnet in Abhängigkeit vom Grad die Anzahl der zu verwendenden Nullstellen/Gewichte
   * der Legendre-Polynome
   */
  int CalcSize(int r, int s);
};

class ReenExport ParameterCorrection
{

public:
  // Konstruktor
  ParameterCorrection(
                unsigned short usUOrder=4,               //Ordnung in u-Richtung (Ordnung=Grad+1)
                unsigned short usVOrder=4,               //Ordnung in v-Richtung
                unsigned short usUCtrlpoints=6,          //Anz. der Kontrollpunkte in u-Richtung
                unsigned short usVCtrlpoints=6);         //Anz. der Kontrollpunkte in v-Richtung

  virtual ~ParameterCorrection()
  {
    delete _pvcPoints;
    delete _pvcUVParam;
  };

protected:
  /**
   * Berechnet die Eigenvektoren der Kovarianzmatrix
   */
  virtual void CalcEigenvectors();

  /**
   * Berechnet eine initiale Fläche zu Beginn des Algorithmus. Dazu wird die Ausgleichsebene zu der 
   * Punktwolke berechnet.
   * Die Punkte werden bzgl. der Basis bestehend aus den Eigenvektoren der Kovarianzmatrix berechnet und 
   * auf die Ausgleichsebene projiziert. Von diesen Punkten wird die Boundingbox berechnet, dann werden 
   * die u/v-Parameter für die Punkte berechnet.
   */
  virtual bool DoInitialParameterCorrection(double fSizeFactor=0.0f);

  /** 
   * Berechnet die u.v-Werte der Punkte
   */
  virtual bool GetUVParameters(double fSizeFactor);

  /** 
   * Führt eine Parameterkorrektur durch.
   */
  virtual void DoParameterCorrection(unsigned short usIter)=0;

  /**
   * Löst Gleichungssystem
   */
  virtual bool SolveWithoutSmoothing()=0;

  /**
   * Löst ein reguläres Gleichungssystem
   */
  virtual bool SolveWithSmoothing(double fWeight)=0;

public:
  /**
   * Berechnet eine B-Spline-Fläche.aus den geg. Punkten
   */
  virtual Handle_Geom_BSplineSurface CreateSurface(
                                           const TColgp_Array1OfPnt& points,
                                           unsigned short usIter,
                                           bool bParaCor,
                                           double fSizeFactor=0.0f);
  /**
   * Setzen der u/v-Richtungen
   * Dritter Parameter gibt an, ob die Richtungen tatsächlich verwendet werden sollen.
   */
  virtual void SetUVW(const Base::Vector3d& clU, const Base::Vector3d& clV, const Base::Vector3d& clW, bool bUseDir=true);

  /**
   * Gibt die u/v/w-Richtungen zurück
   */
  virtual void GetUVW(Base::Vector3d& clU, Base::Vector3d& clV, Base::Vector3d& clW) const;

  /**
   * 
   */
  virtual Base::Vector3d GetGravityPoint() const;

  /**
   * Verwende Glättungsterme
   */
  virtual void EnableSmoothing(bool bSmooth=true, double fSmoothInfl=1.0f);

protected:
  bool                    _bGetUVDir;        //! Stellt fest, ob u/v-Richtung vorgegeben wird
  bool                    _bSmoothing;       //! Glättung verwenden
  double                  _fSmoothInfluence; //! Einfluß der Glättung
  unsigned short                 _usUOrder;         //! Ordnung in u-Richtung
  unsigned short                 _usVOrder;         //! Ordnung in v-Richtung
  unsigned short                 _usUCtrlpoints;    //! Anzahl der Kontrollpunkte in u-Richtung
  unsigned short                 _usVCtrlpoints;    //! Anzahl der Kontrollpunkte in v-Richtung
  Base::Vector3d               _clU;              //! u-Richtung
  Base::Vector3d               _clV;              //! v-Richtung
  Base::Vector3d               _clW;              //! w-Richtung (senkrecht zu u-und w-Richtung)
  TColgp_Array1OfPnt*     _pvcPoints;        //! Punktliste der Rohdaten
  TColgp_Array1OfPnt2d*   _pvcUVParam;       //! Parameterwerte zu den Punkten aus der Liste
  TColgp_Array2OfPnt      _vCtrlPntsOfSurf;  //! Array von Kontrollpunkten
  TColStd_Array1OfReal    _vUKnots;          //! Knotenvektor der B-Spline-Fläche in u-Richtung
  TColStd_Array1OfReal    _vVKnots;          //! Knotenvektor der B-Spline-Fläche in v-Richtung
  TColStd_Array1OfInteger _vUMults;          //! Vielfachheit der Knoten im Knotenvektor
  TColStd_Array1OfInteger _vVMults;          //! Vielfachheit der Knoten im Knotenvektor
};

///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Diese Klasse berechnet auf einer beliebigen Punktwolke (auch scattered data) eine
 * B-Spline-Fläche. Die Fläche wird iterativ mit Hilfe einer Parameterkorrektur erzeugt.
 * Siehe dazu Hoschek/Lasser 2. Auflage (1992).
 * Erweitert wird die Approximation um Glättungsterme, so daß glatte Flächen erzeugt werden
 * können.
 */

class ReenExport BSplineParameterCorrection : public ParameterCorrection
{

public:
  // Konstruktor
  BSplineParameterCorrection(
                unsigned short usUOrder=4,               //Ordnung in u-Richtung (Ordnung=Grad+1)
                unsigned short usVOrder=4,               //Ordnung in v-Richtung
                unsigned short usUCtrlpoints=6,          //Anz. der Kontrollpunkte in u-Richtung
                unsigned short usVCtrlpoints=6);         //Anz. der Kontrollpunkte in v-Richtung

  virtual ~BSplineParameterCorrection(){};

protected:
  /**
   * Initialisierung
   */
  virtual void Init();

  /** 
   * Führt eine Parameterkorrektur durch.
   */
  virtual void DoParameterCorrection(unsigned short usIter);

  /**
   * Löst ein überbestimmtes LGS mit Hilfe der Householder-Transformation
   */
  virtual bool SolveWithoutSmoothing();

  /**
   * Löst ein reguläres Gleichungssystem durch LU-Zerlegung. Es fließen je nach Gewichtung
   * Glättungsterme mit ein
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
   * Gibt die erste Matrix der Glättungsterme zurück, falls berechnet
   */
  virtual const math_Matrix& GetFirstSmoothMatrix() const;

  /**
   * Gibt die zweite Matrix der Glättungsterme zurück, falls berechnet
   */
  virtual const math_Matrix& GetSecondSmoothMatrix() const;

  /**
   * Gibt die dritte Matrix der Glättungsterme zurück, falls berechnet
   */
  virtual const math_Matrix& GetThirdSmoothMatrix() const;

  /**
   * Setzt die erste Matrix der Glättungsterme 
   */
  virtual void SetFirstSmoothMatrix(const math_Matrix& rclMat);

  /**
   * Setzt die zweite Matrix der Glättungsterme
   */
  virtual void SetSecondSmoothMatrix(const math_Matrix& rclMat);

  /**
   * Setzt die dritte Matrix der Glättungsterme
   */
  virtual void SetThirdSmoothMatrix(const math_Matrix& rclMat);

  /**
   * Verwende Glättungsterme
   */
  virtual void EnableSmoothing(bool bSmooth=true, double fSmoothInfl=1.0f);

  /**
   * Verwende Glättungsterme
   */
  virtual void EnableSmoothing(bool bSmooth, double fSmoothInfl,
                               double fFirst, double fSec,  double fThird);

protected:
  /**
   * Berechnet die Matrix zu den Glättungstermen
   * (siehe Dissertation U.Dietz)
   */
  virtual void CalcSmoothingTerms(bool bRecalc, double fFirst, double fSecond, double fThird);

  /**
   * Berechnet die Matrix zum ersten Glättungsterm
   * (siehe Diss. U.Dietz)
   */
  virtual void CalcFirstSmoothMatrix(Base::SequencerLauncher&);

  /**
   * Berechnet die Matrix zum zweiten Glättunsterm
   * (siehe Diss. U.Dietz)
   */
  virtual void CalcSecondSmoothMatrix(Base::SequencerLauncher&);

  /**
   * Berechnet die Matrix zum dritten Glättungsterm
   */
  virtual void CalcThirdSmoothMatrix(Base::SequencerLauncher&);

protected:
  BSplineBasis           _clUSpline;        //! B-Spline-Basisfunktion in u-Richtung
  BSplineBasis           _clVSpline;        //! B-Spline-Basisfunktion in v-Richtung
  math_Matrix             _clSmoothMatrix;   //! Matrix der Glättungsfunktionale
  math_Matrix             _clFirstMatrix;    //! Matrix der 1. Glättungsfunktionale
  math_Matrix             _clSecondMatrix;   //! Matrix der 2. Glättungsfunktionale
  math_Matrix             _clThirdMatrix;    //! Matrix der 3. Glättungsfunktionale
};

} // namespace Reen

#endif // REEN_APPROXSURFACE_H
