//  MEFISTO2: a library to compute 2D triangulation from segmented boundaries
//
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
//  File   : aptrte.cxx   le C++ de l'appel du trianguleur plan
//  Module : SMESH
//  Author : Alain PERRONNET
//  Date   : 13 novembre 2006

#include "Rn.h"
#include "aptrte.h"
#include "utilities.h"

using namespace std;

extern "C"
{
  R aretemaxface_;
  MEFISTO2D_EXPORT   
    R
  #ifdef WIN32
  #ifdef F2C_BUILD
  #else
      __stdcall
  #endif
  #endif
      areteideale(R &_areteideale)
  {
        _areteideale = aretemaxface_;
	return(_areteideale);
  }
}
//calcul de la longueur ideale de l'arete au sommet xyz (z ici inactif)
//dans la direction donnee
//a ajuster pour chaque surface plane et selon l'entier notysu (voir plus bas)


static double cpunew, cpuold=0;

void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
tempscpu_( double & tempsec )
//Retourne le temps CPU utilise en secondes
{  
  tempsec = ( (double) clock() ) / CLOCKS_PER_SEC;
  //MESSAGE( "temps cpu=" << tempsec );
}


void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
deltacpu_( R & dtcpu )
//Retourne le temps CPU utilise en secondes depuis le precedent appel
{
  tempscpu_( cpunew );
  dtcpu  = R( cpunew - cpuold );
  cpuold = cpunew;
  //MESSAGE( "delta temps cpu=" << dtcpu );
  return;
}


