//  SMESH MEFISTO2 : algorithm for meshing
//
// Copyright (C) 2006-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  File   : aptrte.h
//  Author : Alain PERRONNET
//  Module : SMESH
//  Date   : 13 novembre 2006

#ifndef aptrte__h
#define aptrte__h

#include <climits>   // limites min max int long real ...
#ifndef WIN32
#include <unistd.h>   // gethostname, ...
#endif
#include <stdio.h>
#ifndef WIN32
#include <iostream> // pour cout cin ...
#include <iomanip>  // pour le format des io setw, stx, setfill, ...
#endif
#include <string.h>   // pour les fonctions sur les chaines de caracteres
#include <ctype.h>
#include <stdlib.h>
#include <math.h>     // pour les fonctions mathematiques
#include <time.h>

#include <sys/types.h>
#ifndef WIN32
#include <sys/time.h>
#endif

#ifdef WIN32
 #if defined MEFISTO2D_EXPORTS
  #define MEFISTO2D_EXPORT __declspec( dllexport )
 #else
  #define MEFISTO2D_EXPORT __declspec( dllimport )
 #endif
 #define F2C_BUILD
#else
 #define MEFISTO2D_EXPORT
#endif


MEFISTO2D_EXPORT
  void  aptrte( Z nutysu, R aretmx,
              Z nblf,   Z *nudslf, R_2 *uvslf,
              Z nbpti,  R_2 *uvpti,
              Z & nbst, R_2 * & uvst, Z & nbt, Z * & nust,
              Z & ierr );
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// but : appel de la triangulation par un arbre-4 recouvrant
// ----- de triangles equilateraux
//       le contour du domaine plan est defini par des lignes fermees
//       la premiere ligne etant l'enveloppe de toutes les autres
//       la fonction areteideale_(s,d) donne la taille d'arete
//       au point s dans la direction d (direction inactive pour l'instant)
//       des lors toute arete issue d'un sommet s devrait avoir une longueur
//       comprise entre 0.65 areteideale_(s,d) et 1.3 areteideale_(s,d)
//
//Attention:
//  Les tableaux uvslf et uvpti sont supposes ne pas avoir de sommets identiques!
//  De meme, un sommet d'une ligne fermee ne peut appartenir a une autre ligne fermee
//
// entrees:
// --------
// nutysu : numero de traitement de areteideale_() selon le type de surface
//          0 pas d'emploi de la fonction areteideale_() et aretmx est active
//          1 il existe une fonction areteideale_(s,d)
//            dont seules les 2 premieres composantes de uv sont actives
//          ... autres options a definir ...
// aretmx : longueur maximale des aretes de la future triangulation
// nblf   : nombre de lignes fermees de la surface
// nudslf : numero du dernier sommet de chacune des nblf lignes fermees
//          nudslf(0)=0 pour permettre la difference sans test
//          Attention le dernier sommet de chaque ligne est raccorde au premier
//          tous les sommets et les points internes ont des coordonnees
//          UV differentes <=> Pas de point double!
// uvslf  : uv des nudslf(nblf) sommets des lignes fermees
// nbpti  : nombre de points internes futurs sommets de la triangulation
// uvpti  : uv des points internes futurs sommets de la triangulation
//
// sorties:
// --------
// nbst   : nombre de sommets de la triangulation finale
// uvst   : coordonnees uv des nbst sommets de la triangulation
// nbt    : nombre de triangles de la triangulation finale
// nust   : 3 numeros dans uvst des sommets des nbt triangles
// ierr   : 0 si pas d'erreur
//        > 0 sinon
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// auteur : Alain Perronnet  Analyse Numerique Paris UPMC   decembre 2001
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#if WIN32 & DFORTRAN
  #define tempscpu TEMPSCPU
  #define deltacpu DELTACPU
  #define insoar   INSOAR
  #define azeroi   AZEROI
  #define fasoar   FASOAR
  #define teajte   TEAJTE
  #define tehote   TEHOTE
  #define tetrte   TETRTE
  #define aisoar   AISOAR
  #define tedela   TEDELA
  #define terefr   TEREFR
  #define tesuex   TESUEX
  #define teamqt   TEAMQT
  #define nusotr   NUSOTR
  #define qutr2d   QUTR_2D
  #define surtd2   SURTD2
  #define qualitetrte   QUALITETRTE
  
  #define areteideale ARETEIDEALE
  
#else
  #define tempscpu tempscpu_
  #define deltacpu deltacpu_
  #define insoar   insoar_
  #define azeroi   azeroi_
  #define fasoar   fasoar_
  #define teajte   teajte_
  #define tehote   tehote_
  #define tetrte   tetrte_
  #define aisoar   aisoar_
  #define tedela   tedela_
  #define terefr   terefr_
  #define tesuex   tesuex_
  #define teamqt   teamqt_
  #define nusotr   nusotr_
  #define qutr2d   qutr2d_
  #define surtd2   surtd2_
  #define qualitetrte   qualitetrte_

  #define areteideale areteideale_

#endif


extern "C" { void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
   qualitetrte( R_3 *mnpxyd,
                   Z & mosoar, Z & mxsoar, Z *mnsoar,
                   Z & moartr, Z & mxartr, Z *mnartr,
                   Z & nbtria, R & quamoy, R & quamin ); }
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// but :    calculer la qualite moyenne et minimale de la triangulation
// -----    actuelle definie par les tableaux nosoar et noartr
// entrees:
// --------
// mnpxyd : tableau des coordonnees 2d des points
//          par point : x  y  distance_souhaitee
// mosoar : nombre maximal d'entiers par arete et
//          indice dans nosoar de l'arete suivante dans le hachage
// mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar
//          attention: mxsoar>3*mxsomm obligatoire!
// nosoar : numero des 2 sommets , no ligne, 2 triangles de l'arete,
//          chainage des aretes frontalieres, chainage du hachage des aretes
//          hachage des aretes = nosoar(1)+nosoar(2)*2
//          avec mxsoar>=3*mxsomm
//          une arete i de nosoar est vide <=> nosoar(1,i)=0 et
//          nosoar(2,arete vide)=l'arete vide qui precede
//          nosoar(3,arete vide)=l'arete vide qui suit
// moartr : nombre maximal d'entiers par arete du tableau noartr
// mxartr : nombre maximal de triangles declarables
// noartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3
//          arete1 = 0 si triangle vide => arete2 = triangle vide suivant
// sorties:
// --------
// nbtria : nombre de triangles internes au domaine
// quamoy : qualite moyenne  des triangles actuels
// quamin : qualite minimale des triangles actuels
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern "C" {  void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  tempscpu( double & tempsec );
}
    
//Retourne le temps CPU utilise en secondes

extern "C" { void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  deltacpu( R & dtcpu );
}
    
//Retourne le temps CPU utilise en secondes depuis le precedent appel

//initialiser le tableau mnsoar pour le hachage des aretes
extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  insoar( Z & mxsomm, Z & mosoar, Z & mxsoar, Z & n1soar, Z * mnsoar );
}

//mettre a zero les nb entiers de tab
extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  azeroi( Z & nb, Z * tab );
}

extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  fasoar( Z & ns1, Z & ns2, Z & nt1, Z & nt2, Z & nolign,
                          Z & mosoar,  Z & mxsoar,  Z & n1soar,  Z * mnsoar,  Z * mnarst,
                          Z & noar, Z & ierr );
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// but :    former l'arete de sommet ns1-ns2 dans le hachage du tableau
// -----    nosoar des aretes de la triangulation
// entrees:
// --------
// ns1 ns2: numero pxyd des 2 sommets de l'arete
// nt1    : numero du triangle auquel appartient l'arete
//          nt1=-1 si numero inconnu
// nt2    : numero de l'eventuel second triangle de l'arete si connu
//          nt2=-1 si numero inconnu
// nolign : numero de la ligne fermee de l'arete
//          =0 si l'arete n'est une arete de ligne
//          ce numero est ajoute seulement si l'arete est creee
// mosoar : nombre maximal d'entiers par arete du tableau nosoar
// mxsoar : nombre maximal d'aretes stockables dans le tableau nosoar
// modifies:
// ---------
// n1soar : numero de la premiere arete vide dans le tableau nosoar
//          une arete i de nosoar est vide  <=>  nosoar(1,i)=0
//          chainage des aretes vides amont et aval
//          l'arete vide qui precede=nosoar(4,i)
//          l'arete vide qui suit   =nosoar(5,i)
// nosoar : numero des 2 sommets, no ligne, 2 triangles de l'arete,
//          chainage momentan'e d'aretes, chainage du hachage des aretes
//          hachage des aretes = min( nosoar(1), nosoar(2) )
// noarst : noarst(np) numero d'une arete du sommet np