void  aptrte( Z   nutysu, R      aretmx,
              Z   nblf,   Z  *   nudslf,  R_2 * uvslf,
              Z   nbpti,  R_2 *   uvpti,
              Z & nbst,   R_2 * & uvst,
              Z & nbt,    Z  * & nust,
              Z & ierr )
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// but : appel de la triangulation par un arbre-4 recouvrant
// ----- de triangles equilateraux
//       le contour du domaine plan est defini par des lignes fermees
//       la premiere ligne etant l'enveloppe de toutes les autres
//       la fonction areteideale(s,d) donne la taille d'arete
//       au point s dans la direction (actuellement inactive) d
//       des lors toute arete issue d'un sommet s devrait avoir une longueur
//       comprise entre 0.65 areteideale_(s,d) et 1.3 areteideale_(s,d)
//
//Attention:
//  Les tableaux uvslf et uvpti sont supposes ne pas avoir de sommets identiques!
//  De meme, un sommet d'une ligne fermee ne peut appartenir a une autre ligne fermee
//
// entrees:
// --------
// nutysu : numero de traitement de areteideale_(s,d) selon le type de surface
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
// nust   : 4 numeros dans uvst des sommets des nbt triangles
//          s1, s2, s3, 0: no dans uvst des 3 sommets et 0 car quadrangle!
// ierr   : 0 si pas d'erreur
//        > 0 sinon
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// auteur : Alain Perronnet  Laboratoire J.-L. LIONS Paris UPMC mars 2006
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
{
  Z  nbsttria=4; //Attention: 4 sommets stockes par triangle
                 //no st1, st2, st3, 0 (non quadrangle)

  R  d, tcpu=0;
//  R_3 direction=R_3(0,0,0);  //direction pour areteideale() inactive ici!
  Z  nbarfr=nudslf[nblf];  //nombre total d'aretes des lignes fermees
  Z  mxtrou = Max( 1024, nblf );  //nombre maximal de trous dans la surface

  R_3 *mnpxyd=NULL;
  Z  *mnsoar=NULL, mosoar=7, mxsoar, n1soar; //le hachage des aretes
  Z  *mnartr=NULL, moartr=3, mxartr, n1artr; //le no des 3 aretes des triangles
  Z  *mntree=NULL, motree=9, mxtree; //L'arbre 4 de TE et nombre d'entiers par TE
  Z  *mnqueu=NULL, mxqueu;
  Z  *mn1arcf=NULL;
  Z  *mnarcf=NULL, mxarcf;
  Z  *mnarcf1=NULL;
  Z  *mnarcf2=NULL;
  Z  *mnarcf3=NULL;
  Z  *mntrsu=NULL;
  Z  *mnslig=NULL;
  Z  *mnarst=NULL;
  Z  *mnlftr=NULL;

  R_3 comxmi[2];            //coordonnees UV Min et Maximales
  R  aremin, aremax;       //longueur minimale et maximale des aretes
  R  airemx;               //aire maximale souhaitee d'un triangle
  R  quamoy, quamin;

  Z  noar0, noar, na;
  Z  i, l, n, ns, ns0, ns1, ns2, nosotr[3], nt;
  Z  mxsomm, nbsomm, nbarpi, nbarli, ndtri0, mn;
  Z  moins1=-1;
  Z  nuds = 0;

  // initialisation du temps cpu
  deltacpu_( d );
  ierr = 0;

  // quelques reservations de tableaux pour faire les calculs
  // ========================================================
  // declaration du tableau des coordonnees des sommets de la frontiere
  // puis des sommets internes ajoutes
  // majoration empirique du nombre de sommets de la triangulation
  i = 4*nbarfr/10;
  mxsomm = Max( 20000, 64*nbpti+i*i );
  MESSAGE( "APTRTE: Debut de la triangulation plane avec " );
  MESSAGE( "nutysu=" << nutysu << "  aretmx=" << aretmx
           << "  mxsomm=" << mxsomm );
  MESSAGE( nbarfr << " sommets sur la frontiere et " << nbpti << " points internes");

 NEWDEPART:
  //mnpxyd( 3, mxsomm ) les coordonnees UV des sommets et la taille d'arete aux sommets
  if( mnpxyd!=NULL ) delete [] mnpxyd;
  mnpxyd = new R_3[mxsomm];
  if( mnpxyd==NULL ) goto ERREUR;

  // le tableau mnsoar des aretes des triangles
  // 1: sommet 1 dans pxyd,
  // 2: sommet 2 dans pxyd,
  // 3: numero de 1 a nblf de la ligne qui supporte l'arete
  // 4: numero dans mnartr du triangle 1 partageant cette arete,
  // 5: numero dans mnartr du triangle 2 partageant cette arete,
  // 6: chainage des aretes frontalieres ou internes ou
  //    des aretes simples des etoiles de triangles,
  // 7: chainage du hachage des aretes
  // nombre d'aretes = 3 ( nombre de sommets - 1 + nombre de trous )
  // pour le hachage des aretes mxsoar doit etre > 3*mxsomm!
  // h(ns1,ns2) = min( ns1, ns2 )
  if( mnsoar!=NULL ) delete [] mnsoar;
  mxsoar = 3 * ( mxsomm + mxtrou );
  mnsoar = new Z[mosoar*mxsoar];
  if( mnsoar==NULL ) goto ERREUR;
  //initialiser le tableau mnsoar pour le hachage des aretes
  insoar( mxsomm, mosoar, mxsoar, n1soar, mnsoar );

  // mnarst( mxsomm ) numero mnsoar d'une arete pour chacun des sommets
  if( mnarst!=NULL ) delete [] mnarst;
  mnarst = new Z[1+mxsomm];
  if( mnarst==NULL ) goto ERREUR;
  n = 1+mxsomm;
  azeroi( n, mnarst );

  // mnslig( mxsomm ) no de sommet dans sa ligne pour chaque sommet frontalier
  //               ou no du point si interne forc'e par l'utilisateur
  //               ou  0 si interne cree par le module
  if( mnslig!=NULL ) delete [] mnslig;
  mnslig = new Z[mxsomm];
  if( mnslig==NULL ) goto ERREUR;
  azeroi( mxsomm, mnslig );

  // initialisation des aretes frontalieres de la triangulation future
  // renumerotation des sommets des aretes des lignes pour la triangulation
  // mise a l'echelle des coordonnees des sommets pour obtenir une
  // meilleure precision lors des calculs + quelques verifications
  // boucle sur les lignes fermees qui forment la frontiere
  // ======================================================================
  noar = 0;
  aremin = 1e100;
  aremax = 0;

  for (n=1; n<=nblf; n++)
  {
    //l'initialisation de la premiere arete de la ligne n dans la triangulation
    //-------------------------------------------------------------------------
    //le sommet ns0 est le numero de l'origine de la ligne
    ns0 = nudslf[n-1];
    mnpxyd[ns0].x = uvslf[ns0].x;
    mnpxyd[ns0].y = uvslf[ns0].y;
    mnpxyd[ns0].z = areteideale((R &)mnpxyd[ns0].z);//( mnpxyd[ns0], direction );
//     MESSAGE("Sommet " << ns0 << ": " << mnpxyd[ns0].x
//       << " " << mnpxyd[ns0].y << " longueur arete=" << mnpxyd[ns0].z);

    //carre de la longueur de l'arete 1 de la ligne fermee n
    d = pow( uvslf[ns0+1].x - uvslf[ns0].x, 2 ) 
      + pow( uvslf[ns0+1].y - uvslf[ns0].y, 2 ) ;
    aremin = Min( aremin, d );
    aremax = Max( aremax, d );

    //le numero des 2 sommets (ns1,ns2) de la premiere arete de la ligne
    //initialisation de la 1-ere arete ns1-ns1+1 de cette ligne fermee n
    //le numero des 2 sommets ns1 ns2 de la 1-ere arete
    //Attention: les numeros ns debutent a 1 (ils ont >0)
    //           les tableaux c++ demarrent a zero!
    //           les tableaux fortran demarrent ou l'on veut!
    ns0++;
    ns1 = ns0;
    ns2 = ns1+1;

     //le numero n de la ligne du sommet et son numero ns1 dans la ligne
    mnslig[ns0-1] = 1000000 * n + ns1-nudslf[n-1];
    fasoar( ns1, ns2, moins1, moins1, n,
             mosoar, mxsoar, n1soar, mnsoar, mnarst,
             noar0,  ierr );
    //pas de test sur ierr car pas de saturation possible a ce niveau

    //le pointeur dans le hachage sur la premiere arete de la ligne fermee n
    //mndalf[n] = noar0;

    //la nouvelle arete est la suivante de l'arete definie juste avant
    if( noar > 0 )
      mnsoar[mosoar * noar - mosoar + 5] = noar0;

    //l'initialisation des aretes suivantes de la ligne dans la triangulation
    //-----------------------------------------------------------------------
    nbarli = nudslf[n] - nudslf[n-1];  //nombre d'aretes=sommets de la ligne n
    for (i=2; i<=nbarli; i++)
    {
      ns1 = ns2; //le numero de l'arete et le numero du premier sommet de l'arete
      if( i < nbarli )
        //nbs+1 est le 2-eme sommet de l'arete i de la ligne fermee n
        ns2 = ns1+1;
      else
        //le 2-eme sommet de la derniere arete est le premier sommet de la ligne
        ns2 = ns0;

      //l'arete precedente est dotee de sa suivante:celle cree ensuite
      //les 2 coordonnees du sommet ns2 de la ligne
      ns = ns1 - 1;
//debut ajout  5/10/2006  ................................................
      nuds = Max( nuds, ns );   //le numero du dernier sommet traite
//fin   ajout  5/10/2006  ................................................
      mnpxyd[ns].x = uvslf[ns].x;
      mnpxyd[ns].y = uvslf[ns].y;
      mnpxyd[ns].z = areteideale((R &)mnpxyd[ns].z);//( mnpxyd[ns], direction );
//       MESSAGE("Sommet " << ns << ": " << mnpxyd[ns].x
//         << " " << mnpxyd[ns].y << " longueur arete=" << mnpxyd[ns].z);

      //carre de la longueur de l'arete
      d = pow( uvslf[ns2-1].x - uvslf[ns1-1].x, 2) 
        + pow( uvslf[ns2-1].y - uvslf[ns1-1].y, 2);
      aremin = Min( aremin, d );
      aremax = Max( aremax, d );

//debut ajout du 5/10/2006  .............................................
      //la longueur de l'arete ns1-ns2
      d = sqrt( d );
      //longueur arete = Min ( aretmx, aretes incidentes )
      mnpxyd[ns   ].z = Min( mnpxyd[ns   ].z, d );
      mnpxyd[ns2-1].z = Min( mnpxyd[ns2-1].z, d );
//fin ajout du 5/10/2006  ...............................................

      //le numero n de la ligne du sommet et son numero ns1 dans la ligne
      mnslig[ns] = 1000000 * n + ns1-nudslf[n-1];

      //ajout de l'arete dans la liste
      fasoar( ns1, ns2, moins1, moins1, n,
               mosoar, mxsoar, n1soar, mnsoar,
               mnarst, noar, ierr );
      //pas de test sur ierr car pas de saturation possible a ce niveau

      //chainage des aretes frontalieres en position 6 du tableau mnsoar
      //la nouvelle arete est la suivante de l'arete definie juste avant
      mnsoar[ mosoar * noar0 - mosoar + 5 ] = noar;
      noar0 = noar;
   }
    //attention: la derniere arete de la ligne fermee enveloppe
    //           devient en fait la premiere arete de cette ligne
    //           dans le chainage des aretes de la frontiere!
  }
  if( ierr != 0 ) goto ERREUR;

  aremin = sqrt( aremin );  //longueur minimale d'une arete des lignes fermees
  aremax = sqrt( aremax );  //longueur maximale d'une arete

//debut ajout  9/11/2006  ................................................
  // devenu un commentaire aretmx = Min( aretmx, aremax ); //pour homogeneiser

  // protection contre une arete max desiree trop grande ou trop petite
  if( aretmx > aremax*2.05 ) aretmx = aremax;

  // protection contre une arete max desiree trop petite
  if( (aremax-aremin) > (aremin+aremax)*0.05 && aretmx < aremin*0.5 )
    aretmx =(aremin+aremax*2)/3.0;

  if( aretmx < aremin  && aremin > 0 )
    aretmx = aremin;

  //sauvegarde pour la fonction areteideale_
  aretemaxface_ = aretmx;

  //aire maximale souhaitee des triangles
  airemx = aretmx * aretmx * sqrt(3.0) / 2.0;  //Aire triangle equilateral

  for(i=0; i<=nuds; i++ )
    mnpxyd[i].z = Min( mnpxyd[i].z, aretmx );
  //MESSAGE("Numero du dernier sommet frontalier=" << nuds+1);
//fin  ajout 9/11/2006  .................................................


  MESSAGE("Sur  le  bord: arete min=" << aremin << " arete max=" << aremax );
  MESSAGE("Triangulation: arete mx=" << aretmx
          << " triangle aire mx=" << airemx );

  //chainage des aretes frontalieres : la derniere arete frontaliere
  mnsoar[ mosoar * noar - mosoar + 5 ] = 0;

  //tous les sommets et aretes frontaliers sont numerotes de 1 a nbarfr
  //reservation du tableau des numeros des 3 aretes de chaque triangle
  //mnartr( moartr, mxartr )
  //En nombre: Triangles = Aretes Internes + Aretes Frontalieres - Sommets + 1-Trous
  //          3Triangles = 2 Aretes internes + Aretes frontalieres
  //       d'ou 3T/2 < AI + AF => T < 3T/2  - Sommets + 1-Trous
  //nombre de triangles < 2 ( nombre de sommets - 1 + nombre de trous )
  if( mnartr!=NULL ) delete [] mnartr;
  mxartr = 2 * ( mxsomm + mxtrou );
  mnartr = new Z[moartr*mxartr];
  if( mnartr==NULL ) goto ERREUR;

  //Ajout des points internes
  ns1 = nudslf[ nblf ];
  for (i=0; i<nbpti; i++)
  {
    //les 2 coordonnees du point i de sommet nbs
    mnpxyd[ns1].x = uvpti[i].x;
    mnpxyd[ns1].y = uvpti[i].y;
    mnpxyd[ns1].z = areteideale((R &)mnpxyd[ns1].z);//( mnpxyd[ns1], direction );
    //le numero i du point interne
    mnslig[ns1] = i+1;
    ns1++;
  }

  //nombre de sommets de la frontiere et internes
  nbarpi = ns1;

  // creation de l'arbre-4 des te (tableau letree)
  // ajout dans les te des sommets des lignes et des points internes imposes
  // =======================================================================
  // premiere estimation de mxtree
  mxtree = 2 * mxsomm;

 NEWTREE:  //en cas de saturation de l'un des tableaux, on boucle
  MESSAGE( "Debut triangulation avec mxsomm=" << mxsomm );
  if( mntree != NULL ) delete [] mntree;
  nbsomm = nbarpi;
  mntree = new Z[motree*(1+mxtree)];
  if( mntree==NULL ) goto ERREUR;

  //initialisation du tableau letree et ajout dans letree des sommets 1 a nbsomm
  comxmi[0].x = comxmi[1].x = uvslf[0].x;
  comxmi[0].y = comxmi[1].y = uvslf[0].y;
  teajte( mxsomm, nbsomm, mnpxyd, comxmi, aretmx, mxtree, mntree, ierr );
  comxmi[0].z=0;
  comxmi[1].z=0;

  if( ierr == 51 )
  {
    //saturation de letree => sa taille est augmentee et relance
    mxtree = mxtree * 2;
    ierr   = 0;
    MESSAGE( "Nouvelle valeur de mxtree=" << mxtree );
    goto NEWTREE;
  }

  deltacpu_( d );
  tcpu += d;
  MESSAGE( "Temps de l'ajout arbre-4 des Triangles Equilateraux=" << d << " secondes" );
  if( ierr != 0 ) goto ERREUR;
  //ici le tableau mnpxyd contient les sommets des te et les points frontaliers et internes

  // homogeneisation de l'arbre des te a un saut de taille au plus
  // prise en compte des tailles d'aretes souhaitees autour des sommets initiaux
  // ===========================================================================
  // reservation de la queue pour parcourir les te de l'arbre
  if( mnqueu != NULL ) delete [] mnqueu;
  mxqueu = mxtree;
  mnqueu = new Z[mxqueu];
  if( mnqueu==NULL) goto ERREUR;

  tehote( nutysu, nbarpi, mxsomm, nbsomm, mnpxyd,
           comxmi, aretmx,
           mntree, mxqueu, mnqueu,
           ierr );

  deltacpu_( d );
  tcpu += d;
  MESSAGE("Temps de l'adaptation et l'homogeneisation de l'arbre-4 des TE="
       << d << " secondes");
  if( ierr != 0 )
  {
    //destruction du tableau auxiliaire et de l'arbre
    if( ierr == 51 )
    {
      //letree sature
      mxtree = mxtree * 2;
      MESSAGE( "Redemarrage avec la valeur de mxtree=" << mxtree );
      ierr = 0;
      goto NEWTREE;
    }
    else
      goto ERREUR;
  }

  // trianguler les triangles equilateraux feuilles a partir de leurs 3 sommets
  // et des points de la frontiere, des points internes imposes interieurs
  // ==========================================================================
  tetrte( comxmi, aretmx, nbarpi, mxsomm, mnpxyd,
           mxqueu, mnqueu, mntree, mosoar, mxsoar, n1soar, mnsoar,
           moartr, mxartr, n1artr, mnartr, mnarst,
           ierr );

  // destruction de la queue et de l'arbre devenus inutiles
  delete [] mnqueu;  mnqueu=NULL;
  delete [] mntree;  mntree=NULL;

  //Temps calcul
  deltacpu_( d );
  tcpu += d;
  MESSAGE( "Temps de la triangulation des TE=" << d << " secondes" );

  // ierr =0 si pas d'erreur
  //      =1 si le tableau mnsoar est sature
  //      =2 si le tableau mnartr est sature
  //      =3 si aucun des triangles ne contient l'un des points internes
  //      =5 si saturation de la queue de parcours de l'arbre des te
  if( ierr != 0 ) goto ERREUR;

  //qualites de la triangulation actuelle
  qualitetrte( mnpxyd, mosoar, mxsoar, mnsoar, moartr, mxartr, mnartr,
                nbt, quamoy, quamin );

  // boucle sur les aretes internes (non sur une ligne de la frontiere)
  // avec echange des 2 diagonales afin de rendre la triangulation delaunay
  // ======================================================================
  // formation du chainage 6 des aretes internes a echanger eventuellement
  aisoar( mosoar, mxsoar, mnsoar, na );
  tedela( mnpxyd, mnarst,
           mosoar, mxsoar, n1soar, mnsoar, na,
           moartr, mxartr, n1artr, mnartr, n );

  MESSAGE( "Nombre d'echanges des diagonales de 2 triangles=" << n );
  deltacpu_( d );
  tcpu += d;
  MESSAGE("Temps de la triangulation Delaunay par echange des diagonales="
       << d << " secondes");

  //qualites de la triangulation actuelle
  qualitetrte( mnpxyd, mosoar, mxsoar, mnsoar, moartr, mxartr, mnartr,
                nbt, quamoy, quamin );

  // detection des aretes frontalieres initiales perdues
  // triangulation frontale pour les restaurer
  // ===================================================
  mxarcf = mxsomm/5;
  if( mn1arcf != NULL ) delete [] mn1arcf;
  if( mnarcf  != NULL ) delete [] mnarcf;
  if( mnarcf1 != NULL ) delete [] mnarcf1;
  if( mnarcf2 != NULL ) delete [] mnarcf2;
  mn1arcf = new Z[1+mxarcf];
  if( mn1arcf == NULL ) goto ERREUR;
  mnarcf  = new Z[3*mxarcf];
  if( mnarcf == NULL ) goto ERREUR;
  mnarcf1 = new Z[mxarcf];
  if( mnarcf1 == NULL ) goto ERREUR;
  mnarcf2 = new Z[mxarcf];
  if( mnarcf2 == NULL ) goto ERREUR;

  terefr( nbarpi, mnpxyd,
           mosoar, mxsoar, n1soar, mnsoar,
           moartr, mxartr, n1artr, mnartr, mnarst,
           mxarcf, mn1arcf, mnarcf, mnarcf1, mnarcf2,
           n, ierr );

  MESSAGE( "Restauration de " << n << " aretes perdues de la frontiere  ierr=" << ierr );
  deltacpu_( d );
  tcpu += d;
  MESSAGE("Temps de la recuperation des aretes perdues de la frontiere="
       << d << " secondes");

  if( ierr != 0 ) goto ERREUR;

  //qualites de la triangulation actuelle
  qualitetrte( mnpxyd, mosoar, mxsoar, mnsoar, moartr, mxartr, mnartr,
                nbt, quamoy, quamin );

  // fin de la triangulation avec respect des aretes initiales frontalieres

  // suppression des triangles externes a la surface
  // ===============================================
  // recherche du dernier triangle utilise
  mn = mxartr * moartr;
  for ( ndtri0=mxartr; ndtri0<=1; ndtri0-- )
  {
    mn -= moartr;
    if( mnartr[mn] != 0 ) break;
  }

  if( mntrsu != NULL ) delete [] mntrsu;
  mntrsu = new Z[ndtri0];
  if( mntrsu == NULL ) goto ERREUR;

  if( mnlftr != NULL ) delete [] mnlftr;
  mnlftr = new Z[nblf];
  if( mnlftr == NULL ) goto ERREUR;

  for (n=0; n<nblf; n++)  //numero de la ligne fermee de 1 a nblf
    mnlftr[n] = n+1;

  tesuex( nblf,   mnlftr,
           ndtri0, nbsomm, mnpxyd, mnslig,
           mosoar, mxsoar, mnsoar,
           moartr, mxartr, n1artr, mnartr, mnarst,
           nbt, mntrsu, ierr );

  delete [] mnlftr; mnlftr=NULL;
  delete [] mntrsu; mntrsu=NULL;

  deltacpu_( d );
  tcpu += d;
  MESSAGE( "Temps de la suppression des triangles externes=" << d << "ierr=" << ierr );
  if( ierr != 0 ) goto ERREUR;

  //qualites de la triangulation actuelle
  qualitetrte( mnpxyd, mosoar, mxsoar, mnsoar, moartr, mxartr, mnartr,
                nbt, quamoy, quamin );

  // amelioration de la qualite de la triangulation par
  // barycentrage des sommets internes a la triangulation
  // suppression des aretes trop longues ou trop courtes
  // modification de la topologie des groupes de triangles
  // mise en delaunay de la triangulation
  // =====================================================
  mnarcf3 = new Z[mxarcf];
  if( mnarcf3 == NULL )
  {
    MESSAGE ( "aptrte: MC saturee mnarcf3=" << mnarcf3 );
    goto ERREUR;
  }
  teamqt( nutysu,  aretmx,  airemx,
           mnarst,  mosoar,  mxsoar, n1soar, mnsoar,
           moartr,  mxartr,  n1artr, mnartr,
           mxarcf,  mnarcf2, mnarcf3,
           mn1arcf, mnarcf,  mnarcf1,
           nbarpi,  nbsomm, mxsomm, mnpxyd, mnslig,
           ierr );
  if( mnarcf3 != NULL ) {delete [] mnarcf3; mnarcf3=NULL;}
  if( mn1arcf != NULL ) {delete [] mn1arcf; mn1arcf=NULL;}
  if( mnarcf  != NULL ) {delete [] mnarcf;  mnarcf =NULL;}
  if( mnarcf1 != NULL ) {delete [] mnarcf1; mnarcf1=NULL;}
  if( mnarcf2 != NULL ) {delete [] mnarcf2; mnarcf2=NULL;}

  deltacpu_( d );
  tcpu += d;
  MESSAGE( "Temps de l'amelioration de la qualite de la triangulation=" << d );
  if( ierr == -13 ) ierr=0; //6/10/2006 arret de l'amelioration apres boucle infinie dans caetoi
  if( ierr !=   0 ) goto ERREUR;

  //qualites de la triangulation finale
  qualitetrte( mnpxyd, mosoar, mxsoar, mnsoar, moartr, mxartr, mnartr,
                nbt, quamoy, quamin );

  // renumerotation des sommets internes: mnarst(i)=numero final du sommet
  // ===================================
  for (i=0; i<=nbsomm; i++)
    mnarst[i] = 0;

  for (nt=1; nt<=mxartr; nt++)
  {
    if( mnartr[nt*moartr-moartr] != 0 )
    {
      //le numero des 3 sommets du triangle nt
      nusotr( nt, mosoar, mnsoar, moartr, mnartr, nosotr );
      //les 3 sommets du triangle sont actifs
      mnarst[ nosotr[0] ] = 1;
      mnarst[ nosotr[1] ] = 1;
      mnarst[ nosotr[2] ] = 1;
    }
  }
  nbst = 0;
  for (i=1; i<=nbsomm; i++)
  {
    if( mnarst[i] >0 )
      mnarst[i] = ++nbst;
  }

  // generation du tableau uvst de la surface triangulee
  // ---------------------------------------------------
  if( uvst != NULL ) delete [] uvst;
  uvst = new R_2[nbst];
  if( uvst == NULL ) goto ERREUR;

  nbst=-1;
  for (i=0; i<nbsomm; i++ )
  {
    if( mnarst[i+1]>0 )
    {
      nbst++;
      uvst[nbst].x = mnpxyd[i].x;
      uvst[nbst].y = mnpxyd[i].y;

      //si le sommet est un point ou appartient a une ligne
      //ses coordonnees initiales sont restaurees
      n = mnslig[i];
      if( n > 0 )
      {
        if( n >= 1000000 )
        {
          //sommet d'une ligne
          //retour aux coordonnees initiales dans uvslf
          l = n / 1000000;
          n = n - 1000000 * l + nudslf[l-1] - 1;
          uvst[nbst].x = uvslf[n].x;
          uvst[nbst].y = uvslf[n].y;
        }
        else
        {
          //point utilisateur n interne impose
          //retour aux coordonnees initiales dans uvpti
          uvst[nbst].x = uvpti[n-1].x;
          uvst[nbst].y = uvpti[n-1].y;
        }
      }
    }
  }
  nbst++;

  // generation du tableau 'nsef' de la surface triangulee
  // -----------------------------------------------------
  // boucle sur les triangles occupes (internes et externes)
  if( nust != NULL ) delete [] nust;
  nust = new Z[nbsttria*nbt];
  if( nust == NULL ) goto ERREUR;
  nbt = 0;
  for (i=1; i<=mxartr; i++)
  {
    //le triangle i de mnartr
    if( mnartr[i*moartr-moartr] != 0 )
    {
      //le triangle i est interne => nosotr numero de ses 3 sommets
      nusotr( i, mosoar, mnsoar, moartr, mnartr,  nosotr );
      nust[nbt++] = mnarst[ nosotr[0] ];
      nust[nbt++] = mnarst[ nosotr[1] ];
      nust[nbt++] = mnarst[ nosotr[2] ];
      nust[nbt++] = 0;
    }
  }
  nbt /= nbsttria;  //le nombre final de triangles de la surface
  MESSAGE( "APTRTE: Fin de la triangulation plane avec "<<nbst<<" sommets et "
           << nbt << " triangles" );
  deltacpu_( d );
  tcpu += d;
  MESSAGE( "APTRTE: Temps total de la triangulation plane=" << tcpu << " secondes" );

  // destruction des tableaux auxiliaires
  // ------------------------------------
 NETTOYAGE:
  if( mnarst != NULL ) delete [] mnarst;
  if( mnartr != NULL ) delete [] mnartr;
  if( mnslig != NULL ) delete [] mnslig;
  if( mnsoar != NULL ) delete [] mnsoar;
  if( mnpxyd != NULL ) delete [] mnpxyd;
  if( mntree != NULL ) delete [] mntree;
  if( mnqueu != NULL ) delete [] mnqueu;
  if( mntrsu != NULL ) delete [] mntrsu;
  if( mnlftr != NULL ) delete [] mnlftr;
  if( mn1arcf != NULL ) delete [] mn1arcf;
  if( mnarcf  != NULL ) delete [] mnarcf;
  if( mnarcf1 != NULL ) delete [] mnarcf1;
  if( mnarcf2 != NULL ) delete [] mnarcf2;
  if( mnarcf3 != NULL ) delete [] mnarcf3;
  return;

 ERREUR:
  if( ierr == 51 || ierr == 52 )
  {
    //saturation des sommets => redepart avec 2 fois plus de sommets
    mxsomm = 2 * mxsomm;
    ierr   = 0;
    goto NEWDEPART;
  }
  else
  {
    MESSAGE( "APTRTE: Triangulation NON REALISEE  avec erreur=" << ierr );
    if( ierr == 0 ) ierr=1;
    goto NETTOYAGE;
  }
}
void
#ifdef WIN32
#ifdef F2C_BUILD
#else
              __stdcall