// ierr   : si < 0  en entree pas d'affichage en cas d'erreur du type
//         "arete appartenant a plus de 2 triangles et a creer!"
//          si >=0  en entree       affichage de ce type d'erreur
// sorties:
// --------
// noar   : >0 numero de l'arete retrouvee ou ajoutee
// ierr   : =0 si pas d'erreur
//          =1 si le tableau nosoar est sature
//          =2 si arete a creer et appartenant a 2 triangles distincts
//             des triangles nt1 et nt2
//          =3 si arete appartenant a 2 triangles distincts
//             differents des triangles nt1 et nt2
//          =4 si arete appartenant a 2 triangles distincts
//             dont le second n'est pas le triangle nt2
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//initialisation du tableau letree et ajout dans letree des sommets 1 a nbsomm
extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  teajte( Z & mxsomm, Z &  nbsomm, R_3 * mnpxyd,  R_3 * comxmi,
                            R & aretmx,  Z & mxtree, Z * letree,
                            Z & ierr );
}

extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  tehote( Z & nutysu, Z & nbarpi, Z &  mxsomm, Z &  nbsomm, R_3 * mnpxyd,
                            R_3 * comxmi, R & aretmx,
                            Z * letree, Z & mxqueu, Z * mnqueu,
                            Z & ierr );
}
// homogeneisation de l'arbre des te a un saut de taille au plus
// prise en compte des tailles d'aretes souhaitees autour des sommets initiaux

extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  tetrte( R_3 * comxmi, R & aretmx, Z & nbarpi, Z & mxsomm, R_3 * mnpxyd,
                            Z & mxqueu,  Z * mnqueu,  Z * mntree,
                            Z & mosoar,  Z & mxsoar,  Z & n1soar, Z * mnsoar,
                            Z & moartr, Z &  mxartr,  Z & n1artr,  Z * mnartr,  Z * mnarst,
                            Z & ierr );
}
// trianguler les triangles equilateraux feuilles a partir de leurs 3 sommets
// et des points de la frontiere, des points internes imposes interieurs

extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  aisoar( Z & mosoar, Z & mxsoar, Z * mnsoar, Z & na );
}
// formation du chainage 6 des aretes internes a echanger eventuellement

extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  tedela( R_3 * mnpxyd, Z * mnarst,
                            Z & mosoar, Z & mxsoar, Z & n1soar, Z * mnsoar, Z & na,
                            Z & moartr, Z & mxartr, Z & n1artr, Z * mnartr, Z & n );
}
// boucle sur les aretes internes (non sur une ligne de la frontiere)
// avec echange des 2 diagonales afin de rendre la triangulation delaunay
 
extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  terefr( Z & nbarpi, R_3 * mnpxyd,
                            Z & mosoar, Z & mxsoar, Z & n1soar, Z * mnsoar,
                            Z & moartr, Z & mxartr, Z & n1artr, Z * mnartr, Z * mnarst,
                            Z & mxarcf, Z * mnarc1, Z * mnarc2,
                            Z * mnarc3, Z * mnarc4,
                            Z & n, Z & ierr );
}
// detection des aretes frontalieres initiales perdues
// triangulation frontale pour les restaurer

extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  tesuex( Z & nblf, Z * nulftr,
                            Z & ndtri0, Z & nbsomm, R_3 * mnpxyd, Z * mnslig,
                            Z & mosoar, Z & mxsoar, Z * mnsoar,
                            Z & moartr, Z & mxartr, Z & n1artr, Z * mnartr, Z * mnarst,
                            Z & nbtria, Z * mntrsu, Z & ierr );
}
// suppression des triangles externes a la surface

extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  teamqt( Z & nutysu, R & aretmx, R & airemx,
                            Z * mnarst, Z & mosoar, Z & mxsoar, Z & n1soar, Z * mnsoar,
                            Z & moartr, Z & mxartr, Z & n1artr, Z * mnartr,
                            Z & mxarcf, Z * mntrcf, Z * mnstbo,
                            Z * n1arcf, Z * mnarcf, Z * mnarc1,
                            Z & nbarpi, Z & nbsomm, Z & mxsomm,
                            R_3 * mnpxyd, Z * mnslig,
                            Z & ierr );
}
// amelioration de la qualite de la triangulation par
// barycentrage des sommets internes a la triangulation
// suppression des aretes trop longues ou trop courtes
// modification de la topologie des groupes de triangles
// mise en delaunay de la triangulation
 
extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  nusotr( Z & nt, Z & mosoar, Z * mnsoar, Z & moartr, Z * mnartr,Z * nosotr );
}
//retrouver les numero des 3 sommets du triangle nt

extern "C" {void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  qutr2d( R_3 & p1, R_3 & p2, R_3 & p3, R & qualite );
}
//calculer la qualite d'un triangle de R2 de sommets p1, p2, p3

extern "C" { R
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
  surtd2( R_3 & p1, R_3 & p2, R_3 & p3 );
}
//calcul de la surface d'un triangle defini par 3 points de r**2

#endif