#endif
#endif
 qualitetrte( R_3 *mnpxyd,
                   Z & mosoar, Z & mxsoar, Z *mnsoar,
                   Z & moartr, Z & mxartr, Z *mnartr,
                   Z & nbtria, R & quamoy, R & quamin )
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// but :    calculer la qualite moyenne et minimale de la triangulation
// -----    actuelle definie par les tableaux mnsoar et mnartr
// entrees:
// --------
// mnpxyd : tableau des coordonnees 2d des points
//          par point : x  y  distance_souhaitee
// mosoar : nombre maximal d'entiers par arete et
//          indice dans mnsoar de l'arete suivante dans le hachage
// mxsoar : nombre maximal d'aretes stockables dans le tableau mnsoar
//          attention: mxsoar>3*mxsomm obligatoire!
// mnsoar : numero des 2 sommets , no ligne, 2 triangles de l'arete,
//          chainage des aretes frontalieres, chainage du hachage des aretes
//          hachage des aretes = mnsoar(1)+mnsoar(2)*2
//          avec mxsoar>=3*mxsomm
//          une arete i de mnsoar est vide <=> mnsoar(1,i)=0 et
//          mnsoar(2,arete vide)=l'arete vide qui precede
//          mnsoar(3,arete vide)=l'arete vide qui suit
// moartr : nombre maximal d'entiers par arete du tableau mnartr
// mxartr : nombre maximal de triangles declarables
// mnartr : les 3 aretes des triangles +-arete1, +-arete2, +-arete3
//          arete1 = 0 si triangle vide => arete2 = triangle vide suivant
// sorties:
// --------
// nbtria : nombre de triangles internes au domaine
// quamoy : qualite moyenne  des triangles actuels
// quamin : qualite minimale des triangles actuels
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
{
  R  d, aire, qualite;
  Z  nosotr[3], mn, nbtrianeg, nt, ntqmin;

  aire   = 0;
  quamoy = 0;
  quamin = 2.0;
  nbtria = 0;
  nbtrianeg = 0;
  ntqmin = 0;

  mn = -moartr;
  for ( nt=1; nt<=mxartr; nt++ )
  {
    mn += moartr;
    if( mnartr[mn]!=0 )
    {
      //un triangle occupe de plus
      nbtria++;

      //le numero des 3 sommets du triangle nt
      nusotr( nt, mosoar, mnsoar, moartr, mnartr,  nosotr );

      //la qualite du triangle ns1 ns2 ns3
      qutr2d( mnpxyd[nosotr[0]-1], mnpxyd[nosotr[1]-1], mnpxyd[nosotr[2]-1],
               qualite );

      //la qualite moyenne
      quamoy += qualite;

      //la qualite minimale
      if( qualite < quamin )
      {
         quamin = qualite;
         ntqmin = nt;
      }

      //aire signee du triangle nt
      d = surtd2( mnpxyd[nosotr[0]-1], mnpxyd[nosotr[1]-1], mnpxyd[nosotr[2]-1] );
      if( d<0 )
      {
        //un triangle d'aire negative de plus
        nbtrianeg++;
        MESSAGE("ATTENTION: le triangle " << nt << " de sommets:"
             << nosotr[0] << " " << nosotr[1] << " " << nosotr[2]
             << " a une aire " << d <<"<=0");
      }

      //aire des triangles actuels
      aire += Abs(d);
    }
  }

  //les affichages
  quamoy /= nbtria;
  MESSAGE("Qualite moyenne=" << quamoy
       << "  Qualite minimale=" << quamin
       << " des " << nbtria << " triangles de surface plane totale="
       << aire);

  if( quamin<0.3 )
  {
    //le numero des 3 sommets du triangle ntqmin de qualite minimale
    nusotr(ntqmin, mosoar, mnsoar, moartr, mnartr,  nosotr );
    MESSAGE("Triangle de qualite minimale "<<quamin<<" de sommets:"
            <<nosotr[0]<<" "<<nosotr[1]<<" "<<nosotr[2]<<" ");
    for (int i=0;i<3;i++)
      MESSAGE("Sommet "<<nosotr[i]<<": x="<< mnpxyd[nosotr[i]-1].x
              <<" y="<< mnpxyd[nosotr[i]-1].y);
  }

  if( nbtrianeg>0 )
    MESSAGE( "ATTENTION: "<< nbtrianeg << " TRIANGLES d'AIRE NEGATIVE" );

  MESSAGE(" ");
  return;
}
